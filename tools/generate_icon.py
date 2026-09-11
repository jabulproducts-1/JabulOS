#!/usr/bin/env python3
import os
import struct
import sys

try:
    from PIL import Image, ImageOps
except ImportError:
    Image = None
    ImageOps = None

ICON_SIZE = 128


def write_bmp(output_path: str, pixels, width: int, height: int) -> None:
    row_size = (width * 3 + 3) & ~3
    image_size = row_size * height
    file_size = 54 + image_size

    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "wb") as handle:
        handle.write(b"BM")
        handle.write(struct.pack("<IHHI", file_size, 0, 0, 54))
        handle.write(struct.pack("<IIIHHIIIIII",
                                 40,
                                 width,
                                 height,
                                 1,
                                 24,
                                 0,
                                 image_size,
                                 2835,
                                 2835,
                                 0,
                                 0))

        padding = b"\x00" * (row_size - width * 3)
        for y in range(height - 1, -1, -1):
            row = bytearray()
            for x in range(width):
                red, green, blue = pixels[x, y]
                row.extend((blue, green, red))
            row.extend(padding)
            handle.write(row)


def write_fallback_bmp(output_path: str) -> None:
    image = [[(40, 42, 48) for _ in range(ICON_SIZE)] for _ in range(ICON_SIZE)]

    for y in range(18, 102):
        for x in range(18, 110):
            image[y][x] = (213, 156, 58)
    for y in range(30, 42):
        for x in range(26, 56):
            image[y][x] = (235, 182, 84)

    class PixelView:
        def __getitem__(self, key):
            x, y = key
            return image[y][x]

    write_bmp(output_path, PixelView(), ICON_SIZE, ICON_SIZE)


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: generate_icon.py <output.bmp> <source-image>", file=sys.stderr)
        return 1

    output_path = sys.argv[1]
    source_path = sys.argv[2]

    if Image is not None and ImageOps is not None and os.path.exists(source_path):
        image = Image.open(source_path).convert("RGB")
        image = ImageOps.fit(image, (ICON_SIZE, ICON_SIZE), method=Image.LANCZOS, centering=(0.5, 0.5))
        write_bmp(output_path, image.load(), ICON_SIZE, ICON_SIZE)
        return 0

    if Image is None:
        print("warning: Pillow not available, using fallback icon art", file=sys.stderr)
        write_fallback_bmp(output_path)
        return 0

    print(f"error: source image not found: {source_path}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
