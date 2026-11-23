// The story's rules off the device: echo stones speak once, frogs hear words,
// heart shards wait on the third floor of each zone, cracked walls answer a
// third knock, the last run's ghost waits where it fell, and the save file
// keeps it all.

#include <gtest/gtest.h>

#include "../src/screen/MessageCard.h"
#include "../src/screen/Paper.h"
#include "../src/screen/Transitions.h"
#include "../src/world/CrawlerGame.h"
#include "Walk.h"

#include <cstdlib>
#include <cstring>
#include <vector>

using namespace Dungeon;

using namespace Dungeon::TestWalk;

TEST(Lore, TheVeryFirstFloorHasAnEchoStoneThatSpeaksOnce)
{
    int heard = 0;
    for (uint32_t seed = 1; seed < 40 && heard < 3; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed);
        Cell stone;
        ASSERT_TRUE(Find(game, Tile::EchoWall, stone)) << "seed " << seed;
        Cell stand;
        int way = 0;
        if (!StandBeside(game, stone, stand, way) || !WalkTo(game, stand))
        {
            continue;
        }
        Face(game, way);
        game.Act(Action::Forward);
        ASSERT_EQ(game.GetMenu(), Menu::Echo);
        EXPECT_EQ(game.GetEchoShown(), 0);
        EXPECT_EQ(game.GetLegend().echoes, 1u);
        EXPECT_EQ(game.GetMap().At(stone), Tile::Wall);
        game.Act(Action::Wait);  // any key closes it
        EXPECT_EQ(game.GetMenu(), Menu::None);
        ++heard;
    }
    EXPECT_GE(heard, 3);
}

TEST(Lore, EachZoneTellsItsOwnEchoesInOrder)
{
    CrawlerGame game;
    Legend legend;
    legend.echoes = 0x000F;  // the first zone's four heard
    game.SetLegend(legend);
    for (uint32_t seed = 1; seed < 30; ++seed)
    {
        game.NewRun(seed, 6);  // the bone catacombs
        Cell stone;
        if (!Find(game, Tile::EchoWall, stone))
        {
            continue;
        }
        Cell stand;
        int way = 0;
        if (!StandBeside(game, stone, stand, way) || !WalkTo(game, stand))
        {
            continue;
        }
        Face(game, way);
        game.Act(Action::Forward);
        ASSERT_EQ(game.GetMenu(), Menu::Echo);
        EXPECT_EQ(game.GetEchoShown(), 4);
        return;
    }
    FAIL() << "no stone reached in the catacombs";
}

TEST(Lore, AHeartShardWaitsOnTheThirdFloorOfEachZone)
{
    for (int depth : { 3, 8, 13, 18 })
    {
        for (uint32_t seed = 1; seed < 6; ++seed)
        {
            CrawlerGame game;
            game.NewRun(seed, depth);
            int shards = 0;
            for (int i = 0; i < CrawlerGame::kMaxItems; ++i)
            {
                const Item& item = game.GetItem(i);
                shards += !item.taken && item.kind == ItemKind::Shard ? 1 : 0;
            }
            EXPECT_EQ(shards, 1) << "depth " << depth << " seed " << seed;
        }
    }
    CrawlerGame game;
    game.NewRun(3, 4);
    for (int i = 0; i < CrawlerGame::kMaxItems; ++i)
    {
        EXPECT_FALSE(!game.GetItem(i).taken && game.GetItem(i).kind == ItemKind::Shard);
    }
}

