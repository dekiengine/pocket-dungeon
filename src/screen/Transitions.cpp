#include "Transitions.h"

#include "../render/SmoothText.h"

#include <cstdio>

namespace Dungeon::Transitions
{

namespace
{

using SmoothText::Rgb;

constexpr uint16_t kText = Rgb(240, 240, 244);
constexpr uint16_t kDim = Rgb(150, 156, 170);
constexpr uint16_t kAccent = Rgb(245, 182, 66);
constexpr uint16_t kNew = Rgb(120, 230, 140);  // something new, or within reach
constexpr int kLineGap = 22;
constexpr int kFadeInMs = 250;  // a line, or a place's name, coming up out of the dark
constexpr int kEchoGap = 26;
constexpr int kEchoLines = 4;

// Each voice in the echo stones has its colour.
uint16_t VoiceColour(Lore::Speaker speaker)
{
    switch (speaker)
    {
        case Lore::Speaker::Orlo: return Rgb(196, 160, 255);
        case Lore::Speaker::Maren: return Rgb(120, 230, 140);
        case Lore::Speaker::Puddle: return Rgb(245, 182, 66);
        case Lore::Speaker::Keeper: return Rgb(244, 63, 94);
        case Lore::Speaker::You: return kText;
    }
    return kText;
}

// A fall's lines: how it ended, where, what was kept, and waking up home.
struct FallLines
{
    static constexpr int kMost = 10;
    char text[kMost][48] = {};
    uint16_t colour[kMost] = {};
    int count = 0;

