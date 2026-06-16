#!/bin/sh
# run-tests.sh — run all classy-fuzz tests
#
# Usage: sh classy-fuzz/run-tests.sh ./c2m
#
# Valid tests should compile, run, and exit 0 (all checks pass).
# Bad tests should fail at compile time (non-zero exit).

C2M="${1:-./bin/classyc}"
DIR="$(dirname "$0")"
PASS=0
FAIL=0

echo "=== classy-fuzz test runner ==="
echo "compiler: $C2M"
echo ""

run_valid() {
    name="$1"
    echo -n "  VALID  $name ... "
    output=$($C2M "$DIR/$name" -eg 2>&1)
    rc=$?
    if [ $rc -eq 0 ]; then
        echo "OK"
        PASS=$((PASS + 1))
    else
        echo "FAIL (exit $rc)"
        echo "$output" | tail -5
        FAIL=$((FAIL + 1))
    fi
}

run_bad() {
    name="$1"
    echo -n "  BAD    $name ... "
    output=$($C2M "$DIR/$name" -eg 2>&1)
    rc=$?
    if [ $rc -ne 0 ]; then
        echo "OK (rejected)"
        PASS=$((PASS + 1))
    else
        echo "FAIL (should have been rejected)"
        FAIL=$((FAIL + 1))
    fi
}

	echo "-- valid tests (should compile & run) --"
	run_valid fuzz-dict-valid.c
	run_valid fuzz-class-valid.c
	run_valid fuzz-string-valid.c
	run_valid fuzz-string-ownership.c
	run_valid fuzz-mixed.c
	run_valid fuzz-ctor.c
	run_valid fuzz-defer.c
	run_valid fuzz-static-valid.c

echo ""
echo "-- bad tests (should be rejected at compile time) --"
run_bad fuzz-dict-bad.c
run_bad fuzz-class-bad.c
run_bad fuzz-string-bad.c

echo ""
echo "=== results: $PASS passed, $FAIL failed ==="
exit $FAIL