TEST(Lore, AStoneListenerOffersChoicesAndNothingIsTyped)
{
    for (uint32_t seed = 1; seed < 400; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 4);
        int shrine = -1;
        for (int i = 0; i < CrawlerGame::kMaxFeatures; ++i)
        {
            if (game.GetFeature(i).present && game.GetFeature(i).kind == FeatureKind::Shrine)
            {
                shrine = i;
            }
        }
        if (shrine < 0)
        {
            continue;
        }
        const Cell at = game.GetFeature(shrine).at;
        Cell stand;
        int way = 0;
        if (!StandBeside(game, at, stand, way) || !WalkTo(game, stand))
        {
            continue;
        }
        Face(game, way);
        ASSERT_STREQ(game.GetTarget().name, "Stone listener");
        game.Act(Action::Forward);
        ASSERT_EQ(game.GetMenu(), Menu::Thing);
        EXPECT_STREQ(game.GetThingName(), "Stone listener");
        // A prayer and a name; the name on the heart only with a piece of
        // it in hand.
        ASSERT_EQ(game.GetMenuCount(), 2);
        EXPECT_EQ(game.GetMenuEntry(0).deed, Deed::PrayStone);
        EXPECT_EQ(game.GetMenuEntry(1).deed, Deed::GiveName);
        game.Act(Action::MenuClose);
        // The first name it is given: a snack. Shown on a card.
        const int snacks = game.Carried(ItemKind::Snack);
        game.Act(Action::Interact);
        game.Act(Action::MenuDown);
        game.Act(Action::MenuSelect);
        // What it gave is shown tossed up in the view: no card to shut.
        EXPECT_EQ(game.GetMenu(), Menu::None);
        EXPECT_EQ(game.GetResultItem(), static_cast<int>(ItemKind::Snack));
        EXPECT_EQ(game.Carried(ItemKind::Snack), snacks + 1);
        // Two more names it has not heard, each once; then none.
        const int mp = game.GetMaxMp();
        ASSERT_TRUE(Choose(game, Deed::GiveName));
        ASSERT_TRUE(Choose(game, Deed::GiveName));
        EXPECT_EQ(game.GetMaxMp(), mp + 2);
        EXPECT_FALSE(Choose(game, Deed::GiveName));
        // The prayer, once. Then it has nothing more, and says so.
        ASSERT_TRUE(Choose(game, Deed::PrayStone));
        EXPECT_FALSE(game.IsInSeam());
        if (!game.IsDead() && game.MonsterAt(at) < 0)
        {
            EXPECT_EQ(game.GetTarget().verb, nullptr);
        }
        return;
    }
    FAIL() << "no shrine reached";
}

TEST(Lore, TheLastRunsGhostWaitsWhereItFell)
{
    CrawlerGame game;
    Legend legend;
    legend.ghostDepth = 2;
    legend.ghostGold = 80;
    game.SetLegend(legend);
    game.NewRun(5, 2);
    int ghosts = 0;
    for (int i = 0; i < game.GetMonsterCount(); ++i)
    {
        ghosts += game.GetMonster(i).alive && game.GetMonster(i).kind == MonsterKind::Ghost ? 1 : 0;
    }
    EXPECT_EQ(ghosts, 1);
}

TEST(Lore, DyingIsRemembered)
{
    for (uint32_t seed = 1; seed < 20; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 14);
        Random random;
        random.Seed(seed);
        constexpr Action kActions[] = { Action::Forward, Action::TurnLeft, Action::TurnRight, Action::Wait };
        for (int i = 0; i < 20000 && !game.IsDead(); ++i)
        {
            if (game.GetMenu() != Menu::None)
            {
                game.Act(Action::MenuClose);
            }
            game.Act(kActions[random.Range(0, 3)]);
            Settle(game);
        }
        if (!game.IsDead())
        {
            continue;
        }
        EXPECT_EQ(game.GetLegend().deaths, 1);
        EXPECT_EQ(game.GetLegend().ghostDepth, game.GetDepth());
        EXPECT_EQ(game.GetLegend().ghostGold + game.GetLegend().bank, game.GetGold());
        game.Act(Action::Restart);
        EXPECT_FALSE(game.IsDead());
        EXPECT_EQ(game.GetLegend().deaths, 1);
        return;
    }
    FAIL() << "never died";
}

TEST(Lore, TheSaveKeepsTheLegendAndReadsTheOldOne)
{
    Legend legend;
    legend.deaths = 7;
    legend.echoes = 0xA5A5;
    legend.ghostDepth = 12;
    legend.ghostGold = 345;
    legend.ducks = 5;
    legend.endings = 3;
    legend.lastEnding = 2;
    legend.marenSaved = 2;
    legend.staff = true;
    legend.disco = true;
    char text[96];
    Lore::WriteSave(text, sizeof(text), 19, 11, legend);
    int deepest = 0;
    int pages = 0;
    Legend read;
    Lore::ReadSave(text, deepest, pages, read);
    EXPECT_EQ(deepest, 19);
    EXPECT_EQ(pages, 11);
    EXPECT_EQ(read.deaths, 7);
    EXPECT_EQ(read.echoes, 0xA5A5);
    EXPECT_EQ(read.ghostDepth, 12);
    EXPECT_EQ(read.ghostGold, 345);
    EXPECT_EQ(read.ducks, 5);
    EXPECT_EQ(read.endings, 3);
    EXPECT_EQ(read.lastEnding, 2);
    EXPECT_EQ(read.marenSaved, 2);
    EXPECT_TRUE(read.staff);
    EXPECT_TRUE(read.disco);

    Legend old;
    Lore::ReadSave("9 4", deepest, pages, old);
    EXPECT_EQ(deepest, 9);
    EXPECT_EQ(pages, 4);
    EXPECT_EQ(old.deaths, 0);
    EXPECT_EQ(old.echoes, 0);
}

