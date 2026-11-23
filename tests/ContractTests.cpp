// The contracts of Last Hearth off the device: someone asks in a few lines,
// the contract is taken and listed on the notice board, the thing turns up
// below, word of it is paid, and what was paid betters every run after.

#include <gtest/gtest.h>

#include "../src/world/Contracts.h"
#include "../src/world/CrawlerGame.h"
#include "../src/world/Village.h"
#include "Walk.h"

#include <cstring>
#include <string>

using namespace Dungeon;
using namespace Dungeon::TestWalk;
using Contracts::Id;
using Contracts::State;

namespace
{

/// Lets everything being said go by, a line a key.
void HearOut(CrawlerGame& game)
{
    for (int i = 0; i < 8 && game.GetMenu() == Menu::Talk; ++i)
    {
        game.Act(Action::MenuClose);
    }
}

/// Up to Ulf, and his Talk line taken.
void TalkToUlf(CrawlerGame& game)
{
    const Cell fence = Village::FenceCell();
    ASSERT_TRUE(WalkTo(game, { fence.x + 1, fence.y }, Village::WellCell()));
    Face(game, 2);
    game.Act(Action::Interact);
    ASSERT_EQ(game.GetMenu(), Menu::Fence);
    for (int i = 0; i + 1 < game.GetMenuCount(); ++i)
    {
        game.Act(Action::MenuDown);
    }
    game.Act(Action::MenuSelect);
}

Legend With(Id id, State state)
{
    Legend legend;
    Contracts::SetState(legend, id, state);
    return legend;
}

}  // namespace

TEST(Contracts, EachKeepsItsOwnState)
{
    Legend legend;
    Contracts::SetState(legend, Id::Doll, State::Found);
    Contracts::SetState(legend, Id::Hammer, State::Kept);
    EXPECT_EQ(Contracts::GetState(legend, Id::Doll), State::Found);
    EXPECT_EQ(Contracts::GetState(legend, Id::Hammer), State::Kept);
    EXPECT_EQ(Contracts::GetState(legend, Id::Brother), State::Unasked);
    Contracts::SetState(legend, Id::Doll, State::Unasked);
    EXPECT_EQ(Contracts::GetState(legend, Id::Hammer), State::Kept);
    // Every line someone speaks fits a line of talk.
    for (int c = 0; c < Contracts::kContracts; ++c)
    {
        const Contracts::Info& info = Contracts::GetInfo(static_cast<Id>(c));
        for (const char* line : info.offer)
        {
            EXPECT_LT(std::strlen(line), static_cast<size_t>(CrawlerGame::kMessageLength)) << line;
        }
        for (const char* line : info.back)
        {
            EXPECT_LT(std::strlen(line), static_cast<size_t>(CrawlerGame::kMessageLength)) << line;
        }
        EXPECT_LT(std::strlen(info.remind), static_cast<size_t>(CrawlerGame::kMessageLength));
        EXPECT_LT(std::strlen(info.gossip), static_cast<size_t>(CrawlerGame::kMessageLength));
        EXPECT_LT(std::strlen(info.name) + std::strlen(info.reward) + 17,
                  static_cast<size_t>(CrawlerGame::kMessageLength))
            << info.name;
    }
}

TEST(Contracts, UlfAsksInThreeLinesAndTheBoardListsIt)
{
    CrawlerGame game;
    game.GoHome(5);
    const Contracts::Info& info = Contracts::GetInfo(Id::Brother);
    TalkToUlf(game);
    // His three lines, one a key; then it is taken, on a card.
    for (const char* line : info.offer)
    {
        ASSERT_EQ(game.GetMenu(), Menu::Talk);
        EXPECT_STREQ(game.GetSpeech(), line);
        EXPECT_EQ(Contracts::GetState(game.GetLegend(), Id::Brother), State::Unasked);
        game.Act(Action::MenuClose);
    }
    // His three lines, one a key; then it is taken, and said so.
    ASSERT_EQ(game.GetMenu(), Menu::None);
    EXPECT_NE(std::strstr(game.GetResultText(), info.name), nullptr);
    EXPECT_EQ(Contracts::GetState(game.GetLegend(), Id::Brother), State::Taken);
    // The notice board: his contract, open; and Hilde wanting a word. The
    // keepers of ruins are not on it.
    const Cell board = Village::BoardCell();
    ASSERT_TRUE(WalkTo(game, { board.x, board.y + 1 }, Village::WellCell()));
    Face(game, 3);
    ASSERT_STREQ(game.GetTarget().name, "Notice board");
    game.Act(Action::Interact);
    ASSERT_EQ(game.GetMenu(), Menu::Contracts);
    ASSERT_EQ(game.GetMenuCount(), 2);
    EXPECT_STREQ(game.GetMenuEntry(0).label, info.name);
    EXPECT_EQ(game.GetMenuEntry(0).count, static_cast<int>(State::Taken));
    EXPECT_STREQ(game.GetContractHint(game.GetMenuEntry(0)), info.hints[0]);
    EXPECT_STREQ(game.GetMenuEntry(1).label, Contracts::GetInfo(Id::Doll).wants);
    game.Act(Action::MenuClose);
    // Asked again, he says what is still to do, or his own lines: never
    // the offer.
    TalkToUlf(game);
    ASSERT_EQ(game.GetMenu(), Menu::Talk);
    EXPECT_STRNE(game.GetSpeech(), info.offer[0]);
}

