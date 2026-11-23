#include "Village.h"

namespace Dungeon::Village
{

namespace
{

// The square: tiles x 8..16, y 6..12, houses round it: room to walk about
// in, every front still a few steps away.
constexpr Room kPlaza{ 8, 6, 9, 7 };
// Off its west side: an alley one tile wide (x 2..7, y 9) with a dead end
// where a man waits, a turn north up it (x 3, y 4..8), and the yard of
// stones at its end (x 2..5, y 2..3).
constexpr Room kAlley{ 2, 9, 6, 1 };
constexpr Room kAlleyNorth{ 3, 4, 1, 5 };
constexpr Room kYard{ 2, 2, 4, 2 };
constexpr Room kPlaces[4] = { kPlaza, kAlley, kAlleyNorth, kYard };

constexpr const char* kFenceLines[4] = {
    "Ulf: I sell what the dead dropped. Do not ask how.",
    "Ulf: Go deeper and I will have more to sell.",
    "Ulf: The council buys from me too. At night.",
    "Ulf: My brother went down that well. I sell his boots.",
};
// For one of each: moss, bone, shell, ore.
constexpr int kMaterialPrices[kMaterials] = { 25, 45, 70, 100 };
constexpr Cell kStoneCells[kStones] = { { 2, 2 }, { 4, 2 }, { 5, 2 } };
constexpr const char* kStoneLines[kStones] = {
    "\"Sten, foreman. No body lies under this.\"",
    "\"For the two hundred. The council paid for the stone.\"",
    "\"Astrid, six. Her mother still leaves bread.\"",
};

// Three fronts on the north side, three on the south, two houses between
// each.
constexpr Cell kBuildingCells[kBuildings] = { { 9, 5 }, { 12, 5 }, { 15, 5 }, { 9, 13 }, { 12, 13 }, { 15, 13 } };

constexpr const char* kNames[kBuildings] = {
    "Blacksmith", "Apothecary", "Tavern", "Wizard tower", "Bank", "Mine lift"
};
constexpr const char* kMaterialNames[kMaterials] = { "moss", "bone", "shell", "ore" };

// What each level does: level 0 is the ruin.
constexpr const char* kEffects[kBuildings][kMaxLevel + 1] = {
    { "Ruined. Build it to start with Atk +1.", "Start with Atk +1", "Start with Atk +2, Def +1",
      "Start with Atk +3, Def +2" },
    { "Ruined. Build it to start with 2 potions.", "Start with 2 potions", "Start with 3 potions",
      "Start with 4 potions" },
    { "Ruined. Build it for max HP +8.", "Max HP +8", "Max HP +16", "Max HP +24" },
    { "Ruined. Build it for max MP +3.", "Max MP +3", "Max MP +6", "Max MP +9" },
    { "You keep half your gold on death", "You keep 65% of your gold on death", "You keep 80% of your gold on death",
      "You keep all your gold on death" },
    { "Ruined. Build it to start at B6.", "Rides down to B6", "Rides down to B11", "Rides down to B16" },
};

// Gold and material for levels 1, 2 and 3.
constexpr Cost kCosts[kBuildings][kMaxLevel] = {
    { { 50, 0, 3 }, { 150, 1, 4 }, { 350, 3, 5 } },   // blacksmith: moss, bone, ore
    { { 40, 0, 3 }, { 120, 2, 4 }, { 300, 1, 5 } },   // apothecary: moss, shell, bone
    { { 40, 0, 2 }, { 120, 1, 4 }, { 300, 2, 5 } },   // tavern: moss, bone, shell
    { { 60, 0, 3 }, { 160, 2, 4 }, { 350, 3, 5 } },   // tower: moss, shell, ore
    { { 80, 1, 2 }, { 200, 2, 4 }, { 400, 3, 5 } },   // bank: bone, shell, ore
    { { 100, 1, 3 }, { 250, 2, 4 }, { 500, 3, 5 } },  // lift: bone, shell, ore
};

constexpr const char* kVillagerNames[kVillagers] = { "Old Hilde", "Leif" };
constexpr const char* kVillagerAbout[kVillagers] = { "Old enough to remember the door.",
                                                     "Too young to be afraid. Yet." };
constexpr int kLines = 6;
constexpr const char* kVillagerLines[kVillagers][kLines] = {
    { "Hilde: Moss first. Then bone. Then what lies deeper.", "Hilde: Bring the gold home. The fallen keep only half.",
      "Hilde: Build the bank. Coin under a bed draws shades.",
      "Hilde: My mother whispered names to the frogs. We all did.",
      "Hilde: The lift will not pass a beast that still breathes.",
      "Hilde: I was six when they shut the door. I heard them sing." },
    { "Leif: Is it true some walls have a loose brick?", "Leif: I saw a shade down the well. It had your face.",
      "Leif: Puddle says food buys anything. Even from the dead.",
      "Leif: Gran says the Keeper had a name. Nobody knows it.",
      "Leif: With nothing hunting you, you can walk back up.",
      "Leif: The books down there teach things. Torvald can't read." },
};

// Inside a building: a room of 5 by 3, the door in the middle of the near
// wall, the keeper at the back, two steps in.
constexpr Room kInterior{ 9, 9, 5, 3 };

// The props' places in CrawlerGame's kPropInfo.
constexpr uint8_t kBarrel = 1;
constexpr uint8_t kCandles = 5;
constexpr uint8_t kMushrooms = 9;
constexpr uint8_t kAnvil = 12;
constexpr uint8_t kBrazier = 13;
constexpr uint8_t kRack = 14;
constexpr uint8_t kBookshelf = 16;
constexpr uint8_t kBedroll = 17;
constexpr uint8_t kLockedChest = 18;

// Along the side walls, facing into the room; the middle stays clear.
constexpr Furnishing kFurnishings[kBuildings][kMaxFurnishings] = {
    { { { 9, 9 }, kAnvil, 0 }, { { 13, 9 }, kRack, 2 } },
    { { { 9, 9 }, kBrazier, 0 }, { { 13, 9 }, kBookshelf, 2 }, { { 9, 10 }, kMushrooms, 0 } },
    { { { 9, 9 }, kBarrel, 0 }, { { 9, 10 }, kBarrel, 0 }, { { 13, 10 }, kBedroll, 2 } },
    { { { 9, 9 }, kBookshelf, 0 }, { { 13, 9 }, kBookshelf, 2 }, { { 9, 10 }, kCandles, 0 } },
    { { { 9, 9 }, kLockedChest, 0 }, { { 13, 9 }, kLockedChest, 2 } },
    {},
};
constexpr int kFurnishingCounts[kBuildings] = { 2, 3, 3, 3, 2, 0 };

constexpr const char* kKeeperNames[kBuildings] = { "Torvald", "Ingrid", "Gunnar", "Asgeir", "Runa", "" };
constexpr const char* kKeeperAbout[kBuildings] = {
    "The smith. His father forged the door.",
    "Herbs for fever. Forty years late.",
    "Pours drinks. Hears everything.",
    "Halvard's old teacher. Sleeps badly.",
    "Counts coins. Twice.",
    "",
};
constexpr int kKeeperLines = 4;
constexpr const char* kKeeperLineText[kBuildings][kKeeperLines] = {
    { "Torvald: A dull blade gets men killed. Keep it keen.",
      "Torvald: Foundry ore makes the best steel. Bring me some.",
      "Torvald: A hammer opens a cracked wall. So I am told.",
      "Torvald: My father made that door. He never spoke again." },
    { "Ingrid: Cave mushrooms show what is hidden. Sometimes.",
      "Ingrid: Moss, steeped, breaks a fever. We know that now.", "Ingrid: Food over a brazier makes a meal. Eat.",
      "Ingrid: If you meet a woman mired in gold, heal her." },
    { "Gunnar: Ale makes you brave. And clumsy.", "Gunnar: That bog trader owes me twelve meals.",
      "Gunnar: The frogs take names. My father gave them nine.",
      "Gunnar: The council drank here the night they voted." },
    { "Asgeir: Magic comes back as you walk.", "Asgeir: Halvard was my student. Clever. He held the key.",
      "Asgeir: The books down there teach. Read them.", "Asgeir: I voted no. It was not enough. It never is." },
    { "Runa: Gold in the bank stays in the bank.", "Runa: Your shade keeps what I do not.",
      "Runa: Gems and goblets? Puddle buys those.", "Runa: The baron's ledgers are below. I want them back." },
    { "", "", "", "" },
};

}  // namespace

const char* BuildingName(int building)
{
    return building >= 0 && building < kBuildings ? kNames[building] : "";
}

const char* MaterialName(int material)
{
    return material >= 0 && material < kMaterials ? kMaterialNames[material] : "";
}

const char* Effect(int building, int level)
{
    if (building < 0 || building >= kBuildings)
    {
        return "";
    }
    return kEffects[building][level < 0 ? 0 : (level > kMaxLevel ? kMaxLevel : level)];
}

Cost UpgradeCost(int building, int nextLevel)
{
    if (building < 0 || building >= kBuildings || nextLevel < 1 || nextLevel > kMaxLevel)
    {
        return {};
    }
    return kCosts[building][nextLevel - 1];
}

int BossNeeded(int building, int nextLevel)
{
    // The lift goes down past a zone only once its boss is beaten.
    return building == static_cast<int>(Building::Lift) && nextLevel >= 1 && nextLevel <= kMaxLevel
               ? 1 << (nextLevel - 1)
               : 0;
}

int LiftDepth(int level)
{
    return 1 + 5 * (level < 0 ? 0 : (level > kMaxLevel ? kMaxLevel : level));
}

int KeptOnDeath(int level)
{
    constexpr int kKept[kMaxLevel + 1] = { 50, 65, 80, 100 };
    return kKept[level < 0 ? 0 : (level > kMaxLevel ? kMaxLevel : level)];
}

void BuildMap(DungeonMap& map)
{
    map.BuildPlaces(kPlaces, 4, StartCell(), WellCell());
}

int Gloom(Cell c)
{
    const auto of = [](Cell at)
    { return kAlley.Contains(at) || kAlleyNorth.Contains(at) ? 7 : (kYard.Contains(at) ? 4 : 0); };
    if (of(c) != 0 || kPlaza.Contains(c))
    {
        return of(c);
    }
    // A wall: as deep in shadow as what it stands beside, so the line where
    // the floor meets its foot (and the cloth its top) is not left bright.
    int deepest = 0;
    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            const int beside = of({ c.x + dx, c.y + dy });
            deepest = beside > deepest ? beside : deepest;
        }
    }
    return deepest;
}

