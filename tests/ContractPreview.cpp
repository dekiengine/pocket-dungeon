// Renders the contracts on a PC: the notice board across the square from
// where a visit starts, its list, and someone asking. Not a test: a preview.
// Usage: ContractPreview <out>

#include "Preview.h"
#include "Walk.h"

#include "../src/screen/MessageCard.h"
#include "../src/world/Contracts.h"

namespace
{

const Zone kVillage{
    { T(VillageWall), nullptr, nullptr, nullptr }, 1, T(VillageWall), { T(Cobble), nullptr }, nullptr, nullptr
};

std::vector<uint16_t> Shoot(const CrawlerGame& game, bool panel)
{
    Fix x = 0;
    Fix y = 0;
    int angle = 0;
    game.GetView(x, y, angle);
    return Frame(game, kVillage, x, y, angle, panel, true, nullptr, kFixOne * 22);
}

}  // namespace

int main(int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : ".";
    g_Village = true;
    std::vector<uint16_t> sheet(320 * 4 * 240);
    CrawlerGame game;
    Legend legend;
    legend.buildings[2] = 1;
    legend.buildings[4] = 1;
    Contracts::SetState(legend, Contracts::Id::Brother, Contracts::State::Found);
    Contracts::SetState(legend, Contracts::Id::Meals, Contracts::State::Kept);
    Contracts::SetState(legend, Contracts::Id::Doll, Contracts::State::Taken);
    game.SetLegend(legend);
    game.GoHome(5);
    // From where a visit starts: the board, straight ahead.
    Put(sheet, 4, 0, 0, Shoot(game, false));
    const Cell board = Village::BoardCell();
    TestWalk::WalkTo(game, { board.x, board.y + 1 }, Village::WellCell());
    TestWalk::Face(game, 3);
    Put(sheet, 4, 1, 0, Shoot(game, false));
    game.Act(Action::Interact);
    game.Act(Action::MenuDown);
    Put(sheet, 4, 2, 0, Shoot(game, true));
    game.Act(Action::MenuClose);
    // Ulf, paying.
    const Cell fence = Village::FenceCell();
    TestWalk::WalkTo(game, { fence.x + 1, fence.y }, Village::WellCell());
    TestWalk::Face(game, 2);
    game.Act(Action::Interact);
    for (int i = 0; i + 1 < game.GetMenuCount(); ++i)
    {
        game.Act(Action::MenuDown);
    }
    game.Act(Action::MenuSelect);
    std::vector<uint16_t> frame = Shoot(game, false);
    MessageCard::Draw(frame.data(), 320, 0, 240, 0, 320, 320, 240, game.GetSpeech(), 1000, true);
    Put(sheet, 4, 3, 0, frame);
    WritePpm(out + "/contracts.ppm", sheet, 320 * 4, 240);
    return 0;
}
