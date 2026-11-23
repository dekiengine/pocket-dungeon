// Renders an impossible door on a PC: from the front (the hall through it),
// from the side and from behind (just a frame), and a walk through it frame
// by frame. Not a test: a preview. Usage: PortalPreview <out folder>

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

#define TEXTURE(n) Textures::k##n##Texture

const Texture kStone = TEXTURE(WallStone), kCracked = TEXTURE(WallCracked), kDoor = TEXTURE(Door),
              kFloor = TEXTURE(Floor), kHole = TEXTURE(FloorHole), kSpikes = TEXTURE(FloorSpikes),
              kRune = TEXTURE(FloorRune), kCeiling = TEXTURE(Ceiling);
const Texture kMonsters[kMonsterKinds] = { TEXTURE(Droolblob),    TEXTURE(Candleghoul), TEXTURE(Eyejar),
                                           TEXTURE(Eyebat),       TEXTURE(Flamespider), TEXTURE(Bonehound),
                                           TEXTURE(Teasnail),     TEXTURE(Stewpot),     TEXTURE(Grimoire),
                                           TEXTURE(Ragmummy),     TEXTURE(Slimewizard), TEXTURE(Rockstack),
                                           TEXTURE(Glimmerjelly), TEXTURE(Mimic),       TEXTURE(Grubking),
                                           TEXTURE(Hydragon) };
const Texture kItems[kItemKinds] = { TEXTURE(Potion), TEXTURE(Gold),  TEXTURE(Sword),  TEXTURE(Shield),
                                     TEXTURE(Heart),  TEXTURE(Chest), TEXTURE(Key),    TEXTURE(Scroll),
                                     TEXTURE(Bomb),   TEXTURE(Wing),  TEXTURE(Elixir), TEXTURE(Page) };
const Texture kFeatures[kFeatureKinds] = { TEXTURE(Fountain), TEXTURE(Merchant), TEXTURE(Shrine) };

CrawlerTextures Textures()
{
    CrawlerTextures t;
    for (int i = 0; i < kMonsterKinds; ++i)
    {
        t.monsters[i] = &kMonsters[i];
    }
    for (int i = 0; i < kItemKinds; ++i)
    {
        t.items[i] = &kItems[i];
    }
    for (int i = 0; i < kFeatureKinds; ++i)
    {
        t.features[i] = &kFeatures[i];
    }
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

std::vector<uint16_t> Render(const CrawlerGame& game, Fix x, Fix y, int angle)
{
    Raycaster ray;
    ray.SetStorage(RayStorage(), kRayStorageBytes);
    ray.SetSize(320, 240);
    ray.SetWallTextures(&kStone, nullptr);
    ray.SetHideoutWalls(&kCracked, &kDoor);
    ray.SetFloorTextures(&kFloor, &kCeiling);
    ray.SetFloorLook(FloorLook::StairsHole, &kHole);
    ray.SetFloorLook(FloorLook::Spikes, &kSpikes);
    ray.SetFloorLook(FloorLook::Rune, &kRune);
    ray.SetLightReach(kFixOne * 70 / 10);
    Billboard billboards[Raycaster::kMaxBillboards];
    const int count = game.GetBillboards(Textures(), billboards, Raycaster::kMaxBillboards);
    ray.Prepare(game.GetMap(), x, y, angle, billboards, count);
    std::vector<uint16_t> frame(320 * 240);
    ray.RenderRows(frame.data(), 640, 0, 240);
    return frame;
}

Fix Centre(int v)
{
    return (v << kFixShift) + kFixOne / 2;
}

void Settle(CrawlerGame& game)
{
    for (int i = 0; i < 100 && game.IsGliding(); ++i)
    {
        game.Tick(16);
    }
}

}  // namespace

int main(int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : ".";
    for (uint32_t seed = 1; seed < 400; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed);
        const Portal in = game.GetMap().GetPortal(0);
        if (!in.present)
        {
            continue;
        }
        const Cell front = DungeonMap::Step(in.at, in.dir + 2);
        const Cell farFront = DungeonMap::Step(front, in.dir + 2);
        const Cell back = DungeonMap::Step(in.at, in.dir);
        const Cell farBack = DungeonMap::Step(back, in.dir);
        const DungeonMap& map = game.GetMap();
        if (!map.IsWalkable(farFront) || !map.IsWalkable(farBack))
        {
            continue;
        }
        const int facing = in.dir * kAngleSteps / 4;
        // Views: from the front, from the side at an angle, from behind.
        std::vector<std::vector<uint16_t>> views;
        views.push_back(Render(game, Centre(farFront.x), Centre(farFront.y), facing));
        const Cell side = DungeonMap::Step(farFront, in.dir + 1);
        if (map.IsWalkable(side))
        {
            views.push_back(Render(game, Centre(side.x), Centre(side.y), facing - kAngleSteps / 12));
        }
        views.push_back(Render(game, Centre(farBack.x), Centre(farBack.y), facing + kAngleSteps / 2));
        std::vector<uint16_t> sheet(320 * 240 * views.size());
        for (size_t v = 0; v < views.size(); ++v)
        {
            for (int y = 0; y < 240; ++y)
            {
                for (int x = 0; x < 320; ++x)
                {
                    sheet[y * 320 * views.size() + v * 320 + x] = views[v][y * 320 + x];
                }
            }
        }
        WritePpm(out + "/door_views.ppm", sheet, 320 * static_cast<int>(views.size()), 240);

        // The walk: stand two tiles in front, step up, then through.
        bool ready = false;
        for (int steps = 0; steps < 400 && !game.IsDead(); ++steps)
        {
            if (game.GetMenu() != Menu::None)
            {
                game.Act(Action::MenuClose);
                continue;
            }
            const Cell hero = game.GetHeroCell();
            if (hero == farFront && game.GetFacing() == in.dir)
            {
                ready = true;
                break;
            }
            DungeonMap path = game.GetMap();
            path.Set(in.at.x, in.at.y, Tile::Wall);
            path.ComputeDistances(farFront);
            const Cell next = hero == farFront ? hero : path.StepTowardTarget(hero);
            int want = hero == farFront ? in.dir : game.GetFacing();
            for (int f = 0; f < 4; ++f)
            {
                if (DungeonMap::Step(hero, f) == next && next != hero)
                {
                    want = f;
                }
            }
            if (want != game.GetFacing())
            {
                game.Act(((want - game.GetFacing()) & 3) == 3 ? Action::TurnLeft : Action::TurnRight);
            }
            else if (next != hero)
            {
                game.Act(Action::Forward);
            }
            else
            {
                break;
            }
            Settle(game);
        }
        if (!ready)
        {
            continue;
        }
        int frameNumber = 0;
        for (int step = 0; step < 3; ++step)
        {
            if (game.GetMenu() != Menu::None)
            {
                game.Act(Action::MenuClose);
            }
            game.Act(Action::Forward);
            for (int i = 0; i < 12; ++i)
            {
                Fix x = 0;
                Fix y = 0;
                int angle = 0;
                game.GetView(x, y, angle);
                WritePpm(out + "/walk_" + std::to_string(frameNumber++) + ".ppm", Render(game, x, y, angle), 320, 240);
                game.Tick(16);
            }
            Settle(game);
        }
        std::printf("seed %u, door at %d,%d heading %d, hall at %d,%d; hero now %d,%d\n", seed, in.at.x, in.at.y,
                    in.dir, in.to.x, in.to.y, game.GetHeroCell().x, game.GetHeroCell().y);
        return 0;
    }
    std::printf("no door found\n");
    return 1;
}