TEST(Contracts, OrmWalksTheThirdFloorAndHisKnifeIsPaidFor)
{
    for (uint32_t seed = 1; seed < 60; ++seed)
    {
        CrawlerGame game;
        Legend legend = With(Id::Brother, State::Taken);
        legend.buildings[0] = 3;  // a good blade, to see it through
        legend.buildings[2] = 3;
        legend.bank = 100;
        game.SetLegend(legend);
        game.NewRun(seed, Contracts::kOrmFloor);
        int orm = -1;
        for (int i = 0; i < game.GetMonsterCount(); ++i)
        {
            orm = game.GetMonster(i).alive && game.GetMonster(i).named ? i : orm;
        }
        ASSERT_GE(orm, 0) << "seed " << seed;
        EXPECT_GT(game.GetMonster(orm).maxHp, 20);
        for (int blows = 0; blows < 200 && game.GetMonster(orm).alive && !game.IsDead(); ++blows)
        {
            Cell stand;
            int way = 0;
            if (!StandBeside(game, game.GetMonster(orm).at, stand, way) || !WalkTo(game, stand))
            {
                break;
            }
            Face(game, way);
            if (game.MonsterAt(game.GetMonster(orm).at) == orm)
            {
                EXPECT_STREQ(game.GetTarget().name, "Orm the Thirsty");
            }
            game.Act(Action::Forward);
            Settle(game);
        }
        if (game.IsDead() || game.GetMonster(orm).alive)
        {
            continue;
        }
        // His knife, shown; the contract waits to be paid.
        EXPECT_EQ(Contracts::GetState(game.GetLegend(), Id::Brother), State::Found);
        EXPECT_STREQ(game.GetResultText(), Contracts::GetInfo(Id::Brother).found);
        EXPECT_EQ(game.GetResultItem(), static_cast<int>(ItemKind::Dagger));
        // Home: Ulf pays in cheaper goods.
        legend = game.GetLegend();
        game.SetLegend(legend);
        game.GoHome(5);
        TalkToUlf(game);
        ASSERT_EQ(game.GetMenu(), Menu::Talk);
        EXPECT_STREQ(game.GetSpeech(), Contracts::GetInfo(Id::Brother).back[0]);
        HearOut(game);
        EXPECT_EQ(Contracts::GetState(game.GetLegend(), Id::Brother), State::Kept);
        const Cell fence = Village::FenceCell();
        ASSERT_EQ(game.GetHeroCell().x, fence.x + 1);
        game.Act(Action::Interact);
        ASSERT_EQ(game.GetMenu(), Menu::Fence);
        EXPECT_EQ(game.GetMenuEntry(0).price, Village::MaterialPrice(0) * 6 / 10);
        return;
    }
    FAIL() << "Orm was never put to rest";
}

TEST(Contracts, TheDollTurnsUpInTheOssuary)
{
    for (uint32_t seed = 1; seed < 300; ++seed)
    {
        CrawlerGame game;
        game.SetLegend(With(Id::Doll, State::Taken));
        game.NewRun(seed, 6 + static_cast<int>(seed % 4));
        for (int i = 0; i < CrawlerGame::kMaxProps; ++i)
        {
            const Prop& prop = game.GetProp(i);
            const int kind = prop.present ? game.PropIndex(prop) : -1;
            Cell stand;
            int way = 0;
            if ((kind != kZoneProps && kind != kZoneProps + 2) || !game.HasDeeds(prop) ||
                !StandBeside(game, prop.at, stand, way) || !WalkTo(game, stand))
            {
                continue;
            }
            Face(game, way);
            game.Act(Action::Interact);
            if (game.GetMenu() != Menu::Thing)
            {
                continue;
            }
            game.Act(Action::MenuSelect);
            if (Contracts::GetState(game.GetLegend(), Id::Doll) == State::Found)
            {
                // Shown as soon as nothing else is in the way.
                if (game.GetMenu() == Menu::None)
                {
                    EXPECT_STREQ(game.GetResultText(), Contracts::GetInfo(Id::Doll).found);
                }
                return;
            }
            HearOut(game);
        }
    }
    FAIL() << "the doll was never found";
}

