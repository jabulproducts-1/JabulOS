#!/usr/bin/env python3
import os
import struct
import sys

try:
    from PIL import Image
except ImportError:
    Image = None

WIDTH = 1024
HEIGHT = 768

if len(sys.argv) >= 5:
    WIDTH = int(sys.argv[4])
if len(sys.argv) >= 6:
    HEIGHT = int(sys.argv[5])
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
SOURCE_WALLPAPER = os.path.join(PROJECT_DIR, "assets", "main-wallpaper.png")
SOURCE_BOOT_SPLASH = os.path.join(PROJECT_DIR, "assets", "boot-splash.jpg")


def clamp(value: int) -> int:
    return max(0, min(255, value))


def pixel(x: int, y: int):
    fx = x / (WIDTH - 1)
    fy = y / (HEIGHT - 1)

    red = int(26 + 84 * fx)
    green = int(58 + 120 * (1.0 - fy))
    blue = int(96 + 130 * fy)

    glow_x = x - WIDTH * 0.72
    glow_y = y - HEIGHT * 0.30
    glow = max(0.0, 1.0 - ((glow_x * glow_x + glow_y * glow_y) / (WIDTH * HEIGHT * 0.10)))

    red = clamp(int(red + 90 * glow))
    green = clamp(int(green + 40 * glow))
    blue = clamp(int(blue + 100 * glow))
    return blue, green, red


def cover_resize(image):
    src_width, src_height = image.size
    if src_width == 0 or src_height == 0:
        return image.resize((WIDTH, HEIGHT))

    scale = max(WIDTH / src_width, HEIGHT / src_height)
    scaled_width = max(WIDTH, int(round(src_width * scale)))
    scaled_height = max(HEIGHT, int(round(src_height * scale)))
    resized = image.resize((scaled_width, scaled_height), Image.LANCZOS)
    left = max(0, (scaled_width - WIDTH) // 2)
    top = max(0, (scaled_height - HEIGHT) // 2)
    return resized.crop((left, top, left + WIDTH, top + HEIGHT))


def contain_resize(image):
    src_width, src_height = image.size
    if src_width == 0 or src_height == 0:
        return Image.new("RGB", (WIDTH, HEIGHT), (0, 0, 0))

    scale = min(WIDTH / src_width, HEIGHT / src_height)
    scaled_width = max(1, int(round(src_width * scale)))
    scaled_height = max(1, int(round(src_height * scale)))
    resized = image.resize((scaled_width, scaled_height), Image.LANCZOS)
    canvas = Image.new("RGB", (WIDTH, HEIGHT), (0, 0, 0))
    left = max(0, (WIDTH - scaled_width) // 2)
    top = max(0, (HEIGHT - scaled_height) // 2)
    canvas.paste(resized, (left, top))
    return canvas


def write_bmp_image(image, output_path: str):
    row_size = (WIDTH * 3 + 3) & ~3
    image_size = row_size * HEIGHT
    file_size = 54 + image_size

    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "wb") as handle:
        handle.write(b"BM")
        handle.write(struct.pack("<IHHI", file_size, 0, 0, 54))
        handle.write(struct.pack("<IIIHHIIIIII",
                                 40,
                                 WIDTH,
                                 HEIGHT,
                                 1,
                                 24,
                                 0,
                                 image_size,
                                 2835,
                                 2835,
                                 0,
                                 0))

        padding = b"\x00" * (row_size - WIDTH * 3)
        pixels = image.load()
        for y in range(HEIGHT):
            source_y = HEIGHT - 1 - y
            row = bytearray()
            for x in range(WIDTH):
                red, green, blue = pixels[x, source_y]
                row.extend((blue, green, red))
            row.extend(padding)
            handle.write(row)


def write_bmp_from_source(source_path: str, output_path: str, mode: str):
    image = Image.open(source_path).convert("RGB")
    if mode == "contain":
        image = contain_resize(image)
    else:
        image = cover_resize(image)
    write_bmp_image(image, output_path)


def write_bmp(path: str):
    row_size = (WIDTH * 3 + 3) & ~3
    image_size = row_size * HEIGHT
    file_size = 54 + image_size

    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as handle:
        handle.write(b"BM")
        handle.write(struct.pack("<IHHI", file_size, 0, 0, 54))
        handle.write(struct.pack("<IIIHHIIIIII",
                                 40,
                                 WIDTH,
                                 HEIGHT,
                                 1,
                                 24,
                                 0,
                                 image_size,
                                 2835,
                                 2835,
                                 0,
                                 0))

        padding = b"\x00" * (row_size - WIDTH * 3)
        for y in range(HEIGHT):
            source_y = HEIGHT - 1 - y
            row = bytearray()
            for x in range(WIDTH):
                row.extend(pixel(x, source_y))
            row.extend(padding)
            handle.write(row)


if __name__ == "__main__":
    if len(sys.argv) not in (2, 3, 4, 6):
        print("usage: generate_wallpaper.py <output.bmp> [source.png] [cover|contain] [width] [height]", file=sys.stderr)
        sys.exit(1)

    output_path = sys.argv[1]
    source_path = sys.argv[2] if len(sys.argv) >= 3 else SOURCE_WALLPAPER
    mode = sys.argv[3] if len(sys.argv) >= 4 else "cover"

    if Image is not None and os.path.exists(source_path):
        write_bmp_from_source(source_path, output_path, mode)
    else:
        write_bmp(output_path)
