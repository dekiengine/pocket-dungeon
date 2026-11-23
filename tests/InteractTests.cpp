// Using things off the device: E says what is ahead and uses it, things are
// taken from where you stand, props run out, a beer makes you tipsy for a
// while, and a monster ahead shows its health.

#include <gtest/gtest.h>

#include "../src/world/CrawlerGame.h"
#include "../src/world/CrawlerGameData.h"
#include "Walk.h"

#include <cstdlib>
#include <cstring>

using namespace Dungeon;
using namespace Dungeon::TestWalk;

namespace
{

/// Walks beside prop `kind` (of the zone's kZoneProps) and faces it. -1 if
/// none could be reached on this floor, else the prop's index.
int FaceProp(CrawlerGame& game, int kind)
{
    for (int i = 0; i < CrawlerGame::kMaxProps; ++i)
    {
        const Prop& p = game.GetProp(i);
        if (!p.present || p.any || p.kind != kind)
        {
            continue;
        }
        Cell stand;
        int way = 0;
        if (StandBeside(game, p.at, stand, way) && WalkTo(game, stand))
        {
            Face(game, way);
            return i;
        }
    }
    return -1;
}

}  // namespace

TEST(Interact, NothingAheadMeansNoPromptAndNothingHappens)
{
    CrawlerGame game;
    game.NewRun(3);
    // Face a plain wall.
    for (int f = 0; f < 4; ++f)
    {
        const Cell h = game.GetHeroCell();
        const Cell ahead{ h.x + kDx[f], h.y + kDy[f] };
        if (game.GetMap().At(ahead) == Tile::Wall)
        {
            Face(game, f);
            break;
        }
    }
    const Target target = game.GetTarget();
    EXPECT_EQ(target.verb, nullptr);
    const Cell before = game.GetHeroCell();
    game.Act(Action::Interact);
    Settle(game);
    EXPECT_EQ(game.GetHeroCell(), before);
}

TEST(Interact, ABarrelsAleMakesYouTipsyThreeTimesThenRunsDry)
{
    for (uint32_t seed = 1; seed < 300; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2);  // the mossy halls: the barrel is their second prop
        const int barrel = FaceProp(game, 1);
        if (barrel < 0)
        {
            continue;
        }
        Target target = game.GetTarget();
        ASSERT_STREQ(target.name, "Ale barrel");
        ASSERT_STREQ(target.verb, "Look closer");
        for (int i = 0; i < 3; ++i)
        {
            ASSERT_TRUE(Choose(game, Deed::Drink)) << "gulp " << i;
        }
        EXPECT_GT(game.GetTipsy(), 0);
        // Dry: only smashing it is left.
        EXPECT_FALSE(Choose(game, Deed::Drink));
        EXPECT_TRUE(game.HasDeeds(game.GetProp(barrel)));
        // Waiting wears it off.
        for (int i = 0; i < 40 && game.GetTipsy() > 0 && !game.IsDead(); ++i)
        {
            game.Act(Action::Wait);
            Settle(game);
        }
        EXPECT_TRUE(game.GetTipsy() == 0 || game.IsDead());
        return;
    }
    FAIL() << "no barrel reached";
}

TEST(Interact, SearchingAPropUsesItUp)
{
    for (uint32_t seed = 1; seed < 300; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2);
        const int pack = FaceProp(game, 0);
        if (pack < 0)
        {
            continue;
        }
        ASSERT_STREQ(game.GetTarget().verb, "Look closer");
        ASSERT_TRUE(Choose(game, Deed::Search));
        EXPECT_FALSE(game.HasDeeds(game.GetProp(pack)));
        EXPECT_EQ(game.GetTarget().verb, nullptr);
        return;
    }
    FAIL() << "no pack reached";
}

TEST(Interact, AThingAheadIsTakenWithoutStepping)
{
    for (uint32_t seed = 1; seed < 300; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2);
        for (int i = 0; i < CrawlerGame::kMaxItems; ++i)
        {
            const Item& item = game.GetItem(i);
            if (item.taken || item.kind == ItemKind::Chest)
            {
                continue;
            }
            Cell stand;
            int way = 0;
            if (!StandBeside(game, item.at, stand, way) || !WalkTo(game, stand) || game.GetItem(i).taken)
            {
                continue;
            }
            Face(game, way);
            const Target target = game.GetTarget();
            if (!target.verb || std::strcmp(target.verb, "Take") != 0 || game.MonsterAt(item.at) >= 0)
            {
                continue;
            }
            const Cell before = game.GetHeroCell();
            game.Act(Action::Interact);
            EXPECT_TRUE(game.GetItem(i).taken);
            EXPECT_EQ(game.GetHeroCell(), before);
            return;
        }
    }
    FAIL() << "no item reached";
}

