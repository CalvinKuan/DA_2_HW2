#!/usr/bin/env bash
# Build, run every testcase in testcases/inputs/, and compare with testcases/expected/.
# Usage: scripts/run_tests.sh [name-filter]      e.g. scripts/run_tests.sh tc06
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/eol_drc"
IN_DIR="$ROOT/testcases/inputs"
EXP_DIR="$ROOT/testcases/expected"
OUT_DIR="$ROOT/output"
FILTER="${1:-}"

make -C "$ROOT" -s || exit 1
mkdir -p "$OUT_DIR"

# Round numbers to 3 decimals and sort the violation lines, so the order of
# violations and the number formatting do not matter.
normalize() {
    tr ',' ' ' < "$1" | awk '
        $1 == "number_of_violations" { print; next }
        $1 == "end_of_violation"     { next }
        NF == 5 { printf "%d %.3f %.3f %.3f %.3f\n", $1, $2, $3, $4, $5; next }
        NF > 0  { print "UNEXPECTED LINE: " $0 }' | sort
}

pass=0
fail=0
TIMEFORMAT=%R
for in in "$IN_DIR"/*.txt; do
    name="$(basename "$in" .txt)"
    if [[ -n "$FILTER" && "$name" != *"$FILTER"* ]]; then continue; fi

    out="$OUT_DIR/$name.out"
    log="$OUT_DIR/$name.log"
    exp="$EXP_DIR/$name.out"
    rm -f "$out"

    { time "$BIN" "$in" "$out" > "$log" 2>&1; echo $? > "$OUT_DIR/.rc"; } 2> "$OUT_DIR/.time"
    rc="$(cat "$OUT_DIR/.rc")"
    sec="$(tail -n 1 "$OUT_DIR/.time")"

    if [[ "$rc" != 0 || ! -f "$out" ]]; then
        printf "[FAIL] %-22s program exited with code %s (see %s)\n" "$name" "$rc" "${log#$ROOT/}"
        fail=$((fail + 1))
        continue
    fi

    found="$(awk '$1 == "number_of_violations" { print $2 }' "$out")"
    if [[ ! -f "$exp" ]]; then
        printf "[ -- ] %-22s %s violations, %ss (no expected answer)\n" "$name" "$found" "$sec"
        continue
    fi

    if diff <(normalize "$exp") <(normalize "$out") > "$OUT_DIR/$name.diff"; then
        printf "[PASS] %-22s %s violations, %ss\n" "$name" "$found" "$sec"
        pass=$((pass + 1))
        rm -f "$OUT_DIR/$name.diff"
    else
        printf "[FAIL] %-22s diff ('<' expected, '>' yours):\n" "$name"
        head -n 20 "$OUT_DIR/$name.diff" | sed 's/^/         /'
        fail=$((fail + 1))
    fi
done
rm -f "$OUT_DIR/.rc" "$OUT_DIR/.time"

echo "----"
echo "passed: $pass, failed: $fail"
[[ $fail -eq 0 ]]
