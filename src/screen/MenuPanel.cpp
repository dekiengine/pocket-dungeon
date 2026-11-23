#include "MenuPanel.h"

#include "Paper.h"

#include "../render/SmoothText.h"

#include <cstdio>
#include <cstring>

namespace Dungeon
{

namespace
{

using SmoothText::Rgb;

// A see-through dark card, one warm accent, clean text.
constexpr uint16_t kCardTint = Rgb(20, 24, 34);
constexpr uint16_t kCardBorder = Rgb(58, 66, 84);
constexpr uint16_t kDivider = Rgb(52, 58, 74);
constexpr uint16_t kHighlight = Rgb(44, 51, 68);
constexpr uint16_t kText = Rgb(232, 234, 240);
constexpr uint16_t kDim = Rgb(139, 147, 161);
constexpr uint16_t kAccent = Rgb(245, 182, 66);
constexpr uint16_t kMp = Rgb(140, 185, 255);
constexpr int kLineHeight = 22;

constexpr int kPad = 14;

}  // namespace

void MenuPanel::AddText(int x, int y, uint16_t colour, const char* text, bool bold)
{
    if (m_TextCount >= kMaxTexts)
    {
        return;
    }
    Text& t = m_Texts[m_TextCount++];
    t.x = x;
    t.y = y;
    t.colour = colour;
    t.bold = bold;
    std::snprintf(t.text, sizeof(t.text), "%s", text);
}

void MenuPanel::Build(const CrawlerGame& game, const CrawlerTextures& textures, int viewWidth, int viewHeight,
                      bool hidden)
{
    m_Paper = false;
    m_ViewWidth = viewWidth;
    m_ViewHeight = viewHeight;
    // An echo is heard on the dark (Transitions.h), not read from a panel.
    // Nor is someone speaking, or what a deed did: they have their own cards.
    const Menu open = game.GetMenu();
    const Menu menu = hidden || open == Menu::Echo || open == Menu::Talk || open == Menu::Result ? Menu::None : open;
    m_Shown = menu != Menu::None;
    m_TextCount = 0;
    m_IconCount = 0;
    m_HighlightY = -1;
    if (!m_Shown)
    {
        return;
    }
    const auto& small = SmoothFont::kSmall;
    char line[32];
    char have[40] = {};  // a help line made here, until it is added
    m_X = 14;
    m_Y = 12;
    m_W = viewWidth - 28;
    const int left = m_X + kPad;
    const int right = m_X + m_W - kPad;

    if (menu == Menu::Book)
    {
        // A book or a note: its title over a rule, and its lines.
        const Lore::Book* book = game.GetReading();
        m_Paper = true;
        m_PaperHand = false;
        m_PaperTitle = book ? book->title : nullptr;
        m_PaperText = book ? book->text : "";
        m_PaperFooter[0] = 0;
        return;
    }

    if (menu == Menu::Page)
    {
        // A diary page: a loose leaf in his hand, its number at the foot.
        m_Paper = true;
        m_PaperHand = true;
        m_PaperTitle = nullptr;
        m_PaperText = CrawlerGame::PageText(game.GetPageShown());
        std::snprintf(m_PaperFooter, sizeof(m_PaperFooter), "- %d -", game.GetPageShown());
        return;
    }

    if (menu == Menu::Controls)
    {
        // The keys, once: what each does.
        constexpr const char* kKeys[][2] = {
            { "Arrows", "Walk and turn" }, { "Q  C", "Step aside" },          { "E  Enter", "Use, talk, fight" },
            { "Space", "Wait a turn" },    { "M", "Menu: bag, magic, keys" }, { "Esc", "Close" },
        };
        constexpr int kRows = static_cast<int>(sizeof(kKeys) / sizeof(kKeys[0]));
        m_H = 40 + kRows * 20 + 34;
        AddText(left, m_Y + 10, kAccent, "CONTROLS", true);
        m_DividerY = m_Y + 32;
        for (int i = 0; i < kRows; ++i)
        {
            AddText(left, m_Y + 42 + i * 20, kAccent, kKeys[i][0], true);
            AddText(left + 92, m_Y + 42 + i * 20, kText, kKeys[i][1]);
        }
        AddText(left, m_Y + m_H - 24, kDim, "Any key to start");
        return;
    }

    if (menu == Menu::Thing)
    {
        // A thing standing about: its name, what can be done with it (or
        // leave it), and what the hero knows of the chosen one. Kept short
        // and at the top, so the thing shows under it.
        const int count = game.GetMenuCount();
        m_Y = 6;
        m_H = 38 + (count + 1) * kLineHeight + 28;
        char title[32];
        std::snprintf(title, sizeof(title), "%s", game.GetThingName());
        for (char* c = title; *c != 0; ++c)
        {
            *c = *c >= 'a' && *c <= 'z' ? static_cast<char>(*c - 'a' + 'A') : *c;
        }
        AddText(left, m_Y + 9, kAccent, title, true);
        m_DividerY = m_Y + 30;
        const int cursor = game.GetMenuCursor();
        for (int i = 0; i <= count; ++i)
        {
            const int y = m_Y + 38 + i * kLineHeight;
            const bool chosen = i == cursor;
            if (chosen)
            {
                m_HighlightY = y - 3;
            }
            if (i == count)
            {
                AddText(left + 12, y + 1, chosen ? kText : kDim, "Leave it", chosen);
                continue;
            }
            const MenuEntry& e = game.GetMenuEntry(i);
            AddText(left + 12, y + 1, e.count > 0 ? kText : kDim, e.label, chosen);
        }
        AddText(left, m_Y + m_H - 23, kDim,
                cursor < count ? game.DeedHint(game.GetMenuEntry(cursor)) : game.GetThingAbout());
        return;
    }

    if (menu == Menu::Descend)
    {
        // Standing on a way down: go, or not yet. Short and at the top, so
        // the hole shows under it.
        const bool village = game.IsInVillage();
        const int depth = game.GetDescendDepth();
        m_Y = 6;
        m_H = 38 + 2 * kLineHeight + 28;
        AddText(left, m_Y + 9, kAccent, village ? (depth > 1 ? "THE MINE LIFT" : "THE OLD WELL") : "THE WAY DOWN",
                true);
        m_DividerY = m_Y + 30;
        const int cursor = game.GetMenuCursor();
        for (int i = 0; i < 2; ++i)
        {
            const int y = m_Y + 38 + i * kLineHeight;
            const bool chosen = i == cursor;
            if (chosen)
            {
                m_HighlightY = y - 3;
            }
            AddText(left + 12, y + 1, chosen || i == 0 ? kText : kDim, i == 0 ? game.GetMenuEntry(0).label : "Not yet",
                    chosen);
        }
        char hint[40];
        std::snprintf(hint, sizeof(hint), village ? "The dungeon. B%d." : "B%d. Deeper, darker, better loot.", depth);
        AddText(left, m_Y + m_H - 23, kDim, cursor == 0 ? hint : "It will still be here.");
        return;
    }

    const int count = game.GetMenuCount();
    // The ending has no Close line: it must be chosen.
    const bool ending = menu == Menu::Ending;
    const int rows = ending ? count : count + 1;
    // A long bag shows a window of its lines, moving with the chosen one.
    constexpr int kVisible = 7;
    const int shown = rows < kVisible ? rows : kVisible;
    int first = game.GetMenuCursor() - kVisible / 2;
    first = first > rows - shown ? rows - shown : first;
    first = first < 0 ? 0 : first;
    m_H = 40 + shown * kLineHeight + 34;
    // The title, and the gold (or, for magic, the MP).
    const bool magic = menu == Menu::Magic;
    AddText(left, m_Y + 10, ending ? kAccent : kText,
            menu == Menu::Bag         ? "BAG"
            : magic                   ? "MAGIC"
            : ending                  ? "THE KEEPER FALLS"
            : menu == Menu::Main      ? "MENU"
            : menu == Menu::Keys      ? "CONTROLS"
            : menu == Menu::Fence     ? "ULF"
            : menu == Menu::Contracts ? "CONTRACTS"
            : menu == Menu::Notes     ? "NOTES"
                                      : "PUDDLE'S SHOP",
            true);
    if (magic)
    {
        std::snprintf(line, sizeof(line), "%d / %d MP", game.GetMp(), game.GetMaxMp());
    }
    else if (ending || menu == Menu::Bag || menu == Menu::Main || menu == Menu::Keys || menu == Menu::Contracts ||
             menu == Menu::Notes)
    {
        line[0] = 0;
    }
    else
    {
        // The gold that pays: the run's below, the village's at home.
        std::snprintf(line, sizeof(line), "%d G", menu == Menu::Fence ? game.GetLegend().bank : game.GetGold());
    }
    AddText(right - SmoothText::Width(line, small), m_Y + 10, magic ? kMp : kAccent, line, true);
    m_DividerY = m_Y + 32;

    // A line per thing, then Close; the chosen one highlighted.
    const int firstLine = m_Y + 40;
    for (int i = first; i < first + shown; ++i)
    {
        const int y = firstLine + (i - first) * kLineHeight;
        const bool chosen = i == game.GetMenuCursor();
        if (chosen)
        {
            m_HighlightY = y - 3;
        }
        if (i == count)
        {
            AddText(left + 26, y + 1, chosen ? kText : kDim,
                    menu == Menu::Shop || menu == Menu::Fence ? "Leave" : "Close", chosen);
            continue;
        }
        const MenuEntry& e = game.GetMenuEntry(i);
        if (e.label)
        {
            // A line that is not a thing: feeding Puddle, an ending.
            if (m_IconCount < kMaxIcons)
            {
                Icon& icon = m_Icons[m_IconCount++];
                icon.texture = e.spell >= 0 ? textures.spells[e.spell] : textures.items[static_cast<int>(e.item)];
                icon.x = left + 2;
                icon.y = y;
            }
            AddText(left + 26, y + 1, menu == Menu::Main && e.count == 0 ? kDim : kText, e.label, chosen);
            if (menu == Menu::Shop && e.item != ItemKind::Note)
            {
                if (e.item == ItemKind::Snack)
                {
                    std::snprintf(line, sizeof(line), "%d / 3 fed", e.count);
                }
                else
                {
                    std::snprintf(line, sizeof(line), "+%d G", e.price);
                }
                AddText(right - SmoothText::Width(line, small), y + 1, kAccent, line, true);
            }
            else if (menu == Menu::Main && e.price == 4)
            {
                // How loud the sounds are now.
                if (game.GetSoundVolume() > 0)
                {
                    std::snprintf(line, sizeof(line), "%d%%", game.GetSoundVolume());
                }
                else
                {
                    std::snprintf(line, sizeof(line), "off");
                }
                AddText(right - SmoothText::Width(line, small), y + 1, kAccent, line, true);
            }
            else if (menu == Menu::Contracts)
            {
                // Where it stands: to be asked about, open, done, paid.
                constexpr const char* kStates[4] = { "?", "open", "return", "kept" };
                const char* state = kStates[e.count & 3];
                AddText(right - SmoothText::Width(state, small), y + 1, e.count == 2 ? kAccent : kDim, state,
                        e.count == 2);
            }
            else if (menu == Menu::Keys && m_KeyNames && i < m_KeyCount)
            {
                // The key it is on now, or that it waits for a new one.
                const char* name = i == m_KeyWanted ? "press a key" : m_KeyNames[i];
                AddText(right - SmoothText::Width(name, small), y + 1, kAccent, name, true);
            }
            continue;
        }
        if (magic)
        {
            const Spell spell = static_cast<Spell>(e.spell);
            if (m_IconCount < kMaxIcons)
            {
                Icon& icon = m_Icons[m_IconCount++];
                icon.texture = textures.spells[e.spell];
                icon.x = left + 2;
                icon.y = y;
            }
            const bool affordable = game.GetMp() >= e.price;
            AddText(left + 26, y + 1, affordable ? kText : kDim, CrawlerGame::SpellName(spell), chosen);
            std::snprintf(line, sizeof(line), "%d MP", e.price);
            AddText(right - SmoothText::Width(line, small), y + 1, affordable ? kMp : kDim, line, affordable);
            continue;
        }
        if (m_IconCount < kMaxIcons)
        {
            Icon& icon = m_Icons[m_IconCount++];
            icon.texture = textures.items[static_cast<int>(e.item)];
            icon.x = left + 2;
            icon.y = y;
        }
        const bool available = menu == Menu::Bag || e.count > 0;
        AddText(left + 26, y + 1, available ? kText : kDim, CrawlerGame::ItemName(e.item), chosen);
        if (menu == Menu::Bag)
        {
            std::snprintf(line, sizeof(line), "x%d", e.count);
            AddText(right - SmoothText::Width(line, small), y + 1, kDim, line);
        }
        else
        {
            if (e.count > 0 || menu == Menu::Fence)
            {
                std::snprintf(line, sizeof(line), "%d G", e.price);
            }
            else
            {
                std::snprintf(line, sizeof(line), "sold");
            }
            AddText(right - SmoothText::Width(line, small), y + 1, e.count > 0 ? kAccent : kDim, line, e.count > 0);
        }
    }

    // What the chosen thing does.
    const int cursor = game.GetMenuCursor();
    const char* help = cursor < count
                           ? (magic ? CrawlerGame::SpellHelp(static_cast<Spell>(game.GetMenuEntry(cursor).spell))
                                    : CrawlerGame::ItemHelp(game.GetMenuEntry(cursor).item))
                           : (menu == Menu::Bag || magic ? "" : "Go well. Come back with gold.");
    if (ending)
    {
        help = cursor == 0 ? "Halvard walks free. The dead rest." : "The hat, the staff, the gold. Yours.";
    }
    else if (menu == Menu::Main)
    {
        constexpr const char* kHelp[7] = { "What you carry.",           "What you can cast.",
                                           "Home, with all you carry.", "Choose your keys.",
                                           "How loud. Enter steps it.", "What the town asked of you.",
                                           "What you have read." };
        const bool barred = cursor == 2 && cursor < count && game.GetMenuEntry(cursor).count == 0;
        help = cursor >= count || cursor >= 7 ? "" : barred ? "Not while something hunts you." : kHelp[cursor];
    }
    else if (menu == Menu::Keys)
    {
        help = m_KeyWanted >= 0 ? "Press the new key. Esc keeps it." : cursor < count ? "Enter changes it." : "";
    }
    else if (menu == Menu::Notes)
    {
        constexpr const char* kKinds[3] = { "Left by someone before you.", "A book you read on this descent.",
                                            "Halvard's diary." };
        help = count == 0 ? "Nothing read yet." : (cursor < count ? kKinds[game.GetMenuEntry(cursor).price % 3] : "");
    }
    else if (menu == Menu::Contracts)
    {
        help = count == 0 ? "Nobody has asked anything of you yet."
                          : (cursor < count ? game.GetContractHint(game.GetMenuEntry(cursor)) : "");
    }
    else if (menu == Menu::Fence)
    {
        // What the village has of it already.
        help = cursor >= count ? "He was never here." : "Hear what he knows.";
        if (cursor < count && !game.GetMenuEntry(cursor).label)
        {
            const int material = static_cast<int>(game.GetMenuEntry(cursor).item) - static_cast<int>(ItemKind::Moss);
            std::snprintf(have, sizeof(have), "The village has %d %s.", game.GetLegend().materials[material],
                          Village::MaterialName(material));
            help = have;
        }
    }
    else if (cursor < count && game.GetMenuEntry(cursor).label)
    {
        const ItemKind item = game.GetMenuEntry(cursor).item;
        help = item == ItemKind::Snack  ? "Three snacks and he shares a secret."
               : item == ItemKind::Note ? "Hear what he has heard."
                                        : "Gems and goblets, for gold.";
    }
    if (menu == Menu::Bag)
    {
        // This run's materials, for the village, by the title.
        for (int m = Village::kMaterials - 1, x = right; m >= 0; --m)
        {
            std::snprintf(line, sizeof(line), "%d", game.GetMaterial(m));
            x -= SmoothText::Width(line, small);
            AddText(x, m_Y + 10, kText, line, true);
            x -= 19;
            if (m_IconCount < kMaxIcons)
            {
                Icon& icon = m_Icons[m_IconCount++];
                icon.texture = textures.items[static_cast<int>(ItemKind::Moss) + m];
                icon.x = x;
                icon.y = m_Y + 9;
            }
            x -= 8;
        }
    }
    if (count == 0 && menu == Menu::Bag)
    {
        help = "Nothing yet.";
    }
    AddText(left, m_Y + m_H - 24, kDim, help);
    // More lines above or below the window.
    if (rows > shown)
    {
        const char* more = first > 0 && first + shown < rows ? "^ v" : first > 0 ? "^" : "v";
        AddText(right - SmoothText::Width(more, small), m_Y + m_H - 24, kAccent, more, true);
    }
}

DUNGEON_HOT void MenuPanel::DrawRows(uint16_t* dst, int strideBytes, int firstRow, int rowCount, int clipLeft,
                                     int clipRight) const
{
    if (!m_Shown)
    {
        return;
    }
    const int stride = strideBytes / 2;
    if (m_Paper)
    {
        Paper::Draw(dst, stride, firstRow, rowCount, clipLeft, clipRight, m_ViewWidth, m_ViewHeight, m_PaperTitle,
                    m_PaperText, m_PaperFooter[0] != 0 ? m_PaperFooter : nullptr, m_PaperHand);
        return;
    }
    const int x1 = m_X + m_W;
    SmoothText::Card(dst, stride, firstRow, rowCount, clipLeft, clipRight, m_X, m_Y, x1, m_Y + m_H, 6, kCardTint,
                     kCardBorder);
    SmoothText::Fill(dst, stride, firstRow, rowCount, clipLeft, clipRight, m_X + kPad, m_DividerY, x1 - kPad,
                     m_DividerY + 1, 0, kDivider);
    if (m_HighlightY >= 0)
    {
        SmoothText::Fill(dst, stride, firstRow, rowCount, clipLeft, clipRight, m_X + 8, m_HighlightY, x1 - 8,
                         m_HighlightY + kLineHeight, 4, kHighlight);
        SmoothText::Fill(dst, stride, firstRow, rowCount, clipLeft, clipRight, m_X + 8, m_HighlightY + 3, m_X + 11,
                         m_HighlightY + kLineHeight - 3, 1, kAccent);
    }
    // The icons.
    for (int k = 0; k < m_IconCount; ++k)
    {
        const Icon& icon = m_Icons[k];
        const Texture* t = icon.texture;
        if (!t || !t->index)
        {
            continue;
        }
        for (int i = 0; i < rowCount; ++i)
        {
            const int v = firstRow + i - icon.y;
            if (v < 0 || v >= t->size)
            {
                continue;
            }
            uint16_t* out = dst + i * stride;
            const uint8_t* texRow = t->index + v * t->size;
            for (int u = 0; u < t->size; ++u)
            {
                const int x = icon.x + u;
                const uint16_t pixel = t->palette[texRow[u]];
                if (x >= clipLeft && x < clipRight && pixel != kTransparent)
                {
                    out[x] = pixel;
                }
            }
        }
    }
    for (int k = 0; k < m_TextCount; ++k)
    {
        const Text& t = m_Texts[k];
        SmoothText::Draw(dst, stride, firstRow, rowCount, clipLeft, clipRight, t.x, t.y, t.text, SmoothFont::kSmall,
                         t.colour, t.bold);
    }
}

}  // namespace Dungeon
