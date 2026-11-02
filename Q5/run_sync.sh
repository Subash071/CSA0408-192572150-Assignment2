#!/bin/bash
echo "=== short critical section (counter++), 2,000,000 updates in total; mean of 3 runs shown as the median run"
for m in 0 1 2 3; do for t in 1 4 16; do
  for r in 1 2 3; do timeout 60 ./syncbench $m $t 0 2000000 || echo "method $m threads $t TIMEOUT"; done | sort -t= -k5 | sed -n 2p
done; done
echo "=== long critical section (counter++ then usleep(100) inside the lock), 4,000 updates"
for m in 0 1 3; do for t in 1 4 16; do
  for r in 1 2 3; do timeout 60 ./syncbench $m $t 100 4000 || echo "method $m threads $t TIMEOUT"; done | sed -n 2p
done; done
