#pragma once

// The game's sounds, made as they play: three voices of square, triangle and
// noise, as an old console's sound chip would. No samples, no files: each
// sound is a few notes in a table, and Render fills a buffer with what the
// voices are playing. No engine: whatever plays audio asks Render for more.
//
// Play may be called from the game while Render runs from an audio callback
// or an interrupt: the sounds asked for wait in a small ring that one side
// only writes and the other only reads.

#include <atomic>
#include <cstdint>

namespace Dungeon::Chip
{

enum class Sfx : uint8_t
{
    Step,
    Bump,  // a wall, or nothing to do
    Hit,
    Critical,
    Miss,
    Hurt,
    Kill,
    Pickup,
    Coin,
    Unwrap,  // something shut, coming open
    Reveal,  // what was in it
    Smash,
    LevelUp,
    Door,
    Stairs,
    Fall,
    Echo,
    Talk,  // a letter of someone speaking
    MenuMove,
    MenuSelect,
    MenuClose,
    Spell,
    Trap,
    Drink,
    Eat,
    Chime,  // a prayer, a blessing
    Dig,
    Lock,
    Scrape,  // a blade on a stone
    Buy,
    Error,
    Heal,
    Warp,  // the floor folding
    Bell,
    Roar,
    Home,
    NewFloor,
    Heartbeat,
    Drip,
    Wind,
    Crackle,
    Birds,
    Swing,    // a weapon through the air
    Clang,    // on stone that does not give
    Crack,    // on a wall that does
    Crumble,  // and the wall coming down
    Stumble,
};
constexpr int kSounds = static_cast<int>(Sfx::Stumble) + 1;

/// A tune that plays under the sounds, round and round, on voices of its own.
enum class Music : uint8_t
{
    None,
    Title,
};

class Player
{
public:
    static constexpr int kVoices = 3;

    /// Asks for a sound: it starts with the next Render. One too many at
    /// once is dropped.
    void Play(Sfx sfx);

    /// The tune to play from now on (from its start), or None for none.
    /// Asking again for the one playing changes nothing.
    void SetMusic(Music music) { m_TuneAsked.store(static_cast<uint8_t>(music), std::memory_order_relaxed); }

    /// 0 (silent) to 100. Boards with a speaker start low.
    void SetVolume(int percent);
    int GetVolume() const { return m_Volume.load(std::memory_order_relaxed); }

    /// Fills `out` with `frames` samples of what is playing, signed 16-bit
    /// mono at `sampleRate` a second; silence when nothing is.
    void Render(int16_t* out, int frames, int sampleRate);

    /// True while any voice sounds or a sound waits to start.
    bool IsPlaying() const;

private:
    struct Voice
    {
        const struct Note* note = nullptr;  // the note sounding, null: silent
        uint8_t left = 0;                   // notes of its sound after this one
        uint32_t phase = 0;
        uint32_t step = 0;        // phase a sample, for the pitch now
        int32_t samplesLeft = 0;  // of this note
        int32_t samples = 1;      // this note's length
        uint16_t noise = 0x4A3D;  // the noise's shift register
        int16_t last = 0;         // the noise's level until it steps again
        uint8_t order = 0;        // when it started: the oldest is taken first
        // A tune's part: its first note and how many, to go round again.
        const struct Note* first = nullptr;
        uint8_t count = 0;
    };
    static constexpr int kTuneVoices = 2;

    static constexpr int kPending = 8;
    std::atomic<uint8_t> m_Asked{ 0 };  // written by Play
    std::atomic<uint8_t> m_Taken{ 0 };  // written by Render
    uint8_t m_Pending[kPending] = {};
    std::atomic<int> m_Volume{ 50 };
    Voice m_Voices[kVoices];
    uint8_t m_Order = 0;
    std::atomic<uint8_t> m_TuneAsked{ 0 };
    uint8_t m_TunePlaying = 0;  // Render's
    Voice m_TuneVoices[kTuneVoices];

    void StartTune(Music music, int sampleRate, uint32_t turnPerHz);
    static void RenderVoice(Voice& voice, int16_t* out, int frames, int sampleRate, int volume, uint32_t turnPerHz);

    void Start(Sfx sfx, int sampleRate, uint32_t turnPerHz);
    static void Tune(Voice& voice, int hz, uint32_t turnPerHz);
};

}  // namespace Dungeon::Chip
