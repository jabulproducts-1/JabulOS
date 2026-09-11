#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path


UPDATE_MAGIC = b"JBAUPD1\x00"
SECTOR_SIZE = 512
HEADER_STRUCT = struct.Struct("<8sIIIII48s436s")
PROJECT_DIR = Path(__file__).resolve().parent.parent
DEFAULT_INBOX = PROJECT_DIR / "updates" / "inbox"
DEFAULT_ARCHIVE = PROJECT_DIR / "updates" / "archive"
DEFAULT_STATE = PROJECT_DIR / "build" / "update-service" / "state.json"
DEFAULT_DISK = PROJECT_DIR / "build" / "disk.img"
DEFAULT_REBUILD_COMMAND = "./build_iso.sh"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Stage JabulOS ISO updates into the VM disk image.")
    subparsers = parser.add_subparsers(dest="command", required=True)

    common = argparse.ArgumentParser(add_help=False)
    common.add_argument("--disk", type=Path, default=DEFAULT_DISK, help="Target disk image.")
    common.add_argument("--archive", type=Path, default=DEFAULT_ARCHIVE, help="Processed ISO archive directory.")
    common.add_argument("--state", type=Path, default=DEFAULT_STATE, help="State file path.")
    common.add_argument("--rebuild-command", default=DEFAULT_REBUILD_COMMAND, help="Optional command to rebuild the project ISO.")
    common.add_argument("--skip-rebuild", action="store_true", help="Do not run the rebuild command after staging an update.")

    stage = subparsers.add_parser("stage", parents=[common], help="Stage a single ISO immediately.")
    stage.add_argument("--iso", type=Path, required=True, help="ISO file to import.")

    watch = subparsers.add_parser("watch", parents=[common], help="Watch an inbox folder for imported ISOs.")
    watch.add_argument("--inbox", type=Path, default=DEFAULT_INBOX, help="Folder to watch for new ISO files.")
    watch.add_argument("--poll-seconds", type=float, default=2.0, help="Polling interval in seconds.")

    return parser.parse_args()


def ensure_parent(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)