TEST(Interact, AMonsterAheadShowsItsHealth)
{
    for (uint32_t seed = 1; seed < 100; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 3);
        for (int i = 0; i < game.GetMonsterCount(); ++i)
        {
            const Monster& m = game.GetMonster(i);
            if (!m.alive)
            {
                continue;
            }
            Cell stand;
            int way = 0;
            if (!StandBeside(game, m.at, stand, way) || !WalkTo(game, stand))
            {
                continue;
            }
            Face(game, way);
            const Target target = game.GetTarget();
            if (target.hp < 0)
            {
                continue;  // it moved
            }
            EXPECT_STREQ(target.verb, "Attack");
            EXPECT_GT(target.maxHp, 0);
            EXPECT_LE(target.hp, target.maxHp);
            return;
        }
    }
    FAIL() << "no monster faced";
}

TEST(Interact, ThingsShowTheSideFacingTheViewer)
{
    // A thing at the origin facing east (0): in front of it is east.
    const Fix one = kFixOne;
    EXPECT_EQ(CrawlerGame::ViewIndex(0, 0, 0, 3 * one, 0), 0);         // in front: its front
    EXPECT_EQ(CrawlerGame::ViewIndex(0, 0, 0, -3 * one, 0), 4);        // behind: its back
    EXPECT_EQ(CrawlerGame::ViewIndex(0, 0, 0, 0, 3 * one), 2);         // south of it: its right side
    EXPECT_EQ(CrawlerGame::ViewIndex(0, 0, 0, 0, -3 * one), 6);        // north: its left side
    EXPECT_EQ(CrawlerGame::ViewIndex(0, 0, 0, 3 * one, 3 * one), 1);   // front right
    EXPECT_EQ(CrawlerGame::ViewIndex(0, 0, 0, 3 * one, -3 * one), 7);  // front left
    // Facing north (3): in front of it is north.
    EXPECT_EQ(CrawlerGame::ViewIndex(0, 0, 3, 0, -2 * one), 0);
    EXPECT_EQ(CrawlerGame::ViewIndex(0, 0, 3, 2 * one, 0), 2);  // east of a north-facing thing: its right
}

TEST(Interact, AFloorsThingsAreSpreadOverItsRooms)
{
    // Never the same thing twice in a room (pots excepted: they huddle),
    // never more than two of a zone's kind on a floor, and nothing standing
    // right beside another thing that is not a pot.
    for (uint32_t seed = 1; seed <= 60; ++seed)
    {
        for (int depth : { 1, 4, 8, 13, 18 })
        {
            CrawlerGame game;
            game.NewRun(seed, depth);
            const DungeonMap& map = game.GetMap();
            int ofKind[16] = {};
            for (int i = 0; i < CrawlerGame::kMaxProps; ++i)
            {
                const Prop& p = game.GetProp(i);
                if (!p.present || p.any)
                {
                    continue;  // a themed room's things come in pairs, on purpose
                }
                ++ofKind[p.kind];
                const bool pot = p.kind == kZoneProps + 3;
                const bool chest = p.kind == kZoneProps + 6;  // a vault holds two
                for (int j = i + 1; j < CrawlerGame::kMaxProps; ++j)
                {
                    const Prop& q = game.GetProp(j);
                    if (!q.present || q.any)
                    {
                        continue;
                    }
                    for (int r = 0; r < map.GetRoomCount(); ++r)
                    {
                        const bool together = map.GetRoom(r).Contains(p.at) && map.GetRoom(r).Contains(q.at);
                        EXPECT_FALSE(together && p.kind == q.kind && !pot && !chest)
                            << "seed " << seed << " depth " << depth << ": two of kind " << int(p.kind) << " in a room";
                    }
                    const int apart = std::abs(p.at.x - q.at.x) + std::abs(p.at.y - q.at.y);
                    EXPECT_FALSE(apart == 1 && !(pot && q.kind == p.kind))
                        << "seed " << seed << " depth " << depth << ": things side by side";
                }
            }
            for (int k = 0; k < kZoneProps; ++k)
            {
                EXPECT_LE(ofKind[k], 2) << "seed " << seed << " depth " << depth;
            }
        }
    }
}

