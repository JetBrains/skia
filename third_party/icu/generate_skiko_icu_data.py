#!/usr/bin/env python3

import argparse
import os
from pathlib import Path
import platform
import shutil
import subprocess


SCRIPT_DIR = Path(__file__).resolve().parent
SKIA_ROOT = SCRIPT_DIR.parent.parent
ICU_ROOT = SKIA_ROOT / "third_party" / "externals" / "icu"


def parse_args():
    parser = argparse.ArgumentParser(
        description="Build a filtered ICU data package from Skia's pinned ICU."
    )
    parser.add_argument("--filter", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--build-dir", required=True, type=Path)
    return parser.parse_args()


def main():
    args = parse_args()
    filter_file = args.filter.resolve()
    output_file = args.output.resolve()
    build_dir = args.build_dir.resolve()
    configure = ICU_ROOT / "source" / "runConfigureICU"
    patch_locale = ICU_ROOT / "cast" / "patch_locale.sh"

    if not filter_file.is_file():
        raise SystemExit(f"Missing ICU data filter: {filter_file}")
    if not configure.is_file() or not os.access(configure, os.X_OK):
        raise SystemExit(f"Missing pinned ICU checkout at {ICU_ROOT}")
    if not patch_locale.is_file() or not os.access(patch_locale, os.X_OK):
        raise SystemExit(f"Missing Chromium ICU locale patch at {patch_locale}")
    if build_dir == Path(build_dir.anchor):
        raise SystemExit(f"Refusing to use {build_dir} as the build directory")

    if platform.system() != "Darwin":
        raise SystemExit("Filtered ICU data generation is supported on macOS only")

    shutil.rmtree(build_dir, ignore_errors=True)
    build_dir.mkdir(parents=True)
    output_file.parent.mkdir(parents=True, exist_ok=True)

    # Chromium's make_data_all.sh applies the Cast break-iterator patch before
    # generating its Android and iOS packages. We apply it to an isolated source
    # copy so the generated package matches that flow without modifying the
    # pinned ICU checkout.
    source_root = build_dir / "icu"
    shutil.copytree(ICU_ROOT / "source", source_root / "source")
    shutil.copytree(ICU_ROOT / "cast", source_root / "cast")
    subprocess.run(
        [str(source_root / "cast" / "patch_locale.sh")],
        cwd=source_root,
        check=True,
    )

    icu_build_dir = build_dir / "build"
    icu_build_dir.mkdir()
    configure = source_root / "source" / "runConfigureICU"

    env = os.environ.copy()
    env["ICU_DATA_FILTER_FILE"] = str(filter_file)

    subprocess.run(
        [str(configure), "MacOSX"]
        + [
            "--disable-tests",
            "--disable-samples",
            "--disable-layoutex",
            "--enable-rpath",
            "--prefix=" + str(build_dir / "install"),
        ],
        cwd=icu_build_dir,
        env=env,
        check=True,
    )
    subprocess.run(
        ["make", "-j", str(os.cpu_count() or 1)],
        cwd=icu_build_dir,
        check=True,
    )

    packages = list(
        (icu_build_dir / "data" / "out" / "tmp").glob("icudt*l.dat")
    )
    if len(packages) != 1:
        raise SystemExit(
            f"Expected one little-endian ICU package, found {len(packages)}"
        )

    shutil.copyfile(packages[0], output_file)
    print(f"{output_file}: {output_file.stat().st_size} bytes")


if __name__ == "__main__":
    main()
