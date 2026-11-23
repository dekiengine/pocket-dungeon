// Renders a few frames of a generated floor with the raycaster and writes
// them as PPM pictures, to look at the 3D view on a PC. Not a test: a
// preview. Usage: RaycasterPreview <out folder>

#include "../src/world/DungeonMap.h"
#include "../src/render/Raycaster.h"
#include "RayStorage.h"
#include "../src/render/Textures.gen.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace Dungeon;

namespace
{

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
    DungeonMap map;
    map.Generate(1234, 1);

    const Texture stone = Textures::kWallStoneTexture;
    const Texture moss = Textures::kWallStoneTexture;
    const Texture floor = Textures::kFloorTexture;
    const Texture ceiling = Textures::kCeilingTexture;
    const Texture skeleton = Textures::kBonehoundTexture;
    const Texture slime = Textures::kCandleghoulTexture;
    const Texture potion = Textures::kPotionTexture;

    Raycaster ray;
    ray.SetStorage(RayStorage(), kRayStorageBytes);
    ray.SetSize(320, 240);
    ray.SetWallTextures(&stone, &moss);
    ray.SetFloorColours(0x0841, 0x2945, 0x0841, 0x4A49);
    ray.SetFloorTextures(&floor, &ceiling);

    // Look along the four directions from the start, with things placed in
    // the first room.
    const Cell start = map.GetStart();
    const Fix sx = (start.x << kFixShift) + kFixOne / 2;
    const Fix sy = (start.y << kFixShift) + kFixOne / 2;
    Billboard things[3];
    const Room& room = map.GetRoom(0);
    things[0] = { ((room.x + room.w - 1) << kFixShift) + kFixOne / 2, sy, &skeleton, kFixOne };
    things[1] = { sx, ((room.y + room.h - 1) << kFixShift) + kFixOne / 2, &slime, kFixOne * 3 / 4 };
    things[2] = { ((room.x) << kFixShift) + kFixOne / 2, ((room.y) << kFixShift) + kFixOne / 2, &potion, kFixOne / 2 };

    std::vector<uint16_t> frame(320 * 240);
    for (int i = 0; i < 4; ++i)
    {
        const int angle = i * kAngleSteps / 4;
        ray.Prepare(map, sx, sy, angle, things, 3);
        // In two strips, as the engine draws, to check they join.
        ray.RenderRows(frame.data(), 320 * 2, 0, 120);
        ray.RenderRows(frame.data() + 120 * 320, 320 * 2, 120, 120);
        WritePpm(out + "/view" + std::to_string(i) + ".ppm", frame, 320, 240);
    }

    // The map from above, for comparing.
    std::printf("start %d,%d\n", start.x, start.y);
    for (int y = 0; y < kMapHeight; ++y)
    {
        for (int x = 0; x < kMapWidth; ++x)
        {
            const Tile t = map.At(x, y);
            std::putchar(Cell{ x, y } == start ? '@'
                         : t == Tile::Wall     ? '#'
                         : t == Tile::Floor    ? '.'
                         : t == Tile::Stairs   ? '>'
                                               : ' ');
        }
        std::putchar('\n');
    }
    return 0;
}
