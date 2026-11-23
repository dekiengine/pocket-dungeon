// What a key does, off the device: things are used and foes fought only by
// facing them, the weapon swings at nothing and breaks a cracked wall in five
// blows, a way down asks before it takes, a beer's feet go their own way, and
// what a deed did comes up as the deed was: unwrapped, or after the blow.

#include <gtest/gtest.h>

#include "../src/world/CrawlerGame.h"
#include "../src/world/Village.h"
#include "Walk.h"

#include <cstring>

using namespace Dungeon;
using namespace Dungeon::TestWalk;

namespace
{

/// A prop the hero can stand beside, faced: its place in the game's props,
/// or -1.
int FaceAnyProp(CrawlerGame& game, int kind)
{
    for (int i = 0; i < CrawlerGame::kMaxProps; ++i)
    {
        const Prop& prop = game.GetProp(i);
        Cell stand;
        int way = 0;
        if (prop.present && game.PropIndex(prop) == kind && game.HasDeeds(prop) &&
            StandBeside(game, prop.at, stand, way) && WalkTo(game, stand))
        {
            Face(game, way);
            return i;
        }
    }
    return -1;
}

constexpr int kClayPot = kZones * kZoneProps + 3;

}  // namespace

TEST(Actions, ThingsAreUsedOnlyByFacingThem)
{
    for (uint32_t seed = 1; seed < 60; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2);
        const int pot = FaceAnyProp(game, kClayPot);
        if (pot < 0)
        {
            continue;
        }
        // Turned round, a step back is into it: nothing opens, nothing breaks.
        game.Act(Action::TurnRight);
        Settle(game);
        game.Act(Action::TurnRight);
        Settle(game);
        const Cell stood = game.GetHeroCell();
        game.Act(Action::Back);
        Settle(game);
        EXPECT_EQ(game.GetMenu(), Menu::None);
        EXPECT_EQ(game.GetHeroCell(), stood);
        EXPECT_TRUE(game.GetProp(pot).present);
        // Facing it again, the same key that walks opens its choices.
        game.Act(Action::TurnRight);
        Settle(game);
        game.Act(Action::TurnRight);
        Settle(game);
        game.Act(Action::Forward);
        EXPECT_EQ(game.GetMenu(), Menu::Thing);
        return;
    }
    FAIL() << "no pot reached";
}

TEST(Actions, TheWeaponSwingsAtNothingAndATurnPasses)
{
    CrawlerGame game;
    game.GoHome(5);
    // Out in the square, nothing ahead to use.
    ASSERT_EQ(game.GetTarget().verb, nullptr);
    ASSERT_LT(game.SwingProgress(), 0);
    game.Act(Action::Interact);
    EXPECT_GE(game.SwingProgress(), 0) << "the weapon is swung";
    Cue cues[CrawlerGame::kMaxCues];
    const int count = game.TakeCues(cues, CrawlerGame::kMaxCues);
    bool swung = false;
    for (int i = 0; i < count; ++i)
    {
        swung = swung || cues[i] == Cue::Swing;
    }
    EXPECT_TRUE(swung);
    EXPECT_TRUE(game.IsGliding()) << "it took a turn";
}

TEST(Actions, ACrackedWallComesDownInFiveBlows)
{
    for (uint32_t seed = 1; seed < 200; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2);
        Cell crack;
        if (!Find(game, Tile::SecretWall, crack))
        {
            continue;
        }
        Cell stand;
        int way = 0;
        if (!StandBeside(game, crack, stand, way) || !game.GetMap().IsWalkable(stand) || !WalkTo(game, stand))
        {
            continue;
        }
        Face(game, way);
        if (game.IsDead() || game.GetHp() < 12)
        {
            continue;
        }
        // Walking into it only says it is hollow.
        game.Act(Action::Forward);
        EXPECT_EQ(game.GetMap().At(crack), Tile::SecretWall);
        EXPECT_EQ(game.GetWallBlows(), 0);
        EXPECT_STREQ(game.GetTarget().verb, "Strike");
        for (int blow = 1; blow < CrawlerGame::kWallBlows; ++blow)
        {
            game.TakeEvents(nullptr, 0);
            game.Act(Action::Interact);
            Settle(game);
            ASSERT_EQ(game.GetMap().At(crack), Tile::SecretWall) << "blow " << blow;
            EXPECT_EQ(game.GetWallBlows(), blow);
        }
        game.Act(Action::Interact);
        EXPECT_EQ(game.GetMap().At(crack), Tile::Floor);
        // Stone flew: the screen is told.
        bool chipped = false;
        for (int i = 0; i < game.GetEventCount(); ++i)
        {
            chipped = chipped || game.GetEvent(i).kind == EventKind::Chip;
        }
        EXPECT_TRUE(chipped);
        return;
    }
    FAIL() << "no cracked wall reached";
}

