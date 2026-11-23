// The village and the dungeon's new secrets off the device: runs start at
// home, the well goes down, buildings cost gold and materials and change how
// a run starts, the lift waits for a boss, and bookshelves, locked chests and
// loose bricks do what they say.

#include <gtest/gtest.h>

#include "../src/world/Contracts.h"
#include "../src/world/CrawlerGame.h"
#include "../src/world/Village.h"
#include "Walk.h"

#include <cstdlib>
#include <cstring>
#include <string>

using namespace Dungeon;
using namespace Dungeon::TestWalk;

namespace
{

/// Walks up to a building's front and faces it, never through the well.
bool FaceBuilding(CrawlerGame& game, Village::Building building)
{
    const Cell front = Village::BuildingCell(static_cast<int>(building));
    // The fronts are on the north wall (y 3) and the south wall (y 15).
    const bool north = front.y < Village::StartCell().y;
    const Cell stand{ front.x, north ? front.y + 1 : front.y - 1 };
    if (!WalkTo(game, stand, Village::WellCell()))
    {
        return false;
    }
    Face(game, north ? 3 : 1);
    return true;
}

int FindProp(const CrawlerGame& game, int kind)
{
    for (int i = 0; i < CrawlerGame::kMaxProps; ++i)
    {
        if (game.GetProp(i).present && !game.GetProp(i).any && game.GetProp(i).kind == kind)
        {
            return i;
        }
    }
    return -1;
}

}  // namespace

TEST(Village, RunsStartAtHomeAndTheWellGoesDown)
{
    CrawlerGame game;
    game.GoHome(5);
    EXPECT_TRUE(game.IsInVillage());
    EXPECT_EQ(game.GetMonsterCount() > 0 ? game.GetMonster(0).alive : false, false);
    ASSERT_TRUE(WalkTo(game, { Village::WellCell().x, Village::WellCell().y + 1 }));
    Face(game, 3);
    const Target target = game.GetTarget();
    ASSERT_STREQ(target.name, "The old well");
    game.Act(Action::Interact);
    Settle(game);
    // On the well: asked first. Not yet leaves the hero at home.
    ASSERT_EQ(game.GetMenu(), Menu::Descend);
    EXPECT_TRUE(game.IsInVillage());
    game.Act(Action::MenuDown);
    game.Act(Action::MenuSelect);
    EXPECT_EQ(game.GetMenu(), Menu::None);
    EXPECT_TRUE(game.IsInVillage());
    // Asked again with the use key, and taken.
    game.Act(Action::Interact);
    ASSERT_EQ(game.GetMenu(), Menu::Descend);
    game.Act(Action::MenuSelect);
    EXPECT_FALSE(game.IsInVillage());
    EXPECT_EQ(game.GetDepth(), 1);
}

TEST(Village, BuildingCostsGoldAndMaterialAndBettersTheNextRun)
{
    CrawlerGame game;
    Legend legend;
    legend.bank = 100;
    legend.materials[0] = 5;  // moss
    game.SetLegend(legend);
    game.GoHome(5);
    const int attack = game.GetAttack();
    ASSERT_TRUE(FaceBuilding(game, Village::Building::Blacksmith));
    const Target target = game.GetTarget();
    ASSERT_STREQ(target.verb, "Build");
    game.Act(Action::Interact);
    EXPECT_EQ(game.GetLegend().buildings[0], 1);
    EXPECT_EQ(game.GetLegend().bank, 50);
    EXPECT_EQ(game.GetLegend().materials[0], 2);
    // Built, it is a way in; its keeper builds the next level, but there is
    // not enough for it: nothing changes.
    ASSERT_STREQ(game.GetTarget().verb, "Enter");
    game.Act(Action::Interact);
    ASSERT_EQ(game.GetInside(), 0);
    ASSERT_TRUE(WalkTo(game, { Village::KeeperCell().x, Village::KeeperCell().y + 1 }));
    Face(game, 3);
    // What it costs is shown beside what there is; with too little, the
    // key only talks.
    EXPECT_STREQ(game.GetTarget().verb, "Talk");
    EXPECT_GT(game.GetTarget().needGold, game.GetTarget().haveGold);
    game.Act(Action::Interact);
    for (int i = 0; i < 6 && game.GetMenu() != Menu::None; ++i)
    {
        game.Act(Action::MenuClose);
    }
    EXPECT_EQ(game.GetLegend().buildings[0], 1);
    EXPECT_EQ(game.GetLegend().bank, 50);
    // The next run starts with the better blade.
    game.NewRun(9);
    EXPECT_EQ(game.GetAttack(), attack + 1);
}

