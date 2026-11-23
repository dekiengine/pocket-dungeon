// Renders every thing standing about with its choices open, on a PC: the
// hero beside it, facing it, its menu over the view; a full bag and food
// taken out of it; a diary page, a book and a note; and the message card
// with someone speaking. Not a test: a preview.
// Usage: ChoicePreview <out>

#include "../src/screen/MessageCard.h"
#include "../src/screen/Paper.h"
#include "../src/world/CrawlerGameData.h"
#include "Preview.h"
#include "Walk.h"

int main(int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : ".";
    constexpr int kColumns = 6;
    constexpr int kRows = 6;
    std::vector<uint16_t> sheet(320 * kColumns * 240 * kRows);
    for (int index = 0; index < kPropKinds; ++index)
    {
        const int zone = index < kZones * kZoneProps ? index / kZoneProps : 0;
        const int kind = index < kZones * kZoneProps ? index % kZoneProps : kZoneProps + index - kZones * kZoneProps;
        bool done = false;
        for (uint32_t seed = 1; seed < 600 && !done; ++seed)
        {
            CrawlerGame game;
            game.SetDeepest(99);
            game.NewRun(seed, zone * kZoneFloors + 2);
            for (int i = 0; i < CrawlerGame::kMaxProps && !done; ++i)
            {
                const Prop& p = game.GetProp(i);
                Cell stand;
                int way = 0;
                if (!p.present || p.any || p.kind != kind || !TestWalk::StandBeside(game, p.at, stand, way) ||
                    !TestWalk::WalkTo(game, stand))
                {
                    continue;
                }
                TestWalk::Face(game, way);
                game.Act(Action::Interact);
                if (game.GetMenu() != Menu::Thing)
                {
                    continue;
                }
                Fix x = 0;
                Fix y = 0;
                int angle = 0;
                game.GetView(x, y, angle);
                Put(sheet, kColumns, index % kColumns, index / kColumns,
                    Frame(game, kZoneList[zone], x, y, angle, true));
                done = true;
            }
        }
        std::printf("thing %d: %s\n", index, done ? "shown" : "not found");
    }
    // A full bag, scrolled to its middle, and food taken out of it.
    {
        CrawlerGame game;
        game.NewRun(3);
        for (const ItemKind kind : GameData::kBagKinds)
        {
            game.SetCarried(kind, 2);
        }
        Fix x = 0;
        Fix y = 0;
        int angle = 0;
        game.GetView(x, y, angle);
        game.Act(Action::OpenBag);
        for (int i = 0; i < 4; ++i)
        {
            game.Act(Action::MenuDown);
        }
        Put(sheet, kColumns, 0, kRows - 1, Frame(game, kZoneList[0], x, y, angle, true));
        game.Act(Action::MenuSelect);  // the fish
        Put(sheet, kColumns, 1, kRows - 1, Frame(game, kZoneList[0], x, y, angle, true));
    }
    // The menu, the keys, what a deed did (waiting, then shown), a thing
    // used up, and someone speaking until a key.
    {
        CrawlerGame game;
        game.NewRun(3);
        Fix x = 0;
        Fix y = 0;
        int angle = 0;
        game.GetView(x, y, angle);
        game.Act(Action::OpenMenu);
        Put(sheet, kColumns, 0, kRows - 2, Frame(game, kZoneList[0], x, y, angle, true));
        game.Act(Action::MenuClose);
        std::vector<uint16_t> frame = Frame(game, kZoneList[0], x, y, angle, false);
        const char* said = "Hilde: I was six when they shut the door. I heard them sing.";
        MessageCard::Draw(frame.data(), 320, 0, 240, 0, 320, 320, 240, said, 100, true);
        Put(sheet, kColumns, 4, kRows - 2, frame);
    }
    // Things to read: a diary page, a book, a note.
    {
        CrawlerGame game;
        game.NewRun(3);
        Fix x = 0;
        Fix y = 0;
        int angle = 0;
        game.GetView(x, y, angle);
        const Lore::Book& book = Lore::GetBook(4);
        const Lore::Book& note = Lore::GetNote(Lore::NoteHint::Crack, 0);
        std::vector<uint16_t> page = Frame(game, kZoneList[0], x, y, angle, false);
        Paper::Draw(page.data(), 320, 0, 240, 0, 320, 320, 240, nullptr, CrawlerGame::PageText(3), "- 3 -", true);
        Put(sheet, kColumns, 2, kRows - 1, page);
        page = Frame(game, kZoneList[0], x, y, angle, false);
        Paper::Draw(page.data(), 320, 0, 240, 0, 320, 320, 240, book.title, book.text, nullptr, false);
        Put(sheet, kColumns, 3, kRows - 1, page);
        page = Frame(game, kZoneList[0], x, y, angle, false);
        Paper::Draw(page.data(), 320, 0, 240, 0, 320, 320, 240, note.title, note.text, nullptr, false);
        Put(sheet, kColumns, 4, kRows - 1, page);
    }
    // And someone speaking: the name on a tag, the words half typed out.
    {
        CrawlerGame game;
        game.NewRun(3);
        Fix x = 0;
        Fix y = 0;
        int angle = 0;
        game.GetView(x, y, angle);
        std::vector<uint16_t> frame = Frame(game, kZoneList[0], x, y, angle, false);
        const char* said = "Puddle: Good. 2 more, and I give what I was given.";
        MessageCard::Draw(frame.data(), 320, 0, 240, 0, 320, 320, 240, said, 33);
        Put(sheet, kColumns, kColumns - 1, kRows - 1, frame);
    }
    WritePpm(out + "/choices.ppm", sheet, 320 * kColumns, 240 * kRows);
    return 0;
}
