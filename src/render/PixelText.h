#pragma once

// Text in the monogram pixel font (Font.gen.h), drawn straight into rows of
// RGB565 at any whole scale, with an optional outline, drop shadow and a
// colour that changes down the letters (a gradient). No engine: the HUD, the
// title and the message box draw with it, part of a frame at a time.

#include <cstdint>

namespace Dungeon::PixelText
{

struct Style
{
    int scale = 1;
    uint16_t colour = 0xFFFF;
    /// Down the letters' 10 font rows, top first: a colour each (or null for
    /// `colour` all the way).
    const uint16_t* gradient = nullptr;
    bool outlined = false;
    uint16_t outline = 0x0000;  // one font pixel all round
    bool shadowed = false;
    uint16_t shadow = 0x0000;  // one font pixel down and right
};

/// The width of `text` in pixels at `scale`.
int Width(const char* text, int scale);
/// The height of a line at `scale`.
int Height(int scale);

/// Draws `text` with its top left at (x, y), into the rows [firstRow,
/// firstRow + rowCount) of a target whose row `firstRow` starts at `dst`, one
/// row every `stride` pixels, only in columns [clipLeft, clipRight).
void Draw(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int x, int y,
          const char* text, const Style& style);

}  // namespace Dungeon::PixelText
