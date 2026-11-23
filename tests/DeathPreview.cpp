// Renders the black screens on a PC: a hero stands still until the floor's
// monsters beat them, then frames of the fall's screen as it goes black and
// its lines come up, and a floor's card. Not a test: a preview. Usage:
// DeathPreview <out>

#include "../src/screen/Transitions.h"
#include "Preview.h"
#include "Walk.h"

int main(int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : ".";
    CrawlerGame game;
    game.NewRun(7, 9);
    // Walk at the nearest monster and wait there: it wins in the end.
    for (int steps = 0; steps < 3000 && !game.IsDead(); ++steps)
    {
        int nearest = -1;
        int best = 1 << 30;
        for (int i = 0; i < game.GetMonsterCount(); ++i)
        {
            const Monster& m = game.GetMonster(i);
            const int d = std::abs(m.at.x - game.GetHeroCell().x) + std::abs(m.at.y - game.GetHeroCell().y);
            if (m.alive && d < best)
            {
                best = d;
                nearest = i;
            }
        }
        if (nearest < 0)
        {
            break;
        }
        Cell stand;
        int way = 0;
        if (best > 1 && TestWalk::StandBeside(game, game.GetMonster(nearest).at, stand, way))
        {
            TestWalk::WalkTo(game, stand);
        }
        game.Act(Action::Wait);
        Settle(game);
    }
    if (!game.IsDead())
    {
        std::printf("the hero lived\n");
        return 1;
    }
    Fix x = 0;
    Fix y = 0;
    int angle = 0;
    game.GetView(x, y, angle);
    const Zone& zone = kZoneList[game.GetZone()];
    const int length = Transitions::FallLength(game);
    const int times[3] = { 400, Transitions::kFallBlackMs + 2 * Transitions::kFallLineMs + 100, length - 700 };
    std::vector<uint16_t> sheet(320 * 3 * 240 * 2);
    for (int i = 0; i < 3; ++i)
    {
        std::vector<uint16_t> frame = Frame(game, zone, x, y, angle, false);
        Transitions::DrawFall(frame.data(), 320, 0, 240, 0, 320, 320, 240, game, times[i]);
        Put(sheet, 3, i, 0, frame);
    }
    // A floor's card: dark, then the view coming through.
    const int left[3] = { 1200, 600, 200 };
    for (int i = 0; i < 3; ++i)
    {
        std::vector<uint16_t> frame = Frame(game, zone, x, y, angle, false);
        Transitions::DrawPlace(frame.data(), 320, 0, 240, 0, 320, 320, 240, CrawlerGame::ZoneName(game.GetZone()), "B9",
                               left[i]);
        Put(sheet, 3, i, 1, frame);
    }
    WritePpm(out + "/death.ppm", sheet, 320 * 3, 240 * 2);
    std::printf("%s | bank kept %d, echo %d, materials %d\n", game.GetMessage(), game.GetDeathReport().keptGold,
                game.GetDeathReport().echoGold, game.GetDeathReport().materials);
    return 0;
}
