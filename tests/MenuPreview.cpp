// Renders the game's look on a PC, to check art and menus without a board:
//   cast.ppm   every monster, a few things and the frog, one per view
//   bag.ppm    the bag open over the view
//   shop.ppm   the frog's shop (the hero walks to a frog on depth 6)
//   secrets.ppm  the stairs hole, a cracked wall and a vault door
// Not a test. Usage: MenuPreview <out folder>

#include "../src/world/CrawlerGame.h"
#include "../src/screen/MenuPanel.h"
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
              kRune = TEXTURE(FloorRune), kCeiling = TEXTURE(Ceiling), kHand = TEXTURE(Hand);
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

Raycaster MakeRaycaster()
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
    ray.SetLightReach(kFixOne * 55 / 10);
    return ray;
}

// The game's view as the Crawler draws it, with its menu over it.
std::vector<uint16_t> Frame(CrawlerGame& game)
{
    Raycaster ray = MakeRaycaster();
    const CrawlerTextures textures = Textures();
    Billboard billboards[Raycaster::kMaxBillboards];
    const int count = game.GetBillboards(textures, billboards, Raycaster::kMaxBillboards);
    Fix x = 0;
    Fix y = 0;
    int angle = 0;
    game.GetView(x, y, angle);
    Overlay sword;
    sword.texture = &kHand;
    sword.scale = 2;
    sword.x = 320 - 128 + 4;
    sword.y = 240 - 128 + 36;
    ray.SetOverlay(sword);
    ray.Prepare(game.GetMap(), x, y, angle, billboards, count);
    std::vector<uint16_t> frame(320 * 240);
    ray.RenderRows(frame.data(), 640, 0, 240);
    MenuPanel panel;
    panel.Build(game, textures, 320, 240, false);
    panel.DrawRows(frame.data(), 640, 0, 240, 0, 320);
    return frame;
}

void Settle(CrawlerGame& game)
{
    for (int i = 0; i < 100 && game.IsGliding(); ++i)
    {
        game.Tick(16);
    }
}

// Walks the hero up to `target` and into it. False if it never got there.
bool WalkInto(CrawlerGame& game, Cell target)
{
    constexpr int kDx[4] = { 1, 0, -1, 0 };
    constexpr int kDy[4] = { 0, 1, 0, -1 };
    for (int steps = 0; steps < 300 && !game.IsDead(); ++steps)
    {
        DungeonMap map = game.GetMap();
        map.Set(target.x, target.y, Tile::Floor);
        map.ComputeDistances(target);
        const Cell hero = game.GetHeroCell();
        const Cell next = map.StepTowardTarget(hero);
        int want = game.GetFacing();
        for (int f = 0; f < 4; ++f)
        {
            if (hero.x + kDx[f] == next.x && hero.y + kDy[f] == next.y)
            {
                want = f;
            }
        }
        if (want != game.GetFacing())
        {
            game.Act(((want - game.GetFacing()) & 3) == 3 ? Action::TurnLeft : Action::TurnRight);
            Settle(game);
            continue;
        }
        game.Act(Action::Forward);
        Settle(game);
        if (next == target && game.GetMenu() == Menu::Shop)
        {
            return true;
        }
    }
    return false;
}

}  // namespace

