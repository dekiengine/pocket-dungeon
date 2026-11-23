"""Cuts a sprite down to a few colours, keeping its accents.

A plain median cut spends its colours on the big areas and loses the small
bright ones (eyes, fire, gems). So: a median cut for most of the palette, then
the colours furthest from it (by how far, a little by how many pixels) added
one at a time, and every pixel mapped to the nearest. Used by bake_textures.py.
"""

from PIL import Image


def _distance(a, b):
    # Weighted RGB distance: the eye is most sensitive to green.
    return 2 * (a[0] - b[0]) ** 2 + 4 * (a[1] - b[1]) ** 2 + 3 * (a[2] - b[2]) ** 2


def reduce_colours(image, colours, accents=3):
    """`image` RGBA; returns RGBA with at most `colours` opaque colours."""
    image = image.convert("RGBA")
    pixels = list(image.getdata())
    opaque = [p[:3] for p in pixels if p[3] >= 128]
    if not opaque:
        return image
    counts = {}
    for p in opaque:
        counts[p] = counts.get(p, 0) + 1
    if len(counts) <= colours:
        return image

    base = max(1, colours - accents)
    strip = Image.new("RGB", (len(opaque), 1))
    strip.putdata(opaque)
    quantized = strip.quantize(colors=base, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    flat = quantized.getpalette()[: 3 * base]
    palette = [tuple(flat[i:i + 3]) for i in range(0, len(flat), 3)]

    while len(palette) < colours:
        best = None
        best_score = 0
        for colour, count in counts.items():
            if count < 2:
                continue
            gap = min(_distance(colour, p) for p in palette)
            score = gap * (count ** 0.25)
            if score > best_score:
                best, best_score = colour, score
        if best is None:
            break
        palette.append(best)

    nearest = {c: min(palette, key=lambda p: _distance(c, p)) for c in counts}
    out = [(*nearest[p[:3]], 255) if p[3] >= 128 else (0, 0, 0, 0) for p in pixels]
    result = Image.new("RGBA", image.size)
    result.putdata(out)
    return result


def clean_speckles(image, passes=2):
    """Removes lone pixels: an opaque pixel whose colour none of its four
    neighbours share takes the colour most of them have. Leaves the dark
    outline alone, so the shape keeps its edge."""
    image = image.convert("RGBA")
    w, h = image.size
    for _ in range(passes):
        src = image.load()
        out = image.copy()
        dst = out.load()
        for y in range(h):
            for x in range(w):
                p = src[x, y]
                if p[3] < 128 or sum(p[:3]) < 90:
                    continue
                around = [src[nx, ny] for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1))
                          if 0 <= nx < w and 0 <= ny < h]
                opaque = [q for q in around if q[3] >= 128 and sum(q[:3]) >= 90]
                if not opaque or any(q[:3] == p[:3] for q in around):
                    continue
                counts = {}
                for q in opaque:
                    counts[q] = counts.get(q, 0) + 1
                best = max(counts.items(), key=lambda kv: kv[1])
                if best[1] >= 2:
                    dst[x, y] = best[0]
        image = out
    return image
