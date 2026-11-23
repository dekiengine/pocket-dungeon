// The floor generator, the hero's view and the monsters' way to the hero,
// over many seeds: a mistake here is a floor with no way to the stairs, a
// monster walking through a wall, or the hero seeing through one.

#include <gtest/gtest.h>

#include "../src/world/DungeonMap.h"

using namespace Dungeon;

namespace
{

constexpr int kSeeds = 500;

}  // namespace

TEST(DungeonMap, TheSameSeedMakesTheSameFloor)
{
    DungeonMap a;
    DungeonMap b;
    a.Generate(1234, 3);
    b.Generate(1234, 3);
    for (int y = 0; y < kMapHeight; ++y)
    {
        for (int x = 0; x < kMapWidth; ++x)
        {
            ASSERT_EQ(a.At(x, y), b.At(x, y));
        }
    }
    EXPECT_EQ(a.GetStairs(), b.GetStairs());
}

TEST(DungeonMap, EveryFloorHasAWayFromTheStartToTheStairs)
{
    DungeonMap map;
    for (uint32_t seed = 1; seed <= kSeeds; ++seed)
    {
        map.Generate(seed, static_cast<int>(seed % 10));
        ASSERT_GE(map.GetRoomCount(), 2) << "seed " << seed;
        ASSERT_EQ(map.At(map.GetStart()), Tile::Floor) << "seed " << seed;
        ASSERT_EQ(map.At(map.GetStairs()), Tile::Stairs) << "seed " << seed;
        ASSERT_NE(map.GetStart(), map.GetStairs()) << "seed " << seed;
        map.ComputeDistances(map.GetStart());
        ASSERT_NE(map.DistanceAt(map.GetStairs()), DungeonMap::kUnreached) << "seed " << seed;
    }
}

TEST(DungeonMap, FloorIsEnclosedByWallsAndNeverTouchesTheEdge)
{
    DungeonMap map;
    for (uint32_t seed = 1; seed <= kSeeds; ++seed)
    {
        map.Generate(seed, 1);
        for (int y = 0; y < kMapHeight; ++y)
        {
            for (int x = 0; x < kMapWidth; ++x)
            {
                if (!map.IsWalkable({ x, y }))
                {
                    continue;
                }
                ASSERT_TRUE(x > 0 && y > 0 && x < kMapWidth - 1 && y < kMapHeight - 1)
                    << "seed " << seed << " floor on the edge at " << x << "," << y;
                for (int dy = -1; dy <= 1; ++dy)
                {
                    for (int dx = -1; dx <= 1; ++dx)
                    {
                        ASSERT_NE(map.At(x + dx, y + dy), Tile::Void) << "seed " << seed << " at " << x << "," << y;
                    }
                }
            }
        }
    }
}

TEST(DungeonMap, AStepTowardTheTargetIsAlwaysOneNearerAndWalkable)
{
    DungeonMap map;
    for (uint32_t seed = 1; seed <= 100; ++seed)
    {
        map.Generate(seed, 2);
        map.ComputeDistances(map.GetStairs());
        Cell at = map.GetStart();
        int steps = 0;
        while (at != map.GetStairs())
        {
            const Cell next = map.StepTowardTarget(at);
            ASSERT_NE(next, at) << "seed " << seed << " stuck";
            ASSERT_TRUE(map.IsWalkable(next));
            ASSERT_EQ(std::abs(next.x - at.x) + std::abs(next.y - at.y), 1);
            ASSERT_EQ(map.DistanceAt(next) + 1, map.DistanceAt(at));
            at = next;
            ASSERT_LT(++steps, kMapWidth * kMapHeight);
        }
    }
}

