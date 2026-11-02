import subprocess, re, json
def one(m, t, cs, total):
    runs = []
    for _ in range(3):
        o = subprocess.run(["./syncbench", str(m), str(t), str(cs), str(total)], capture_output=True, text=True, timeout=120).stdout.strip()
        w = float(re.search(r"wall=\s*([\d.]+)", o).group(1)); runs.append((w, o))
    runs.sort(); return runs[1][1]            # median by wall time
res = []
print("=== short critical section: counter++ , 2,000,000 updates in total (median of 3 runs)")
for m in (0, 1, 2, 3):
    for t in (1, 4, 16):
        o = one(m, t, 0, 2000000); print(o); res.append(o)
print("=== long critical section: counter++ then usleep(100) inside the lock, 4,000 updates (median of 3 runs)")
for m in (0, 1, 3):
    for t in (1, 4, 16):
        o = one(m, t, 100, 4000); print(o); res.append(o)