Cell FenceCell()
{
    return { 2, 9 };
}

const char* FenceName()
{
    return "Ulf";
}

const char* FenceAbout()
{
    return "Sells what the dead dropped.";
}

const char* FenceLine(int index)
{
    return kFenceLines[((index % 4) + 4) % 4];
}

int MaterialPrice(int material)
{
    return material >= 0 && material < kMaterials ? kMaterialPrices[material] : 0;
}

int MaterialDepth(int material)
{
    return material > 0 ? material * 5 + 1 : 1;  // the first floor of its place
}

Cell StoneCell(int stone)
{
    return stone >= 0 && stone < kStones ? kStoneCells[stone] : Cell{ -1, -1 };
}

const char* StoneLine(int stone)
{
    return stone >= 0 && stone < kStones ? kStoneLines[stone] : "";
}

Cell CandlesCell()
{
    return { 5, 3 };
}

Furnishing Dressing(int i)
{
    // A brazier burning by the east houses, crates stacked in a corner.
    constexpr uint8_t kCrates = 20;
    return i == 0 ? Furnishing{ { 16, 10 }, kBrazier, 2 } : Furnishing{ { 8, 12 }, kCrates, 0 };
}

Cell BoardCell()
{
    return { 10, 6 };  // straight ahead from where a visit starts
}