TEST(DungeonMap, WallsStopTheViewAndSeenTilesAreRemembered)
{
    DungeonMap map;
    for (int y = 0; y < kMapHeight; ++y)
    {
        for (int x = 0; x < kMapWidth; ++x)
        {
            map.Set(x, y, Tile::Floor);
        }
    }
    // A wall across the middle with the hero on the left of it.
    for (int y = 0; y < kMapHeight; ++y)
    {
        map.Set(10, y, Tile::Wall);
    }
    map.ComputeView({ 8, 8 }, 5);
    EXPECT_TRUE(map.IsVisible({ 8, 8 }));
    EXPECT_TRUE(map.IsVisible({ 6, 8 }));
    EXPECT_TRUE(map.IsVisible({ 10, 8 }));   // the wall itself is seen
    EXPECT_FALSE(map.IsVisible({ 11, 8 }));  // behind it is not
    EXPECT_FALSE(map.IsVisible({ 2, 8 }));   // too far
    map.ComputeView({ 3, 3 }, 5);
    EXPECT_FALSE(map.IsVisible({ 8, 8 }));
    EXPECT_TRUE(map.IsSeen({ 8, 8 }));
}

TEST(DungeonMap, RandomFloorSkipsTheStartAndWhatIsTaken)
{
    DungeonMap map;
    map.Generate(77, 1);
    Random random(5);
    for (int i = 0; i < 50; ++i)
    {
        Cell c;
        ASSERT_TRUE(map.RandomFloor(random, [](Cell) { return true; }, c));
        EXPECT_EQ(map.At(c), Tile::Floor);
        EXPECT_NE(c, map.GetStart());
    }
    Cell none;
    EXPECT_FALSE(map.RandomFloor(random, [](Cell) { return false; }, none));
}

TEST(Random, RangeStaysInsideItsBounds)
{
    Random random(42);
    for (int i = 0; i < 10000; ++i)
    {
        const int v = random.Range(3, 7);
        ASSERT_GE(v, 3);
        ASSERT_LE(v, 7);
    }
}

// A hideout is a room with one way in, a cracked wall or a locked door, that
// the start cannot reach until that way is opened.
TEST(DungeonMap, HideoutsHaveOneWayInAndOpenOnlyThroughIt)
{
    int secrets = 0;
    int vaults = 0;
    for (uint32_t seed = 1; seed <= 150; ++seed)
    {
        for (int depth = 1; depth <= 4; ++depth)
        {
            DungeonMap map;
            map.Generate(seed, depth);
            EXPECT_EQ(map.LookAt(map.GetStairs()), FloorLook::StairsHole);
            for (const Hideout* h : { &map.GetSecretRoom(), &map.GetVault() })
            {
                if (!h->present)
                {
                    continue;
                }
                const bool vault = h == &map.GetVault();
                (vault ? vaults : secrets) += 1;
                EXPECT_EQ(map.At(h->entrance), vault ? Tile::Door : Tile::SecretWall) << "seed " << seed;
                const Cell inside = { h->room.x, h->room.y };
                map.ComputeDistances(map.GetStart());
                EXPECT_EQ(map.DistanceAt(inside), DungeonMap::kUnreached) << "seed " << seed;
                map.Open(h->entrance);
                map.ComputeDistances(map.GetStart());
                EXPECT_NE(map.DistanceAt(inside), DungeonMap::kUnreached) << "seed " << seed;
            }
        }
    }
    EXPECT_GT(secrets, 200) << "most floors hide a room";
    EXPECT_GT(vaults, 50);
}

// An impossible door: a frame standing in a room, walls either side, its two
// ways mirror each other, and the hall behind it has no other way in.
TEST(DungeonMap, EachPlaceHasItsOwnShapeOfRooms)
{
    // Many small cells in the second place, a few wide caverns in the third.
    int cells = 0;
    int caverns = 0;
    int cellArea = 0;
    int cavernArea = 0;
    for (uint32_t seed = 1; seed <= 40; ++seed)
    {
        DungeonMap small;
        small.Generate(seed, 7);
        DungeonMap wide;
        wide.Generate(seed, 12);
        EXPECT_FALSE(small.GetPocket().present);
        for (int i = 0; i < small.GetRoomCount(); ++i)
        {
            ++cells;
            cellArea += small.GetRoom(i).w * small.GetRoom(i).h;
        }
        for (int i = 0; i < wide.GetRoomCount(); ++i)
        {
            ++caverns;
            cavernArea += wide.GetRoom(i).w * wide.GetRoom(i).h;
        }
    }
    EXPECT_GT(cells, caverns) << "more rooms where they are small";
    EXPECT_GT(cavernArea / caverns, 2 * (cellArea / cells)) << "and far bigger ones where they are few";
}
