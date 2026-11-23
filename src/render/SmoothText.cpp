#include "SmoothText.h"

#include "../Hot.h"

#include <cstring>

namespace Dungeon::SmoothText
{

namespace
{

// How far in a row of a box with rounded corners starts (and ends) at
// `row`: 0 away from the corners. The same pixels as testing each one
// against the corner's circle, worked out once per row.
int Inset(int row, int y0, int y1, int radius)
{
    if (radius <= 0 || row < y0 || row >= y1)
    {
        return 0;
    }
    const int dy = row < y0 + radius ? y0 + radius - row : (row >= y1 - radius ? row - (y1 - radius - 1) : 0);
    if (dy <= 0)
    {
        return 0;
    }
    // The widest dx with dx * dx + dy * dy <= radius * radius.
    int dx = 0;
    while ((dx + 1) * (dx + 1) + dy * dy <= radius * radius)
    {
        ++dx;
    }
    return radius - dx;
}

}  // namespace

DUNGEON_HOT uint16_t Mix(uint16_t a, uint16_t b, int t256)
{
    // Green moved up out of the way of red and blue, so one multiply blends
    // all three: 5 bits of weight are plenty for 5 and 6-bit channels.
    const uint32_t spreadA = (a | (static_cast<uint32_t>(a) << 16)) & 0x07E0F81Fu;
    const uint32_t spreadB = (b | (static_cast<uint32_t>(b) << 16)) & 0x07E0F81Fu;
    const uint32_t t = static_cast<uint32_t>(t256) >> 3;  // 0..32
    const uint32_t mixed = ((spreadA * (32 - t) + spreadB * t) >> 5) & 0x07E0F81Fu;
    return static_cast<uint16_t>(mixed | (mixed >> 16));
}

int Width(const char* text, const SmoothFont::Face& face)
{
    return static_cast<int>(std::strlen(text)) * face.width;
}

DUNGEON_HOT void Draw(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int x, int y,
                      const char* text, const SmoothFont::Face& face, uint16_t colour, bool bold)
{
    if (y + face.height <= firstRow || y >= firstRow + rowCount)
    {
        return;
    }
    // The blend for each coverage level, once.
    const int glyphSize = face.width * face.height;
    const int top = y > firstRow ? y : firstRow;
    const int bottom = y + face.height < firstRow + rowCount ? y + face.height : firstRow + rowCount;
    for (int pass = 0; pass < (bold ? 2 : 1); ++pass)
    {
        for (int row = top; row < bottom; ++row)
        {
            uint16_t* out = dst + (row - firstRow) * stride;
            const int gy = row - y;
            int cx = x + pass;
            for (const char* p = text; *p != 0; ++p, cx += face.width)
            {
                const int code = static_cast<unsigned char>(*p);
                if (code <= SmoothFont::kFirst || code > SmoothFont::kLast || cx >= clipRight ||
                    cx + face.width <= clipLeft)
                {
                    continue;
                }
                const uint8_t* coverage = face.coverage + (code - SmoothFont::kFirst) * glyphSize + gy * face.width;
                for (int gx = 0; gx < face.width; ++gx)
                {
                    const int a = coverage[gx];
                    const int px = cx + gx;
                    if (a == 0 || px < clipLeft || px >= clipRight)
                    {
                        continue;
                    }
                    out[px] = a == 3 ? colour : Mix(out[px], colour, a * 85);
                }
            }
        }
    }
}

DUNGEON_HOT void Card(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int x0,
                      int y0, int x1, int y1, int radius, uint16_t tint, uint16_t border)
{
    const uint16_t tintHalf = static_cast<uint16_t>((tint >> 1) & 0x7BEF);
    for (int i = 0; i < rowCount; ++i)
    {
        const int row = firstRow + i;
        if (row < y0 || row >= y1)
        {
            continue;
        }
        // This row's span, and the spans above and below it: where they are
        // narrower, this row's pixels beyond them are the border.
        const int inset = Inset(row, y0, y1, radius);
        const int left = x0 + inset;
        const int right = x1 - inset;  // one past the last
        const bool edgeRow = row == y0 || row == y1 - 1;
        const int above = row - 1 >= y0 ? Inset(row - 1, y0, y1, radius) : radius;
        const int below = row + 1 < y1 ? Inset(row + 1, y0, y1, radius) : radius;
        const int outer = above > below ? above : below;
        const int innerLeft = x0 + (outer > inset ? outer : inset + 1);
        const int innerRight = x1 - (outer > inset ? outer : inset + 1);
        uint16_t* out = dst + i * stride;
        const int from = clipLeft > left ? clipLeft : left;
        const int to = clipRight < right ? clipRight : right;
        for (int x = from; x < to; ++x)
        {
            if (edgeRow || x < innerLeft || x >= innerRight)
            {
                out[x] = border;
                continue;
            }
            // An eighth of what is behind, plus the tint.
            out[x] = static_cast<uint16_t>(((out[x] >> 3) & 0x18E3) + tintHalf);  // channels stay under their tops
        }
    }
}

DUNGEON_HOT void Fill(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int x0,
                      int y0, int x1, int y1, int radius, uint16_t colour)
{
    for (int i = 0; i < rowCount; ++i)
    {
        const int row = firstRow + i;
        if (row < y0 || row >= y1)
        {
            continue;
        }
        const int inset = Inset(row, y0, y1, radius);
        const int from = clipLeft > x0 + inset ? clipLeft : x0 + inset;
        const int to = clipRight < x1 - inset ? clipRight : x1 - inset;
        uint16_t* out = dst + i * stride;
        for (int x = from; x < to; ++x)
        {
            out[x] = colour;
        }
    }
}

}  // namespace Dungeon::SmoothText
