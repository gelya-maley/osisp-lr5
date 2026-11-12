import csv
import os
import socket
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SERVER = os.path.join(ROOT, "build", "server.exe")
CLIENT = os.path.join(ROOT, "build", "client.exe")
DATA = os.path.join(ROOT, "data")
HOST = "127.0.0.1"
PORT = 20071
SEED = 7887
RUNS = 3

CONFIGS = [("A", 1, 100), ("B", 1, 500), ("C", 3, 100), ("D", 3, 500)]


def wait_port(host, port, timeout=10):
    t0 = time.time()
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    while time.time() - t0 < timeout:
        s.sendto(b"PING requestId=0", (host, port))
        time.sleep(0.1)
        try:
            s.settimeout(0.2)
            s.recvfrom(65536)
            return True
        except (socket.timeout, ConnectionResetError):
            s.settimeout(None)
    s.close()
    return False


def parse_stats(out):
    for line in out.splitlines():
        if line.startswith("STATS "):
            return dict(t.split("=", 1) for t in line.split()[1:])
    return {}


def run_once(cfg, clients, requests, run_no):
    srv = subprocess.Popen([SERVER, "--host", HOST, "--port", str(PORT)],
                           stdout=subprocess.DEVNULL)
    time.sleep(0.7)
    procs = []
    t0 = time.time()
    for i in range(1, clients + 1):
        log = os.path.join(DATA, f"client_{cfg}_r{run_no}_c{i}.log")
        procs.append(subprocess.Popen(
            [CLIENT, "--host", HOST, "--port", str(PORT), "--id", str(i),
             "--mode", "batch", "--requests", str(requests), "--seed", str(SEED),
             "--timeout", "2000", "--log", log],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True))
    outs = [p.communicate()[0] for p in procs]
    wall_ms = (time.time() - t0) * 1000.0
    srv.terminate()
    srv.wait(timeout=10)

    stats = [parse_stats(o) for o in outs]
    agg = lambda k, d=0: sum(float(s.get(k, d)) for s in stats if s)
    ok = int(agg("ok"))
    timeouts = int(agg("timeouts"))
    errors = int(agg("errors"))
    total_ms = max(float(s.get("totalMs", 0)) for s in stats)
    return {
        "config": cfg, "clients": clients, "requestsPerClient": requests, "run": run_no,
        "ok": ok, "timeouts": timeouts, "errors": errors,
        "bytesSent": int(agg("bytesSent")), "bytesRecv": int(agg("bytesRecv")),
        "totalMs": f"{total_ms:.3f}", "wallMs": f"{wall_ms:.3f}",
        "avgRttMs": f"{agg('avgRttMs') / len(stats):.4f}",
        "throughput": f"{ok * 1000.0 / wall_ms if wall_ms else 0:.3f}",
    }


def main():
    os.makedirs(DATA, exist_ok=True)
    rows = []
    for cfg, clients, requests in CONFIGS:
        for run in range(1, RUNS + 1):
            r = run_once(cfg, clients, requests, run)
            rows.append(r)
            print(f"config={cfg} run={run} wall={r['wallMs']}ms ok={r['ok']} "
                  f"timeouts={r['timeouts']} errors={r['errors']} rtt={r['avgRttMs']}")
            sys.stdout.flush()
            time.sleep(0.4)
    out = os.path.join(DATA, "experiments.csv")
    with open(out, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print("saved " + out)


if __name__ == "__main__":
    main()