TEST(Actions, TheWayDownAsksFirst)
{
    for (uint32_t seed = 1; seed < 40; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2);
        if (!WalkTo(game, game.GetMap().GetStairs()) || game.IsDead())
        {
            continue;
        }
        // On the stairs: still on this floor, asked.
        ASSERT_EQ(game.GetDepth(), 2);
        ASSERT_EQ(game.GetMenu(), Menu::Descend);
        EXPECT_EQ(game.GetDescendDepth(), 3);
        // Not yet: nothing happens, and the use key asks again.
        game.Act(Action::MenuClose);
        Settle(game);
        EXPECT_EQ(game.GetMenu(), Menu::None);
        EXPECT_EQ(game.GetDepth(), 2);
        // (Something to use may be ahead; turn until nothing is.)
        for (int turn = 0; turn < 4 && game.GetTarget().verb; ++turn)
        {
            game.Act(Action::TurnRight);
            Settle(game);
        }
        if (game.GetTarget().verb)
        {
            continue;
        }
        game.Act(Action::Interact);
        ASSERT_EQ(game.GetMenu(), Menu::Descend);
        // Go down: the first line.
        game.Act(Action::MenuSelect);
        EXPECT_EQ(game.GetDepth(), 3);
        EXPECT_EQ(game.GetMenu(), Menu::None);
        return;
    }
    FAIL() << "the stairs were never reached";
}

TEST(Actions, ABeersFeetGoTheirOwnWay)
{
    CrawlerGame game;
    game.GoHome(5);
    // An ale barrel by the tavern: drink.
    const Cell barrel = Village::BarrelCell(0);
    Cell stand;
    int way = 0;
    ASSERT_TRUE(StandBeside(game, barrel, stand, way));
    ASSERT_TRUE(WalkTo(game, stand, Village::WellCell()));
    Face(game, way);
    ASSERT_TRUE(Choose(game, Deed::Drink));
    ASSERT_GT(game.GetTipsy(), 0);
    // Turning only: sober, forty quarter turns end where they began. Tipsy,
    // some of them become something else, and it is said.
    bool stumbled = false;
    for (int i = 0; i < 40 && game.GetTipsy() > 0; ++i)
    {
        game.Act(Action::TurnLeft);
        Settle(game);
        Dismiss(game);
        if (game.GetMenu() != Menu::None)
        {
            game.Act(Action::MenuClose);
        }
        stumbled = stumbled || std::strstr(game.GetMessage(), "feet") != nullptr;
    }
    EXPECT_TRUE(stumbled);
}

TEST(Actions, WhatADeedDidIsShownWithNothingToShut)
{
    bool smashed = false;
    for (uint32_t seed = 1; seed < 60 && !smashed; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2);
        if (FaceAnyProp(game, kClayPot) < 0)
        {
            continue;
        }
        game.Act(Action::Interact);
        ASSERT_EQ(game.GetMenu(), Menu::Thing);
        for (int i = 0; i < game.GetMenuCount(); ++i)
        {
            if (game.GetMenuEntry(i).deed == Deed::Smash)
            {
                const uint32_t shown = game.GetResultCount();
                game.Act(Action::MenuSelect);
                // Nothing to shut: the blow plays, and what fell out is shown.
                ASSERT_EQ(game.GetMenu(), Menu::None);
                ASSERT_EQ(game.GetResultCount(), shown + 1);
                EXPECT_EQ(game.GetReveal(), Reveal::Blow);
                EXPECT_GE(game.SwingProgress(), 0);
                smashed = true;
                break;
            }
            game.Act(Action::MenuDown);
        }
    }
    EXPECT_TRUE(smashed);
    constexpr int kOldChest = kZones * kZoneProps + 6;
    for (uint32_t seed = 1; seed < 200; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2);
        if (FaceAnyProp(game, kOldChest) < 0)
        {
            continue;
        }
        game.Act(Action::Interact);
        ASSERT_EQ(game.GetMenu(), Menu::Thing);
        ASSERT_EQ(game.GetMenuEntry(0).deed, Deed::OpenChest);
        const uint32_t shown = game.GetResultCount();
        game.Act(Action::MenuSelect);
        if (game.GetResultCount() == shown)
        {
            continue;  // it was not a chest
        }
        EXPECT_EQ(game.GetReveal(), Reveal::Opened);
        return;
    }
    FAIL() << "no chest opened";
}

