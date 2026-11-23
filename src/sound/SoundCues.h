#pragma once

// What the game sounds like: watches the game from frame to frame and asks
// the sound chip (Chip.h) for a sound when something changed. A step, a
// blow, gold, a menu's line, a lid coming up, someone speaking. No engine.

#include "../world/CrawlerGame.h"
#include "Chip.h"

namespace Dungeon
{

class SoundCues
{
public:
    /// Takes the game as it is for the start: nothing sounds for what is
    /// already so.
    void Reset(const CrawlerGame& game);

    /// Sounds what changed since the last call. `heard` counts the blows
    /// that just landed, by EventKind (the screen takes them from the game);
    /// `typed` is how many letters of someone's speech are out (0: nobody
    /// speaks); `resultMs` how long a result has been up, shown after
    /// `resultWaitMs` when it was something shut.
    /// `cues` are the sounds the game asked for by name (`cueCount` of them),
    /// and `ms` is how long since the last call: a place makes its own quiet
    /// noises now and then, and a heart beats when the hero is nearly gone.
    void Update(const CrawlerGame& game, const uint8_t* heard, int typed, int resultMs, int resultWaitMs,
                const Cue* cues, int cueCount, int ms, Chip::Player& player);

private:
    Cell m_Hero;
    int m_Hp = 0;
    int m_Mp = 0;
    int m_Gold = 0;
    int m_Level = 0;
    int m_Carried = 0;
    int m_Depth = 0;
    int m_Inside = -1;
    bool m_InVillage = false;
    bool m_Dead = false;
    Menu m_Menu = Menu::None;
    int m_Cursor = 0;
    int m_Typed = 0;
    uint32_t m_Result = 0;
    bool m_Revealed = true;
    int m_AmbientMs = 6000;          // until the place's next noise
    int m_HeartMs = 0;               // until the next heartbeat
    uint32_t m_Chance = 0x2F6E2B1u;  // for when the noises come

    static int CarriedIn(const CrawlerGame& game);
};

}  // namespace Dungeon