TEST(Village, TheLiftWaitsForTheBossThenRidesDeeper)
{
    CrawlerGame game;
    Legend legend;
    legend.bank = 1000;
    legend.materials[1] = 10;  // bone
    game.SetLegend(legend);
    game.GoHome(5);
    ASSERT_TRUE(FaceBuilding(game, Village::Building::Lift));
    game.Act(Action::Interact);
    EXPECT_EQ(game.GetLegend().buildings[5], 0) << "the mimic is not beaten yet";

    legend.bosses = 1;
    game.SetLegend(legend);
    game.GoHome(5);
    ASSERT_TRUE(FaceBuilding(game, Village::Building::Lift));
    game.Act(Action::Interact);
    EXPECT_EQ(game.GetLegend().buildings[5], 1);
    // Its platform takes you down to depth 6.
    // Standing at its door: one step north onto the platform.
    Face(game, 3);
    game.Act(Action::Forward);
    Settle(game);
    ASSERT_EQ(game.GetMenu(), Menu::Descend);
    EXPECT_EQ(game.GetDescendDepth(), 6);
    game.Act(Action::MenuSelect);
    EXPECT_FALSE(game.IsInVillage());
    EXPECT_EQ(game.GetDepth(), 6);
}

TEST(Village, BuiltBuildingsCanBeEnteredAndLeft)
{
    for (int b = 0; b < Village::kBuildings; ++b)
    {
        CrawlerGame game;
        Legend legend;
        legend.buildings[b] = 1;
        game.SetLegend(legend);
        game.GoHome(5);
        ASSERT_TRUE(FaceBuilding(game, static_cast<Village::Building>(b)));
        game.Act(Action::Interact);
        if (!Village::CanEnter(b))
        {
            EXPECT_EQ(game.GetInside(), -1) << "the lift is ridden, not entered";
            continue;
        }
        ASSERT_EQ(game.GetInside(), b);
        EXPECT_TRUE(game.IsInVillage());
        EXPECT_EQ(game.GetHeroCell(), Village::InteriorStart());
        // The keeper waits at the back, straight ahead.
        const int keeper = game.FeatureAt(Village::KeeperCell());
        ASSERT_GE(keeper, 0);
        EXPECT_EQ(game.GetFeature(keeper).kind, FeatureKind::Keeper);
        // The door is behind: turning round and walking into it goes out,
        // in front of the building, facing the plaza.
        Face(game, (Village::InteriorFacing() + 2) & 3);
        ASSERT_STREQ(game.GetTarget().verb, "Go out");
        game.Act(Action::Forward);
        Settle(game);
        EXPECT_EQ(game.GetInside(), -1);
        EXPECT_TRUE(game.IsInVillage());
        EXPECT_EQ(game.GetHeroCell(), Village::OutsideCell(b));
        EXPECT_EQ(game.GetFacing(), Village::OutsideFacing(b));
    }
}

TEST(Village, RuinsCannotBeEntered)
{
    CrawlerGame game;
    game.GoHome(5);
    ASSERT_TRUE(FaceBuilding(game, Village::Building::Tavern));
    // With nothing to build it with, it only says what it needs.
    const Target ruin = game.GetTarget();
    EXPECT_EQ(ruin.verb, nullptr);
    EXPECT_EQ(ruin.needGold, Village::UpgradeCost(2, 1).gold);
    EXPECT_EQ(ruin.haveGold, 0);
    EXPECT_NE(ruin.gain, nullptr);
    game.Act(Action::Interact);
    EXPECT_EQ(game.GetInside(), -1);
}

TEST(Village, ARuinSaysWhatItNeedsFromAFewStepsAway)
{
    CrawlerGame game;
    game.GoHome(5);
    // Three tiles back from the tavern's ruin, nothing between.
    const Cell front = Village::BuildingCell(2);
    ASSERT_TRUE(WalkTo(game, { front.x, front.y + 3 }, Village::WellCell()));
    Face(game, 3);
    const Target far = game.GetTarget();
    ASSERT_NE(far.name, nullptr);
    EXPECT_EQ(far.verb, nullptr) << "seen from afar, built from up close";
    EXPECT_EQ(far.needGold, Village::UpgradeCost(2, 1).gold);
}

