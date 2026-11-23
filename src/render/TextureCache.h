#pragma once

// Textures copied from flash into RAM. On a board the textures live in
// flash, read through a small cache; the walls, floor and ceiling of a zone
// are read for nearly every pixel of every frame and do not all fit that
// cache, so each read can wait on the flash. Held in RAM they cost a plain
// load. The room is handed in (the Crawler allocates it through the
// engine); what does not fit stays where it is. No engine.

#include "Raycaster.h"

#include <cstddef>
#include <cstdint>

namespace Dungeon
{

class TextureCache
{
public:
    static constexpr int kMaxHeld = 24;

    /// The RAM to copy into, `bytes` long; null: nothing is held.
    void SetStorage(uint8_t* storage, size_t bytes);

    /// Forgets everything held (a new zone needs other textures).
    void Clear();

    /// `texture` read from RAM: a copy of its pixels, and of its palettes
    /// unless `livePalette` (they change every frame and are in RAM
    /// already). The same texture again gives the same copy. `texture`
    /// itself when there is no room left, or it is null.
    const Texture* Hold(const Texture* texture, bool livePalette = false);

    /// Bytes taken so far.
    size_t GetUsed() const { return m_Used; }

private:
    struct Held
    {
        const Texture* source = nullptr;
        Texture copy;
    };
    Held m_Held[kMaxHeld];
    int m_Count = 0;
    uint8_t* m_Storage = nullptr;
    size_t m_Size = 0;
    size_t m_Used = 0;

    void* Take(size_t bytes);
};

}  // namespace Dungeon