TEST(Interact, APotSmashesAndLeavesItsTileFree)
{
    constexpr int kPot = kZoneProps + 3;
    for (uint32_t seed = 1; seed < 40; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2);
        const int pot = FaceProp(game, kPot);
        if (pot < 0)
        {
            continue;
        }
        const Cell at = game.GetProp(pot).at;
        ASSERT_STREQ(game.GetTarget().verb, "Look closer");
        game.TakeEvents(nullptr, 0);
        const uint32_t said = game.GetMessageCount();
        ASSERT_TRUE(Choose(game, Deed::Smash));
        EXPECT_FALSE(game.GetProp(pot).present);
        EXPECT_EQ(game.PropAt(at), -1);
        EXPECT_NE(game.GetMessageCount(), said) << "what was in it is said";
        // Its pieces fly: the screen is told.
        bool broke = false;
        for (int i = 0; i < game.GetEventCount(); ++i)
        {
            broke = broke || (game.GetEvent(i).kind == EventKind::Break && game.GetEvent(i).at == at);
        }
        EXPECT_TRUE(broke);
        // And the way through is open.
        game.Act(Action::Forward);
        Settle(game);
        EXPECT_EQ(game.GetHeroCell(), at);
        return;
    }
    FAIL() << "no floor had a pot to reach";
}

TEST(Interact, AThingOffersChoicesAndLeavingItChangesNothing)
{
    for (uint32_t seed = 1; seed < 300; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2);
        const int barrel = FaceProp(game, 1);
        if (barrel < 0)
        {
            continue;
        }
        const int hp = game.GetHp();
        game.Act(Action::Interact);
        ASSERT_EQ(game.GetMenu(), Menu::Thing);
        EXPECT_STREQ(game.GetThingName(), "Ale barrel");
        ASSERT_EQ(game.GetMenuCount(), 2);
        EXPECT_EQ(game.GetMenuEntry(0).deed, Deed::Drink);
        EXPECT_EQ(game.GetMenuEntry(1).deed, Deed::Smash);
        // The line after the choices leaves it.
        game.Act(Action::MenuDown);
        game.Act(Action::MenuDown);
        game.Act(Action::MenuSelect);
        EXPECT_EQ(game.GetMenu(), Menu::None);
        EXPECT_EQ(game.GetHp(), hp);
        EXPECT_EQ(game.GetTipsy(), 0);
        EXPECT_TRUE(game.GetProp(barrel).present);
        // And smashing it takes it away.
        ASSERT_TRUE(Choose(game, Deed::Smash));
        EXPECT_FALSE(game.GetProp(barrel).present);
        return;
    }
    FAIL() << "no barrel reached";
}

TEST(Interact, KnockingOnACoffinTellsWhoIsHome)
{
    for (uint32_t seed = 1; seed < 300; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 7);  // the bone catacombs: the coffin is their third prop
        const int coffin = FaceProp(game, 2);
        if (coffin < 0)
        {
            continue;
        }
        ASSERT_STREQ(game.GetTarget().name, "Stone coffin");
        const bool home = game.GetProp(coffin).state != 0;
        ASSERT_TRUE(Choose(game, Deed::Knock));
        EXPECT_EQ(std::strstr(game.GetMessage(), "knocks back") != nullptr, home);
        EXPECT_FALSE(Choose(game, Deed::Knock)) << "once is enough";
        // Opening it lets out whoever was in, and then it can be emptied.
        int before = 0;
        for (int i = 0; i < CrawlerGame::kMaxMonsters; ++i)
        {
            before += game.GetMonster(i).alive ? 1 : 0;
        }
        ASSERT_TRUE(Choose(game, Deed::OpenCoffin));
        int after = 0;
        for (int i = 0; i < CrawlerGame::kMaxMonsters; ++i)
        {
            after += game.GetMonster(i).alive ? 1 : 0;
        }
        if (home && after > before)
        {
            EXPECT_TRUE(game.HasDeeds(game.GetProp(coffin))) << "what it lay on is still there";
        }
        else
        {
            EXPECT_FALSE(game.HasDeeds(game.GetProp(coffin)));
        }
        return;
    }
    FAIL() << "no coffin reached";
}

