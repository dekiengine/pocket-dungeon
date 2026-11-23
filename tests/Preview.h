#pragma once

// What the previews share: the game's textures as the Crawler hands them
// out, the zones' walls and floors, a frame of the 3D view, walking about,
// and writing pictures. One preview per program: all in a nameless
// namespace.

#include "../src/world/CrawlerGame.h"
#include "../src/screen/ViewEffects.h"
#include "../src/screen/MenuPanel.h"
#include "../src/render/Raycaster.h"
#include "RayStorage.h"
#include "../src/render/SmoothText.h"
#include "../src/render/Textures.gen.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace Dungeon;

namespace
{

#define T(n) &Textures::k##n##Texture

constexpr int kDx[4] = { 1, 0, -1, 0 };
constexpr int kDy[4] = { 0, 1, 0, -1 };

struct Zone
{
    const Texture* walls[4];
    int wallCount;
    const Texture* cracked;
    const Texture* floors[2];
    const Texture* ceiling;
    const Texture* echo;
};
const Zone kZoneList[kZones] = {
    { { T(WallStone), nullptr, nullptr, nullptr },
      1,
      T(WallCracked),
      { T(Floor), nullptr },
      T(Ceiling),
      T(WallEchoStone) },
    { { T(WallBone), T(WallBone2), nullptr, nullptr },
      2,
      T(CrackedBone),
      { T(FloorBone), nullptr },
      T(CeilingBone),
      T(WallEchoBone) },
    { { T(WallWet), T(WallWet2), nullptr, nullptr },
      2,
      T(CrackedWet),
      { T(FloorWet), nullptr },
      T(CeilingWet),
      T(WallEchoWet) },
    { { T(WallForge), T(WallForge2), nullptr, nullptr },
      2,
      T(CrackedForge),
      { T(FloorForge), nullptr },
      T(CeilingForge),
      T(WallEchoForge) },
};

CrawlerTextures MakeTextures()
{
    CrawlerTextures t;
    const Texture* monsters[kMonsterKinds] = { T(Droolblob),    T(Candleghoul), T(Eyejar),      T(Eyebat),
                                               T(Flamespider),  T(Bonehound),   T(Teasnail),    T(Stewpot),
                                               T(Grimoire),     T(Ragmummy),    T(Slimewizard), T(Rockstack),
                                               T(Glimmerjelly), T(Mimic),       T(Grubking),    T(Hydragon),
                                               T(Keeper),       T(Ghost) };
    const Texture* items[kItemKinds] = { T(Potion), T(Gold),    T(Sword),  T(Shield), T(Heart), T(Chest), T(Key),
                                         T(Scroll), T(Bomb),    T(Wing),   T(Elixir), T(Page),  T(Blade), T(Axe),
                                         T(Spear),  T(Dagger),  T(Hammer), T(Shard),  T(Duck),  T(Snack), T(Lockpick),
                                         T(Gem),    T(Goblet),  T(Note),   T(Moss),   T(Bone),  T(Shell), T(Ore),
                                         T(Water),  T(Glowcap), T(Fish),   T(Pearl),  T(Vial) };
    const Texture* props[kPropKinds] = { T(PropPack),      T(PropBarrel),    T(PropBush),        T(PropStatue),
                                         T(PropSkulls),    T(PropCandles),   T(PropCoffin),      T(PropGrave),
                                         T(PropCrystal),   T(PropMushrooms), T(PropClam),        T(PropBasket),
                                         T(PropAnvil),     T(PropBrazier),   T(PropRack),        T(PropCart),
                                         T(PropBookshelf), T(PropCamp),      T(PropLockedChest), T(PropPot),
                                         T(PropCrate),     T(PropCage),      T(PropOldChest),    T(PropMirror) };
    for (int i = 0; i < kMonsterKinds; ++i)
    {
        t.monsters[i] = monsters[i];
    }
    for (int i = 0; i < kItemKinds; ++i)
    {
        t.items[i] = items[i];
    }
    for (int i = 0; i < kPropKinds; ++i)
    {
        t.props[i] = props[i];
    }
    t.pots[0] = T(PropPotUrn);
    t.pots[1] = T(PropPotJar);
    t.spent[2] = T(PropBushUsed);
    t.spent[3] = T(PropStatueUsed);
    t.spent[5] = T(PropCandlesUsed);
    t.spent[6] = T(PropCoffinUsed);
    t.spent[8] = T(PropCrystalUsed);
    t.spent[9] = T(PropMushroomsUsed);
    t.spent[10] = T(PropClamUsed);
    t.spent[11] = T(PropBasketUsed);
    t.spent[14] = T(PropRackUsed);
    t.spent[17] = T(PropCampUsed);
    t.spent[21] = T(PropCageUsed);
    t.spent[18] = T(PropChestOpen);
    t.spent[22] = T(PropChestOpen);
    t.fountainDry = T(FountainDry);
    t.features[0] = T(Fountain);
    t.features[1] = T(Merchant);
    t.features[2] = T(Shrine);
    t.features[3] = T(PortalHome);
    t.features[4] = T(Granny);
    t.features[5] = T(Kid);
    t.features[7] = T(Fence);
    t.features[8] = T(Board);
    const Texture* keepers[Village::kBuildings] = { T(KeeperSmith),  T(KeeperHerbs), T(KeeperBar),
                                                    T(KeeperWizard), T(KeeperBank),  nullptr };
    for (int i = 0; i < Village::kBuildings; ++i)
    {
        t.keepers[i] = keepers[i];
    }
    t.torch[0] = T(TorchA);
    t.torch[1] = T(TorchB);
    // Things drawn from eight sides, as the Crawler hands them out.
#define VIEWS(n) T(n), T(n##View1), T(n##View2), T(n##View3), T(n##View4), T(n##View5), T(n##View6), T(n##View7)
    const Texture* torch[8] = { T(TorchA),      T(TorchAView1), T(TorchAView2), T(TorchAView3),
                                T(TorchAView4), T(TorchAView5), T(TorchAView6), T(TorchAView7) };
    for (int i = 0; i < 8; ++i)
    {
        t.torchViews[i] = torch[i];
    }
    const Texture* spells[kSpells] = { T(SpellFire), T(SpellHeal), T(SpellFrost), T(SpellBlink), T(SpellFold) };
    for (int i = 0; i < kSpells; ++i)
    {
        t.spells[i] = spells[i];
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

void Settle(CrawlerGame& game)
{
    for (int i = 0; i < 100 && game.IsGliding(); ++i)
    {
        game.Tick(16);
    }
}

bool Blocked(const CrawlerGame& game, Cell c)
{
    return !game.GetMap().IsWalkable(c) || game.FeatureAt(c) >= 0 || game.PropAt(c) >= 0;
}

// Walks next to `target` and into it (bumps it). False if it never got there.
[[maybe_unused]] bool WalkInto(CrawlerGame& game, Cell target)
{
    for (int steps = 0; steps < 400 && !game.IsDead(); ++steps)
    {
        if (game.GetMenu() != Menu::None)
        {
            game.Act(Action::MenuClose);
        }
        DungeonMap map = game.GetMap();
        map.Set(target.x, target.y, Tile::Floor);
        map.ComputeDistances(target);
        const Cell hero = game.GetHeroCell();
        Cell next = hero;
        uint16_t best = map.DistanceAt(hero);
        int want = -1;
        for (int f = 0; f < 4; ++f)
        {
            const Cell n{ hero.x + kDx[f], hero.y + kDy[f] };
            const uint16_t d = map.DistanceAt(n);
            if (d != DungeonMap::kUnreached && d < best && (n == target || !Blocked(game, n)))
            {
                best = d;
                next = n;
                want = f;
            }
        }
        if (want < 0)
        {
            return false;
        }
        while (game.GetFacing() != want)
        {
            game.Act(Action::TurnRight);
            Settle(game);
        }
        game.Act(Action::Forward);
        Settle(game);
        if (next == target)
        {
            return true;
        }
    }
    return false;
}

// The view's pixel size, as the Crawler sets it.
inline int g_PixelSize = 2;
// Out in the village: the sky, the dusk and each building's front, as the
// Crawler sets them for it.
inline bool g_Village = false;
// What Frame leaves out, for timing each part (BenchPreview).
inline bool g_NoShadows = false;
inline bool g_NoLights = false;

std::vector<uint16_t> Frame(const CrawlerGame& game, const Zone& z, Fix x, Fix y, int angle, bool withPanel,
                            bool withBillboards = true, ViewEffects* effects = nullptr, Fix reach = kFixOne * 55 / 10)
{
    Raycaster ray;
    ray.SetStorage(RayStorage(), kRayStorageBytes);
    ray.SetPixelSize(g_PixelSize);
    ray.SetSize(320, 240);
    ray.SetWalls(z.walls, z.wallCount);
    ray.SetHideoutWalls(z.cracked, T(Door), z.echo);
    ray.SetFloors(z.floors, 2, z.ceiling);
    static uint8_t cells[kMapWidth * kMapHeight];
    static const Texture* const fronts[Village::kBuildings + 1] = { T(VBlacksmith), T(VApothecary), T(VTavern),
                                                                    T(VTower),      T(VBank),       T(VLift),
                                                                    T(Ruin) };
    if (g_Village)
    {
        for (int b = 0; b < Village::kBuildings; ++b)
        {
            const Cell c = Village::BuildingCell(b);
            cells[c.y * kMapWidth + c.x] =
                static_cast<uint8_t>(game.GetLegend().buildings[b] > 0 ? b + 1 : Village::kBuildings + 1);
        }
        ray.SetCellWalls(cells, fronts, Village::kBuildings + 1);
        static const Texture* const tops[2] = { T(VTopA), T(VTopB) };
        static const Texture* const frontTops[Village::kBuildings + 1] = { nullptr, nullptr, nullptr,    T(VTopTower),
                                                                           nullptr, nullptr, T(VTopRuin) };
        ray.SetWallTops(tops, 2, frontTops);
        ray.SetSky(T(Sky));
        // The alley between the houses, in their shadow.
        static uint8_t gloom[kMapWidth * kMapHeight];
        for (int ty = 0; ty < kMapHeight; ++ty)
        {
            for (int tx = 0; tx < kMapWidth; ++tx)
            {
                gloom[ty * kMapWidth + tx] = static_cast<uint8_t>(Village::Gloom({ tx, ty }));
            }
        }
        ray.SetGloom(gloom);
        ray.SetCanopy(T(Canopy));
        ray.SetFloorColours(SmoothText::Rgb(206, 122, 98), SmoothText::Rgb(24, 26, 62), 0x0000, 0x4208);
    }
    ray.SetFloorLook(FloorLook::StairsHole, T(FloorHole));
    ray.SetFloorLook(FloorLook::Spikes, T(FloorSpikes));
    ray.SetFloorLook(FloorLook::Rune, T(FloorRune));
    ray.SetLightReach(reach);
    // The torches light what is round them, as in the Crawler.
    Fix lightX[CrawlerGame::kMaxTorches];
    Fix lightY[CrawlerGame::kMaxTorches];
    int lights = 0;
    for (int i = 0; i < CrawlerGame::kMaxTorches; ++i)
    {
        if (game.GetTorch(i).present)
        {
            lightX[lights] = game.GetTorch(i).x;
            lightY[lights] = game.GetTorch(i).y;
            ++lights;
        }
    }
    ray.SetLights(lightX, lightY, g_NoLights ? 0 : lights, kFixOne * 5 / 2);
    Billboard billboards[Raycaster::kMaxBillboards];
    const CrawlerTextures textures = MakeTextures();
    int count = withBillboards ? game.GetBillboards(textures, billboards, Raycaster::kMaxBillboards, x, y) : 0;
    if (effects)
    {
        count = effects->AddBillboards(billboards, count, Raycaster::kMaxBillboards);
        ray.SetHorizonOffset(effects->Kick());
    }
    Raycaster::Shadow shadows[Raycaster::kMaxShadows];
    ray.SetShadows(shadows, withBillboards && !g_NoShadows ? game.GetShadows(shadows, Raycaster::kMaxShadows) : 0);
    ray.Prepare(game.GetMap(), x, y, angle, billboards, count);
    std::vector<uint16_t> frame(320 * 240);
    ray.RenderRows(frame.data(), 640, 0, 240);
    if (effects)
    {
        effects->Place(ray);
        effects->Draw(frame.data(), 320, 0, 240, 0, 320);
    }
    if (withPanel)
    {
        MenuPanel panel;
        panel.Build(game, textures, 320, 240, false);
        panel.DrawRows(frame.data(), 640, 0, 240, 0, 320);
    }
    return frame;
}

void Put(std::vector<uint16_t>& sheet, int columns, int column, int row, const std::vector<uint16_t>& frame)
{
    for (int r = 0; r < 240; ++r)
    {
        for (int c = 0; c < 320; ++c)
        {
            sheet[(row * 240 + r) * 320 * columns + column * 320 + c] = frame[r * 320 + c];
        }
    }
}

}  // namespace
