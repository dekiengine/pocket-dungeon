#pragma once

// Something to read, as the thing it is: a sheet of old paper over the
// darkened view, ragged at the edges and browner toward them, the words in
// ink. A diary page (a loose leaf with its number at the foot), a book or a
// note (its title over a rule). No engine: drawn into rows of RGB565.

#include <cstdint>

namespace Dungeon::Paper
{

/// Draws a sheet with up to four lines of `text` (split at '\n') over view
/// rows [firstRow, firstRow + rowCount) of a view `width` by `height`, in
/// columns [clipLeft, clipRight). `dst` is the first of those rows, one every
/// `stride` pixels. `title` (or null) goes over a rule at the top, `footer`
/// (or null) faded at the foot. `hand`: written by hand, a heavier line.
void Draw(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int width, int height,
          const char* title, const char* text, const char* footer, bool hand);

}  // namespace Dungeon::Paper
