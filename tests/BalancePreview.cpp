// Plays runs by a simple bot, on a PC, to see how the game is balanced: how
// deep a run gets, what it brings home, how many runs each village upgrade
// takes. The bot picks up what lies about, smashes and searches what it
// passes, drinks a potion when hurt, fights what comes and takes the stairs.
// It plays worse than a person (no spells, no bombs, no running away), so
// the numbers are a floor, and what matters is how they compare.
// Not a test. Usage: BalancePreview [runs] [details]

#include "../src/world/CrawlerGame.h"
#include "Walk.h"

#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>

using namespace Dungeon;
using namespace Dungeon::TestWalk;

namespace
{

void CloseAnyMenu(CrawlerGame& game)
{
    for (int i = 0; i < 4 && game.GetMenu() != Menu::None; ++i)
    {
        const Menu menu = game.GetMenu();
        game.Act(menu == Menu::Ending || menu == Menu::Descend ? Action::MenuSelect : Action::MenuClose);
    }
}

void DrinkIfHurt(CrawlerGame& game)
{
    if (game.GetHp() * 100 < game.GetMaxHp() * 45 && game.Carried(ItemKind::Potion) > 0 && game.GetMenu() == Menu::None)
    {
        game.Act(Action::OpenBag);  // the potion is the bag's first line
        game.Act(Action::MenuSelect);
        CloseAnyMenu(game);
    }
}

/// One step toward `goal`, hitting a monster beside the hero first.
bool StepToward(CrawlerGame& game, Cell goal)
{
    CloseAnyMenu(game);
    DrinkIfHurt(game);
    const Cell hero = game.GetHeroCell();
    for (int f = 0; f < 4; ++f)
    {
        if (game.MonsterAt({ hero.x + kDx[f], hero.y + kDy[f] }) >= 0)
        {
            Face(game, f);
            game.Act(Action::Forward);
            Settle(game);
            return true;
        }
    }
    const int depth = game.GetDepth();
    const bool home = game.IsInVillage();
    if (!WalkTo(game, goal) && game.GetHeroCell() == hero && game.GetDepth() == depth && game.IsInVillage() == home)
    {
        return false;
    }
    return true;
}

// Also where the runs ended and what ended them.
bool g_Details = false;

struct Result
{
    int depth = 0;
    int gold = 0;
    int materials = 0;
    int level = 0;
    int turns = 0;
};

Result PlayRun(CrawlerGame& game, uint32_t seed)
{
    game.NewRun(seed);
    Result r;
    int floorSteps = 0;
    int lastDepth = 1;
    for (int steps = 0; steps < 6000 && !game.IsDead() && !game.IsInVillage() && !game.IsRunWon(); ++steps)
    {
        ++r.turns;
        r.depth = game.GetDepth();
        r.level = game.GetLevel();
        r.gold = game.GetGold();
        r.materials = 0;
        for (int m = 0; m < Village::kMaterials; ++m)
        {
            r.materials += game.GetMaterial(m);
        }
        if (game.GetDepth() != lastDepth)
        {
            lastDepth = game.GetDepth();
            floorSteps = 0;
        }
        // What to go for: something lying about, a thing to use, the stairs.
        Cell goal = game.GetMap().GetStairs();
        bool found = false;
        for (int i = 0; i < CrawlerGame::kMaxItems && !found && floorSteps < 250; ++i)
        {
            const Item& item = game.GetItem(i);
            if (!item.taken)
            {
                goal = item.at;
                found = true;
            }
        }
        for (int i = 0; i < CrawlerGame::kMaxProps && !found && floorSteps < 250; ++i)
        {
            const Prop& p = game.GetProp(i);
            Cell stand;
            int way = 0;
            if (p.present && game.HasDeeds(p) && p.kind != kZoneProps + 1 && StandBeside(game, p.at, stand, way))
            {
                if (game.GetHeroCell() == stand)
                {
                    // Its first choice, whatever that is.
                    Face(game, way);
                    game.Act(Action::Interact);
                    if (game.GetMenu() == Menu::Thing)
                    {
                        game.Act(Action::MenuSelect);
                    }
                    CloseAnyMenu(game);
                }
                else
                {
                    goal = stand;
                    found = true;
                }
            }
        }
        ++floorSteps;
        if (!StepToward(game, goal))
        {
            // Stuck on that one: straight to the stairs from now on.
            floorSteps = 250;
            if (!StepToward(game, game.GetMap().GetStairs()))
            {
                break;
            }
        }
    }
    return r;
}

void Report(const char* name, const Legend& legend, int runs)
{
    long depth = 0;
    long gold = 0;
    long materials = 0;
    long level = 0;
    long turns = 0;
    int reached[5] = {};  // past B2, B5, B10, B15, B20
    int deepest = 0;
    int fellOn[26] = {};  // how many runs ended on each floor
    std::map<std::string, int> killers;
    for (int i = 0; i < runs; ++i)
    {
        CrawlerGame game;
        game.SetLegend(legend);
        game.SetDeepest(99);
        const Result r = PlayRun(game, static_cast<uint32_t>(1000 + i * 7919));
        depth += r.depth;
        gold += r.gold;
        materials += r.materials;
        level += r.level;
        turns += r.turns;
        deepest = r.depth > deepest ? r.depth : deepest;
        ++fellOn[r.depth < 25 ? r.depth : 25];
        if (game.IsDead())
        {
            ++killers[game.GetMessage()];
        }
        constexpr int kMarks[5] = { 2, 5, 10, 15, 20 };
        for (int k = 0; k < 5; ++k)
        {
            reached[k] += r.depth > kMarks[k] ? 1 : 0;
        }
    }
    std::printf("%-22s depth %4.1f (best %2d) | past B2 %3d%% B5 %3d%% B10 %3d%% B15 %3d%% B20 %3d%% | gold %5.0f "
                "mats %4.1f | lvl %4.1f | turns %5.0f\n",
                name, static_cast<double>(depth) / runs, deepest, reached[0] * 100 / runs, reached[1] * 100 / runs,
                reached[2] * 100 / runs, reached[3] * 100 / runs, reached[4] * 100 / runs,
                static_cast<double>(gold) / runs, static_cast<double>(materials) / runs,
                static_cast<double>(level) / runs, static_cast<double>(turns) / runs);
    if (g_Details)
    {
        std::printf("    fell on:");
        for (int d = 1; d <= 25; ++d)
        {
            if (fellOn[d] > 0)
            {
                std::printf(" B%d:%d", d, fellOn[d]);
            }
        }
        std::printf("\n    by:");
        for (const auto& [what, count] : killers)
        {
            if (count * 20 >= runs)
            {
                std::printf(" [%s x%d]", what.c_str(), count);
            }
        }
        std::printf("\n");
    }
}

}  // namespace

int main(int argc, char** argv)
{
    const int runs = argc > 1 ? std::atoi(argv[1]) : 200;
    g_Details = argc > 2;
    Legend fresh;
    Report("nothing built", fresh, runs);
    for (int tier = 1; tier <= Village::kMaxLevel; ++tier)
    {
        Legend legend;
        for (int b = 0; b < Village::kBuildings - 1; ++b)  // not the lift: runs start at B1
        {
            legend.buildings[b] = static_cast<uint8_t>(tier);
        }
        char name[32];
        std::snprintf(name, sizeof(name), "all at level %d", tier);
        Report(name, legend, runs);
    }
    return 0;
}