TEST(Actions, WhatWasReadCanBeReadAgainFromTheMenu)
{
    for (uint32_t seed = 1; seed < 300; ++seed)
    {
        CrawlerGame game;
        game.SetPagesKnown(2);
        game.NewRun(seed, 2 + static_cast<int>(seed % 3));
        Cell note{ -1, -1 };
        for (int i = 0; i < CrawlerGame::kMaxItems; ++i)
        {
            if (!game.GetItem(i).taken && game.GetItem(i).kind == ItemKind::Note)
            {
                note = game.GetItem(i).at;
            }
        }
        if (note.x < 0 || !WalkTo(game, note) || game.IsDead())
        {
            continue;
        }
        // Read on the way (and closed): now it is in the menu's notes, over
        // the diary's two pages.
        game.Act(Action::OpenMenu);
        for (int i = 0; i < 6; ++i)
        {
            game.Act(Action::MenuDown);
        }
        game.Act(Action::MenuSelect);
        ASSERT_EQ(game.GetMenu(), Menu::Notes);
        ASSERT_EQ(game.GetMenuCount(), 3);
        EXPECT_EQ(game.GetMenuEntry(0).item, ItemKind::Note);
        game.Act(Action::MenuSelect);
        ASSERT_EQ(game.GetMenu(), Menu::Book);
        ASSERT_NE(game.GetReading(), nullptr);
        EXPECT_STREQ(game.GetReading()->title, game.GetMenuEntry(0).label);
        game.Act(Action::MenuClose);
        // A page of the diary, the same way.
        game.Act(Action::OpenMenu);
        for (int i = 0; i < 6; ++i)
        {
            game.Act(Action::MenuDown);
        }
        game.Act(Action::MenuSelect);
        game.Act(Action::MenuDown);
        game.Act(Action::MenuDown);
        game.Act(Action::MenuSelect);
        EXPECT_EQ(game.GetMenu(), Menu::Page);
        EXPECT_EQ(game.GetPageShown(), 2);
        return;
    }
    FAIL() << "no note was found";
}

TEST(Actions, NothingStandsInADoorwayOrACorridor)
{
    int things = 0;
    for (uint32_t seed = 1; seed < 120; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 1 + static_cast<int>(seed % 19));
        const DungeonMap& map = game.GetMap();
        const auto inRoom = [&](Cell c)
        {
            for (int i = 0; i < map.GetRoomCount(); ++i)
            {
                if (map.GetRoom(i).Contains(c))
                {
                    return true;
                }
            }
            return (map.GetSecretRoom().present && map.GetSecretRoom().room.Contains(c)) ||
                   (map.GetVault().present && map.GetVault().room.Contains(c));
        };
        const auto check = [&](Cell c, const char* what)
        {
            ++things;
            EXPECT_TRUE(inRoom(c)) << what << " in a corridor, seed " << seed;
            for (int f = 0; f < 4; ++f)
            {
                const Cell n{ c.x + kDx[f], c.y + kDy[f] };
                EXPECT_FALSE(map.IsWalkable(n) && !inRoom(n)) << what << " in a doorway, seed " << seed;
                EXPECT_NE(map.At(n), Tile::SecretWall) << what << " before a cracked wall, seed " << seed;
                EXPECT_NE(map.At(n), Tile::Door) << what << " before a locked door, seed " << seed;
            }
        };
        for (int i = 0; i < CrawlerGame::kMaxFeatures; ++i)
        {
            if (game.GetFeature(i).present)
            {
                check(game.GetFeature(i).at, "a feature");
            }
        }
        for (int i = 0; i < CrawlerGame::kMaxProps; ++i)
        {
            if (game.GetProp(i).present)
            {
                check(game.GetProp(i).at, "a thing");
            }
        }
    }
    EXPECT_GT(things, 300);
}
