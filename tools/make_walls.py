"""Makes the pictures that are worked out from other pictures of walls:

- The village's upper storeys, filled in down to the wall they stand on. The
  art is a whole little house with empty sides and an empty foot, so a storey
  drawn with it floated over its wall; here the empty part below each
  column's first pixel takes the wall's own posts and beams.
- Each zone's cracked wall after some blows, and after more: longer cracks,
  then stones knocked out of it.

Run: python tools/make_walls.py <art folder>   (before bake_textures.py)
"""

import math
import random
import sys
from pathlib import Path

from PIL import Image

# The storey, the wall whose pixels fill it in, and how far down that wall
# the filling starts (so a storey's foot takes the wall's top beam).
TOPS = [
    ("v_top_a.png", "v_wall.png", 2),
    ("v_top_b.png", "v_wall.png", 2),
    ("v_top_ruin.png", "v_ruin.png", 8),
    ("v_top_tower.png", "v_tower.png", 8),
]
CRACKED = ["wall64_cracked.png", "wall64_bone_cracked.png", "wall64_wet_cracked.png", "wall64_forge_cracked.png"]


def fill_top(folder, top, wall, shift):
    image = Image.open(folder / top).convert("RGBA")
    backing = Image.open(folder / wall).convert("RGBA")
    width, height = image.size
    pixels = image.load()
    behind = backing.load()
    # Each column's first pixel from the top; an empty column takes its
    # nearest neighbour's.
    first = []
    for x in range(width):
        rows = [y for y in range(height) if pixels[x, y][3] > 0]
        first.append(rows[0] if rows else None)
    known = [x for x in range(width) if first[x] is not None]
    for x in range(width):
        if first[x] is None:
            first[x] = first[min(known, key=lambda k: abs(k - x))]
    for x in range(width):
        for y in range(first[x], height):
            if pixels[x, y][3] == 0:
                r, g, b, _ = behind[x, (y + shift) % height]
                pixels[x, y] = (r, g, b, 255)
    image.save(folder / top.replace(".png", "_full.png"))


def crack(pixels, size, rng, start, angle, length, dark, light):
    """A crack: a line that wanders, dark, with a lit edge under it."""
    x, y = start
    for _ in range(length):
        angle += rng.uniform(-0.7, 0.7)
        x += math.cos(angle)
        y += math.sin(angle)
        ix, iy = int(round(x)), int(round(y))
        if not (0 <= ix < size and 0 <= iy < size):
            break
        pixels[ix, iy] = dark
        if rng.random() < 0.35 and ix + 1 < size:
            pixels[ix + 1, iy] = dark
        if iy + 1 < size and pixels[ix, iy + 1] != dark and rng.random() < 0.5:
            pixels[ix, iy + 1] = light(pixels[ix, iy + 1])


def hole(pixels, size, rng, centre, radius, dark, light):
    """A stone knocked out: a ragged dark hole, deepest in its middle."""
    cx, cy = centre
    edge = [radius * rng.uniform(0.65, 1.2) for _ in range(12)]
    for y in range(size):
        for x in range(size):
            dx, dy = x - cx, y - cy
            d = math.hypot(dx, dy)
            reach = edge[int((math.atan2(dy, dx) + math.pi) / (2 * math.pi) * 12) % 12]
            if d < reach * 0.7:
                pixels[x, y] = (6, 6, 9, 255)
            elif d < reach:
                pixels[x, y] = dark
            elif d < reach + 1.2 and dy > 0:
                pixels[x, y] = light(pixels[x, y])  # the lower lip catches the light


def damage(folder, name, seed):
    source = Image.open(folder / name).convert("RGBA")
    size = source.size[0]
    colours = sorted({p[:3] for p in source.getdata()}, key=sum)
    darkest = colours[0]
    dark = (darkest[0] * 2 // 3, darkest[1] * 2 // 3, darkest[2] * 2 // 3, 255)

    def light(p):
        return (min(255, p[0] + 36), min(255, p[1] + 36), min(255, p[2] + 34), 255)

    for stage in (1, 2):
        rng = random.Random(seed * 10 + stage)
        image = source.copy()
        pixels = image.load()
        middle = (size // 2 + rng.randint(-3, 3), size // 2 + rng.randint(-3, 3))
        for k in range(4 if stage == 1 else 7):
            angle = k * (2 * math.pi / (4 if stage == 1 else 7)) + rng.uniform(-0.4, 0.4)
            crack(pixels, size, rng, middle, angle, rng.randint(14, 22) if stage == 1 else rng.randint(20, 34), dark,
                  light)
        hole(pixels, size, rng, middle, 3.5 if stage == 1 else 8.5, dark, light)
        if stage == 2:
            hole(pixels, size, rng, (size // 4 + rng.randint(-3, 3), size // 3 + rng.randint(-3, 3)), 5.5, dark, light)
            hole(pixels, size, rng, (size * 3 // 4 + rng.randint(-3, 3), size * 2 // 3 + rng.randint(-3, 3)), 6.5, dark,
                 light)
            hole(pixels, size, rng, (size * 2 // 3 + rng.randint(-2, 2), size // 5), 3.5, dark, light)
        image.save(folder / name.replace(".png", f"_{stage}.png"))


def canopy(folder):
    """The cloth over the alley, seen from below: wide dull stripes, the
    weave, a seam and a pole across, a few patches. It repeats tile to tile."""
    size = 32
    rng = random.Random(77)
    image = Image.new("RGBA", (size, size))
    pixels = image.load()
    for y in range(size):
        for x in range(size):
            red = (x // 8) % 2 == 0
            r, g, b = (96, 50, 44) if red else (78, 68, 58)
            weave = 6 if (x + y) % 2 == 0 else -4
            sag = -10 if y % 16 in (0, 15) else 0  # a seam, where two cloths meet
            n = rng.randint(-4, 4)
            pixels[x, y] = (max(0, r + weave + sag + n), max(0, g + weave + sag + n), max(0, b + weave + sag + n), 255)
    for x in range(size):  # a pole across, holding it up
        for y, shade in ((7, 52), (8, 70), (9, 44)):
            pixels[x, y] = (shade + 18, shade + 6, shade - 8, 255)
    for px, py in ((5, 20), (22, 25)):  # patches, sewn on
        for y in range(py, py + 5):
            for x in range(px, px + 6):
                edge = y in (py, py + 4) or x in (px, px + 5)
                pixels[x, y] = (58, 50, 46, 255) if edge else (112, 96, 74, 255)
    image.save(folder / "v_canopy.png")


def main():
    folder = Path(sys.argv[1])
    canopy(folder)
    for top, wall, shift in TOPS:
        fill_top(folder, top, wall, shift)
    for seed, name in enumerate(CRACKED):
        damage(folder, name, seed + 1)
    print("made", len(TOPS), "storeys and", len(CRACKED) * 2, "broken walls")


if __name__ == "__main__":
    main()