int main(int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : ".";

    // The cast, each in front of the viewer down a corridor.
    {
        DungeonMap map;
        map.Generate(1234, 1);
        Cell s{};
        for (int y = 0; y < kMapHeight && s.x == 0; ++y)
        {
            for (int x = 0; x + 2 < kMapWidth; ++x)
            {
                if (map.IsWalkable({ x, y }) && map.IsWalkable({ x + 1, y }) && map.IsWalkable({ x + 2, y }))
                {
                    s = { x, y };
                    break;
                }
            }
        }
        const int count = kMonsterKinds + 2;
        const int across = 6;
        const int down = (count + across - 1) / across;
        std::vector<uint16_t> sheet(320 * across * 240 * down, 0xF81F);
        Raycaster ray = MakeRaycaster();
        std::vector<uint16_t> frame(320 * 240);
        for (int i = 0; i < count; ++i)
        {
            Billboard b;
            b.x = ((s.x + 2) << kFixShift) + kFixOne / 2 - kFixOne * 3 / 4;
            b.y = (s.y << kFixShift) + kFixOne / 2;
            b.texture = i < kMonsterKinds ? &kMonsters[i] : &kFeatures[i - kMonsterKinds];
            b.scale = i >= 13 && i < 16 ? kFixOne * 9 / 10 : kFixOne * 7 / 10;
            ray.Prepare(map, (s.x << kFixShift) + kFixOne / 2, (s.y << kFixShift) + kFixOne / 2, 0, &b, 1);
            ray.RenderRows(frame.data(), 640, 0, 240);
            const int ox = (i % across) * 320;
            const int oy = (i / across) * 240;
            for (int y = 0; y < 240; ++y)
            {
                for (int x = 0; x < 320; ++x)
                {
                    sheet[(oy + y) * 320 * across + ox + x] = frame[y * 320 + x];
                }
            }
        }
        WritePpm(out + "/cast.ppm", sheet, 320 * across, 240 * down);
    }

    // The bag.
    {
        CrawlerGame game;
        game.NewRun(5, 3);
        game.Act(Action::OpenBag);
        game.Act(Action::MenuDown);
        WritePpm(out + "/bag.ppm", Frame(game), 320, 240);
    }

    // The keys, as the very first start shows them.
    {
        CrawlerGame game;
        game.GoHome(5);
        game.ShowControls();
        WritePpm(out + "/controls.ppm", Frame(game), 320, 240);
    }

    // The shop: depth 6 always has the frog.
    for (uint32_t seed = 1; seed < 200; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 6);
        Cell frog{ -1, -1 };
        for (int i = 0; i < CrawlerGame::kMaxFeatures; ++i)
        {
            if (game.GetFeature(i).present && game.GetFeature(i).kind == FeatureKind::Merchant)
            {
                frog = game.GetFeature(i).at;
            }
        }
        if (frog.x < 0 || !WalkInto(game, frog))
        {
            continue;
        }
        WritePpm(out + "/shop.ppm", Frame(game), 320, 240);
        game.Act(Action::MenuClose);
        WritePpm(out + "/frog.ppm", Frame(game), 320, 240);
        std::printf("shop from seed %u: %s\n", seed, game.GetMessage());
        break;
    }

    // Secrets: a floor with a vault, seen from in front of its door and
    // of the stairs.
    for (uint32_t seed = 1; seed < 400; ++seed)
    {
        DungeonMap map;
        map.Generate(seed, 3);
        if (!map.GetVault().present || !map.GetSecretRoom().present)
        {
            continue;
        }
        Raycaster ray = MakeRaycaster();
        std::vector<uint16_t> sheet(320 * 3 * 240);
        std::vector<uint16_t> frame(320 * 240);
        const Cell spots[3] = { map.GetVault().entrance, map.GetSecretRoom().entrance, map.GetStairs() };
        for (int k = 0; k < 3; ++k)
        {
            // Stand two tiles from it on open floor and look at it.
            constexpr int kDx[4] = { 1, 0, -1, 0 };
            constexpr int kDy[4] = { 0, 1, 0, -1 };
            Cell from = spots[k];
            int facing = 0;
            for (int f = 0; f < 4; ++f)
            {
                const Cell a{ spots[k].x - kDx[f], spots[k].y - kDy[f] };
                const Cell b{ spots[k].x - 2 * kDx[f], spots[k].y - 2 * kDy[f] };
                if (map.IsWalkable(a) && map.IsWalkable(b))
                {
                    from = b;
                    facing = f;
                    break;
                }
            }
            ray.Prepare(map, (from.x << kFixShift) + kFixOne / 2, (from.y << kFixShift) + kFixOne / 2,
                        facing * kAngleSteps / 4, nullptr, 0);
            ray.RenderRows(frame.data(), 640, 0, 240);
            for (int y = 0; y < 240; ++y)
            {
                for (int x = 0; x < 320; ++x)
                {
                    sheet[y * 960 + k * 320 + x] = frame[y * 320 + x];
                }
            }
        }
        WritePpm(out + "/secrets.ppm", sheet, 960, 240);
        std::printf("secrets from seed %u\n", seed);
        break;
    }
    return 0;
}
