#!/usr/bin/env python3
"""Create / patch a PS4 param.sfo, including the PS4 Pro (Neo) ATTRIBUTE bit.

SFO layout (little endian):
  header : magic 0x00505346 ("\\0PSF"), version 0x00000101,
           key_table_start, data_table_start, tables_entries
  index  : 16 bytes/entry: key_offset u16, param_fmt u16, param_len u32,
           param_max_len u32, data_offset u32
  keys   : NUL-terminated names, alphabetically ordered
  data   : 0x0404 = uint32, 0x0204 = NUL-terminated UTF-8

ATTRIBUTE bit 23 (0x00800000) enables PS4 Pro / Neo mode.
"""
import argparse
import struct
import sys

MAGIC = 0x46535000  # b"\x00PSF" as little-endian u32
VERSION = 0x00000101
FMT_INT = 0x0404
FMT_STR = 0x0204
NEO_BIT = 1 << 23

DEFAULT_ENTRIES = [
    ("APP_TYPE", FMT_INT, 1, 4),
    ("APP_VER", FMT_STR, "01.00", 8),
    ("ATTRIBUTE", FMT_INT, 0, 4),
    ("CATEGORY", FMT_STR, "gd", 4),
    ("CONTENT_ID", FMT_STR, "EP0000-LBTY00001_00-0000000000000000", 48),
    ("DOWNLOAD_DATA_SIZE", FMT_INT, 0, 4),
    ("SYSTEM_VER", FMT_INT, 0, 4),
    ("TITLE", FMT_STR, "Grand Theft Auto IV", 128),
    ("TITLE_ID", FMT_STR, "LBTY00001", 12),
    ("VERSION", FMT_STR, "01.00", 8),
]


def _encode(value, fmt, max_len):
    if fmt == FMT_INT:
        return struct.pack("<I", int(value) & 0xFFFFFFFF)
    raw = str(value).encode("utf-8")
    if len(raw) + 1 > max_len:
        raw = raw[: max_len - 1]
    return raw + b"\x00" * (max_len - len(raw))


def build(entries):
    entries = sorted(entries, key=lambda e: e[0])
    key_table, key_offsets = bytearray(), {}
    for name, *_ in entries:
        key_offsets[name] = len(key_table)
        key_table += name.encode("ascii") + b"\x00"
    while len(key_table) % 4:
        key_table += b"\x00"

    data, index = bytearray(), bytearray()
    for name, fmt, value, max_len in entries:
        payload = _encode(value, fmt, max_len)
        used = 4 if fmt == FMT_INT else len(str(value).encode("utf-8")) + 1
        used = min(used, max_len)
        index += struct.pack("<HHIII", key_offsets[name], fmt, used,
                             max_len, len(data))
        data += payload
    while len(data) % 4:
        data += b"\x00"

    key_start = 20 + 16 * len(entries)
    data_start = key_start + len(key_table)
    header = struct.pack("<IIIII", MAGIC, VERSION, key_start, data_start,
                         len(entries))
    return bytes(header + index + key_table + data)


def parse(buf):
    if len(buf) < 20 or struct.unpack_from("<I", buf, 0)[0] != MAGIC:
        raise ValueError("not a PSF/SFO file")
    _, _, key_start, data_start, count = struct.unpack_from("<IIIII", buf, 0)
    out = []
    for i in range(count):
        off = 20 + 16 * i
        key_off, fmt, used, max_len, data_off = struct.unpack_from(
            "<HHIII", buf, off)
        end = buf.index(0, key_start + key_off)
        name = buf[key_start + key_off:end].decode("ascii", "replace")
        out.append([name, fmt, used, max_len, data_off])
    return key_start, data_start, out


def patch(path, attribute):
    try:
        buf = bytearray(open(path, "rb").read())
        key_start, data_start, entries = parse(buf)
    except Exception:
        return False
    for name, fmt, used, max_len, data_off in entries:
        if name != "ATTRIBUTE":
            continue
        if fmt != FMT_INT or used < 4:
            return False
        pos = data_start + data_off
        current = struct.unpack_from("<I", buf, pos)[0]
        struct.pack_into("<I", buf, pos, (current | int(attribute)) & 0xFFFFFFFF)
        open(path, "wb").write(buf)
        return True
    return False


def inspect(path):
    buf = open(path, "rb").read()
    _, data_start, entries = parse(buf)
    for name, fmt, used, max_len, data_off in entries:
        if name == "ATTRIBUTE":
            value = struct.unpack_from("<I", buf, data_start + data_off)[0]
            print(f"ATTRIBUTE = 0x{value:08X} ({value}); Neo bit 23 = "
                  f"{'SET' if value & NEO_BIT else 'clear'}")
            return 0
    print("ATTRIBUTE entry not found", file=sys.stderr)
    return 1


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name in ("create", "patch"):
        p = sub.add_parser(name)
        p.add_argument("path")
        p.add_argument("--attribute", type=lambda x: int(x, 0), default=0)
        p.add_argument("--title-id", default="LBTY00001")
        p.add_argument("--content-id", default="EP0000-LBTY00001_00-0000000000000000")
        p.add_argument("--title", default="Grand Theft Auto IV")
        p.add_argument("--version", default="01.00")
        p.add_argument("--category", default="gd")
    p = sub.add_parser("inspect")
    p.add_argument("path")
    a = ap.parse_args()

    if a.cmd == "inspect":
        return inspect(a.path)

    if a.cmd == "patch":
        # If the file produced by make_param_sfo.py / PkgTool.Core is valid,
        # just OR the bits into ATTRIBUTE. Otherwise regenerate a full SFO.
        if patch(a.path, a.attribute):
            print(f"patched {a.path}: ATTRIBUTE |= 0x{int(a.attribute):08X}")
            return 0

    entries = [
        (name, fmt,
         int(a.attribute) if name == "ATTRIBUTE" else value,
         max_len)
        for name, fmt, value, max_len in DEFAULT_ENTRIES
    ]
    for name, fmt, value, max_len in list(entries):
        if name == "TITLE_ID":
            entries[entries.index((name, fmt, value, max_len))] = (name, fmt, a.title_id, max_len)
        elif name == "CONTENT_ID":
            entries[entries.index((name, fmt, value, max_len))] = (name, fmt, a.content_id, max_len)
        elif name == "TITLE":
            entries[entries.index((name, fmt, value, max_len))] = (name, fmt, a.title, max_len)
        elif name in ("APP_VER", "VERSION"):
            entries[entries.index((name, fmt, value, max_len))] = (name, fmt, a.version, max_len)
        elif name == "CATEGORY":
            entries[entries.index((name, fmt, value, max_len))] = (name, fmt, a.category, max_len)
    open(a.path, "wb").write(build(entries))
    print(f"wrote {a.path}: ATTRIBUTE = 0x{int(a.attribute):08X}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
