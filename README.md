# JabulOS
This is the official source for developers and beta testers.

# How to compile the ISO
You must have WSL installed with APT, And you must install these packages

- sudo apt install gcc g++ make binutils nasm xorriso mtools 
- sudo apt install grub-pc-bin grub-efi-amd64-bin grub-common
- sudo apt install python3 python3-pil 
- sudo apt install qemu-system-x86 qemu-utils pulseaudio ffmpeg

And when you run wsl build iso in your project folder and it complains about missing the gnu ld tools, Just run
make iso LD=ld

