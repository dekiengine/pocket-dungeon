"""Makes the pictures of things once they are used up, from the pictures of
them whole: the berries off the bush, the caps off the mushrooms, the pearl
out of the clam, the flames off the candles and the camp, the gem out of the
sentinel's eye, the weapons off the rack, the fish out of the basket, the
eyes out of the cage, the hum out of the crystal, the water out of the
fountain. Each is written beside its source as <name>_used.png (the
fountain as fountain_dry.png).

Usage: python tools/make_used.py <raw art folder>
"""

import colorsys
import pathlib
import sys

from PIL import Image

CLEAR = (0, 0, 0, 0)


def load(path):
    return Image.open(path).convert("RGBA")


def recolour(image, test, to, rows=None):
    """Every opaque pixel `test` likes (in the first `rows` rows, or all)
    becomes `to` (a colour, or CLEAR)."""
    out = image.copy()
    px = out.load()
    for y in range(rows if rows else out.height):
        for x in range(out.width):
            r, g, b, a = px[x, y]
            if a and test(r, g, b):
                px[x, y] = to(r, g, b) if callable(to) else to
    return out


def fill_holes(image, colour):
    """Clear pixels with the picture above and below them in their column
    are inside it: an opening, dark."""
    out = image.copy()
    px = out.load()
    for x in range(out.width):
        rows = [y for y in range(out.height) if px[x, y][3]]
        if not rows:
            continue
        for y in range(rows[0], rows[-1]):
            if not px[x, y][3]:
                px[x, y] = colour
    return out


def saturation(r, g, b):
    return colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)[1]


def grey(r, g, b):
    v = int((0.30 * r + 0.59 * g + 0.11 * b) * 0.6)
    return (v, v, int(v * 1.08) if v * 1.08 < 255 else 255, 255)


def main():
    raw = pathlib.Path(sys.argv[1])

    def save(image, name):
        image.save(raw / name)

    black = lambda r, g, b: r < 40 and g < 40 and b < 40
    # The bush: its berries picked, leaves where they were.
    save(recolour(load(raw / "prop_bush.png"), lambda r, g, b: r > 140 and g < 100 and b < 110, (62, 104, 58, 255)),
         "prop_bush_used.png")
    # The mushrooms: the caps cut, the stems left.
    save(recolour(load(raw / "prop_mushrooms.png"), lambda r, g, b: b > r + 20 and g > r,
                  lambda r, g, b: (int(g * 0.55), int(g * 0.46), int(g * 0.40), 255)),
         "prop_mushrooms_used.png")
    # The clam: no pearl in the dark of it.
    save(recolour(load(raw / "prop_clam.png"), lambda r, g, b: r > 200 and g > 170 and b > 170 and saturation(r, g, b) < 0.2,
                  (88, 30, 52, 255)),
         "prop_clam_used.png")
    # The candles and the camp: out.
    save(recolour(load(raw / "prop_candles.png"), lambda r, g, b: r > 170 and r - b > 60, CLEAR, rows=9),
         "prop_candles_used.png")
    fire = lambda r, g, b: r > 160 and (saturation(r, g, b) > 0.85 or (g > 200 and b < 150))
    save(recolour(load(raw / "prop_camp.png"), fire, (84, 78, 74, 255)), "prop_camp_used.png")
    # The sentinel: an empty eye.
    save(recolour(load(raw / "prop_statue.png"), lambda r, g, b: r > 200 and g < 160 and r - g > 60, (36, 38, 44, 255)),
         "prop_statue_used.png")
    # The crystal: grey, the hum gone out of it.
    save(recolour(load(raw / "prop_crystal.png"), lambda r, g, b: not black(r, g, b), grey), "prop_crystal_used.png")
    # The rack: bare wood.
    rack = recolour(load(raw / "prop_rack.png"), lambda r, g, b: not black(r, g, b) and saturation(r, g, b) < 0.22,
                    CLEAR)
    save(rack, "prop_rack_used.png")
    # The basket: the fish gone, the dark of the basket where they lay.
    basket = recolour(load(raw / "prop_basket.png"),
                      lambda r, g, b: not black(r, g, b) and (b > r + 8 or saturation(r, g, b) < 0.18), CLEAR)
    save(fill_holes(basket, (52, 30, 22, 255)), "prop_basket_used.png")
    # The cage: nothing looking out.
    save(recolour(load(raw / "prop_cage.png"), lambda r, g, b: r > 170 and g > 170 and b > 150, (30, 34, 40, 255)),
         "prop_cage_used.png")
    # The fountain: dry stone.
    save(recolour(load(raw / "fountain.png"), lambda r, g, b: b > r + 22 and b > 110, (74, 76, 86, 255)),
         "fountain_dry.png")
    # The plain chest: the locked one without its padlock (the planks to the
    # left of it copied over it), and a small clasp instead.
    chest = load(raw / "prop_lockedchest.png")
    px = chest.load()
    for y in range(13, 25):
        for x in range(12, 19):
            px[x, y] = px[x - 7, y]
    for y in range(17, 20):
        for x in range(14, 17):
            px[x, y] = (214, 170, 60, 255) if (x, y) != (15, 18) else (40, 30, 24, 255)
    save(chest, "prop_oldchest.png")
    print("made the used pictures in", raw)


if __name__ == "__main__":
    main()
