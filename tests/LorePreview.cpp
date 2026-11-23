// Renders the story's pieces on a PC: each zone's echo stone in a wall (top
// row), then an echo being heard and a word being typed to a frog (bottom
// row). Not a test: a preview. Usage: LorePreview <out>

#include "../src/screen/Transitions.h"
#include "Preview.h"

int main(int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : ".";
    constexpr int kColumns = 4;
    std::vector<uint16_t> sheet(320 * kColumns * 240 * 2);

    // Each zone's stone, seen from two tiles away.
    for (int zone = 0; zone < kZones; ++zone)
    {
        for (uint32_t seed = 1; seed < 60; ++seed)
        {
            CrawlerGame game;
            Legend legend;
            legend.echoes = static_cast<uint16_t>((1u << (zone * Lore::kEchoesPerZone)) - 1);
            game.SetLegend(legend);
            game.NewRun(seed, zone * kZoneFloors + 2);
            bool done = false;
            for (int y = 1; y < kMapHeight - 1 && !done; ++y)
            {
                for (int x = 1; x < kMapWidth - 1 && !done; ++x)
                {
                    if (game.GetMap().At(x, y) != Tile::EchoWall)
                    {
                        continue;
                    }
                    for (int f = 0; f < 4 && !done; ++f)
                    {
                        const Cell near{ x - kDx[f], y - kDy[f] };
                        const Cell far{ x - 2 * kDx[f], y - 2 * kDy[f] };
                        if (!game.GetMap().IsWalkable(near) || !game.GetMap().IsWalkable(far))
                        {
                            continue;
                        }
                        Put(sheet, kColumns, zone, 0,
                            Frame(game, kZoneList[zone], (far.x << kFixShift) + kFixOne / 2,
                                  (far.y << kFixShift) + kFixOne / 2, f * kAngleSteps / 4, false));
                        done = true;
                    }
                }
            }
            if (done)
            {
                std::printf("zone %d: stone on seed %u\n", zone, seed);
                break;
            }
        }
    }

    // An echo heard: walk into the first floor's stone.
    for (uint32_t seed = 1; seed < 40; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed);
        Cell stone{ -1, -1 };
        for (int y = 0; y < kMapHeight; ++y)
        {
            for (int x = 0; x < kMapWidth; ++x)
            {
                if (game.GetMap().At(x, y) == Tile::EchoWall)
                {
                    stone = { x, y };
                }
            }
        }
        if (stone.x < 0 || !WalkInto(game, stone) || game.GetMenu() != Menu::Echo)
        {
            continue;
        }
        Fix x = 0;
        Fix y = 0;
        int angle = 0;
        game.GetView(x, y, angle);
        // Heard on the dark: all its lines up.
        std::vector<uint16_t> frame = Frame(game, kZoneList[0], x, y, angle, false);
        Transitions::DrawEcho(frame.data(), 320, 0, 240, 0, 320, 320, 240, Lore::GetEcho(game.GetEchoShown()), false,
                              Transitions::EchoShownMs(false));
        Put(sheet, kColumns, 0, 1, frame);
        std::printf("echo heard on seed %u\n", seed);
        break;
    }

    // A word for a frog.
    for (uint32_t seed = 1; seed < 400; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 4);
        int shrine = -1;
        for (int i = 0; i < CrawlerGame::kMaxFeatures; ++i)
        {
            if (game.GetFeature(i).present && game.GetFeature(i).kind == FeatureKind::Shrine)
            {
                shrine = i;
            }
        }
        if (shrine < 0 || !WalkInto(game, game.GetFeature(shrine).at) || game.GetMenu() != Menu::Thing)
        {
            continue;
        }
        Fix x = 0;
        Fix y = 0;
        int angle = 0;
        game.GetView(x, y, angle);
        Put(sheet, kColumns, 1, 1, Frame(game, kZoneList[0], x, y, angle, true));
        // And, from the same place, what it gives for a name: a snack.
        game.Act(Action::MenuDown);
        game.Act(Action::MenuSelect);
        game.Act(Action::MenuClose);
        game.Act(Action::OpenBag);
        Put(sheet, kColumns, 2, 1, Frame(game, kZoneList[0], x, y, angle, true));
        std::printf("frog on seed %u: %s\n", seed, game.GetMessage());
        break;
    }

    // The ghost of the last run.
    {
        CrawlerGame game;
        Legend legend;
        legend.ghostDepth = 2;
        legend.ghostGold = 50;
        game.SetLegend(legend);
        game.NewRun(5, 2);
        for (int i = 0; i < game.GetMonsterCount(); ++i)
        {
            const Monster& m = game.GetMonster(i);
            if (!m.alive || m.kind != MonsterKind::Ghost)
            {
                continue;
            }
            for (int f = 0; f < 4; ++f)
            {
                const Cell c{ m.at.x - 2 * kDx[f], m.at.y - 2 * kDy[f] };
                if (game.GetMap().IsWalkable(c) && game.GetMap().IsWalkable({ m.at.x - kDx[f], m.at.y - kDy[f] }))
                {
                    Put(sheet, kColumns, 3, 1,
                        Frame(game, kZoneList[0], (c.x << kFixShift) + kFixOne / 2, (c.y << kFixShift) + kFixOne / 2,
                              f * kAngleSteps / 4, false));
                    break;
                }
            }
        }
    }
    // A bedroll from four sides, a tile and a half away, and a torch.
    for (uint32_t seed = 1; seed < 400; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2);
        int found = -1;
        for (int i = 0; i < CrawlerGame::kMaxProps; ++i)
        {
            if (game.GetProp(i).present && game.GetProp(i).kind == kZoneProps + 1)
            {
                found = i;
            }
        }
        if (found < 0)
        {
            continue;
        }
        const Prop& p = game.GetProp(found);
        std::vector<uint16_t> strip(320 * 4 * 240);
        const int kAngles[4][2] = { { 3, 0 }, { 2, 2 }, { 0, 3 }, { -2, 2 } };  // in units of half a tile
        for (int v = 0; v < 4; ++v)
        {
            // Round the bedroll from its front, in its own frame.
            const int f = p.facing;
            const Fix ax = (kDx[f] * kAngles[v][0] - kDy[f] * kAngles[v][1]) * kFixOne / 2;
            const Fix ay = (kDy[f] * kAngles[v][0] + kDx[f] * kAngles[v][1]) * kFixOne / 2;
            const Fix px = (p.at.x << kFixShift) + kFixOne / 2;
            const Fix py = (p.at.y << kFixShift) + kFixOne / 2;
            // Looking back at it.
            const double angle = std::atan2(-static_cast<double>(ay), -static_cast<double>(ax));
            int steps = static_cast<int>(std::lround(angle / 6.283185307179586 * kAngleSteps)) & (kAngleSteps - 1);
            const std::vector<uint16_t> frame = Frame(game, kZoneList[0], px + ax, py + ay, steps, false);
            for (int r = 0; r < 240; ++r)
            {
                for (int c = 0; c < 320; ++c)
                {
                    strip[r * 1280 + v * 320 + c] = frame[r * 320 + c];
                }
            }
        }
        WritePpm(out + "/ground.ppm", strip, 1280, 240);
        break;
    }
    // Every prop, from its front and from its front right, a tile and a half
    // away: two views each, four props a row.
    {
        constexpr int kKinds = kPropKinds;
        std::vector<uint16_t> grid(320 * 8 * 240 * 4, 0);
        int cell = 0;
        for (int index = 0; index < kKinds && cell < 32; ++index)
        {
            const int zone = index < kZones * kZoneProps ? index / kZoneProps : 0;
            const int kind =
                index < kZones * kZoneProps ? index % kZoneProps : kZoneProps + index - kZones * kZoneProps;
            bool done = false;
            for (uint32_t seed = 1; seed < 600 && !done; ++seed)
            {
                CrawlerGame game;
                game.NewRun(seed, zone * kZoneFloors + 2);
                for (int i = 0; i < CrawlerGame::kMaxProps && !done; ++i)
                {
                    const Prop& p = game.GetProp(i);
                    if (!p.present || p.kind != kind)
                    {
                        continue;
                    }
                    const int f = p.facing;
                    const Cell front{ p.at.x + 2 * kDx[f], p.at.y + 2 * kDy[f] };
                    if (!game.GetMap().IsWalkable(front))
                    {
                        continue;
                    }
                    const int kOffsets[2][2] = { { 3, 0 }, { 2, 2 } };
                    for (int view = 0; view < 2; ++view)
                    {
                        const Fix ax = (kDx[f] * kOffsets[view][0] - kDy[f] * kOffsets[view][1]) * kFixOne / 2;
                        const Fix ay = (kDy[f] * kOffsets[view][0] + kDx[f] * kOffsets[view][1]) * kFixOne / 2;
                        const Fix px = (p.at.x << kFixShift) + kFixOne / 2;
                        const Fix py = (p.at.y << kFixShift) + kFixOne / 2;
                        const double angle = std::atan2(-static_cast<double>(ay), -static_cast<double>(ax));
                        const int steps =
                            static_cast<int>(std::lround(angle / 6.283185307179586 * kAngleSteps)) & (kAngleSteps - 1);
                        const std::vector<uint16_t> frame =
                            Frame(game, kZoneList[zone], px + ax, py + ay, steps, false);
                        const int column = cell % 8;
                        const int row = cell / 8;
                        for (int r = 0; r < 240; ++r)
                        {
                            for (int c = 0; c < 320; ++c)
                            {
                                grid[(row * 240 + r) * 320 * 8 + column * 320 + c] = frame[r * 320 + c];
                            }
                        }
                        ++cell;
                    }
                    done = true;
                    std::printf("prop %d: seed %u\n", index, seed);
                }
            }
        }
        WritePpm(out + "/props.ppm", grid, 320 * 8, 240 * 4);
    }
    WritePpm(out + "/lore.ppm", sheet, 320 * kColumns, 240 * 2);
    return 0;
}
