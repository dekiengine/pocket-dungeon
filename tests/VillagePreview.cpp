// Renders the village on a PC: from the start, turned each way, and up
// close to a front, with some buildings built and some ruins. Not a test: a
// preview. Usage: VillagePreview <out>

#include "Preview.h"
#include "Walk.h"

namespace
{

const Zone kVillage{
    { T(VillageWall), nullptr, nullptr, nullptr }, 1, T(VillageWall), { T(Cobble), nullptr }, nullptr, nullptr
};

}  // namespace

int main(int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : ".";
    g_Village = true;
    CrawlerGame game;
    Legend legend;
    legend.buildings[0] = 1;
    legend.buildings[2] = 2;
    legend.buildings[3] = 1;
    legend.buildings[5] = 1;
    game.SetLegend(legend);
    game.GoHome(5);
    std::vector<uint16_t> sheet(320 * 4 * 240 * 2);
    const auto shoot = [&](int column, int row)
    {
        Fix x = 0;
        Fix y = 0;
        int angle = 0;
        game.GetView(x, y, angle);
        Put(sheet, 4, column, row, Frame(game, kVillage, x, y, angle, false, true, nullptr, kFixOne * 22));
    };
    // From the start, each way.
    for (int turn = 0; turn < 4; ++turn)
    {
        shoot(turn, 0);
        game.Act(Action::TurnRight);
        Settle(game);
    }
    // Up to the tavern's front, then the blacksmith's, then along the wall.
    for (int i = 0; i < 2; ++i)
    {
        const int building = i == 0 ? 2 : 0;
        TestWalk::WalkTo(game, Village::OutsideCell(building), Village::WellCell());
        TestWalk::Face(game, (Village::OutsideFacing(building) + 2) & 3);
        shoot(i * 2, 1);
        game.Act(Action::TurnRight);
        Settle(game);
        shoot(i * 2 + 1, 1);
    }
    WritePpm(out + "/village.ppm", sheet, 320 * 4, 240 * 2);
    return 0;
}