TEST(Village, TheAlleyHasAManWhoSellsAndAYardOfStones)
{
    CrawlerGame game;
    Legend legend;
    legend.bank = 100;
    game.SetLegend(legend);
    game.GoHome(5);
    // In shadow, off the square.
    EXPECT_GT(Village::Gloom({ 5, 9 }), 0);
    EXPECT_EQ(Village::Gloom(Village::StartCell()), 0);
    // The man at the alley's end: moss for the village's gold.
    const Cell fence = Village::FenceCell();
    ASSERT_GE(game.FeatureAt(fence), 0);
    ASSERT_TRUE(WalkTo(game, { fence.x + 1, fence.y }, Village::WellCell()));
    Face(game, 2);
    ASSERT_STREQ(game.GetTarget().name, Village::FenceName());
    game.Act(Action::Interact);
    ASSERT_EQ(game.GetMenu(), Menu::Fence);
    // Only moss: nothing deeper was reached yet. And a word with him.
    ASSERT_EQ(game.GetMenuCount(), 2);
    EXPECT_EQ(game.GetMenuEntry(0).item, ItemKind::Moss);
    game.Act(Action::MenuSelect);
    EXPECT_EQ(game.GetLegend().bank, 100 - Village::MaterialPrice(0));
    EXPECT_EQ(game.GetLegend().materials[0], 1);
    EXPECT_EQ(game.GetMenu(), Menu::Fence) << "he goes on selling";
    game.Act(Action::MenuClose);
    // The yard: a stone is read.
    const Cell stone = Village::StoneCell(1);
    ASSERT_GE(game.PropAt(stone), 0);
    ASSERT_TRUE(WalkTo(game, { stone.x, stone.y + 1 }, Village::WellCell()));
    Face(game, 3);
    ASSERT_STREQ(game.GetTarget().verb, "Read");
    const uint32_t shown = game.GetResultCount();
    game.Act(Action::Interact);
    // Said and shown; nothing to shut.
    EXPECT_EQ(game.GetMenu(), Menu::None);
    EXPECT_EQ(game.GetResultCount(), shown + 1);
    EXPECT_STREQ(game.GetResultText(), Village::StoneLine(1));
}

TEST(Village, TheKeeperBuildsThenChats)
{
    CrawlerGame game;
    Legend legend;
    legend.bank = 1000;
    legend.materials[1] = 10;  // bone
    legend.buildings[2] = 1;   // the tavern
    // (Gunnar's contract is done with, so he has nothing to ask first.)
    Contracts::SetState(legend, Contracts::Id::Meals, Contracts::State::Kept);
    game.SetLegend(legend);
    game.GoHome(5);
    ASSERT_TRUE(FaceBuilding(game, Village::Building::Tavern));
    game.Act(Action::Interact);
    ASSERT_EQ(game.GetInside(), 2);
    ASSERT_TRUE(WalkTo(game, { Village::KeeperCell().x, Village::KeeperCell().y + 1 }));
    Face(game, 3);
    EXPECT_STREQ(game.GetTarget().name, Village::KeeperName(2));
    game.Act(Action::Interact);
    EXPECT_EQ(game.GetLegend().buildings[2], 2);
    EXPECT_EQ(game.GetLegend().bank, 880);
    // All built: the keeper only talks, a new line each time.
    legend = game.GetLegend();
    legend.buildings[2] = Village::kMaxLevel;
    game.SetLegend(legend);
    ASSERT_STREQ(game.GetTarget().verb, "Talk");
    game.Act(Action::Interact);
    ASSERT_EQ(game.GetMenu(), Menu::Talk);
    const std::string first = game.GetSpeech();
    // It stays until the key; then the next line.
    game.Act(Action::Forward);
    EXPECT_EQ(game.GetMenu(), Menu::Talk);
    game.Act(Action::MenuClose);
    EXPECT_EQ(game.GetMenu(), Menu::None);
    game.Act(Action::Interact);
    EXPECT_NE(first, game.GetSpeech());
    game.Act(Action::MenuClose);
    EXPECT_EQ(game.GetLegend().bank, 880);
}

