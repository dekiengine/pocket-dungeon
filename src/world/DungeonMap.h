#pragma once

// One floor of the dungeon: which tile is where, what the hero can see, and
// the way to the hero for the monsters. Some floors hide a room behind a
// cracked wall (a bomb opens it) or a vault behind a locked door (a key
// opens it), and some have an impossible door: a doorframe standing alone
// in a room that leads into a hall somewhere else, a hall with no other way
// in. Plain data and logic, no engine, so it is tested on its own.

#include <cstdint>

namespace Dungeon
{

constexpr int kMapWidth = 24;
constexpr int kMapHeight = 20;
constexpr int kMaxRooms = 9;
constexpr int kMaxPortals = 2;

enum class Tile : uint8_t
{
    Void,
    Floor,
    Wall,
    Stairs,
    SecretWall,  // a wall with cracks: a hidden room is behind it
    Door,        // a vault's locked door
    EchoWall,    // an echo stone: bump it to hear what it keeps
    LooseBrick,  // looks like any wall; facing it shows a brick to pull
};

/// How a floor tile looks, beyond plain floor.
enum class FloorLook : uint8_t
{
    Plain,
    StairsHole,  // the way down: a hole with steps
    Spikes,      // a trap the hero has found
    Rune,        // the glowing mark under an impossible door
};
constexpr int kFloorLooks = 4;

struct Cell
{
    int x = 0;
    int y = 0;

    bool operator==(const Cell& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Cell& other) const { return !(*this == other); }
};

struct Room
{
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    Cell Centre() const { return { x + w / 2, y + h / 2 }; }
    bool Contains(Cell c) const { return c.x >= x && c.x < x + w && c.y >= y && c.y < y + h; }
};

/// A small room off the dungeon with one way in: a cracked wall or a locked
/// door.
struct Hideout
{
    Room room;
    Cell entrance;
    bool present = false;
};

/// One side of an impossible door: whatever enters `at` heading `dir` (0
/// east, 1 south, 2 west, 3 north) enters `to` instead, heading `toDir`,
/// turned with it. A door is two of these, one each way: in through the
/// frame, and out through the wall behind the hall's entrance.
struct Portal
{
    Cell at;
    int dir = 0;
    Cell to;
    int toDir = 0;
    bool present = false;
};

/// xorshift32: small, fast and the same on every target, so a floor made
/// from a seed is the same floor on the PicoCalc and in the editor.
class Random
{
public:
    explicit Random(uint32_t seed = 0x2545F491u) { Seed(seed); }

    void Seed(uint32_t seed) { m_State = seed != 0 ? seed : 0x2545F491u; }
    uint32_t Next();
    /// A whole number from `low` to `high`, both included.
    int Range(int low, int high);
    /// True `percent` times in a hundred.
    bool Chance(int percent) { return Range(0, 99) < percent; }
    uint32_t State() const { return m_State; }

private:
    uint32_t m_State = 0x2545F491u;
};

class DungeonMap
{
public:
    /// Makes a floor: rooms joined by corridors, walls around the floor, the
    /// hero's start in the first room and the stairs in the room furthest
    /// from it. The same seed and depth make the same floor.
    void Generate(uint32_t seed, int depth);
    /// A floor laid out by hand instead: one open room (a plaza), walls round
    /// it, the hero's start and the stairs where given (none when off the
    /// map).
    void BuildPlaza(const Room& plaza, Cell start, Cell stairs) { BuildPlaces(&plaza, 1, start, stairs); }
    /// The same of several open places that touch (a plaza, an alley off it,
    /// a yard at the alley's end): the first is the plaza. Up to kMaxRooms.
    void BuildPlaces(const Room* places, int count, Cell start, Cell stairs);

    Tile At(int x, int y) const;
    Tile At(Cell c) const { return At(c.x, c.y); }
    void Set(int x, int y, Tile tile);
    static bool InBounds(int x, int y) { return x >= 0 && y >= 0 && x < kMapWidth && y < kMapHeight; }

    /// Floor and stairs can be walked on.
    bool IsWalkable(Cell c) const;
    /// Walls and the void stop sight.
    bool BlocksSight(Cell c) const;

    /// Makes a secret wall or a door into floor.
    void Open(Cell c);