TEST(Lore, EveryEchoFitsItsWindow)
{
    for (int i = 0; i < Lore::kEchoes + 2; ++i)
    {
        const char* text = Lore::GetEcho(i).text;
        int lines = 1;
        int length = 0;
        for (const char* p = text; *p != 0; ++p)
        {
            if (*p == '\n')
            {
                ++lines;
                length = 0;
                continue;
            }
            ++length;
            EXPECT_LE(length, 34) << "echo " << i;
        }
        EXPECT_EQ(lines, 4) << "echo " << i;
    }
    for (int i = 0; i < Lore::kShards; ++i)
    {
        EXPECT_EQ(Lore::ShardLetter(i), "STEN"[i]);
    }
}

TEST(Lore, AnEchoIsHeardALineAtATime)
{
    // How many pixels of a bright 320x240 view are lit, `ms` after the stone spoke.
    const auto lit = [](int ms)
    {
        std::vector<uint16_t> view(320 * 240, 0xFFFF);
        Transitions::DrawEcho(view.data(), 320, 0, 240, 0, 320, 320, 240, Lore::GetEcho(0), false, ms);
        int count = 0;
        for (const uint16_t pixel : view)
        {
            count += (pixel >> 11) > 8 ? 1 : 0;  // red, which every voice has: the dark is not quite zero
        }
        return count;
    };
    // Dark first, then each line adds to what is lit, until all are up.
    EXPECT_EQ(lit(Transitions::kEchoDarkMs), 0);
    int before = 0;
    for (int line = 0; line < 4; ++line)
    {
        const int now = lit(Transitions::kEchoDarkMs + line * Transitions::kEchoLineMs + 300);
        EXPECT_GT(now, before) << "line " << line;
        before = now;
    }
    EXPECT_EQ(lit(Transitions::EchoShownMs(false)), before);
    // An ending has one line more.
    EXPECT_GT(Transitions::EchoShownMs(true), Transitions::EchoShownMs(false));
}

TEST(Lore, SomeoneSpeakingGetsANameTagAndTypedWords)
{
    // Who speaks: the bog trader, the dungeon's voices, the village's people. Not
    // a thing that something was found in.
    EXPECT_EQ(MessageCard::SpeakerLength("Puddle: Pretty things. 12 gold for the lot."), 6);
    EXPECT_EQ(MessageCard::SpeakerLength("Keeper (you): Come in. I kept your place."), 12);
    EXPECT_EQ(MessageCard::SpeakerLength("Hilde: Eat something. You look like a ghost already."), 5);
    EXPECT_EQ(MessageCard::SpeakerLength("Torvald: Every swing counts. Even the misses."), 7);
    EXPECT_EQ(MessageCard::SpeakerLength("Old pack: +1 Potion."), 0);
    EXPECT_EQ(MessageCard::SpeakerLength("The basin is dry."), 0);
    // Speech comes a letter at a time and stays longer for it; anything
    // else is all there at once.
    const char* said = "Puddle: Go well.";
    EXPECT_EQ(MessageCard::Typed(said, 0), 0);
    EXPECT_EQ(MessageCard::Typed(said, 3 * MessageCard::kTypeMs), 3);
    // And never more letters than it has: their sound stops with them.
    EXPECT_EQ(MessageCard::Typed(said, 60000), MessageCard::Letters(said));
    EXPECT_GT(MessageCard::ShownMs(said), MessageCard::kShownMs);
    EXPECT_GE(MessageCard::Typed("The basin is dry.", 0), 64);
    EXPECT_EQ(MessageCard::ShownMs("The basin is dry."), MessageCard::kShownMs);
}

TEST(Lore, ADiaryPageIsReadFromASheetOfPaper)
{
    // Over a bright view: the view goes dark, the sheet in the middle is
    // pale, and its words are ink on it.
    std::vector<uint16_t> view(320 * 240, 0xFFFF);
    Paper::Draw(view.data(), 320, 0, 240, 0, 320, 320, 240, nullptr, CrawlerGame::PageText(1), "- 1 -", true);
    const auto green = [&](int x, int y) { return (view[static_cast<size_t>(y * 320 + x)] >> 5) & 63; };
    EXPECT_LT(green(4, 4), 32) << "the view behind is dimmed";
    EXPECT_LT(green(316, 236), 32);
    int paper = 0;
    int ink = 0;
    for (int y = 90; y < 150; ++y)
    {
        for (int x = 110; x < 210; ++x)
        {
            paper += green(x, y) > 40 ? 1 : 0;
            ink += green(x, y) < 20 ? 1 : 0;
        }
    }
    EXPECT_GT(paper, 3000);
    EXPECT_GT(ink, 100);
    // Drawn in strips, as the screen asks for it, it comes out the same.
    std::vector<uint16_t> strips(320 * 240, 0xFFFF);
    for (int row = 0; row < 240; row += 20)
    {
        Paper::Draw(strips.data() + row * 320, 320, row, 20, 0, 320, 320, 240, nullptr, CrawlerGame::PageText(1),
                    "- 1 -", true);
    }
    EXPECT_EQ(view, strips);
}

