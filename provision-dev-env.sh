echo attempting to setup jabul so dev tools please wait patiently

if grep -qi microsoft /proc/version 2>/dev/null; then
    echo "[jabulOS Aero] WSL environment detected."
else
    echo "[jabulOS Aero] Warning: this script is intended for WSL Ubuntu, continuing anyway."
fi


sudo apt-get update
sudo apt-get install -y \
    git \
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

mkdir jabul-dev

cd jabul-dev

gh repo clone 2cookie/JabulOS

echo cloned repo!