def ensure_directory(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def load_state(path: Path) -> dict:
    if not path.exists():
        return {"version": 0, "last_staged_sha256": ""}
    return json.loads(path.read_text(encoding="utf-8"))


def save_state(path: Path, state: dict) -> None:
    ensure_parent(path)
    path.write_text(json.dumps(state, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        while True:
            chunk = handle.read(1024 * 1024)
            if not chunk:
                break
            digest.update(chunk)
    return digest.hexdigest()


def sanitize_label(name: str) -> str:
    sanitized = "".join(ch if 32 <= ord(ch) <= 126 else "_" for ch in name).strip()
    return sanitized[:47] or "JabulOS update"


def extract_iso_payload(iso_path: Path) -> tuple[bytes, bytes]:
    with tempfile.TemporaryDirectory(prefix="jabul_update_") as temp_dir:
        temp_path = Path(temp_dir)
        kernel_path = temp_path / "kernel.elf"
        initrd_path = temp_path / "initrd.tar"

        command = [
            "xorriso",
            "-osirrox",
            "on",
            "-indev",
            str(iso_path),
            "-extract",
            "/boot/kernel.elf",
            str(kernel_path),
            "-extract",
            "/boot/initrd/initrd.tar",
            str(initrd_path),
        ]
        try:
            subprocess.run(command, cwd=PROJECT_DIR, check=True, capture_output=True, text=True)
        except FileNotFoundError as exc:
            raise RuntimeError("xorriso is required in WSL/Linux PATH to import update ISOs.") from exc
        except subprocess.CalledProcessError as exc:
            raise RuntimeError(exc.stderr.strip() or exc.stdout.strip() or "failed to extract ISO contents") from exc

        return kernel_path.read_bytes(), initrd_path.read_bytes()


def write_blob(handle, start_lba: int, data: bytes) -> None:
    sectors = (len(data) + SECTOR_SIZE - 1) // SECTOR_SIZE
    padded = data + (b"\x00" * (sectors * SECTOR_SIZE - len(data)))
    handle.seek(start_lba * SECTOR_SIZE)
    handle.write(padded)


def stage_iso_into_disk(disk_path: Path, iso_path: Path, version: int, label: str) -> dict:
    kernel_data, initrd_data = extract_iso_payload(iso_path)

    if not disk_path.exists():
        raise RuntimeError(f"disk image not found: {disk_path}")

    disk_size = disk_path.stat().st_size
    if disk_size < SECTOR_SIZE * 8:
        raise RuntimeError("disk image is too small for update staging")

    total_sectors = disk_size // SECTOR_SIZE
    header_lba = total_sectors - 1
    kernel_sectors = (len(kernel_data) + SECTOR_SIZE - 1) // SECTOR_SIZE
    initrd_sectors = (len(initrd_data) + SECTOR_SIZE - 1) // SECTOR_SIZE
    kernel_lba = header_lba - initrd_sectors - kernel_sectors
    initrd_lba = kernel_lba + kernel_sectors

    if kernel_lba <= 0:
        raise RuntimeError("disk image does not have enough free space for the staged update")

    label_bytes = sanitize_label(label).encode("ascii", "replace")[:47]
    label_bytes = label_bytes.ljust(48, b"\x00")
    header = HEADER_STRUCT.pack(
        UPDATE_MAGIC,
        version,
        kernel_lba,
        len(kernel_data),
        initrd_lba,
        len(initrd_data),
        label_bytes,
        b"\x00" * 436,
    )

    with disk_path.open("r+b") as handle:
        write_blob(handle, kernel_lba, kernel_data)
        write_blob(handle, initrd_lba, initrd_data)
        handle.seek(header_lba * SECTOR_SIZE)
        handle.write(header)
        handle.flush()

    return {
        "version": version,
        "label": sanitize_label(label),
        "kernel_lba": kernel_lba,
        "kernel_size": len(kernel_data),
        "initrd_lba": initrd_lba,
        "initrd_size": len(initrd_data),
    }


def maybe_rebuild_iso(command: str, skip_rebuild: bool) -> None:
    if skip_rebuild or not command:
        return
    subprocess.run(command, cwd=PROJECT_DIR, shell=True, check=True)


def archive_iso(iso_path: Path, archive_dir: Path) -> Path:
    ensure_directory(archive_dir)
    destination = archive_dir / iso_path.name
    if destination.exists():
        timestamp = time.strftime("%Y%m%d-%H%M%S")
        destination = archive_dir / f"{iso_path.stem}-{timestamp}{iso_path.suffix}"
    shutil.move(str(iso_path), str(destination))
    return destination


def process_iso(iso_path: Path, disk_path: Path, archive_dir: Path, state_path: Path, rebuild_command: str, skip_rebuild: bool) -> dict:
    state = load_state(state_path)
    iso_hash = sha256_file(iso_path)

    if iso_hash == state.get("last_staged_sha256"):
        archived = archive_iso(iso_path, archive_dir)
        return {"status": "duplicate", "archived_to": str(archived)}

    version = int(state.get("version", 0)) + 1
    staged = stage_iso_into_disk(disk_path, iso_path, version, iso_path.stem)
    maybe_rebuild_iso(rebuild_command, skip_rebuild)
    archived = archive_iso(iso_path, archive_dir)

    state.update(
        {
            "version": version,
            "last_staged_sha256": iso_hash,
            "last_label": staged["label"],
            "last_archived_iso": str(archived),
            "last_staged_at": time.strftime("%Y-%m-%d %H:%M:%S"),
        }
    )
    save_state(state_path, state)
    staged["status"] = "staged"
    staged["archived_to"] = str(archived)
    return staged


def newest_iso(inbox: Path) -> Path | None:
    ensure_directory(inbox)
    candidates = sorted((path for path in inbox.glob("*.iso") if path.is_file()), key=lambda path: path.stat().st_mtime)
    return candidates[0] if candidates else None


def run_watch_loop(args: argparse.Namespace) -> int:
    ensure_directory(args.inbox)
    ensure_directory(args.archive)
    ensure_parent(args.state)

    print(f"[update-service] watching {args.inbox}")
    while True:
        iso_path = newest_iso(args.inbox)
        if iso_path is not None:
            try:
                result = process_iso(iso_path, args.disk, args.archive, args.state, args.rebuild_command, args.skip_rebuild)
                print(f"[update-service] {result['status']}: {iso_path.name}")
            except Exception as exc:  # noqa: BLE001
                print(f"[update-service] failed: {iso_path.name}: {exc}", file=sys.stderr)
        time.sleep(args.poll_seconds)


def run_stage(args: argparse.Namespace) -> int:
    result = process_iso(args.iso, args.disk, args.archive, args.state, args.rebuild_command, args.skip_rebuild)
    print(json.dumps(result, indent=2))
    return 0


def main() -> int:
    args = parse_args()
    try:
        if args.command == "watch":
            return run_watch_loop(args)
        if args.command == "stage":
            return run_stage(args)
        raise RuntimeError(f"unsupported command: {args.command}")
    except Exception as exc:  # noqa: BLE001
        print(f"update-service error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
