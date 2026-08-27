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


def shell_path(path):
    if platform.system() == "Windows":
        return subprocess.check_output(
            ["cygpath", "--unix", str(path)], text=True
        ).strip()
    return Path(path).as_posix()


def parse_args():
    parser = argparse.ArgumentParser(
        description="Build a filtered ICU data package from Skia's pinned ICU."
    )
    parser.add_argument("--filter", required=True, type=Path)
    parser.add_argument("--filter-patch", required=True, type=Path)
    parser.add_argument("--apply-cast-patch", action="store_true")
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--build-dir", required=True, type=Path)
    return parser.parse_args()


def main():
    args = parse_args()
    source_filter = args.filter.resolve()
    filter_patch = args.filter_patch.resolve()
    output_file = args.output.resolve()
    build_dir = args.build_dir.resolve()
    configure = ICU_ROOT / "source" / "runConfigureICU"
    patch_locale = ICU_ROOT / "cast" / "patch_locale.sh"

    if not source_filter.is_file():
        raise SystemExit(f"Missing ICU data filter: {source_filter}")
    if not filter_patch.is_file():
        raise SystemExit(f"Missing ICU data filter patch: {filter_patch}")
    if not configure.is_file():
        raise SystemExit(f"Missing pinned ICU checkout at {ICU_ROOT}")
    if args.apply_cast_patch and not patch_locale.is_file():
        raise SystemExit(f"Missing Chromium ICU locale patch at {patch_locale}")
    if build_dir == Path(build_dir.anchor):
        raise SystemExit(f"Refusing to use {build_dir} as the build directory")

    host_system = platform.system()
    configure_platform = {
        "Darwin": "MacOSX",
        "Linux": "Linux/gcc",
        "Windows": "MinGW",
    }.get(host_system)
    if configure_platform is None:
        raise SystemExit(
            f"Filtered ICU data generation is not supported on {host_system}"
        )

    # On Windows, use the MSYS Bash that launched the build. A bare `bash`
    # can resolve to the WSL launcher instead.
    bash = os.environ.get("SHELL", "bash") if host_system == "Windows" else "bash"

    shutil.rmtree(build_dir, ignore_errors=True)
    build_dir.mkdir(parents=True)
    output_file.parent.mkdir(parents=True, exist_ok=True)

    filter_file = build_dir / "filter.json"
    shutil.copyfile(source_filter, filter_file)
    subprocess.run(
        ["patch", "--batch", str(filter_file), str(filter_patch)],
        cwd=build_dir,
        check=True,
    )

    # Chromium's make_data_all.sh applies the Cast break-iterator patch before
    # generating its Android and iOS packages. We apply it to an isolated source
    # copy so the generated package matches that flow without modifying the
    # pinned ICU checkout.
    source_root = build_dir / "icu"
    shutil.copytree(ICU_ROOT / "source", source_root / "source")
    if args.apply_cast_patch:
        shutil.copytree(ICU_ROOT / "cast", source_root / "cast")
        subprocess.run(
            [bash, shell_path(source_root / "cast" / "patch_locale.sh")],
            cwd=source_root,
            check=True,
        )

    icu_build_dir = build_dir / "build"
    icu_build_dir.mkdir()
    configure = source_root / "source" / "runConfigureICU"

    env = os.environ.copy()
    env["ICU_DATA_FILTER_FILE"] = shell_path(filter_file)
    configure_args = []
    if host_system == "Windows":
        msys_root = Path(bash).parents[2]
        clang_bin = msys_root / "clang64" / "bin"
        env["CC"] = shell_path(clang_bin / "clang.exe")
        env["CXX"] = shell_path(clang_bin / "clang++.exe")
        env["PATH"] = str(clang_bin) + os.pathsep + env["PATH"]
        # ICU source data is UTF-8, while Windows otherwise uses its system
        # code page when tools such as genrb read it.
        env["CPPFLAGS"] = (
            env.get("CPPFLAGS", "") + " -DU_CHARSET_IS_UTF8=1"
        ).strip()
        configure_args = [
            "--build=x86_64-w64-mingw32",
            "--host=x86_64-w64-mingw32",
        ]

    subprocess.run(
        [bash, shell_path(configure), configure_platform]
        + configure_args
        + [
            "--disable-tests",
            "--disable-samples",
            "--disable-layoutex",
            "--enable-rpath",
            "--prefix=" + shell_path(build_dir / "install"),
        ],
        cwd=icu_build_dir,
        env=env,
        check=True,
    )
    subprocess.run(
        ["make", "-j", str(os.cpu_count() or 1)],
        cwd=icu_build_dir,
        env=env,
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
