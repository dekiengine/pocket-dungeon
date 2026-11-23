#include "Paper.h"

#include "../Hot.h"
#include "../render/SmoothText.h"

#include <cstring>

namespace Dungeon::Paper
{

namespace
{

using SmoothText::Rgb;

// Old paper: mostly the first, flecked with the others; browner at the edges.
constexpr uint16_t kPaper[4] = { Rgb(224, 203, 160), Rgb(224, 203, 160), Rgb(214, 191, 146), Rgb(232, 213, 172) };
constexpr uint16_t kEdge = Rgb(150, 116, 74);
constexpr uint16_t kFold = Rgb(196, 172, 128);
constexpr uint16_t kInk = Rgb(56, 38, 28);
constexpr uint16_t kFaded = Rgb(142, 112, 78);
constexpr int kMostLines = 4;
constexpr int kLineGap = 22;
constexpr int kMargin = 22;  // paper round the words, left and right
constexpr int kBurn = 9;     // how far in from an edge the paper is browner

// A number that looks random, the same for the same `n`.
uint32_t Scramble(uint32_t n)
{
    n ^= n >> 15;
    n *= 0x2C1B3C6Du;
    n ^= n >> 12;
    n *= 0x297A2D39u;
    return n ^ (n >> 15);
}

// How far the paper's edge is eaten in at `along` it: torn in short runs.
int Rag(int along, int most, uint32_t salt)
{
    return static_cast<int>(Scramble(static_cast<uint32_t>(along / 3) + salt) % static_cast<uint32_t>(most + 1));
}

}  // namespace

DUNGEON_HOT void Draw(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int width,
                      int height, const char* title, const char* text, const char* footer, bool hand)
{
    const auto& face = SmoothFont::kSmall;
    // The lines, and how wide the widest thing on the sheet is.
    char lines[kMostLines][40] = {};
    int count = 0;
    int widest = title ? SmoothText::Width(title, face) + 8 : 0;
    for (const char* p = text ? text : ""; *p != 0 && count < kMostLines; ++count)
    {
        int n = 0;
        while (p[n] != 0 && p[n] != '\n' && n < 38)
        {
            lines[count][n] = p[n];
            ++n;
        }
        lines[count][n] = 0;
        p += n;
        p += *p == '\n' ? 1 : 0;
        const int w = SmoothText::Width(lines[count], face);
        widest = w > widest ? w : widest;
    }
    // The sheet, in the middle of the view.
    const int top = title ? 44 : 22;
    const int foot = footer ? 38 : 22;
    const int sheetWidth = widest + 2 * kMargin < 150 ? 150 : widest + 2 * kMargin;
    const int sheetHeight = top + count * kLineGap + foot;
    const int x0 = (width - sheetWidth) / 2;
    const int x1 = x0 + sheetWidth;
    const int y0 = (height - sheetHeight) / 2;
    const int y1 = y0 + sheetHeight;
    const int fold = y0 + sheetHeight / 2;
    const int from = clipLeft > 0 ? clipLeft : 0;
    const int to = clipRight < width ? clipRight : width;
    for (int i = 0; i < rowCount; ++i)
    {
        const int y = firstRow + i;
        uint16_t* out = dst + i * stride;
        // The view goes dark behind the page: a quarter as bright, by a
        // shift (a blend for every pixel would cost frames).
        for (int x = from; x < to; ++x)
        {
            out[x] = static_cast<uint16_t>((out[x] >> 2) & 0x39E7);
        }
        if (y < y0 || y >= y1)
        {
            continue;
        }
        const int left = x0 + Rag(y, 2, 11);
        const int right = x1 - Rag(y, 2, 53);
        for (int x = left > from ? left : from; x < right && x < to; ++x)
        {
            // Torn along the top a little, along the foot more.
            if (y < y0 + Rag(x, 2, 97) || y >= y1 - Rag(x, 4, 131))
            {
                continue;
            }
            uint16_t colour = kPaper[Scramble(static_cast<uint32_t>((x >> 1) * 977 + (y >> 1) * 131)) & 3];
            if (y == fold)
            {
                colour = kFold;  // folded once, long ago
            }
            // Browner toward the nearest edge.
            int near = x - x0 < x1 - 1 - x ? x - x0 : x1 - 1 - x;
            near = y - y0 < near ? y - y0 : near;
            near = y1 - 1 - y < near ? y1 - 1 - y : near;
            if (near < kBurn)
            {
                colour = SmoothText::Mix(colour, kEdge, (kBurn - near) * 150 / kBurn);
            }
            out[x] = colour;
        }
    }
    // The words, in ink.
    int y = y0 + 16;
    if (title)
    {
        SmoothText::Draw(dst, stride, firstRow, rowCount, clipLeft, clipRight,
                         (width - SmoothText::Width(title, face)) / 2, y, title, face, kInk, true);
        SmoothText::Fill(dst, stride, firstRow, rowCount, clipLeft, clipRight, x0 + kMargin - 6, y + 19,
                         x1 - kMargin + 6, y + 20, 0, kFaded);
        y += 28;
    }
    else
    {
        y += 6;
    }
    // A block of lines, its left edge where the widest line's would be.
    int block = 0;
    for (int i = 0; i < count; ++i)
    {
        const int w = SmoothText::Width(lines[i], face);
        block = w > block ? w : block;
    }
    for (int i = 0; i < count; ++i)
    {
        SmoothText::Draw(dst, stride, firstRow, rowCount, clipLeft, clipRight, (width - block) / 2, y + i * kLineGap,
                         lines[i], face, kInk, hand);
    }
    if (footer)
    {
        SmoothText::Draw(dst, stride, firstRow, rowCount, clipLeft, clipRight,
                         (width - SmoothText::Width(footer, face)) / 2, y1 - 30, footer, face, kFaded, false);
    }
}

}  // namespace Dungeon::Paper
