// Renders every echo stone and both endings as they are heard, on a PC: a
// sheet of dark screens with all their lines up. Not a test: a preview.
// Usage: EchoPreview <out>

#include "../src/screen/Transitions.h"
#include "Preview.h"

int main(int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : ".";
    constexpr int kColumns = 4;
    constexpr int kRows = 5;
    std::vector<uint16_t> sheet(320 * kColumns * 240 * kRows);
    for (int i = 0; i < Lore::kEchoes + 2; ++i)
    {
        const bool ending = i >= Lore::kEchoes;
        std::vector<uint16_t> frame(320 * 240);
        Transitions::DrawEcho(frame.data(), 320, 0, 240, 0, 320, 320, 240, Lore::GetEcho(i), ending,
                              Transitions::EchoShownMs(ending));
        Put(sheet, kColumns, i % kColumns, i / kColumns, frame);
    }
    WritePpm(out + "/echoes.ppm", sheet, 320 * kColumns, 240 * kRows);
    return 0;
}
