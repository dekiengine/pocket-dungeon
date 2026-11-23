// Renders things as the hero meets them, on a PC: items lying on the floor
// from two tiles away and from one, the village's people, a keeper, and the
// merchant. Not a test: a preview. Usage: ThingsPreview <out>

#include "Preview.h"
#include "Walk.h"

namespace
{

const Zone kVillage{
    { T(VillageWall), nullptr, nullptr, nullptr }, 1, T(VillageWall), { T(Cobble), nullptr }, nullptr, nullptr
};
const Zone kInterior{
    { T(InWall), nullptr, nullptr, nullptr }, 1, T(InWall), { T(InFloor), nullptr }, T(InCeiling), nullptr
};

std::vector<uint16_t> Shot(const CrawlerGame& game, const Zone& zone, Fix reach)
{
    Fix x = 0;
    Fix y = 0;
    int angle = 0;
    game.GetView(x, y, angle);
    return Frame(game, zone, x, y, angle, false, true, nullptr, reach);
}

// Stands the hero `away` tiles from `c` in a straight line, facing it.
bool StandOff(CrawlerGame& game, Cell c, int away)
{
    for (int f = 0; f < 4; ++f)
    {
        bool clear = true;
        for (int k = 1; k <= away; ++k)
        {
            const Cell n{ c.x - TestWalk::kDx[f] * k, c.y - TestWalk::kDy[f] * k };
            clear = clear && !TestWalk::Blocked(game, n) && game.MonsterAt(n) < 0;
        }
        const Cell stand{ c.x - TestWalk::kDx[f] * away, c.y - TestWalk::kDy[f] * away };
        if (clear && TestWalk::WalkTo(game, stand, c))
        {
            TestWalk::Face(game, f);
            return game.GetHeroCell() == stand;
        }
    }
    return false;
}

}  // namespace

int main(int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : ".";
    constexpr int kColumns = 4;
    std::vector<uint16_t> sheet(320 * kColumns * 240 * 4);
    int cell = 0;
    const auto put = [&](const std::vector<uint16_t>& frame)
    {
        if (cell < kColumns * 4)
        {
            Put(sheet, kColumns, cell % kColumns, cell / kColumns, frame);
            ++cell;
        }
    };
    // Items on the floor: two tiles away, then one.
    int shown = 0;
    bool seen[kItemKinds] = {};
    for (uint32_t seed = 1; seed < 60 && shown < 4; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 2);
        for (int i = 0; i < CrawlerGame::kMaxItems && shown < 4; ++i)
        {
            const Item item = game.GetItem(i);
            if (item.taken || seen[static_cast<int>(item.kind)])
            {
                continue;
            }
            if (StandOff(game, item.at, 2) && !game.GetItem(i).taken)
            {
                seen[static_cast<int>(item.kind)] = true;
                put(Shot(game, kZoneList[game.GetZone()], kFixOne * 55 / 10));
                game.Act(Action::Forward);
                Settle(game);
                put(Shot(game, kZoneList[game.GetZone()], kFixOne * 55 / 10));
                ++shown;
                std::printf("item %s\n", CrawlerGame::ItemName(item.kind));
            }
        }
    }
    cell = 8;
    // The village's people, from two tiles and from one.
    {
        g_Village = true;
        CrawlerGame game;
        game.GoHome(5);
        for (int v = 0; v < Village::kVillagers; ++v)
        {
            if (StandOff(game, Village::VillagerCell(v), 2))
            {
                put(Shot(game, kVillage, kFixOne * 22));
                game.Act(Action::Forward);
                Settle(game);
                put(Shot(game, kVillage, kFixOne * 22));
            }
        }
        g_Village = false;
    }
    // A keeper, and the merchant down below.
    {
        CrawlerGame game;
        Legend legend;
        legend.buildings[1] = 1;
        game.SetLegend(legend);
        game.GoHome(5);
        TestWalk::WalkTo(game, Village::OutsideCell(1), Village::WellCell());
        TestWalk::Face(game, (Village::OutsideFacing(1) + 2) & 3);
        game.Act(Action::Interact);
        Settle(game);
        put(Shot(game, kInterior, kFixOne * 715 / 100));
        game.Act(Action::Forward);
        Settle(game);
        put(Shot(game, kInterior, kFixOne * 715 / 100));
    }
    for (uint32_t seed = 1; seed < 200; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed, 6);
        Cell bear{ -1, -1 };
        for (int i = 0; i < CrawlerGame::kMaxFeatures; ++i)
        {
            if (game.GetFeature(i).present && game.GetFeature(i).kind == FeatureKind::Merchant)
            {
                bear = game.GetFeature(i).at;
            }
        }
        if (bear.x >= 0 && StandOff(game, bear, 2))
        {
            put(Shot(game, kZoneList[game.GetZone()], kFixOne * 55 / 10));
            game.Act(Action::Forward);
            Settle(game);
            put(Shot(game, kZoneList[game.GetZone()], kFixOne * 55 / 10));
            break;
        }
    }
    WritePpm(out + "/things.ppm", sheet, 320 * kColumns, 240 * 4);
    return 0;
}
