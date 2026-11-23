// Times the parts of a frame on a PC: the same views drawn whole, then
// without billboards, the shadows and the torch light, so the costly parts
// show. Only the share of each matters: a PC is not a Pico.
// Not a test. Usage: BenchPreview

#include "Preview.h"

#include <chrono>

namespace
{

struct View
{
    CrawlerGame game;
    Fix x = 0;
    Fix y = 0;
    int angle = 0;
};

double Time(std::vector<View>& views, bool billboards)
{
    const auto from = std::chrono::steady_clock::now();
    int frames = 0;
    for (int round = 0; round < 6; ++round)
    {
        for (View& v : views)
        {
            const std::vector<uint16_t> frame =
                Frame(v.game, kZoneList[v.game.GetZone()], v.x, v.y, v.angle, false, billboards);
            frames += frame[0] == 1 ? 2 : 1;
        }
    }
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - from).count();
    return ms / frames;
}

}  // namespace

int main()
{
    // Views across the zones, each turned four ways, standing still.
    std::vector<View> views;
    for (int depth : { 2, 4, 7, 9, 12, 14, 17, 19 })
    {
        for (int facing = 0; facing < 4; ++facing)
        {
            View v;
            v.game.NewRun(static_cast<uint32_t>(depth * 31 + 7), depth);
            for (int t = 0; t < facing; ++t)
            {
                v.game.Act(Action::TurnRight);
                Settle(v.game);
            }
            v.game.GetView(v.x, v.y, v.angle);
            views.push_back(std::move(v));
        }
    }
    const double whole = Time(views, true);
    g_NoShadows = true;
    const double noShadows = Time(views, true);
    g_NoShadows = false;
    g_NoLights = true;
    const double noLights = Time(views, true);
    g_NoLights = false;
    const double noBillboards = Time(views, false);
    g_NoShadows = g_NoLights = true;
    const double bare = Time(views, false);
    g_NoShadows = g_NoLights = false;
    g_PixelSize = 1;
    const double full = Time(views, true);
    g_PixelSize = 2;
    std::printf("ms per frame: whole %.3f | no shadows %.3f | no torch light %.3f | no billboards %.3f | walls and "
                "floors only %.3f | whole at full size %.3f\n",
                whole, noShadows, noLights, noBillboards, bare, full);
    return 0;
}
