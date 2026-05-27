#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parent.parent


SUPPORTED_BOARDS = [
    {
        "board": "xiao-esp32-c3",
        "name": "xiao-esp32-c3",
        "full_name": "xiao-esp32-c3",
        "flasher_board_id": "c3",
        "manifest": "manifest-c3.json",
        "include_assets": False,
    },
    {
        "board": "xiao-esp32-c6",
        "name": "xiao-esp32-c6",
        "full_name": "xiao-esp32-c6",
        "flasher_board_id": "c6",
        "manifest": "manifest-c6.json",
        "include_assets": False,
    },
    {
        "board": "xiao-esp32-s3-sense",
        "name": "xiao-esp32-s3-sense",
        "full_name": "xiao-esp32-s3-sense",
        "flasher_board_id": "s3",
        "manifest": "manifest-s3.json",
        "include_assets": True,
    },
    {
        "board": "waveshare/esp32-s3-touch-amoled-1.8",
        "name": "esp32-s3-touch-amoled-1.8",
        "full_name": "waveshare-esp32-s3-touch-amoled-1.8",
        "flasher_board_id": "waveshare-s3-amoled18",
        "manifest": "manifest-waveshare-s3-amoled18.json",
        "include_assets": True,
    },
]


def get_supported_boards() -> list[dict[str, object]]:
    return [dict(entry) for entry in SUPPORTED_BOARDS]


def get_supported_board(board: str) -> dict[str, object]:
    for entry in SUPPORTED_BOARDS:
        if entry["board"] == board:
            return dict(entry)
    raise KeyError(f"Unsupported board: {board}")


def _changed_files(base: str, head: str) -> list[str]:
    result = subprocess.run(
        ["git", "diff", "--name-only", base, head],
        cwd=PROJECT_ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    return [line.strip() for line in result.stdout.splitlines() if line.strip()]


def _requires_full_supported_matrix(path: str) -> bool:
    if path.startswith("main/boards/common/"):
        return True
    if not path.startswith("main/boards/"):
        return True
    if path in {
        "main/CMakeLists.txt",
        "main/Kconfig.projbuild",
        "CMakeLists.txt",
        "switch-board.sh",
        "sdkconfig.defaults",
        "sdkconfig.defaults.esp32c3",
        "sdkconfig.defaults.esp32c6",
        "sdkconfig.defaults.esp32s3",
    }:
        return True
    return False


def select_supported_boards_from_changes(base: str, head: str) -> list[dict[str, object]]:
    changed = _changed_files(base, head)
    if not changed:
        return []

    if any(_requires_full_supported_matrix(path) for path in changed):
        return get_supported_boards()

    selected: list[dict[str, object]] = []
    for entry in SUPPORTED_BOARDS:
        prefix = f"main/boards/{entry['board']}/"
        if any(path.startswith(prefix) for path in changed):
            selected.append(dict(entry))
    return selected


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--json", action="store_true", help="Print supported boards as JSON")
    parser.add_argument("--changed-from", help="Git base ref/SHA for change-based board selection")
    parser.add_argument("--changed-to", help="Git head ref/SHA for change-based board selection")
    args = parser.parse_args()

    boards = get_supported_boards()
    if args.changed_from or args.changed_to:
        if not args.changed_from or not args.changed_to:
            raise SystemExit("--changed-from and --changed-to must be used together")
        boards = select_supported_boards_from_changes(args.changed_from, args.changed_to)

    if args.json:
        print(json.dumps(boards))
        return

    for entry in boards:
        print(f"{entry['board']} -> {entry['full_name']}")


if __name__ == "__main__":
    main()
