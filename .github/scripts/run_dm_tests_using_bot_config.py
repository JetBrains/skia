#!/usr/bin/env python3

import argparse
import json
import shlex
import subprocess
import sys


def parse_json_string_list(raw: str, flag_name: str) -> list[str]:
    try:
        value = json.loads(raw)
    except json.JSONDecodeError as exc:
        raise SystemExit(f"Invalid JSON passed to {flag_name}: {exc}") from exc

    if not isinstance(value, list) or not all(isinstance(item, str) for item in value):
        raise SystemExit(f"Expected {flag_name} to decode to a list of strings")
    return value


def dm_wrapper() -> list[str]:
    if sys.platform.startswith("linux"):
        return ["xvfb-run", "--auto-servernum"]
    if sys.platform == "darwin":
        return []
    raise SystemExit(f"Unsupported platform for DM test runner: {sys.platform}")


def build_command(configs: list[str], matches: list[str]) -> list[str]:
    wrapper = dm_wrapper()

    cmd = [*wrapper, "out/Debug/dm", "-v"]
    if configs:
        cmd.extend(["--config", *configs])
    cmd.extend(["--src", "tests"])
    if matches:
        cmd.extend(["--match", *matches])
    return cmd


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run DM unit tests using configs and matches passed from GitHub Actions.",
    )
    parser.add_argument(
        "--configs-json",
        required=True,
        help="JSON array of DM configs.",
    )
    parser.add_argument(
        "--matches-json",
        required=True,
        help="JSON array of DM --match patterns.",
    )
    parser.add_argument(
        "--print-command",
        action="store_true",
        help="Print the resolved DM command and exit.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    configs = parse_json_string_list(args.configs_json, "--configs-json")
    matches = parse_json_string_list(args.matches_json, "--matches-json")
    cmd = build_command(configs, matches)

    if args.print_command:
        print(shlex.join(cmd))
        return 0

    print("Using DM test args from GitHub Actions", file=sys.stderr)
    return subprocess.call(cmd)


if __name__ == "__main__":
    raise SystemExit(main())
