// Renders the inside of each building on a PC: the view on walking in (the
// keeper at the back) and the view back at the door, all on one sheet. Not a
// test: a preview. Usage: InteriorPreview <out>

#include "Preview.h"
#include "Walk.h"

namespace
{

// Inside a building, as the Crawler draws it.
const Zone kInterior{
    { T(InWall), nullptr, nullptr, nullptr }, 1, T(InWall), { T(InFloor), nullptr }, T(InCeiling), nullptr
};

}  // namespace

int main(int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : ".";
    constexpr int kEnterable = Village::kBuildings - 1;  // the lift is ridden
    std::vector<uint16_t> sheet(320 * kEnterable * 240 * 2);
    for (int b = 0; b < kEnterable; ++b)
    {
        CrawlerGame game;
        Legend legend;
        legend.buildings[b] = 1;
        game.SetLegend(legend);
        game.GoHome(5);
        // Straight in, as walking into its front does.
        const Cell stand = Village::OutsideCell(b);
        if (!TestWalk::WalkTo(game, stand, Village::WellCell()))
        {
            std::printf("could not reach building %d\n", b);
            return 1;
        }
        while (game.GetFacing() != (Village::OutsideFacing(b) + 2) % 4)
        {
            game.Act(Action::TurnRight);
            Settle(game);
        }
        game.Act(Action::Interact);
        Settle(game);
        if (game.GetInside() != b)
        {
            std::printf("building %d did not open\n", b);
            return 1;
        }
        Fix x = 0;
        Fix y = 0;
        int angle = 0;
        game.GetView(x, y, angle);
        Put(sheet, kEnterable, b, 0, Frame(game, kInterior, x, y, angle, false, true, nullptr, kFixOne * 715 / 100));
        // A step in, looking back at the door.
        game.Act(Action::Forward);
        Settle(game);
        game.Act(Action::TurnRight);
        Settle(game);
        game.Act(Action::TurnRight);
        Settle(game);
        game.GetView(x, y, angle);
        Put(sheet, kEnterable, b, 1, Frame(game, kInterior, x, y, angle, false, true, nullptr, kFixOne * 715 / 100));
    }
    WritePpm(out + "/interiors.ppm", sheet, 320 * kEnterable, 240 * 2);
    std::printf("wrote %s/interiors.ppm\n", out.c_str());
    return 0;
}
