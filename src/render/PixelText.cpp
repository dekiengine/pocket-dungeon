#include "PixelText.h"

#include "Font.gen.h"

#include <cstring>

namespace Dungeon::PixelText
{

namespace
{

// One pass of the letters in one colour (or the gradient), shifted by
// (dx, dy) font pixels.
void Pass(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int x, int y,
          const char* text, int scale, uint16_t colour, const uint16_t* gradient, int dx, int dy)
{
    x += dx * scale;
    y += dy * scale;
    const int height = Font::kHeight * scale;
    const int top = y > firstRow ? y : firstRow;
    const int bottom = y + height < firstRow + rowCount ? y + height : firstRow + rowCount;
    for (int row = top; row < bottom; ++row)
    {
        const int gy = (row - y) / scale;
        const uint16_t c = gradient ? gradient[gy] : colour;
        uint16_t* out = dst + (row - firstRow) * stride;
        int cx = x;
        for (const char* p = text; *p != 0; ++p, cx += Font::kWidth * scale)
        {
            const int code = static_cast<unsigned char>(*p);
            if (code < Font::kFirst || code > Font::kLast || cx >= clipRight || cx + Font::kWidth * scale <= clipLeft)
            {
                continue;
            }
            const uint8_t bits = Font::kGlyphs[code - Font::kFirst][gy];
            for (int bx = 0; bits != 0 && bx < Font::kWidth; ++bx)
            {
                if ((bits & (1 << (Font::kWidth - 1 - bx))) == 0)
                {
                    continue;
                }
                const int px = cx + bx * scale;
                for (int s = 0; s < scale; ++s)
                {
                    if (px + s >= clipLeft && px + s < clipRight)
                    {
                        out[px + s] = c;
                    }
                }
            }
        }
    }
}

}  // namespace

int Width(const char* text, int scale)
{
    return static_cast<int>(std::strlen(text)) * Font::kWidth * scale;
}

int Height(int scale)
{
    return Font::kHeight * scale;
}

void Draw(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int x, int y,
          const char* text, const Style& style)
{
    const int s = style.scale > 0 ? style.scale : 1;
    // Rows the text cannot touch (with its outline and shadow) cost nothing.
    if (y + (Font::kHeight + 2) * s <= firstRow || y - s >= firstRow + rowCount)
    {
        return;
    }
    if (style.shadowed)
    {
        Pass(dst, stride, firstRow, rowCount, clipLeft, clipRight, x, y, text, s, style.shadow, nullptr, 2, 2);
    }
    if (style.outlined)
    {
        constexpr int kAround[8][2] = { { -1, -1 }, { 0, -1 }, { 1, -1 }, { -1, 0 },
                                        { 1, 0 },   { -1, 1 }, { 0, 1 },  { 1, 1 } };
        for (const auto& d : kAround)
        {
            Pass(dst, stride, firstRow, rowCount, clipLeft, clipRight, x, y, text, s, style.outline, nullptr, d[0],
                 d[1]);
        }
    }
    Pass(dst, stride, firstRow, rowCount, clipLeft, clipRight, x, y, text, s, style.colour, style.gradient, 0, 0);
}

}  // namespace Dungeon::PixelText
