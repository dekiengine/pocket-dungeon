// The texture cache: copies into the RAM it is given, and leaves a texture
// where it is when that is full.

#include "../src/render/TextureCache.h"

#include <gtest/gtest.h>

#include <cstring>

using namespace Dungeon;

namespace
{

constexpr uint8_t kIndex[16] = { 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3 };
constexpr uint16_t kPalette[4] = { kTransparent, 0x1111, 0x2222, 0x3333 };
constexpr Texture kSmall{ kIndex, kPalette, 4, 1, 4 };

}  // namespace

TEST(TextureCacheTests, CopiesIntoTheRoomGiven)
{
    uint8_t room[64] = {};
    TextureCache cache;
    cache.SetStorage(room, sizeof(room));
    const Texture* held = cache.Hold(&kSmall);
    ASSERT_NE(held, &kSmall);
    EXPECT_GE(held->index, room);
    EXPECT_LT(held->index, room + sizeof(room));
    EXPECT_EQ(std::memcmp(held->index, kIndex, sizeof(kIndex)), 0);
    EXPECT_EQ(held->palette[3], 0x3333);
    EXPECT_EQ(held->size, 4);
    // The same texture again: the same copy, no more room taken.
    const size_t used = cache.GetUsed();
    EXPECT_EQ(cache.Hold(&kSmall), held);
    EXPECT_EQ(cache.GetUsed(), used);
}

TEST(TextureCacheTests, LeavesItWhereItIsWhenFull)
{
    uint8_t room[20] = {};  // the pixels fit, the palette does not
    TextureCache cache;
    cache.SetStorage(room, sizeof(room));
    EXPECT_EQ(cache.Hold(&kSmall), &kSmall);
    EXPECT_EQ(cache.GetUsed(), 0u);
    // A live palette is not copied, so the pixels alone fit.
    const Texture* held = cache.Hold(&kSmall, true);
    ASSERT_NE(held, &kSmall);
    EXPECT_EQ(held->palette, kPalette);
}

TEST(TextureCacheTests, NothingHeldWithoutRoom)
{
    TextureCache cache;
    EXPECT_EQ(cache.Hold(&kSmall), &kSmall);
    EXPECT_EQ(cache.Hold(nullptr), nullptr);
}
