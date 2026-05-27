#!/usr/bin/env python3

from __future__ import annotations

import argparse
import shutil
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parent.parent
WEB_FLASHER_DIR = PROJECT_ROOT / "web-flasher"


def copy_tree(src: Path, dest: Path) -> None:
    if dest.exists():
        shutil.rmtree(dest)
    shutil.copytree(src, dest, ignore=shutil.ignore_patterns(".gitkeep"))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--firmware-root", required=True, help="Directory containing firmware/<board-id>/ payloads")
    parser.add_argument("--output-root", required=True, help="Directory to assemble the publishable site into")
    parser.add_argument("--release-zips", help="Optional directory containing release zip files to publish alongside the site")
    args = parser.parse_args()

    firmware_root = Path(args.firmware_root).resolve()
    output_root = Path(args.output_root).resolve()
    output_root.mkdir(parents=True, exist_ok=True)

    for path in WEB_FLASHER_DIR.iterdir():
        if path.name == ".gitignore":
            continue
        target = output_root / path.name
        if path.is_dir():
            copy_tree(path, target)
        else:
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, target)

    target_firmware_dir = output_root / "firmware"
    copy_tree(firmware_root, target_firmware_dir)

    if args.release_zips:
        release_zips = Path(args.release_zips).resolve()
        if release_zips.exists():
            copy_tree(release_zips, output_root / "releases")


if __name__ == "__main__":
    main()
