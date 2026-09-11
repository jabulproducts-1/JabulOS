import struct
import sys
from pathlib import Path


FILE_ALIGNMENT = 0x200
SECTION_ALIGNMENT = 0x1000
OPTIONAL_HEADER_SIZE = 0xF0
IMAGE_BASE = 0x0000000140000000
ENTRY_RVA = 0x1000
TEXT_RAW_OFFSET = 0x200


def align_up(value: int, alignment: int) -> int:
    return (value + alignment - 1) // alignment * alignment


def build_code() -> bytes:
    message = b"Hello from a JabulOS PE32+ executable!\0"
    code = bytearray()

    code += b"\xC7\x47\x08" + struct.pack("<I", 1)

    for index in range(0, len(message), 8):
        chunk = message[index:index + 8].ljust(8, b"\0")
        displacement = 0x10 + index
        code += b"\x48\xB8" + chunk
        code += b"\x48\x89\x47" + struct.pack("B", displacement)

    code += b"\xB8" + struct.pack("<I", 2026)
    code += b"\xC3"
    return bytes(code)


def build_optional_header(text_raw_size: int, text_virtual_size: int, size_of_image: int, size_of_headers: int) -> bytes:
    header = bytearray()
    header += struct.pack("<HBBIIIII", 0x20B, 1, 0, text_raw_size, 0, 0, ENTRY_RVA, ENTRY_RVA)
    header += struct.pack("<QII", IMAGE_BASE, SECTION_ALIGNMENT, FILE_ALIGNMENT)
    header += struct.pack("<HHHHHH", 6, 0, 0, 0, 6, 0)
    header += struct.pack("<IIIIHH", 0, size_of_image, size_of_headers, 0, 3, 0)
    header += struct.pack("<QQQQ", 0x100000, 0x1000, 0x100000, 0x1000)
    header += struct.pack("<II", 0, 16)
    header += b"\0" * (16 * 8)
    if len(header) != OPTIONAL_HEADER_SIZE:
        raise ValueError(f"optional header size mismatch: {len(header)}")
    return bytes(header)


def build_pe_image() -> bytes:
    text = build_code()
    text_raw_size = align_up(len(text), FILE_ALIGNMENT)
    size_of_headers = FILE_ALIGNMENT
    size_of_image = SECTION_ALIGNMENT + align_up(len(text), SECTION_ALIGNMENT)

    dos_header = bytearray(0x80)
    dos_header[0:2] = b"MZ"
    dos_header[0x3C:0x40] = struct.pack("<I", 0x80)

    coff_header = struct.pack(
        "<HHIIIHH",
        0x8664,
        1,
        0,
        0,
        0,
        OPTIONAL_HEADER_SIZE,
        0x0022,
    )
    optional_header = build_optional_header(text_raw_size, len(text), size_of_image, size_of_headers)
    section_header = struct.pack(
        "<8sIIIIIIHHI",
        b".text\0\0\0",
        len(text),
        ENTRY_RVA,
        text_raw_size,
        TEXT_RAW_OFFSET,
        0,
        0,
        0,
        0,
        0x60000020,
    )

    headers = bytes(dos_header) + b"PE\0\0" + coff_header + optional_header + section_header
    headers = headers.ljust(size_of_headers, b"\0")
    section_data = text.ljust(text_raw_size, b"\x90")
    return headers + section_data


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: generate_pe_sample.py <output-path>")
        return 1

    output_path = Path(sys.argv[1])
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(build_pe_image())
    print(f"wrote {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
