#!/bin/sh
# run-perf.sh — run the classy performance suite across execution modes
#
# Usage:
#   sh perf-tests/run-perf.sh [./c2m] [-O2]
#
# Runs each test (math, strings, dict) under the interpreter (-ei), lazily
# generated code (-el), and fully generated code (-eg).  Each test prints a
# deterministic checksum that must be identical in every mode (only timing
# should change) plus a PASS/FAIL line.

C2M="${1:-./c2m}"
OPT="${2:--O2}"
DIR="$(dirname "$0")"

TESTS="perf-math.c perf-strings.c perf-dict.c"
MODES="-ei -el -eg"

PASS=0
FAIL=0

echo "=== classy performance suite ==="
echo "compiler: $C2M    opt: $OPT"
echo ""

for t in $TESTS; do
    echo "########## $t ##########"
    for m in $MODES; do
        echo "---- mode $m ----"
        out=$("$C2M" $OPT "$DIR/$t" $m 2>&1 | grep -vE 'class_member|Found class|gen processing')
        rc=$?
        echo "$out" | grep -E 'elapsed|throughput|method calls|build|lookup|iterate|total|PASS|FAIL|checksum'
        if echo "$out" | grep -q 'PASS'; then
            PASS=$((PASS + 1))
        else
            FAIL=$((FAIL + 1))
            echo "  (no PASS line — see full output: $C2M $OPT $DIR/$t $m)"
        fi
        echo ""
    done
done

echo "=== suite: $PASS passed, $FAIL failed ==="
exit $FAIL
