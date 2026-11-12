import csv
import os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(ROOT, "data", "experiments.csv")
OUT = os.path.join(ROOT, "report", "img")


def load():
    with open(DATA, encoding="utf-8") as f:
        return list(csv.DictReader(f))


def main():
    os.makedirs(OUT, exist_ok=True)
    rows = load()

    def mean(cfg, key):
        vals = [float(r[key]) for r in rows if r["config"] == cfg]
        return sum(vals) / len(vals)

    reqs = [100, 500]
    rtt_one = [mean("A", "avgRttMs"), mean("B", "avgRttMs")]
    rtt_three = [mean("C", "avgRttMs"), mean("D", "avgRttMs")]
    thr_one = [mean("A", "throughput"), mean("B", "throughput")]
    thr_three = [mean("C", "throughput"), mean("D", "throughput")]

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11, 5), dpi=200)
    ax1.plot(reqs, rtt_one, marker="o", color="black", linestyle="-", label="1 клиент")
    ax1.plot(reqs, rtt_three, marker="s", color="black", linestyle="--", label="3 клиента")
    ax1.set_xlabel("Число запросов на клиента")
    ax1.set_ylabel("Средний RTT запроса, мс")
    ax1.set_title("Средний RTT от числа запросов\nпри одном и трёх клиентах")
    ax1.legend(frameon=True, edgecolor="black")
    ax1.grid(alpha=0.3, color="0.7")

    ax2.plot(reqs, thr_one, marker="o", color="black", linestyle="-", label="1 клиент")
    ax2.plot(reqs, thr_three, marker="s", color="black", linestyle="--", label="3 клиента")
    ax2.set_xlabel("Число запросов на клиента")
    ax2.set_ylabel("Пропускная способность, запросов/с")
    ax2.set_title("Пропускная способность от числа запросов\nпри одном и трёх клиентах")
    ax2.legend(frameon=True, edgecolor="black")
    ax2.grid(alpha=0.3, color="0.7")

    fig.tight_layout()
    fig.savefig(os.path.join(OUT, "plot_rtt.png"), facecolor="white")
    print("saved plot_rtt.png")


if __name__ == "__main__":
    main()