TEST(Interact, AnAnvilSharpensOrMendsNotBoth)
{
    for (uint32_t seed = 1; seed < 300; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 17);  // the ember forge: the anvil is its first prop
        const int anvil = FaceProp(game, 0);
        if (anvil < 0)
        {
            continue;
        }
        const int attack = game.GetAttack();
        const int defense = game.GetDefense();
        ASSERT_TRUE(Choose(game, Deed::MendArmour));
        EXPECT_EQ(game.GetDefense(), defense + 1);
        EXPECT_EQ(game.GetAttack(), attack);
        EXPECT_FALSE(game.HasDeeds(game.GetProp(anvil)));
        return;
    }
    FAIL() << "no anvil reached";
}

TEST(Interact, AFountainIsDrunkFromOrWashedIn)
{
    for (uint32_t seed = 1; seed < 400; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2);
        int fountain = -1;
        for (int i = 0; i < CrawlerGame::kMaxFeatures; ++i)
        {
            if (game.GetFeature(i).present && game.GetFeature(i).kind == FeatureKind::Fountain)
            {
                fountain = i;
            }
        }
        Cell stand;
        int way = 0;
        if (fountain < 0 || !StandBeside(game, game.GetFeature(fountain).at, stand, way) || !WalkTo(game, stand))
        {
            continue;
        }
        Face(game, way);
        game.Act(Action::Interact);
        ASSERT_EQ(game.GetMenu(), Menu::Thing);
        ASSERT_EQ(game.GetMenuCount(), 3);
        game.Act(Action::MenuClose);
        const int hp = game.GetHp();
        ASSERT_TRUE(Choose(game, Deed::Wash));
        EXPECT_GE(game.GetHp(), hp);
        EXPECT_TRUE(game.GetFeature(fountain).used);
        EXPECT_EQ(game.GetTarget().verb, nullptr);
        return;
    }
    FAIL() << "no fountain reached";
}

TEST(Interact, EveryThingsTextFitsItsMenu)
{
    for (const GameData::PropInfo& info : GameData::kPropInfo)
    {
        EXPECT_LE(std::strlen(info.about), 37u) << info.name;
        EXPECT_NE(info.deeds[0], Deed::None) << info.name;
    }
    for (int d = 1; d < kDeeds; ++d)
    {
        const GameData::DeedInfo& info = GameData::kDeedInfo[d];
        EXPECT_GT(std::strlen(info.label), 0u) << "deed " << d;
        EXPECT_LE(std::strlen(info.label), 24u) << info.label;
        EXPECT_LE(std::strlen(info.hint), 37u) << info.label;
    }
}

namespace
{

/// Opens the bag on `kind`'s line and takes it. False if it is not in there.
bool TakeFromBag(CrawlerGame& game, ItemKind kind)
{
    game.Act(Action::OpenBag);
    for (int i = 0; i < game.GetMenuCount(); ++i)
    {
        if (game.GetMenuEntry(i).item == kind)
        {
            for (int k = 0; k < i; ++k)
            {
                game.Act(Action::MenuDown);
            }
            game.Act(Action::MenuSelect);
            return true;
        }
    }
    game.Act(Action::MenuClose);
    return false;
}

/// Takes the open thing menu's line that does `deed`.
bool Pick(CrawlerGame& game, Deed deed)
{
    if (game.GetMenu() != Menu::Thing)
    {
        return false;
    }
    for (int i = 0; i < game.GetMenuCount(); ++i)
    {
        if (game.GetMenuEntry(i).deed == deed)
        {
            for (int k = 0; k < i; ++k)
            {
                game.Act(Action::MenuDown);
            }
            game.Act(Action::MenuSelect);
            return true;
        }
    }
    return false;
}

/// The first monster in a straight line ahead of the hero, up to five
/// tiles off, or -1.
int MonsterInLine(const CrawlerGame& game)
{
    Cell c = game.GetHeroCell();
    for (int i = 0; i < 5; ++i)
    {
        c = { c.x + kDx[game.GetFacing()], c.y + kDy[game.GetFacing()] };
        if (!game.GetMap().IsWalkable(c) || game.PropAt(c) >= 0 || game.FeatureAt(c) >= 0)
        {
            return -1;
        }
        if (game.MonsterAt(c) >= 0)
        {
            return game.MonsterAt(c);
        }
    }
    return -1;
}

}  // namespace