    void Add(uint16_t c, const char* line)
    {
        if (count < kMost)
        {
            std::snprintf(text[count], sizeof(text[count]), "%s", line);
            colour[count++] = c;
        }
    }
};

FallLines LinesOf(const CrawlerGame& game)
{
    FallLines lines;
    const CrawlerGame::DeathReport& report = game.GetDeathReport();
    char line[48];
    lines.Add(kText, game.GetMessage());
    if (report.depth > 0)
    {
        std::snprintf(line, sizeof(line), "You faint on B%d...", report.depth);
    }
    else
    {
        std::snprintf(line, sizeof(line), "You faint in the Seam...");
    }
    lines.Add(kText, line);
    if (report.record)
    {
        lines.Add(kAccent, "You have never been so deep.");
    }
    if (report.keptGold > 0 || report.echoGold > 0)
    {
        std::snprintf(line, sizeof(line), "The bank kept %d gold.", report.keptGold);
        lines.Add(kAccent, line);
    }
    if (report.echoGold > 0)
    {
        std::snprintf(line, sizeof(line), "Your echo holds %d gold.", report.echoGold);
        lines.Add(kText, line);
    }
    if (report.materials > 0)
    {
        std::snprintf(line, sizeof(line),
                      report.materials == 1 ? "1 material made it home." : "%d materials made it home.",
                      report.materials);
        lines.Add(kText, line);
    }
    // What the run opened up, and what to work toward: the reason to go
    // down once more.
    if (report.unlocked)
    {
        lines.Add(kNew, report.unlocked);
    }
    if (report.goal[0][0] != 0)
    {
        lines.Add(report.goalReady ? kNew : kText, report.goal[0]);
    }
    if (report.goal[1][0] != 0)
    {
        lines.Add(kDim, report.goal[1]);
    }
    lines.Add(kDim, "You wake up in Last Hearth.");
    return lines;
}

}  // namespace

// Toward black by `amount` of 255, over every pixel of the rows: done here,
// a row at a time, as a call for each pixel costs the PicoCalc 50 ms a frame.
void Darken(uint16_t* dst, int stride, int rowCount, int clipLeft, int clipRight, int width, int amount)
{
    const int to = clipRight < width ? clipRight : width;
    // How much of each pixel stays, in 32nds: none once it is all but black.
    const uint32_t keep = amount >= 248 ? 0 : 32 - (static_cast<uint32_t>(amount > 0 ? amount : 0) >> 3);
    for (int i = 0; i < rowCount; ++i)
    {
        uint16_t* out = dst + i * stride;
        if (keep == 0)
        {
            for (int x = clipLeft; x < to; ++x)
            {
                out[x] = 0;
            }
            continue;
        }
        for (int x = clipLeft; x < to; ++x)
        {
            // Green moved up out of the way of red and blue: one multiply.
            const uint32_t spread = (out[x] | (static_cast<uint32_t>(out[x]) << 16)) & 0x07E0F81Fu;
            const uint32_t kept = ((spread * keep) >> 5) & 0x07E0F81Fu;
            out[x] = static_cast<uint16_t>(kept | (kept >> 16));
        }
    }
}

int FallLength(const CrawlerGame& game)
{
    return kFallBlackMs + LinesOf(game).count * kFallLineMs + kFallReadMs + kFallOutMs;
}

void DrawFall(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int width, int height,
              const CrawlerGame& game, int deadMs)
{
    Darken(dst, stride, rowCount, clipLeft, clipRight, width,
           deadMs < kFallBlackMs ? deadMs * 255 / kFallBlackMs : 255);
    const FallLines lines = LinesOf(game);
    const int length = kFallBlackMs + lines.count * kFallLineMs + kFallReadMs + kFallOutMs;
    // The lines leave together at the end, back into the dark.
    const int out = deadMs > length - kFallOutMs ? (length - deadMs) * 255 / kFallOutMs : 255;
    const int top = (height - lines.count * kLineGap) / 2;
    for (int i = 0; i < lines.count; ++i)
    {
        const int since = deadMs - (kFallBlackMs + i * kFallLineMs);
        if (since <= 0)
        {
            break;
        }
        int alpha = since < kFadeInMs ? since * 255 / kFadeInMs : 255;
        alpha = alpha * (out < 0 ? 0 : out) / 255;
        const uint16_t colour = SmoothText::Mix(0x0000, lines.colour[i], alpha);
        SmoothText::Draw(dst, stride, firstRow, rowCount, clipLeft, clipRight,
                         (width - SmoothText::Width(lines.text[i], SmoothFont::kSmall)) / 2, top + i * kLineGap,
                         lines.text[i], SmoothFont::kSmall, colour, i == 0);
    }
}

int EchoShownMs(bool ending)
{
    return kEchoDarkMs + (kEchoLines - 1 + (ending ? 1 : 0)) * kEchoLineMs + kFadeInMs;
}

void DrawEcho(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int width, int height,
              const Lore::Echo& echo, bool ending, int ms)
{
    Darken(dst, stride, rowCount, clipLeft, clipRight, width, ms < kEchoDarkMs ? ms * 255 / kEchoDarkMs : 255);
    const int top = (height - kEchoLines * kEchoGap) / 2 - (ending ? kEchoGap / 2 : 0);
    const auto draw = [&](int line, int y, uint16_t colour, const char* text)
    {
        const int since = ms - (kEchoDarkMs + line * kEchoLineMs);
        if (since <= 0)
        {
            return;
        }
        const int alpha = since < kFadeInMs ? since * 255 / kFadeInMs : 255;
        SmoothText::Draw(dst, stride, firstRow, rowCount, clipLeft, clipRight,
                         (width - SmoothText::Width(text, SmoothFont::kSmall)) / 2, y, text, SmoothFont::kSmall,
                         SmoothText::Mix(0x0000, colour, alpha), true);
    };
    const char* text = echo.text;
    for (int i = 0; i < kEchoLines && *text != 0; ++i)
    {
        char row[40];
        int n = 0;
        while (text[n] != 0 && text[n] != '\n' && n < 38)
        {
            row[n] = text[n];
            ++n;
        }
        row[n] = 0;
        draw(i, top + i * kEchoGap, VoiceColour(echo.voice[i]), row);
        text += n;
        if (*text == '\n')
        {
            ++text;
        }
    }
    if (ending)
    {
        draw(kEchoLines, top + kEchoLines * kEchoGap + kEchoGap / 2, kDim, "THE END");
    }
}

void DrawPlace(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int width,
               int height, const char* name, const char* floor, int msLeft)
{
    if (msLeft <= 0)
    {
        return;
    }
    // Black, until the last moments let the view through.
    const int black = msLeft < kPlaceOutMs ? msLeft * 255 / kPlaceOutMs : 255;
    Darken(dst, stride, rowCount, clipLeft, clipRight, width, black);
    const int since = kPlaceMs - msLeft;
    // The name leaves faster than the dark, so it never floats over the view.
    const int alpha = (since < kFadeInMs ? since * 255 / kFadeInMs : 255) * black / 255 * black / 255;
    const bool hasFloor = floor && floor[0] != 0;
    const int nameY = hasFloor ? height / 2 - 22 : height / 2 - 12;
    SmoothText::Draw(dst, stride, firstRow, rowCount, clipLeft, clipRight,
                     (width - SmoothText::Width(name, SmoothFont::kLarge)) / 2, nameY, name, SmoothFont::kLarge,
                     SmoothText::Mix(0x0000, kText, alpha), true);
    if (hasFloor)
    {
        SmoothText::Draw(dst, stride, firstRow, rowCount, clipLeft, clipRight,
                         (width - SmoothText::Width(floor, SmoothFont::kSmall)) / 2, nameY + 32, floor,
                         SmoothFont::kSmall, SmoothText::Mix(0x0000, kAccent, alpha), true);
    }
}

}  // namespace Dungeon::Transitions
