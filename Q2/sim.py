"""Q2 scheduling simulator (1 time unit steps) - FCFS, SJF, SRTF, Priority, RR, MLFQ"""
import json, sys

JOBS = [  # name, arrival, burst, priority (1 = highest)
    ("P1", 0, 8, 2), ("P2", 1, 4, 3), ("P3", 2, 9, 1), ("P4", 3, 5, 4)]

def metrics(jobs, gantt):
    """gantt: list of (name,start,end). returns per-job dict"""
    res = {}
    for n, a, b, pr in jobs:
        segs = [g for g in gantt if g[0] == n]
        first = min(s for _, s, _ in segs)
        fin = max(e for _, _, e in segs)
        tat = fin - a
        res[n] = dict(arrival=a, burst=b, start=first, finish=fin,
                      tat=tat, wt=tat - b, rt=first - a)
    return res

def merge(gantt):
    out = []
    for n, s, e in gantt:
        if out and out[-1][0] == n and out[-1][2] == s:
            out[-1] = (n, out[-1][1], e)
        else:
            out.append((n, s, e))
    return out

def sim(jobs, pick, preemptive):
    t, rem, gantt, cur = 0, {j[0]: j[2] for j in jobs}, [], None
    info = {j[0]: j for j in jobs}
    while any(rem.values()):
        ready = [n for n in rem if rem[n] > 0 and info[n][1] <= t]
        if not ready:
            t += 1; cur = None; continue
        if cur is None or rem[cur] == 0 or preemptive:
            cur = pick(ready, rem, info, cur)
        gantt.append((cur, t, t + 1)); rem[cur] -= 1; t += 1
    return merge(gantt)

def fcfs(r, rem, info, cur): return min(r, key=lambda n: (info[n][1], n))
def sjf(r, rem, info, cur):  return min(r, key=lambda n: (info[n][2], info[n][1], n))
def srtf(r, rem, info, cur): return min(r, key=lambda n: (rem[n], info[n][1], n))
def prio(r, rem, info, cur): return min(r, key=lambda n: (info[n][3], info[n][1], n))

def rr(jobs, q):
    t, rem = 0, {j[0]: j[2] for j in jobs}
    arr = {j[0]: j[1] for j in jobs}
    order = sorted(jobs, key=lambda j: (j[1], j[0]))
    queue, added, gantt = [], set(), []
    def admit():
        for n, a, b, p in order:
            if n not in added and a <= t: queue.append(n); added.add(n)
    admit()
    while len(added) < len(jobs) or queue:
        if not queue:
            t += 1; admit(); continue
        n = queue.pop(0)
        run = min(q, rem[n]); gantt.append((n, t, t + run)); t += run; rem[n] -= run
        admit()
        if rem[n] > 0: queue.append(n)
    return gantt   # not merged: each slice is a turn

def switches(gantt):
    g = merge(gantt)
    return len(g) - 1

def fmt(g): return " ".join(f"{n}({s}-{e})" for n, s, e in g)

if __name__ == "__main__":
    out = {}
    algos = {
        "FCFS": merge(sim(JOBS, fcfs, False)),
        "SJF": merge(sim(JOBS, sjf, False)),
        "SRTF": merge(sim(JOBS, srtf, True)),
        "Priority": merge(sim(JOBS, prio, False)),
        "RR q=3": rr(JOBS, 3),
    }
    for k, g in algos.items():
        m = metrics(JOBS, g)
        n = len(JOBS)
        out[k] = dict(gantt=fmt(g), jobs=m,
                      avg_wt=sum(v["wt"] for v in m.values()) / n,
                      avg_tat=sum(v["tat"] for v in m.values()) / n,
                      avg_rt=sum(v["rt"] for v in m.values()) / n,
                      switches=switches(g))
        print(f"{k:9s} {fmt(g)}")
        for nme, v in m.items():
            print(f"   {nme} WT={v['wt']:2d} TAT={v['tat']:2d} RT={v['rt']:2d}")
        print(f"   avg WT={out[k]['avg_wt']:.2f} TAT={out[k]['avg_tat']:.2f} "
              f"RT={out[k]['avg_rt']:.2f} switches={out[k]['switches']}")
    print("\n--- quantum study ---")
    qs = {}
    for q in (1, 3, 5, 20):
        g = rr(JOBS, q); m = metrics(JOBS, merge(g)) if False else metrics(JOBS, g)
        n = len(JOBS)
        qs[q] = dict(avg_wt=sum(v["wt"] for v in m.values()) / n,
                     avg_tat=sum(v["tat"] for v in m.values()) / n,
                     avg_rt=sum(v["rt"] for v in m.values()) / n,
                     switches=switches(g), turns=len(g))
        print(q, qs[q])
    out["quantum"] = qs
    json.dump(out, open("sim_results.json", "w"), indent=1)
