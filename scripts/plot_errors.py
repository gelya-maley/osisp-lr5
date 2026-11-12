import csv
import os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(ROOT, "data", "experiments.csv")
OUT = os.path.join(ROOT, "report", "img")

LABELS = {"A": "100 запр.,\n1 клиент", "B": "500 запр.,\n1 клиент",
          "C": "100 запр.,\n3 клиента", "D": "500 запр.,\n3 клиента"}


def load():
    with open(DATA, encoding="utf-8") as f:
        return list(csv.DictReader(f))


def main():
    os.makedirs(OUT, exist_ok=True)
    rows = load()
    cfgs = ["A", "B", "C", "D"]

    def mean(cfg, key):
        vals = [float(r[key]) for r in rows if r["config"] == cfg]
        return sum(vals) / len(vals)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11, 5), dpi=200)
    x = range(len(cfgs))
    w = 0.35
    to = [mean(c, "timeouts") for c in cfgs]
    er = [mean(c, "errors") for c in cfgs]
    ax1.bar([i - w / 2 for i in x], to, w, color="white", edgecolor="black",
            hatch="//", label="Тайм-ауты")
    ax1.bar([i + w / 2 for i in x], er, w, color="0.75", edgecolor="black",
            label="Ошибки ответа")
    ax1.set_xticks(list(x))
    ax1.set_xticklabels([LABELS[c] for c in cfgs])
    ax1.set_ylabel("Число событий за прогон")
    ax1.set_title("Тайм-ауты и ошибки по конфигурациям\n(среднее по 3 запускам)")
    ax1.legend(frameon=True, edgecolor="black")
    ax1.grid(axis="y", alpha=0.3, color="0.7")

    bs = [mean(c, "bytesSent") for c in cfgs]
    br = [mean(c, "bytesRecv") for c in cfgs]
    ax2.bar([i - w / 2 for i in x], bs, w, color="white", edgecolor="black",
            hatch="xx", label="Передано байт")
    ax2.bar([i + w / 2 for i in x], br, w, color="0.75", edgecolor="black",
            label="Принято байт")
    ax2.set_xticks(list(x))
    ax2.set_xticklabels([LABELS[c] for c in cfgs])
    ax2.set_ylabel("Объём данных за прогон, байт")
    ax2.set_title("Объём переданных и принятых данных\nпо конфигурациям")
    ax2.legend(frameon=True, edgecolor="black")
    ax2.grid(axis="y", alpha=0.3, color="0.7")

    fig.tight_layout()
    fig.savefig(os.path.join(OUT, "plot_errors.png"), facecolor="white")
    print("saved plot_errors.png")


if __name__ == "__main__":
    main()