TEST(Lore, TheWholeHeartAndItsNameOpenTheSeamAndTheStoryEnds)
{
    bool ended = false;
    for (uint32_t seed = 1; seed < 400 && !ended; ++seed)
    {
        CrawlerGame game;
        Legend legend;
        for (uint8_t& building : legend.buildings)
        {
            building = Village::kMaxLevel;  // as strong a start as the village gives
        }
        game.SetLegend(legend);
        game.NewRun(seed, 4);
        int shrine = -1;
        for (int i = 0; i < CrawlerGame::kMaxFeatures; ++i)
        {
            if (game.GetFeature(i).present && game.GetFeature(i).kind == FeatureKind::Shrine)
            {
                shrine = i;
            }
        }
        Cell stand;
        int way = 0;
        if (shrine < 0 || !StandBeside(game, game.GetFeature(shrine).at, stand, way) || !WalkTo(game, stand))
        {
            continue;
        }
        Face(game, way);
        const char* ahead = game.GetTarget().name;
        if (game.IsDead() || game.GetMenu() != Menu::None || !ahead || std::strcmp(ahead, "Stone listener") != 0)
        {
            continue;  // fell on the way, or something stands between
        }
        // One piece: the line is there, but the heart is not whole.
        game.SetShards(1);
        game.Act(Action::Interact);
        ASSERT_EQ(game.GetMenu(), Menu::Thing) << "seed " << seed;
        ASSERT_EQ(game.GetMenuCount(), 3);
        EXPECT_EQ(game.GetMenuEntry(2).deed, Deed::SayHeartName);
        EXPECT_EQ(game.GetMenuEntry(2).count, 0);
        game.Act(Action::MenuClose);
        // All four: the name opens the Seam, where the Keeper waits.
        game.SetShards(Lore::kShards);
        ASSERT_TRUE(Choose(game, Deed::SayHeartName));
        ASSERT_TRUE(game.IsInSeam());
        EXPECT_EQ(game.GetShards(), 0);
        int keeper = -1;
        for (int i = 0; i < game.GetMonsterCount(); ++i)
        {
            keeper = game.GetMonster(i).alive && game.GetMonster(i).kind == MonsterKind::Keeper ? i : keeper;
        }
        ASSERT_GE(keeper, 0);
        // Brought down with bombs (the test hero is no match for it with a
        // blade; a bomb's kill counts like any other): a choice, then the
        // story is told and the run is won.
        const auto use = [&](ItemKind kind)
        {
            game.SetCarried(kind, 5);
            game.Act(Action::OpenBag);
            for (int i = 0; i < game.GetMenuCount(); ++i)
            {
                if (game.GetMenuEntry(i).item == kind)
                {
                    game.Act(Action::MenuSelect);
                    return;
                }
                game.Act(Action::MenuDown);
            }
            game.Act(Action::MenuClose);
        };
        for (int turns = 0; turns < 300 && game.GetMonster(keeper).alive && !game.IsDead(); ++turns)
        {
            Dismiss(game);
            const Cell at = game.GetMonster(keeper).at;
            const Cell hero = game.GetHeroCell();
            const bool near = std::abs(at.x - hero.x) <= 2 && std::abs(at.y - hero.y) <= 2;
            if (game.GetHp() * 2 < game.GetMaxHp())
            {
                use(ItemKind::Elixir);
            }
            else if (near)
            {
                use(ItemKind::Bomb);
            }
            else
            {
                // It keeps its place: up to it.
                Cell beside;
                int toward = 0;
                if (!StandBeside(game, at, beside, toward) || !WalkTo(game, beside))
                {
                    break;
                }
            }
            Settle(game);
        }
        if (game.IsDead() || game.GetMenu() != Menu::Ending)
        {
            continue;
        }
        game.Act(Action::MenuSelect);
        EXPECT_TRUE(game.IsRunWon());
        EXPECT_EQ(game.GetLegend().lastEnding, 1);
        ended = true;
    }
    EXPECT_TRUE(ended) << "the Keeper was never beaten";
}
