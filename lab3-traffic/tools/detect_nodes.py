"""Находит на изображении карты синие кружки (перекрёстки) и печатает их центры в пикселях.
Запуск: python3 tools/detect_nodes.py data/map.png     Нужны пакеты: pillow, numpy, scipy."""
import sys
import numpy as np
from PIL import Image
from scipy import ndimage as ndi

img = np.array(Image.open(sys.argv[1] if len(sys.argv) > 1 else "data/map.png").convert("RGB")).astype(int)
r, g, b = img[..., 0], img[..., 1], img[..., 2]
mask = (b > 150) & (r < 90) & (g < 90)                 # "синие чернила"
mask = ndi.binary_dilation(mask, iterations=3)         # склеиваем разрывы линии
labels, n = ndi.label(mask)
for i, sl in enumerate(ndi.find_objects(labels), 1):
    ys, xs = sl
    if (labels[sl] == i).sum() < 40:                   # мелкий шум пропускаем
        continue
    print(f"{(xs.start + xs.stop) / 2:.0f},{(ys.start + ys.stop) / 2:.0f}")
