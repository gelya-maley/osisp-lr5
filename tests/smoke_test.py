import math
import socket
import subprocess
import sys
import threading
import time

HOST = "127.0.0.1"
PORT = 20071
SERVER = r"..\build\server.exe"
results = []


def check(name, cond):
    results.append((name, bool(cond)))
    print(("PASS " if cond else "FAIL ") + name)


def ask(sock, text, timeout=2.0):
    sock.settimeout(timeout)
    sock.sendto(text.encode(), (HOST, PORT))
    data, _ = sock.recvfrom(65536)
    return data.decode()


def parse(msg):
    parts = msg.split(" ")
    fields = {}
    for p in parts[1:]:
        if "=" in p:
            k, v = p.split("=", 1)
            fields[k] = v
    return parts[0], fields


def main():
    srv = subprocess.Popen([SERVER, "--host", HOST, "--port", str(PORT), "--log", "server_test.log"],
                           stdout=subprocess.DEVNULL, cwd=".")
    time.sleep(1.0)
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        _, f = parse(ask(s, "RECTANGLE requestId=1 a=2 b=3"))
        check("RECTANGLE 2x3 area=6 perimeter=10",
              abs(float(f["area"]) - 6) < 1e-6 and abs(float(f["perimeter"]) - 10) < 1e-6)

        _, f = parse(ask(s, "CIRCLE requestId=2 r=1"))
        check("CIRCLE r=1 area=pi circ=2pi",
              abs(float(f["area"]) - math.pi) < 1e-6 and abs(float(f["circumference"]) - 2 * math.pi) < 1e-6)

        _, f = parse(ask(s, "TRIANGLE requestId=3 a=3 b=4 c=5"))
        check("TRIANGLE 3-4-5 area=6", abs(float(f["area"]) - 6) < 1e-6)

        cmd, f = parse(ask(s, "TRIANGLE requestId=4 a=1 b=2 c=10"))
        check("impossible triangle ERROR", cmd == "ERROR" and f["code"] == "IMPOSSIBLE_TRIANGLE")

        cmd, f = parse(ask(s, "RECTANGLE requestId=5 a=-1 b=2"))
        check("negative side ERROR BAD_DIMENSIONS", cmd == "ERROR" and f["code"] == "BAD_DIMENSIONS")

        cmd, f = parse(ask(s, "CIRCLE requestId=6"))
        check("missing field ERROR", cmd == "ERROR" and f["code"] == "MISSING_FIELD")

        cmd, f = parse(ask(s, "FOO requestId=7"))
        check("unknown command ERROR", cmd == "ERROR" and f["code"] == "UNKNOWN_COMMAND")

        cmd, f = parse(ask(s, "RECTANGLE requestId=8 a=xx b=2"))
        check("bad number ERROR", cmd == "ERROR" and f["code"] == "BAD_NUMBER")

        cmd, f = parse(ask(s, "X" * 1500))
        check("oversized datagram ERROR MSG_TOO_LARGE", cmd == "ERROR" and f.get("code") == "MSG_TOO_LARGE")

        dead = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        dead.settimeout(0.7)
        dead.sendto(b"RECTANGLE requestId=99 a=1 b=1", (HOST, PORT + 1))
        try:
            dead.recvfrom(65536)
            no_reply = False
        except (socket.timeout, ConnectionResetError):
            no_reply = True
        check("no response when server unavailable", no_reply)
        dead.close()

        def worker(wid, out):
            ws = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            okc = 0
            for i in range(30):
                ws.settimeout(2.0)
                ws.sendto(f"RECTANGLE requestId={wid * 1000 + i} a=2 b=2".encode(), (HOST, PORT))
                try:
                    data, _ = ws.recvfrom(65536)
                    c, ff = parse(data.decode())
                    if c == "OK" and ff.get("requestId") == str(wid * 1000 + i):
                        okc += 1
                except socket.timeout:
                    pass
            out[wid] = okc
            ws.close()

        outs = {}
        threads = [threading.Thread(target=worker, args=(w, outs)) for w in (1, 2, 3)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()
        check("three concurrent clients served", all(outs.get(w) == 30 for w in (1, 2, 3)))

        s.close()
    finally:
        srv.send_signal(1)
        try:
            srv.wait(timeout=5)
        except subprocess.TimeoutExpired:
            srv.kill()

    failed = [n for n, ok in results if not ok]
    print(f"\nTOTAL {len(results)} PASSED {len(results) - len(failed)} FAILED {len(failed)}")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
