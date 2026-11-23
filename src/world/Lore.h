#pragma once

// The story behind the dungeon, and what of it you carry from run to run.
//
// In the Hunger Winter the council of Last Hearth sealed two hundred
// fever-sick in the mine. Halvard, its scholar, held the key; his brother Sten,
// the foreman, went down with the sick and swore to keep them. The oath
// became the curse: he keeps them still, and everyone who comes down (the
// fallen wake above, a shade of them stays), and he has forgotten his name.
// Forty years on Halvard went down to open what he shut: the Keeper took his
// hat, his name and his heart (in four pieces, his brother's name cut into
// it), and holds him in the Seam. Sigrun, Halvard's apprentice, touched the
// grave-gold and is mired; Puddle is the last of the Listeners, whose kin
// are the stone frogs that hear names. What walks the deep is what the town
// left there: the tithe chest, the baron who hid grain, the three who
// signed. The hero holds the contract to end it. Echo stones hold moments
// they overheard.

#include "Village.h"

#include <cstdint>

namespace Dungeon
{

/// What one game remembers from all the runs before: saved with the best
/// depth and the diary pages.
struct Legend
{
    int deaths = 0;
    uint16_t echoes = 0;     // which echo stones were heard, a bit each
    int ghostDepth = 0;      // where the last run died (0: nowhere), its ghost waits there
    int ghostGold = 0;       // the gold it died with
    uint8_t ducks = 0;       // which of the three rubber ducks were found
    uint8_t endings = 0;     // 1: Halvard freed, 2: the staff taken
    uint8_t lastEnding = 0;  // the most recent: 0 none, 1 freed, 2 staff
    int marenSaved = 0;      // times Sigrun was healed back
    bool staff = false;      // Puddle's Pocket Staff earned, ever
    bool disco = false;      // all ducks found, and the dungeon told to dance
    // The village: its gold and materials, each building's level, and the
    // bosses beaten (1 mimic, 2 grub king, 4 hydragon) that open the lift.
    int bank = 0;
    int materials[Village::kMaterials] = {};
    uint8_t buildings[Village::kBuildings] = {};
    uint8_t bosses = 0;
    bool controlsShown = false;  // the keys card was shown, on the first start
    // The contracts of Last Hearth, two bits each (Contracts.h). Kept in
    // the save after the Crawler's keys, so older saves read as none taken.
    uint16_t contracts = 0;
};

namespace Lore
{

constexpr int kEchoes = 16;
constexpr int kEchoesPerZone = 4;
constexpr int kDucks = 3;
constexpr int kShards = 4;
// The two endings are told like echoes, after the sixteen.
constexpr int kEndingFreed = kEchoes;
constexpr int kEndingStaff = kEchoes + 1;

enum class Speaker : uint8_t
{
    Orlo,
    Maren,
    Puddle,
    Keeper,
    You,
};

/// An echo: four lines of at most 34 characters, and who says each.
struct Echo
{
    Speaker voice[4];
    const char* text;
};

const Echo& GetEcho(int index);
int CountBits(uint32_t bits);

/// The letter engraved on each heart shard, in the order they are found.
char ShardLetter(int shard);

/// The dungeon's voice, by the last ending: the Keeper, then Halvard once
/// freed, then you once you took the staff.
const char* VoiceName(const Legend& legend);
/// A line it says now and then on a new floor; `roll` picks one.
const char* VoiceLine(const Legend& legend, int roll);

/// A book on a shelf or a note on the floor: a title and four lines of at
/// most 34 characters.
struct Book
{
    const char* title;
    const char* text;
};

/// What reading a book gives, once a run.
enum class BookGift : uint8_t
{
    None,
    Attack,
    Defense,
    MaxHp,
    MaxMp,
    Lockpick,
};

constexpr int kBooks = 12;
const Book& GetBook(int index);
BookGift GetBookGift(int index);

/// What a note on the floor hints at: a secret on its own floor, or nothing.
enum class NoteHint : uint8_t
{
    LooseBrick,
    Crack,
    Shard,
    Echo,
    Vault,
    Nothing,
};
const Book& GetNote(NoteHint hint, int roll);

/// How many numbers the save file's text holds: what comes after them is
/// the Crawler's (its keys).
constexpr int kSaveValues = 25;

/// The save file's text: "<deepest> <pages> <the legend>". Reading takes the
/// older "<deepest> <pages>" too.
int WriteSave(char* out, int size, int deepest, int pages, const Legend& legend);
void ReadSave(const char* text, int& deepest, int& pages, Legend& legend);

}  // namespace Lore

}  // namespace Dungeon
