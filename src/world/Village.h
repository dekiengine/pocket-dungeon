#pragma once

// The village above the dungeon, where every run starts and ends: a plaza at
// dusk with six buildings round it, each a ruin until built, then upgraded
// twice more with gold and the materials the dungeon gives: moss from the
// mossy halls, bone from the catacombs, shell from the flooded caves, ore from
// the forge. Deeper zones' materials for the later levels, so the village
// grows only as fast as the dives get deeper. No engine: rules and layout.

#include "DungeonMap.h"

#include <cstdint>

namespace Dungeon::Village
{

enum class Building : uint8_t
{
    Blacksmith,  // a better start: attack
    Apothecary,  // potions to start with
    Tavern,      // max HP
    Tower,       // max MP
    Bank,        // gold kept when you die
    Lift,        // start deeper, once that zone's boss is beaten
};
constexpr int kBuildings = 6;
constexpr int kMaxLevel = 3;
// One material per zone, in zone order.
constexpr int kMaterials = 4;

const char* BuildingName(int building);
const char* MaterialName(int material);

/// What a building at `level` does, in a few words ("Start: Atk +2").
const char* Effect(int building, int level);

/// What the next level costs: gold, and an amount of one material.
struct Cost
{
    int gold = 0;
    int material = 0;
    int amount = 0;
};
Cost UpgradeCost(int building, int nextLevel);
/// The boss (a bit of Legend::bosses) the lift's next level needs beaten
/// first, or 0.
int BossNeeded(int building, int nextLevel);

/// The depth the lift goes down to at `level` (1 when not built).
int LiftDepth(int level);
/// Gold kept when dying, in percent, with the bank at `level`.
int KeptOnDeath(int level);

/// A thing standing inside a building: where, which prop (a kPropInfo index,
/// whatever the zone) and the way its front faces.
struct Furnishing
{
    Cell at;
    uint8_t prop = 0;
    uint8_t facing = 1;
};
/// The village's map: the plaza, walls round it, the well into the dungeon
/// and the lift's platform; off its west side a dark alley between the
/// houses, a man at its end who sells what he should not have, and past him
/// the yard of stones for those who have no graves.
void BuildMap(DungeonMap& map);
/// How deep in shadow a tile is: 0 in the open, more in the alley (eighths
/// of the way to dark).
int Gloom(Cell c);

/// The man in the alley: he sells the dungeon's materials for the village's
/// gold, those of the places the hero has been as deep as.
Cell FenceCell();
const char* FenceName();
const char* FenceAbout();
const char* FenceLine(int index);
/// What one of a material costs from him, and the depth that must have
/// been reached for him to have it.
int MaterialPrice(int material);
int MaterialDepth(int material);

/// The yard: stones with a line each, and candles someone keeps lit.
constexpr int kStones = 3;
Cell StoneCell(int stone);
const char* StoneLine(int stone);
Cell CandlesCell();
/// The notice board in the square: who wants what.
Cell BoardCell();
/// What else stands in the square, only to look at: a lit brazier, crates.
constexpr int kDressings = 2;
Furnishing Dressing(int i);
Cell StartCell();
int StartFacing();
Cell WellCell();
Cell LiftCell();
Cell BarrelCell(int i);  // two beer barrels by the tavern

/// The two villagers: where they stand, what they are called and say.
constexpr int kVillagers = 2;
Cell VillagerCell(int villager);
const char* VillagerName(int villager);
const char* VillagerAbout(int villager);
/// Their lines, in turn: tips on how the village and the dungeon work.
const char* VillagerLine(int villager, int index);
/// The wall tile each building's front is on, and the building at a tile
/// (-1 for none).
Cell BuildingCell(int building);
int BuildingAt(Cell c);

/// Built buildings can be walked into, all but the lift (which is ridden).
bool CanEnter(int building);
/// Inside a building: one room, the door behind the hero, its keeper at the
/// back. The same room for every building, furnished differently.
void BuildInterior(DungeonMap& map);
Cell InteriorStart();
int InteriorFacing();
Cell InteriorDoor();
Cell KeeperCell();
/// Where the hero stands on leaving a building, and the way they face: out
/// into the plaza, the front behind them.
Cell OutsideCell(int building);
int OutsideFacing(int building);

constexpr int kMaxFurnishings = 4;
/// The building's furnishings; returns how many.
int Furnish(int building, const Furnishing*& out);

/// Each building's keeper: who they are, and what they say in turn once
/// there is nothing left to build.
const char* KeeperName(int building);
const char* KeeperAbout(int building);
const char* KeeperLine(int building, int index);

}  // namespace Dungeon::Village
