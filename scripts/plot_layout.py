#!/usr/bin/env python3
"""Plot a layout, optionally with violation markers and the regions the checker looked at.

Usage: python3 scripts/plot_layout.py <layout_file> -o <picture.png>
                                      [-r <result_file>] [-c <regions_file>] [-t <title>]
  -r  result file (./eol_drc output): violations are drawn in red as V1, V2, ...
  -c  regions file (./eol_drc --regions output): for every EOL edge, the edge itself,
      the two parallel regions (filled when a shape was found there) and the EOL
      spacing region in front of the edge (EOLSpacing deep, EOLWithin wider on each side)
The output format follows the extension of -o (.png / .svg / .pdf).
Rectangles are labelled #1, #2, ... in input order.
"""
import argparse
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.collections import PatchCollection  # noqa: E402
from matplotlib.lines import Line2D  # noqa: E402
from matplotlib.patches import Patch, Rectangle  # noqa: E402
from matplotlib.ticker import MultipleLocator  # noqa: E402

COLORS = ["#4472c4", "#70ad47", "#ed7d31", "#7030a0", "#00a0c0", "#bf9000"]
EDGE_COLOR = "#ff8c00"
SIDE_COLOR = "#2e9e44"
FRONT_COLOR = "#8e44ad"
MAX_LABELLED_RECTS = 80
MAX_LABELLED_MARKS = 40
MAX_DRAWN_CHECKS = 300


def read_tokens(path):
    return open(path).read().replace(",", " ").split()


def read_layout(path):
    tok = read_tokens(path)
    i = tok.index("number_of_layers") + 2
    names = {}
    while tok[i] != "end_of_layer":
        names[int(tok[i + 1])] = tok[i]
        i += 2
    i = tok.index("number_of_rectangles", i) + 2
    rects = []
    while tok[i] != "end_of_rectangle":
        L = int(tok[i])
        x1, y1, x2, y2 = (float(t) for t in tok[i + 1:i + 5])
        rects.append((L, min(x1, x2), min(y1, y2), max(x1, x2), max(y1, y2)))
        i += 5
    return names, rects


def read_result(path):
    tok = read_tokens(path)
    i = tok.index("number_of_violations") + 2
    marks = []
    while tok[i] != "end_of_violation":
        marks.append((int(tok[i]), *(float(t) for t in tok[i + 1:i + 5])))
        i += 5
    return marks


def read_checks(path):
    """Lines: <layer> <dir> edge x1 y1 x2 y2 side xl yl xh yh <hit> side ... <hit>
    front xl yl xh yh applies <yes|no> metal_in_front <yes|no> violation <yes|no>"""
    checks = []
    for line in open(path):
        t = line.split()
        if len(t) < 28 or t[2] != "edge":
            continue
        box = lambda k: tuple(float(v) for v in t[k:k + 4])  # noqa: E731
        flags = dict(zip(t[24::2], t[25::2]))
        checks.append({
            "edge": box(3),
            "sides": [(box(8), t[12] == "yes"), (box(14), t[18] == "yes")],
            "front": box(20),
            "applies": flags.get("applies") == "yes",
            "front_hit": flags.get("metal_in_front") == "yes",
            "violated": flags.get("violation") == "yes",
        })
    return checks


def place_label(ax, renderer, placed, xy_list, text, **style):
    """Try candidate positions [(xy, offset_pt, ha, va), ...]; keep the first one that does
    not overlap anything in `placed` (display-space boxes). Falls back to the first candidate."""
    for k, (xy, off, ha, va) in enumerate(xy_list + [xy_list[0]]):
        t = ax.annotate(text, xy=xy, xytext=off, textcoords="offset points", ha=ha, va=va, **style)
        bb = t.get_window_extent(renderer).expanded(1.08, 1.15)
        if k == len(xy_list) or not any(bb.overlaps(p) for p in placed):
            placed.append(bb)
            return t
        t.remove()


def tick_step(span):
    for limit, step in ((16, 1), (40, 2), (100, 5), (200, 10)):
        if span <= limit:
            return step
    return None


