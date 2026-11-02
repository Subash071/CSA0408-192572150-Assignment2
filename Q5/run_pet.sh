#!/bin/bash
run() { for r in 1 2 3 4 5; do s=$(date +%s.%N); out=$(timeout 60 ./$1); rc=$?; echo "$2 run$r: ${out:-TIMEOUT} time=$(echo "$(date +%s.%N) - $s" | bc) s"; done; }
run pa2000 "atomic, spin, 2000 loops"
run pp2000 "plain,  spin, 2000 loops"
run pay "atomic, spin+yield, 100000 loops"
