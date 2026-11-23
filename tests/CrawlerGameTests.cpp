// The crawler's rules off the device: walls stop the hero, glides finish,
// turning is free, fighting kills, and the stairs lead deeper.

#include <gtest/gtest.h>

#include "../src/world/CrawlerGame.h"

#include <string>

using namespace Dungeon;

namespace
{

void Settle(CrawlerGame& game)
{
    for (int i = 0; i < 100 && game.IsGliding(); ++i)
    {
        game.Tick(16);
    }
}

}  // namespace

TEST(CrawlerGame, ANewRunStartsOnDepthOneFacingAWayOut)
{
    CrawlerGame game;
    game.NewRun(42);
    EXPECT_EQ(game.GetDepth(), 1);
    EXPECT_EQ(game.GetHp(), game.GetMaxHp());
    const Cell hero = game.GetHeroCell();
    constexpr int kDx[4] = { 1, 0, -1, 0 };
    constexpr int kDy[4] = { 0, 1, 0, -1 };
    const int f = game.GetFacing();
    EXPECT_TRUE(game.GetMap().IsWalkable({ hero.x + kDx[f], hero.y + kDy[f] }));
}

TEST(CrawlerGame, TurningGlidesAndEndsFacingTheNewWay)
{
    CrawlerGame game;
    game.NewRun(7);
    const int facing = game.GetFacing();
    game.Act(Action::TurnRight);
    EXPECT_TRUE(game.IsGliding());
    game.Act(Action::TurnRight);  // ignored mid-glide
    Settle(game);
    EXPECT_EQ(game.GetFacing(), (facing + 1) & 3);
    Fix x = 0;
    Fix y = 0;
    int angle = 0;
    game.GetView(x, y, angle);
    EXPECT_EQ(angle, game.GetFacing() * kAngleSteps / 4);
}

TEST(CrawlerGame, WalkingIntoAWallDoesNotMove)
{
    CrawlerGame game;
    game.NewRun(9);
    // Turn until a wall is ahead, then walk into it.
    constexpr int kDx[4] = { 1, 0, -1, 0 };
    constexpr int kDy[4] = { 0, 1, 0, -1 };
    for (int i = 0; i < 4; ++i)
    {
        const Cell h = game.GetHeroCell();
        const int f = game.GetFacing();
        if (!game.GetMap().IsWalkable({ h.x + kDx[f], h.y + kDy[f] }))
        {
            break;
        }
        game.Act(Action::TurnLeft);
        Settle(game);
    }
    const Cell before = game.GetHeroCell();
    game.Act(Action::Forward);
    Settle(game);
    const Cell h = game.GetHeroCell();
    const int f = game.GetFacing();
    if (!game.GetMap().IsWalkable({ before.x + kDx[f], before.y + kDy[f] }))
    {
        EXPECT_EQ(h, before);
    }
}

TEST(CrawlerGame, PlayingRandomlyNeverBreaksTheRules)
{
    // A thousand random moves over several runs: the hero only ever stands on
    // walkable tiles, HP stays in range, and dying then Enter starts again.
    CrawlerGame game;
    game.NewRun(1234);
    Random random(99);
    const Action actions[] = { Action::Forward,    Action::Forward,     Action::Forward,
                               Action::TurnLeft,   Action::TurnRight,   Action::Back,
                               Action::StrafeLeft, Action::StrafeRight, Action::Wait };
    int deepest = 1;
    for (int i = 0; i < 3000; ++i)
    {
        if (game.IsDead())
        {
            game.Act(Action::Restart);
            ASSERT_FALSE(game.IsDead());
            ASSERT_EQ(game.GetDepth(), 1);
        }
        game.Act(actions[random.Range(0, 8)]);
        Settle(game);
        ASSERT_TRUE(game.GetMap().IsWalkable(game.GetHeroCell()));
        ASSERT_GE(game.GetHp(), 0);
        ASSERT_LE(game.GetHp(), game.GetMaxHp());
        deepest = game.GetDepth() > deepest ? game.GetDepth() : deepest;
    }
    EXPECT_GE(game.GetDeepest(), deepest);
}

TEST(CrawlerGame, TheBagOpensListsAndCloses)
{
    CrawlerGame game;
    game.NewRun(3);
    EXPECT_EQ(game.Carried(ItemKind::Potion), 1);
    EXPECT_EQ(game.Carried(ItemKind::Bomb), 1);
    game.Act(Action::OpenBag);
    ASSERT_EQ(game.GetMenu(), Menu::Bag);
    ASSERT_EQ(game.GetMenuCount(), 2);
    EXPECT_EQ(game.GetMenuEntry(0).item, ItemKind::Potion);
    EXPECT_EQ(game.GetMenuEntry(1).item, ItemKind::Bomb);
    // Up from the top wraps to Close, the line after the last.
    game.Act(Action::MenuUp);
    EXPECT_EQ(game.GetMenuCursor(), 2);
    game.Act(Action::MenuDown);
    EXPECT_EQ(game.GetMenuCursor(), 0);
    // Walking does nothing while the bag is open.
    const Cell hero = game.GetHeroCell();
    game.Act(Action::Forward);
    EXPECT_EQ(game.GetHeroCell(), hero);
    // A potion at full HP stays in the bag.
    game.Act(Action::MenuSelect);
    EXPECT_EQ(game.GetMenu(), Menu::None);
    EXPECT_EQ(game.Carried(ItemKind::Potion), 1);
}

