#pragma once

// The first-person crawler in the scene: turns keys into moves, runs the game
// (CrawlerGame), and draws the 3D view and the HUD's bar and minimap through
// its own render pass, strip by strip, straight into the frame: the words
// too, in its own fonts. The scene holds only this and its camera.

#include "world/CrawlerGame.h"
#include "screen/ViewEffects.h"
#include "sound/Chip.h"
#include "sound/SoundCues.h"
#include "screen/MenuPanel.h"
#include "render/Raycaster.h"
#include "render/TextureCache.h"

#include <deki/Engine.h>
#include <deki/ISortableProvider.h>
#include <deki/reflection/ObjectRef.h>

#include "CameraComponent.h"

#include <deki/providers/Buffer.h>

#include <cstdint>

namespace DekiRendering
{
struct RenderContext;
class DirtyRegion;
}  // namespace DekiRendering

DEKI_CATEGORY("Game")
DEKI_DESCRIPTION("A first-person dungeon crawler: the 3D view, the minimap and the HP bar, drawn by its render pass.")
class Crawler : public Deki::Component, public Deki::ISortableProvider
{
public:
    DEKI_EXPORT
    DEKI_TOOLTIP("The camera that shows the game. In the editor's Scene View the dungeon is drawn inside its frame.")
    Deki::ObjectRef<DekiRendering::CameraComponent> gameCamera;

    DEKI_EXPORT
    DEKI_TOOLTIP("Rows of the screen the 3D view takes, from the top. The rest is the HUD.")
    int32_t viewHeight = 240;

    DEKI_EXPORT
    DEKI_TOOLTIP("How far the hero's torch lights, in tenths of a tile.")
    int32_t lightReach = 55;

    ~Crawler() override;

    // The 2D renderer only visits objects it can sort; drawn first, under the
    // HUD's words.
    int32_t GetSortingOrder() const override { return -1000; }
    void Start() override;
    void Update() override;

    /// Draws the 3D view and the HUD graphics into the rows of `ctx`, inside
    /// its redraw rectangle. Called by the pass for each part of the frame.
    void Draw(DekiRendering::RenderContext& ctx);
    /// True when `ctx`'s frame is the game's own screen, pixel for pixel.
    bool IsNativeScreen(const DekiRendering::RenderContext& ctx);

    /// Where this frame differs from the last, for redrawing only that: the
    /// 3D view when the view moved, the HUD panel when what it shows changed.
    /// Where the screen is scaled into `ctx`'s frame (the editor's Play view),
    /// the whole frame when anything changed.
    void AddChanges(const DekiRendering::RenderContext& ctx, DekiRendering::DirtyRegion& changed);

private:
    Dungeon::CrawlerGame m_Game;
    Dungeon::Raycaster m_Ray;
    // Its working memory: outside the component, which the engine keeps
    // under 64 KB.
    Deki::Buffer<uint64_t> m_RayStorage;
    Dungeon::Billboard m_Billboards[Dungeon::Raycaster::kMaxBillboards];
    size_t m_Listener = 0;
    Dungeon::Action m_Queued = Dungeon::Action::None;
    uint32_t m_ToastMessage = 0xFFFFFFFF;
    uint32_t m_TimeMs = 0;
    uint64_t m_FpsWindowUs = 0;
    bool m_OnTitle = true;
    int m_TitleAngle = 0;
    uint32_t m_ViewSignature = 0;
    uint32_t m_HudSignature = 0;
    bool m_ViewChanged = true;
    bool m_HudChanged = true;
    uint32_t m_FpsFrames = 0;
    // Where the frames' time went since the last log line, in microseconds.
    uint64_t m_PrepareUs = 0;
    uint64_t m_RaysUs = 0;
    uint64_t m_ViewUs = 0;
    uint64_t m_OverlaysUs = 0;
    uint64_t m_HudUs = 0;
    int m_Deepest = 1;
    int m_PagesKnown = 0;
    Dungeon::Legend m_Legend;
    uint32_t m_SavedLegend = 0;  // the game's legend version last saved

    Dungeon::MenuPanel m_Panel;
    Dungeon::Target m_Target;  // what the hero faces, for the prompt
    // The village: which building front each wall tile shows, and whether
    // the sky is up.
    uint8_t m_VillageCells[Dungeon::kMapWidth * Dungeon::kMapHeight] = {};
    bool m_Sky = false;
    Dungeon::Raycaster::Shadow m_Shadows[Dungeon::Raycaster::kMaxShadows];
    int m_ShadowCount = 0;

    bool m_Ready = false;

