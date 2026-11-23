#include "Chip.h"

#include "../Hot.h"

namespace Dungeon::Chip
{

enum class Wave : uint8_t
{
    Square,
    Triangle,
    Noise,
};

// One note of a sound: a wave at a pitch that slides to another, for so
// long, at a loudness of 0 to 15 that either holds or dies away.
struct Note
{
    Wave wave;
    uint16_t hz;    // for noise: how often it changes, a second
    uint16_t toHz;  // where the pitch has slid to by the note's end
    uint8_t ms10;   // its length, in hundredths of a second
    uint8_t loud;
    bool fades;
};

namespace
{

constexpr Wave Sq = Wave::Square;
constexpr Wave Tr = Wave::Triangle;
constexpr Wave No = Wave::Noise;

// Each sound: its notes, played one after another on one voice.
constexpr Note kStep[] = { { No, 260, 180, 3, 3, true } };
constexpr Note kBump[] = { { Tr, 90, 70, 6, 6, true } };
constexpr Note kHit[] = { { No, 1400, 300, 7, 10, true } };
constexpr Note kCritical[] = { { Sq, 880, 1760, 5, 9, false }, { No, 2400, 300, 12, 11, true } };
constexpr Note kMiss[] = { { No, 5200, 2200, 6, 4, true } };
constexpr Note kHurt[] = { { Sq, 240, 110, 14, 10, true } };
constexpr Note kKill[] = { { No, 900, 120, 10, 9, true }, { Tr, 196, 98, 14, 8, true } };
constexpr Note kPickup[] = { { Sq, 880, 880, 4, 6, false }, { Sq, 1320, 1320, 7, 6, true } };
constexpr Note kCoin[] = { { Sq, 988, 988, 5, 7, false }, { Sq, 1319, 1319, 14, 7, true } };
constexpr Note kUnwrap[] = { { Tr, 392, 494, 5, 6, false },
                             { Tr, 494, 587, 5, 6, false },
                             { Tr, 587, 784, 6, 6, false },
                             { Tr, 784, 1175, 8, 6, true } };
constexpr Note kReveal[] = { { Sq, 523, 523, 6, 7, false },
                             { Sq, 659, 659, 6, 7, false },
                             { Sq, 784, 784, 6, 7, false },
                             { Sq, 1047, 1047, 18, 7, true } };
constexpr Note kSmash[] = { { No, 3200, 400, 16, 11, true } };
constexpr Note kLevelUp[] = { { Sq, 523, 523, 8, 8, false },
                              { Sq, 659, 659, 8, 8, false },
                              { Sq, 784, 784, 8, 8, false },
                              { Sq, 1047, 1047, 8, 8, false },
                              { Sq, 1319, 1319, 26, 8, true } };
constexpr Note kDoor[] = { { Tr, 130, 82, 16, 8, true } };
constexpr Note kStairs[] = { { No, 300, 200, 6, 6, true },
                             { No, 260, 170, 6, 6, true },
                             { No, 220, 140, 6, 6, true },
                             { No, 180, 110, 9, 6, true } };
constexpr Note kFall[] = { { Tr, 440, 55, 70, 9, true } };
constexpr Note kEcho[] = { { Tr, 330, 330, 30, 5, true },
                           { Tr, 392, 392, 30, 5, true },
                           { Tr, 294, 294, 55, 5, true } };
constexpr Note kTalk[] = { { Sq, 520, 480, 2, 3, false } };
constexpr Note kMenuMove[] = { { Sq, 660, 660, 2, 4, false } };
constexpr Note kMenuSelect[] = { { Sq, 880, 880, 3, 5, false }, { Sq, 1320, 1320, 5, 5, true } };
constexpr Note kMenuClose[] = { { Sq, 660, 440, 5, 4, true } };
constexpr Note kSpell[] = { { Sq, 500, 1500, 22, 7, true } };
constexpr Note kTrap[] = { { Sq, 160, 150, 8, 9, false }, { Sq, 120, 110, 18, 9, true } };
constexpr Note kDrink[] = { { Tr, 300, 420, 6, 6, false },
                            { Tr, 280, 400, 6, 6, false },
                            { Tr, 260, 380, 9, 6, true } };
constexpr Note kEat[] = { { No, 700, 500, 5, 6, true }, { No, 650, 450, 5, 6, true }, { No, 600, 400, 7, 6, true } };
constexpr Note kChime[] = { { Tr, 784, 784, 12, 6, true },
                            { Tr, 1175, 1175, 12, 6, true },
                            { Tr, 1568, 1568, 30, 6, true } };
constexpr Note kDig[] = { { No, 420, 200, 8, 7, true }, { No, 380, 180, 8, 7, true } };
constexpr Note kLock[] = { { Sq, 1900, 1900, 2, 6, false },
                           { No, 900, 500, 4, 5, true },
                           { Sq, 1300, 1300, 3, 6, true } };
constexpr Note kScrape[] = { { No, 6000, 7500, 14, 5, false }, { No, 7000, 8500, 16, 5, true } };
constexpr Note kBuy[] = { { Sq, 1319, 1319, 4, 7, false },
                          { Sq, 1568, 1568, 4, 7, false },
                          { Sq, 2093, 2093, 14, 7, true } };
constexpr Note kError[] = { { Sq, 150, 150, 8, 7, false }, { Sq, 110, 110, 14, 7, true } };
constexpr Note kHeal[] = { { Tr, 523, 784, 10, 7, false },
                           { Tr, 784, 1047, 10, 7, false },
                           { Tr, 1047, 1568, 22, 7, true } };
constexpr Note kWarp[] = { { Sq, 1400, 200, 18, 7, false }, { Sq, 200, 1400, 22, 7, true } };
constexpr Note kBell[] = { { Tr, 1760, 1760, 25, 9, true },
                           { Tr, 1760, 1760, 25, 8, true },
                           { Tr, 1760, 1760, 45, 7, true } };
constexpr Note kRoar[] = { { No, 180, 90, 30, 11, false }, { Sq, 70, 45, 50, 10, true } };
constexpr Note kHome[] = { { Sq, 392, 392, 10, 7, false },
                           { Sq, 523, 523, 10, 7, false },
                           { Sq, 659, 659, 10, 7, false },
                           { Sq, 784, 784, 34, 7, true } };
constexpr Note kNewFloor[] = { { Tr, 220, 220, 16, 6, true },
                               { Tr, 165, 165, 16, 6, true },
                               { Tr, 110, 110, 40, 6, true } };
constexpr Note kHeartbeat[] = { { Tr, 70, 55, 9, 9, true }, { Tr, 0, 0, 7, 0, false }, { Tr, 65, 50, 12, 8, true } };
constexpr Note kDrip[] = { { Tr, 1500, 900, 4, 4, true } };
constexpr Note kWind[] = { { No, 300, 700, 60, 2, false }, { No, 700, 250, 80, 2, true } };
constexpr Note kCrackle[] = { { No, 2500, 900, 3, 4, true },
                              { No, 0, 0, 5, 0, false },
                              { No, 3200, 1200, 3, 4, true } };
constexpr Note kBirds[] = { { Sq, 2400, 2900, 5, 3, true }, { Sq, 0, 0, 6, 0, false }, { Sq, 2600, 3200, 6, 3, true } };
constexpr Note kSwing[] = { { No, 2600, 6500, 5, 4, false }, { No, 6500, 1800, 8, 5, true } };
constexpr Note kClang[] = { { Sq, 1480, 1400, 3, 8, false }, { Tr, 1480, 1320, 16, 7, true } };
constexpr Note kCrack[] = { { No, 1800, 500, 5, 10, true }, { Tr, 160, 90, 10, 8, true } };
constexpr Note kCrumble[] = { { No, 1200, 300, 10, 11, false },
                              { No, 500, 140, 22, 10, true },
                              { Tr, 110, 55, 30, 9, true } };
constexpr Note kStumble[] = { { Tr, 330, 220, 6, 5, false }, { Tr, 250, 170, 9, 5, true } };

// The title's tune: D minor, slow, eight bars of eight quavers. A plucked
// line on top, the chords spread out under it.
constexpr uint8_t kQuaver = 30;  // hundredths of a second
#define DUNGEON_TOP(hz, quavers) { Sq, hz, hz, static_cast<uint8_t>(kQuaver * (quavers)), 4, true }
#define DUNGEON_REST(quavers) { Sq, 0, 0, static_cast<uint8_t>(kQuaver * (quavers)), 0, false }
constexpr Note kTitleTop[] = {
    DUNGEON_TOP(587, 4),  DUNGEON_TOP(880, 4),                                             // D   A
    DUNGEON_TOP(932, 2),  DUNGEON_TOP(880, 2),  DUNGEON_TOP(784, 4),                       // Bb A G
    DUNGEON_TOP(698, 4),  DUNGEON_TOP(784, 2),  DUNGEON_TOP(880, 2),                       // F  G A
    DUNGEON_TOP(659, 6),  DUNGEON_REST(2),                                                 // E
    DUNGEON_TOP(587, 4),  DUNGEON_TOP(880, 4),                                             // D   A
    DUNGEON_TOP(1175, 2), DUNGEON_TOP(1047, 2), DUNGEON_TOP(932, 4),                       // D  C Bb
    DUNGEON_TOP(880, 2),  DUNGEON_TOP(784, 2),  DUNGEON_TOP(659, 2), DUNGEON_TOP(554, 2),  // A G E C#
    DUNGEON_TOP(587, 8),                                                                   // D
};
#undef DUNGEON_TOP
#undef DUNGEON_REST
// A bar of a chord: its root, fifth, octave and third, up and back.
#define DUNGEON_BAR(root, fifth, octave, third)                                                                        \
    { Tr, root, root, kQuaver, 5, true }, { Tr, fifth, fifth, kQuaver, 4, true },                                      \
        { Tr, octave, octave, kQuaver, 4, true }, { Tr, third, third, kQuaver, 4, true },                              \
        { Tr, octave, octave, kQuaver, 4, true }, { Tr, fifth, fifth, kQuaver, 4, true },                              \
        { Tr, octave, octave, kQuaver, 4, true },                                                                      \
    {                                                                                                                  \
        Tr, fifth, fifth, kQuaver, 4, true                                                                             \
    }
constexpr Note kTitleUnder[] = {
    DUNGEON_BAR(147, 220, 294, 349),  // D minor
    DUNGEON_BAR(196, 294, 392, 466),  // G minor
    DUNGEON_BAR(233, 349, 466, 587),  // B flat
    DUNGEON_BAR(220, 330, 440, 554),  // A
    DUNGEON_BAR(147, 220, 294, 349),  // D minor
    DUNGEON_BAR(233, 349, 466, 587),  // B flat
    DUNGEON_BAR(220, 330, 440, 554),  // A
    DUNGEON_BAR(147, 220, 294, 349),  // D minor
};
#undef DUNGEON_BAR

struct Sound
{
    const Note* notes;
    uint8_t count;
};
#define DUNGEON_SOUND(notes) { notes, static_cast<uint8_t>(sizeof(notes) / sizeof(notes[0])) }
constexpr Sound kSoundTable[kSounds] = {
    DUNGEON_SOUND(kStep),      DUNGEON_SOUND(kBump),      DUNGEON_SOUND(kHit),      DUNGEON_SOUND(kCritical),
    DUNGEON_SOUND(kMiss),      DUNGEON_SOUND(kHurt),      DUNGEON_SOUND(kKill),     DUNGEON_SOUND(kPickup),
    DUNGEON_SOUND(kCoin),      DUNGEON_SOUND(kUnwrap),    DUNGEON_SOUND(kReveal),   DUNGEON_SOUND(kSmash),
    DUNGEON_SOUND(kLevelUp),   DUNGEON_SOUND(kDoor),      DUNGEON_SOUND(kStairs),   DUNGEON_SOUND(kFall),
    DUNGEON_SOUND(kEcho),      DUNGEON_SOUND(kTalk),      DUNGEON_SOUND(kMenuMove), DUNGEON_SOUND(kMenuSelect),
    DUNGEON_SOUND(kMenuClose), DUNGEON_SOUND(kSpell),     DUNGEON_SOUND(kTrap),     DUNGEON_SOUND(kDrink),
    DUNGEON_SOUND(kEat),       DUNGEON_SOUND(kChime),     DUNGEON_SOUND(kDig),      DUNGEON_SOUND(kLock),
    DUNGEON_SOUND(kScrape),    DUNGEON_SOUND(kBuy),       DUNGEON_SOUND(kError),    DUNGEON_SOUND(kHeal),
    DUNGEON_SOUND(kWarp),      DUNGEON_SOUND(kBell),      DUNGEON_SOUND(kRoar),     DUNGEON_SOUND(kHome),
    DUNGEON_SOUND(kNewFloor),  DUNGEON_SOUND(kHeartbeat), DUNGEON_SOUND(kDrip),     DUNGEON_SOUND(kWind),
    DUNGEON_SOUND(kCrackle),   DUNGEON_SOUND(kBirds),     DUNGEON_SOUND(kSwing),    DUNGEON_SOUND(kClang),
    DUNGEON_SOUND(kCrack),     DUNGEON_SOUND(kCrumble),   DUNGEON_SOUND(kStumble),
};
// Each tune's parts, one a voice.
constexpr Sound kTuneTable[][2] = {
    { { nullptr, 0 }, { nullptr, 0 } },
    { DUNGEON_SOUND(kTitleTop), DUNGEON_SOUND(kTitleUnder) },
};
#undef DUNGEON_SOUND

// A voice at its loudest: half of what 16 bits hold, so one sound alone is
// well heard. Three loud ones together would pass the top: the sum is held
// there (a moment of an old console's crunch) and never wraps round.
constexpr int kVoicePeak = 16000;
// How often a note's pitch and loudness are worked out again, in samples.
constexpr int kSlice = 32;

}  // namespace

void Player::Play(Sfx sfx)
{
    const uint8_t asked = m_Asked.load(std::memory_order_relaxed);
    const uint8_t taken = m_Taken.load(std::memory_order_acquire);
    if (static_cast<uint8_t>(asked - taken) >= kPending)
    {
        return;  // too many at once: this one is not heard
    }
    m_Pending[asked % kPending] = static_cast<uint8_t>(sfx);
    m_Asked.store(static_cast<uint8_t>(asked + 1), std::memory_order_release);
}

void Player::SetVolume(int percent)
{
    m_Volume.store(percent < 0 ? 0 : (percent > 100 ? 100 : percent), std::memory_order_relaxed);
}

bool Player::IsPlaying() const
{
    if (m_Asked.load(std::memory_order_relaxed) != m_Taken.load(std::memory_order_relaxed) ||
        m_TuneAsked.load(std::memory_order_relaxed) != 0 || m_TunePlaying != 0)
    {
        return true;
    }
    for (const Voice& voice : m_Voices)
    {
        if (voice.note)
        {
            return true;
        }
    }
    return false;
}

void Player::Tune(Voice& voice, int hz, uint32_t turnPerHz)
{
    // A whole turn of the wave is 2^32 of phase; `turnPerHz` is how much of
    // it a sample moves for each hertz (worked out once a Render: a divide
    // here would cost the Pico more than the voices do).
    voice.step = static_cast<uint32_t>(hz > 0 ? hz : 1) * turnPerHz;
}

void Player::Start(Sfx sfx, int sampleRate, uint32_t turnPerHz)
{
    const Sound& sound = kSoundTable[static_cast<int>(sfx) < kSounds ? static_cast<int>(sfx) : 0];
    // A silent voice, or else the one that started longest ago.
    Voice* voice = &m_Voices[0];
    for (Voice& v : m_Voices)
    {
        if (!v.note)
        {
            voice = &v;
            break;
        }
        if (static_cast<uint8_t>(m_Order - v.order) > static_cast<uint8_t>(m_Order - voice->order))
        {
            voice = &v;
        }
    }
    voice->note = sound.notes;
    voice->left = static_cast<uint8_t>(sound.count - 1);
    voice->phase = 0;
    voice->samples = sampleRate * voice->note->ms10 / 100;
    voice->samples = voice->samples > 0 ? voice->samples : 1;
    voice->samplesLeft = voice->samples;
    voice->order = m_Order++;
    Tune(*voice, voice->note->hz, turnPerHz);
}

void Player::StartTune(Music music, int sampleRate, uint32_t turnPerHz)
{
    const int which = static_cast<int>(music) < static_cast<int>(sizeof(kTuneTable) / sizeof(kTuneTable[0]))
                          ? static_cast<int>(music)
                          : 0;
    for (int part = 0; part < kTuneVoices; ++part)
    {
        const Sound& sound = kTuneTable[which][part];
        Voice& voice = m_TuneVoices[part];
        voice = Voice{};
        if (!sound.notes)
        {
            continue;
        }
        voice.first = sound.notes;
        voice.count = sound.count;
        voice.note = sound.notes;
        voice.left = static_cast<uint8_t>(sound.count - 1);
        voice.samples = sampleRate * voice.note->ms10 / 100;
        voice.samples = voice.samples > 0 ? voice.samples : 1;
        voice.samplesLeft = voice.samples;
        Tune(voice, voice.note->hz, turnPerHz);
    }
}

DUNGEON_HOT void Player::Render(int16_t* out, int frames, int sampleRate)
{
    const uint32_t turnPerHz =
        static_cast<uint32_t>((static_cast<uint64_t>(1) << 32) / static_cast<uint32_t>(sampleRate));
    // What was asked for since the last time.
    uint8_t taken = m_Taken.load(std::memory_order_relaxed);
    const uint8_t asked = m_Asked.load(std::memory_order_acquire);
    while (taken != asked)
    {
        Start(static_cast<Sfx>(m_Pending[taken % kPending]), sampleRate, turnPerHz);
        ++taken;
    }
    m_Taken.store(taken, std::memory_order_release);

    for (int i = 0; i < frames; ++i)
    {
        out[i] = 0;
    }
    const int volume = m_Volume.load(std::memory_order_relaxed);
    // The tune asked for, if it is not the one playing.
    const uint8_t tune = m_TuneAsked.load(std::memory_order_relaxed);
    if (tune != m_TunePlaying)
    {
        m_TunePlaying = tune;
        StartTune(static_cast<Music>(tune), sampleRate, turnPerHz);
    }
    for (Voice& voice : m_Voices)
    {
        RenderVoice(voice, out, frames, sampleRate, volume, turnPerHz);
    }
    for (Voice& voice : m_TuneVoices)
    {
        RenderVoice(voice, out, frames, sampleRate, volume, turnPerHz);
    }
}

DUNGEON_HOT void Player::RenderVoice(Voice& voice, int16_t* out, int frames, int sampleRate, int volume,
                                     uint32_t turnPerHz)
{
    int at = 0;
    while (voice.note && at < frames)
    {
        const Note& note = *voice.note;
        // A slice of the note at one pitch and one loudness.
        int slice = frames - at < kSlice ? frames - at : kSlice;
        slice = slice < voice.samplesLeft ? slice : voice.samplesLeft;
        const int done = voice.samples - voice.samplesLeft;
        const int hz = note.hz + (note.toHz - note.hz) * done / voice.samples;
        Tune(voice, hz, turnPerHz);
        int level = kVoicePeak * note.loud / 15 * volume / 100;
        level = note.fades ? level * voice.samplesLeft / voice.samples : level;
        for (int k = 0; k < slice; ++k)
        {
            const uint32_t before = voice.phase;
            voice.phase += voice.step;
            int sample = 0;
            if (note.wave == Wave::Square)
            {
                sample = (voice.phase >> 31) != 0 ? level : -level;
            }
            else if (note.wave == Wave::Triangle)
            {
                // Up for half the turn, down for the other half.
                const int32_t ramp = static_cast<int32_t>(voice.phase >> 16) - 32768;  // -32768..32767
                const int32_t fold = ramp < 0 ? -ramp : ramp;                          // 0..32768
                sample = (fold - 16384) * level / 16384;
            }
            else
            {
                // Noise: a new level each time the phase comes round.
                if (voice.phase < before)
                {
                    const uint16_t bit = static_cast<uint16_t>((voice.noise ^ (voice.noise >> 1)) & 1u);
                    voice.noise = static_cast<uint16_t>((voice.noise >> 1) | (bit << 14));
                    voice.last = static_cast<int16_t>((voice.noise & 1u) != 0 ? level : -level);
                }
                sample = voice.last;
            }
            const int mixed = out[at + k] + sample;
            out[at + k] = static_cast<int16_t>(mixed > 32767 ? 32767 : (mixed < -32768 ? -32768 : mixed));
        }
        at += slice;
        voice.samplesLeft -= slice;
        if (voice.samplesLeft <= 0)
        {
            // The sound's next note, or silence; a tune's part goes
            // round again.
            if (voice.left == 0 && !voice.first)
            {
                voice.note = nullptr;
                break;
            }
            if (voice.left == 0)
            {
                voice.note = voice.first;
                voice.left = static_cast<uint8_t>(voice.count - 1);
            }
            else
            {
                --voice.left;
                ++voice.note;
            }
            voice.samples = sampleRate * voice.note->ms10 / 100;
            voice.samples = voice.samples > 0 ? voice.samples : 1;
            voice.samplesLeft = voice.samples;
        }
    }
}

}  // namespace Dungeon::Chip