TEST(Village, FurnishingsAreOnlyToLookAt)
{
    CrawlerGame game;
    Legend legend;
    legend.buildings[2] = 1;  // the tavern: barrels and a bed
    game.SetLegend(legend);
    game.GoHome(5);
    ASSERT_TRUE(FaceBuilding(game, Village::Building::Tavern));
    game.Act(Action::Interact);
    ASSERT_EQ(game.GetInside(), 2);
    int furnishings = 0;
    for (int i = 0; i < CrawlerGame::kMaxProps; ++i)
    {
        const Prop& p = game.GetProp(i);
        if (!p.present)
        {
            continue;
        }
        ++furnishings;
        EXPECT_TRUE(p.fixed);
        Cell stand;
        int way = 0;
        ASSERT_TRUE(StandBeside(game, p.at, stand, way));
        ASSERT_TRUE(WalkTo(game, stand));
        Face(game, way);
        EXPECT_EQ(game.GetTarget().verb, nullptr);
        const int hp = game.GetHp();
        game.Act(Action::Interact);
        EXPECT_EQ(game.GetTipsy(), 0);
        EXPECT_EQ(game.GetHp(), hp);
        EXPECT_EQ(game.GetInside(), 2) << "no ambush in the tavern's bed";
    }
    EXPECT_EQ(furnishings, 3);
}

TEST(Village, AFallIsReportedThenGoesHome)
{
    CrawlerGame game;
    game.NewRun(7, 9);
    // Stand by the nearest monster and wait: it wins in the end.
    for (int steps = 0; steps < 3000 && !game.IsDead(); ++steps)
    {
        int nearest = -1;
        int best = 1 << 30;
        for (int i = 0; i < game.GetMonsterCount(); ++i)
        {
            const Monster& m = game.GetMonster(i);
            const int d = std::abs(m.at.x - game.GetHeroCell().x) + std::abs(m.at.y - game.GetHeroCell().y);
            if (m.alive && d < best)
            {
                best = d;
                nearest = i;
            }
        }
        ASSERT_GE(nearest, 0);
        Cell stand;
        int way = 0;
        if (best > 1 && StandBeside(game, game.GetMonster(nearest).at, stand, way))
        {
            WalkTo(game, stand);
        }
        game.Act(Action::Wait);
        Settle(game);
    }
    ASSERT_TRUE(game.IsDead());
    const int gold = game.GetGold();
    const CrawlerGame::DeathReport& report = game.GetDeathReport();
    EXPECT_EQ(report.depth, game.GetDepth());
    EXPECT_EQ(report.keptGold + report.echoGold, gold);
    EXPECT_EQ(game.GetLegend().bank, report.keptGold);
    const uint32_t said = game.GetMessageCount();
    game.Act(Action::Restart);
    EXPECT_FALSE(game.IsDead());
    EXPECT_TRUE(game.IsInVillage());
    EXPECT_EQ(game.GetInside(), -1);
    // Home says only its own welcome: the fall's card told the rest.
    EXPECT_EQ(game.GetMessageCount(), said + 1);
}

TEST(Village, TheSaveKeepsTheVillage)
{
    Legend legend;
    legend.bank = 1234;
    legend.materials[0] = 1;
    legend.materials[3] = 9;
    legend.buildings[2] = 3;
    legend.buildings[5] = 2;
    legend.bosses = 3;
    char text[192];
    Lore::WriteSave(text, sizeof(text), 7, 3, legend);
    int deepest = 0;
    int pages = 0;
    Legend read;
    Lore::ReadSave(text, deepest, pages, read);
    EXPECT_EQ(read.bank, 1234);
    EXPECT_EQ(read.materials[0], 1);
    EXPECT_EQ(read.materials[3], 9);
    EXPECT_EQ(read.buildings[2], 3);
    EXPECT_EQ(read.buildings[5], 2);
    EXPECT_EQ(read.bosses, 3);
}

