#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import zipfile
from pathlib import Path

from supported_boards import get_supported_board, get_supported_boards


PROJECT_ROOT = Path(__file__).resolve().parent.parent
WEB_FLASHER_DIR = PROJECT_ROOT / "web-flasher"


def get_project_version() -> str:
    cmake = (PROJECT_ROOT / "CMakeLists.txt").read_text(encoding="utf-8").splitlines()
    for line in cmake:
        if line.startswith('set(PROJECT_VER "'):
            return line.split('"')[1]
    raise RuntimeError("Failed to determine PROJECT_VER from CMakeLists.txt")


def build_dir_for(board: str) -> Path:
    return PROJECT_ROOT / f"build-{board.replace('/', '-')}"


def run(cmd: list[str], *, env: dict[str, str] | None = None) -> None:
    subprocess.run(cmd, cwd=PROJECT_ROOT, check=True, env=env)


def ensure_idf_available_env() -> dict[str, str]:
    env = os.environ.copy()
    if shutil.which("idf.py"):
        return env
    idf_export = Path(env.get("IDF_PATH", str(Path.home() / "esp" / "esp-idf"))) / "export.sh"
    command = (
        f'. "{idf_export}" >/dev/null 2>&1 && '
        'python3 - <<\'PY\'\n'
        'import os, json\n'
        'print(json.dumps(dict(os.environ)))\n'
        'PY'
    )
    result = subprocess.run(
        ["bash", "-lc", command],
        cwd=PROJECT_ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    return json.loads(result.stdout)


def ensure_built(board: str, do_build: bool) -> Path:
    build_dir = build_dir_for(board)
    if do_build:
        run(["bash", "./switch-board.sh", board, "build"])
    if not (build_dir / "xiaozhi.bin").exists():
        raise FileNotFoundError(f"Missing firmware image for {board}: {build_dir / 'xiaozhi.bin'}")
    return build_dir


def merge_binary(build_dir: Path) -> None:
    env = ensure_idf_available_env()
    run(
        [
            "idf.py",
            "-B",
            str(build_dir),
            f"-DSDKCONFIG={build_dir / 'sdkconfig'}",
            "merge-bin",
        ],
        env=env,
    )


def copy_file(src: Path, dest: Path) -> None:
    if not src.exists():
        raise FileNotFoundError(src)
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dest)


def package_board(entry: dict[str, object], output_root: Path, do_build: bool) -> None:
    board = str(entry["board"])
    build_dir = ensure_built(board, do_build)
    merge_binary(build_dir)

    firmware_dir = output_root / "firmware" / str(entry["flasher_board_id"])
    firmware_dir.mkdir(parents=True, exist_ok=True)

    copy_file(build_dir / "bootloader" / "bootloader.bin", firmware_dir / "bootloader.bin")
    copy_file(build_dir / "partition_table" / "partition-table.bin", firmware_dir / "partition-table.bin")
    copy_file(build_dir / "ota_data_initial.bin", firmware_dir / "ota_data_initial.bin")
    copy_file(build_dir / "xiaozhi.bin", firmware_dir / "xiaozhi.bin")
    copy_file(build_dir / "merged-binary.bin", firmware_dir / "merged-binary.bin")

    if bool(entry["include_assets"]) and (build_dir / "generated_assets.bin").exists():
        copy_file(build_dir / "generated_assets.bin", firmware_dir / "generated_assets.bin")

    metadata = {
        "board": entry["board"],
        "name": entry["name"],
        "full_name": entry["full_name"],
        "flasher_board_id": entry["flasher_board_id"],
        "manifest": entry["manifest"],
        "project_version": get_project_version(),
    }
    (firmware_dir / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")

    zip_dir = output_root / "release-zips"
    zip_dir.mkdir(parents=True, exist_ok=True)
    zip_path = zip_dir / f"v{metadata['project_version']}_{entry['full_name']}.zip"
    with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(firmware_dir.iterdir()):
            archive.write(path, arcname=path.name)


def write_firmware_index(output_root: Path, boards: list[dict[str, object]]) -> None:
    index_path = output_root / "firmware" / "index.json"
    index_path.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "project_version": get_project_version(),
        "boards": boards,
    }
    index_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--board", help="Supported board path, e.g. xiao-esp32-c6")
    parser.add_argument("--output-root", default="dist", help="Directory to write packaged artifacts into")
    parser.add_argument("--build", action="store_true", help="Build the selected board(s) before packaging")
    args = parser.parse_args()

    output_root = (PROJECT_ROOT / args.output_root).resolve()
    boards = get_supported_boards()
    if args.board:
        boards = [get_supported_board(args.board)]

    for entry in boards:
        package_board(entry, output_root, args.build)
    write_firmware_index(output_root, boards)


if __name__ == "__main__":
    main()