def draw_checks(ax, checks):
    for c in checks:
        for (a, b, cc, d), hit in c["sides"]:
            ax.add_patch(Rectangle((a, b), cc - a, d - b, fill=hit, facecolor=SIDE_COLOR,
                                   alpha=0.25 if hit else 1.0, edgecolor="none", zorder=3))
            ax.add_patch(Rectangle((a, b), cc - a, d - b, fill=False, edgecolor=SIDE_COLOR,
                                   linewidth=1.0, linestyle="--", zorder=3))
        a, b, cc, d = c["front"]
        ax.add_patch(Rectangle((a, b), cc - a, d - b, facecolor=FRONT_COLOR, alpha=0.08,
                               edgecolor="none", zorder=3))
        ax.add_patch(Rectangle((a, b), cc - a, d - b, fill=False, edgecolor=FRONT_COLOR,
                               linewidth=1.2, linestyle="--", zorder=3))
        x1, y1, x2, y2 = c["edge"]
        ax.plot([x1, x2], [y1, y2], color=EDGE_COLOR, linewidth=3.5, solid_capstyle="butt",
                zorder=8)  # above the violation markers, which start at the edge


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("layout")
    ap.add_argument("-o", "--out", required=True)
    ap.add_argument("-r", "--result", help="result file with violation markers")
    ap.add_argument("-c", "--checks", help="regions file written by ./eol_drc --regions")
    ap.add_argument("-t", "--title")
    args = ap.parse_args()

    names, rects = read_layout(args.layout)
    marks = read_result(args.result) if args.result else None
    checks = read_checks(args.checks) if args.checks else None
    if checks is not None and len(checks) > MAX_DRAWN_CHECKS:
        print("[plot] %d EOL edges: too many to draw their regions, skipped" % len(checks),
              file=sys.stderr)
        checks = None

    xs = [v for r in rects for v in (r[1], r[3])] or [0.0, 1.0]
    ys = [v for r in rects for v in (r[2], r[4])] or [0.0, 1.0]
    for c in checks or []:
        for (a, b, cc, d) in [c["front"]] + [s for s, _ in c["sides"]]:
            xs += [a, cc]
            ys += [b, d]
    pad = 0.04 * max(max(xs) - min(xs), max(ys) - min(ys)) + 0.4
    x0, x1, y0, y1 = min(xs) - pad, max(xs) + pad, min(ys) - pad, max(ys) + pad

    fig_w = 13.0
    fig_h = min(max(2.5, fig_w * (y1 - y0) / (x1 - x0)), 13.0)
    fig, ax = plt.subplots(figsize=(fig_w, fig_h))
    ax.set_xlim(x0, x1)
    ax.set_ylim(y0, y1)
    ax.set_aspect("equal")
    step = tick_step(max(x1 - x0, y1 - y0))
    if step:
        ax.xaxis.set_major_locator(MultipleLocator(step))
        ax.yaxis.set_major_locator(MultipleLocator(step))
    ax.grid(True, color="#dddddd", linewidth=0.6, zorder=0)
    ax.set_axisbelow(True)
    ax.set_xlabel("x (um)")
    ax.set_ylabel("y (um)")

    many = len(rects) > MAX_LABELLED_RECTS
    layers = sorted({r[0] for r in rects})
    for L in layers:
        col = COLORS[(L - 1) % len(COLORS)]
        patches = [Rectangle((a, b), c - a, d - b) for (LL, a, b, c, d) in rects if LL == L]
        ax.add_collection(PatchCollection(patches, facecolor=col, edgecolor=col, alpha=0.45,
                                          linewidth=0.3 if many else 1.0, zorder=2))

    legend = []
    if len(layers) > 1:
        legend += [Patch(color=COLORS[(L - 1) % len(COLORS)], alpha=0.45,
                         label="%s (layer %d)" % (names.get(L, "?"), L)) for L in layers]
    if checks is not None:
        draw_checks(ax, checks)
        legend += [
            Line2D([], [], color=EDGE_COLOR, linewidth=3.5, label="EOL edge (length <= 0.6)"),
            Patch(facecolor="none", edgecolor=SIDE_COLOR, linestyle="--",
                  label="parallel region 0.4 x 0.7 (checked)"),
            Patch(facecolor=SIDE_COLOR, alpha=0.35, edgecolor=SIDE_COLOR, linestyle="--",
                  label="parallel region, metal found"),
            Patch(facecolor=FRONT_COLOR, alpha=0.15, edgecolor=FRONT_COLOR, linestyle="--",
                  label="EOL spacing region (checked)\nEOLSpacing 1.0, EOLWithin 0.4"),
        ]
    if marks is not None:
        legend.append(Patch(facecolor="red", alpha=0.5, edgecolor="red", hatch="////",
                            label="violation"))

    fig.canvas.draw()  # fix the axes box so label extents can be measured
    renderer = fig.canvas.get_renderer()
    placed = []

    lines = []
    if marks is not None and len(marks) > MAX_LABELLED_MARKS:
        # too many to label: just draw them
        ax.add_collection(PatchCollection([Rectangle((a, b), c - a, d - b) for (L, a, b, c, d) in marks],
                                          facecolor="red", edgecolor="red", linewidth=0.8, zorder=5))
    elif marks is not None:
        boxes = []
        for (L, a, b, c, d) in marks:
            ax.add_patch(Rectangle((a, b), c - a, d - b, facecolor="red", alpha=0.35,
                                   edgecolor="none", zorder=5))
            p = ax.add_patch(Rectangle((a, b), c - a, d - b, fill=False, edgecolor="red",
                                       linewidth=2.0, hatch="////", zorder=6))
            boxes.append(p.get_window_extent(renderer))
        placed.extend(boxes)
        for k, (L, a, b, c, d) in enumerate(marks, 1):
            cands = [((c, d), (10, 10), "left", "bottom"), ((a, d), (-10, 10), "right", "bottom"),
                     ((c, b), (10, -10), "left", "top"), ((a, b), (-10, -10), "right", "top"),
                     ((c, d), (22, 22), "left", "bottom"), ((a, d), (-22, 22), "right", "bottom")]
            place_label(ax, renderer, placed, cands, "V%d" % k, color="red", fontsize=10,
                        fontweight="bold", zorder=7,
                        arrowprops=dict(arrowstyle="-", color="red", lw=1))
            lines.append("V%d: layer %d  (%.3f, %.3f) - (%.3f, %.3f)" % (k, L, a, b, c, d))

    if not many:
        # identical rectangles share one label, e.g. "#1,#4"
        labels = {}
        for i, r in enumerate(rects):
            labels.setdefault(r, []).append("#%d" % (i + 1))
        style = dict(fontsize=8, color="#1a1a1a", zorder=4,
                     bbox=dict(boxstyle="round,pad=0.15", fc="white", ec="none", alpha=0.75))
        # small rectangles first, so they get the best spot
        for (L, a, b, c, d), ids in sorted(labels.items(),
                                           key=lambda kv: (kv[0][3] - kv[0][1]) * (kv[0][4] - kv[0][2])):
            cands = [(((a + c) / 2, (b + d) / 2), (0, 0), "center", "center"),
                     ((a, b), (3, 3), "left", "bottom"), ((a, d), (3, -3), "left", "top"),
                     ((c, d), (-3, -3), "right", "top"), ((c, b), (-3, 3), "right", "bottom"),
                     ((a, b), (0, -3), "left", "top"), ((a, d), (0, 3), "left", "bottom")]
            place_label(ax, renderer, placed, cands, ",".join(ids), **style)

    title = args.title or args.layout
    if marks is not None:
        title += "  [%d violation%s]" % (len(marks), "" if len(marks) == 1 else "s")
    else:
        title += "  [%d rectangles]" % len(rects)
    ax.set_title(title, fontsize=12)

    if legend:
        ax.legend(handles=legend, loc="upper left", bbox_to_anchor=(1.01, 1.0), fontsize=9,
                  borderaxespad=0.0)

    if lines:
        ax.annotate("\n".join(lines), xy=(0, 0), xycoords="axes fraction", xytext=(0, -42),
                    textcoords="offset points", va="top", ha="left", fontsize=10,
                    family="monospace", color="red")

    fig.savefig(args.out, dpi=150, bbox_inches="tight")


if __name__ == "__main__":
    main()
