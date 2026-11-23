// The game's sounds off the device: the chip's voices make sound when asked
// and fall silent after, never louder than 16 bits hold, and the game's
// changes ask for the right ones.

#include <gtest/gtest.h>

#include "../src/sound/Chip.h"
#include "../src/sound/SoundCues.h"
#include "Walk.h"

#include <cstdlib>
#include <vector>

using namespace Dungeon;
using namespace Dungeon::TestWalk;

namespace
{

constexpr int kRate = 22050;

/// The loudest sample of the next `frames` the player renders.
int Loudest(Chip::Player& player, int frames)
{
    std::vector<int16_t> out(static_cast<size_t>(frames));
    player.Render(out.data(), frames, kRate);
    int loudest = 0;
    for (const int16_t sample : out)
    {
        loudest = std::abs(sample) > loudest ? std::abs(sample) : loudest;
    }
    return loudest;
}

}  // namespace

TEST(Sound, NothingAskedIsSilence)
{
    Chip::Player player;
    EXPECT_EQ(Loudest(player, 2000), 0);
    EXPECT_FALSE(player.IsPlaying());
}

TEST(Sound, TheTitlesTuneGoesRoundUntilToldToStop)
{
    Chip::Player player;
    player.SetMusic(Chip::Music::Title);
    // Half a minute of it: longer than the tune, so it has gone round, and
    // every second of it has sound in it. Never loud.
    for (int second = 0; second < 30; ++second)
    {
        const int loudest = Loudest(player, kRate);
        EXPECT_GT(loudest, 0) << "second " << second;
        EXPECT_LT(loudest, 12000) << "second " << second;
    }
    EXPECT_TRUE(player.IsPlaying());
    // A sound over it is still heard, and stopping it leaves silence.
    player.Play(Chip::Sfx::Coin);
    EXPECT_GT(Loudest(player, 2000), 0);
    player.SetMusic(Chip::Music::None);
    Loudest(player, kRate);
    EXPECT_EQ(Loudest(player, 2000), 0);
    EXPECT_FALSE(player.IsPlaying());
}

TEST(Sound, EverySoundIsHeardAndThenEnds)
{
    for (int s = 0; s < Chip::kSounds; ++s)
    {
        Chip::Player player;
        player.SetVolume(50);
        player.Play(static_cast<Chip::Sfx>(s));
        EXPECT_TRUE(player.IsPlaying());
        EXPECT_GT(Loudest(player, kRate / 50), 200) << "sound " << s;
        // Two seconds on, the longest is over.
        Loudest(player, 2 * kRate);
        EXPECT_FALSE(player.IsPlaying()) << "sound " << s;
        EXPECT_EQ(Loudest(player, 500), 0) << "sound " << s;
    }
}

TEST(Sound, TheVolumeScalesItAndZeroIsSilent)
{
    Chip::Player quiet;
    quiet.SetVolume(10);
    quiet.Play(Chip::Sfx::Hurt);
    Chip::Player loud;
    loud.SetVolume(100);
    loud.Play(Chip::Sfx::Hurt);
    const int low = Loudest(quiet, 800);
    const int high = Loudest(loud, 800);
    EXPECT_GT(low, 0);
    EXPECT_GT(high, low * 5);
    Chip::Player off;
    off.SetVolume(0);
    off.Play(Chip::Sfx::Hurt);
    EXPECT_EQ(Loudest(off, 800), 0);
}

TEST(Sound, ThreeLoudVoicesAtOnceAreHeldAtTheTopNotWrapped)
{
    Chip::Player player;
    player.SetVolume(100);
    // More than the voices there are, and more than the ring holds.
    for (int i = 0; i < 20; ++i)
    {
        player.Play(i % 2 == 0 ? Chip::Sfx::Smash : Chip::Sfx::LevelUp);
    }
    // Held at the top: a sum that wrapped round would come out small or
    // jump from one end to the other between two samples of a square wave.
    std::vector<int16_t> out(4000);
    player.Render(out.data(), 4000, kRate);
    int loudest = 0;
    for (const int16_t sample : out)
    {
        loudest = std::abs(sample) > loudest ? std::abs(sample) : loudest;
    }
    EXPECT_GT(loudest, 20000);
}

TEST(Sound, AStepABlowAndAMenuAskForTheirSounds)
{
    CrawlerGame game;
    game.NewRun(3);
    SoundCues cues;
    cues.Reset(game);
    Chip::Player player;
    // Nothing changed: nothing sounds.
    cues.Update(game, nullptr, 0, 0, 0, nullptr, 0, 16, player);
    EXPECT_FALSE(player.IsPlaying());
    // The menu opens.
    game.Act(Action::OpenMenu);
    cues.Update(game, nullptr, 0, 0, 0, nullptr, 0, 16, player);
    EXPECT_TRUE(player.IsPlaying());
    Loudest(player, 2 * kRate);
    // A line down.
    game.Act(Action::MenuDown);
    cues.Update(game, nullptr, 0, 0, 0, nullptr, 0, 16, player);
    EXPECT_TRUE(player.IsPlaying());
    Loudest(player, 2 * kRate);
    game.Act(Action::MenuClose);
    cues.Update(game, nullptr, 0, 0, 0, nullptr, 0, 16, player);
    Loudest(player, 2 * kRate);
    // A blow that landed.
    uint8_t heard[8] = {};
    heard[static_cast<int>(EventKind::Hit)] = 1;
    cues.Update(game, heard, 0, 0, 0, nullptr, 0, 16, player);
    EXPECT_TRUE(player.IsPlaying());
}

TEST(Sound, WhatTheGameAsksForByNameIsPlayed)
{
    CrawlerGame game;
    game.NewRun(3);
    SoundCues cues;
    cues.Reset(game);
    Chip::Player player;
    const Cue asked[2] = { Cue::Lock, Cue::Bell };
    cues.Update(game, nullptr, 0, 0, 0, asked, 2, 16, player);
    EXPECT_TRUE(player.IsPlaying());
}

TEST(Sound, APlaceMakesItsOwnNoiseNowAndThen)
{
    CrawlerGame game;
    game.NewRun(3);
    SoundCues cues;
    cues.Reset(game);
    Chip::Player player;
    // Nothing happens for twenty seconds: something is heard all the same.
    bool heard = false;
    for (int frame = 0; frame < 600 && !heard; ++frame)
    {
        cues.Update(game, nullptr, 0, 0, 0, nullptr, 0, 33, player);
        heard = player.IsPlaying();
    }
    EXPECT_TRUE(heard);
}
