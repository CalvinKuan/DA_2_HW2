#!/usr/bin/env python3
"""Convert a layout in the project-1 text format to GDSII (for Calibre).

Usage: python3 scripts/txt2gds.py <layout.txt> <out.gds> [top_cell_name]

Every rectangle becomes a BOUNDARY on GDS layer = its layer number, datatype 0,
in one top cell (default "TOP"). User unit = 1 um, database unit = 1 nm.
No external packages needed.
"""
import struct
import sys
import time

DBU_PER_UM = 1000


def read_rects(path):
    tok = open(path).read().replace(",", " ").split()
    i = tok.index("number_of_rectangles", tok.index("end_of_layer")) + 2
    rects = []
    while tok[i] != "end_of_rectangle":
        L = int(tok[i])
        x1, y1, x2, y2 = (round(float(t) * DBU_PER_UM) for t in tok[i + 1:i + 5])
        rects.append((L, min(x1, x2), min(y1, y2), max(x1, x2), max(y1, y2)))
        i += 5
    return rects


def gds_real8(x):
    """IEEE double -> GDSII 8-byte real (excess-64, base-16 exponent)."""
    if x == 0:
        return bytes(8)
    sign = 0x80 if x < 0 else 0
    x = abs(x)
    exp = 64
    while x >= 1:
        x /= 16
        exp += 1
    while x < 1 / 16:
        x *= 16
        exp -= 1
    mantissa = int(round(x * (1 << 56)))
    return bytes([sign | exp]) + mantissa.to_bytes(7, "big")


def record(rtype, dtype, payload=b""):
    return struct.pack(">HBB", 4 + len(payload), rtype, dtype) + payload


def int2(*v):
    return struct.pack(">%dh" % len(v), *v)


def int4(*v):
    return struct.pack(">%di" % len(v), *v)


def string(s):
    b = s.encode("ascii")
    return b + b"\0" if len(b) % 2 else b


def write_gds(rects, out, top="TOP"):
    t = time.localtime()
    stamp = int2(t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec) * 2
    data = [
        record(0x00, 0x02, int2(600)),                         # HEADER
        record(0x01, 0x02, stamp),                             # BGNLIB
        record(0x02, 0x06, string("EOL_LIB")),                 # LIBNAME
        record(0x03, 0x05, gds_real8(1.0 / DBU_PER_UM) + gds_real8(1e-9)),  # UNITS
        record(0x05, 0x02, stamp),                             # BGNSTR
        record(0x06, 0x06, string(top)),                       # STRNAME
    ]
    for (L, xl, yl, xh, yh) in rects:
        data += [
            record(0x08, 0x00),                                # BOUNDARY
            record(0x0D, 0x02, int2(L)),                       # LAYER
            record(0x0E, 0x02, int2(0)),                       # DATATYPE
            record(0x10, 0x03, int4(xl, yl, xh, yl, xh, yh, xl, yh, xl, yl)),  # XY
            record(0x11, 0x00),                                # ENDEL
        ]
    data += [record(0x07, 0x00), record(0x04, 0x00)]           # ENDSTR, ENDLIB
    with open(out, "wb") as f:
        f.write(b"".join(data))


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    rects = read_rects(sys.argv[1])
    write_gds(rects, sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else "TOP")
    print("%s: %d rectangles -> %s" % (sys.argv[1], len(rects), sys.argv[2]))


if __name__ == "__main__":
    main()
