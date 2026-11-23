#pragma once

// The contracts of Last Hearth: what its people ask of the hero, each a
// piece of what the town did in the Hunger Winter. Someone asks (a few lines,
// one at a time), the thing is done down below, and bringing word of it back
// pays: gold for the village, or something every run starts with from then
// on. Which are taken, done and paid is kept over runs (Legend::contracts,
// two bits each). No engine: who asks what, and the words.

#include "CrawlerGame.h"
#include "Lore.h"

#include <cstdint>

namespace Dungeon::Contracts
{

enum class Id : uint8_t
{
    Meals,     // Gunnar: the bog trader owes for twelve meals
    Brother,   // Ulf: his brother went down for water and walks the third floor
    Doll,      // Hilde: a child's doll, in the Ossuary
    Ledger,    // Runa: the baron's ledger, on the carrion king
    Question,  // Asgeir: what the echo stones say of Halvard
    Herb,      // Ingrid: three glowcaps carried home
    Hammer,    // Torvald: his father's hammer, by an anvil in the Foundry
};
constexpr int kContracts = 7;

/// Who asks: the keepers by their building (Village::Building), then the
/// two who stand outside.
enum class Giver : uint8_t
{
    Torvald,
    Ingrid,
    Gunnar,
    Asgeir,
    Runa,
    Hilde = 6,
    Ulf = 7,
};

enum class State : uint8_t
{
    Unasked,
    Taken,
    Found,  // done below: word of it is owed to whoever asked
    Kept,   // paid
};

State GetState(const Legend& legend, Id id);
void SetState(Legend& legend, Id id, State state);
bool IsKept(const Legend& legend, Id id);

constexpr int kOfferLines = 3;
constexpr int kBackLines = 2;

/// The words of one contract. Lines someone speaks start with their name
/// and fit a line of talk (63 characters).
struct Info
{
    const char* name;
    Giver giver;
    const char* wants;  // on the notice board before it is taken: who wants a word
    const char* where;  // and where they are
    const char* offer[kOfferLines];
    const char* remind;
    const char* back[kBackLines];
    const char* found;     // what the hero sees when it is done below (or null)
    ItemKind foundItem;    // and its picture
    const char* hints[3];  // in the list: while taken, once found, once kept
    const char* reward;    // what keeping it gave
    const char* gossip;    // what Leif says of it afterwards
};
const Info& GetInfo(Id id);

/// The contract `giver` is the one to ask for, or -1.
int Of(Giver giver);

/// How many echo stones the teacher's question needs heard.
constexpr int kEchoesAsked = 5;
/// The floor Orm walks.
constexpr int kOrmFloor = 3;
/// Glowcaps to carry home.
constexpr int kGlowcaps = 3;

}  // namespace Dungeon::Contracts
