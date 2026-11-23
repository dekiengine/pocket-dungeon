#pragma once

// The message card at the bottom of the view: what just happened, for a few
// seconds. Someone speaking ("Puddle: Shiny!") gets their name on a tag and
// their words typed out a letter at a time. No engine: drawn into rows of
// RGB565 over the view.

#include <cstdint>

namespace Dungeon::MessageCard
{

constexpr int kTypeMs = 24;     // a letter of speech
constexpr int kShownMs = 3500;  // a message stays this long, speech once it is typed out

/// How long the speaker's name is when `text` is someone speaking, else 0:
/// the dungeon's voices, the bog trader, the village's people.
int SpeakerLength(const char* text);

/// How long `text` stays in all.
int ShownMs(const char* text);

/// How many letters are out `ageMs` after `text` was said: speech comes a
/// letter at a time, anything else all at once.
int Typed(const char* text, int ageMs);

/// Draws `text` with `typed` of its letters out, over view rows [firstRow,
/// firstRow + rowCount) of a view `width` by `height`, in columns [clipLeft,
/// clipRight). `dst` is the first of those rows, one every `stride` pixels.
/// `waits`: it stays until a key, and says so with a mark in its corner.
void Draw(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int width, int height,
          const char* text, int typed, bool waits = false);

/// How many letters `text` has to type out (after the speaker's name).
int Letters(const char* text);

}  // namespace Dungeon::MessageCard
