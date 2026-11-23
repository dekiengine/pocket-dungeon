#pragma once

// What makes the game feel good to play: the numbers that pop out of what
// was hit, sparks and goo, the moment's freeze on a blow, the view's jolt,
// the sword's streak, red edges when hurt, coins thrown up when gold comes
// in, LEVEL UP!, and the HUD's HP and gold catching up instead of jumping.
// Read from the game each frame (CrawlerGame::TakeEvents and what changed),
// drawn into rows of RGB565 over the 3D view. No engine.

#include "../world/CrawlerGame.h"
#include "../render/Raycaster.h"

#include <cstdint>

namespace Dungeon
{

class ViewEffects
{
public:
    // Colours the HUD uses too.
    static constexpr uint16_t kTrailColour = 0xFF59;  // the HP just lost, light
    static constexpr uint16_t kLevelColour = 0xFEAB;  // warm gold
    static constexpr uint16_t kCoinLight = 0xFFB7;

    /// `dot` is drawn for the sparks and the goo (a soft round sprite).
    void SetDot(const Texture* dot) { m_Dot = dot; }

    /// Forgets everything showing; what the game holds now is the start, so
    /// nothing pops for it.
    void Reset(CrawlerGame& game);

    /// How much of `ms` the game moves this frame: none while a blow holds
    /// it (hit-stop).
    int GameMs(int ms);

    /// Takes what happened from the game and moves everything on by `ms`.
    /// `textures` gives the monsters' colours for their goo.
    void Update(CrawlerGame& game, const CrawlerTextures& textures, int viewWidth, int viewHeight, int ms);

    /// How many of each kind of event the last Update took (by EventKind):
    /// for whatever else wants to know of them, the sounds.
    static constexpr int kEventKinds = 6;
    const uint8_t* Heard() const { return m_Heard; }

    /// Rows the view is jolted down (or up) by this frame.
    int Kick() const;

    /// Adds the sparks and the goo as billboards after `count` of them in
    /// `out` (up to `max`); the new count.
    int AddBillboards(Billboard* out, int count, int max) const;

    /// Places the numbers in the view, after the raycaster's Prepare.
    void Place(const Raycaster& ray);

    /// True while anything shows or moves (the view must be redrawn).
    bool IsActive() const;

    /// Draws view rows [firstRow, firstRow + rowCount) of it into `dst` (the
    /// first of those rows, `stride` pixels a row), only in columns
    /// [clipLeft, clipRight).
    void Draw(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight) const;

    /// The HUD: HP times 16 down to which the bar shows the HP just lost,
    /// the gold to show, and whether a level was just gained.
    int GetHpTrail() const { return m_HpTrail; }
    int GetGoldShown() const { return m_GoldShown; }
    bool IsGoldRolling() const { return m_GoldShown >= 0 && m_GoldShown < m_LastGold; }
    int GetLevelMs() const { return m_LevelMs; }

private:
    // Numbers: over what was hit (`world`, placed each frame) or in the view
    // (gold, a hurt), rising.
    struct Popup
    {
        Fix x = 0;
        Fix y = 0;
        int16_t screenX = 0;  // in view, for one that is not in the world
        int16_t screenY = 0;
        int16_t drawX = 0;  // where it is this frame, its middle
        int16_t drawY = 0;
        int16_t ms = 0;            // left to live; 0: free
        uint16_t colour = 0xFFFF;  // 0: the gold gradient (a critical)
        uint8_t scale = 2;
        bool world = false;
        bool shown = false;  // placed in view this frame
        uint8_t above = 70;  // a world one starts this many hundredths of a wall up
        char text[10] = {};
    };
    static constexpr int kPopups = 8;
    // Bits thrown off: sparks from a hit, goo from a kill. In the world,
    // drawn as billboards, falling.
    struct Bit
    {
        Fix x = 0;
        Fix y = 0;
        Fix height = 0;
        Fix vx = 0;  // tiles a second
        Fix vy = 0;
        Fix vz = 0;
        int16_t ms = 0;
        uint8_t kind = 0;  // 0 spark, 1 goo
    };
    static constexpr int kBits = 16;
    // Coins thrown up from the bottom of the view, in view pixels times 16.
    struct Coin
    {
        int x = 0;
        int y = 0;
        int vx = 0;
        int vy = 0;
        int ms = 0;
    };
    static constexpr int kCoins = 10;

    // Something found: its picture tossed up in the middle of the view and
    // its name under it, sparks flying off, as coins are thrown for gold.
    struct Prize
    {
        const Texture* icon = nullptr;
        int16_t ms = 0;  // left to show; 0: none
        char text[20] = {};
    };
    Prize m_Prize;
    uint32_t m_LastResult = 0;
    Popup m_Popups[kPopups];
    Bit m_Bits[kBits];
    Coin m_Coins[kCoins];
    uint16_t m_GooPalette[3] = {};
    const Texture* m_Dot = nullptr;
    uint32_t m_Seed = 777;
    int m_Width = 320;
    int m_Height = 240;
    int m_HitStopMs = 0;
    int m_KickMs = 0;
    int m_KickRows = 0;
    int m_HurtMs = 0;
    int m_LevelMs = 0;
    int m_SlashMs = 0;
    int m_SlashSide = 0;
    bool m_WasSwinging = false;
    int m_HpTrail = -1;
    int m_HpTrailWaitMs = 0;
    int m_GoldShown = -1;
    int m_GoldTickMs = 0;
    int m_LastHp = -1;
    int m_LastGold = -1;
    int m_LastLevel = -1;
    int m_LastXp = -1;
    bool m_LastVillage = false;

    int Random(int range);
    void Kick(int ms, int rows);
    Popup& AddPopup(const char* text, uint16_t colour, int scale, bool world, Fix x, Fix y, int screenX, int screenY);
    void AddBits(Fix x, Fix y, Fix height, int count, int kind, int speed);
    void SetGoo(const Texture* monster, uint16_t otherwise = 0x7EEB);
    void DrawHurt(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int right) const;
    void DrawSlash(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int right) const;
    uint8_t m_Heard[kEventKinds] = {};
};

}  // namespace Dungeon
