// Renders the effects on a PC: a run played by a script (walk to the nearest
// monster, hit it, pick up gold), with a frame kept at set times after a
// hit, a critical, a kill, a hurt, gold and a level gained. Not a test: a
// preview. Usage: EffectsPreview <out>

#include "Preview.h"

namespace
{

struct Shot
{
    const char* name;
    bool taken = false;
};

// One frame of the Crawler's loop: the game moves (unless held by a blow),
// the effects read it.
void Step(CrawlerGame& game, ViewEffects& effects, const CrawlerTextures& textures, int ms)
{
    game.Tick(effects.GameMs(ms));
    effects.Update(game, textures, 320, 240, ms);
}

bool Has(const CrawlerGame& game, EventKind kind)
{
    for (int i = 0; i < game.GetEventCount(); ++i)
    {
        if (game.GetEvent(i).kind == kind)
        {
            return true;
        }
    }
    return false;
}

// One step toward `target` (or a bump into it when next to it). False when
// there is no way.
bool StepToward(CrawlerGame& game, Cell target)
{
    DungeonMap map = game.GetMap();
    map.Set(target.x, target.y, Tile::Floor);
    map.ComputeDistances(target);
    const Cell hero = game.GetHeroCell();
    uint16_t best = map.DistanceAt(hero);
    int want = -1;
    for (int f = 0; f < 4; ++f)
    {
        const Cell n{ hero.x + kDx[f], hero.y + kDy[f] };
        const uint16_t d = map.DistanceAt(n);
        if (d != DungeonMap::kUnreached && d < best && (n == target || !Blocked(game, n)))
        {
            best = d;
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
    return true;
}

}  // namespace

int main(int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : ".";
    constexpr int kColumns = 4;
    constexpr int kRows = 3;
    std::vector<uint16_t> sheet(320 * kColumns * 240 * kRows);
    const CrawlerTextures textures = MakeTextures();
    int shots = 0;
    const auto take = [&](CrawlerGame& game, ViewEffects& effects, const char* what)
    {
        Fix x = 0;
        Fix y = 0;
        int angle = 0;
        game.GetView(x, y, angle);
        Put(sheet, kColumns, shots % kColumns, shots / kColumns,
            Frame(game, kZoneList[0], x, y, angle, false, true, &effects));
        std::printf("%d: %s\n", shots, what);
        ++shots;
    };

    bool hit = false;
    bool critical = false;
    bool kill = false;
    bool hurt = false;
    bool level = false;
    for (uint32_t seed = 1; seed < 200 && shots < kColumns * kRows; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed);
        ViewEffects effects;
        effects.SetDot(&Textures::kDustTexture);
        effects.Reset(game);
        Step(game, effects, textures, 16);
        for (int turn = 0; turn < 300 && !game.IsDead() && shots < kColumns * kRows; ++turn)
        {
            if (game.GetMenu() != Menu::None)
            {
                game.Act(Action::MenuClose);
            }
            // The nearest monster that is awake.
            int nearest = -1;
            int bestDistance = 1 << 30;
            const Cell hero = game.GetHeroCell();
            for (int i = 0; i < game.GetMonsterCount(); ++i)
            {
                const Monster& m = game.GetMonster(i);
                const int d = (m.at.x - hero.x) * (m.at.x - hero.x) + (m.at.y - hero.y) * (m.at.y - hero.y);
                if (m.alive && d < bestDistance)
                {
                    bestDistance = d;
                    nearest = i;
                }
            }
            if (nearest < 0 || !StepToward(game, game.GetMonster(nearest).at))
            {
                break;
            }
            const bool nowHit = Has(game, EventKind::Hit);
            const bool nowCritical = Has(game, EventKind::Critical);
            const bool nowKill = Has(game, EventKind::Kill);
            const int hpBefore = game.GetHp();
            const int levelBefore = game.GetLevel();
            (void)hpBefore;
            Step(game, effects, textures, 16);
            const bool nowLevel = game.GetLevel() > levelBefore || effects.GetLevelMs() > 1600;
            const bool nowHurt = effects.GetHpTrail() > game.GetHp() * 16;
            const auto after = [&](int ms)
            {
                for (int t = 0; t < ms; t += 16)
                {
                    Step(game, effects, textures, 16);
                }
            };
            if (nowCritical && !critical)
            {
                critical = true;
                after(48);
                take(game, effects, "critical +60 ms");
                after(200);
                take(game, effects, "critical +260 ms");
            }
            else if (nowHit && !hit)
            {
                hit = true;
                take(game, effects, "hit +16 ms");
                after(80);
                take(game, effects, "hit +100 ms");
            }
            if (nowKill && !kill)
            {
                kill = true;
                after(100);
                take(game, effects, "kill +120 ms");
                after(380);
                take(game, effects, "kill +500 ms");
            }
            if (nowLevel && !level)
            {
                level = true;
                after(100);
                take(game, effects, "level up +120 ms");
            }
            if (nowHurt && !hurt)
            {
                hurt = true;
                after(40);
                take(game, effects, "hurt +60 ms");
            }
            Settle(game);
            after(200);
        }
    }
    // Gold picked up: walk onto the first floor's gold.
    for (uint32_t seed = 1; seed < 100; ++seed)
    {
        CrawlerGame game;
        game.NewRun(seed);
        ViewEffects effects;
        effects.SetDot(&Textures::kDustTexture);
        effects.Reset(game);
        Step(game, effects, textures, 16);
        Cell gold{ -1, -1 };
        for (int i = 0; i < CrawlerGame::kMaxItems; ++i)
        {
            const Item& item = game.GetItem(i);
            if (!item.taken && item.kind == ItemKind::Gold)
            {
                gold = item.at;
            }
        }
        const int before = game.GetGold();
        if (gold.x < 0 || !WalkInto(game, gold) || game.GetGold() <= before)
        {
            continue;
        }
        for (int t = 0; t < 160; t += 16)
        {
            Step(game, effects, textures, 16);
        }
        take(game, effects, "gold +160 ms");
        for (int t = 0; t < 240; t += 16)
        {
            Step(game, effects, textures, 16);
        }
        take(game, effects, "gold +400 ms");
        break;
    }
    const std::string path = out + "/effects.ppm";
    WritePpm(path, sheet, 320 * kColumns, 240 * kRows);
    std::printf("wrote %s (%d shots)\n", path.c_str(), shots);
    return 0;
}
