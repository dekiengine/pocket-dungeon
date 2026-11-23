// Renders torchlight on a PC: views of dungeon floors looking at their wall
// torches from a few tiles away, where the pools of light on the floor and
// the ceiling show. Not a test: a preview. Usage: LightPreview <out>

#include "Preview.h"
#include "Walk.h"

#include <cstdlib>

int main(int argc, char** argv)
{
    // The hero's own light, as the Crawler sets it (lightReach 55).
    const Fix g_Reach = (55 * kFixOne) / 100 * (argc > 2 ? std::atoi(argv[2]) : 9);
    const std::string out = argc > 1 ? argv[1] : ".";
    constexpr int kColumns = 4;
    std::vector<uint16_t> sheet(320 * kColumns * 240 * 2);
    int cell = 0;
    for (int depth : { 2, 7, 12, 17 })
    {
        int shown = 0;
        for (uint32_t seed = 3; seed < 40 && shown < 1; ++seed)
        {
            CrawlerGame game;
            game.NewRun(seed, depth);
            for (int i = 0; i < CrawlerGame::kMaxTorches && shown < 2; ++i)
            {
                const Torch& t = game.GetTorch(i);
                if (!t.present)
                {
                    continue;
                }
                // Two tiles out from the torch's wall.
                const Cell at{ t.x >> kFixShift, t.y >> kFixShift };
                const int f = t.facing;
                const Cell stand{ at.x + TestWalk::kDx[f] * 2, at.y + TestWalk::kDy[f] * 2 };
                if (!game.GetMap().IsWalkable(stand) || !TestWalk::WalkTo(game, stand))
                {
                    continue;
                }
                // Once looking back at it, once along its wall.
                for (int turn = 0; turn < 2; ++turn)
                {
                    TestWalk::Face(game, (f + 2 + turn) & 3);
                    Fix x = 0;
                    Fix y = 0;
                    int angle = 0;
                    game.GetView(x, y, angle);
                    Put(sheet, kColumns, cell % kColumns, cell / kColumns,
                        Frame(game, kZoneList[game.GetZone()], x, y, angle, false, true, nullptr, g_Reach));
                    ++cell;
                }
                ++shown;
                break;
            }
        }
    }
    WritePpm(out + "/light.ppm", sheet, 320 * kColumns, 240 * 2);
    return 0;
}
