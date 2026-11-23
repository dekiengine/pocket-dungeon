#include "Crawler.h"

#include "screen/MessageCard.h"
#include "screen/Transitions.h"
#include "render/PixelText.h"
#include "render/SmoothText.h"
#include "render/Textures.gen.h"
#include "world/Village.h"

#include "DekiInput.h"
#if defined(DEKI_PICO) && __has_include("PicoCalcSpeaker.h")
#include "PicoCalcSpeaker.h"  // from deki-clockworkpi-boards
#define DUNGEON_HAS_SPEAKER 1
#elif !defined(DEKI_PICO) && __has_include("deki-sdl3-integration/SDL3SampleOutput.h")
#include "deki-sdl3-integration/SDL3SampleOutput.h"  // on a computer, and in the editor's Play view
#define DUNGEON_HAS_SOUND_CARD 1
#endif
#include "DekiRenderPassRegistry.h"
#include "DekiRenderer.h"
#include "DirtyRegion.h"
#include "Keys.h"
#include "QuadBlit.h"

#include <deki/ComponentInterfaceAdapters.h>
#include <deki/LogSystem.h>
#include <deki/Storage.h>
#include <deki/Time.h>
#include <deki/providers/FileSystem.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <string>

using namespace Dungeon;

namespace
{

// The textures, in flash.
const Texture& kWallStoneTexture = Textures::kWallStoneTexture;
const Texture& kSkyTexture = Textures::kSkyTexture;
const Texture& kGrannyTexture = Textures::kGrannyTexture;
const Texture& kKidTexture = Textures::kKidTexture;
const Texture& kPropBookshelfTexture = Textures::kPropBookshelfTexture;
const Texture& kPropCampTexture = Textures::kPropCampTexture;
const Texture& kPropLockedChestTexture = Textures::kPropLockedChestTexture;
const Texture& kPropPotTexture = Textures::kPropPotTexture;
const Texture& kPropStatueTexture = Textures::kPropStatueTexture;
const Texture& kPropGraveTexture = Textures::kPropGraveTexture;
const Texture& kPropBasketTexture = Textures::kPropBasketTexture;
const Texture& kPropCartTexture = Textures::kPropCartTexture;
const Texture& kPropCageTexture = Textures::kPropCageTexture;
const Texture& kPropOldChestTexture = Textures::kPropOldChestTexture;
const Texture& kWaterTexture = Textures::kWaterTexture;
const Texture& kGlowcapTexture = Textures::kGlowcapTexture;
const Texture& kFishTexture = Textures::kFishTexture;
const Texture& kPearlTexture = Textures::kPearlTexture;
const Texture& kPropPotUrnTexture = Textures::kPropPotUrnTexture;
const Texture& kPropPotJarTexture = Textures::kPropPotJarTexture;
const Texture& kPropCrateTexture = Textures::kPropCrateTexture;
const Texture& kLockpickTexture = Textures::kLockpickTexture;
const Texture& kGemTexture = Textures::kGemTexture;
const Texture& kGobletTexture = Textures::kGobletTexture;
const Texture& kNoteTexture = Textures::kNoteTexture;
const Texture& kMossTexture = Textures::kMossTexture;
const Texture& kBoneTexture = Textures::kBoneTexture;
const Texture& kShellTexture = Textures::kShellTexture;
const Texture& kOreTexture = Textures::kOreTexture;
const Texture& kPortalHomeTexture = Textures::kPortalHomeTexture;
const Texture& kVillageWallTexture = Textures::kVillageWallTexture;
const Texture& kCobbleTexture = Textures::kCobbleTexture;
const Texture& kRuinTexture = Textures::kRuinTexture;
const Texture& kVBlacksmithTexture = Textures::kVBlacksmithTexture;
const Texture& kVApothecaryTexture = Textures::kVApothecaryTexture;
const Texture& kVTavernTexture = Textures::kVTavernTexture;
const Texture& kVTowerTexture = Textures::kVTowerTexture;
const Texture& kVBankTexture = Textures::kVBankTexture;
const Texture& kVLiftTexture = Textures::kVLiftTexture;
const Texture& kVTopATexture = Textures::kVTopATexture;
const Texture& kVTopBTexture = Textures::kVTopBTexture;
const Texture& kVTopTowerTexture = Textures::kVTopTowerTexture;
const Texture& kVTopRuinTexture = Textures::kVTopRuinTexture;
const Texture& kInWallTexture = Textures::kInWallTexture;
const Texture& kInFloorTexture = Textures::kInFloorTexture;
const Texture& kInCeilingTexture = Textures::kInCeilingTexture;
const Texture& kKeeperSmithTexture = Textures::kKeeperSmithTexture;
const Texture& kKeeperHerbsTexture = Textures::kKeeperHerbsTexture;
const Texture& kKeeperBarTexture = Textures::kKeeperBarTexture;
const Texture& kKeeperWizardTexture = Textures::kKeeperWizardTexture;
const Texture& kKeeperBankTexture = Textures::kKeeperBankTexture;
const Texture& kKeeperTexture = Textures::kKeeperTexture;
const Texture& kGhostTexture = Textures::kGhostTexture;
const Texture& kShardTexture = Textures::kShardTexture;
const Texture& kDuckTexture = Textures::kDuckTexture;
const Texture& kSnackTexture = Textures::kSnackTexture;
const Texture& kSpellFoldTexture = Textures::kSpellFoldTexture;
const Texture& kWallEchoStoneTexture = Textures::kWallEchoStoneTexture;
const Texture& kWallEchoBoneTexture = Textures::kWallEchoBoneTexture;
const Texture& kWallEchoWetTexture = Textures::kWallEchoWetTexture;
const Texture& kWallEchoForgeTexture = Textures::kWallEchoForgeTexture;
const Texture& kWallCrackedTexture = Textures::kWallCrackedTexture;
const Texture& kDoorTexture = Textures::kDoorTexture;
const Texture& kFloorTexture = Textures::kFloorTexture;
const Texture& kFloorHoleTexture = Textures::kFloorHoleTexture;
const Texture& kFloorSpikesTexture = Textures::kFloorSpikesTexture;
const Texture& kFloorRuneTexture = Textures::kFloorRuneTexture;
const Texture& kWallBoneTexture = Textures::kWallBoneTexture;
const Texture& kCrackedBoneTexture = Textures::kCrackedBoneTexture;
const Texture& kFloorBoneTexture = Textures::kFloorBoneTexture;
const Texture& kCeilingBoneTexture = Textures::kCeilingBoneTexture;
const Texture& kWallWetTexture = Textures::kWallWetTexture;
const Texture& kCrackedWetTexture = Textures::kCrackedWetTexture;
const Texture& kFloorWetTexture = Textures::kFloorWetTexture;
const Texture& kCeilingWetTexture = Textures::kCeilingWetTexture;
const Texture& kWallForgeTexture = Textures::kWallForgeTexture;
const Texture& kCrackedForgeTexture = Textures::kCrackedForgeTexture;
const Texture& kFloorForgeTexture = Textures::kFloorForgeTexture;
const Texture& kCeilingForgeTexture = Textures::kCeilingForgeTexture;

const Texture& kCeilingTexture = Textures::kCeilingTexture;
const Texture& kDroolblobTexture = Textures::kDroolblobTexture;
const Texture& kCandleghoulTexture = Textures::kCandleghoulTexture;
const Texture& kEyejarTexture = Textures::kEyejarTexture;
const Texture& kEyebatTexture = Textures::kEyebatTexture;
const Texture& kFlamespiderTexture = Textures::kFlamespiderTexture;
const Texture& kBonehoundTexture = Textures::kBonehoundTexture;
const Texture& kTeasnailTexture = Textures::kTeasnailTexture;
const Texture& kStewpotTexture = Textures::kStewpotTexture;
const Texture& kGrimoireTexture = Textures::kGrimoireTexture;
const Texture& kRagmummyTexture = Textures::kRagmummyTexture;
const Texture& kSlimewizardTexture = Textures::kSlimewizardTexture;
const Texture& kRockstackTexture = Textures::kRockstackTexture;
const Texture& kGlimmerjellyTexture = Textures::kGlimmerjellyTexture;
const Texture& kMimicTexture = Textures::kMimicTexture;
const Texture& kGrubkingTexture = Textures::kGrubkingTexture;
const Texture& kHydragonTexture = Textures::kHydragonTexture;
const Texture& kPotionTexture = Textures::kPotionTexture;
const Texture& kGoldTexture = Textures::kGoldTexture;
const Texture& kSwordTexture = Textures::kSwordTexture;
const Texture& kShieldTexture = Textures::kShieldTexture;
const Texture& kHeartTexture = Textures::kHeartTexture;
const Texture& kChestTexture = Textures::kChestTexture;
const Texture& kKeyTexture = Textures::kKeyTexture;
const Texture& kScrollTexture = Textures::kScrollTexture;
const Texture& kBombTexture = Textures::kBombTexture;
const Texture& kWingTexture = Textures::kWingTexture;
const Texture& kElixirTexture = Textures::kElixirTexture;
const Texture& kPageTexture = Textures::kPageTexture;
const Texture& kFountainTexture = Textures::kFountainTexture;
const Texture& kMerchantTexture = Textures::kMerchantTexture;
const Texture& kShrineTexture = Textures::kShrineTexture;
const Texture& kPropPackTexture = Textures::kPropPackTexture;
const Texture& kPropBarrelTexture = Textures::kPropBarrelTexture;
const Texture& kPropBushTexture = Textures::kPropBushTexture;
const Texture& kPropSkullsTexture = Textures::kPropSkullsTexture;
const Texture& kPropCandlesTexture = Textures::kPropCandlesTexture;
const Texture& kPropCoffinTexture = Textures::kPropCoffinTexture;
const Texture& kPropCrystalTexture = Textures::kPropCrystalTexture;
const Texture& kPropMushroomsTexture = Textures::kPropMushroomsTexture;
const Texture& kPropClamTexture = Textures::kPropClamTexture;
const Texture& kPropAnvilTexture = Textures::kPropAnvilTexture;
const Texture& kPropBrazierTexture = Textures::kPropBrazierTexture;
const Texture& kPropRackTexture = Textures::kPropRackTexture;
const Texture& kTorchATexture = Textures::kTorchATexture;
const Texture& kBladeTexture = Textures::kBladeTexture;
const Texture& kAxeTexture = Textures::kAxeTexture;
const Texture& kSpearTexture = Textures::kSpearTexture;
const Texture& kDaggerTexture = Textures::kDaggerTexture;
const Texture& kHammerTexture = Textures::kHammerTexture;
const Texture& kSpellFireTexture = Textures::kSpellFireTexture;
const Texture& kSpellHealTexture = Textures::kSpellHealTexture;
const Texture& kSpellFrostTexture = Textures::kSpellFrostTexture;
const Texture& kSpellBlinkTexture = Textures::kSpellBlinkTexture;
const Texture& kBoltTexture = Textures::kBoltTexture;
const Texture& kHandAxeTexture = Textures::kHandAxeTexture;
const Texture& kHandSpearTexture = Textures::kHandSpearTexture;
const Texture& kHandDaggerTexture = Textures::kHandDaggerTexture;
const Texture& kHandHammerTexture = Textures::kHandHammerTexture;
const Texture& kWallBone2Texture = Textures::kWallBone2Texture;
const Texture& kWallWet2Texture = Textures::kWallWet2Texture;
const Texture& kWallForge2Texture = Textures::kWallForge2Texture;
const Texture& kTorchBTexture = Textures::kTorchBTexture;
const Texture& kDustTexture = Textures::kDustTexture;

// The textures whose glowing colours pulse.
// Things drawn from eight sides (CrawlerGame::ViewIndex's order).
#define DUNGEON_VIEWS(name)                                                                                            \
    { &Textures::k##name##Texture,      &Textures::k##name##View1Texture, &Textures::k##name##View2Texture,            \
      &Textures::k##name##View3Texture, &Textures::k##name##View4Texture, &Textures::k##name##View5Texture,            \
      &Textures::k##name##View6Texture, &Textures::k##name##View7Texture }
const Texture* const kTorchViews[8] = DUNGEON_VIEWS(TorchA);
#undef DUNGEON_VIEWS

const Texture* const kAnimatedTextures[] = {
    &kPropBrazierTexture,
    &kPropAnvilTexture,
    &kPropMushroomsTexture,
    &kPropCrystalTexture,
    &kPropCandlesTexture,
    &kTorchATexture,
    &kWallEchoStoneTexture,
    &kWallEchoBoneTexture,
    &kWallEchoWetTexture,
    &kWallEchoForgeTexture,
    // The torch's flame, from every side.
    kTorchViews[1],
    kTorchViews[2],
    kTorchViews[3],
    kTorchViews[4],
    kTorchViews[5],
    kTorchViews[6],
    kTorchViews[7],
};
const Texture& kHandTexture = Textures::kHandTexture;

// Each zone's look: its walls (the first the commonest), its cracked wall,
// floors (the first the commonest), ceiling and echo stone. The floor's marks (the stairs
// hole, spikes, the door's rune) are shared.
struct ZoneLook
{
    const Texture* walls[Raycaster::kMaxWallKinds];
    int wallCount;
    const Texture* cracked;  // and after blows: kBroken
    const Texture* floors[Raycaster::kMaxFloorKinds];
    const Texture* ceiling;
    const Texture* echo;
};
const ZoneLook kZoneLooks[kZones] = {
    { { &kWallStoneTexture, nullptr, nullptr, nullptr },
      1,
      &kWallCrackedTexture,
      { &kFloorTexture, nullptr },
      &kCeilingTexture,
      &kWallEchoStoneTexture },
    { { &kWallBoneTexture, &kWallBone2Texture, nullptr, nullptr },
      2,
      &kCrackedBoneTexture,
      { &kFloorBoneTexture, nullptr },
      &kCeilingBoneTexture,
      &kWallEchoBoneTexture },
    { { &kWallWetTexture, &kWallWet2Texture, nullptr, nullptr },
      2,
      &kCrackedWetTexture,
      { &kFloorWetTexture, nullptr },
      &kCeilingWetTexture,
      &kWallEchoWetTexture },
    { { &kWallForgeTexture, &kWallForge2Texture, nullptr, nullptr },
      2,
      &kCrackedForgeTexture,
      { &kFloorForgeTexture, nullptr },
      &kCeilingForgeTexture,
      &kWallEchoForgeTexture },
};

// Each zone's cracked wall after a blow or two, and after three or four.
const Texture* const kBroken[kZones][2] = {
    { &Textures::kWallCracked1Texture, &Textures::kWallCracked2Texture },
    { &Textures::kCrackedBone1Texture, &Textures::kCrackedBone2Texture },
    { &Textures::kCrackedWet1Texture, &Textures::kCrackedWet2Texture },
    { &Textures::kCrackedForge1Texture, &Textures::kCrackedForge2Texture },
};

// The village: house walls, cobbles, no ceiling (the sky), and each
// building's front by Village::Building, then the ruin they all start as.
const ZoneLook kVillageLook{ { &kVillageWallTexture, nullptr, nullptr, nullptr },
                             1,
                             &kVillageWallTexture,
                             { &kCobbleTexture, nullptr },
                             nullptr,
                             nullptr };
const Texture* const kVillageFronts[Village::kBuildings + 1] = {
    &kVBlacksmithTexture, &kVApothecaryTexture, &kVTavernTexture, &kVTowerTexture,
    &kVBankTexture,       &kVLiftTexture,       &kRuinTexture,
};
// The houses' upper storeys and roofs: two kinds for any house, and the
// tower's and the ruin's own (beside kVillageFronts; null: any house's).
const Texture* const kVillageTops[2] = { &kVTopATexture, &kVTopBTexture };
const Texture* const kVillageFrontTops[Village::kBuildings + 1] = {
    nullptr, nullptr, nullptr, &kVTopTowerTexture, nullptr, nullptr, &kVTopRuinTexture,
};
// Inside a building: plank walls, floorboards, a beamed ceiling; the way
// out is the door. Each building's keeper, by Village::Building.
const ZoneLook kInteriorLook{ { &kInWallTexture, nullptr, nullptr, nullptr },
                              1,
                              &kInWallTexture,
                              { &kInFloorTexture, nullptr },
                              &kInCeilingTexture,
                              nullptr };
const Texture* const kKeepers[Village::kBuildings] = {
    &kKeeperSmithTexture, &kKeeperHerbsTexture, &kKeeperBarTexture, &kKeeperWizardTexture, &kKeeperBankTexture, nullptr,
};

const CrawlerTextures kCrawlerTextures{
    { &kDroolblobTexture, &kCandleghoulTexture, &kEyejarTexture, &kEyebatTexture, &kFlamespiderTexture,
      &kBonehoundTexture, &kTeasnailTexture, &kStewpotTexture, &kGrimoireTexture, &kRagmummyTexture,
      &kSlimewizardTexture, &kRockstackTexture, &kGlimmerjellyTexture, &kMimicTexture, &kGrubkingTexture,
      &kHydragonTexture, &kKeeperTexture, &kGhostTexture },
    { &kPotionTexture, &kGoldTexture,   &kSwordTexture,         &kShieldTexture, &kHeartTexture,  &kChestTexture,
      &kKeyTexture,    &kScrollTexture, &kBombTexture,          &kWingTexture,   &kElixirTexture, &kPageTexture,
      &kBladeTexture,  &kAxeTexture,    &kSpearTexture,         &kDaggerTexture, &kHammerTexture, &kShardTexture,
      &kDuckTexture,   &kSnackTexture,  &kLockpickTexture,      &kGemTexture,    &kGobletTexture, &kNoteTexture,
      &kMossTexture,   &kBoneTexture,   &kShellTexture,         &kOreTexture,    &kWaterTexture,  &kGlowcapTexture,
      &kFishTexture,   &kPearlTexture,  &Textures::kVialTexture },
    { &kFountainTexture, &kMerchantTexture, &kShrineTexture, &kPortalHomeTexture, &kGrannyTexture, &kKidTexture,
      nullptr, &Textures::kFenceTexture, &Textures::kBoardTexture },
    { &kPropPackTexture,      &kPropBarrelTexture,    &kPropBushTexture,        &kPropStatueTexture,
      &kPropSkullsTexture,    &kPropCandlesTexture,   &kPropCoffinTexture,      &kPropGraveTexture,
      &kPropCrystalTexture,   &kPropMushroomsTexture, &kPropClamTexture,        &kPropBasketTexture,
      &kPropAnvilTexture,     &kPropBrazierTexture,   &kPropRackTexture,        &kPropCartTexture,
      &kPropBookshelfTexture, &kPropCampTexture,      &kPropLockedChestTexture, &kPropPotTexture,
      &kPropCrateTexture,     &kPropCageTexture,      &kPropOldChestTexture,    &Textures::kPropMirrorTexture },
    { &kTorchATexture, &kTorchBTexture },
    &kBoltTexture,
    { &kSpellFireTexture, &kSpellHealTexture, &kSpellFrostTexture, &kSpellBlinkTexture, &kSpellFoldTexture }
};

// The hand in view, by weapon.
const Texture* const kHands[kWeapons] = { &kHandTexture, &kHandAxeTexture, &kHandSpearTexture, &kHandDaggerTexture,
                                          &kHandHammerTexture };
// The spells' icons, and the colour each flashes the view.
const Texture* const kSpellIcons[kSpells] = { &kSpellFireTexture, &kSpellHealTexture, &kSpellFrostTexture,
                                              &kSpellBlinkTexture, &kSpellFoldTexture };
constexpr uint16_t kSpellFlash[kSpells] = { SmoothText::Rgb(255, 140, 40), SmoothText::Rgb(90, 230, 120),
                                            SmoothText::Rgb(140, 200, 255), SmoothText::Rgb(190, 120, 255),
                                            SmoothText::Rgb(80, 230, 210) };

// HUD colours, RGB565.
constexpr uint16_t kHudBackground = 0x0008;  // the menus' deep blue
constexpr uint16_t kHudFrame = 0xFFFF;
constexpr uint16_t kHudLine = 0x5AEB;
constexpr uint16_t kXpFull = 0xFEA0;  // gold
constexpr uint16_t kMapBack = 0x0003;
constexpr uint16_t kMapFacing = 0x8C40;
constexpr uint16_t kMapShrine = 0xFC9F;
constexpr uint16_t kBarBack = 0x2104;
constexpr uint16_t kBarFull = 0x2E6A;  // green
constexpr uint16_t kBarLow = 0xE8C4;   // red
constexpr uint16_t kMapWall = 0x5AEB;
constexpr uint16_t kMapFloor = 0x2124;
constexpr uint16_t kMapHero = 0xFFE0;
constexpr uint16_t kMapMonster = 0xF800;
constexpr uint16_t kMapStairs = 0x3D9F;
constexpr uint16_t kMapSecret = 0xA81F;    // a cracked wall, once a scroll shows it
constexpr uint16_t kMapDoor = 0xC440;      // a vault's door
constexpr uint16_t kMapEcho = 0x5F1F;      // an echo stone: it glows, so it shows
constexpr uint16_t kMapMerchant = 0x2FE5;  // Puddle
constexpr uint16_t kMapFountain = 0x5DDF;
constexpr uint16_t kMapTrap = 0xF2A0;

// The PicoCalc's screen: the 3D view on top, the HUD below it.
constexpr int kScreenHeight = 320;

// Where the HUD's graphics sit, in rows below the view.
constexpr int kBarX = 28;  // after the heart
constexpr int kBarWidth = 120;
constexpr int kBarHeight = 9;
constexpr int kBarTop = 9;
constexpr int kXpX = 62;  // after "LV.."
constexpr int kXpWidth = 86;
constexpr int kXpTop = 57;
constexpr int kXpHeight = 4;
constexpr uint16_t kBarGap = 0x0841;
constexpr uint16_t kHudText = 0xFFFF;
constexpr uint16_t kHudDim = 0x9CF3;
constexpr uint16_t kHudGold = 0xFEA0;
// The logo's letters, top row to bottom: cream to deep orange.
constexpr uint16_t kLogoGradient[10] = {
    0xFFF7, 0xFF54, 0xFEEC, 0xFE44, 0xFD83, 0xFCA2, 0xF3C1, 0xD2E1, 0xB241, 0x91A1
};
// A blink of dark through a building's door, in ms.
constexpr int kDoorFadeMs = 250;
constexpr int kSoundRate = 22050;  // samples a second the chip is asked for
// How loud it starts. The PicoCalc has a volume wheel, which sets how loud
// it really is: the game stays high (not at the top) and leaves it to the
// wheel.
#if defined(DEKI_PICO)
constexpr int kFirstVolume = 75;
#else
constexpr int kFirstVolume = 50;
#endif
constexpr int kEchoFadeMs = 400;  // the view coming back after an echo

// The modern UI: flat dark panels, one warm accent, clean text.
namespace Ui
{
using SmoothText::Rgb;
constexpr uint16_t kPanelTop = Rgb(14, 16, 22);
constexpr uint16_t kPanelBottom = Rgb(22, 25, 33);
constexpr uint16_t kDivider = Rgb(42, 47, 60);
constexpr uint16_t kTrack = Rgb(38, 43, 54);
constexpr uint16_t kText = Rgb(232, 234, 240);
constexpr uint16_t kDim = Rgb(139, 147, 161);
constexpr uint16_t kAccent = Rgb(245, 182, 66);  // amber
constexpr uint16_t kHp = Rgb(244, 63, 94);       // rose
constexpr uint16_t kHpLight = Rgb(251, 113, 133);
constexpr uint16_t kHpLow = Rgb(190, 40, 60);
constexpr uint16_t kCardTint = Rgb(20, 24, 34);
constexpr uint16_t kCardBorder = Rgb(58, 66, 84);
constexpr uint16_t kMapPanel = Rgb(10, 12, 17);
}  // namespace Ui
constexpr uint16_t kMp = SmoothText::Rgb(80, 140, 255);
constexpr uint16_t kMpLight = SmoothText::Rgb(140, 185, 255);
constexpr int kMapCell = 3;
constexpr int kMapRight = 9;  // gap to the screen's right edge
constexpr int kMapTop = 7;

const char* kBestPath = "pocketdungeon_best.txt";

// FNV-1a over a few numbers: what a frame shows, to tell whether it changed.
uint32_t Hash(uint32_t hash, int32_t value)
{
    for (int i = 0; i < 4; ++i)
    {
        hash = (hash ^ static_cast<uint8_t>(value >> (8 * i))) * 16777619u;
    }
    return hash;
}

uint16_t Mix565(uint16_t a, uint16_t b, int t256)
{
    const int r = ((a >> 11) * (256 - t256) + (b >> 11) * t256) >> 8;
    const int g = (((a >> 5) & 63) * (256 - t256) + ((b >> 5) & 63) * t256) >> 8;
    const int bl = ((a & 31) * (256 - t256) + (b & 31) * t256) >> 8;
    return static_cast<uint16_t>((r << 11) | (g << 5) | bl);
}

// Draws the 3D view and the HUD graphics of any object with a Crawler. Its one
// hook is Execute, so it draws strip by strip.
class CrawlerPass : public DekiRendering::RenderPass
{
public:
    uint32_t HookMask() const override { return DekiRendering::RenderPassHooks::Execute; }
    bool WantsObject(Deki::Object* obj) const override { return obj->GetComponent<Crawler>() != nullptr; }
    // It writes pixels itself, and says so around them (BeforeWriting,
    // NoteWritten).
    bool WritesBuffer() const override { return false; }
    // Says where it changed, so a frame where only the view moved does not
    // send the HUD again, and a still frame sends nothing.
    bool ReportsChanges() const override { return true; }
    void AddChanges(Deki::Object* obj, const DekiRendering::RenderContext& ctx,
                    DekiRendering::DirtyRegion& changed) override
    {
        if (Crawler* crawler = obj->GetComponent<Crawler>())
        {
            crawler->AddChanges(ctx, changed);
        }
    }
    void Execute(Deki::Object* obj, DekiRendering::RenderContext& ctx) override
    {
        if (Crawler* crawler = obj->GetComponent<Crawler>())
        {
            crawler->Draw(ctx);
        }
    }
    const char* Name() const override { return "Crawler"; }
};

// Registered with autoAttach, so the scene needs no render pipeline change.
struct CrawlerPassRegistrar
{
    CrawlerPassRegistrar()
    {
        DekiRendering::RenderPassInfo info;
        info.factory = []() -> DekiRendering::RenderPass* { return new CrawlerPass(); };
        info.autoAttach = true;
        DekiRendering::DekiRenderPassRegistry::Register("crawler", info);
        // How the renderer finds Crawler's sorting order.
        Deki::ComponentInterfaceAdapters::Register(
            Deki::ISortableProvider::kInterfaceID, Deki::TypeId<Crawler>(), [](Deki::Component* c) -> void*
            { return static_cast<Deki::ISortableProvider*>(static_cast<Crawler*>(c)); });
    }
    ~CrawlerPassRegistrar() { DekiRendering::DekiRenderPassRegistry::Unregister("crawler"); }
};
CrawlerPassRegistrar s_CrawlerPassRegistrar;

}  // namespace

void Crawler::FillSound(void* context, int16_t* out, int frames)
{
    static_cast<Crawler*>(context)->m_Chip.Render(out, frames, kSoundRate);
}

Crawler::~Crawler()
{
#if defined(DUNGEON_HAS_SPEAKER)
    DekiClockworkPi::StopPicoCalcSpeaker();
#elif defined(DUNGEON_HAS_SOUND_CARD)
    if (m_SoundCard)
    {
        DekiSdl3::SDL3SampleOutput::Stop();
    }
#endif
    if (m_Listener != 0)
    {
        DekiInput::DekiInput::UnregisterEventCallback(m_Listener);
    }
}

void Crawler::SetupAnimated()
{
    m_AnimatedCount = 0;
    // Read for every pixel of a glowing surface: internal RAM. One entry
    // for each glowing texture, and one block for all their palettes.
    constexpr size_t kGlowing = sizeof(kAnimatedTextures) / sizeof(kAnimatedTextures[0]);
    static_assert(kGlowing <= static_cast<size_t>(kMaxAnimated), "raise kMaxAnimated");
    size_t entries = 0;
    for (const Texture* source : kAnimatedTextures)
    {
        entries += source->colours <= 32 ? static_cast<size_t>(source->levels * source->colours) : 0;
    }
    if (m_Animated.Allocate(kGlowing, Deki::Memory::Internal) &&
        m_AnimatedPalettes.Allocate(entries, Deki::Memory::Internal))
    {
        uint16_t* next = m_AnimatedPalettes.Data();
        for (const Texture* source : kAnimatedTextures)
        {
            const int count = source->levels * source->colours;
            if (m_AnimatedCount >= kMaxAnimated || source->colours > 32)
            {
                continue;
            }
            Animated& a = m_Animated[m_AnimatedCount++];
            a.source = source;
            a.live = *source;
            a.palette = next;
            a.live.palette = next;
            next += count;
            for (int i = 0; i < count; ++i)
            {
                a.palette[i] = source->palette[i];
            }
            // Which colours glow, from the brightest palette: fiery ones (red
            // well over blue) and watery ones (bright blue-green).
            for (int i = 0; i < source->colours; ++i)
            {
                const uint16_t c = source->palette[i];
                const int r = (c >> 11) * 255 / 31;
                const int g = ((c >> 5) & 63) * 255 / 63;
                const int b = (c & 31) * 255 / 31;
                a.hot[i] = c == kTransparent                 ? 0
                           : (r > 170 && r > g && b < 110)   ? 1
                           : (b > 140 && g > 120 && r < 140) ? 2
                                                             : 0;
            }
        }
    }
    else
    {
        // Nothing glows; everything still shows, in its own colours.
        DEKI_LOG_WARNING("Crawler: no room for the glowing palettes; nothing glows");
    }
    // The billboards' textures, with the animated ones swapped in.
    m_Textures = kCrawlerTextures;
    m_Textures.pots[0] = &kPropPotUrnTexture;
    m_Textures.pots[1] = &kPropPotJarTexture;
    // What things look like used up, by their place in the props' table.
    m_Textures.spent[2] = &Textures::kPropBushUsedTexture;
    m_Textures.spent[3] = &Textures::kPropStatueUsedTexture;
    m_Textures.spent[5] = &Textures::kPropCandlesUsedTexture;
    m_Textures.spent[6] = &Textures::kPropCoffinUsedTexture;
    m_Textures.spent[8] = &Textures::kPropCrystalUsedTexture;
    m_Textures.spent[9] = &Textures::kPropMushroomsUsedTexture;
    m_Textures.spent[10] = &Textures::kPropClamUsedTexture;
    m_Textures.spent[11] = &Textures::kPropBasketUsedTexture;
    m_Textures.spent[14] = &Textures::kPropRackUsedTexture;
    m_Textures.spent[17] = &Textures::kPropCampUsedTexture;
    m_Textures.spent[21] = &Textures::kPropCageUsedTexture;
    m_Textures.spent[18] = &Textures::kPropChestOpenTexture;
    m_Textures.spent[22] = &Textures::kPropChestOpenTexture;
    m_Textures.fountainDry = &Textures::kFountainDryTexture;
    for (const Texture*& t : m_Textures.props)
    {
        t = Live(t);
    }
    for (const Texture*& t : m_Textures.torch)
    {
        t = Live(t);
    }
    // Things drawn from eight sides, each where its kind sits in the tables.
    const struct
    {
        int zone;
        int kind;
        const Texture* const* views;
    } kViewed[] = {};
    for (const auto& viewed : kViewed)
    {
        for (int i = 0; i < 8; ++i)
        {
            m_Textures.propViews[CrawlerGame::PropKindIndex(viewed.zone, viewed.kind)][i] = viewed.views[i];
        }
    }
    for (int i = 0; i < 8; ++i)
    {
        m_Textures.torchViews[i] = Live(kTorchViews[i]);
    }
    for (int b = 0; b < Village::kBuildings; ++b)
    {
        m_Textures.keepers[b] = kKeepers[b];
    }
}

const Texture* Crawler::InRam(const Texture* texture)
{
    // Its glowing copy when it has one (whose palettes are in RAM and
    // change), its pixels in RAM when there is room.
    const Texture* live = Live(texture);
    return m_TextureCache.Hold(live, live != texture);
}

const Texture* Crawler::Live(const Texture* texture) const
{
    for (int i = 0; i < m_AnimatedCount; ++i)
    {
        if (m_Animated[i].source == texture)
        {
            return &m_Animated[i].live;
        }
    }
    return texture;
}

DUNGEON_HOT void Crawler::AnimatePalettes()
{
    // Each glowing colour brightens and dims on its own wave: lava flows,
    // water shimmers, flames flicker. Echo stones only the zone's own.
    const Texture* echo = kZoneLooks[m_Game.GetZone()].echo;
    for (int k = 0; k < m_AnimatedCount; ++k)
    {
        Animated& a = m_Animated[k];
        const Texture& source = *a.source;
        bool otherEcho = false;
        for (const ZoneLook& look : kZoneLooks)
        {
            otherEcho = otherEcho || (look.echo == a.source && a.source != echo);
        }
        if (otherEcho)
        {
            continue;
        }
        for (int i = 0; i < source.colours; ++i)
        {
            if (a.hot[i] == 0)
            {
                continue;
            }
            const int speed = a.hot[i] == 1 ? 2 : 3;
            const Fix wave = Raycaster::Sin(static_cast<int>(m_TimeMs / speed) + i * 97 + k * 31);
            // 0.65 to 1.35 of the colour, in 1/256ths.
            const int scale = 256 + static_cast<int>((static_cast<int64_t>(wave) * 90) >> kFixShift);
            for (int level = 0; level < source.levels; ++level)
            {
                const uint16_t c = source.palette[level * source.colours + i];
                int r = ((c >> 11) * scale) >> 8;
                int g = (((c >> 5) & 63) * scale) >> 8;
                int b = ((c & 31) * scale) >> 8;
                r = r > 31 ? 31 : r;
                g = g > 63 ? 63 : g;
                b = b > 31 ? 31 : b;
                uint16_t value = static_cast<uint16_t>((r << 11) | (g << 5) | b);
                a.palette[level * source.colours + i] = value == kTransparent ? value ^ 1 : value;
            }
        }
    }
}

void Crawler::Setup()
{
    const int height = viewHeight > 0 && viewHeight <= kScreenHeight ? viewHeight : 240;
    // Chunky pixels: the 3D view at half the size each way, doubled. The
    // text and the HUD over it stay sharp.
    m_Ray.SetPixelSize(2);
    m_Ray.SetSize(320, height);
    // Only what a view of this size needs: RAM left free is what lets the
    // screen be sent while the next frame is drawn.
    const size_t rayBytes = Raycaster::StorageBytes(320, height, 2);
    // The view's storage, the textures' RAM and the glowing palettes. False
    // when the first two were not all had.
    const auto take = [&]()
    {
        const bool ray = m_RayStorage.Allocate((rayBytes + 7) / 8, Deki::Memory::Internal);
        const bool textures = m_TextureRam.Allocate(kTextureRam, Deki::Memory::Internal);
        SetupAnimated();
        return ray && textures;
    };
    // The display sends a frame while the next is drawn only if, at the
    // first frame, it finds room for two copies of a strip of rows: two
    // blocks of 25.6 KB on this screen (40 rows of 320). Taken now, the
    // game's own memory leaves what is free in pieces too small for them
    // (and every frame is then sent while the game waits: 18 fps for 30).
    // So their room is held while the game takes its own, and let go after:
    // it stays free, in one piece. If the game's own does not fit beside
    // it, the game comes first.
    {
        constexpr size_t kStripRoom = 320 * 40 * 2 + 2560;  // and a little over, for what is taken before then
        Deki::Buffer<uint8_t> room[2];
        const bool held = room[0].Allocate(kStripRoom, Deki::Memory::Internal) &&
                          room[1].Allocate(kStripRoom, Deki::Memory::Internal);
        const bool taken = take();
        room[0].Reset();
        room[1].Reset();
        if (!taken)
        {
            take();
        }
        if (!held || !taken)
        {
            DEKI_LOG_WARNING("Crawler: no room to keep the screen's strip copies in one piece; frames may be sent "
                             "while the game waits");
        }
    }
    if (m_RayStorage.Data())
    {
        m_Ray.SetStorage(m_RayStorage.Data(), rayBytes);
    }
    else
    {
        DEKI_LOG_ERROR("Crawler: no memory for the 3D view (%u bytes); it stays black",
                       static_cast<unsigned>(rayBytes));
    }
    m_Ray.SetClock([]() -> uint64_t { return Deki::Time::GetTimeUs(); });
    m_Ray.SetWallTextures(&kWallStoneTexture, nullptr);
    m_Ray.SetHideoutWalls(&kWallCrackedTexture, &kDoorTexture);
    m_Ray.SetFloorTextures(&kFloorTexture, &kCeilingTexture);
    if (m_TextureRam.Data())
    {
        m_TextureCache.SetStorage(m_TextureRam.Data(), kTextureRam);
    }
    m_Effects.SetDot(&kDustTexture);
    m_Ready = true;
    DEKI_LOG_INFO("Crawler: %u bytes of RAM free after the game took its own",
                  static_cast<unsigned>(Deki::Memory::GetAvailableInternal()));
}

void Crawler::Start()
{
    Setup();
    ResetKeys();
    m_Game.SetSoundVolume(kFirstVolume);
    LoadBest();
    m_SavedVolume = m_Game.GetSoundVolume();
    m_Chip.SetVolume(m_SavedVolume);
#if defined(DUNGEON_HAS_SPEAKER)
    if (!DekiClockworkPi::StartPicoCalcSpeaker(kSoundRate, &Crawler::FillSound, this))
    {
        DEKI_LOG_WARNING("Crawler: the speakers did not start; no sound");
    }
#elif defined(DUNGEON_HAS_SOUND_CARD)
    m_SoundCard = DekiSdl3::SDL3SampleOutput::Start(kSoundRate, &Crawler::FillSound, this);
    if (!m_SoundCard)
    {
        DEKI_LOG_WARNING("Crawler: no sound card to play through; no sound");
    }
#endif
    m_Game.SetDeepest(m_Deepest);
    m_Game.SetPagesKnown(m_PagesKnown);
    m_Game.SetLegend(m_Legend);
    m_SavedLegend = m_Game.GetLegendVersion();
    m_Game.GoHome(static_cast<uint32_t>(Deki::Time::GetTimeUs()) | 1u);

    ShowTitle(true);
    m_Listener = DekiInput::DekiInput::RegisterEventCallback(
        [this](const DekiInput::InputEvent& e)
        {
            if (e.type == DekiInput::InputEventType::KeyDown)
            {
                OnKey(e.key, e.character);
            }
        });
}

void Crawler::OnKey(uint32_t key, uint32_t character)
{
    namespace Keys = DekiInput::Keys;
    Action action = Action::None;
    if (!m_OnTitle && m_PlaceMs > Transitions::kPlaceOutMs)
    {
        return;  // still dark: the new place is not shown yet
    }
    if (!m_OnTitle && m_Game.IsDead())
    {
        // Any key goes home, once the fall has had a moment.
        if (m_DeadMs >= Transitions::kFallSkipMs)
        {
            m_Queued = Action::Restart;
        }
        return;
    }
    if (!m_OnTitle && m_Game.GetMenu() == Menu::Echo)
    {
        // An echo: a key brings all its lines up, the next one leaves.
        const int shown = Transitions::EchoShownMs(m_Game.GetEchoShown() >= Lore::kEchoes);
        if (m_EchoMs >= shown)
        {
            m_Queued = Action::MenuClose;
        }
        else if (m_EchoMs >= Transitions::kEchoKeyMs)
        {
            m_EchoMs = shown;
        }
        return;
    }
    if (!m_OnTitle && m_Game.GetMenu() == Menu::Talk)
    {
        // Someone speaking: a key brings all the words out, the next one
        // lets them go.
        const char* speech = m_Game.GetSpeech();
        if (MessageCard::Typed(speech, m_TalkMs) < MessageCard::Letters(speech))
        {
            m_TalkMs = 60000;
        }
        else
        {
            m_Queued = Action::MenuClose;
        }
        return;
    }
    const bool takes = key == Keys::Enter || key == m_Keys[6];
    if (!m_OnTitle && m_Game.GetMenu() == Menu::Keys && (m_KeyWanted >= 0 || takes))
    {
        // The keys: Enter on a line waits for its new key; the key that
        // comes is it (Esc keeps the old one), and whichever line had that
        // key takes the old one, so no two share.
        const int cursor = m_Game.GetMenuCursor();
        if (m_KeyWanted >= 0)
        {
            if (key != Keys::Esc && key != Keys::Enter && key < 1000)
            {
                for (uint32_t& other : m_Keys)
                {
                    other = other == key ? m_Keys[m_KeyWanted] : other;
                }
                m_Keys[m_KeyWanted] = key;
                SaveBest();
            }
            m_KeyWanted = -1;
            return;
        }
        if (cursor < kBindings)
        {
            m_KeyWanted = cursor;
            return;
        }
        if (cursor == kBindings)
        {
            ResetKeys();
            SaveBest();
            return;
        }
    }
    if (!m_OnTitle && m_Game.GetMenu() != Menu::None)
    {
        // In a menu: up and down choose, Enter (or the use key) takes, Esc
        // (or the menu key) leaves.
        if (key == Keys::Up || key == m_Keys[0])
        {
            action = Action::MenuUp;
        }
        else if (key == Keys::Down || key == m_Keys[1])
        {
            action = Action::MenuDown;
        }
        else if (takes || key == Keys::Right)
        {
            action = Action::MenuSelect;
        }
        else if (key == Keys::Esc || key == Keys::Backspace || key == Keys::Left || key == Keys::Tab ||
                 key == m_Keys[8])
        {
            action = Action::MenuClose;
        }
        // A card to read closes with any key.
        const Menu menu = m_Game.GetMenu();
        if (action == Action::None && (menu == Menu::Page || menu == Menu::Book || menu == Menu::Controls))
        {
            action = Action::MenuClose;
        }
        if (action != Action::None)
        {
            m_Queued = action;
        }
        return;
    }
    // The keys that can be changed first, then the ones that always work.
    constexpr Action kBound[kBindings] = { Action::Forward,   Action::Back,       Action::TurnLeft,
                                           Action::TurnRight, Action::StrafeLeft, Action::StrafeRight,
                                           Action::Interact,  Action::Wait,       Action::OpenMenu };
    for (int i = 0; i < kBindings; ++i)
    {
        action = key == m_Keys[i] ? kBound[i] : action;
    }
    if (action == Action::None)
    {
        switch (key)
        {
            case Keys::Tab: action = Action::OpenBag; break;
            case Keys::Up: action = Action::Forward; break;
            case Keys::Down: action = Action::Back; break;
            case Keys::Left: action = Action::TurnLeft; break;
            case Keys::Right: action = Action::TurnRight; break;
            case Keys::Enter: action = Action::Interact; break;
            default: break;
        }
    }
    if (m_OnTitle)
    {
        // Any key leaves the title; the run starts fresh.
        ShowTitle(false);
        m_Game.GoHome(static_cast<uint32_t>(Deki::Time::GetTimeUs()) | 1u);
        if (!m_Game.GetLegend().controlsShown)
        {
            m_Game.ShowControls();
        }
        m_ToastMessage = 0xFFFFFFFF;
        return;
    }
    // One move waits while the last still glides, so quick presses chain.
    if (action != Action::None)
    {
        m_Queued = action;
    }
}

void Crawler::Update()
{
    // The frame rate in the log every two seconds, for measuring on a board.
    const uint64_t nowUs = Deki::Time::GetTimeUs();
    if (m_FpsWindowUs == 0)
    {
        m_FpsWindowUs = nowUs;
    }
    ++m_FpsFrames;
    if (nowUs - m_FpsWindowUs >= 2000000)
    {
        const uint32_t tenths = static_cast<uint32_t>(m_FpsFrames * 10000000ull / (nowUs - m_FpsWindowUs));
        // And where a frame's time went, in tenths of a millisecond: getting
        // the view ready (the rays alone in brackets), drawing the view, its
        // overlays and menu, and the HUD. The rest is the engine and the
        // screen.
        const auto tenthsMs = [this](uint64_t us)
        { return static_cast<unsigned>(us / (m_FpsFrames > 0 ? m_FpsFrames : 1) / 100); };
        const unsigned prepare = tenthsMs(m_PrepareUs);
        const unsigned rays = tenthsMs(m_RaysUs);
        const unsigned view = tenthsMs(m_ViewUs);
        const unsigned overlays = tenthsMs(m_OverlaysUs);
        const unsigned hud = tenthsMs(m_HudUs);
        // The view's own stages, and where the hero is (a floor, the village
        // or a building), so a slow place shows itself.
        const unsigned walls = tenthsMs(m_Ray.TakeStageUs(Raycaster::kWallsStage));
        const unsigned floors = tenthsMs(m_Ray.TakeStageUs(Raycaster::kFloorsStage));
        const unsigned sprites = tenthsMs(m_Ray.TakeStageUs(Raycaster::kSpritesStage));
        const int place = m_OnTitle ? -3 : m_Game.GetInside() >= 0 ? -2 : m_Game.IsInVillage() ? -1 : m_Game.GetDepth();
        // And the RAM left free: too little, and the screen is sent while
        // the game waits.
        DEKI_LOG_INFO("Crawler: %u.%u fps at %d | ms per frame: prepare %u.%u (rays %u.%u), view %u.%u (walls %u.%u, "
                      "floors %u.%u, sprites %u.%u), overlays %u.%u, hud %u.%u | %u KB free",
                      static_cast<unsigned>(tenths / 10), static_cast<unsigned>(tenths % 10), place, prepare / 10,
                      prepare % 10, rays / 10, rays % 10, view / 10, view % 10, walls / 10, walls % 10, floors / 10,
                      floors % 10, sprites / 10, sprites % 10, overlays / 10, overlays % 10, hud / 10, hud % 10,
                      static_cast<unsigned>(Deki::Memory::GetAvailableInternal() / 1024));
        m_FpsWindowUs = nowUs;
        m_FpsFrames = 0;
        m_PrepareUs = 0;
        m_RaysUs = 0;
        m_ViewUs = 0;
        m_OverlaysUs = 0;
        m_HudUs = 0;
    }

    const int ms = static_cast<int>(Deki::Time::GetDeltaTimeF());
    // Behind an open menu the world stands still (once the hero has come to
    // rest): no torch flickers, nothing bobs, so the view is not drawn again
    // and the menu costs no frame.
    const bool held = !m_OnTitle && m_Game.GetMenu() != Menu::None && !m_Game.IsGliding();
    m_TimeMs += held ? 0 : static_cast<uint32_t>(ms);
    // A blow freezes the game a moment (hit-stop): the sword stays in the
    // monster, then all goes on.
    m_Game.Tick(held ? 0 : m_Effects.GameMs(ms));
    const bool menuAction = m_Queued == Action::MenuUp || m_Queued == Action::MenuDown ||
                            m_Queued == Action::MenuSelect || m_Queued == Action::MenuClose;
    if (m_Queued != Action::None && (!m_Game.IsGliding() || menuAction))
    {
        const bool wasDead = m_Game.IsDead();
        m_Game.Act(m_Queued);
        m_Queued = Action::None;
        if (m_Game.GetDeepest() > m_Deepest || m_Game.GetPagesKnown() > m_PagesKnown ||
            m_Game.GetLegendVersion() != m_SavedLegend)
        {
            m_Deepest = m_Game.GetDeepest() > m_Deepest ? m_Game.GetDeepest() : m_Deepest;
            m_PagesKnown = m_Game.GetPagesKnown();
            m_Legend = m_Game.GetLegend();
            m_SavedLegend = m_Game.GetLegendVersion();
            SaveBest();
        }
        if (wasDead && !m_Game.IsDead())
        {
            // Home: the village's card takes it from the dark.
            m_DeadMs = 0;
            m_ShownInside = m_Game.GetInside();
        }
        // The story told: back to the title, which now shows how it ended.
        if (m_Game.IsRunWon() && m_Game.GetMenu() == Menu::None)
        {
            ShowTitle(true);
        }
    }
    m_ToastMs = m_ToastMs > ms ? m_ToastMs - ms : 0;
    m_FadeInMs = m_FadeInMs > ms ? m_FadeInMs - ms : 0;
    if (!m_OnTitle && m_Game.GetMenu() == Menu::Talk)
    {
        // Someone speaking: a new line starts from its first letter.
        m_TalkMs = m_Game.GetSpeechCount() != m_TalkSeen ? 0 : (m_TalkMs + ms < 60000 ? m_TalkMs + ms : 60000);
        m_TalkSeen = m_Game.GetSpeechCount();
    }
    if (m_Game.GetMenu() != Menu::Keys)
    {
        m_KeyWanted = -1;
    }
    if (!m_OnTitle && m_Game.GetMenu() == Menu::Echo)
    {
        // An echo stone speaks: counted until all of it is up.
        const int most = Transitions::EchoShownMs(m_Game.GetEchoShown() >= Lore::kEchoes);
        m_EchoMs = m_EchoMs + ms < most ? m_EchoMs + ms : most;
    }
    else if (m_EchoMs > 0)
    {
        // Heard: the view comes back out of the dark.
        m_EchoMs = 0;
        m_FadeInMs = kEchoFadeMs;
        m_FadeInLength = kEchoFadeMs;
    }
    if (!m_OnTitle && m_Game.IsDead())
    {
        // Left a while, the card goes home by itself.
        m_DeadMs += ms;
        if (m_DeadMs >= Transitions::FallLength(m_Game) && m_Queued == Action::None)
        {
            m_Queued = Action::Restart;
        }
    }
    m_PlaceMs = m_PlaceMs > ms ? m_PlaceMs - ms : 0;
    if (!m_OnTitle && PlaceKey() != m_Place)
    {
        // Somewhere new: its name on black first.
        m_Place = PlaceKey();
        m_PlaceMs = Transitions::kPlaceMs;
        m_ShownInside = m_Game.GetInside();
    }
    if (m_Game.GetInside() != m_ShownInside)
    {
        // Through a door: a blink of dark.
        m_ShownInside = m_Game.GetInside();
        m_FadeInMs = kDoorFadeMs;
        m_FadeInLength = kDoorFadeMs;
    }
    if (!m_OnTitle)
    {
        m_Effects.Update(m_Game, m_Textures, m_Ray.GetWidth(), m_Ray.GetHeight(), ms);
        // What changed, as sound; and how loud, kept once it is changed.
        const bool speaking = m_Game.GetMenu() == Menu::Talk;
        Cue asked[CrawlerGame::kMaxCues];
        const int askedCount = m_Game.TakeCues(asked, CrawlerGame::kMaxCues);
        m_Cues.Update(m_Game, m_Effects.Heard(), speaking ? MessageCard::Typed(m_Game.GetSpeech(), m_TalkMs) : 0, 0, 0,
                      asked, askedCount, ms, m_Chip);
        if (m_Game.GetSoundVolume() != m_SavedVolume)
        {
            m_SavedVolume = m_Game.GetSoundVolume();
            m_Chip.SetVolume(m_SavedVolume);
            m_Chip.Play(Chip::Sfx::Coin);  // how loud that is
            SaveBest();
        }
    }
    // The title has its tune; the game its sounds alone.
    m_Chip.SetMusic(m_OnTitle ? Chip::Music::Title : Chip::Music::None);
    const uint64_t prepareFrom = Deki::Time::GetTimeUs();
    PrepareView(ms);
    m_PrepareUs += Deki::Time::GetTimeUs() - prepareFrom;
}

void Crawler::PrepareView(int ms)
{
    // The zone's look. Inside a building is indoors: no sky.
    const bool inside = m_Game.GetInside() >= 0;
    const bool village = m_Game.IsInVillage() && !inside;
    const ZoneLook& look = inside ? kInteriorLook : village ? kVillageLook : kZoneLooks[m_Game.GetZone()];
    const int lookKey = inside ? -2 : village ? -1 : m_Game.GetZone();
    if (lookKey != m_CachedLook)
    {
        // Another zone: its textures into RAM, the most read first.
        m_CachedLook = lookKey;
        m_TextureCache.Clear();
    }
    if (village)
    {
        for (int b = 0; b < Village::kBuildings; ++b)
        {
            const Cell c = Village::BuildingCell(b);
            m_VillageCells[c.y * kMapWidth + c.x] =
                static_cast<uint8_t>(m_Game.GetLegend().buildings[b] > 0 ? b + 1 : Village::kBuildings + 1);
        }
        for (int b = 0; b <= Village::kBuildings; ++b)
        {
            m_VillageFronts[b] = kVillageFronts[b];
        }
        m_Ray.SetCellWalls(m_VillageCells, m_VillageFronts, Village::kBuildings + 1);
        m_Ray.SetWallTops(kVillageTops, 2, kVillageFrontTops);
        m_Ray.SetSky(&kSkyTexture);
        // The alley between the houses, in their shadow.
        for (int y = 0; y < kMapHeight; ++y)
        {
            for (int x = 0; x < kMapWidth; ++x)
            {
                m_Gloom[y * kMapWidth + x] = static_cast<uint8_t>(Village::Gloom({ x, y }));
            }
        }
        m_Ray.SetGloom(m_Gloom);
    }
    else
    {
        m_Ray.SetCellWalls(nullptr, nullptr, 0);
        m_Ray.SetWallTops(nullptr, 0, nullptr);
        m_Ray.SetSky(nullptr);
        m_Ray.SetGloom(nullptr);
    }
    if (village != m_Sky)
    {
        // Dusk over the village: warm at the horizon, deep blue overhead.
        m_Sky = village;
        if (village)
        {
            m_Ray.SetFloorColours(SmoothText::Rgb(206, 122, 98), SmoothText::Rgb(24, 26, 62), 0x0000, 0x4208);
        }
        else
        {
            m_Ray.SetFloorColours(0x0000, 0x2104, 0x0000, 0x4208);
        }
    }
    AnimatePalettes();
    // In the order they are read most: the walls, the floor and ceiling,
    // the floor's marks, then the rarer walls.
    const Texture* walls[Raycaster::kMaxWallKinds];
    for (int i = 0; i < look.wallCount; ++i)
    {
        walls[i] = InRam(look.walls[i]);
    }
    const Texture* floors[Raycaster::kMaxFloorKinds] = { InRam(look.floors[0]), InRam(look.floors[1]) };
    const Texture* ceiling = InRam(look.ceiling);
    m_Ray.SetFloorLook(FloorLook::StairsHole, InRam(&kFloorHoleTexture));
    m_Ray.SetFloorLook(FloorLook::Spikes, InRam(&kFloorSpikesTexture));
    m_Ray.SetFloorLook(FloorLook::Rune, InRam(&kFloorRuneTexture));
    if (village)
    {
        m_Ray.SetSky(InRam(&kSkyTexture));
        for (int b = 0; b <= Village::kBuildings; ++b)
        {
            m_VillageFronts[b] = InRam(kVillageFronts[b]);
        }
        m_Ray.SetCanopy(InRam(&Textures::kCanopyTexture));  // the alley's cloth roof
    }
    else
    {
        m_Ray.SetCanopy(nullptr);
    }
    m_Ray.SetWalls(walls, look.wallCount);
    // The cracked wall as broken as its blows have left it.
    const int blows = m_Game.GetWallBlows();
    const Texture* cracked =
        village || inside || blows == 0 ? look.cracked : kBroken[m_Game.GetZone()][blows < 3 ? 0 : 1];
    m_Ray.SetHideoutWalls(InRam(cracked), InRam(&kDoorTexture), look.echo ? InRam(look.echo) : nullptr);
    m_Textures.rubble = look.walls[0];
    m_Ray.SetFloors(floors, Raycaster::kMaxFloorKinds, ceiling);
    m_Ray.SetReflectiveFloor(m_Game.GetZone() == 2);
    m_Ray.SetTime(m_TimeMs);
    // The torches light the walls round them; their reach flickers.
    Fix lightX[CrawlerGame::kMaxTorches];
    Fix lightY[CrawlerGame::kMaxTorches];
    int lights = 0;
    for (int i = 0; i < CrawlerGame::kMaxTorches; ++i)
    {
        const Torch& t = m_Game.GetTorch(i);
        if (t.present)
        {
            lightX[lights] = t.x;
            lightY[lights] = t.y;
            ++lights;
        }
    }
    // Each torch's pool of light breathes: two waves, so it never repeats
    // plainly.
    m_Ray.SetLights(lightX, lightY, lights,
                    kFixOne * 5 / 2 + Raycaster::Sin(static_cast<int>(m_TimeMs / 3)) / 6 +
                        Raycaster::Sin(static_cast<int>(m_TimeMs * 7 / 5)) / 14);

    // The torch flickers: its reach wavers a little, two slow waves mixed.
    // Out in the sun it reaches across the village; down below, a little
    // less than the torch would like.
    // Indoors, the torches and the windows: more than below, less than
    // outside.
    const Fix reach = village  ? (lightReach * kFixOne) / 10 * 4
                      : inside ? (lightReach * kFixOne) / 100 * 13
                               : (lightReach * kFixOne) / 100 * 8;
    const Fix flicker =
        (Raycaster::Sin(static_cast<int>(m_TimeMs / 4)) + Raycaster::Sin(static_cast<int>(m_TimeMs * 5 / 11))) / 40;
    m_Ray.SetLightReach(reach + flicker);

    // A step bobs the view: down and back up once.
    int bob = 0;
    if (m_Game.IsStepping())
    {
        bob = static_cast<int>((static_cast<int64_t>(Raycaster::Sin(m_Game.GlideProgress() / 2)) * 4) >> kFixShift);
    }
    // A blow or a hurt jolts the view up and down, less and less.
    bob += m_OnTitle ? 0 : m_Effects.Kick();
    m_Ray.SetHorizonOffset(bob);

    Fix x = 0;
    Fix y = 0;
    int angle = 0;
    m_Game.GetView(x, y, angle);
    if (!m_OnTitle && m_Game.GetTipsy() > 0)
    {
        // Tipsy: the view sways, less as it wears off.
        const int amount = m_Game.GetTipsy() < 10 ? m_Game.GetTipsy() : 10;
        angle += static_cast<int>((static_cast<int64_t>(Raycaster::Sin(static_cast<int>(m_TimeMs / 4))) * amount) >>
                                  kFixShift);
    }
    if (m_OnTitle)
    {
        // The title's backdrop: the first room, turning slowly.
        m_TitleAngle += ms;
        angle += m_TitleAngle / 24;
    }

    // The weapon in hand, bottom right: it rises and falls with each breath
    // and drifts a little, swings with a step and thrusts forward on an
    // attack.
    Overlay sword;
    if (!m_OnTitle && !m_Game.IsDead())
    {
        const int time = static_cast<int>(m_TimeMs);
        const Fix breath = Raycaster::Sin(time * 3 / 7);  // about 2.4 s a breath
        const Fix drift = Raycaster::Sin(time * 3 / 14 + kAngleSteps / 4);
        const Fix stride = m_Game.IsStepping() ? Raycaster::Sin(m_Game.GlideProgress() / 2) : 0;
        sword.texture = kHands[static_cast<int>(m_Game.GetWeapon())];
        sword.scale = 2;
        sword.x = m_Ray.GetWidth() - 64 * 2 + 4 + static_cast<int>((static_cast<int64_t>(drift) * 3) >> kFixShift) -
                  static_cast<int>((static_cast<int64_t>(stride) * 6) >> kFixShift);
        sword.y = m_Ray.GetHeight() - 64 * 2 + 36 + bob * 2 +
                  static_cast<int>((static_cast<int64_t>(breath) * 4) >> kFixShift);
        const int swing = m_Game.SwingProgress();
        if (swing >= 0)
        {
            const Fix thrust = Raycaster::Sin(swing / 2);  // 0 -> 1 -> 0
            sword.x -= static_cast<int>((static_cast<int64_t>(thrust) * 56) >> kFixShift);
            sword.y -= static_cast<int>((static_cast<int64_t>(thrust) * 44) >> kFixShift);
        }
    }
    m_Ray.SetOverlay(sword);
    for (Billboard& b : m_Billboards)
    {
        b = Billboard{};  // nothing kept from the last frame (a flash's palette)
    }
    int count = m_Game.GetBillboards(m_Textures, m_Billboards, Raycaster::kMaxBillboards);
    count = m_Effects.AddBillboards(m_Billboards, count, Raycaster::kMaxBillboards);
    m_ShadowCount = m_Game.GetShadows(m_Shadows, Raycaster::kMaxShadows);
    m_Ray.SetShadows(m_Shadows, m_ShadowCount);
    const uint64_t raysFrom = Deki::Time::GetTimeUs();
    m_Ray.Prepare(m_Game.GetMap(), x, y, angle, m_Billboards, count);
    m_RaysUs += Deki::Time::GetTimeUs() - raysFrom;
    m_Effects.Place(m_Ray);

    // What the view and the HUD show, to redraw only what changed.
    uint32_t view = Hash(Hash(Hash(2166136261u, x), y), angle);
    view = Hash(Hash(view, reach + flicker), bob);
    view = Hash(Hash(Hash(view, sword.x), sword.y), sword.texture != nullptr);
    NameKeys();
    m_Panel.SetKeyNames(m_KeyNames, kBindings, m_KeyWanted);
    m_Panel.Build(m_Game, kCrawlerTextures, m_Ray.GetWidth(), m_Ray.GetHeight(), m_OnTitle);
    view = Hash(view, m_Panel.IsShown() ? static_cast<int32_t>(m_Game.GetVersion()) : -1);
    view = Hash(view, ToastShown() ? m_ToastVersion : -1);
    view = Hash(view, ToastShown() && ToastTyped() < 64 ? ToastTyped() : 64);
    view = Hash(Hash(Hash(view, m_Game.IsDead() ? m_DeadMs / 30 : -1), m_FadeInMs / 30), m_PlaceMs / 30);
    view = Hash(view, m_EchoMs / 30);
    // Someone speaking (as far as it is typed), a result (while it waits and
    // comes up), the keys menu's keys.
    const Menu open = m_OnTitle ? Menu::None : m_Game.GetMenu();
    view = Hash(view, open == Menu::Talk ? MessageCard::Typed(m_Game.GetSpeech(), m_TalkMs) : -1);
    view = Hash(view, m_Game.GetWallBlows());
    view = Hash(view, static_cast<int32_t>(m_Game.GetSpeechCount() * 31 + m_Game.GetResultCount()));
    view = Hash(view, m_KeyWanted);
    for (const uint32_t bound : m_Keys)
    {
        view = Hash(view, static_cast<int32_t>(bound));
    }
    // The prompt: what is ahead, what E does to it, a monster's health.
    m_Target = m_Game.GetTarget();
    view = Hash(Hash(Hash(view, static_cast<int32_t>(reinterpret_cast<uintptr_t>(m_Target.name))),
                     static_cast<int32_t>(reinterpret_cast<uintptr_t>(m_Target.verb))),
                m_Target.hp);
    view = Hash(Hash(view, static_cast<int32_t>(reinterpret_cast<uintptr_t>(m_Target.detail))), m_Game.GetTipsy() > 0);
    view = Hash(Hash(view, m_Target.needGold * 16 + m_Target.needMaterial),
                m_Target.haveGold * 64 + m_Target.haveMaterial);
    int flashKind = 0;
    view = Hash(Hash(view, m_Game.FlashMs(flashKind)), static_cast<int32_t>(m_Game.GetWeapon()));
    view = Hash(view, m_OnTitle ? static_cast<int32_t>(m_TimeMs / 50) : -1);
    view = Hash(Hash(view, village ? static_cast<int32_t>(m_Game.GetLegendVersion()) : -1), m_Game.GetInside());
    view = Hash(view, m_Game.GetLegend().disco && !m_OnTitle ? static_cast<int32_t>(m_TimeMs / 60) : -1);
    for (int i = 0; i < count; ++i)
    {
        view = Hash(Hash(Hash(view, m_Billboards[i].x), m_Billboards[i].y), m_Billboards[i].scale);
    }
    const Cell hero = m_Game.GetHeroCell();
    uint32_t hud = Hash(Hash(Hash(2166136261u, static_cast<int32_t>(m_Game.GetVersion())), m_Game.GetHp()),
                        (hero.x * 64 + hero.y) * 4 + m_Game.GetFacing());
    hud = Hash(Hash(Hash(hud, m_Game.GetXp()), m_Game.GetMp()), m_Game.GetShards() * 2 + m_Game.IsInSeam());
    for (int i = 0; i < m_Game.GetMonsterCount(); ++i)
    {
        const Monster& m = m_Game.GetMonster(i);
        hud = Hash(hud, m.alive ? m.at.x * 64 + m.at.y : -1);
    }
    hud = Hash(Hash(Hash(hud, m_Effects.GetHpTrail()), m_Effects.GetGoldShown()), m_Effects.GetLevelMs() > 0);
    m_ViewChanged = m_ViewChanged || view != m_ViewSignature || (!m_OnTitle && m_Effects.IsActive());
    m_HudChanged = m_HudChanged || hud != m_HudSignature;
    m_ViewSignature = view;
    m_HudSignature = hud;
    UpdateHud();
}

void Crawler::UpdateHud()
{
    // Only a message just said shows. Other changes (a menu closed, MP
    // back) must not bring the last one back.
    if (m_Game.GetMessageCount() == m_ToastMessage)
    {
        return;
    }
    m_ToastMessage = m_Game.GetMessageCount();
    if (CardUp())
    {
        return;  // it is on a card of its own: not said twice
    }
    const char* message = m_Game.GetMessage();
    if (message[0] != 0)
    {
        std::snprintf(m_ToastText, sizeof(m_ToastText), "%s", message);
        m_ToastLength = MessageCard::ShownMs(m_ToastText);
        m_ToastMs = m_ToastLength;
        ++m_ToastVersion;
    }
}

int Crawler::ToastTyped() const
{
    return MessageCard::Typed(m_ToastText, m_ToastLength - m_ToastMs);
}

bool Crawler::CardUp() const
{
    return !m_OnTitle && m_Game.GetMenu() == Menu::Talk;
}

void Crawler::ResetKeys()
{
    namespace Keys = DekiInput::Keys;
    constexpr uint32_t kDefault[kBindings] = { Keys::W, Keys::S, Keys::A,     Keys::D, Keys::Q,
                                               Keys::C, Keys::E, Keys::Space, Keys::M };
    for (int i = 0; i < kBindings; ++i)
    {
        m_Keys[i] = kDefault[i];
    }
}

void Crawler::NameKeys()
{
    namespace Keys = DekiInput::Keys;
    for (int i = 0; i < kBindings; ++i)
    {
        const uint32_t key = m_Keys[i];
        const char* name = key == Keys::Space       ? "Space"
                           : key == Keys::Tab       ? "Tab"
                           : key == Keys::Backspace ? "Back"
                                                    : nullptr;
        if (name)
        {
            std::snprintf(m_KeyNames[i], sizeof(m_KeyNames[i]), "%s", name);
        }
        else if (key > 32 && key < 127)
        {
            // A letter shows as its capital.
            const char letter = static_cast<char>(key >= 'a' && key <= 'z' ? key - 'a' + 'A' : key);
            std::snprintf(m_KeyNames[i], sizeof(m_KeyNames[i]), "%c", letter);
        }
        else
        {
            std::snprintf(m_KeyNames[i], sizeof(m_KeyNames[i]), "#%u", static_cast<unsigned>(key));
        }
    }
}

bool Crawler::ToastShown() const
{
    return !m_OnTitle && !m_Panel.IsShown() && m_EchoMs == 0 && !CardUp() && m_ToastText[0] != 0 && m_ToastMs > 0 &&
           !m_Game.IsDead();
}

bool Crawler::IsNativeScreen(const DekiRendering::RenderContext& ctx)
{
    if (ctx.format != Deki::ColorFormat::RGB565)
    {
        return false;
    }
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
    const int fullWidth = ctx.viewWidth > 0 ? ctx.viewWidth : ctx.width;
    const int fullHeight = ctx.viewHeight > 0 ? ctx.viewHeight : ctx.height;
    if (!GameScreenRect(ctx, left, top, right, bottom))
    {
        return fullWidth == m_Ray.GetWidth() && fullHeight == kScreenHeight;
    }
    return std::fabs(left) < 0.5f && std::fabs(top + ctx.viewTop) < 0.5f &&
           std::fabs(right - m_Ray.GetWidth()) < 0.5f && std::fabs(bottom + ctx.viewTop - kScreenHeight) < 0.5f;
}

void Crawler::AddChanges(const DekiRendering::RenderContext& ctx, DekiRendering::DirtyRegion& changed)
{
    if (!IsNativeScreen(ctx))
    {
        // Scaled into a bigger frame: the game's screen rows are not the
        // frame's, so the whole frame, when anything changed.
        if (m_ViewChanged || m_HudChanged)
        {
            changed.Add(0, 0, ctx.viewWidth > 0 ? ctx.viewWidth : ctx.width,
                        ctx.viewHeight > 0 ? ctx.viewHeight : ctx.height);
        }
        m_ViewChanged = false;
        m_HudChanged = false;
        return;
    }
    const int viewEnd = m_Ray.GetHeight();
    if (m_ViewChanged)
    {
        changed.Add(0, 0, m_Ray.GetWidth(), viewEnd);
    }
    if (m_HudChanged)
    {
        changed.Add(0, viewEnd, m_Ray.GetWidth(), kScreenHeight);
    }
    // Reported once: the next frame starts from nothing changed.
    m_ViewChanged = false;
    m_HudChanged = false;
}

void Crawler::ShowTitle(bool shown)
{
    m_OnTitle = shown;
    // Leaving the title, the first place shows its card.
    m_Place = kNoPlace;
    m_PlaceMs = 0;
    m_TitleAngle = 0;
    m_Effects.Reset(m_Game);
    m_Cues.Reset(m_Game);
    m_ToastText[0] = 0;
    m_HudChanged = true;
    m_ViewChanged = true;
}

void Crawler::Draw(DekiRendering::RenderContext& ctx)
{
    // The game draws RGB565, the board's own format. A 32-bit frame (the
    // editor's Play view, a desktop window) gets the same picture through the
    // preview buffer, each pixel widened.
    const bool wide = ctx.format == Deki::ColorFormat::ARGB8888;
    if ((ctx.format != Deki::ColorFormat::RGB565 && !wide) || ctx.viewLeft != 0 ||
        (!wide && ctx.width < m_Ray.GetWidth()))
    {
        return;
    }
    if (!m_Ready)
    {
        // Not started: the editor showing the scene. A floor to look at.
        Setup();
        m_Game.NewRun(7);
        PrepareView(0);
    }
    // Only inside the part being redrawn (all of the buffer when empty), in
    // the view's own rows.
    int clipLeft = 0;
    int clipRight = ctx.width;
    int top = ctx.viewTop;
    int bottom = ctx.viewTop + ctx.height;
    if (!ctx.redraw.Empty())
    {
        clipLeft = ctx.redraw.left;
        clipRight = ctx.redraw.right;
        top = ctx.viewTop + ctx.redraw.top;
        bottom = ctx.viewTop + ctx.redraw.bottom;
    }
    auto* rows = reinterpret_cast<uint16_t*>(ctx.buffer);
    const int stride = ctx.width * 2;

    // Where the game's screen is in this frame. On the game's own screen it
    // is the whole frame, drawn pixel for pixel. Anywhere else (the editor's
    // Scene View, a camera zoomed or moved) the screen is drawn whole into a
    // buffer of its own and scaled into the camera's frame.
    float left = 0.0f;
    float topEdge = 0.0f;
    float right = 0.0f;
    float bottomEdge = 0.0f;
    const bool known = GameScreenRect(ctx, left, topEdge, right, bottomEdge);
    const int fullWidth = ctx.viewWidth > 0 ? ctx.viewWidth : ctx.width;
    const int fullHeight = ctx.viewHeight > 0 ? ctx.viewHeight : ctx.height;
    const bool native = IsNativeScreen(ctx);
    if (!native)
    {
        if (!known)
        {
            // No camera to place it by: the whole frame, as on the board.
            left = 0.0f;
            topEdge = static_cast<float>(-ctx.viewTop);
            right = static_cast<float>(fullWidth);
            bottomEdge = static_cast<float>(fullHeight - ctx.viewTop);
        }
        if (right - left < 1.0f || bottomEdge - topEdge < 1.0f ||
            !m_Preview.Allocate(static_cast<size_t>(m_Ray.GetWidth()) * kScreenHeight, Deki::Memory::External))
        {
            return;
        }
        uint16_t* screen = m_Preview.Data();
        const int width = m_Ray.GetWidth();
        DrawScreen(screen, width * 2, 0, kScreenHeight, 0, width);
        QuadBlit::BeforeWriting(ctx.buffer);
        const int x0 = std::max(clipLeft, static_cast<int>(std::ceil(left)));
        const int x1 = std::min(clipRight, static_cast<int>(std::ceil(right)));
        const int y0 = std::max(static_cast<int>(top - ctx.viewTop), static_cast<int>(std::ceil(topEdge)));
        const int y1 = std::min(static_cast<int>(bottom - ctx.viewTop), static_cast<int>(std::ceil(bottomEdge)));
        const float sx = width / (right - left);
        const float sy = kScreenHeight / (bottomEdge - topEdge);
        for (int y = y0; y < y1; ++y)
        {
            const int from = std::min(kScreenHeight - 1, static_cast<int>((y + 0.5f - topEdge) * sy));
            const uint16_t* in = screen + from * width;
            if (wide)
            {
                // RGB565 to 0xAARRGGBB, the top bits of each channel copied
                // into the new low ones so white stays white.
                uint32_t* out = reinterpret_cast<uint32_t*>(ctx.buffer) + y * ctx.width;
                for (int x = x0; x < x1; ++x)
                {
                    const uint16_t c = in[std::min(width - 1, static_cast<int>((x + 0.5f - left) * sx))];
                    const uint32_t r = (c >> 11) & 31;
                    const uint32_t g = (c >> 5) & 63;
                    const uint32_t b = c & 31;
                    out[x] = 0xFF000000u | (((r << 3) | (r >> 2)) << 16) | (((g << 2) | (g >> 4)) << 8) |
                             ((b << 3) | (b >> 2));
                }
                continue;
            }
            uint16_t* out = rows + y * ctx.width;
            for (int x = x0; x < x1; ++x)
            {
                out[x] = in[std::min(width - 1, static_cast<int>((x + 0.5f - left) * sx))];
            }
        }
        QuadBlit::NoteWritten(ctx.buffer);
        return;
    }

    QuadBlit::BeforeWriting(ctx.buffer);
    DrawScreen(rows - ctx.viewTop * ctx.width, stride, top, bottom, clipLeft, clipRight);
    QuadBlit::NoteWritten(ctx.buffer);
}

bool Crawler::GameScreenRect(const DekiRendering::RenderContext& ctx, float& left, float& top, float& right,
                             float& bottom)
{
    DekiRendering::CameraComponent* camera = gameCamera.Get();
    if (!camera || !ctx.cam.valid)
    {
        return false;
    }
    // The camera sees orthoHeight metres down and as many across as the
    // screen's shape gives; its frame mapped through this frame's camera.
    const float halfH = camera->GetOrthoHeight() / 2.0f;
    const float halfW = halfH * m_Ray.GetWidth() / kScreenHeight;
    const float cx = camera->GetPositionX();
    const float cy = camera->GetPositionY();
    ctx.cam.WorldToScreen(cx - halfW, cy + halfH, left, top);
    ctx.cam.WorldToScreen(cx + halfW, cy - halfH, right, bottom);
    return true;
}

void Crawler::DrawScreen(uint16_t* rowsAt0, int strideBytes, int top, int bottom, int clipLeft, int clipRight)
{
    // `rowsAt0` is where screen row 0 would be; only rows [top, bottom) are
    // touched.
    const int stride = strideBytes / 2;
    const auto rowOf = [&](int screenRow) { return rowsAt0 + screenRow * stride; };

    // The 3D view: rows 0 .. viewHeight.
    const int viewEnd = m_Ray.GetHeight();
    const int first = top > 0 ? top : 0;
    const int last = bottom < viewEnd ? bottom : viewEnd;
    if (first < last)
    {
        const uint64_t viewFrom = Deki::Time::GetTimeUs();
        m_Ray.RenderRows(rowOf(first), strideBytes, first, last - first, clipLeft, clipRight);
        const uint64_t overlaysFrom = Deki::Time::GetTimeUs();
        DrawOverlays(rowOf(first), stride, first, last - first, clipLeft, clipRight);
        m_Panel.DrawRows(rowOf(first), strideBytes, first, last - first, clipLeft, clipRight);
        const uint64_t overlaysTo = Deki::Time::GetTimeUs();
        m_ViewUs += overlaysFrom - viewFrom;
        m_OverlaysUs += overlaysTo - overlaysFrom;
    }

    // The HUD's panel, bar and minimap: the rows below the view.
    const int hudFirst = top > viewEnd ? top : viewEnd;
    if (hudFirst < bottom)
    {
        const uint64_t hudFrom = Deki::Time::GetTimeUs();
        DrawHud(rowOf(hudFirst), strideBytes, hudFirst - viewEnd, bottom - hudFirst, clipLeft, clipRight);
        m_HudUs += Deki::Time::GetTimeUs() - hudFrom;
    }
}

namespace
{

// A 16x16 icon (a texture's brightest palette) with its top left at (x, y).
void DrawIcon(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int x, int y,
              const Texture& t)
{
    for (int i = 0; i < rowCount; ++i)
    {
        const int v = firstRow + i - y;
        if (v < 0 || v >= t.size)
        {
            continue;
        }
        uint16_t* out = dst + i * stride;
        const uint8_t* texRow = t.index + v * t.size;
        for (int u = 0; u < t.size; ++u)
        {
            const int px = x + u;
            const uint16_t c = t.palette[texRow[u]];
            if (px >= clipLeft && px < clipRight && c != kTransparent)
            {
                out[px] = c;
            }
        }
    }
}

// A window: deep blue inside a white frame with rounded corners.
void DrawWindow(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight, int x0, int y0,
                int x1, int y1)
{
    for (int i = 0; i < rowCount; ++i)
    {
        const int row = firstRow + i;
        if (row < y0 || row >= y1)
        {
            continue;
        }
        uint16_t* out = dst + i * stride;
        const int edgeY = row - y0 < y1 - 1 - row ? row - y0 : y1 - 1 - row;
        for (int x = clipLeft > x0 ? clipLeft : x0; x < x1 && x < clipRight; ++x)
        {
            const int edgeX = x - x0 < x1 - 1 - x ? x - x0 : x1 - 1 - x;
            if (edgeX + edgeY < 3)
            {
                continue;
            }
            const int nearest = edgeX < edgeY ? edgeX : edgeY;
            out[x] = nearest < 1 ? 0x0000 : nearest < 3 ? kHudFrame : kHudBackground;
        }
    }
}

}  // namespace

DUNGEON_HOT void Crawler::DrawOverlays(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft,
                                       int clipRight) const
{
    int kind = 0;
    const int flash = m_OnTitle ? 0 : m_Game.FlashMs(kind);
    if (flash > 0)
    {
        // The view tinted toward the spell's colour, fading out.
        const uint16_t colour = kSpellFlash[kind % kSpells];
        const int amount = flash * 110 / 220;
        for (int i = 0; i < rowCount; ++i)
        {
            uint16_t* out = dst + i * stride;
            for (int x = clipLeft; x < clipRight && x < m_Ray.GetWidth(); ++x)
            {
                out[x] = SmoothText::Mix(out[x], colour, amount);
            }
        }
    }
    if (m_Game.GetLegend().disco && !m_OnTitle)
    {
        // Every few rows a colour of its own, the bands sliding down.
        constexpr uint16_t kDisco[6] = { SmoothText::Rgb(255, 60, 120), SmoothText::Rgb(255, 200, 40),
                                         SmoothText::Rgb(60, 230, 120), SmoothText::Rgb(40, 200, 255),
                                         SmoothText::Rgb(150, 90, 255), SmoothText::Rgb(255, 110, 40) };
        const int shift = static_cast<int>(m_TimeMs / 60);
        for (int i = 0; i < rowCount; ++i)
        {
            const uint16_t colour = kDisco[((firstRow + i + shift) / 24) % 6];
            uint16_t* out = dst + i * stride;
            for (int x = clipLeft; x < clipRight && x < m_Ray.GetWidth(); ++x)
            {
                out[x] = SmoothText::Mix(out[x], colour, 56);
            }
        }
    }
    if (m_OnTitle)
    {
        DrawTitle(dst, stride, firstRow, rowCount, clipLeft, clipRight);
    }
    else
    {
        DrawTarget(dst, stride, firstRow, rowCount, clipLeft, clipRight);
        m_Effects.Draw(dst, stride, firstRow, rowCount, clipLeft, clipRight);
        if (ToastShown())
        {
            DrawToast(dst, stride, firstRow, rowCount, clipLeft, clipRight);
        }
        if (m_Game.IsDead())
        {
            DrawDeath(dst, stride, firstRow, rowCount, clipLeft, clipRight);
        }
        else if (m_FadeInMs > 0)
        {
            // Out of the dark: black, easing back to the view.
            Transitions::Darken(dst, stride, rowCount, clipLeft, clipRight, m_Ray.GetWidth(),
                                m_FadeInMs * 255 / m_FadeInLength);
        }
        if (m_Game.GetMenu() == Menu::Talk)
        {
            const char* speech = m_Game.GetSpeech();
            const int typed = MessageCard::Typed(speech, m_TalkMs);
            MessageCard::Draw(dst, stride, firstRow, rowCount, clipLeft, clipRight, m_Ray.GetWidth(), m_Ray.GetHeight(),
                              speech, typed, typed >= MessageCard::Letters(speech));
        }
        if (m_EchoMs > 0)
        {
            const int echo = m_Game.GetEchoShown();
            Transitions::DrawEcho(dst, stride, firstRow, rowCount, clipLeft, clipRight, m_Ray.GetWidth(),
                                  m_Ray.GetHeight(), Lore::GetEcho(echo), echo >= Lore::kEchoes, m_EchoMs);
        }
        if (m_PlaceMs > 0)
        {
            char floor[8] = "";
            const char* name = "Last Hearth";
            if (m_Game.IsInSeam())
            {
                name = "The Seam";
                std::snprintf(floor, sizeof(floor), "B0");
            }
            else if (!m_Game.IsInVillage())
            {
                name = CrawlerGame::ZoneName(m_Game.GetZone());
                std::snprintf(floor, sizeof(floor), "B%d", m_Game.GetDepth());
            }
            Transitions::DrawPlace(dst, stride, firstRow, rowCount, clipLeft, clipRight, m_Ray.GetWidth(),
                                   m_Ray.GetHeight(), name, floor, m_PlaceMs);
        }
    }
}

void Crawler::DrawDeath(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight) const
{
    Transitions::DrawFall(dst, stride, firstRow, rowCount, clipLeft, clipRight, m_Ray.GetWidth(), m_Ray.GetHeight(),
                          m_Game, m_DeadMs);
}

int32_t Crawler::PlaceKey() const
{
    return m_Game.IsInVillage() ? -1 : m_Game.IsInSeam() ? -2 : m_Game.GetDepth();
}

DUNGEON_HOT void Crawler::DrawTarget(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft,
                                     int clipRight) const
{
    if (m_Panel.IsShown() || m_EchoMs > 0 || CardUp() || m_Game.IsDead())
    {
        return;
    }
    const auto& small = SmoothFont::kSmall;
    const int width = m_Ray.GetWidth();
    const auto text = [&](int x, int y, const char* t, uint16_t colour, bool bold)
    { SmoothText::Draw(dst, stride, firstRow, rowCount, clipLeft, clipRight, x, y, t, small, colour, bold); };
    const auto card = [&](int x0, int y0, int x1, int y1)
    {
        SmoothText::Card(dst, stride, firstRow, rowCount, clipLeft, clipRight, x0, y0, x1, y1, 6, Ui::kCardTint,
                         Ui::kCardBorder);
    };

    // A beer's courage, on the left while it lasts.
    if (m_Game.GetTipsy() > 0)
    {
        const char* tipsy = "TIPSY";
        const int w = SmoothText::Width(tipsy, small);
        card(8, 70, 8 + w + 16, 92);
        text(16, 72, tipsy, Ui::kAccent, true);
    }

    const Target& t = m_Target;
    if (!t.name)
    {
        return;
    }
    // Its name, the first letter up.
    char name[32];
    std::snprintf(name, sizeof(name), "%s", t.name);
    if (name[0] >= 'a' && name[0] <= 'z')
    {
        name[0] = static_cast<char>(name[0] - 'a' + 'A');
    }
    const int nameWidth = SmoothText::Width(name, small);
    constexpr int kTop = 6;
    if (t.hp >= 0)
    {
        // A foe: its name over its health, at the top of the view.
        constexpr int kBar = 140;
        const int cardWidth = (nameWidth > kBar ? nameWidth : kBar) + 28;
        const int x0 = (width - cardWidth) / 2;
        card(x0, kTop, x0 + cardWidth, kTop + 38);
        text((width - nameWidth) / 2, kTop + 4, name, Ui::kText, true);
        const int barX = (width - kBar) / 2;
        const int filled = t.maxHp > 0 ? kBar * (t.hp > 0 ? t.hp : 0) / t.maxHp : 0;
        SmoothText::Fill(dst, stride, firstRow, rowCount, clipLeft, clipRight, barX, kTop + 25, barX + kBar, kTop + 31,
                         2, Ui::kTrack);
        if (filled > 0)
        {
            SmoothText::Fill(dst, stride, firstRow, rowCount, clipLeft, clipRight, barX, kTop + 25, barX + filled,
                             kTop + 31, 2, Ui::kHp);
        }
        return;
    }
    if (t.needGold >= 0)
    {
        // Something to raise: what it gives, then what it costs beside what
        // the village has, each green once there is enough, and the key when
        // both are.
        char gold[24];
        char material[24];
        std::snprintf(gold, sizeof(gold), "%d / %d", t.haveGold, t.needGold);
        std::snprintf(material, sizeof(material), "%d / %d %s", t.haveMaterial, t.needMaterial,
                      Village::MaterialName(t.material));
        const int goldWidth = 20 + SmoothText::Width(gold, small);
        const int materialWidth = 20 + SmoothText::Width(material, small);
        const int costWidth = goldWidth + 18 + materialWidth;
        const int gainWidth = t.gain ? SmoothText::Width(t.gain, small) : 0;
        const bool enough = t.haveGold >= t.needGold && t.haveMaterial >= t.needMaterial;
        const char* last = t.verb ? t.verb : (enough ? "Step up to it." : "Not enough yet.");
        const int lastWidth = (t.verb ? 20 : 0) + SmoothText::Width(last, small);
        int cardWidth = nameWidth > costWidth ? nameWidth : costWidth;
        cardWidth = gainWidth > cardWidth ? gainWidth : cardWidth;
        cardWidth = lastWidth > cardWidth ? lastWidth : cardWidth;
        cardWidth = cardWidth + 28 < width - 16 ? cardWidth + 28 : width - 16;
        const int x0 = (width - cardWidth) / 2;
        card(x0, kTop, x0 + cardWidth, kTop + 8 + 4 * 18 + 4);
        int y = kTop + 4;
        text((width - nameWidth) / 2, y, name, Ui::kText, true);
        y += 18;
        if (t.gain)
        {
            text((width - gainWidth) / 2, y, t.gain, Ui::kAccent, false);
        }
        y += 20;
        constexpr uint16_t kEnough = SmoothText::Rgb(120, 220, 120);
        constexpr uint16_t kShort = SmoothText::Rgb(240, 110, 100);
        int x = (width - costWidth) / 2;
        DrawIcon(dst, stride, firstRow, rowCount, clipLeft, clipRight, x, y, *kCrawlerTextures.items[1]);
        text(x + 20, y, gold, t.haveGold >= t.needGold ? kEnough : kShort, true);
        x += goldWidth + 18;
        DrawIcon(dst, stride, firstRow, rowCount, clipLeft, clipRight, x, y,
                 *kCrawlerTextures.items[static_cast<int>(ItemKind::Moss) + t.material]);
        text(x + 20, y, material, t.haveMaterial >= t.needMaterial ? kEnough : kShort, true);
        y += 20;
        x = (width - lastWidth) / 2;
        if (t.verb)
        {
            SmoothText::Fill(dst, stride, firstRow, rowCount, clipLeft, clipRight, x, y + 1, x + 14, y + 16, 3,
                             Ui::kAccent);
            text(x + 4, y, "E", SmoothText::Rgb(20, 24, 34), true);
            x += 20;
        }
        text(x, y, last, t.verb ? Ui::kAccent : Ui::kDim, t.verb != nullptr);
        return;
    }
    // A thing: its name, the key and what it does, and a line about it.
    const int verbWidth = t.verb ? 20 + SmoothText::Width(t.verb, small) : 0;
    const int detailWidth = t.detail ? SmoothText::Width(t.detail, small) : 0;
    int cardWidth = nameWidth;
    cardWidth = verbWidth > cardWidth ? verbWidth : cardWidth;
    cardWidth = detailWidth > cardWidth ? detailWidth : cardWidth;
    cardWidth = cardWidth + 24 < width - 16 ? cardWidth + 24 : width - 16;
    const int rows = 1 + (t.verb ? 1 : 0) + (t.detail ? 1 : 0);
    const int x0 = (width - cardWidth) / 2;
    card(x0, kTop, x0 + cardWidth, kTop + 8 + rows * 18);
    int y = kTop + 4;
    text((width - nameWidth) / 2, y, name, Ui::kText, true);
    y += 18;
    if (t.verb)
    {
        // The key in an amber chip, then the verb.
        const int x = (width - verbWidth) / 2;
        SmoothText::Fill(dst, stride, firstRow, rowCount, clipLeft, clipRight, x, y + 1, x + 14, y + 16, 3,
                         Ui::kAccent);
        text(x + 4, y, "E", SmoothText::Rgb(20, 24, 34), true);
        text(x + 20, y, t.verb, Ui::kAccent, true);
        y += 18;
    }
    if (t.detail)
    {
        text((width - detailWidth) / 2, y, t.detail, Ui::kDim, false);
    }
}

void Crawler::DrawTitle(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight) const
{
    using PixelText::Style;
    const int width = m_Ray.GetWidth();
    Style logo;
    logo.scale = 5;
    logo.gradient = kLogoGradient;
    logo.outlined = true;
    logo.outline = 0x2800;
    logo.shadowed = true;
    logo.shadow = 0x0000;
    // Only its name, in the middle of the view.
    for (const auto& [word, y] : { std::pair<const char*, int>{ "POCKET", 72 }, { "DUNGEON", 122 } })
    {
        PixelText::Draw(dst, stride, firstRow, rowCount, clipLeft, clipRight,
                        (width - PixelText::Width(word, logo.scale)) / 2, y, word, logo);
    }
}

void Crawler::DrawToast(uint16_t* dst, int stride, int firstRow, int rowCount, int clipLeft, int clipRight) const
{
    MessageCard::Draw(dst, stride, firstRow, rowCount, clipLeft, clipRight, m_Ray.GetWidth(), m_Ray.GetHeight(),
                      m_ToastText, ToastTyped());
}

void Crawler::DrawHud(uint16_t* dst, int strideBytes, int firstRow, int rowCount, int clipLeft, int clipRight) const
{
    const int stride = strideBytes / 2;
    const int width = m_Ray.GetWidth();
    const int height = kScreenHeight - m_Ray.GetHeight();
    // Each row is drawn whole into `line`, then the part inside the clip is
    // copied out; the words and icons go on top after.
    uint16_t line[Raycaster::kMaxWidth];
    const DungeonMap& map = m_Game.GetMap();
    const int mapWidth = kMapWidth * kMapCell;
    const int mapLeft = width - kMapRight - mapWidth;
    const int mapBottom = kMapTop + kMapHeight * kMapCell;
    const int hp = m_Game.GetHp();
    const int maxHp = m_Game.GetMaxHp() > 0 ? m_Game.GetMaxHp() : 1;
    const int barWidth = 138;
    const int filled = barWidth * hp / maxHp;
    const int trail = m_Effects.GetHpTrail();
    const int trailFilled = trail > hp * 16 ? barWidth * trail / (16 * maxHp) : filled;
    const uint16_t hpColour = SmoothText::Mix(Ui::kHpLow, Ui::kHp, 256 * hp / maxHp);
    const int mp = m_Game.GetMp();
    const int maxMp = m_Game.GetMaxMp() > 0 ? m_Game.GetMaxMp() : 1;
    const int mpFilled = barWidth * mp / maxMp;
    const int xpWidth = 96;
    const int xpFilled = xpWidth * m_Game.GetXp() / (m_Game.GetXpNeeded() > 0 ? m_Game.GetXpNeeded() : 1);
    const Cell hero = m_Game.GetHeroCell();
    constexpr int kDx[4] = { 1, 0, -1, 0 };
    constexpr int kDy[4] = { 0, 1, 0, -1 };
    const Cell ahead{ hero.x + kDx[m_Game.GetFacing()], hero.y + kDy[m_Game.GetFacing()] };
    // Where the bars sit, in HUD rows and columns.
    constexpr int kLeft = 12;
    constexpr int kHpBarY = 22;
    constexpr int kMpBarY = 29;
    constexpr int kXpBarY = 66;
    constexpr int kXpBarX = 52;

    // A bar of height h with rounded ends: is (x, row) inside it?
    const auto inBar = [](int x, int row, int x0, int y0, int w, int h)
    {
        if (row < y0 || row >= y0 + h || x < x0 || x >= x0 + w)
        {
            return false;
        }
        const bool endRow = row == y0 || row == y0 + h - 1;
        return !(endRow && (x == x0 || x == x0 + w - 1));
    };

    for (int i = 0; i < rowCount; ++i)
    {
        const int row = firstRow + i;
        uint16_t* out = line;
        // The panel: dark, a little lighter toward the bottom, a hairline on top.
        const uint16_t panel =
            row == 0 ? Ui::kDivider : SmoothText::Mix(Ui::kPanelTop, Ui::kPanelBottom, row * 256 / height);
        for (int x = 0; x < width; ++x)
        {
            out[x] = panel;
        }
        // HP: a slim bar with rounded ends, lit along its top.
        for (int x = kLeft; x < kLeft + barWidth; ++x)
        {
            if (inBar(x, row, kLeft, kHpBarY, barWidth, 5))
            {
                out[x] = x - kLeft < filled        ? (row == kHpBarY ? Ui::kHpLight : hpColour)
                         : x - kLeft < trailFilled ? ViewEffects::kTrailColour
                                                   : Ui::kTrack;
            }
        }
        // MP: thinner, blue, under it.
        for (int x = kLeft; x < kLeft + barWidth; ++x)
        {
            if (inBar(x, row, kLeft, kMpBarY, barWidth, 3))
            {
                out[x] = x - kLeft < mpFilled ? (row == kMpBarY ? kMpLight : kMp) : Ui::kTrack;
            }
        }
        // Experience: thinner, in the accent (not at home).
        for (int x = kXpBarX; x < kXpBarX + xpWidth && !m_Game.IsInVillage(); ++x)
        {
            if (inBar(x, row, kXpBarX, kXpBarY, xpWidth, 3))
            {
                out[x] = x - kXpBarX < xpFilled ? Ui::kAccent : Ui::kTrack;
            }
        }
        // The minimap on its own darker card.
        if (row >= kMapTop - 3 && row < mapBottom + 3)
        {
            for (int x = mapLeft - 3; x < mapLeft + mapWidth + 3; ++x)
            {
                const bool corner =
                    (row == kMapTop - 3 || row == mapBottom + 2) && (x == mapLeft - 3 || x == mapLeft + mapWidth + 2);
                if (!corner)
                {
                    out[x] = Ui::kMapPanel;
                }
            }
        }
        const int mapRow = (row - kMapTop) / kMapCell;
        if (row >= kMapTop && mapRow < kMapHeight)
        {
            for (int cx = 0; cx < kMapWidth; ++cx)
            {
                const Cell c{ cx, mapRow };
                if (!map.IsSeen(c))
                {
                    continue;
                }
                uint16_t colour = kMapFloor;
                const Tile t = map.At(c);
                if (t == Tile::Wall || t == Tile::Void || t == Tile::LooseBrick)
                {
                    colour = kMapWall;
                }
                else if (t == Tile::SecretWall)
                {
                    colour = m_Game.KnowsSecrets() ? kMapSecret : kMapWall;
                }
                else if (t == Tile::Door)
                {
                    colour = kMapDoor;
                }
                else if (t == Tile::EchoWall)
                {
                    colour = kMapEcho;
                }
                else if (t == Tile::Stairs)
                {
                    colour = kMapStairs;
                }
                else if (m_Game.HasKnownTrap(c))
                {
                    colour = kMapTrap;
                }
                if (map.LookAt(c) == FloorLook::Rune)
                {
                    colour = kMapSecret;  // the impossible door
                }
                const int feature = m_Game.FeatureAt(c);
                if (feature >= 0)
                {
                    const FeatureKind kind = m_Game.GetFeature(feature).kind;
                    colour = kind == FeatureKind::Merchant ? kMapMerchant
                             : kind == FeatureKind::Shrine ? kMapShrine
                                                           : kMapFountain;
                }
                if (map.IsVisible(c) && m_Game.MonsterAt(c) >= 0)
                {
                    colour = kMapMonster;
                }
                if (c == ahead && colour == kMapFloor)
                {
                    colour = kMapFacing;
                }
                if (c == hero)
                {
                    colour = kMapHero;
                }
                uint16_t* cell = out + mapLeft + cx * kMapCell;
                for (int k = 0; k < kMapCell; ++k)
                {
                    cell[k] = colour;
                }
            }
        }
        uint16_t* target = dst + i * stride;
        for (int x = clipLeft; x < clipRight && x < width; ++x)
        {
            target[x] = line[x];
        }
    }

    // Words and icons on top.
    using SmoothFont::kLarge;
    using SmoothFont::kSmall;
    const auto text = [&](int x, int y, const char* t, const SmoothFont::Face& face, uint16_t colour, bool bold = false)
    { SmoothText::Draw(dst, stride, firstRow, rowCount, clipLeft, clipRight, x, y, t, face, colour, bold); };
    char buffer[24];

    // HP: label on the left, the numbers on the right, above the bar.
    // HP on the left, MP on the right, each label dim and its numbers bold.
    text(kLeft, 4, "HP", kSmall, Ui::kDim);
    std::snprintf(buffer, sizeof(buffer), "%d/%d", hp, m_Game.GetMaxHp());
    text(kLeft + 21, 4, buffer, kSmall, Ui::kText, true);
    std::snprintf(buffer, sizeof(buffer), "%d/%d", mp, m_Game.GetMaxMp());
    const int mpWidth = SmoothText::Width(buffer, kSmall);
    text(kLeft + barWidth - mpWidth, 4, buffer, kSmall, kMpLight, true);
    text(kLeft + barWidth - mpWidth - 21, 4, "MP", kSmall, Ui::kDim);

    // Attack, defence and gold: an icon and a number each.
    struct Stat
    {
        const Texture* icon;
        int value;
        int x;
    };
    const Stat stats[3] = {
        { kCrawlerTextures.items[static_cast<int>(ItemKind::Blade) + static_cast<int>(m_Game.GetWeapon())],
          m_Game.GetAttack(), kLeft - 2 },
        { &kShieldTexture, m_Game.GetDefense(), kLeft + 46 },
        { &kGoldTexture,
          m_Effects.GetGoldShown() >= 0 ? m_Effects.GetGoldShown()
                                        : (m_Game.IsInVillage() ? m_Game.GetLegend().bank : m_Game.GetGold()),
          kLeft + 94 }
    };
    for (const Stat& stat : stats)
    {
        DrawIcon(dst, stride, firstRow, rowCount, clipLeft, clipRight, stat.x, 38, *stat.icon);
        std::snprintf(buffer, sizeof(buffer), "%d", stat.value);
        const bool rolling = stat.icon == &kGoldTexture && m_Effects.IsGoldRolling();
        text(stat.x + 18, 38, buffer, kSmall,
             rolling ? ViewEffects::kCoinLight : (stat.icon == &kGoldTexture ? Ui::kAccent : Ui::kText), true);
    }

    if (m_Game.IsInVillage())
    {
        // At home: the materials in the store, for building.
        for (int m = 0; m < Village::kMaterials; ++m)
        {
            const int x = kLeft - 2 + m * 36;
            DrawIcon(dst, stride, firstRow, rowCount, clipLeft, clipRight, x, 58,
                     *kCrawlerTextures.items[static_cast<int>(ItemKind::Moss) + m]);
            std::snprintf(buffer, sizeof(buffer), "%d", m_Game.GetLegend().materials[m]);
            text(x + 18, 58, buffer, kSmall, Ui::kText, true);
        }
    }
    else
    {
        // Level and its bar.
        std::snprintf(buffer, sizeof(buffer), "LV %d", m_Game.GetLevel());
        text(kLeft, 59, buffer, kSmall,
             m_Effects.GetLevelMs() > 0 && (m_Effects.GetLevelMs() / 120) % 2 == 0 ? ViewEffects::kLevelColour
                                                                                   : Ui::kText,
             true);
    }

    // The depth, big, and the zone under it.
    constexpr int kDepthX = 164;
    std::snprintf(buffer, sizeof(buffer),
                  m_Game.IsInVillage() ? "HOME"
                  : m_Game.IsInSeam()  ? "B0"
                                       : "B%d",
                  m_Game.GetDepth());
    text(kDepthX, 3, buffer, kLarge, m_Game.IsInSeam() ? Ui::kHp : Ui::kAccent, true);
    // The heart shards carried: four slots, once the first is found.
    if (m_Game.GetShards() > 0)
    {
        for (int i = 0; i < Lore::kShards; ++i)
        {
            const int x = kDepthX + 40 + i * 9;
            const bool have = i < m_Game.GetShards();
            SmoothText::Fill(dst, stride, firstRow, rowCount, clipLeft, clipRight, x, 12, x + 7, 21, 2,
                             have ? Ui::kHp : Ui::kTrack);
            if (have)
            {
                SmoothText::Fill(dst, stride, firstRow, rowCount, clipLeft, clipRight, x + 1, 13, x + 3, 15, 1,
                                 SmoothText::Rgb(255, 200, 210));
            }
        }
    }
    // Zone names in two short lines ("The Sunken Reach": SUNKEN / REACH).
    constexpr const char* kZoneLines[kZones][2] = {
        { "WEEPING", "HALLS" }, { "THE", "OSSUARY" }, { "SUNKEN", "REACH" }, { "CINDER", "FOUNDRY" }
    };
    const int zone = m_Game.GetZone();
    const bool named = m_Game.IsInSeam() || m_Game.IsInVillage();
    // Inside a building, its name.
    constexpr const char* kInsideLines[Village::kBuildings] = { "SMITHY", "APOTHECARY", "TAVERN",
                                                                "TOWER",  "BANK",       "LIFT" };
    const int inside = m_Game.GetInside();
    text(kDepthX, 34,
         m_Game.IsInVillage() && inside < 0 ? "LAST"
         : named                            ? "THE"
                                            : kZoneLines[zone][0],
         kSmall, Ui::kDim);
    text(kDepthX, 50,
         inside >= 0            ? kInsideLines[inside]
         : m_Game.IsInVillage() ? "HEARTH"
         : m_Game.IsInSeam()    ? "SEAM"
                                : kZoneLines[zone][1],
         kSmall, Ui::kDim);
}

void Crawler::LoadBest()
{
    const std::string path = Deki::Storage::BuildPath(Deki::Storage::External, kBestPath);
    Deki::IFileSystem* fs = Deki::FileSystem::GetFileSystemForPath(path.c_str());
    if (!fs || !fs->FileExists(path.c_str()))
    {
        return;
    }
    Deki::IFileSystem::FileHandle file = fs->OpenFile(path.c_str(), Deki::IFileSystem::OpenMode::ReadBinary);
    if (!file)
    {
        return;
    }
    char text[224] = {};
    fs->ReadFile(file, text, sizeof(text) - 1);
    fs->CloseFile(file);
    int best = 0;
    Lore::ReadSave(text, best, m_PagesKnown, m_Legend);
    // Past the game's numbers: the keys, when all of them are there.
    const char* at = text;
    uint32_t keys[kBindings] = {};
    int read = 0;
    for (int i = 0; i < Lore::kSaveValues + kBindings + 2; ++i)
    {
        char* end = nullptr;
        const long value = std::strtol(at, &end, 10);
        if (end == at)
        {
            break;
        }
        at = end;
        if (i == Lore::kSaveValues + kBindings + 1)
        {
            // Last of all: the contracts. Saves from before them end sooner.
            m_Legend.contracts = static_cast<uint16_t>(value);
        }
        else if (i == Lore::kSaveValues + kBindings)
        {
            // After the keys: how loud. (10 was the first builds' start,
            // far too quiet under the wheel, and is no step any more.)
            if (value != 10)
            {
                m_Game.SetSoundVolume(static_cast<int>(value));
            }
        }
        else if (i >= Lore::kSaveValues && value > 0)
        {
            keys[read++] = static_cast<uint32_t>(value);
        }
    }
    for (int i = 0; read == kBindings && i < kBindings; ++i)
    {
        m_Keys[i] = keys[i];
    }
    if (best > 0)
    {
        m_Deepest = best;
    }
}

void Crawler::SaveBest()
{
    const std::string path = Deki::Storage::BuildPath(Deki::Storage::External, kBestPath);
    Deki::IFileSystem* fs = Deki::FileSystem::GetFileSystemForPath(path.c_str());
    if (!fs)
    {
        return;
    }
    Deki::IFileSystem::FileHandle file = fs->OpenFile(path.c_str(), Deki::IFileSystem::OpenMode::WriteBinary);
    if (!file)
    {
        return;
    }
    char text[224];
    int length = Lore::WriteSave(text, sizeof(text), m_Deepest, m_PagesKnown, m_Legend);
    for (int i = 0; i < kBindings && length > 0 && length < static_cast<int>(sizeof(text)) - 8; ++i)
    {
        length += std::snprintf(text + length, sizeof(text) - static_cast<size_t>(length), " %u",
                                static_cast<unsigned>(m_Keys[i]));
    }
    if (length > 0 && length < static_cast<int>(sizeof(text)) - 8)
    {
        length +=
            std::snprintf(text + length, sizeof(text) - static_cast<size_t>(length), " %d", m_Game.GetSoundVolume());
    }
    if (length > 0 && length < static_cast<int>(sizeof(text)) - 8)
    {
        length += std::snprintf(text + length, sizeof(text) - static_cast<size_t>(length), " %u",
                                static_cast<unsigned>(m_Legend.contracts));
    }
    fs->WriteFile(file, text, static_cast<size_t>(length));
    fs->CloseFile(file);
}

#if defined(DEKI_PICO)
// When the program cannot go on, the C++ library would print why, and for
// that it carries a name demangler of some 25 KB. Nobody reads it on the
// board: stop, and leave that out of the flash.
namespace __gnu_cxx
{
void __verbose_terminate_handler()
{
    std::abort();
}
}  // namespace __gnu_cxx
#endif
