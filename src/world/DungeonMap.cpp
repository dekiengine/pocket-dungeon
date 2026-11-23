#include "DungeonMap.h"

#include <cstdlib>

namespace Dungeon
{

uint32_t Random::Next()
{
    uint32_t x = m_State;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    m_State = x;
    return x;
}

int Random::Range(int low, int high)
{
    if (high <= low)
    {
        return low;
    }
    const uint32_t span = static_cast<uint32_t>(high - low + 1);
    return low + static_cast<int>(Next() % span);
}

Tile DungeonMap::At(int x, int y) const
{
    return InBounds(x, y) ? m_Tiles[y][x] : Tile::Void;
}

void DungeonMap::Set(int x, int y, Tile tile)
{
    if (InBounds(x, y))
    {
        m_Tiles[y][x] = tile;
    }
}

bool DungeonMap::IsWalkable(Cell c) const
{
    const Tile t = At(c);
    return t == Tile::Floor || t == Tile::Stairs;
}

bool DungeonMap::BlocksSight(Cell c) const
{
    return !IsWalkable(c);
}

void DungeonMap::Open(Cell c)
{
    if (At(c) == Tile::SecretWall || At(c) == Tile::Door)
    {
        Set(c.x, c.y, Tile::Floor);
    }
}

FloorLook DungeonMap::LookAt(Cell c) const
{
    return InBounds(c.x, c.y) ? static_cast<FloorLook>(m_Looks[c.y][c.x]) : FloorLook::Plain;
}

void DungeonMap::SetLook(Cell c, FloorLook look)
{
    if (InBounds(c.x, c.y))
    {
        m_Looks[c.y][c.x] = static_cast<uint8_t>(look);
    }
}

void DungeonMap::RevealAll()
{
    for (int y = 0; y < kMapHeight; ++y)
    {
        for (int x = 0; x < kMapWidth; ++x)
        {
            if (m_Tiles[y][x] != Tile::Void)
            {
                m_Seen[y][x] = 1;
            }
        }
    }
}

Cell DungeonMap::Step(Cell c, int dir)
{
    constexpr int kDx[4] = { 1, 0, -1, 0 };
    constexpr int kDy[4] = { 0, 1, 0, -1 };
    return { c.x + kDx[dir & 3], c.y + kDy[dir & 3] };
}

const Portal* DungeonMap::PortalEntering(Cell c, int dir) const
{
    for (const Portal& p : m_Portals)
    {
        if (p.present && p.at == c && p.dir == (dir & 3))
        {
            return &p;
        }
    }
    return nullptr;
}

bool DungeonMap::FindEmptyRoom(Random& random, int minW, int maxW, int minH, int maxH, Room& out) const
{
    // A room in empty space with empty space all round it, so nothing else
    // touches it.
    for (int tries = 0; tries < 80; ++tries)
    {
        Room r;
        r.w = random.Range(minW, maxW);
        r.h = random.Range(minH, maxH);
        r.x = random.Range(1, kMapWidth - r.w - 1);
        r.y = random.Range(1, kMapHeight - r.h - 1);
        bool clear = true;
        for (int y = r.y - 1; y <= r.y + r.h && clear; ++y)
        {
            for (int x = r.x - 1; x <= r.x + r.w; ++x)
            {
                if (!InBounds(x, y) || m_Tiles[y][x] != Tile::Void)
                {
                    clear = false;
                    break;
                }
            }
        }
        if (clear)
        {
            out = r;
            return true;
        }
    }
    return false;
}

int DungeonMap::CountReachable(Cell from)
{
    ComputeDistances(from);
    int count = 0;
    for (int y = 0; y < kMapHeight; ++y)
    {
        for (int x = 0; x < kMapWidth; ++x)
        {
            count += m_Distance[y][x] != kUnreached ? 1 : 0;
        }
    }
    return count;
}

bool DungeonMap::PlaceImpossibleDoor(Random& random)
{
    // The hall: a room in empty space with no way in at all.
    Room hall;
    if (m_RoomCount < 2 || !FindEmptyRoom(random, 3, 6, 3, 5, hall))
    {
        return false;
    }
    // Where you arrive: the middle of one of its sides, heading in.
    const int side = random.Range(0, 3);
    Cell arrive;
    int arriveDir = 0;
    switch (side)
    {
        case 0:
            arrive = { hall.x + hall.w / 2, hall.y };
            arriveDir = 1;
            break;  // top, heading south
        case 1:
            arrive = { hall.x + hall.w / 2, hall.y + hall.h - 1 };
            arriveDir = 3;
            break;  // bottom, north
        case 2:
            arrive = { hall.x, hall.y + hall.h / 2 };
            arriveDir = 0;
            break;  // left, east
        default:
            arrive = { hall.x + hall.w - 1, hall.y + hall.h / 2 };
            arriveDir = 2;
            break;  // right, west
    }

    // The doorframe: a doorway with a wall each side, standing in a room
    // (not the start's) with floor in front and behind, so you can walk
    // round it. It must not cut any of the floor off.
    const Cell start = m_Rooms[0].Centre();
    const int before = CountReachable(start);
    for (int tries = 0; tries < 80; ++tries)
    {
        const Room& room = m_Rooms[random.Range(1, m_RoomCount - 1)];
        const int dir = random.Range(0, 3);
        const bool acrossX = dir == 1 || dir == 3;  // heading north/south: the frame runs along x
        if ((acrossX && (room.w < 5 || room.h < 3)) || (!acrossX && (room.h < 5 || room.w < 3)))
        {
            continue;
        }
        const Cell at{
            acrossX ? random.Range(room.x + 1, room.x + room.w - 2) : random.Range(room.x, room.x + room.w - 1),
            acrossX ? random.Range(room.y, room.y + room.h - 1) : random.Range(room.y + 1, room.y + room.h - 2)
        };
        const Cell sideA = acrossX ? Cell{ at.x - 1, at.y } : Cell{ at.x, at.y - 1 };
        const Cell sideB = acrossX ? Cell{ at.x + 1, at.y } : Cell{ at.x, at.y + 1 };
        const Cell front = Step(at, dir + 2);
        const Cell back = Step(at, dir);
        if (!room.Contains(front) || !room.Contains(back) || at == room.Centre() || sideA == room.Centre() ||
            sideB == room.Centre())
        {
            continue;
        }
        m_Tiles[sideA.y][sideA.x] = Tile::Wall;
        m_Tiles[sideB.y][sideB.x] = Tile::Wall;
        if (CountReachable(start) != before - 2)
        {
            m_Tiles[sideA.y][sideA.x] = Tile::Floor;
            m_Tiles[sideB.y][sideB.x] = Tile::Floor;
            continue;
        }
        Carve(hall);
        m_Pocket.room = hall;
        m_Pocket.entrance = arrive;
        m_Pocket.present = true;
        // The door is the line between the frame's front and its doorway,
        // and the line at the hall's entrance, behind where you arrive. In
        // through the front of the frame; out by walking back through the
        // hall's wall behind the entrance, to the front of the frame.
        m_Portals[0] = { at, dir, arrive, arriveDir, true };
        m_Portals[1] = { Step(arrive, arriveDir + 2), (arriveDir + 2) & 3, front, (dir + 2) & 3, true };
        SetLook(at, FloorLook::Rune);
        return true;
    }
    return false;
}

bool DungeonMap::CarveHideout(Random& random, Hideout& out)
{
    // A small room in empty space, two tiles from the dungeon's floor on one
    // side: the tile between becomes its way in, and every other tile round
    // it a wall.
    for (int tries = 0; tries < 20; ++tries)
    {
        Room r;
        if (!FindEmptyRoom(random, 2, 3, 2, 3, r))
        {
            return false;
        }
        Cell ways[2 * (3 + 3)];
        int count = 0;
        const auto consider = [&](Cell entrance, int dx, int dy)
        {
            // Its way in opens onto the dungeon, not into the folded hall.
            const Cell beyond{ entrance.x + dx, entrance.y + dy };
            if (At(beyond) == Tile::Floor && !(m_Pocket.present && m_Pocket.room.Contains(beyond)))
            {
                ways[count++] = entrance;
            }
        };
        for (int x = r.x; x < r.x + r.w; ++x)
        {
            consider({ x, r.y - 1 }, 0, -1);
            consider({ x, r.y + r.h }, 0, 1);
        }
        for (int y = r.y; y < r.y + r.h; ++y)
        {
            consider({ r.x - 1, y }, -1, 0);
            consider({ r.x + r.w, y }, 1, 0);
        }
        if (count == 0)
        {
            continue;
        }
        Carve(r);
        out.room = r;
        out.entrance = ways[random.Range(0, count - 1)];
        out.present = true;
        return true;
    }
    return false;
}

void DungeonMap::Carve(const Room& room)
{
    for (int y = room.y; y < room.y + room.h; ++y)
    {
        for (int x = room.x; x < room.x + room.w; ++x)
        {
            Set(x, y, Tile::Floor);
        }
    }
}

void DungeonMap::CarveCorridor(Cell a, Cell b, Random& random)
{
    // An L: across then down, or down then across.
    const bool acrossFirst = random.Chance(50);
    Cell at = a;
    const auto stepToward = [](int& v, int target) { v += v < target ? 1 : -1; };
    if (acrossFirst)
    {
        while (at.x != b.x)
        {
            Set(at.x, at.y, Tile::Floor);
            stepToward(at.x, b.x);
        }
    }
    while (at.y != b.y)
    {
        Set(at.x, at.y, Tile::Floor);
        stepToward(at.y, b.y);
    }
    while (at.x != b.x)
    {
        Set(at.x, at.y, Tile::Floor);
        stepToward(at.x, b.x);
    }
    Set(b.x, b.y, Tile::Floor);
}

void DungeonMap::BuildWalls()
{
    // Every empty tile that touches floor (diagonals too) becomes wall.
    for (int y = 0; y < kMapHeight; ++y)
    {
        for (int x = 0; x < kMapWidth; ++x)
        {
            if (m_Tiles[y][x] != Tile::Void)
            {
                continue;
            }
            bool touchesFloor = false;
            for (int dy = -1; dy <= 1 && !touchesFloor; ++dy)
            {
                for (int dx = -1; dx <= 1; ++dx)
                {
                    const Tile t = At(x + dx, y + dy);
                    if (t == Tile::Floor || t == Tile::Stairs)
                    {
                        touchesFloor = true;
                        break;
                    }
                }
            }
            if (touchesFloor)
            {
                m_Tiles[y][x] = Tile::Wall;
            }
        }
    }
}

void DungeonMap::BuildPlaces(const Room* places, int count, Cell start, Cell stairs)
{
    for (int y = 0; y < kMapHeight; ++y)
    {
        for (int x = 0; x < kMapWidth; ++x)
        {
            m_Tiles[y][x] = Tile::Void;
            m_Seen[y][x] = 0;
            m_Visible[y][x] = 0;
            m_Looks[y][x] = 0;
        }
    }
    m_Secret = Hideout{};
    m_Vault = Hideout{};
    m_Pocket = Hideout{};
    for (Portal& p : m_Portals)
    {
        p = Portal{};
    }
    m_RoomCount = 0;
    for (int i = 0; i < count && i < kMaxRooms; ++i)
    {
        m_Rooms[m_RoomCount++] = places[i];
        Carve(places[i]);
    }
    BuildWalls();
    m_Start = start;
    m_Stairs = stairs;
    Set(m_Stairs.x, m_Stairs.y, Tile::Stairs);
    SetLook(m_Stairs, FloorLook::StairsHole);
}

void DungeonMap::Generate(uint32_t seed, int depth)
{
    Random random(seed ^ (static_cast<uint32_t>(depth) * 0x9E3779B9u));
    for (int y = 0; y < kMapHeight; ++y)
    {
        for (int x = 0; x < kMapWidth; ++x)
        {
            m_Tiles[y][x] = Tile::Void;
            m_Seen[y][x] = 0;
            m_Visible[y][x] = 0;
            m_Looks[y][x] = 0;
        }
    }
    m_Secret = Hideout{};
    m_Vault = Hideout{};
    m_Pocket = Hideout{};
    for (Portal& p : m_Portals)
    {
        p = Portal{};
    }

    // Rooms: placed at random, kept one tile apart (room for a wall between)
    // and one tile off the edge (room for the outer wall).
    // Each place (five floors of it) has its own shape of rooms: halls of
    // every size, then many small cells, then a few wide caverns, then long
    // galleries.
    struct Shape
    {
        int rooms[2];
        int w[2];
        int h[2];
    };
    constexpr Shape kShapes[4] = {
        { { 4, 9 }, { 3, 7 }, { 3, 6 } },
        { { 6, 9 }, { 3, 5 }, { 3, 4 } },
        { { 4, 6 }, { 5, 9 }, { 4, 7 } },
        { { 5, 8 }, { 3, 8 }, { 3, 5 } },
    };
    const Shape& shape = kShapes[depth > 0 ? ((depth - 1) / 5) % 4 : 0];
    m_RoomCount = 0;
    const int wanted = random.Range(shape.rooms[0], shape.rooms[1]);
    for (int tries = 0; tries < 200 && m_RoomCount < wanted; ++tries)
    {
        Room room;
        room.w = random.Range(shape.w[0], shape.w[1]);
        room.h = random.Range(shape.h[0], shape.h[1]);
        room.x = random.Range(1, kMapWidth - room.w - 1);
        room.y = random.Range(1, kMapHeight - room.h - 1);
        bool overlaps = false;
        for (int i = 0; i < m_RoomCount && !overlaps; ++i)
        {
            const Room& other = m_Rooms[i];
            overlaps = room.x - 1 < other.x + other.w && room.x + room.w + 1 > other.x &&
                       room.y - 1 < other.y + other.h && room.y + room.h + 1 > other.y;
        }
        if (overlaps)
        {
            continue;
        }
        m_Rooms[m_RoomCount++] = room;
        Carve(room);
    }

    // Each room joined to the one made before it: every room is reachable.
    for (int i = 1; i < m_RoomCount; ++i)
    {
        CarveCorridor(m_Rooms[i - 1].Centre(), m_Rooms[i].Centre(), random);
    }
    // Most floors hide a room; deeper ones may have a locked vault too.
    if (random.Chance(75))
    {
        CarveHideout(random, m_Secret);
    }
    if (depth >= 2 && random.Chance(45))
    {
        CarveHideout(random, m_Vault);
    }
    BuildWalls();
    if (m_Secret.present)
    {
        Set(m_Secret.entrance.x, m_Secret.entrance.y, Tile::SecretWall);
    }
    if (m_Vault.present)
    {
        Set(m_Vault.entrance.x, m_Vault.entrance.y, Tile::Door);
    }

    m_Start = m_Rooms[0].Centre();
    // The stairs go in the room the longest walk from the start.
    ComputeDistances(m_Start);
    int furthest = m_RoomCount - 1;
    uint16_t best = 0;
    for (int i = 1; i < m_RoomCount; ++i)
    {
        const uint16_t d = DistanceAt(m_Rooms[i].Centre());
        if (d != kUnreached && d > best)
        {
            best = d;
            furthest = i;
        }
    }
    m_Stairs = m_Rooms[furthest].Centre();
    Set(m_Stairs.x, m_Stairs.y, Tile::Stairs);
    SetLook(m_Stairs, FloorLook::StairsHole);
}

bool DungeonMap::LineOfSight(Cell from, Cell to) const
{
    // Bresenham from the hero to the tile; anything that blocks sight on the
    // way (not the tile itself: a wall is seen) hides it.
    int x = from.x;
    int y = from.y;
    const int dx = std::abs(to.x - from.x);
    const int dy = -std::abs(to.y - from.y);
    const int sx = from.x < to.x ? 1 : -1;
    const int sy = from.y < to.y ? 1 : -1;
    int err = dx + dy;
    while (x != to.x || y != to.y)
    {
        if ((x != from.x || y != from.y) && BlocksSight({ x, y }))
        {
            return false;
        }
        const int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y += sy;
        }
    }
    return true;
}

