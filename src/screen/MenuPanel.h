#pragma once

// The window over the 3D view for the bag, the trader, the magic, a thing's
// choices, a word for a frog and the ending: a see-through card with a line
// per thing (its icon, name and count or price), the chosen line
// highlighted, and a line of help, in smooth text. A diary page, a book or a
// note is a sheet of paper instead (Paper.h). No engine: built from the game
// each frame and drawn into rows of RGB565.

#include "../world/CrawlerGame.h"

#include <cstdint>

namespace Dungeon
{

class MenuPanel
{
public:
    /// Lays the window out for the game's open menu (none: hidden, as it is
    /// when `hidden`), in a view `viewWidth` by `viewHeight` pixels.
    void Build(const CrawlerGame& game, const CrawlerTextures& textures, int viewWidth, int viewHeight, bool hidden);
    bool IsShown() const { return m_Shown; }
    /// The keys' names for the keys menu, a line each, and which line is
    /// waiting for its new key (-1: none). Before Build.
    void SetKeyNames(const char (*names)[8], int count, int wanted)
    {
        m_KeyNames = names;
        m_KeyCount = count;
        m_KeyWanted = wanted;
    }

    /// Draws the window's part of view rows [firstRow, firstRow + rowCount)
    /// into `dst` (the first of those rows, one every `strideBytes`), only
    /// in columns [clipLeft, clipRight).
    void DrawRows(uint16_t* dst, int strideBytes, int firstRow, int rowCount, int clipLeft, int clipRight) const;

private:
    struct Text
    {
        int x = 0;
        int y = 0;
        uint16_t colour = 0xFFFF;
        bool bold = false;
        char text[40] = {};
    };
    struct Icon
    {
        int x = 0;
        int y = 0;
        const Texture* texture = nullptr;
    };
    static constexpr int kMaxTexts = 24;
    static constexpr int kMaxIcons = 14;

    bool m_Shown = false;
    int m_X = 0;
    int m_Y = 0;
    int m_W = 0;
    int m_H = 0;
    Text m_Texts[kMaxTexts];
    int m_TextCount = 0;
    Icon m_Icons[kMaxIcons];
    int m_IconCount = 0;
    int m_DividerY = 0;
    int m_HighlightY = -1;  // the chosen line's top, -1 none
    // Something to read: a sheet of paper, with its title (a book's), its
    // lines and its footer (a diary page's number).
    bool m_Paper = false;
    bool m_PaperHand = false;
    const char* m_PaperTitle = nullptr;
    const char* m_PaperText = nullptr;
    char m_PaperFooter[12] = {};
    const char (*m_KeyNames)[8] = nullptr;
    int m_KeyCount = 0;
    int m_KeyWanted = -1;
    int m_ViewWidth = 320;
    int m_ViewHeight = 240;

    void AddText(int x, int y, uint16_t colour, const char* text, bool bold = false);
};

}  // namespace Dungeon
