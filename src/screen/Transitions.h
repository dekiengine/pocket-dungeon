#pragma once

// The black screens between places, as in the old dungeon games: after a
// fall, lines of white text appear one by one (how it ended, what was kept)
// before home; on reaching a new floor, its name; and an echo stone's voices,
// a line at a time. No engine: drawn into rows of RGB565 over the view.

#include "../world/CrawlerGame.h"

#include <cstdint>

namespace Dungeon::Transitions
{

// After a fall, in ms: the view goes black, then a line of text every
// kFallLineMs, a while to read them, and the last moments fade them out.
constexpr int kFallBlackMs = 700;
constexpr int kFallLineMs = 550;
constexpr int kFallReadMs = 1800;
constexpr int kFallOutMs = 500;
constexpr int kFallSkipMs = 900;  // keys go home from here on

/// Darkens `rowCount` rows toward black by `amount` of 255, in columns
/// [clipLeft, clipRight) of a view `width` wide: a fade, without a call for
/// every pixel.
void Darken(uint16_t* dst, int stride, int rowCount, int clipLeft, int clipRight, int width, int amount);

/// How long the fall's screen lasts: it has more lines with more to tell.
int FallLength(const CrawlerGame& game);

/// Draws the fall's screen `deadMs` after it, over view rows [firstRow,
/// firstRow + rowCount) of a view `width` by `height`, in columns [clipLeft,
/// clipRight). `dst` is the first of those rows, one every `stride` pixels.
void DrawFall(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int width, int height,
              const CrawlerGame& game, int deadMs);

// A new place's card, in ms: black with its name, then the view shows
// through over the last kPlaceOutMs.
constexpr int kPlaceMs = 1000;
constexpr int kPlaceOutMs = 300;

/// Draws the card for `name` (and `floor` under it, or nothing) with `msLeft`
/// of it to go.
void DrawPlace(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int width,
               int height, const char* name, const char* floor, int msLeft);

// An echo stone heard, in ms: the view goes dark, then a line every
// kEchoLineMs, each in its voice's colour. It stays until a key.
constexpr int kEchoDarkMs = 400;
constexpr int kEchoLineMs = 1100;
constexpr int kEchoKeyMs = 500;  // keys do nothing before

/// When all of an echo's lines are up (an ending has one more: THE END).
int EchoShownMs(bool ending);

/// Draws `echo` as heard `ms` ago.
void DrawEcho(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int width, int height,
              const Lore::Echo& echo, bool ending, int ms);

}  // namespace Dungeon::Transitions