void DungeonMap::ComputeView(Cell from, int radius)
{
    for (int y = 0; y < kMapHeight; ++y)
    {
        for (int x = 0; x < kMapWidth; ++x)
        {
            m_Visible[y][x] = 0;
            const int dx = x - from.x;
            const int dy = y - from.y;
            if (dx * dx + dy * dy > radius * radius + radius)
            {
                continue;
            }
            if (LineOfSight(from, { x, y }))
            {
                m_Visible[y][x] = 1;
                m_Seen[y][x] = 1;
            }
        }
    }
}

bool DungeonMap::IsVisible(Cell c) const
{
    return InBounds(c.x, c.y) && m_Visible[c.y][c.x] != 0;
}

bool DungeonMap::IsSeen(Cell c) const
{
    return InBounds(c.x, c.y) && m_Seen[c.y][c.x] != 0;
}

void DungeonMap::ComputeDistances(Cell to)
{
    for (int y = 0; y < kMapHeight; ++y)
    {
        for (int x = 0; x < kMapWidth; ++x)
        {
            m_Distance[y][x] = kUnreached;
        }
    }
    if (!InBounds(to.x, to.y))
    {
        return;
    }
    // Breadth first over the walkable tiles; the queue holds at most every
    // tile once. It is a member, not on the stack: the Pico's is small.
    Cell* queue = m_Queue;
    int head = 0;
    int tail = 0;
    m_Distance[to.y][to.x] = 0;
    queue[tail++] = to;
    constexpr Cell kSteps[4] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    while (head < tail)
    {
        const Cell c = queue[head++];
        const uint16_t next = static_cast<uint16_t>(m_Distance[c.y][c.x] + 1);
        for (const Cell& step : kSteps)
        {
            const Cell n{ c.x + step.x, c.y + step.y };
            if (IsWalkable(n) && m_Distance[n.y][n.x] == kUnreached)
            {
                m_Distance[n.y][n.x] = next;
                queue[tail++] = n;
            }
        }
    }
}

uint16_t DungeonMap::DistanceAt(Cell c) const
{
    return InBounds(c.x, c.y) ? m_Distance[c.y][c.x] : kUnreached;
}

Cell DungeonMap::StepTowardTarget(Cell from) const
{
    constexpr Cell kSteps[4] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    Cell best = from;
    uint16_t bestDistance = DistanceAt(from);
    for (const Cell& step : kSteps)
    {
        const Cell n{ from.x + step.x, from.y + step.y };
        const uint16_t d = DistanceAt(n);
        if (d < bestDistance)
        {
            bestDistance = d;
            best = n;
        }
    }
    return best;
}

}  // namespace Dungeon