TEST(Contracts, TheBogTraderPaysGunnarAfterHisFashion)
{
    for (uint32_t seed = 1; seed < 200; ++seed)
    {
        CrawlerGame game;
        game.SetLegend(With(Id::Meals, State::Taken));
        game.NewRun(seed, 2);
        int trader = -1;
        for (int i = 0; i < CrawlerGame::kMaxFeatures; ++i)
        {
            trader = game.GetFeature(i).present && game.GetFeature(i).kind == FeatureKind::Merchant ? i : trader;
        }
        Cell stand;
        int way = 0;
        if (trader < 0 || !StandBeside(game, game.GetFeature(trader).at, stand, way) || !WalkTo(game, stand))
        {
            continue;
        }
        Face(game, way);
        game.Act(Action::Interact);
        if (game.GetMenu() != Menu::Shop)
        {
            continue;
        }
        for (int i = 0; i < game.GetMenuCount(); ++i)
        {
            const MenuEntry& entry = game.GetMenuEntry(i);
            if (entry.label && std::strcmp(entry.label, "Talk") == 0)
            {
                game.Act(Action::MenuSelect);
                ASSERT_EQ(game.GetMenu(), Menu::Talk);
                EXPECT_NE(std::strstr(game.GetSpeech(), "Gunnar"), nullptr);
                EXPECT_EQ(Contracts::GetState(game.GetLegend(), Id::Meals), State::Found);
                return;
            }
            game.Act(Action::MenuDown);
        }
    }
    FAIL() << "the trader was never reached";
}

TEST(Contracts, TheTeacherIsAnsweredOnceFiveStonesWereHeard)
{
    CrawlerGame game;
    Legend legend = With(Id::Question, State::Taken);
    legend.buildings[3] = 1;  // the tower
    legend.echoes = 0x1F;     // five stones heard
    legend.bank = 0;
    game.SetLegend(legend);
    game.GoHome(5);
    const Cell front = Village::BuildingCell(3);
    ASSERT_TRUE(WalkTo(game, Village::OutsideCell(3), Village::WellCell()));
    Face(game, front.y < Village::StartCell().y ? 3 : 1);
    game.Act(Action::Interact);
    ASSERT_EQ(game.GetInside(), 3);
    ASSERT_TRUE(WalkTo(game, { Village::KeeperCell().x, Village::KeeperCell().y + 1 }));
    Face(game, 3);
    ASSERT_STREQ(game.GetTarget().verb, "Talk");
    game.Act(Action::Interact);
    ASSERT_EQ(game.GetMenu(), Menu::Talk);
    EXPECT_STREQ(game.GetSpeech(), Contracts::GetInfo(Id::Question).back[0]);
    game.Act(Action::MenuClose);
    game.Act(Action::MenuClose);
    ASSERT_EQ(game.GetMenu(), Menu::None);
    EXPECT_NE(std::strstr(game.GetResultText(), "Contract kept"), nullptr);
    EXPECT_EQ(Contracts::GetState(game.GetLegend(), Id::Question), State::Kept);
}

TEST(Contracts, ThreeGlowcapsCarriedHomeAreIngrids)
{
    for (uint32_t seed = 1; seed < 60; ++seed)
    {
        CrawlerGame game;
        game.SetLegend(With(Id::Herb, State::Taken));
        game.NewRun(seed, 2);
        game.SetCarried(ItemKind::Glowcap, Contracts::kGlowcaps);
        if (!game.CanReturnHome())
        {
            continue;
        }
        game.Act(Action::OpenMenu);
        game.Act(Action::MenuDown);
        game.Act(Action::MenuDown);
        game.Act(Action::MenuSelect);
        ASSERT_TRUE(game.IsInVillage());
        EXPECT_EQ(Contracts::GetState(game.GetLegend(), Id::Herb), State::Found);
        EXPECT_STREQ(game.GetResultText(), Contracts::GetInfo(Id::Herb).found);
        return;
    }
    FAIL() << "never a quiet moment to go home";
}

TEST(Contracts, WhatWasPaidBettersEveryRunAndTheTownTalks)
{
    CrawlerGame plain;
    plain.NewRun(3);
    CrawlerGame game;
    Legend legend;
    for (const Id id : { Id::Doll, Id::Question, Id::Herb, Id::Hammer })
    {
        Contracts::SetState(legend, id, State::Kept);
    }
    game.SetLegend(legend);
    game.NewRun(3);
    EXPECT_EQ(game.GetMaxHp(), plain.GetMaxHp() + 5);
    EXPECT_EQ(game.GetMaxMp(), plain.GetMaxMp() + 3);
    EXPECT_EQ(game.Carried(ItemKind::Potion), plain.Carried(ItemKind::Potion) + 1);
    EXPECT_EQ(game.GetAttack(), plain.GetAttack() + 1);
    // Leif speaks of each once, before his own lines.
    game.GoHome(5);
    const Cell leif = Village::VillagerCell(1);
    Cell stand;
    int way = 0;
    ASSERT_TRUE(StandBeside(game, leif, stand, way));
    ASSERT_TRUE(WalkTo(game, stand, Village::WellCell()));
    Face(game, way);
    std::string heard;
    for (int i = 0; i < 5; ++i)
    {
        game.Act(Action::Interact);
        ASSERT_EQ(game.GetMenu(), Menu::Talk);
        heard += game.GetSpeech();
        game.Act(Action::MenuClose);
    }
    for (const Id id : { Id::Doll, Id::Question, Id::Herb, Id::Hammer })
    {
        EXPECT_NE(heard.find(Contracts::GetInfo(id).gossip), std::string::npos);
    }
    EXPECT_NE(heard.find(Village::VillagerLine(1, 4)), std::string::npos) << "then his own";
}