TEST(CrawlerGame, OpeningAndClosingTheBagSaysNothing)
{
    // The HUD shows the message again whenever this count moves, so
    // changes that say nothing must leave it alone.
    CrawlerGame game;
    game.NewRun(3);
    const uint32_t said = game.GetMessageCount();
    const uint32_t version = game.GetVersion();
    game.Act(Action::OpenBag);
    game.Act(Action::MenuDown);
    game.Act(Action::OpenBag);
    EXPECT_GT(game.GetVersion(), version);
    EXPECT_EQ(game.GetMessageCount(), said);
}

TEST(CrawlerGame, ABombIsUsedUpAndTakesATurn)
{
    CrawlerGame game;
    game.NewRun(11);
    game.Act(Action::OpenBag);
    game.Act(Action::MenuDown);
    game.Act(Action::MenuSelect);
    EXPECT_EQ(game.Carried(ItemKind::Bomb), 0);
    EXPECT_GT(game.BlastMs(), 0);
    EXPECT_EQ(std::string(game.GetMessage()).substr(0, 5), "BOOM!");
}

// Nothing the floor holds cuts the way down off: features stand where they
// block no path, and every loose thing outside a hideout can be reached.
TEST(CrawlerGame, FeaturesNeverBlockTheWayAndThingsCanBeReached)
{
    int merchants = 0;
    int fountains = 0;
    for (uint32_t seed = 1; seed <= 60; ++seed)
    {
        for (int depth = 2; depth <= 16; depth += 2)
        {
            CrawlerGame game;
            game.NewRun(seed, depth);
            DungeonMap map = game.GetMap();
            for (int i = 0; i < CrawlerGame::kMaxFeatures; ++i)
            {
                const Feature& f = game.GetFeature(i);
                if (f.present)
                {
                    (f.kind == FeatureKind::Merchant ? merchants : fountains) += 1;
                    map.Set(f.at.x, f.at.y, Tile::Wall);
                }
            }
            map.ComputeDistances(map.GetStart());
            ASSERT_NE(map.DistanceAt(map.GetStairs()), DungeonMap::kUnreached) << seed << "/" << depth;
            for (int i = 0; i < CrawlerGame::kMaxItems; ++i)
            {
                const Item& item = game.GetItem(i);
                const bool hidden = (map.GetSecretRoom().present && map.GetSecretRoom().room.Contains(item.at)) ||
                                    (map.GetVault().present && map.GetVault().room.Contains(item.at)) ||
                                    (map.GetPocket().present && map.GetPocket().room.Contains(item.at));
                if (!item.taken && !hidden)
                {
                    EXPECT_NE(map.DistanceAt(item.at), DungeonMap::kUnreached) << seed << "/" << depth;
                }
            }
        }
    }
    EXPECT_GT(merchants, 100);
    EXPECT_GT(fountains, 60);
}

TEST(CrawlerGame, CuteOnesNearTheTopFierceOnesDeepAndBossesEveryFifthFloor)
{
    const auto kinds = [](int depth, uint32_t seed)
    {
        CrawlerGame game;
        game.NewRun(seed, depth);
        uint32_t seen = 0;
        for (int i = 0; i < game.GetMonsterCount(); ++i)
        {
            if (game.GetMonster(i).alive)
            {
                seen |= 1u << static_cast<int>(game.GetMonster(i).kind);
            }
        }
        return seen;
    };
    const uint32_t fierce = (1u << static_cast<int>(MonsterKind::Teasnail)) - 1u;  // the six cute ones
    for (uint32_t seed = 1; seed <= 20; ++seed)
    {
        EXPECT_EQ(kinds(1, seed) & ~fierce, 0u) << "only cute ones on depth 1";
        EXPECT_EQ(kinds(16, seed) & fierce, 0u) << "no cute ones on depth 16";
        EXPECT_TRUE(kinds(5, seed) & (1u << static_cast<int>(MonsterKind::Mimic)));
        EXPECT_TRUE(kinds(10, seed) & (1u << static_cast<int>(MonsterKind::Grubking)));
        EXPECT_TRUE(kinds(15, seed) & (1u << static_cast<int>(MonsterKind::Hydragon)));
    }
}

// Walking into the door from its front puts the hero in the hall, turned as
// the door turns; walking back out returns to the front of the frame.
TEST(CrawlerGame, NoFloorHasADoorToNowhere)
{
    // The folded halls are gone: a floor is only what it looks like.
    for (uint32_t seed = 1; seed <= 60; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 1 + static_cast<int>(seed % 18));
        EXPECT_FALSE(game.GetMap().GetPocket().present) << "seed " << seed;
        for (int i = 0; i < CrawlerGame::kMaxFeatures; ++i)
        {
            const Feature& f = game.GetFeature(i);
            EXPECT_FALSE(f.present && f.kind == FeatureKind::PortalHome) << "seed " << seed;
        }
    }
}
