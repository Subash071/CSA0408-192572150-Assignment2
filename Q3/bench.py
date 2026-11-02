import subprocess, re, statistics, json
configs = [("seq",1),("pool",4),("pool",8),("pool",16),("percam",64),("proc",4),("proc",8),("proc",16)]
out = {}
for io in (2000, 0):
    base = None
    print(f"=== io_us={io}")
    for mode, k in configs:
        if io == 0 and mode == "proc" and k != 4: continue
        ts = []
        for _ in range(5):
            r = subprocess.run(["./cams", mode, str(k), str(io)], capture_output=True, text=True).stdout
            ts.append(float(re.search(r"time=\s*([\d.]+)", r).group(1)))
            plates = int(re.search(r"plates=(\d+)", r).group(1))
        m = statistics.mean(ts)
        if mode == "seq": base = m
        sp = base / m
        out[f"{io}:{mode}:{k}"] = dict(ms=m, speedup=sp, plates=plates, runs=ts)
        print(f"{mode:7s} K={k:<3d} mean={m:8.1f} ms  speed-up={sp:4.2f}  plates={plates}  runs={ts}")
json.dump(out, open("bench_results.json","w"), indent=1)
