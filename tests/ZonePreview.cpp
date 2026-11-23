// Renders a floor of each zone from a few places, side by side, with its wall
// variants, floors, props, torches and their light, and the flooded caves'
// reflections: the zones' looks on a PC. Not a test: a preview.
// Usage: ZonePreview <out>

#include "../src/world/CrawlerGame.h"
#include "../src/render/Raycaster.h"
#include "RayStorage.h"
#include "../src/render/Textures.gen.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace Dungeon;

namespace
{

#define T(n) &Textures::k##n##Texture

struct Zone
{
    const Texture* walls[4];
    int wallCount;
    const Texture* cracked;
    const Texture* floors[2];
    const Texture* ceiling;
};
const Zone kZoneList[kZones] = {
    { { T(WallStone), nullptr, nullptr, nullptr }, 1, T(WallCracked), { T(Floor), nullptr }, T(Ceiling) },
    { { T(WallBone), T(WallBone2), nullptr, nullptr }, 2, T(CrackedBone), { T(FloorBone), nullptr }, T(CeilingBone) },
    { { T(WallWet), T(WallWet2), nullptr, nullptr }, 2, T(CrackedWet), { T(FloorWet), nullptr }, T(CeilingWet) },
    { { T(WallForge), T(WallForge2), nullptr, nullptr },
      2,
      T(CrackedForge),
      { T(FloorForge), nullptr },
      T(CeilingForge) },
};

CrawlerTextures MakeTextures()
{
    CrawlerTextures t;
    const Texture* props[kZones * kZoneProps] = { T(PropPack),    T(PropBarrel),    T(PropBush),   T(PropStatue),
                                                  T(PropSkulls),  T(PropCandles),   T(PropCoffin), T(PropGrave),
                                                  T(PropCrystal), T(PropMushrooms), T(PropClam),   T(PropBasket),
                                                  T(PropAnvil),   T(PropBrazier),   T(PropRack),   T(PropCart) };
    for (int i = 0; i < kZones * kZoneProps; ++i)
    {
        t.props[i] = props[i];
    }
    t.torch[0] = T(TorchA);
    t.torch[1] = T(TorchB);
    return t;
}

void WritePpm(const std::string& path, const std::vector<uint16_t>& pixels, int width, int height)
{
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f)
    {
        return;
    }
    std::fprintf(f, "P6\n%d %d\n255\n", width, height);
    for (uint16_t p : pixels)
    {
        const unsigned char rgb[3] = { static_cast<unsigned char>(((p >> 11) & 31) * 255 / 31),
                                       static_cast<unsigned char>(((p >> 5) & 63) * 255 / 63),
                                       static_cast<unsigned char>((p & 31) * 255 / 31) };
        std::fwrite(rgb, 1, 3, f);
    }
    std::fclose(f);
}

}  // namespace

int main(int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : ".";
    constexpr int kViews = 3;
    std::vector<uint16_t> sheet(320 * kViews * 240 * kZones);
    for (int zone = 0; zone < kZones; ++zone)
    {
        CrawlerGame game;
        game.NewRun(21, zone * kZoneFloors + 2);
        const Zone& z = kZoneList[zone];
        Raycaster ray;
        ray.SetStorage(RayStorage(), kRayStorageBytes);
        ray.SetSize(320, 240);
        ray.SetWalls(z.walls, z.wallCount);
        ray.SetHideoutWalls(z.cracked, T(Door));
        ray.SetFloors(z.floors, 2, z.ceiling);
        ray.SetFloorLook(FloorLook::StairsHole, T(FloorHole));
        ray.SetFloorLook(FloorLook::Spikes, T(FloorSpikes));
        ray.SetFloorLook(FloorLook::Rune, T(FloorRune));
        ray.SetLightReach(kFixOne * 55 / 10);
        ray.SetReflectiveFloor(zone == 2);
        Fix lx[CrawlerGame::kMaxTorches];
        Fix ly[CrawlerGame::kMaxTorches];
        int lights = 0;
        for (int i = 0; i < CrawlerGame::kMaxTorches; ++i)
        {
            if (game.GetTorch(i).present)
            {
                lx[lights] = game.GetTorch(i).x;
                ly[lights] = game.GetTorch(i).y;
                ++lights;
            }
        }
        ray.SetLights(lx, ly, lights, kFixOne * 2);
        Billboard billboards[Raycaster::kMaxBillboards];
        const int count = game.GetBillboards(MakeTextures(), billboards, Raycaster::kMaxBillboards);
        // From the start, looking each way the start opens to, and from the
        // first torch's room looking at it.
        const Cell start = game.GetHeroCell();
        int view = 0;
        for (int f = 0; f < 4 && view < kViews; ++f)
        {
            if (!game.GetMap().IsWalkable(DungeonMap::Step(start, f)))
            {
                continue;
            }
            ray.Prepare(game.GetMap(), (start.x << kFixShift) + kFixOne / 2, (start.y << kFixShift) + kFixOne / 2,
                        f * kAngleSteps / 4, billboards, count);
            std::vector<uint16_t> frame(320 * 240);
            ray.RenderRows(frame.data(), 640, 0, 240);
            for (int r = 0; r < 240; ++r)
            {
                for (int c = 0; c < 320; ++c)
                {
                    sheet[(zone * 240 + r) * 320 * kViews + view * 320 + c] = frame[r * 320 + c];
                }
            }
            ++view;
        }
        std::printf("%s: %d torches\n", CrawlerGame::ZoneName(zone), lights);
    }
    WritePpm(out + "/zones.ppm", sheet, 320 * kViews, 240 * kZones);
    return 0;
}