TEST(Interact, FoodFromTheBagIsEatenOrThrown)
{
    CrawlerGame game;
    game.NewRun(3);
    game.SetCarried(ItemKind::Snack, 2);
    // Taking it out asks: eat it, or throw it.
    ASSERT_TRUE(TakeFromBag(game, ItemKind::Snack));
    ASSERT_EQ(game.GetMenu(), Menu::Thing);
    EXPECT_STREQ(game.GetThingName(), "Snack");
    ASSERT_EQ(game.GetMenuCount(), 2);
    ASSERT_TRUE(Pick(game, Deed::Eat));
    EXPECT_EQ(game.Carried(ItemKind::Snack), 1);
    EXPECT_EQ(game.GetMenu(), Menu::None);
}

TEST(Interact, ThrownFoodKeepsAMonsterBusy)
{
    for (uint32_t seed = 1; seed < 200; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2);
        game.SetCarried(ItemKind::Fish, 1);
        // Nothing in line: nothing to throw it to, and the fish is kept.
        if (MonsterInLine(game) < 0)
        {
            ASSERT_TRUE(TakeFromBag(game, ItemKind::Fish));
            ASSERT_TRUE(Pick(game, Deed::Throw));
            EXPECT_EQ(game.Carried(ItemKind::Fish), 1) << "seed " << seed;
        }
        // Wait for something to come into line, turning to look.
        int target = -1;
        for (int turn = 0; turn < 60 && target < 0 && !game.IsDead(); ++turn)
        {
            for (int f = 0; f < 4 && target < 0; ++f)
            {
                Face(game, f);
                target = MonsterInLine(game);
            }
            if (target < 0)
            {
                game.Act(Action::Wait);
                Settle(game);
            }
        }
        if (target < 0 || game.IsDead() || game.GetMonster(target).kind >= MonsterKind::Glimmerjelly)
        {
            continue;
        }
        ASSERT_TRUE(TakeFromBag(game, ItemKind::Fish));
        ASSERT_TRUE(Pick(game, Deed::Throw));
        EXPECT_EQ(game.Carried(ItemKind::Fish), 0) << "seed " << seed;
        EXPECT_GT(game.GetMonster(target).frozen, 0) << "seed " << seed;
        return;
    }
    FAIL() << "no monster came into line";
}

TEST(Interact, AMineCartIsShovedAway)
{
    for (uint32_t seed = 1; seed < 300; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 17);  // the ember forge: the mine cart is its fourth prop
        const int cart = FaceProp(game, 3);
        if (cart < 0)
        {
            continue;
        }
        ASSERT_STREQ(game.GetTarget().name, "Mine cart");
        const Cell at = game.GetProp(cart).at;
        const uint32_t said = game.GetMessageCount();
        ASSERT_TRUE(Choose(game, Deed::PushCart));
        EXPECT_FALSE(game.GetProp(cart).present);
        EXPECT_EQ(game.PropAt(at), -1);
        EXPECT_NE(game.GetMessageCount(), said);
        return;
    }
    FAIL() << "no mine cart reached";
}

TEST(Interact, AnOldChestMayNotBeAChest)
{
    constexpr int kOldChest = kZoneProps + 6;
    bool sawChest = false;
    bool sawOther = false;
    for (uint32_t seed = 1; seed < 600 && !(sawChest && sawOther); ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 7);
        const int chest = FaceProp(game, kOldChest);
        if (chest < 0)
        {
            continue;
        }
        const bool other = game.GetProp(chest).state != 0;
        if ((other && sawOther) || (!other && sawChest))
        {
            continue;
        }
        ASSERT_TRUE(Choose(game, Deed::Prod));
        if (other)
        {
            // It ran: the tile is free.
            EXPECT_FALSE(game.GetProp(chest).present) << "seed " << seed;
            sawOther = true;
        }
        else
        {
            // Only wood: it is still there to open.
            EXPECT_TRUE(game.GetProp(chest).present);
            ASSERT_TRUE(Choose(game, Deed::OpenChest));
            EXPECT_FALSE(game.HasDeeds(game.GetProp(chest)));
            sawChest = true;
        }
    }
    EXPECT_TRUE(sawChest);
    EXPECT_TRUE(sawOther);
}

