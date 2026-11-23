#pragma once

// The modern UI's drawing: smooth text (JetBrains Mono, SmoothFont.gen.h)
// blended onto what is behind it, and see-through cards with rounded
// corners. Into rows of RGB565, part of a frame at a time, no engine.

#include "SmoothFont.gen.h"

#include <cstdint>

namespace Dungeon::SmoothText
{

/// RGB565 from 8-bit channels.
constexpr uint16_t Rgb(int r, int g, int b)
{
    return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

/// `t256` of the way from a to b.
uint16_t Mix(uint16_t a, uint16_t b, int t256);

int Width(const char* text, const SmoothFont::Face& face);

/// Draws `text` with its top left at (x, y) into the rows [firstRow, firstRow
/// + rowCount) of a target whose row `firstRow` starts at `dst`, `stride`
/// pixels a row, only in columns [clipLeft, clipRight). `bold` draws it twice,
/// a pixel apart.
void Draw(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int x, int y,
          const char* text, const SmoothFont::Face& face, uint16_t colour, bool bold = false);

/// A card over the view: what is behind it darkened to an eighth and tinted
/// toward `tint`, corners rounded by `radius`, a one-pixel `border`.
void Card(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int x0, int y0, int x1,
          int y1, int radius, uint16_t tint, uint16_t border);

/// A solid rectangle with rounded corners (radius 0: square).
void Fill(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int x0, int y0, int x1,
          int y1, int radius, uint16_t colour);

}  // namespace Dungeon::SmoothText
