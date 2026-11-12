import os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "report", "img")


def box(ax, x, y, w, h, text, fs=9):
    b = FancyBboxPatch((x, y), w, h, boxstyle="square,pad=0.04", fc="white", ec="black", lw=1.1)
    ax.add_patch(b)
    ax.text(x + w / 2, y + h / 2, text, ha="center", va="center", fontsize=fs)


def arrow(ax, x1, y1, x2, y2, label="", lx=0.0, ly=0.08):
    a = FancyArrowPatch((x1, y1), (x2, y2), arrowstyle="-|>", mutation_scale=13,
                        lw=1.0, color="black")
    ax.add_patch(a)
    if label:
        ax.text((x1 + x2) / 2 + lx, (y1 + y2) / 2 + ly, label, ha="center", fontsize=8)


def architecture():
    fig, ax = plt.subplots(figsize=(10.5, 6.8), dpi=200)
    box(ax, 0.4, 5.2, 2.4, 0.8, "client.exe 1..3\n--host --port --id\n--requests --timeout")
    box(ax, 3.9, 5.2, 2.6, 0.8, "Приём датаграмм\nrecvfrom, select,\nSO_RCVTIMEO")
    box(ax, 7.6, 5.2, 2.6, 0.8, "Разбор запроса\nrequestId, kind,\nпроверка полей")
    box(ax, 7.6, 3.9, 2.6, 0.8, "Вычисление и проверки\nRECTANGLE, CIRCLE,\nTRIANGLE, неравенства")
    box(ax, 3.9, 3.9, 2.6, 0.8, "Формирование ответа\nOK с результатом или\nERROR с кодом")
    box(ax, 3.9, 2.6, 2.6, 0.8, "Отправка датаграммы\nsendto, контроль\nWSAGetLastError")
    box(ax, 0.4, 2.6, 2.4, 0.8, "Журнал событий\nвремя, роль, peer,\nrequestId, результат")
    box(ax, 7.6, 2.6, 2.6, 0.8, "UDP-сокет сервера\nbind 127.0.0.1:20071,\nдатаграмма <= 1200 Б")
    arrow(ax, 2.8, 5.6, 3.9, 5.6, "запрос")
    arrow(ax, 6.5, 5.6, 7.6, 5.6)
    arrow(ax, 8.9, 5.2, 8.9, 4.7)
    arrow(ax, 7.6, 4.3, 6.5, 4.3)
    arrow(ax, 5.2, 3.9, 5.2, 3.4)
    arrow(ax, 3.9, 3.0, 2.8, 3.0, "события")
    arrow(ax, 6.5, 3.0, 7.6, 3.0, "ответ")
    arrow(ax, 8.9, 2.6, 8.9, 2.2)
    ax.text(7.6, 2.0, "один сокет обслуживает всех клиентов", fontsize=8)
    ax.set_xlim(0, 10.6)
    ax.set_ylim(1.7, 6.3)
    ax.axis("off")
    ax.set_title("Архитектура геометрической вычислительной службы (UDP)", fontsize=11)
    fig.tight_layout()
    fig.savefig(os.path.join(OUT, "architecture.png"), facecolor="white", bbox_inches="tight")
    plt.close(fig)
    print("saved architecture.png")


def sequence():
    actors = ["Клиент 1", "Клиент 2", "Сервер UDP"]
    xs = [1.5, 5.0, 8.5]
    msgs = [
        (0, 2, "RECTANGLE requestId=1 a=2 b=3", False),
        (2, 0, "OK requestId=1 area=6 perimeter=10", True),
        (1, 2, "TRIANGLE requestId=2 a=1 b=2 c=10", False),
        (2, 1, "ERROR requestId=2 code=IMPOSSIBLE_TRIANGLE", True),
        (0, 2, "CIRCLE requestId=3 r=-1", False),
        (2, 0, "ERROR requestId=3 code=BAD_DIMENSIONS", True),
        (1, 2, "FOO requestId=4", False),
        (2, 1, "ERROR requestId=4 code=UNKNOWN_COMMAND", True),
        (0, None, "запрос на закрытый порт: ответ не приходит, SO_RCVTIMEO", None),
        (0, 2, "DISCONNECT не требуется: сеанс завершает клиент", False),
    ]
    fig, ax = plt.subplots(figsize=(10.5, 8.2), dpi=200)
    y1 = len(msgs) * 0.55 + 1.6
    for x, name in zip(xs, actors):
        b = FancyBboxPatch((x - 0.95, y1 - 0.55), 1.9, 0.5, boxstyle="square,pad=0.03",
                           fc="white", ec="black", lw=1.1)
        ax.add_patch(b)
        ax.text(x, y1 - 0.3, name, ha="center", va="center", fontsize=9)
        ax.plot([x, x], [0.4, y1 - 0.6], color="black", lw=0.7, linestyle=(0, (5, 4)))
    for i, (a, b, label, dashed) in enumerate(msgs):
        y = y1 - 1.1 - i * 0.55
        if dashed is None:
            ax.plot([xs[a] + 0.12, xs[a] + 0.12], [y, y - 0.32], color="black", lw=1.0)
            ax.text(xs[a] + 0.25, y - 0.16, label, ha="left", va="center", fontsize=7.5, style="italic")
            continue
        xa, xb = xs[a], xs[b]
        ar = FancyArrowPatch((xa, y), (xb, y), arrowstyle="-|>", mutation_scale=12, lw=1.0,
                             color="black", linestyle="--" if dashed else "-")
        ax.add_patch(ar)
        ax.text((xa + xb) / 2, y + 0.05, label, ha="center", va="bottom", fontsize=7.5)
    ax.set_xlim(0, 10.2)
    ax.set_ylim(0.2, y1 + 0.2)
    ax.axis("off")
    ax.set_title("Диаграмма последовательности обмена датаграммами (вариант 18)", fontsize=11)
    fig.tight_layout()
    fig.savefig(os.path.join(OUT, "sequence.png"), facecolor="white", bbox_inches="tight")
    plt.close(fig)
    print("saved sequence.png")


def main():
    os.makedirs(OUT, exist_ok=True)
    architecture()
    sequence()


if __name__ == "__main__":
    main()
