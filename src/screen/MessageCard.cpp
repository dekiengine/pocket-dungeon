#include "MessageCard.h"

#include "../Hot.h"
#include "../render/SmoothText.h"
#include "../world/Village.h"

#include <cstdio>
#include <cstring>

namespace Dungeon::MessageCard
{

namespace
{

using SmoothText::Rgb;

constexpr uint16_t kCardTint = Rgb(20, 24, 34);
constexpr uint16_t kCardBorder = Rgb(58, 66, 84);
constexpr uint16_t kText = Rgb(232, 234, 240);
constexpr uint16_t kAccent = Rgb(245, 182, 66);
constexpr int kPerLine = 38;
constexpr int kMostLines = 3;

}  // namespace

int SpeakerLength(const char* text)
{
    const char* colon = std::strstr(text, ": ");
    if (!colon || colon == text || colon - text > 14)
    {
        return 0;
    }
    const int length = static_cast<int>(colon - text);
    const auto is = [&](const char* name)
    {
        return static_cast<int>(std::strlen(name)) == length &&
               std::strncmp(text, name, static_cast<size_t>(length)) == 0;
    };
    if (is("Puddle") || is("Halvard") || is("Keeper") || is("Keeper (you)") || is("Hilde") || is("Leif") || is("Ulf"))
    {
        return length;
    }
    for (int b = 0; b < Village::kBuildings; ++b)
    {
        if (is(Village::KeeperName(b)))
        {
            return length;
        }
    }
    return 0;
}

int ShownMs(const char* text)
{
    const int name = SpeakerLength(text);
    return kShownMs + (name > 0 ? (static_cast<int>(std::strlen(text)) - name) * kTypeMs : 0);
}

int Typed(const char* text, int ageMs)
{
    // No more than there is to type: whatever counts the letters as they
    // come (their sound) stops when the last is out.
    const int letters = Letters(text);
    const int typed = ageMs / kTypeMs;
    return SpeakerLength(text) > 0 ? (typed < letters ? typed : letters) : 1000;
}

int Letters(const char* text)
{
    const int name = SpeakerLength(text);
    return static_cast<int>(std::strlen(text)) - (name > 0 ? name + 2 : 0);
}

DUNGEON_HOT void Draw(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int width,
                      int height, const char* text, int typed, bool waits)
{
    // Word-wrapped to the card. Someone speaking: the name on a tag, the
    // words after it.
    char lines[kMostLines][kPerLine + 1] = {};
    int count = 0;
    const int nameLength = SpeakerLength(text);
    const bool speech = nameLength > 0;
    const char* p = text + (speech ? nameLength + 2 : 0);
    while (*p != 0 && count < kMostLines)
    {
        while (*p == ' ')
        {
            ++p;
        }
        const int length = static_cast<int>(std::strlen(p));
        int take = length;
        if (length > kPerLine)
        {
            take = kPerLine;
            while (take > 0 && p[take] != ' ')
            {
                --take;
            }
            take = take > 0 ? take : kPerLine;
        }
        std::memcpy(lines[count], p, static_cast<size_t>(take));
        lines[count][take] = 0;
        ++count;
        p += take;
    }
    const int lineHeight = SmoothFont::kSmall.height;
    const int y1 = height - 8;
    const int y0 = y1 - (count * lineHeight + (speech ? 20 : 12));
    const int x0 = 10;
    const int x1 = width - 10;
    SmoothText::Card(dst, stride, firstRow, rowCount, clipLeft, clipRight, x0, y0, x1, y1, 4, kCardTint, kCardBorder);
    int textY = y0 + 6;
    if (speech)
    {
        // The name on a tag across the box's top edge.
        char name[16];
        std::snprintf(name, sizeof(name), "%.*s", nameLength, text);
        const int tagWidth = SmoothText::Width(name, SmoothFont::kSmall) + 14;
        SmoothText::Fill(dst, stride, firstRow, rowCount, clipLeft, clipRight, x0 + 10, y0 - 9, x0 + 10 + tagWidth,
                         y0 + 9, 4, kAccent);
        SmoothText::Draw(dst, stride, firstRow, rowCount, clipLeft, clipRight, x0 + 17, y0 - 8, name,
                         SmoothFont::kSmall, kCardTint, true);
        textY = y0 + 14;
    }
    else
    {
        // An accent stripe down its left.
        SmoothText::Fill(dst, stride, firstRow, rowCount, clipLeft, clipRight, x0 + 4, y0 + 5, x0 + 7, y1 - 5, 1,
                         kAccent);
    }
    if (waits)
    {
        // Waiting for a key: a mark in the corner.
        SmoothText::Fill(dst, stride, firstRow, rowCount, clipLeft, clipRight, x1 - 16, y1 - 11, x1 - 8, y1 - 9, 0,
                         kAccent);
        SmoothText::Fill(dst, stride, firstRow, rowCount, clipLeft, clipRight, x1 - 14, y1 - 9, x1 - 10, y1 - 7, 0,
                         kAccent);
    }
    for (int i = 0; i < count && typed > 0; ++i)
    {
        const int length = static_cast<int>(std::strlen(lines[i]));
        if (typed < length)
        {
            lines[i][typed] = 0;
        }
        typed -= length;
        SmoothText::Draw(dst, stride, firstRow, rowCount, clipLeft, clipRight, x0 + 14, textY + i * lineHeight,
                         lines[i], SmoothFont::kSmall, kText);
    }
}

}  // namespace Dungeon::MessageCard
