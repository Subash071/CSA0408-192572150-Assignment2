"""Q2: starvation with and without ageing, and MLFQ versus single queues"""
import json

# ---------- starvation / ageing (preemptive priority, 1 unit steps) ----------
def prio_run(jobs, age_every=None, horizon=400):
    # jobs: name, arrival, burst, base_priority (1 = highest)
    rem = {j[0]: j[2] for j in jobs}; info = {j[0]: j for j in jobs}
    wait = {j[0]: 0 for j in jobs}; t = 0; start = {}; fin = {}
    while any(rem.values()) and t < horizon:
        ready = [n for n in rem if rem[n] > 0 and info[n][1] <= t]
        if not ready: t += 1; continue
        def eff(n):
            p = info[n][3]
            if age_every: p -= wait[n] // age_every
            return (p, info[n][1], n)
        cur = min(ready, key=eff)
        start.setdefault(cur, t)
        for n in ready:
            if n != cur: wait[n] += 1
        rem[cur] -= 1; t += 1
        if rem[cur] == 0: fin[cur] = t
    return start, fin

stream = [(f"H{i:02d}", 3 * i, 3, 1) for i in range(0, 21)]   # high-priority, 3 units every 3 units
long_job = [("LONG", 0, 10, 5)]
jobs = long_job + stream
res = {}
for label, age in (("no ageing", None), ("ageing (1 level per 10 units waited)", 10)):
    s, f = prio_run(jobs, age)
    res[label] = dict(start=s["LONG"], finish=f["LONG"], waited=s["LONG"] - 0)
    print(f"{label:40s} LONG starts t={s['LONG']}  finishes t={f['LONG']}")

# ---------- MLFQ vs single queues ----------
# workload: 2 long jobs (40 units) at t=0, 10 short jobs (2 units) every 4 units from t=2
W = [("L1", 0, 40), ("L2", 0, 40)] + [(f"S{i:02d}", 2 + 4 * i, 2) for i in range(10)]

def run_rr(jobs, q):
    t, rem = 0, {j[0]: j[2] for j in jobs}
    order = sorted(jobs, key=lambda j: (j[1], j[0])); queue = []; added = set()
    gantt = []; first = {}; fin = {}
    def admit():
        for n, a, b in order:
            if n not in added and a <= t: queue.append(n); added.add(n)
    admit()
    while len(added) < len(jobs) or queue:
        if not queue: t += 1; admit(); continue
        n = queue.pop(0); run = min(q, rem[n]); first.setdefault(n, t)
        gantt.append(n); t += run; rem[n] -= run; admit()
        if rem[n]: queue.append(n)
        else: fin[n] = t
    return first, fin, gantt

def run_srtf(jobs):
    rem = {j[0]: j[2] for j in jobs}; arr = {j[0]: j[1] for j in jobs}
    t = 0; first = {}; fin = {}; last = None; sw = 0
    while any(rem.values()):
        ready = [n for n in rem if rem[n] and arr[n] <= t]
        if not ready: t += 1; continue
        cur = min(ready, key=lambda n: (rem[n], arr[n], n)); first.setdefault(cur, t)
        if last is not None and last != cur: sw += 1
        last = cur; rem[cur] -= 1; t += 1
        if rem[cur] == 0: fin[cur] = t
    return first, fin, sw

def run_fcfs(jobs):
    t = 0; first = {}; fin = {}
    for n, a, b in sorted(jobs, key=lambda j: (j[1], j[0])):
        t = max(t, a); first[n] = t; t += b; fin[n] = t
    return first, fin

def run_mlfq(jobs, quanta=(2, 4, 8)):
    """3 RR levels with quanta 2,4,8; new job enters level 0; a job that uses its full
    quantum is demoted; a higher level always preempts at quantum boundaries."""
    rem = {j[0]: j[2] for j in jobs}; arr = {j[0]: j[1] for j in jobs}
    levels = [[] for _ in quanta]; added = set(); lvl = {}
    t = 0; first = {}; fin = {}; last = None; sw = 0
    order = sorted(jobs, key=lambda j: (j[1], j[0]))
    def admit():
        for n, a, b in order:
            if n not in added and a <= t: levels[0].append(n); added.add(n); lvl[n] = 0
    admit()
    while len(fin) < len(jobs):
        admit()
        cur = None
        for i, L in enumerate(levels):
            if L: cur = L.pop(0); li = i; break
        if cur is None: t += 1; continue
        first.setdefault(cur, t)
        if last is not None and last != cur: sw += 1
        last = cur
        run = min(quanta[li], rem[cur]); t += run; rem[cur] -= run
        admit()
        if rem[cur] == 0: fin[cur] = t
        else:
            nl = min(li + 1, len(quanta) - 1); lvl[cur] = nl; levels[nl].append(cur)
    return first, fin, sw

def summary(name, first, fin, sw):
    sh = [j for j in W if j[0].startswith("S")]; lg = [j for j in W if j[0].startswith("L")]
    wt = lambda j: fin[j[0]] - j[1] - j[2]
    rt = lambda j: first[j[0]] - j[1]
    r = dict(name=name,
             short_wt=sum(map(wt, sh)) / len(sh), long_wt=sum(map(wt, lg)) / len(lg),
             short_rt=sum(map(rt, sh)) / len(sh), switches=sw,
             makespan=max(fin.values()))
    print(f"{name:22s} short WT={r['short_wt']:6.2f}  long WT={r['long_wt']:6.2f}  "
          f"short RT={r['short_rt']:6.2f}  switches={sw:3d}  finish={r['makespan']}")
    return r

rows = []
f, e = run_fcfs(W); rows.append(summary("FCFS", f, e, len(W) - 1))
f, e, sw = run_srtf(W); rows.append(summary("SRTF (needs burst)", f, e, sw))
for q in (2, 10):
    f, e, g = run_rr(W, q)
    sw = sum(1 for i in range(1, len(g)) if g[i] != g[i - 1])
    rows.append(summary(f"RR q={q}", f, e, sw))
f, e, sw = run_mlfq(W); rows.append(summary("MLFQ q=2/4/8", f, e, sw))
json.dump(dict(starvation=res, mlfq=rows), open("sim2_results.json", "w"), indent=1)
