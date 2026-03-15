#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

# Build
echo "Building chord_test..."
make chord_test 2>&1 || { echo "BUILD FAILED"; exit 1; }
echo ""

pass=0; fail=0

run() {
    local id="$1"
    local desc="$2"
    local num="${id%%_*}"
    python3 tests/generate.py "$id"  > tests/expected/"$id".txt
    timeout 10 ./chord_test "$num"   > tests/actual/"$id".txt 2>/dev/null
    if diff -q tests/expected/"$id".txt tests/actual/"$id".txt > /dev/null 2>&1; then
        echo "PASS  $id  $desc"
        pass=$((pass+1))
    else
        echo "FAIL  $id  $desc"
        diff tests/expected/"$id".txt tests/actual/"$id".txt | head -40
        echo "---"
        fail=$((fail+1))
    fi
}

mkdir -p tests/expected tests/actual

run "01_baseline"         "TA sample test case"
run "02_two_nodes"        "Two nodes, both halves of ring"
run "03_adjacent"         "Nodes 0 and 1"
run "04_single"           "Single node only"
run "05_wrap_keys"        "Keys that wrap around node 0"
run "06_key_eq_node"      "Key ID equals node ID"
run "07_key_at_boundary"  "Keys just before/after each node"
run "08_even_4"           "Four evenly-spaced nodes: 0 64 128 192"
run "09_even_8"           "Eight evenly-spaced nodes"
run "10_dense"            "Dense ring, many nodes close together"
run "11_sparse"           "Two nodes far apart: 1 and 254"
run "12_join_order_asc"   "Five nodes joined ascending"
run "13_join_order_desc"  "Same five nodes joined descending"
run "14_join_between"     "New node joins between two existing nodes"
run "15_bulk_migration"   "Many keys migrate on single join"
run "16_no_migration"     "Join produces zero migration"
run "17_wrap_migration"   "Migration across the wrap-around boundary"
run "18_lookup_local"     "Lookup key owned by the querying node"
run "19_lookup_all_256"   "All 256 keys inserted and looked up"
run "20_leave_middle"     "Middle node leaves"
run "21_leave_successor"  "Direct successor leaves"
run "22_leave_all_keys"   "Leaving node had many keys"
run "23_random_a"         "Random ring config A"
run "24_random_b"         "Random ring config B"
run "25_random_c"         "Random ring config C"

echo ""
echo "Results: $pass passed, $fail failed out of $((pass+fail)) tests"
[ $fail -eq 0 ] && echo "ALL TESTS PASS" || echo "FAILURES PRESENT"
