#include "ViewEffects.h"

#include "../Hot.h"
#include "../render/PixelText.h"
#include "../render/SmoothText.h"

#include <cstdio>

namespace Dungeon
{

namespace
{

using SmoothText::Rgb;

// Sparks glow: drawn unshaded.
constexpr uint16_t kSparkPalette[3] = { kTransparent, Rgb(255, 196, 90), Rgb(255, 250, 220) };
constexpr uint16_t kHitColour = 0xFFFF;
constexpr uint16_t kHurtNumber = Rgb(255, 84, 104);
constexpr uint16_t kHealColour = Rgb(110, 240, 140);
constexpr uint16_t kXpColour = Rgb(190, 160, 255);
constexpr uint16_t kGoldColour = Rgb(255, 214, 0);
constexpr uint16_t kMissColour = Rgb(170, 178, 192);
constexpr uint16_t kSlashColour = Rgb(255, 250, 232);
constexpr uint16_t kHurtColour = Rgb(200, 0, 24);
constexpr uint16_t kCoinColour = Rgb(255, 198, 40);
constexpr uint16_t kCoinDark = Rgb(176, 112, 10);
// A critical's number and LEVEL UP!, top row to bottom: white-hot to orange.
constexpr uint16_t kGoldGradient[10] = { Rgb(255, 255, 236), Rgb(255, 248, 170), Rgb(255, 236, 110), Rgb(255, 220, 60),
                                         Rgb(255, 200, 30),  Rgb(255, 176, 20),  Rgb(250, 148, 16),  Rgb(240, 120, 12),
                                         Rgb(220, 92, 10),   Rgb(190, 66, 8) };
constexpr int kPopupMs = 800;
constexpr int kPrizeMs = 1300;
constexpr int kSlashMs = 150;
constexpr int kHurtMs = 320;
constexpr int kLevelUpMs = 1700;

int SquareRoot(int n)
{
    if (n <= 0)
    {
        return 0;
    }
    int root = 0;
    int bit = 1 << 30;
    while (bit > n)
    {
        bit >>= 2;
    }
    while (bit != 0)
    {
        if (n >= root + bit)
        {
            n -= root + bit;
            root = (root >> 1) + bit;
        }
        else
        {
            root >>= 1;
        }
        bit >>= 2;
    }
    return root;
}

Fix Times(Fix a, Fix b)
{
    return static_cast<Fix>((static_cast<int64_t>(a) * b) >> kFixShift);
}

Fix Centre(int cell)
{
    return (cell << kFixShift) + kFixOne / 2;
}

}  // namespace

int ViewEffects::Random(int range)
{
    m_Seed = m_Seed * 1103515245u + 12345u;
    return static_cast<int>((m_Seed >> 8) % static_cast<uint32_t>(range > 0 ? range : 1));
}

void ViewEffects::Reset(CrawlerGame& game)
{
    for (Popup& p : m_Popups)
    {
        p = Popup{};
    }
    for (Bit& b : m_Bits)
    {
        b = Bit{};
    }
    for (Coin& c : m_Coins)
    {
        c = Coin{};
    }
    m_Prize = Prize{};
    m_LastResult = game.GetResultCount();
    m_HitStopMs = 0;
    m_KickMs = 0;
    m_HurtMs = 0;
    m_LevelMs = 0;
    m_SlashMs = 0;
    m_LastHp = -1;  // read afresh on the next update
    Event discard[CrawlerGame::kMaxEvents];
    game.TakeEvents(discard, CrawlerGame::kMaxEvents);
}

int ViewEffects::GameMs(int ms)
{
    const int moved = m_HitStopMs > 0 ? 0 : ms;
    m_HitStopMs = m_HitStopMs > ms ? m_HitStopMs - ms : 0;
    return moved;
}

void ViewEffects::Kick(int ms, int rows)
{
    m_KickMs = m_KickMs > ms ? m_KickMs : ms;
    m_KickRows = m_KickRows > rows && m_KickMs > 0 ? m_KickRows : rows;
}

int ViewEffects::Kick() const
{
    if (m_KickMs <= 0)
    {
        return 0;
    }
    // Up and down every 30 ms, less and less.
    const int rows = m_KickRows * m_KickMs / 180 + 1;
    return (m_KickMs / 30) % 2 == 0 ? rows : -rows;
}

ViewEffects::Popup& ViewEffects::AddPopup(const char* text, uint16_t colour, int scale, bool world, Fix x, Fix y,
                                          int screenX, int screenY)
{
    // A free one, or else the one nearest its end.
    Popup* slot = &m_Popups[0];
    for (Popup& p : m_Popups)
    {
        if (p.ms <= 0)
        {
            slot = &p;
            break;
        }
        slot = p.ms < slot->ms ? &p : slot;
    }
    *slot = Popup{};
    std::snprintf(slot->text, sizeof(slot->text), "%s", text);
    slot->colour = colour;
    slot->scale = static_cast<uint8_t>(scale);
    slot->world = world;
    slot->x = x;
    slot->y = y;
    slot->screenX = static_cast<int16_t>(screenX);
    slot->screenY = static_cast<int16_t>(screenY);
    slot->ms = kPopupMs;
    return *slot;
}

void ViewEffects::AddBits(Fix x, Fix y, Fix height, int count, int kind, int speed)
{
    for (Bit& b : m_Bits)
    {
        if (count <= 0)
        {
            break;
        }
        if (b.ms > 0)
        {
            continue;
        }
        --count;
        // Off in any direction at `speed` hundredths of a tile a second,
        // give or take a half, and up.
        const int angle = Random(kAngleSteps);
        const Fix v = kFixOne / 100 * speed / 2 + kFixOne / 100 * Random(speed);
        b.x = x;
        b.y = y;
        b.height = height;
        b.vx = Times(Raycaster::Cos(angle), v);
        b.vy = Times(Raycaster::Sin(angle), v);
        b.vz = kFixOne * (60 + Random(110)) / 100;
        b.kind = static_cast<uint8_t>(kind);
        b.ms = static_cast<int16_t>(kind == 0 ? 280 + Random(200) : 600 + Random(350));
    }
}

void ViewEffects::SetGoo(const Texture* monster, uint16_t otherwise)
{
    // The monster's own colour: the average of its texels, every third.
    uint16_t goo = otherwise;
    if (monster && monster->index)
    {
        uint32_t r = 0;
        uint32_t g = 0;
        uint32_t b = 0;
        uint32_t n = 0;
        const uint16_t* palette = monster->Palette(0);
        for (int k = 0; k < monster->size * monster->size; k += 3)
        {
            if (monster->index[k] != 0)
            {
                const uint16_t c = palette[monster->index[k]];
                r += (c >> 11) & 31;
                g += (c >> 5) & 63;
                b += c & 31;
                ++n;
            }
        }
        if (n > 0)
        {
            goo = static_cast<uint16_t>(((r / n) << 11) | ((g / n) << 5) | (b / n));
        }
    }
    m_GooPalette[0] = kTransparent;
    m_GooPalette[1] = SmoothText::Mix(goo, 0xFFFF, 40);
    m_GooPalette[2] = SmoothText::Mix(goo, 0xFFFF, 150);
}

void ViewEffects::Update(CrawlerGame& game, const CrawlerTextures& textures, int viewWidth, int viewHeight, int ms)
{
    m_Width = viewWidth;
    m_Height = viewHeight;
    Fix heroX = 0;
    Fix heroY = 0;
    int angle = 0;
    game.GetView(heroX, heroY, angle);

    // What happened in the world: numbers, sparks, goo, a moment's freeze.
    Event events[CrawlerGame::kMaxEvents];
    const int eventCount = game.TakeEvents(events, CrawlerGame::kMaxEvents);
    for (uint8_t& heard : m_Heard)
    {
        heard = 0;
    }
    for (int i = 0; i < eventCount; ++i)
    {
        const int kind = static_cast<int>(events[i].kind);
        m_Heard[kind < kEventKinds ? kind : 0] += 1;
    }
    bool killed = false;
    Fix killX = 0;
    Fix killY = 0;
    char text[12];
    for (int i = 0; i < eventCount; ++i)
    {
        const Event& e = events[i];
        const Fix ex = Centre(e.at.x);
        const Fix ey = Centre(e.at.y);
        // Sparks fly off its front, the side the blow came from.
        const Fix frontX = ex + (heroX - ex) * 3 / 10;
        const Fix frontY = ey + (heroY - ey) * 3 / 10;
        switch (e.kind)
        {
            case EventKind::Hit:
                std::snprintf(text, sizeof(text), "%d", e.amount);
                AddPopup(text, kHitColour, 2, true, ex, ey, 0, 0);
                AddBits(frontX, frontY, kFixOne * 45 / 100, 5, 0, 160);
                m_HitStopMs = m_HitStopMs > 45 ? m_HitStopMs : 45;
                Kick(80, 2);
                break;
            case EventKind::Critical:
                std::snprintf(text, sizeof(text), "%d!", e.amount);
                AddPopup(text, 0, 3, true, ex, ey, 0, 0);
                AddBits(frontX, frontY, kFixOne * 45 / 100, 10, 0, 240);
                m_HitStopMs = 95;
                Kick(180, 5);
                break;
            case EventKind::Miss: AddPopup("MISS", kMissColour, 2, true, ex, ey, 0, 0); break;
            case EventKind::Kill:
                SetGoo(textures.monsters[static_cast<int>(e.monster)]);
                AddBits(ex, ey, kFixOne * 35 / 100, 12, 1, 200);
                m_HitStopMs = m_HitStopMs > 80 ? m_HitStopMs : 80;
                Kick(150, 4);
                killed = true;
                killX = ex;
                killY = ey;
                break;
            case EventKind::Break:
                if (e.prop >= 0 && e.prop < kPropKinds)
                {
                    SetGoo(textures.props[e.prop]);
                }
                AddBits(ex, ey, kFixOne * 25 / 100, 14, 1, 230);
                m_HitStopMs = m_HitStopMs > 50 ? m_HitStopMs : 50;
                Kick(130, 3);
                break;
            case EventKind::Chip:
                // Stone off the wall ahead, from just in front of its face,
                // in the wall's own colour.
                SetGoo(textures.rubble, Rgb(128, 124, 116));
                AddBits(ex + (heroX - ex) * 6 / 10, ey + (heroY - ey) * 6 / 10, kFixOne * 50 / 100, 9, 1, 170);
                m_HitStopMs = m_HitStopMs > 40 ? m_HitStopMs : 40;
                Kick(110, 3);
                break;
        }
    }

    // Something found (what a deed did, with a thing in it): tossed up.
    // Gold has its coins already.
    if (game.GetResultCount() != m_LastResult)
    {
        m_LastResult = game.GetResultCount();
        const int item = game.GetResultItem();
        if (item >= 0 && item < kItemKinds && item != static_cast<int>(ItemKind::Gold) && textures.items[item])
        {
            m_Prize.icon = textures.items[item];
            m_Prize.ms = kPrizeMs;
            std::snprintf(m_Prize.text, sizeof(m_Prize.text), "%s!",
                          CrawlerGame::ItemName(static_cast<ItemKind>(item)));
            for (char* c = m_Prize.text; *c != 0; ++c)
            {
                *c = *c >= 'a' && *c <= 'z' ? static_cast<char>(*c - 'a' + 'A') : *c;
            }
        }
    }

    // What the HUD counts: HP, gold, the level and experience.
    const bool village = game.IsInVillage();
    const int hp = game.GetHp();
    const int gold = village ? game.GetLegend().bank : game.GetGold();
    const int level = game.GetLevel();
    const int xp = game.GetXp();
    if (m_LastHp < 0 || village != m_LastVillage)
    {
        m_HpTrail = hp * 16;
        m_GoldShown = gold;
    }
    else
    {
        if (hp < m_LastHp)
        {
            // Hurt: red round the edges, a jolt, the number, and the HP lost
            // lingers on the bar a moment.
            m_HurtMs = kHurtMs;
            Kick(140, 4);
            std::snprintf(text, sizeof(text), "-%d", m_LastHp - hp);
            AddPopup(text, kHurtNumber, 2, false, 0, 0, m_Width / 2 - 70 + Random(30), m_Height - 60);
            m_HpTrail = m_HpTrail > m_LastHp * 16 ? m_HpTrail : m_LastHp * 16;
            m_HpTrailWaitMs = 420;
        }
        else if (hp > m_LastHp && level == m_LastLevel)
        {
            std::snprintf(text, sizeof(text), "+%d", hp - m_LastHp);
            AddPopup(text, kHealColour, 2, false, 0, 0, m_Width / 2 - 70, m_Height - 60);
        }
        if (gold > m_LastGold)
        {
            // Gold: coins thrown up, the amount, and the count rolls up.
            std::snprintf(text, sizeof(text), "+%dG", gold - m_LastGold);
            AddPopup(text, kGoldColour, 2, false, 0, 0, m_Width / 2 + 50, m_Height - 70);
            int coins = 4 + (gold - m_LastGold) / 3;
            for (Coin& c : m_Coins)
            {
                if (coins <= 0)
                {
                    break;
                }
                if (c.ms > 0)
                {
                    continue;
                }
                --coins;
                c.x = (m_Width / 2 + 30 + Random(40)) * 16;
                c.y = (m_Height - 4) * 16;
                c.vx = (Random(260) - 130) * 16;
                c.vy = -(300 + Random(220)) * 16;
                c.ms = 1200;
            }
        }
        else if (gold < m_LastGold)
        {
            m_GoldShown = gold;  // spent: no counting down
        }
        if (level > m_LastLevel)
        {
            m_LevelMs = kLevelUpMs;
            AddBits(heroX + Raycaster::Cos(angle) * 3 / 4, heroY + Raycaster::Sin(angle) * 3 / 4, kFixOne / 4, 12, 0,
                    260);
        }
        else if (xp > m_LastXp)
        {
            std::snprintf(text, sizeof(text), "+%dXP", xp - m_LastXp);
            if (killed)
            {
                AddPopup(text, kXpColour, 2, true, killX, killY, 0, 0).above = 105;  // over the last blow's number
            }
            else
            {
                AddPopup(text, kXpColour, 2, false, 0, 0, m_Width / 2, m_Height - 90);
            }
        }
    }
    m_LastHp = hp;
    m_LastGold = gold;
    m_LastLevel = level;
    m_LastXp = xp;
    m_LastVillage = village;

    // The HUD catching up: the HP lost drains after a moment, the gold
    // counts up in steps.
    if (m_HpTrail < hp * 16)
    {
        m_HpTrail = hp * 16;
    }
    else if (m_HpTrail > hp * 16)
    {
        if (m_HpTrailWaitMs > 0)
        {
            m_HpTrailWaitMs -= ms;
        }
        else
        {
            const int drop = game.GetMaxHp() * 16 * ms / 700;
            m_HpTrail -= drop > 1 ? drop : 1;
            m_HpTrail = m_HpTrail > hp * 16 ? m_HpTrail : hp * 16;
        }
    }
    m_GoldTickMs += ms;
    while (m_GoldTickMs >= 30)
    {
        m_GoldTickMs -= 30;
        if (m_GoldShown < gold)
        {
            const int step = (gold - m_GoldShown) / 6;
            m_GoldShown += step > 1 ? step : 1;
        }
    }
    m_GoldShown = m_GoldShown > gold ? gold : m_GoldShown;

    // A new swing of the sword: its streak, from alternate sides.
    const bool swinging = game.SwingProgress() >= 0;
    if (swinging && !m_WasSwinging)
    {
        m_SlashMs = kSlashMs;
        m_SlashSide ^= 1;
    }
    m_WasSwinging = swinging;

    // Everything ages.
    const auto down = [ms](int& value) { value = value > ms ? value - ms : 0; };
    down(m_KickMs);
    down(m_HurtMs);
    down(m_LevelMs);
    down(m_SlashMs);
    for (Popup& p : m_Popups)
    {
        p.ms = static_cast<int16_t>(p.ms > ms ? p.ms - ms : 0);
    }
    m_Prize.ms = static_cast<int16_t>(m_Prize.ms > ms ? m_Prize.ms - ms : 0);
    const Fix gravity = kFixOne * 5;
    for (Bit& b : m_Bits)
    {
        if (b.ms <= 0)
        {
            continue;
        }
        b.ms = static_cast<int16_t>(b.ms > ms ? b.ms - ms : 0);
        b.vz -= gravity / 1000 * ms;
        b.x += b.vx / 1000 * ms;
        b.y += b.vy / 1000 * ms;
        b.height += b.vz / 1000 * ms;
        if (b.height < 0)
        {
            // Sparks bounce and skid; goo splats and stays.
            b.height = 0;
            b.vz = b.kind == 0 ? -b.vz / 3 : 0;
            b.vx = b.kind == 0 ? b.vx / 2 : 0;
            b.vy = b.kind == 0 ? b.vy / 2 : 0;
        }
    }
    for (Coin& c : m_Coins)
    {
        if (c.ms <= 0)
        {
            continue;
        }
        c.ms = c.ms > ms ? c.ms - ms : 0;
        c.vy += 1100 * 16 * ms / 1000;
        c.x += c.vx * ms / 1000;
        c.y += c.vy * ms / 1000;
        if (c.y > (m_Height + 8) * 16)
        {
            c.ms = 0;
        }
    }
}

bool ViewEffects::IsActive() const
{
    if (m_KickMs > 0 || m_HurtMs > 0 || m_LevelMs > 0 || m_SlashMs > 0 || m_HitStopMs > 0 || m_Prize.ms > 0)
    {
        return true;
    }
    for (const Popup& p : m_Popups)
    {
        if (p.ms > 0)
        {
            return true;
        }
    }
    for (const Bit& b : m_Bits)
    {
        if (b.ms > 0)
        {
            return true;
        }
    }
    for (const Coin& c : m_Coins)
    {
        if (c.ms > 0)
        {
            return true;
        }
    }
    return false;
}

int ViewEffects::AddBillboards(Billboard* out, int count, int max) const
{
    if (!m_Dot)
    {
        return count;
    }
    for (const Bit& bit : m_Bits)
    {
        if (bit.ms <= 0 || count >= max)
        {
            continue;
        }
        Billboard& b = out[count++];
        b.foot = 0;
        b = Billboard{};
        b.x = bit.x;
        b.y = bit.y;
        b.texture = m_Dot;
        // Goo shrinks as it dries; a spark is gone in a flash.
        const int size = bit.kind == 0 ? 5 : 4 + (bit.ms > 300 ? 4 : bit.ms / 75);
        b.scale = kFixOne * size / 100;
        b.palette = bit.kind == 0 ? kSparkPalette : m_GooPalette;
        b.lift = static_cast<Fix>((static_cast<int64_t>(bit.height) << kFixShift) / b.scale);
        b.squash = bit.kind == 1 && bit.height == 0 ? kFixOne * 2 : kFixOne;  // a splat lies flat
    }
    return count;
}

void ViewEffects::Place(const Raycaster& ray)
{
    for (Popup& p : m_Popups)
    {
        p.shown = false;
        if (p.ms <= 0)
        {
            continue;
        }
        const int age = kPopupMs - p.ms;
        if (p.world)
        {
            // Over the monster, rising and slowing: up 0.4 of a wall in all.
            const int eased = 1024 - (p.ms * p.ms / kPopupMs) * 1024 / kPopupMs;
            const Fix height = kFixOne * p.above / 100 + kFixOne / 1024 * eased * 4 / 10;
            int x = 0;
            int y = 0;
            Fix depth = 0;
            if (!ray.Project(p.x, p.y, height, x, y, depth))
            {
                continue;
            }
            // Kept inside the view when what was hit is close and tall.
            p.drawX = static_cast<int16_t>(x);
            p.drawY = static_cast<int16_t>(y > 18 ? y : 18);
        }
        else
        {
            p.drawX = p.screenX;
            p.drawY = static_cast<int16_t>(p.screenY - age * 40 / kPopupMs);
        }
        p.shown = true;
    }
}

DUNGEON_HOT void ViewEffects::DrawHurt(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft,
                                       int right) const
{
    // The edges of the view go red, most at the very edge.
    constexpr int kBand = 40;
    const int strength = m_HurtMs * 190 / kHurtMs;
    for (int i = 0; i < rowCount; ++i)
    {
        const int row = firstRow + i;
        const int edgeY = row < m_Height - 1 - row ? row : m_Height - 1 - row;
        uint16_t* out = dst + i * stride;
        for (int x = clipLeft; x < right; ++x)
        {
            if (edgeY >= kBand && x >= kBand && x < m_Width - kBand)
            {
                x = m_Width - kBand;  // the middle untouched
                if (x >= right)
                {
                    break;
                }
            }
            const int edgeX = x < m_Width - 1 - x ? x : m_Width - 1 - x;
            const int edge = edgeX < edgeY ? edgeX : edgeY;
            if (edge < kBand)
            {
                const int t = kBand - edge;
                out[x] = SmoothText::Mix(out[x], kHurtColour, strength * t * t / (kBand * kBand));
            }
        }
    }
}

DUNGEON_HOT void ViewEffects::DrawSlash(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft,
                                        int right) const
{
    // An arc swept across the view: a bright head and a fading tail, thick
    // in the middle of its run, brightest along its middle line.
    const int age = (kSlashMs - m_SlashMs) * 1024 / kSlashMs;
    constexpr int kRadius = 250;
    constexpr int kTail = 90;
    const int centreX = m_SlashSide ? 20 : m_Width - 20;
    const int centreY = m_Height + 70;
    const int topRow = centreY - kRadius;
    const int run = m_Height - topRow;
    const int head = topRow + run * age / 700;  // past the bottom before it ends
    for (int i = 0; i < rowCount; ++i)
    {
        const int row = firstRow + i;
        if (row > head || row < head - kTail || row < topRow)
        {
            continue;
        }
        const int behind = head - row;  // 0 at the head
        const int along = (row - topRow) * 1024 / run;
        const int middle = along < 512 ? along : 1024 - along;
        const int thick = 3 + middle * 12 / 512 - behind * 5 / kTail;
        if (thick <= 0)
        {
            continue;
        }
        const int dy = row - centreY;
        const int outer = SquareRoot((kRadius + thick) * (kRadius + thick) - dy * dy);
        const int inner = SquareRoot((kRadius - thick) * (kRadius - thick) - dy * dy);
        const int x0 = m_SlashSide ? centreX + inner : centreX - outer;
        const int x1 = m_SlashSide ? centreX + outer : centreX - inner;
        const int strength = 240 - behind * 220 / kTail;
        const int span = x1 - x0 > 0 ? x1 - x0 : 1;
        uint16_t* out = dst + i * stride;
        for (int x = x0 > clipLeft ? x0 : clipLeft; x < x1 && x < right; ++x)
        {
            const int fromMiddle = 2 * (x - x0) - span;
            const int core = 256 - (fromMiddle < 0 ? -fromMiddle : fromMiddle) * 200 / span;
            out[x] = SmoothText::Mix(out[x], kSlashColour, strength * core >> 8);
        }
    }
}

DUNGEON_HOT void ViewEffects::Draw(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft,
                                   int clipRight) const
{
    const int right = clipRight < m_Width ? clipRight : m_Width;
    if (m_HurtMs > 0)
    {
        DrawHurt(dst, stride, firstRow, rowCount, clipLeft, right);
    }

    // A level gained: a warm flash over all of it, fading fast.
    if (m_LevelMs > kLevelUpMs - 300)
    {
        const int amount = (m_LevelMs - (kLevelUpMs - 300)) * 150 / 300;
        for (int i = 0; i < rowCount; ++i)
        {
            uint16_t* out = dst + i * stride;
            for (int x = clipLeft; x < right; ++x)
            {
                out[x] = SmoothText::Mix(out[x], kLevelColour, amount);
            }
        }
    }

    if (m_SlashMs > 0)
    {
        DrawSlash(dst, stride, firstRow, rowCount, clipLeft, right);
    }

    // Coins, spinning: wide, thin, wide.
    for (const Coin& c : m_Coins)
    {
        if (c.ms <= 0)
        {
            continue;
        }
        const int x = c.x >> 4;
        const int y = c.y >> 4;
        const int spin = Raycaster::Sin(c.ms * 3) >> 8;             // -256 .. 256
        const int half = 1 + ((spin < 0 ? -spin : spin) * 5 >> 8);  // 1 .. 6
        const int round = half > 3 ? 5 : 1;
        // A dark rim, the gold face, a glint at its top left, and a dark
        // line down the middle when edge-on.
        SmoothText::Fill(dst, stride, firstRow, rowCount, clipLeft, right, x - half - 1, y - 7, x + half + 2, y + 8,
                         round, kCoinDark);
        SmoothText::Fill(dst, stride, firstRow, rowCount, clipLeft, right, x - half, y - 6, x + half + 1, y + 7, round,
                         kCoinColour);
        if (half > 2)
        {
            SmoothText::Fill(dst, stride, firstRow, rowCount, clipLeft, right, x - half + 2, y - 4, x - half + 4, y, 1,
                             kCoinLight);
        }
        else
        {
            SmoothText::Fill(dst, stride, firstRow, rowCount, clipLeft, right, x, y - 5, x + 1, y + 6, 0, kCoinDark);
        }
    }

    using PixelText::Style;
    // Something found: thrown up fast, a size too big for a moment, then
    // itself, sparks flying off it; its name under it; and off upward,
    // blinking, as it goes.
    if (m_Prize.ms > 0 && m_Prize.icon && m_Prize.icon->index)
    {
        const Texture& icon = *m_Prize.icon;
        const int age = kPrizeMs - m_Prize.ms;
        const int base = icon.size <= 16 ? 2 : 1;
        const int scale = age < 70 ? base : (age < 220 ? base + 1 : base);
        int rise = age < 160 ? age * 44 / 160 : (age < 260 ? 44 - (age - 160) * 8 / 100 : 36);
        rise += m_Prize.ms < 220 ? (220 - m_Prize.ms) / 6 : 0;
        const int cx = m_Width / 2;
        const int cy = m_Height / 2 + 26 - rise;
        const bool going = m_Prize.ms < 220 && (m_Prize.ms / 45) % 2 == 0;
        const int size = icon.size * scale;
        for (int i = 0; i < rowCount && !going; ++i)
        {
            const int v = firstRow + i - (cy - size / 2);
            if (v < 0 || v >= size)
            {
                continue;
            }
            uint16_t* out = dst + i * stride;
            const uint8_t* row = icon.index + (v / scale) * icon.size;
            for (int u = 0; u < size; ++u)
            {
                const int x = cx - size / 2 + u;
                const uint16_t pixel = icon.palette[row[u / scale]];
                if (x >= clipLeft && x < right && pixel != kTransparent)
                {
                    out[x] = pixel;
                }
            }
        }
        if (age >= 120 && age < 620)
        {
            // Eight sparks, straight and slantwise, flying out and going.
            constexpr int kWay[8][2] = { { 10, 0 },  { 7, 7 },   { 0, 10 },  { -7, 7 },
                                         { -10, 0 }, { -7, -7 }, { 0, -10 }, { 7, -7 } };
            const int reach = 18 + (age - 120) * 26 / 500;
            const int big = age < 370 ? 2 : 1;
            for (const auto& way : kWay)
            {
                const int sx = cx + way[0] * reach / 10;
                const int sy = cy + way[1] * reach / 10;
                SmoothText::Fill(dst, stride, firstRow, rowCount, clipLeft, right, sx - big, sy - big, sx + big,
                                 sy + big, 0, kGoldColour);
            }
        }
        if (age >= 140 && !going)
        {
            Style style;
            style.scale = age < 230 ? 3 : 2;
            style.gradient = kGoldGradient;
            style.outlined = true;
            style.outline = 0x0000;
            style.shadowed = true;
            style.shadow = 0x0000;
            PixelText::Draw(dst, stride, firstRow, rowCount, clipLeft, right,
                            cx - PixelText::Width(m_Prize.text, style.scale) / 2, cy + 24, m_Prize.text, style);
        }
    }

    // The numbers: popping in a size up, shrinking a size as they go.
    for (const Popup& p : m_Popups)
    {
        if (!p.shown || p.ms <= 0)
        {
            continue;
        }
        const int age = kPopupMs - p.ms;
        Style style;
        style.scale = p.scale + (age < 90 ? 1 : 0) - (p.ms < 160 && p.scale > 1 ? 1 : 0);
        style.colour = p.colour != 0 ? p.colour : 0xFFFF;
        style.gradient = p.colour == 0 ? kGoldGradient : nullptr;
        style.outlined = true;
        style.outline = 0x0000;
        style.shadowed = true;
        style.shadow = 0x0000;
        const int w = PixelText::Width(p.text, style.scale);
        const int h = PixelText::Height(style.scale);
        PixelText::Draw(dst, stride, firstRow, rowCount, clipLeft, right, p.drawX - w / 2, p.drawY - h / 2, p.text,
                        style);
    }

    // LEVEL UP!, popping in big and settling, then floating off.
    if (m_LevelMs > 0)
    {
        const int age = kLevelUpMs - m_LevelMs;
        Style style;
        style.scale = age < 70 ? 6 : (age < 140 ? 5 : 4);
        style.gradient = kGoldGradient;
        style.outlined = true;
        style.outline = Rgb(60, 20, 0);
        style.shadowed = true;
        style.shadow = 0x0000;
        const char* word = "LEVEL UP!";
        const int y = m_LevelMs < 300 ? 64 - (300 - m_LevelMs) / 6 : 64;
        PixelText::Draw(dst, stride, firstRow, rowCount, clipLeft, right,
                        (m_Width - PixelText::Width(word, style.scale)) / 2, y, word, style);
    }
}

}  // namespace Dungeon
