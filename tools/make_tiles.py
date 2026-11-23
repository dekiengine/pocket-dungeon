"""Makes the tiles that are drawn from other art, into the art folder:

- wall64_cracked.png: the stone wall with cracks, the way into a secret room;
- floor32_hole.png: the floor with a stairwell going down in the middle;
- floor32_spikes.png: the floor with a spike trap;
- wall64_<zone>_echo.png: each zone's wall with an echo stone set in it.

Run before tools/bake_textures.py: python tools/make_tiles.py <art folder>
"""

import random
import sys
from pathlib import Path

from PIL import Image


def crack(image, points, dark, light):
    """A jagged crack through `points`, lit on one side."""
    px = image.load()
    w, h = image.size
    for (x0, y0), (x1, y1) in zip(points, points[1:]):
        steps = max(abs(x1 - x0), abs(y1 - y0))
        for i in range(steps + 1):
            x = round(x0 + (x1 - x0) * i / steps)
            y = round(y0 + (y1 - y0) * i / steps)
            if 0 <= x < w and 0 <= y < h:
                px[x, y] = dark
            if 0 <= x + 1 < w and 0 <= y < h and px[x + 1, y] != dark:
                px[x + 1, y] = light


def cracked_wall(art, source="wall64_stone.png", out="wall64_cracked.png"):
    wall = Image.open(art / source).convert("RGBA")
    dark = (18, 20, 22, 255)
    light = (150, 156, 150, 255)
    rng = random.Random(7)
    # One long crack down the middle with branches, and a few short ones.
    trunk = [(30, 2)]
    while trunk[-1][1] < 62:
        x, y = trunk[-1]
        trunk.append((max(4, min(60, x + rng.randint(-4, 4))), y + rng.randint(3, 6)))
    crack(wall, trunk, dark, light)
    for start in (10, 22, 38, 50):
        x, y = trunk[min(len(trunk) - 1, start // 5)]
        branch = [(x, y)]
        direction = -1 if start % 20 < 10 else 1
        for _ in range(rng.randint(3, 5)):
            bx, by = branch[-1]
            branch.append((bx + direction * rng.randint(2, 5), by + rng.randint(-2, 3)))
        crack(wall, branch, dark, light)
    # Grit fallen at the foot of the wall.
    px = wall.load()
    for _ in range(40):
        x = rng.randint(14, 50)
        y = rng.randint(56, 63)
        px[x, y] = (70, 72, 70, 255) if rng.random() < 0.6 else dark
    wall.save(art / out)


def tinted(colour, toward, amount):
    return tuple(int(c + (t - c) * amount) for c, t in zip(colour[:3], toward)) + (255,)


def hole_floor(art):
    """A square stairwell cut into the floor: lit steps going down into the
    dark, so it reads as a way down even from a few tiles off."""
    out = Image.open(art / "floor32b.png").convert("RGBA")
    px = out.load()
    left, top, right, bottom = 3, 3, 28, 28
    steps = 6
    for y in range(top, bottom + 1):
        for x in range(left, right + 1):
            # Down the tile the steps sink: each one darker than the last.
            depth = (y - top) / (bottom - top)
            step = min(steps - 1, int(depth * steps))
            within = (y - top) - step * (bottom - top + 1) / steps
            shade = int(170 * (1.0 - step / steps) ** 1.6)
            if within < 1.2:
                shade = min(255, shade + 55)  # the lit nose of the step
            elif within > 2.8:
                shade = shade // 3  # the shadow under it
            # The stairwell's side walls, darker.
            if x - left < 3 or right - x < 3:
                shade = shade // 2
            px[x, y] = (shade * 92 // 100, shade, shade * 96 // 100, 255)
    # A rim: lit on the near edge, a dark lip on the far one.
    for i in range(left - 1, right + 2):
        px[i, top - 1] = (18, 22, 24, 255)
        px[i, bottom + 1] = (150, 182, 176, 255)
    for i in range(top - 1, bottom + 2):
        px[left - 1, i] = (18, 22, 24, 255)
        px[right + 1, i] = (18, 22, 24, 255)
    out.save(art / "floor32_hole.png")


def spikes_floor(art):
    floor = Image.open(art / "floor32b.png").convert("RGBA")
    px = floor.load()
    # A darker plate with a rusty edge, and a grid of spikes on it.
    for y in range(4, 28):
        for x in range(4, 28):
            r, g, b, a = px[x, y]
            edge = x in (4, 27) or y in (4, 27)
            px[x, y] = (120, 60, 40, 255) if edge else (r * 55 // 100, g * 55 // 100, b * 55 // 100, 255)
    for cy in (9, 16, 23):
        for cx in (9, 16, 23):
            px[cx + 1, cy + 1] = (20, 20, 24, 255)  # shadow
            px[cx, cy + 1] = (40, 40, 46, 255)
            px[cx + 1, cy] = (40, 40, 46, 255)
            px[cx - 1, cy] = (150, 152, 160, 255)
            px[cx, cy - 1] = (150, 152, 160, 255)
            px[cx, cy] = (235, 238, 245, 255)  # the point
    floor.save(art / "floor32_spikes.png")


def rune_floor(art):
    """The mark under an impossible door: a glowing violet ring of runes."""
    floor = Image.open(art / "floor32b.png").convert("RGBA")
    px = floor.load()
    import math
    for y in range(32):
        for x in range(32):
            d = math.hypot(x - 15.5, y - 15.5)
            r, g, b, a = px[x, y]
            if 9.0 <= d <= 11.5:
                px[x, y] = (200, 120, 255, 255) if (int(math.atan2(y - 15.5, x - 15.5) * 6) % 2) == 0 else (130, 70, 200, 255)
            elif d < 9.0:
                k = 0.6 + 0.4 * (d / 9.0)
                px[x, y] = (int(r * k + 60 * (1 - k)), int(g * k + 20 * (1 - k)), int(b * k + 90 * (1 - k)), 255)
            elif d < 13.0:
                px[x, y] = (min(255, r + 25), g, min(255, b + 40), 255)
    floor.save(art / "floor32_rune.png")


def echo_wall(art, source, out):
    """An echo stone: the zone's wall with a dark tablet set in it, glowing
    cyan runes on the tablet and a faint glow round it."""
    import math
    wall = Image.open(art / source).convert("RGBA")
    px = wall.load()
    x0, y0, x1, y1 = 18, 10, 46, 52
    # The glow on the stone round the tablet.
    for y in range(64):
        for x in range(64):
            dx = max(x0 - x, 0, x - x1)
            dy = max(y0 - y, 0, y - y1)
            d = math.hypot(dx, dy)
            if 0 < d < 6:
                r, g, b, a = px[x, y]
                k = (6 - d) / 6 * 0.45
                px[x, y] = (int(r + (90 - r) * k), int(g + (220 - g) * k), int(b + (255 - b) * k), 255)
    # The tablet: dark slate, a lit top edge.
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            edge = x in (x0, x1) or y in (y0, y1)
            px[x, y] = (14, 18, 26, 255) if edge else (30, 36, 48, 255)
    for x in range(x0 + 1, x1):
        px[x, y0 + 1] = (58, 68, 86, 255)
    # Runes: three rows of three glyphs, each a few strokes.
    rng = random.Random(11)
    bright = (120, 235, 255, 255)
    soft = (60, 150, 200, 255)
    strokes = [((0, 0), (4, 0)), ((0, 0), (0, 6)), ((4, 0), (4, 6)), ((0, 3), (4, 3)), ((0, 6), (4, 6)),
               ((0, 0), (4, 6)), ((4, 0), (0, 6)), ((2, 0), (2, 6))]
    for row in range(3):
        for col in range(3):
            gx = x0 + 4 + col * 8
            gy = y0 + 6 + row * 12
            for (ax, ay), (bx, by) in rng.sample(strokes, 3):
                steps = max(abs(bx - ax), abs(by - ay))
                for i in range(steps + 1):
                    x = gx + round(ax + (bx - ax) * i / steps)
                    y = gy + round(ay + (by - ay) * i / steps)
                    px[x, y] = bright
                    if px[x + 1, y + 1] != bright:
                        px[x + 1, y + 1] = soft
    wall.save(art / out)


def seamless_sky(art, source="v_sky.png", out="v_sky_seamless.png"):
    """The village sky, made to wrap round the viewer: its left half, then
    that half mirrored, so the two edges meet with no seam."""
    sky = Image.open(art / source).convert("RGBA")
    w, h = sky.size
    half = sky.crop((0, 0, w // 2, h))
    out_image = Image.new("RGBA", (w, h))
    out_image.paste(half, (0, 0))
    out_image.paste(half.transpose(Image.FLIP_LEFT_RIGHT), (w // 2, 0))
    out_image.save(art / out)


def glow(art, name, core, edge):
    """A tiny glowing dot for a particle: bright core, soft edge."""
    import math
    image = Image.new("RGBA", (8, 8), (0, 0, 0, 0))
    px = image.load()
    for y in range(8):
        for x in range(8):
            d = math.hypot(x - 3.5, y - 3.5)
            if d < 1.6:
                px[x, y] = core + (255,)
            elif d < 2.9:
                px[x, y] = edge + (255,)
    image.save(art / name)


def main():
    art = Path(sys.argv[1] if len(sys.argv) > 1 else r"C:\dkt\pocket-dungeon-art\raw")
    cracked_wall(art)
    for zone in ("bone", "wet", "forge"):
        if (art / f"wall64_{zone}.png").exists():
            cracked_wall(art, f"wall64_{zone}.png", f"wall64_{zone}_cracked.png")
    for zone in ("stone", "bone", "wet", "forge"):
        if (art / f"wall64_{zone}.png").exists():
            echo_wall(art, f"wall64_{zone}.png", f"wall64_{zone}_echo.png")
    if (art / "v_sky.png").exists():
        seamless_sky(art)
    hole_floor(art)
    spikes_floor(art)
    rune_floor(art)
    glow(art, "particle_ember.png", (255, 230, 120), (240, 110, 30))
    glow(art, "particle_drip.png", (200, 240, 255), (90, 150, 220))
    glow(art, "particle_dust.png", (230, 220, 190), (150, 140, 120))
    print("wrote wall64_cracked.png, floor32_hole.png, floor32_spikes.png, floor32_rune.png")


if __name__ == "__main__":
    main()