Cell StartCell()
{
    return { 10, 10 };
}

int StartFacing()
{
    return 3;  // north, at the well and the first buildings
}

Cell WellCell()
{
    return { 12, 9 };
}

Cell LiftCell()
{
    return { 15, 11 };  // two steps out from the lift's door
}

Cell BarrelCell(int i)
{
    return i == 0 ? Cell{ 16, 6 } : Cell{ 16, 7 };
}

Cell VillagerCell(int villager)
{
    return villager == 0 ? Cell{ 11, 8 } : Cell{ 14, 11 };
}

const char* VillagerName(int villager)
{
    return villager >= 0 && villager < kVillagers ? kVillagerNames[villager] : "";
}

const char* VillagerAbout(int villager)
{
    return villager >= 0 && villager < kVillagers ? kVillagerAbout[villager] : "";
}

const char* VillagerLine(int villager, int index)
{
    if (villager < 0 || villager >= kVillagers)
    {
        return "";
    }
    return kVillagerLines[villager][((index % kLines) + kLines) % kLines];
}

Cell BuildingCell(int building)
{
    return building >= 0 && building < kBuildings ? kBuildingCells[building] : Cell{ -1, -1 };
}

int BuildingAt(Cell c)
{
    for (int b = 0; b < kBuildings; ++b)
    {
        if (kBuildingCells[b] == c)
        {
            return b;
        }
    }
    return -1;
}

bool CanEnter(int building)
{
    return building >= 0 && building < kBuildings && building != static_cast<int>(Building::Lift);
}

void BuildInterior(DungeonMap& map)
{
    map.BuildPlaza(kInterior, InteriorStart(), Cell{ -1, -1 });
    map.Set(InteriorDoor().x, InteriorDoor().y, Tile::Door);
}

Cell InteriorStart()
{
    return { 11, 11 };
}

int InteriorFacing()
{
    return 3;  // north, at the keeper
}

Cell InteriorDoor()
{
    return { 11, 12 };
}

Cell KeeperCell()
{
    return { 11, 9 };
}

Cell OutsideCell(int building)
{
    const Cell front = BuildingCell(building);
    return { front.x, front.y < kPlaza.y ? front.y + 1 : front.y - 1 };
}

int OutsideFacing(int building)
{
    return BuildingCell(building).y < kPlaza.y ? 1 : 3;
}

int Furnish(int building, const Furnishing*& out)
{
    if (building < 0 || building >= kBuildings)
    {
        out = nullptr;
        return 0;
    }
    out = kFurnishings[building];
    return kFurnishingCounts[building];
}

const char* KeeperName(int building)
{
    return building >= 0 && building < kBuildings ? kKeeperNames[building] : "";
}

const char* KeeperAbout(int building)
{
    return building >= 0 && building < kBuildings ? kKeeperAbout[building] : "";
}

const char* KeeperLine(int building, int index)
{
    if (building < 0 || building >= kBuildings)
    {
        return "";
    }
    return kKeeperLineText[building][((index % kKeeperLines) + kKeeperLines) % kKeeperLines];
}

}  // namespace Dungeon::Village
