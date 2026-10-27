#!/usr/bin/env bash
# Draw every testcase into testcases/figures/:
#   <name>_input.png      the input layout
#   <name>_expected.png   the expected violations (if testcases/expected/<name>.out exists)
# If your program's result output/<name>.out exists, it is drawn to output/<name>.png as well.
# Usage: scripts/make_figures.sh [name-filter]
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PLOT="$ROOT/scripts/plot_layout.py"
FIG="$ROOT/testcases/figures"
FILTER="${1:-}"
mkdir -p "$FIG"

for in in "$ROOT"/testcases/inputs/*.txt; do
    name="$(basename "$in" .txt)"
    if [[ -n "$FILTER" && "$name" != *"$FILTER"* ]]; then continue; fi

    python3 "$PLOT" "$in" -o "$FIG/${name}_input.png" -t "$name - input"
    echo "testcases/figures/${name}_input.png"

    exp="$ROOT/testcases/expected/$name.out"
    if [[ -f "$exp" ]]; then
        python3 "$PLOT" "$in" -r "$exp" -o "$FIG/${name}_expected.png" -t "$name - expected"
        echo "testcases/figures/${name}_expected.png"
    fi

    out="$ROOT/output/$name.out"
    if [[ -f "$out" ]]; then
        python3 "$PLOT" "$in" -r "$out" -o "$ROOT/output/$name.png" -t "$name - your result"
        echo "output/$name.png"
    fi
done