TEST(Interact, AFountainFillsAWaterskinForLater)
{
    for (uint32_t seed = 1; seed < 400; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2);
        int fountain = -1;
        for (int i = 0; i < CrawlerGame::kMaxFeatures; ++i)
        {
            if (game.GetFeature(i).present && game.GetFeature(i).kind == FeatureKind::Fountain)
            {
                fountain = i;
            }
        }
        Cell stand;
        int way = 0;
        if (fountain < 0 || !StandBeside(game, game.GetFeature(fountain).at, stand, way) || !WalkTo(game, stand))
        {
            continue;
        }
        Face(game, way);
        ASSERT_TRUE(Choose(game, Deed::FillSkin));
        EXPECT_EQ(game.Carried(ItemKind::Water), 1);
        EXPECT_TRUE(game.GetFeature(fountain).used);
        return;
    }
    FAIL() << "no fountain reached";
}

TEST(Interact, EveryItemHasAName)
{
    for (int k = 0; k < kItemKinds; ++k)
    {
        EXPECT_STRNE(CrawlerGame::ItemName(static_cast<ItemKind>(k)), "?") << "item " << k;
    }
    // What goes in the bag says what it is for.
    for (const ItemKind kind : GameData::kBagKinds)
    {
        EXPECT_GT(std::strlen(CrawlerGame::ItemHelp(kind)), 0u) << CrawlerGame::ItemName(kind);
    }
}

TEST(Interact, TheBagShowsThingsNotTheLastMenusChoices)
{
    for (uint32_t seed = 1; seed < 300; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2);
        if (FaceProp(game, 1) < 0)
        {
            continue;
        }
        // A thing's choices were open; then the bag.
        game.Act(Action::Interact);
        ASSERT_EQ(game.GetMenu(), Menu::Thing);
        game.Act(Action::MenuClose);
        game.SetCarried(ItemKind::Potion, 1);
        game.SetCarried(ItemKind::Bomb, 1);
        game.Act(Action::OpenBag);
        ASSERT_EQ(game.GetMenu(), Menu::Bag);
        for (int i = 0; i < game.GetMenuCount(); ++i)
        {
            EXPECT_EQ(game.GetMenuEntry(i).label, nullptr) << "line " << i;
            EXPECT_EQ(game.GetMenuEntry(i).deed, Deed::None) << "line " << i;
        }
        return;
    }
    FAIL() << "no barrel reached";
}

TEST(Interact, WhatADeedDidIsShownAndPlayGoesOn)
{
    for (uint32_t seed = 1; seed < 300; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2);
        if (FaceProp(game, 1) < 0)
        {
            continue;
        }
        game.Act(Action::Interact);
        ASSERT_EQ(game.GetMenu(), Menu::Thing);
        const uint32_t shown = game.GetResultCount();
        game.Act(Action::MenuSelect);  // drink
        // Said (the screen shows it), with no card to shut: the next key
        // is the hero's again.
        EXPECT_EQ(game.GetMenu(), Menu::None);
        EXPECT_EQ(game.GetResultCount(), shown + 1);
        EXPECT_GT(std::strlen(game.GetResultText()), 0u);
        EXPECT_EQ(game.GetResultItem(), -1) << "a drink leaves nothing in hand";
        const int facing = game.GetFacing();
        game.Act(Action::TurnRight);
        EXPECT_NE(game.GetFacing(), facing);
        return;
    }
    FAIL() << "no barrel reached";
}

