// Renders on a PC what build 17 changed to look at: the way down as a hole
// with steps (from each side, near and far, and its question), the village's
// alley, its man and the yard of stones, a cracked wall as it breaks, and
// something shut being unwrapped. Not a test: a preview.
// Usage: TownPreview <out>

#include "Preview.h"
#include "Walk.h"

namespace
{

const Zone kVillage{
    { T(VillageWall), nullptr, nullptr, nullptr }, 1, T(VillageWall), { T(Cobble), nullptr }, nullptr, nullptr
};

std::vector<uint16_t> Shoot(const CrawlerGame& game, const Zone& zone, bool panel, Fix reach = kFixOne * 55 / 10)
{
    Fix x = 0;
    Fix y = 0;
    int angle = 0;
    game.GetView(x, y, angle);
    return Frame(game, zone, x, y, angle, panel, true, nullptr, reach);
}

}  // namespace

int main(int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : ".";
    std::vector<uint16_t> sheet(320 * 4 * 240 * 4);

    // Row 0: the way down, from two tiles off on each of its sides.
    {
        for (uint32_t seed = 1; seed < 80; ++seed)
        {
            CrawlerGame game;
            game.NewRun(seed, 2);
            const Cell stairs = game.GetMap().GetStairs();
            int shot = 0;
            for (int way = 0; way < 4; ++way)
            {
                const Cell stand{ stairs.x - kDx[way] * 2, stairs.y - kDy[way] * 2 };
                const Cell between{ stairs.x - kDx[way], stairs.y - kDy[way] };
                if (Blocked(game, stand) || Blocked(game, between) || game.MonsterAt(stand) >= 0 ||
                    !TestWalk::WalkTo(game, stand, stairs))
                {
                    continue;
                }
                TestWalk::Face(game, way);
                Put(sheet, 4, shot++, 0, Shoot(game, kZoneList[0], false));
            }
            if (shot == 4)
            {
                // Row 1: right before it, on it with its question, and the
                // same hole in the village's cobbles.
                const Cell before{ stairs.x - 1, stairs.y };
                TestWalk::WalkTo(game, before, stairs);
                TestWalk::Face(game, 0);
                Put(sheet, 4, 0, 1, Shoot(game, kZoneList[0], false));
                game.Act(Action::Forward);
                Settle(game);
                Put(sheet, 4, 1, 1, Shoot(game, kZoneList[0], true));
                break;
            }
        }
    }
    {
        g_Village = true;
        CrawlerGame game;
        Legend legend;
        legend.buildings[0] = 1;
        legend.buildings[2] = 2;
        legend.bank = 60;
        game.SetLegend(legend);
        game.GoHome(5);
        const Cell well = Village::WellCell();
        TestWalk::WalkTo(game, { well.x, well.y + 2 }, well);
        TestWalk::Face(game, 3);
        Put(sheet, 4, 2, 1, Shoot(game, kVillage, false, kFixOne * 22));
        // Up to a house's front: the storey over it stands on its wall.
        TestWalk::WalkTo(game, { 13, 7 }, well);
        TestWalk::Face(game, 3);
        Put(sheet, 4, 3, 1, Shoot(game, kVillage, false, kFixOne * 22));

        // Row 2: the alley from the square, down it at the man, up it to
        // the yard, and the yard's stones.
        TestWalk::WalkTo(game, { 9, 9 + 1 }, well);
        TestWalk::WalkTo(game, { 8, 9 }, well);
        TestWalk::Face(game, 2);
        Put(sheet, 4, 0, 2, Shoot(game, kVillage, false, kFixOne * 22));
        TestWalk::WalkTo(game, { 4, 9 }, well);
        TestWalk::Face(game, 2);
        Put(sheet, 4, 1, 2, Shoot(game, kVillage, false, kFixOne * 22));
        TestWalk::WalkTo(game, { 3, 7 }, well);
        TestWalk::Face(game, 3);
        Put(sheet, 4, 2, 2, Shoot(game, kVillage, false, kFixOne * 22));
        TestWalk::WalkTo(game, { 3, 3 }, well);
        TestWalk::Face(game, 0);
        Put(sheet, 4, 3, 2, Shoot(game, kVillage, false, kFixOne * 22));
        g_Village = false;
    }

    // Row 3: a chest opened, and what was in it tossed up in the view: four
    // moments of it.
    {
        const CrawlerTextures textures = MakeTextures();
        constexpr int kOldChest = kZones * kZoneProps + 6;
        for (uint32_t seed = 1; seed < 300; ++seed)
        {
            CrawlerGame game;
            game.NewRun(seed, 2);
            int chest = -1;
            for (int i = 0; i < CrawlerGame::kMaxProps; ++i)
            {
                const Prop& prop = game.GetProp(i);
                chest = prop.present && game.PropIndex(prop) == kOldChest ? i : chest;
            }
            Cell stand;
            int way = 0;
            if (chest < 0 || !TestWalk::StandBeside(game, game.GetProp(chest).at, stand, way) ||
                !TestWalk::WalkTo(game, stand))
            {
                continue;
            }
            TestWalk::Face(game, way);
            ViewEffects effects;
            effects.Reset(game);
            game.Act(Action::Interact);
            if (game.GetMenu() != Menu::Thing)
            {
                continue;
            }
            game.Act(Action::MenuSelect);
            if (game.GetResultItem() < 0 || game.GetResultItem() == static_cast<int>(ItemKind::Gold))
            {
                continue;
            }
            Fix x = 0;
            Fix y = 0;
            int angle = 0;
            game.GetView(x, y, angle);
            const int steps[4] = { 60, 140, 260, 600 };
            for (int i = 0; i < 4; ++i)
            {
                effects.Update(game, textures, 320, 240, steps[i]);
                Put(sheet, 4, i, 3, Frame(game, kZoneList[0], x, y, angle, false, true, &effects));
            }
            break;
        }
    }
    WritePpm(out + "/town.ppm", sheet, 320 * 4, 240 * 4);
    return 0;
}
