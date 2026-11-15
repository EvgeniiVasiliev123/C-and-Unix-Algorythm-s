"""Рисует граф дорог на карте (data/map.png), цветом показывает максимальную загрузку участков,
а также графики для отчёта. Запуск (из папки lab3-traffic, после ./traffic):  python3 tools/draw_map.py
Нужны пакеты: pillow, matplotlib."""
import csv
from PIL import Image, ImageDraw, ImageFont
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

SCALE = 2
nodes = {r["id"]: (float(r["x"]), float(r["y"])) for r in csv.DictReader(open("data/nodes.csv", encoding="utf-8"))}
edges = list(csv.DictReader(open("data/edges.csv", encoding="utf-8")))
load = {r["edge"]: float(r["peak_load_pct"]) for r in csv.DictReader(open("results/edges_baseline.csv", encoding="utf-8"))}
top = [r["edge"] for r in csv.DictReader(open("results/top10_baseline.csv", encoding="utf-8"))]

def color(p):               # зелёный -> жёлтый -> красный
    p = max(0.0, min(100.0, p)) / 100.0
    if p < 0.5:
        return (int(60 + 390 * p), 190, 60)
    return (230, int(190 - 380 * (p - 0.5)), 40)

def font(size):
    return ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", size)

def draw(with_load, path):
    im = Image.open("data/map.png").convert("RGB")
    im = im.resize((im.width * SCALE, im.height * SCALE), Image.LANCZOS)
    d = ImageDraw.Draw(im, "RGBA")
    for e in edges:
        a, b = e["a"], e["b"]
        (x1, y1), (x2, y2) = [(nodes[k][0] * SCALE, nodes[k][1] * SCALE) for k in (a, b)]
        dx, dy = x2 - x1, y2 - y1
        n = (dx * dx + dy * dy) ** 0.5 or 1
        ox, oy = -dy / n * 3.5, dx / n * 3.5        # смещение, чтобы два направления были видны рядом
        for sign, name in ((1, f"{a}->{b}"), (-1, f"{b}->{a}")):
            c = color(load[name]) if with_load else (40, 60, 200)
            w = 5 if (with_load and name in top) else 3
            d.line([(x1 + sign * ox, y1 + sign * oy), (x2 + sign * ox, y2 + sign * oy)], fill=c + (230,), width=w)
    f = font(13)
    for k, (x, y) in nodes.items():
        x, y = x * SCALE, y * SCALE
        d.ellipse([x - 4, y - 4, x + 4, y + 4], fill=(255, 255, 255, 255), outline=(0, 0, 0, 255))
        d.text((x + 6, y - 16), k, fill=(0, 0, 0, 255), font=f, stroke_width=2, stroke_fill=(255, 255, 255, 255))
    im.save(path)

draw(False, "doc/graph.png")
draw(True, "doc/graph_load.png")

# График загрузки TOP-5 участков в течение суток
rows = list(csv.reader(open("results/series_top10.csv", encoding="utf-8")))
hdr, data = rows[0], rows[1:]
fig, ax = plt.subplots(figsize=(10, 4.2))
for j in range(1, 6):
    ax.plot([int(r[0]) / 60 for r in data], [float(r[j]) for r in data], label=hdr[j], linewidth=1.6)
ax.axhline(90, color="red", linestyle="--", linewidth=1)
ax.text(0.2, 91.5, "порог 90%", color="red")
ax.set_xlim(5, 24); ax.set_ylim(0, 105); ax.set_xticks(range(5, 25, 2))
ax.set_xlabel("время суток, ч"); ax.set_ylabel("загрузка участка, %")
ax.set_title("Загрузка пяти самых загруженных участков"); ax.grid(alpha=0.3); ax.legend(fontsize=8)
fig.tight_layout(); fig.savefig("doc/top5_series.png", dpi=130)

# Сравнение сценариев
sc = list(csv.DictReader(open("results/scenarios.csv", encoding="utf-8")))
labels = [r["scenario"][:2] for r in sc]
fig, axs = plt.subplots(1, 2, figsize=(10, 3.8))
axs[0].bar(labels, [float(r["mean_travel_min"]) for r in sc], color="#4477aa", label="среднее")
axs[0].plot(labels, [float(r["p95_travel_min"]) for r in sc], "o-", color="#cc3311", label="95-й процентиль")
axs[0].set_title("Время в пути, мин"); axs[0].legend(fontsize=8); axs[0].grid(alpha=0.3)
axs[1].bar(labels, [int(r["segment_minutes_above_90"]) for r in sc], color="#cc6677")
axs[1].set_title("Минут загрузки >90% (сумма по участкам)"); axs[1].grid(alpha=0.3)
fig.tight_layout(); fig.savefig("doc/scenarios.png", dpi=130)
print("ok")