TEST(Interact, TheMenuHoldsTheBagTheMagicAndTheWayHome)
{
    CrawlerGame game;
    game.NewRun(3);
    game.Act(Action::OpenMenu);
    ASSERT_EQ(game.GetMenu(), Menu::Main);
    ASSERT_EQ(game.GetMenuCount(), 7);
    // The fifth line steps how loud the sounds are, and stays in the menu.
    const int loud = game.GetSoundVolume();
    for (int i = 0; i < 4; ++i)
    {
        game.Act(Action::MenuDown);
    }
    game.Act(Action::MenuSelect);
    EXPECT_NE(game.GetSoundVolume(), loud);
    EXPECT_EQ(game.GetMenu(), Menu::Main);
    for (int i = 0; i < 4; ++i)
    {
        game.Act(Action::MenuUp);
    }
    game.Act(Action::MenuSelect);
    EXPECT_EQ(game.GetMenu(), Menu::Bag);
    game.Act(Action::MenuClose);
    // The way home: only with nothing hunting nearby.
    for (uint32_t seed = 1; seed < 100; ++seed)
    {
        CrawlerGame run;
        run.NewRun(seed);
        if (!run.CanReturnHome())
        {
            continue;
        }
        run.Act(Action::OpenMenu);
        run.Act(Action::MenuDown);
        run.Act(Action::MenuDown);
        EXPECT_EQ(run.GetMenuEntry(2).count, 1);
        run.Act(Action::MenuSelect);
        EXPECT_TRUE(run.IsInVillage()) << "seed " << seed;
        return;
    }
    FAIL() << "no quiet start";
}

TEST(Interact, AMurkyVialDoesSomething)
{
    CrawlerGame game;
    game.NewRun(3);
    game.SetCarried(ItemKind::Vial, 1);
    const uint32_t said = game.GetMessageCount();
    ASSERT_TRUE(TakeFromBag(game, ItemKind::Vial));
    EXPECT_EQ(game.Carried(ItemKind::Vial), 0);
    EXPECT_NE(game.GetMessageCount(), said);
    EXPECT_FALSE(game.IsDead()) << "it never kills";
}

TEST(Interact, AnEchoStoneCanAlwaysBeReached)
{
    for (uint32_t seed = 1; seed <= 80; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed);  // the very first floor always has one
        Cell stone;
        ASSERT_TRUE(Find(game, Tile::EchoWall, stone)) << "seed " << seed;
        bool open = false;
        for (int f = 0; f < 4; ++f)
        {
            const Cell c{ stone.x + kDx[f], stone.y + kDy[f] };
            open = open || (game.GetMap().IsWalkable(c) && game.PropAt(c) < 0 && game.FeatureAt(c) < 0);
        }
        EXPECT_TRUE(open) << "seed " << seed << ": nothing free beside the stone";
    }
}

TEST(Interact, AChestIsAThingThatStaysNotSomethingToPocket)
{
    int chests = 0;
    for (uint32_t seed = 1; seed <= 60; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2 + static_cast<int>(seed % 15));
        for (int i = 0; i < CrawlerGame::kMaxItems; ++i)
        {
            EXPECT_FALSE(!game.GetItem(i).taken && game.GetItem(i).kind == ItemKind::Chest) << "seed " << seed;
        }
        for (int i = 0; i < CrawlerGame::kMaxProps; ++i)
        {
            const Prop& p = game.GetProp(i);
            chests += p.present && !p.any && p.kind == kZoneProps + 6 ? 1 : 0;
        }
    }
    EXPECT_GT(chests, 20) << "chests stand about as things";
}

TEST(Interact, SomeRoomsAreSomethingAndSomeHavePillars)
{
    int themed = 0;
    int pillars = 0;
    for (uint32_t seed = 1; seed <= 60; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2 + static_cast<int>(seed % 15));
        const DungeonMap& map = game.GetMap();
        int dressed = 0;
        for (int i = 0; i < CrawlerGame::kMaxProps; ++i)
        {
            dressed += game.GetProp(i).present && game.GetProp(i).any ? 1 : 0;
        }
        themed += dressed >= 2 ? 1 : 0;
        for (int r = 1; r < map.GetRoomCount(); ++r)
        {
            const Room& room = map.GetRoom(r);
            for (int y = room.y; y < room.y + room.h; ++y)
            {
                for (int x = room.x; x < room.x + room.w; ++x)
                {
                    pillars += map.At(x, y) == Tile::Wall ? 1 : 0;
                }
            }
        }
        // Whatever stands in the rooms, the stairs can still be reached.
        ASSERT_TRUE(WalkTo(game, map.GetStairs()) || game.IsDead() ||
                    game.GetDepth() != 2 + static_cast<int>(seed % 15))
            << "seed " << seed;
    }
    EXPECT_GT(themed, 15);
    EXPECT_GT(pillars, 20);
}