TEST(Village, TheControlsCardShowsOnceAndCloses)
{
    CrawlerGame game;
    game.GoHome(3);
    EXPECT_EQ(game.GetMenu(), Menu::None);  // home alone never opens it
    const uint32_t legendVersion = game.GetLegendVersion();
    game.ShowControls();
    EXPECT_EQ(game.GetMenu(), Menu::Controls);
    EXPECT_TRUE(game.GetLegend().controlsShown);
    EXPECT_NE(game.GetLegendVersion(), legendVersion);  // so it is saved
    game.Act(Action::MenuClose);
    EXPECT_EQ(game.GetMenu(), Menu::None);

    // Seen once, seen for good.
    char text[192];
    Lore::WriteSave(text, sizeof(text), 1, 0, game.GetLegend());
    int deepest = 0;
    int pages = 0;
    Legend read;
    Lore::ReadSave(text, deepest, pages, read);
    EXPECT_TRUE(read.controlsShown);
    Legend old;
    Lore::ReadSave("9 4", deepest, pages, old);
    EXPECT_FALSE(old.controlsShown);
}

TEST(Secrets, ABookshelfIsReadAndTeachesSomething)
{
    for (uint32_t seed = 1; seed < 400; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2);
        const int shelf = FindProp(game, kZoneProps + 0);
        if (shelf < 0)
        {
            continue;
        }
        Cell stand;
        int way = 0;
        if (!StandBeside(game, game.GetProp(shelf).at, stand, way) || !WalkTo(game, stand))
        {
            continue;
        }
        Face(game, way);
        ASSERT_STREQ(game.GetTarget().verb, "Look closer");
        ASSERT_TRUE(Choose(game, Deed::Read));
        ASSERT_EQ(game.GetMenu(), Menu::Book);
        ASSERT_NE(game.GetReading(), nullptr);
        EXPECT_GT(std::strlen(game.GetReading()->title), 0u);
        game.Act(Action::Wait);
        EXPECT_EQ(game.GetMenu(), Menu::None);
        EXPECT_FALSE(Choose(game, Deed::Read)) << "read already";
        return;
    }
    FAIL() << "no bookshelf reached";
}

TEST(Secrets, ALockedChestNeedsALockpick)
{
    for (uint32_t seed = 1; seed < 400; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2);
        const int chest = FindProp(game, kZoneProps + 2);
        if (chest < 0 || game.Carried(ItemKind::Lockpick) > 0)
        {
            continue;
        }
        Cell stand;
        int way = 0;
        if (!StandBeside(game, game.GetProp(chest).at, stand, way) || !WalkTo(game, stand) ||
            game.Carried(ItemKind::Lockpick) > 0)
        {
            continue;
        }
        Face(game, way);
        ASSERT_TRUE(Choose(game, Deed::PickLock));
        EXPECT_TRUE(game.HasDeeds(game.GetProp(chest)));  // still locked
        EXPECT_NE(std::strstr(game.GetMessage(), "lockpick"), nullptr);
        return;
    }
    FAIL() << "no locked chest reached";
}

TEST(Secrets, ALooseBrickHidesInAPlainWall)
{
    for (uint32_t seed = 1; seed < 200; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2);
        Cell brick;
        if (!Find(game, Tile::LooseBrick, brick))
        {
            continue;
        }
        Cell stand;
        int way = 0;
        if (!StandBeside(game, brick, stand, way) || !WalkTo(game, stand))
        {
            continue;
        }
        Face(game, way);
        ASSERT_STREQ(game.GetTarget().name, "Loose brick");
        game.Act(Action::Interact);
        EXPECT_EQ(game.GetMap().At(brick), Tile::Wall);
        return;
    }
    FAIL() << "no loose brick reached";
}

TEST(Village, VillagersTalkInTurn)
{
    CrawlerGame game;
    game.GoHome(5);
    const Cell pip = Village::VillagerCell(1);
    Cell stand;
    int way = 0;
    ASSERT_TRUE(StandBeside(game, pip, stand, way));
    ASSERT_TRUE(WalkTo(game, stand, Village::WellCell()));
    Face(game, way);
    ASSERT_STREQ(game.GetTarget().verb, "Talk");
    game.Act(Action::Interact);
    const std::string first = game.GetSpeech();
    EXPECT_EQ(first.rfind("Leif:", 0), 0u);
    game.Act(Action::MenuClose);
    game.Act(Action::Interact);
    EXPECT_NE(first, game.GetSpeech());
}

