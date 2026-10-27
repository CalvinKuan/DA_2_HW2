#!/usr/bin/env bash
# Draw every testcase into testcases/figures/:
#   <name>_input.png      the input layout
#   <name>_expected.png   the expected violations (if testcases/expected/<name>.out exists)
# If the program's result output/<name>.out exists (run `make test` first), it is drawn to
# output/<name>.png as well, together with the regions checked (output/<name>.regions).
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
    regions="$ROOT/output/$name.regions"
    if [[ -f "$out" ]]; then
        extra=()
        if [[ -f "$regions" ]]; then extra=(-c "$regions"); fi
        python3 "$PLOT" "$in" -r "$out" ${extra[@]+"${extra[@]}"} -o "$ROOT/output/$name.png" \
            -t "$name - output of ./eol_drc"
        echo "output/$name.png"
    fi
done
