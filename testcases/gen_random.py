#!/usr/bin/env python3
"""Random routing-like layout generator for performance tests (no golden answer).

Usage: python3 testcases/gen_random.py <num_rects> [seed] [num_layers] > out.txt
Coordinates are on a 0.1 um grid; rectangles may overlap or abut.
"""
import math
import random
import sys


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    n = int(sys.argv[1])
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    num_layers = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    rng = random.Random(seed)

    side = max(20, int(math.sqrt(n * 6)))  # chip is side x side um
    widths = [0.4, 0.5, 0.6, 0.6, 0.8, 1.0]
    rects = []
    for _ in range(n):
        layer = rng.randint(1, num_layers)
        kind = rng.random()
        if kind < 0.95:
            w = rng.choice(widths)
            length = rng.randint(6, 80) / 10
            track = rng.randint(0, side - 2) + rng.choice([0.0, 0.1, 0.2, 0.3])
            start = rng.randint(0, int((side - length) * 10)) / 10
            if kind < 0.65:  # horizontal wire
                x1, y1, x2, y2 = start, track, start + length, track + w
            else:            # vertical wire
                x1, y1, x2, y2 = track, start, track + w, start + length
        else:                # block
            bw, bh = rng.randint(10, 30) / 10, rng.randint(10, 30) / 10
            x1 = rng.randint(0, int((side - bw) * 10)) / 10
            y1 = rng.randint(0, int((side - bh) * 10)) / 10
            x2, y2 = x1 + bw, y1 + bh
        rects.append((layer, x1, y1, x2, y2))

    out = sys.stdout
    out.write("number_of_layers %d\n" % num_layers)
    for i in range(1, num_layers + 1):
        out.write("M%d  %d\n" % (i, i))
    out.write("end_of_layer\nnumber_of_rectangles  %d\n" % n)
    for (L, x1, y1, x2, y2) in rects:
        out.write("%d  %.1f %.1f,  %.1f %.1f\n" % (L, x1, y1, x2, y2))
    out.write("end_of_rectangle\n")


if __name__ == "__main__":
    main()
