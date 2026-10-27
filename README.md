# Project 2 — End-of-Line (EOL) DRC Checker

Reads a layout in the project-1 format, checks the end-of-line spacing rule on every layer,
and reports where the violations are.

## Quick start

```bash
make          # build ./eol_drc (C++17, no external libraries)
make run      # check the layout given in the spec and print the result
make test     # run all testcases and compare with the expected results
```

Check any layout file:

```bash
./eol_drc testcases/inputs/tc07_overlap.txt                        # result to stdout
./eol_drc testcases/inputs/tc07_overlap.txt output/tc07_overlap.out # result to a file + details
```

## Example: tc07_overlap

Input: rectangles are numbered in input order (#1 and #4 are the same rectangle; #5 lies
inside #1).

![tc07 input](docs/tc07_input.png)

Output: two violations, drawn in red.

![tc07 output](docs/tc07_output.png)

- **V1**: the top end of wire #1 (width 0.6) is 0.5 um below #2, and #3 is beside it.
  The duplicated rectangle #4 and the inner rectangle #5 do not create extra violations.
- **Middle group**: #7 lies completely inside block #6, so it has no edge of its own and
  nothing is reported.
- **V2**: in the hook shape, the top end of #8 faces #11, another part of the same metal,
  only 0.5 um away. #12 is beside it.

## Directory

```
.
├── Makefile
├── src/
│   ├── main.cpp          entry point (parse -> check -> write result)
│   ├── layout.h/.cpp     data structures + layout file parser
│   ├── eol_rule.h        rule parameters (EOL spacing 1.0, length of end 0.6,
│   │                     parallel length 0.7, parallel halo 0.4)
│   ├── eol_checker.h/.cpp  the EOL checking algorithm
│   └── report.h/.cpp     result writer
├── testcases/
│   ├── inputs/           test layouts (tc00_sample.txt = the layout given in the spec)
│   ├── expected/         expected results
│   └── figures/          pictures of every testcase and its expected violations
├── docs/                 pictures used in this README
└── scripts/
    ├── run_tests.sh      runs all testcases and compares with expected results
    ├── plot_layout.py    draws a layout + violation markers (needs matplotlib)
    └── make_figures.sh   draws every testcase into testcases/figures/
```

## Compile

Requires a C++17 compiler (g++ 7+ or clang++). No external libraries.

```bash
make
```

## Run

```bash
./eol_drc <layout_file> [result_file]

# e.g. the layout given in the spec
mkdir -p output
./eol_drc testcases/inputs/tc00_sample.txt output/tc00_sample.out
```

If `result_file` is given, the result is written there and a summary (with details of each
violation) is printed to stdout. Otherwise the result is printed to stdout.

Run all testcases:

```bash
make test
```

## Input format

```
number_of_layers 1
M1  1
end_of_layer
number_of_rectangles  6
1  4.0  0,   7.8  0.6        <layer> <x1> <y1>, <x2> <y2>   (um)
...
end_of_rectangle
```

## Output format

```
number_of_violations 2
1  6.100 1.000,  7.000 1.600     <layer> <xl> <yl>, <xh> <yh>   (um)
...
end_of_violation
```

Each line is one violation marker: the rectangle between the EOL edge and the shape that is
too close to it.

## Rule interpretation

- Only shapes on the same layer interact.
- Overlapping or abutting rectangles on the same layer form one piece of metal; the seam
  between them is not an edge.
- **EOL edge**: an edge of the merged metal whose length is <= 0.6 um, with convex corners
  at both ends.
- **Parallel condition**: the rule applies to an EOL edge only if some metal ("object 3")
  lies within 0.4 um of one of its sides, within 0.7 um back from the EOL edge.
  A distance of exactly 0.4 um counts (in the given layout, rect #1 is exactly 0.4 um below
  rect #2).
- **Violation**: metal in front of the EOL edge, overlapping its span, closer than 1.0 um.
  Exactly 1.0 um is legal.
- The side extension of the EOL window (eolWithin) is not given in the spec, so it is 0.
- Coordinates are converted to integer database units (1 nm) to avoid floating-point errors
  such as 2.3 - 1.3 = 0.9999999999999998.

## Algorithm

All four directions are handled by one routine that checks EOL edges facing **up**: for
direction k, the layer is rotated by k x 90 degrees, checked, and the results are rotated back.

1. Parse the file, convert um to integer DBU, group the rectangles by layer.
2. For each layer and each rotation, build a uniform-grid spatial index
   (cell size ~ sqrt(bounding-box area / n), at least 1 um). A "which rects touch this box"
   query then only visits nearby rectangles.
3. **Find EOL edges.** For each rectangle r with width <= 0.6, start from its top side
   y = Y, x in [xa, xb] = [r.xl, r.xh]:
   - grow [xa, xb] with every rectangle that is filled just below Y and touches the interval
     (this gives the edge of the merged metal); stop as soon as it is longer than 0.6;
   - reject it if any rectangle covers (xa, xb) from above (then it is not a boundary edge,
     or one of its corners is concave);
   - skip edges already checked (the same edge can be reached from several rectangles).
4. **Parallel check.** Query the box [xa - 0.4, xb + 0.4] x [Y - 0.7, Y]. The rule applies
   if some rectangle there sticks out beyond xa or beyond xb.
5. **Spacing check.** Query the box [xa, xb] x [Y, Y + 1.0]. Every rectangle s whose bottom
   is in [Y, Y + 1.0) and which overlaps (xa, xb) is a violation, with marker
   (max(xa, s.xl), Y) - (min(xb, s.xh), s.yl).
6. Rotate the EOL edge and marker back, then remove duplicate markers (a pair of facing EOL
   edges is found from both ends), sort them and write the result.

**Complexity**: building the index is O(n). Every rectangle runs a constant number of
queries over its neighbourhood, so the check is about O(n * d), where d is the number of
rectangles near an edge (small for real layouts).

| rectangles | check time | total time |
|---:|---:|---:|
| 10,000 | 14 ms | 0.02 s |
| 100,000 | 57 ms | 0.10 s |
| 1,000,000 | 0.95 s | 1.5 s |

(random layouts from `testcases/gen_random.py`, Apple M-series laptop)

## Result on the given layout

```
$ ./eol_drc testcases/inputs/tc00_sample.txt output/tc00_sample.out
$ cat output/tc00_sample.out
number_of_violations 2
1  6.100 1.000,  7.000 1.600
1  10.400 1.800,  11.000 2.200
end_of_violation
```

1. **(6.1, 1.0) - (7.0, 1.6)**: the right end of rect #2 (x = 6.1, height 0.6) and the left
   end of rect #4 (x = 7.0) are 0.9 um apart (< 1.0). Rect #1 is 0.4 um below both ends, so
   the parallel condition holds. Both ends report the same marker.
2. **(10.4, 1.8) - (11.0, 2.2)**: the top end of rect #5 (y = 1.8, width 0.6) is 0.4 um
   below rect #6 (< 1.0). Rect #4 lies right beside it, so the parallel condition holds.

Picture: `testcases/figures/tc00_sample_expected.png`.
