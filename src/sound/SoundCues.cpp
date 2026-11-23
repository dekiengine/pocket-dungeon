#include "SoundCues.h"

#include "../world/CrawlerGameData.h"

namespace Dungeon
{

using Chip::Sfx;

int SoundCues::CarriedIn(const CrawlerGame& game)
{
    int carried = 0;
    for (const ItemKind kind : GameData::kBagKinds)
    {
        carried += game.Carried(kind);
    }
    return carried;
}

void SoundCues::Reset(const CrawlerGame& game)
{
    m_Hero = game.GetHeroCell();
    m_Hp = game.GetHp();
    m_Mp = game.GetMp();
    m_Gold = game.GetGold();
    m_Level = game.GetLevel();
    m_Carried = CarriedIn(game);
    m_Depth = game.GetDepth();
    m_Inside = game.GetInside();
    m_InVillage = game.IsInVillage();
    m_Dead = game.IsDead();
    m_Menu = game.GetMenu();
    m_Cursor = game.GetMenuCursor();
    m_Typed = 0;
    m_Result = game.GetResultCount();
    m_Revealed = true;
}

void SoundCues::Update(const CrawlerGame& game, const uint8_t* heard, int typed, int /*resultMs*/, int /*resultWaitMs*/,
                       const Cue* cues, int cueCount, int ms, Chip::Player& player)
{
    // What the game asked for by name.
    constexpr Sfx kCueSounds[] = { Sfx::Drink, Sfx::Eat,   Sfx::Chime, Sfx::Dig,   Sfx::Lock,    Sfx::Scrape, Sfx::Buy,
                                   Sfx::Error, Sfx::Heal,  Sfx::Warp,  Sfx::Bell,  Sfx::Roar,    Sfx::Home,   Sfx::Bump,
                                   Sfx::Trap,  Sfx::Swing, Sfx::Clang, Sfx::Crack, Sfx::Crumble, Sfx::Stumble };
    constexpr int kCueCount = static_cast<int>(sizeof(kCueSounds) / sizeof(kCueSounds[0]));
    for (int i = 0; i < cueCount; ++i)
    {
        const int cue = static_cast<int>(cues[i]);
        player.Play(kCueSounds[cue < kCueCount ? cue : 0]);
    }

    // A fight: the loudest thing that happened.
    if (heard)
    {
        if (heard[static_cast<int>(EventKind::Kill)] != 0)
        {
            player.Play(Sfx::Kill);
        }
        else if (heard[static_cast<int>(EventKind::Critical)] != 0)
        {
            player.Play(Sfx::Critical);
        }
        else if (heard[static_cast<int>(EventKind::Hit)] != 0)
        {
            player.Play(Sfx::Hit);
        }
        else if (heard[static_cast<int>(EventKind::Miss)] != 0)
        {
            player.Play(Sfx::Miss);
        }
        if (heard[static_cast<int>(EventKind::Break)] != 0)
        {
            player.Play(Sfx::Smash);
        }
    }

    // Somewhere else: fallen, down a floor, through a door.
    const bool dead = game.IsDead();
    const bool moved = game.GetDepth() != m_Depth || game.IsInVillage() != m_InVillage;
    if (dead && !m_Dead)
    {
        player.Play(Sfx::Fall);
    }
    else if (moved)
    {
        // Down the stairs, and a few low notes for the new floor.
        player.Play(Sfx::Stairs);
        if (!game.IsInVillage())
        {
            player.Play(Sfx::NewFloor);
        }
        m_AmbientMs = 5000;
    }
    else if (game.GetInside() != m_Inside)
    {
        player.Play(Sfx::Door);
    }
    else if (game.GetHeroCell() != m_Hero)
    {
        player.Play(Sfx::Step);
    }

    // The hero: hurt, stronger, a spell cast.
    if (game.GetLevel() > m_Level && !moved)
    {
        player.Play(Sfx::LevelUp);
    }
    else if (game.GetHp() < m_Hp && !dead && !moved)
    {
        player.Play(Sfx::Hurt);
    }
    if (game.GetMp() <= m_Mp - 2 && !moved)
    {
        player.Play(Sfx::Spell);
    }

    // Menus: opened, a line up or down, shut. An echo stone has its own.
    const Menu menu = game.GetMenu();
    const bool card = menu == Menu::Result || menu == Menu::Talk;
    if (menu != m_Menu)
    {
        if (menu == Menu::Echo)
        {
            player.Play(Sfx::Echo);
        }
        else if (menu == Menu::None && !card)
        {
            player.Play(Sfx::MenuClose);
        }
        else if (!card)
        {
            player.Play(Sfx::MenuSelect);
        }
    }
    else if (menu != Menu::None && game.GetMenuCursor() != m_Cursor)
    {
        player.Play(Sfx::MenuMove);
    }

    // What a deed did: a thing found rings out as it is tossed up.
    if (game.GetResultCount() != m_Result)
    {
        m_Result = game.GetResultCount();
        if (game.GetResultItem() >= 0)
        {
            player.Play(Sfx::Reveal);
        }
    }
    else if (!card)
    {
        // Picked up in passing: gold, or something for the bag.
        if (game.GetGold() > m_Gold && !moved)
        {
            player.Play(Sfx::Coin);
        }
        else if (CarriedIn(game) > m_Carried)
        {
            player.Play(Sfx::Pickup);
        }
    }

    // Someone speaking: a blip every few letters.
    if (menu == Menu::Talk && typed >= m_Typed + 3)
    {
        player.Play(Sfx::Talk);
        m_Typed = typed;
    }
    else if (menu != Menu::Talk || typed < m_Typed)
    {
        m_Typed = typed < 0 ? 0 : (menu == Menu::Talk ? typed : 0);
    }

    // The place itself, now and then: a drip in the halls and the reach, wind
    // through the bones, the foundry's embers, birds over the village.
    if (menu == Menu::None && !dead)
    {
        m_AmbientMs -= ms;
        if (m_AmbientMs <= 0)
        {
            constexpr Sfx kPlace[kZones] = { Sfx::Drip, Sfx::Wind, Sfx::Drip, Sfx::Crackle };
            m_Chance = m_Chance * 1103515245u + 12345u;
            m_AmbientMs = 5000 + static_cast<int>((m_Chance >> 16) % 9000u);
            if (game.GetInside() < 0)
            {
                player.Play(game.IsInVillage() ? Sfx::Birds : kPlace[game.GetZone() % kZones]);
            }
        }
        // Nearly gone: a heart, twice a breath.
        m_HeartMs -= ms;
        if (game.GetHp() * 4 <= game.GetMaxHp() && m_HeartMs <= 0)
        {
            player.Play(Sfx::Heartbeat);
            m_HeartMs = 1400;
        }
    }

    m_Hero = game.GetHeroCell();
    m_Hp = game.GetHp();
    m_Mp = game.GetMp();
    m_Gold = game.GetGold();
    m_Level = game.GetLevel();
    m_Carried = CarriedIn(game);
    m_Depth = game.GetDepth();
    m_Inside = game.GetInside();
    m_InVillage = game.IsInVillage();
    m_Dead = dead;
    m_Menu = menu;
    m_Cursor = game.GetMenuCursor();
}

}  // namespace Dungeon
