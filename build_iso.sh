#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if grep -qi microsoft /proc/version 2>/dev/null; then
    echo "[jabulOS Aero] WSL environment detected."
else
    echo "[jabulOS Aero] Warning: this script is intended for WSL Ubuntu, continuing anyway."
fi

sudo apt-get update
sudo apt-get install -y \
    build-essential \
    binutils-x86-64-linux-gnu \
    gcc-x86-64-linux-gnu \
    g++-x86-64-linux-gnu \
    nasm \
    xorriso \
    grub-pc-bin \
    grub-efi-amd64-bin \
    grub-common \
    mtools \
    ffmpeg \
    python3 \
    python3-pil \
    python3-opencv \
    make \
    tar

make -C "$SCRIPT_DIR" clean
make -C "$SCRIPT_DIR" iso

echo "[jabulOS Aero] ISO ready: $SCRIPT_DIR/JABULOS30K.iso"