namespace
{

/// Stands by the nearest monster and waits until it wins.
void FallOn(CrawlerGame& game)
{
    for (int steps = 0; steps < 3000 && !game.IsDead(); ++steps)
    {
        int nearest = -1;
        int best = 1 << 30;
        for (int i = 0; i < game.GetMonsterCount(); ++i)
        {
            const Monster& m = game.GetMonster(i);
            const int d = std::abs(m.at.x - game.GetHeroCell().x) + std::abs(m.at.y - game.GetHeroCell().y);
            if (m.alive && d < best)
            {
                best = d;
                nearest = i;
            }
        }
        if (nearest < 0)
        {
            return;
        }
        Cell stand;
        int way = 0;
        if (best > 1 && StandBeside(game, game.GetMonster(nearest).at, stand, way))
        {
            WalkTo(game, stand);
        }
        game.Act(Action::Wait);
        Settle(game);
    }
}

bool FloorHasLockedWeapon(const CrawlerGame& game)
{
    for (int i = 0; i < CrawlerGame::kMaxItems; ++i)
    {
        const Item& item = game.GetItem(i);
        const int weapon = static_cast<int>(item.kind) - static_cast<int>(ItemKind::Blade);
        if (!item.taken && weapon > 0 && weapon < kWeapons && !game.IsWeaponUnlocked(static_cast<Weapon>(weapon)))
        {
            return true;
        }
    }
    return false;
}

}  // namespace

TEST(Village, WeaponsTurnUpOnlyOnceARecordUnlockedThem)
{
    CrawlerGame fresh;
    EXPECT_TRUE(fresh.IsWeaponUnlocked(Weapon::Blade));
    EXPECT_FALSE(fresh.IsWeaponUnlocked(Weapon::Axe));
    // A hero who never got past B9: no hammer lies about, however deep the
    // lift takes them.
    for (uint32_t seed = 1; seed <= 80; ++seed)
    {
        CrawlerGame game;
        game.SetDeepest(9);
        game.NewRun(seed, 9);
        EXPECT_TRUE(game.IsWeaponUnlocked(Weapon::Dagger));
        EXPECT_FALSE(game.IsWeaponUnlocked(Weapon::Hammer));
        EXPECT_FALSE(FloorHasLockedWeapon(game)) << "seed " << seed;
    }
    fresh.SetDeepest(CrawlerGame::WeaponUnlockDepth(Weapon::Hammer));
    EXPECT_TRUE(fresh.IsWeaponUnlocked(Weapon::Hammer));
}

TEST(Village, AFallSaysTheRecordAndWhatToWorkToward)
{
    // A floor whose monsters come for the hero.
    CrawlerGame game;
    for (uint32_t seed = 1; seed < 30 && !game.IsDead(); ++seed)
    {
        game = CrawlerGame{};
        game.NewRun(seed, 4);
        FallOn(game);
    }
    ASSERT_TRUE(game.IsDead());
    const CrawlerGame::DeathReport& report = game.GetDeathReport();
    EXPECT_TRUE(report.record) << "the first run is always the deepest yet";
    ASSERT_NE(report.unlocked, nullptr) << "B4 is past where the axes open";
    EXPECT_NE(std::string(report.unlocked).find("lie about"), std::string::npos);
    // Nothing built and little gold: a building to save for, and what is in.
    EXPECT_NE(std::string(report.goal[0]).find("needs"), std::string::npos);
    EXPECT_NE(std::string(report.goal[1]).find("The village has"), std::string::npos);
    EXPECT_FALSE(report.goalReady);

    // With enough at home already, the fall says so.
    CrawlerGame rich;
    Legend legend;
    legend.bank = 500;
    legend.materials[0] = 9;
    rich.SetLegend(legend);
    for (uint32_t seed = 1; seed < 30 && !rich.IsDead(); ++seed)
    {
        rich = CrawlerGame{};
        rich.SetLegend(legend);
        rich.SetDeepest(20);
        rich.NewRun(seed, 4);
        FallOn(rich);
    }
    ASSERT_TRUE(rich.IsDead());
    EXPECT_FALSE(rich.GetDeathReport().record);
    EXPECT_EQ(rich.GetDeathReport().unlocked, nullptr);
    EXPECT_TRUE(rich.GetDeathReport().goalReady);
    EXPECT_NE(std::string(rich.GetDeathReport().goal[0]).find("can be built"), std::string::npos);
}
