# CSA04 Operating Systems - Assignment 2 (CO2)

Name: <SUBASH G>
Register number: <192572150>

## Environment used for the results in the report
- OS: Ubuntu 24.04.4 LTS (Linux 6.18), 1 virtual CPU core, 4 GB RAM
- Compiler: gcc 13.3.0
- Compile command: `gcc -std=c11 -Wall -Wextra -pthread -o <name> <name>.c`
  (add `-O2` for benchmarks, `-lrt` for message queues, `-D_GNU_SOURCE` or `-std=gnu11` where noted in the file header)

## Folders
| Folder | What it contains |
|---|---|
| Q1 | fork_demo.c (fork, ps, /proc), scale.c (2000 processes vs threads, context switch) |
| Q2 | rr.c (Round Robin, TODO 1 done), sim.py, sim2.py (all algorithms, ageing, MLFQ), pick_cost.c |
| Q3 | cams.c (threads and processes), race.c (shared counter), bench.py |
| Q4 | pipe_demo.c, pipe_noclose.c, shm_weather.c, xfer.c (1/10/100 MB), small.c |
| Q5 | bb.c (TODO 1 and 2 done), bb_multi.c, peterson.c, booking.c, monitor_bb.c, rw.c, dp.c, syncbench.c |
| Q6 | banker.c (TODO 1 done), bank_bench.c, detect.c |

Each `.out` file is the recorded output of the program of the same name.

## Result of each task (one line)
- Q1: fork() children seen as R, S and Z in ps and /proc. A fixed worker pool is recommended.
- Q2: RR output matches the expected output. Multilevel feedback queue with ageing is recommended.
- Q3: pool speed-up about 3.1 on 1 core (limit 3). Counter protected by mutex or atomic.
- Q4: pipe and shared-memory versions work. Shared memory is about 3x faster at 100 MB.
- Q5: bounded buffer sum is 210 in 100 runs. One lock per coach gives 21x the global-lock throughput.
- Q6: Banker's output matches. Safety algorithm time grows as O(m * n^2).

## Run
Compile each file with the command above, then run it, for example:
`gcc -std=c11 -Wall -Wextra -o rr rr.c && ./rr`
Timings depend on the machine. Re-run the benchmarks on your own computer and update the tables if the numbers differ.
