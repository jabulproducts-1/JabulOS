#!/usr/bin/env python3
import argparse
import os
import pathlib
import shutil
import struct
import subprocess
import sys


MAGIC = b"JBAUD01\0"
HEADER = struct.Struct("<8sIII")
DEFAULT_SAMPLE_RATE = 16000


def build_output_path(source: pathlib.Path, source_root: pathlib.Path, output_root: pathlib.Path) -> pathlib.Path:
    relative = source.relative_to(source_root)
    return output_root / relative.with_suffix(".jba")


def command_path_for_host(path: pathlib.Path, executable: str) -> str:
    if os.name == "nt" or not executable.lower().endswith(".exe"):
        return str(path)

    result = subprocess.run(["wslpath", "-w", str(path)], capture_output=True, text=True)
    if result.returncode == 0:
        windows_path = result.stdout.strip()
        if windows_path:
            return windows_path
    return str(path)


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert an audio asset into JabulOS speaker PCM.")
    parser.add_argument("source")
    parser.add_argument("source_root")
    parser.add_argument("output_root")
    parser.add_argument("--sample-rate", type=int, default=DEFAULT_SAMPLE_RATE)
    args = parser.parse_args()

    source = pathlib.Path(args.source).resolve()
    source_root = pathlib.Path(args.source_root).resolve()
    output_root = pathlib.Path(args.output_root).resolve()
    output_path = build_output_path(source, source_root, output_root)

    if not source.is_file():
        print(f"audio preprocess: source not found: {source}", file=sys.stderr)
        return 1

    ffmpeg = os.environ.get("FFMPEG") or shutil.which("ffmpeg") or shutil.which("ffmpeg.exe")
    if ffmpeg is None:
        print("audio preprocess: ffmpeg is required for MP3/WAV conversion but was not found in PATH", file=sys.stderr)
        return 1

    output_path.parent.mkdir(parents=True, exist_ok=True)

    command = [
        ffmpeg,
        "-v",
        "error",
        "-y",
        "-i",
        command_path_for_host(source, ffmpeg),
        "-vn",
        "-ac",
        "1",
        "-ar",
        str(args.sample_rate),
        "-f",
        "u8",
        "-",
    ]
    result = subprocess.run(command, capture_output=True)
    if result.returncode != 0:
        sys.stderr.buffer.write(result.stderr)
        return result.returncode

    sample_count = len(result.stdout)
    if sample_count == 0:
        print(f"audio preprocess: no PCM data generated for {source}", file=sys.stderr)
        return 1

    with output_path.open("wb") as handle:
        handle.write(HEADER.pack(MAGIC, args.sample_rate, sample_count, 0))
        handle.write(result.stdout)

    print(f"audio preprocess: {source} -> {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
