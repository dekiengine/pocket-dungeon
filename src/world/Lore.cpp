#include "Lore.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Dungeon::Lore
{

namespace
{

// Four per zone, heard in this order: the first zone's on its floors, and so
// on. Each is a moment the stone overheard, a voice to a line, and the story
// turns as you go deeper.
const Echo kEchoTable[kEchoes + 2] = {
    // The Weeping Halls: the sealing. Sten speaks in the Keeper's colour.
    { { Speaker::Keeper, Speaker::Orlo, Speaker::Keeper, Speaker::Orlo },
      "Two hundred, Halvard. Children.\n"
      "The council says seal the mine.\n"
      "Then I go down with them.\n"
      "Sten. I hold the key. Do not." },
    { { Speaker::Orlo, Speaker::Keeper, Speaker::Keeper, Speaker::Keeper },
      "Forgive me. It is done. Locked.\n"
      "I hear you through the door.\n"
      "Go home. I will keep them.\n"
      "All of them. I swear it." },
    { { Speaker::Puddle, Speaker::Puddle, Speaker::Puddle, Speaker::Puddle },
      "He shared his last bread with me.\n"
      "A foreman, feeding a bog thing.\n"
      "I kept his staff. Feed me thrice\n"
      "as he did, and you may have it." },
    { { Speaker::Maren, Speaker::Orlo, Speaker::Maren, Speaker::Maren },
      "Coins, master. On every eyelid.\n"
      "For the ferryman. Touch nothing.\n"
      "One would not be missed.\n"
      "Master. It will not let go." },
    // The Ossuary: forty years on.
    { { Speaker::Orlo, Speaker::Keeper, Speaker::Keeper, Speaker::Keeper },
      "Sten? Forty years. Is it you?\n"
      "Sten. I kept so many names.\n"
      "I lost that one. Give me yours.\n"
      "And the hat. I am Halvard now." },
    { { Speaker::Maren, Speaker::Maren, Speaker::Maren, Speaker::Maren },
      "The gold is in my hands now.\n"
      "Soon I will not know my name.\n"
      "If we meet below, do not strike.\n"
      "Heal me. I will know you then." },
    { { Speaker::Orlo, Speaker::Orlo, Speaker::Keeper, Speaker::Keeper },
      "It took my heart out, whole.\n"
      "I had cut his name into it.\n"
      "A heart keeps better in four.\n"
      "One in each deep. Third floor." },
    { { Speaker::Maren, Speaker::Puddle, Speaker::Puddle, Speaker::Puddle },
      "The stone frogs, Puddle?\n"
      "Listeners. My kin, before stone.\n"
      "Tell one a name. Mine. His. Hers.\n"
      "Each gives something back, once." },
    // The Sunken Reach: the truth comes out.
    { { Speaker::Orlo, Speaker::Orlo, Speaker::Orlo, Speaker::Orlo },
      "If a stone still hears me:\n"
      "I am kept in the Seam, below all.\n"
      "Bring my heart to a Listener.\n"
      "Say the name it has forgotten." },
    { { Speaker::Maren, Speaker::Keeper, Speaker::Maren, Speaker::Maren },
      "You wear his hat. Not his name.\n"
      "I am what I keep.\n"
      "You had one of your own. Short.\n"
      "A stone's name, in the old tongue." },
    { { Speaker::Keeper, Speaker::Keeper, Speaker::Keeper, Speaker::Keeper },
      "I swore to keep them. I keep you.\n"
      "Fall, and you wake above.\n"
      "But a shade of you stays with me,\n"
      "minding your gold. Go and see." },
    { { Speaker::Keeper, Speaker::Keeper, Speaker::Puddle, Speaker::Puddle },
      "I carved three ducks for them.\n"
      "The children. For the dark.\n"
      "They are still down here.\n"
      "Find all three. Tell a Listener." },
    // The Cinder Foundry: the last pieces.
    { { Speaker::Orlo, Speaker::Orlo, Speaker::Orlo, Speaker::Orlo },
      "I locked the door on my brother.\n"
      "Then I forgot how to say him.\n"
      "Four letters. One cut into each\n"
      "piece of my heart. In that order." },
    { { Speaker::Keeper, Speaker::Keeper, Speaker::Maren, Speaker::Maren },
      "The hat. The staff. The gold.\n"
      "Take it. Someone must keep them.\n"
      "That is how he became this.\n"
      "Mend the heart. Let it end." },
    { { Speaker::Keeper, Speaker::Keeper, Speaker::Keeper, Speaker::Keeper },
      "I am Halvard. I am the Keeper.\n"
      "I held the door. No. I was\n"
      "behind the door. Who held it?\n"
      "Who am I keeping them from?" },
    { { Speaker::Puddle, Speaker::Puddle, Speaker::Puddle, Speaker::Puddle },
      "No bottom here. Only the Seam.\n"
      "Four pieces, a frog, his name.\n"
      "He was kind once. Remember that\n"
      "when you raise your blade." },
    // The endings.
    { { Speaker::Keeper, Speaker::Keeper, Speaker::Orlo, Speaker::Orlo },
      "Sten. Yes. That was it.\n"
      "Then I can put them down.\n"
      "Brother. I held the key.\n"
      "Go up. Tell them who he was." },
    { { Speaker::You, Speaker::You, Speaker::You, Speaker::Keeper },
      "The hat fits. The staff is warm.\n"
      "Two hundred shades turn to you.\n"
      "Someone must keep them.\n"
      "Keep." },
};

const char* const kKeeperLines[] = {
    "Keeper: Come down. I keep a place for you.",        "Keeper: No one dies here. I swore it.",
    "Keeper: I am Halvard. See? I have the hat.",        "Keeper: Two hundred, and you. I count every night.",
    "Keeper: Fall where you like. I keep what is left.", "Keeper: I hear you through the stone. I always did.",
    "Keeper: The frogs listen. They never answer me.",   "Keeper: Who locked the door? I ask. No one says.",
};
const char* const kOrloLines[] = {
    "Halvard: You again. Good. The dead need visitors.",
    "Halvard: He rests. I do not. That is fair.",
    "Halvard: Puddle asks after you. And food.",
    "Halvard: Sigrun has her hands back. She will not talk.",
};
const char* const kYouLines[] = {
    "Keeper (you): Come down. I kept your place.",
    "Keeper (you): Someone must keep them.",
    "Keeper (you): I hear you through the stone.",
    "Keeper (you): Was I always this hungry?",
};

// Books on shelves: some teach something (kBookGifts), some point at secrets,
// some are only silly.
const Book kBookTable[kBooks] = {
    { "The Sellsword's Edge", "A dull blade is a debt.\n"
                              "Pay it each morning,\n"
                              "on a stone, before\n"
                              "it is collected." },
    { "Shield Drill, Part II", "The wall holds because\n"
                               "each shield guards the\n"
                               "man to its left.\n"
                               "Alone, guard yourself." },
    { "Remedies of the Reach", "Scald crawler tea:\n"
                               "steep, strain, drink\n"
                               "hot. Foul. You will\n"
                               "live longer for it." },
    { "On Folding", "All magic is folding.\n"
                    "Fold the air, the fire,\n"
                    "the space between\n"
                    "two steps." },
    { "Rites of the Listeners", "The frogs keep names.\n"
                                "Give one a name you\n"
                                "know. It gives back,\n"
                                "once for each name." },
    { "The Bog Trader", "He buys gems, pearls\n"
                        "and goblets, and keeps\n"
                        "none. Ask him why.\n"
                        "He changes the subject." },
    { "Five Blows", "A cracked wall is\n"
                    "a shut mouth.\n"
                    "Strike it five times.\n"
                    "It will open." },
    { "The Mason's Confession", "I left one brick loose\n"
                                "on every floor, for\n"
                                "my wages. Stand close.\n"
                                "Pull it." },
    { "On the Tithe Chest", "It took all we had.\n"
                            "Then it learned to\n"
                            "take. Prod a lid\n"
                            "before you lift it." },
    { "Locks, Patiently", "Picks break. A\n"
                          "dagger helps. So\n"
                          "does waiting. Here:\n"
                          "take a spare." },
    { "The Baron's Table", "Morning: the dead.\n"
                           "Noon: the dead.\n"
                           "Night: whoever came\n"
                           "down. He is not full." },
    { "Three Carved Ducks", "Made for children\n"
                            "who went below. Look\n"
                            "where secrets hide.\n"
                            "Then tell a Listener." },
};
constexpr BookGift kBookGifts[kBooks] = {
    BookGift::Attack, BookGift::Defense, BookGift::MaxHp, BookGift::MaxMp,    BookGift::None, BookGift::None,
    BookGift::None,   BookGift::None,    BookGift::None,  BookGift::Lockpick, BookGift::None, BookGift::None,
};

// Notes left by those who came before: two for each hint.
const Book kNoteTable[6][2] = {
    { { "A crumpled note", "Hid my gold behind a\n"
                           "loose brick on this floor.\n"
                           "The wall wiggles. Don't\n"
                           "tell Gorm. - Ivar" },
      { "A muddy note", "Gorm here. Ivar hides\n"
                        "things in walls. Face\n"
                        "every wall, slowly.\n"
                        "Something sticks out." } },
    { { "A crumpled note", "Cold air blows out of a\n"
                           "wall here. Five blows,\n"
                           "Mum said. Or bomb\n"
                           "it. - Astrid" },
      { "A sooty note", "There's a room behind a\n"
                        "cracked wall. I heard\n"
                        "snoring in it. Bring a\n"
                        "bomb. Or a hammer." } },
    { { "A shaking note", "Something red hums on\n"
                          "this floor. Warm, like a\n"
                          "heartbeat. The voice\n"
                          "told me to leave it." },
      { "A bloody note", "The Keeper screamed at\n"
                         "me when I got close to\n"
                         "the red shard. So I\n"
                         "got closer. - Ivar" } },
    { { "A damp note", "A stone in the wall here\n"
                       "sings. It only sings\n"
                       "once. Listen well.\n"
                       "- Astrid" },
      { "A neat note", "The glowing runes speak\n"
                       "in Halvard's voice. Or\n"
                       "something that sounds\n"
                       "like him." } },
    { { "A crumpled note", "Lost the vault key again.\n"
                           "Puddle sells spares.\n"
                           "He charges too much.\n"
                           "- Ivar" },
      { "A greasy note", "Behind the locked door:\n"
                         "two chests and a potion.\n"
                         "I saw it through the\n"
                         "keyhole. - Gorm" } },
    { { "A crumpled note", "Day 9. Ate a mushroom.\n"
                           "Saw the bones of the\n"
                           "floor. Would eat again.\n"
                           "- Astrid" },
      { "A tear-stained note", "Dear Mum. Nothing dies\n"
                               "down here. Neither do\n"
                               "I. I am not coming\n"
                               "home. - Gorm" } },
};

}  // namespace

const Book& GetBook(int index)
{
    return kBookTable[index >= 0 && index < kBooks ? index : 0];
}

BookGift GetBookGift(int index)
{
    return kBookGifts[index >= 0 && index < kBooks ? index : 0];
}

const Book& GetNote(NoteHint hint, int roll)
{
    const int h = static_cast<int>(hint);
    return kNoteTable[h >= 0 && h < 6 ? h : 5][roll & 1];
}

const Echo& GetEcho(int index)
{
    return kEchoTable[index >= 0 && index < kEchoes + 2 ? index : 0];
}

int CountBits(uint32_t bits)
{
    int count = 0;
    for (; bits != 0; bits &= bits - 1)
    {
        ++count;
    }
    return count;
}

char ShardLetter(int shard)
{
    constexpr char kName[kShards + 1] = "STEN";
    return kName[shard >= 0 && shard < kShards ? shard : 0];
}

const char* VoiceName(const Legend& legend)
{
    return legend.lastEnding == 1 ? "Halvard" : legend.lastEnding == 2 ? "Keeper (you)" : "Keeper";
}

const char* VoiceLine(const Legend& legend, int roll)
{
    if (legend.lastEnding == 1)
    {
        return kOrloLines[roll % (sizeof(kOrloLines) / sizeof(kOrloLines[0]))];
    }
    if (legend.lastEnding == 2)
    {
        return kYouLines[roll % (sizeof(kYouLines) / sizeof(kYouLines[0]))];
    }
    return kKeeperLines[roll % (sizeof(kKeeperLines) / sizeof(kKeeperLines[0]))];
}

int WriteSave(char* out, int size, int deepest, int pages, const Legend& legend)
{
    const int length = std::snprintf(out, static_cast<size_t>(size), "%d %d %d %u %d %d %u %u %u %d %d %d", deepest,
                                     pages, legend.deaths, static_cast<unsigned>(legend.echoes), legend.ghostDepth,
                                     legend.ghostGold, static_cast<unsigned>(legend.ducks),
                                     static_cast<unsigned>(legend.endings), static_cast<unsigned>(legend.lastEnding),
                                     legend.marenSaved, legend.staff ? 1 : 0, legend.disco ? 1 : 0);
    if (length < 0 || length >= size)
    {
        return length;
    }
    // Then the village.
    const int more =
        std::snprintf(out + length, static_cast<size_t>(size - length), " %d %d %d %d %d %u %u %u %u %u %u %u %d",
                      legend.bank, legend.materials[0], legend.materials[1], legend.materials[2], legend.materials[3],
                      static_cast<unsigned>(legend.buildings[0]), static_cast<unsigned>(legend.buildings[1]),
                      static_cast<unsigned>(legend.buildings[2]), static_cast<unsigned>(legend.buildings[3]),
                      static_cast<unsigned>(legend.buildings[4]), static_cast<unsigned>(legend.buildings[5]),
                      static_cast<unsigned>(legend.bosses), legend.controlsShown ? 1 : 0);
    return more < 0 ? length : length + more;
}

void ReadSave(const char* text, int& deepest, int& pages, Legend& legend)
{
    // Numbers apart by spaces; any missing ones (an older file) stay as
    // they were.
    constexpr int kValues = kSaveValues;
    long values[kValues];
    int count = 0;
    const char* p = text;
    while (count < kValues)
    {
        char* end = nullptr;
        const long v = std::strtol(p, &end, 10);
        if (end == p)
        {
            break;
        }
        values[count++] = v;
        p = end;
    }
    const auto at = [&](int i, long fallback) { return i < count ? values[i] : fallback; };
    deepest = static_cast<int>(at(0, deepest));
    pages = static_cast<int>(at(1, pages));
    legend.deaths = static_cast<int>(at(2, legend.deaths));
    legend.echoes = static_cast<uint16_t>(at(3, legend.echoes));
    legend.ghostDepth = static_cast<int>(at(4, legend.ghostDepth));
    legend.ghostGold = static_cast<int>(at(5, legend.ghostGold));
    legend.ducks = static_cast<uint8_t>(at(6, legend.ducks));
    legend.endings = static_cast<uint8_t>(at(7, legend.endings));
    legend.lastEnding = static_cast<uint8_t>(at(8, legend.lastEnding));
    legend.marenSaved = static_cast<int>(at(9, legend.marenSaved));
    legend.staff = at(10, legend.staff ? 1 : 0) != 0;
    legend.disco = at(11, legend.disco ? 1 : 0) != 0;
    legend.bank = static_cast<int>(at(12, legend.bank));
    for (int m = 0; m < Village::kMaterials; ++m)
    {
        legend.materials[m] = static_cast<int>(at(13 + m, legend.materials[m]));
    }
    for (int b = 0; b < Village::kBuildings; ++b)
    {
        const long level = at(17 + b, legend.buildings[b]);
        legend.buildings[b] =
            static_cast<uint8_t>(level < 0 ? 0 : (level > Village::kMaxLevel ? Village::kMaxLevel : level));
    }
    legend.bosses = static_cast<uint8_t>(at(23, legend.bosses));
    legend.controlsShown = at(24, legend.controlsShown ? 1 : 0) != 0;
}

}  // namespace Dungeon::Lore
