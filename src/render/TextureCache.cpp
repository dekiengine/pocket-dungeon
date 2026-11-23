#include "TextureCache.h"

#include <cstring>

namespace Dungeon
{

void TextureCache::SetStorage(uint8_t* storage, size_t bytes)
{
    m_Storage = storage;
    m_Size = storage ? bytes : 0;
    Clear();
}

void TextureCache::Clear()
{
    m_Count = 0;
    m_Used = 0;
}

void* TextureCache::Take(size_t bytes)
{
    // Kept 4-byte aligned, for the palettes' 16-bit reads.
    const size_t at = (m_Used + 3) & ~static_cast<size_t>(3);
    if (!m_Storage || at + bytes > m_Size)
    {
        return nullptr;
    }
    m_Used = at + bytes;
    return m_Storage + at;
}

const Texture* TextureCache::Hold(const Texture* texture, bool livePalette)
{
    if (!texture || !texture->index)
    {
        return texture;
    }
    for (int i = 0; i < m_Count; ++i)
    {
        if (m_Held[i].source == texture)
        {
            return &m_Held[i].copy;
        }
    }
    if (m_Count >= kMaxHeld)
    {
        return texture;
    }
    const size_t pixels = static_cast<size_t>(texture->size) * texture->size;
    const size_t colours = static_cast<size_t>(texture->levels) * texture->colours;
    const size_t used = m_Used;
    uint8_t* index = static_cast<uint8_t*>(Take(pixels));
    uint16_t* palette = livePalette ? nullptr : static_cast<uint16_t*>(Take(colours * sizeof(uint16_t)));
    if (!index || (!livePalette && !palette))
    {
        m_Used = used;  // no room for all of it: none of it
        return texture;
    }
    std::memcpy(index, texture->index, pixels);
    Held& held = m_Held[m_Count++];
    held.source = texture;
    held.copy = *texture;
    held.copy.index = index;
    if (palette)
    {
        std::memcpy(palette, texture->palette, colours * sizeof(uint16_t));
        held.copy.palette = palette;
    }
    return &held.copy;
}

}  // namespace Dungeon