    // Textures whose glowing colours pulse (lava, water, crystals, flames):
    // a copy of each one's palettes in RAM, changed every frame.
    struct Animated
    {
        const Dungeon::Texture* source = nullptr;
        Dungeon::Texture live;
        uint16_t* palette = nullptr;  // every shade of its colours, in m_AnimatedPalettes
        uint8_t hot[32] = {};         // which colours glow (1 lava/fire, 2 water/crystal)
    };
    static constexpr int kMaxAnimated = 24;
    // Allocated, not inline: the engine addresses a component's fields
    // with 16-bit offsets, so a component stays under 64 KB.
    Deki::Buffer<Animated> m_Animated;
    // Their palettes, one after another: each as long as its texture's own
    // (a sprite's a few hundred bytes, a wall's a kilobyte).
    Deki::Buffer<uint16_t> m_AnimatedPalettes;
    int m_AnimatedCount = 0;
    Dungeon::CrawlerTextures m_Textures;

    // The zone's walls, floors and ceiling copied into RAM (TextureCache.h),
    // for the look they were copied for (the zone, or -1 the village).
    static constexpr size_t kTextureRam = 32 * 1024;
    Deki::Buffer<uint8_t> m_TextureRam;
    Dungeon::TextureCache m_TextureCache;
    int m_CachedLook = -2;
    const Dungeon::Texture* m_VillageFronts[8] = {};
    const Dungeon::Texture* InRam(const Dungeon::Texture* texture);

    // Numbers, sparks, jolts and the HUD catching up (ViewEffects.h).
    Dungeon::ViewEffects m_Effects;

    // The sounds: a chip of three voices, asked for them as the game
    // changes, and pulled for samples by the board's speakers (from an
    // interrupt on the PicoCalc; from SDL's audio thread on a computer).
    Dungeon::Chip::Player m_Chip;
    Dungeon::SoundCues m_Cues;
    int m_SavedVolume = -1;
    bool m_SoundCard = false;  // on a computer: this one started the sound card, and stops it
    // The village's tiles in shadow, for the raycaster (Village::Gloom).
    uint8_t m_Gloom[Dungeon::kMapWidth * Dungeon::kMapHeight] = {};
    static void FillSound(void* context, int16_t* out, int frames);

    // The message box: what just happened, over the bottom of the view for a
    // few seconds.
    char m_ToastText[64] = {};
    int m_ToastMs = 0;
    // How long the box stays in all, and how many of its letters are out
    // (speech is typed out: MessageCard.h).
    int m_ToastLength = 0;
    int ToastTyped() const;
    int32_t m_ToastVersion = 0;

    // Falling: black, and lines of text before home (Transitions.h). A new
    // place shows its name on black first; going in or out of a building,
    // a blink of dark.
    int m_DeadMs = 0;
    int m_EchoMs = 0;  // how long an echo stone has been speaking (0: none is)
    // Someone speaking, and what a deed did: how long each has been up, and
    // which one (the game's count) that is.
    int m_TalkMs = 0;
    uint32_t m_TalkSeen = 0;
    // The keys that can be changed, in the keys menu's order (forward, back,
    // turn left, turn right, step left, step right, use, wait, menu), their
    // names for it, and which one waits for its new key (-1: none).
    static constexpr int kBindings = 9;
    uint32_t m_Keys[kBindings] = {};
    char m_KeyNames[kBindings][8] = {};
    int m_KeyWanted = -1;
    void ResetKeys();
    void NameKeys();
    /// True while a card of its own is up: someone speaking, or a result.
    bool CardUp() const;
    int m_FadeInMs = 0;
    int m_FadeInLength = 1;
    int m_ShownInside = -1;
    static constexpr int32_t kNoPlace = -1000000;
    int32_t m_Place = kNoPlace;  // which place the card was last shown for
    int m_PlaceMs = 0;
    int32_t PlaceKey() const;
    void DrawDeath(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight) const;

    bool ToastShown() const;
    void DrawOverlays(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight) const;
    void DrawTitle(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight) const;
    void DrawToast(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight) const;
    void DrawTarget(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight) const;
    void SetupAnimated();
    void AnimatePalettes();
    const Dungeon::Texture* Live(const Dungeon::Texture* texture) const;
    // The whole game screen, for drawing it scaled into a frame that is not
    // the game's own (the editor's Scene View). Never allocated on a board.
    Deki::Buffer<uint16_t> m_Preview;

    void Setup();
    void PrepareView(int ms);
    void DrawScreen(uint16_t* rows, int strideBytes, int top, int bottom, int clipLeft, int clipRight);
    bool GameScreenRect(const DekiRendering::RenderContext& ctx, float& left, float& top, float& right, float& bottom);
    void OnKey(uint32_t key, uint32_t character);
    void UpdateHud();
    void ShowTitle(bool shown);
    void DrawHud(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight) const;
    void LoadBest();
    void SaveBest();
};