    /// How each floor tile looks, one byte per tile, row by row (FloorLook).
    const uint8_t* GetLooks() const { return &m_Looks[0][0]; }
    FloorLook LookAt(Cell c) const;
    void SetLook(Cell c, FloorLook look);

    /// The room behind a cracked wall, and the vault behind a locked door.
    const Hideout& GetSecretRoom() const { return m_Secret; }
    const Hideout& GetVault() const { return m_Vault; }

    /// The impossible door's way through when entering `c` heading `dir`,
    /// or null.
    const Portal* PortalEntering(Cell c, int dir) const;
    const Portal& GetPortal(int i) const { return m_Portals[i]; }
    /// The hall behind the impossible door (its entrance: where you arrive).
    const Hideout& GetPocket() const { return m_Pocket; }
    /// The neighbouring tile heading `dir` (0 east, 1 south, 2 west, 3 north).
    static Cell Step(Cell c, int dir);

    /// Marks every tile next to floor as seen: a map scroll.
    void RevealAll();

    Cell GetStart() const { return m_Start; }
    Cell GetStairs() const { return m_Stairs; }
    int GetRoomCount() const { return m_RoomCount; }
    const Room& GetRoom(int i) const { return m_Rooms[i]; }

    /// What the hero sees from `from`, up to `radius` tiles, walls stopping
    /// the view. Seen tiles stay seen (the map remembers them).
    void ComputeView(Cell from, int radius);
    bool IsVisible(Cell c) const;
    bool IsSeen(Cell c) const;

    /// Fills the walking distance from `to` to every reachable floor tile,
    /// for the monsters to follow. Tiles it cannot reach get kUnreached.
    void ComputeDistances(Cell to);
    static constexpr uint16_t kUnreached = 0xFFFF;
    uint16_t DistanceAt(Cell c) const;
    /// The neighbouring tile (no diagonals) that is one step nearer the
    /// target of ComputeDistances, or `from` itself when none is.
    Cell StepTowardTarget(Cell from) const;

    /// A random floor tile that is not the start or the stairs, or false
    /// when the floor has none left that `isFree` accepts.
    template <typename IsFree>
    bool RandomFloor(Random& random, IsFree isFree, Cell& out) const;

private:
    Tile m_Tiles[kMapHeight][kMapWidth] = {};
    uint8_t m_Seen[kMapHeight][kMapWidth] = {};
    uint8_t m_Visible[kMapHeight][kMapWidth] = {};
    uint8_t m_Looks[kMapHeight][kMapWidth] = {};
    Hideout m_Secret;
    Hideout m_Vault;
    Hideout m_Pocket;
    Portal m_Portals[kMaxPortals];
    uint16_t m_Distance[kMapHeight][kMapWidth] = {};
    Cell m_Queue[kMapWidth * kMapHeight] = {};
    Room m_Rooms[kMaxRooms] = {};
    int m_RoomCount = 0;
    Cell m_Start;
    Cell m_Stairs;

    void Carve(const Room& room);
    void CarveCorridor(Cell a, Cell b, Random& random);
    void BuildWalls();
    bool CarveHideout(Random& random, Hideout& out);
    bool FindEmptyRoom(Random& random, int minW, int maxW, int minH, int maxH, Room& out) const;
    int CountReachable(Cell from);
    bool PlaceImpossibleDoor(Random& random);
    bool LineOfSight(Cell from, Cell to) const;
};

template <typename IsFree>
bool DungeonMap::RandomFloor(Random& random, IsFree isFree, Cell& out) const
{
    // A few random tries first, then a sweep, so a crowded floor still finds
    // the last free tile.
    for (int tries = 0; tries < 64; ++tries)
    {
        const Cell c{ random.Range(0, kMapWidth - 1), random.Range(0, kMapHeight - 1) };
        if (At(c) == Tile::Floor && c != m_Start && isFree(c))
        {
            out = c;
            return true;
        }
    }
    for (int y = 0; y < kMapHeight; ++y)
    {
        for (int x = 0; x < kMapWidth; ++x)
        {
            const Cell c{ x, y };
            if (At(c) == Tile::Floor && c != m_Start && isFree(c))
            {
                out = c;
                return true;
            }
        }
    }
    return false;
}

}  // namespace Dungeon
