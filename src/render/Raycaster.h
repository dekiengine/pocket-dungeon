#pragma once

// A first-person view of a DungeonMap, drawn the way the early 3D games did:
// one ray per screen column finds the nearest wall, whose height on screen is
// the inverse of its distance. Fixed point throughout the per-column and
// per-pixel work: the RP2040 has no floating-point unit.
//
// Prepare() does the rays once a frame. RenderRows() then fills any band of
// rows from that: Deki draws the frame strip by strip, and each strip asks for
// its own rows. Plain code with no engine in it, so it is tested on its own.

#include "../world/DungeonMap.h"
#include "../Hot.h"

#include <cstddef>
#include <cstdint>

namespace Dungeon
{

/// 16.16 fixed point.
using Fix = int32_t;
constexpr int kFixShift = 16;
constexpr Fix kFixOne = 1 << kFixShift;

/// Angles go round in 1024 steps.
constexpr int kAngleSteps = 1024;

/// A texture in flash: a size x size picture of colour indices, and `levels`
/// palettes of `colours` RGB565 colours each, each darker than the last
/// (distance fog), level 0 the brightest. Colours equal to kTransparent are
/// not drawn (a sprite's index 0).
struct Texture
{
    const uint8_t* index = nullptr;
    const uint16_t* palette = nullptr;
    int size = 16;
    int levels = 1;
    int colours = 1;

    const uint16_t* Palette(int level) const { return palette + level * colours; }
};

constexpr uint16_t kTransparent = 0xF81F;

/// Something standing on the floor, drawn facing the viewer.
struct Billboard
{
    Fix x = 0;  // tile units, the centre of a tile is n + 0.5
    Fix y = 0;
    const Texture* texture = nullptr;
    /// 1 = full tile height, 0.5 = half, standing on the floor.
    Fix scale = kFixOne;
    /// Drawn this many shade levels brighter or darker (a hit flash: -8).
    int shadeBias = 0;
    /// Raised off the floor by this much of its height (a hop, a bob).
    Fix lift = 0;
    /// How far its base reaches toward the viewer from its middle, in tiles
    /// (about half its width): where it meets the floor. A flat picture set
    /// down at its middle would hover behind its own shadow. 0 for what does
    /// not touch the floor.
    Fix foot = 0;
    /// Its width against its height: kFixOne square, more for a squash.
    Fix squash = kFixOne;
    /// Breathed out: one of the view's rows shorter, its feet where they
    /// were. The row goes from its middle and every other row is drawn as it
    /// was, so its outline keeps its thickness (stretching it would make rows
    /// of it come and go). 0 or 1.
    uint8_t breath = 0;
    /// Drawn in these colours instead of its own (as many as its texture
    /// has; index 0 kTransparent), unshaded: kFlashPalette for a hit, one
    /// colour for a spark. Null: its own.
    const uint16_t* palette = nullptr;
};

/// A sprite drawn in it is a white silhouette: index 0 transparent, every
/// other colour white. For Billboard::palette, up to 256 colours.
extern const uint16_t kFlashPalette[256];

/// A picture drawn over the view in screen pixels, not in the world: the
/// sword in the hero's hand. Its texture's first level is used.
struct Overlay
{
    const Texture* texture = nullptr;
    int x = 0;  // its top left corner, in view pixels
    int y = 0;
    int scale = 1;  // each texel this many pixels across
};

class Raycaster
{
public:
    static constexpr int kMaxWidth = 320;
    static constexpr int kMaxBillboards = 48;
    static constexpr int kMaxLights = 8;

    Raycaster();

    /// The memory it works in (a view's columns, rows and placed sprites),
    /// aligned for anything and kept alive as long as the raycaster. Kept
    /// outside so whatever holds a raycaster stays small, and no bigger than
    /// the view needs: StorageBytes for a view of `width` by `height` screen
    /// pixels at `pixelSize`. Until it has enough, nothing is cast or drawn.
    static size_t StorageBytes(int width, int height, int pixelSize);
    void SetStorage(void* memory, size_t bytes);

    /// How big the view's own pixels are on screen: 1, or 2 for a view worked
    /// out at half the size each way and drawn with every pixel doubled (a
    /// quarter of the work, and pixels the size of the sprites'). Everything
    /// else stays in screen pixels: sizes, rows, Project, the overlay. Set
    /// before SetSize.
    void SetPixelSize(int size) { m_Pixel = size >= 2 ? 2 : 1; }
    /// The view's size in screen pixels, at most kMaxWidth across (even
    /// numbers when the pixels are doubled).
    void SetSize(int width, int height);
    int GetWidth() const { return m_Width * m_Pixel; }
    int GetHeight() const { return m_Height * m_Pixel; }

    static constexpr int kMaxWallKinds = 4;
    static constexpr int kMaxFloorKinds = 2;

    /// The walls by tile: the first the commonest, the others less and less
    /// often, each tile always the same one. Up to kMaxWallKinds.
    void SetWalls(const Texture* const* walls, int count);
    /// A plain wall and, one tile in five, another.
    void SetWallTextures(const Texture* wall, const Texture* altWall)
    {
        const Texture* walls[2] = { wall, altWall };
        SetWalls(walls, altWall ? 2 : 1);
    }

    /// A soft round shadow on the floor (under a monster, a barrel, a coin):
    /// its middle, in tile units, and its radius, at most half a tile.
    struct Shadow
    {
        Fix x = 0;
        Fix y = 0;
        Fix radius = kFixOne / 3;
    };
    static constexpr int kMaxShadows = 36;
    /// The shadows to draw this frame (call before Prepare). Up to
    /// kMaxShadows; a tile shows one of them, the last that touches it.
    void SetShadows(const Shadow* shadows, int count)
    {
        m_ShadowInput = shadows;
        m_ShadowInputCount = count < kMaxShadows ? count : kMaxShadows;
    }

    /// A sky above the walls where there is no ceiling: the texture wrapped
    /// round the viewer a few times, turning with the view. Null: none.
    void SetSky(const Texture* sky) { m_Sky = sky; }

    /// Walls with a picture of their own, tile by tile: `cells` holds a byte
    /// per map tile, row by row, 0 for none or 1 + an index into `textures`.
    /// Null: none.
    void SetCellWalls(const uint8_t* cells, const Texture* const* textures, int count)
    {
        m_CellWalls = cells;
        m_CellWallTextures = textures;
        m_CellWallCount = count;
    }
    /// A second storey on every wall (houses under the sky): a picture as
    /// wide as the wall standing on top of it, as tall again, its index 0
    /// see-through (the sky shows round a roof). Each tile takes one of
    /// `tops`, always the same one; a tile with a wall of its own takes
    /// `cellTops`' entry for it (beside SetCellWalls' textures) when that is
    /// not null. Count 0: walls end at their top.
    void SetWallTops(const Texture* const* tops, int count, const Texture* const* cellTops)
    {
        m_WallTops = tops;
        m_WallTopCount = count;
        m_CellWallTops = cellTops;
    }
    /// The walls a hideout is behind: the cracked one and the locked door.
    /// Null: drawn as plain walls.
    void SetHideoutWalls(const Texture* secretWall, const Texture* door, const Texture* echoWall = nullptr)
    {
        m_SecretWall = secretWall;
        m_Door = door;
        m_EchoWall = echoWall;
    }
    /// Ceiling and floor colours at the horizon (dark) and at the viewer's
    /// feet (bright), RGB565: drawn where there is no floor or ceiling
    /// texture.
    void SetFloorColours(uint16_t ceilingFar, uint16_t ceilingNear, uint16_t floorFar, uint16_t floorNear);
    /// Textures for the floor and the ceiling, drawn in perspective. Sizes
    /// must be powers of two. Null: the flat colours.
    void SetFloorTextures(const Texture* floor, const Texture* ceiling) { SetFloors(&floor, floor ? 1 : 0, ceiling); }
    /// Floors (the first the commonest, up to kMaxFloorKinds) and the ceiling.
    /// Each tile's floor and ceiling are turned and mirrored their own way, so
    /// the grid of repeats does not show. All the same size, with the same
    /// number of colours.
    void SetFloors(const Texture* const* floors, int count, const Texture* ceiling);
    /// The floor's other looks (FloorLook: the stairs hole, spikes), drawn on
    /// the tiles the map gives that look. Each must be the floor texture's
    /// size and levels, or it is drawn as plain floor.
    void SetFloorLook(FloorLook look, const Texture* texture) { m_Looks[static_cast<int>(look)] = texture; }
    /// How far the viewer's light reaches: things this far away (tile units,
    /// 16.16) are at the darkest shade. Changing it a little every frame makes
    /// the light flicker.
    void SetLightReach(Fix reach) { m_LightReach = reach > kFixOne ? reach : kFixOne; }
    /// Moves the horizon this many rows down from the middle of the view
    /// (the head bob of a step).
    void SetHorizonOffset(int rows) { m_HorizonOffset = rows; }
    /// Lights in the world (tile units, 16.16): walls, floors, ceilings and
    /// things within `radius` of one are drawn brighter, most at its middle,
    /// fading out smoothly to its edge. Up to kMaxLights; count 0: none.
    void SetLights(const Fix* xs, const Fix* ys, int count, Fix radius);

    /// Tiles in shadow (an alley between tall houses): a byte per map tile,
    /// row by row, how many eighths of the way to dark its floor, the walls
    /// round it and what stands on it are drawn darker. Null: none.
    void SetGloom(const uint8_t* cells) { m_Gloom = cells; }
    /// Tiles this deep in shadow or deeper are roofed.
    static constexpr int kCoveredGloom = 6;
    /// The roof over them, where there is no ceiling: a cloth stretched
    /// across at the walls' tops, hiding the sky and the storeys above. Null:
    /// none.
    void SetCanopy(const Texture* cloth) { m_Canopy = cloth; }

    /// The first floor is water: walls show in it, upside down, rippling,
    /// every other pixel (the floor shows through the rest).
    void SetReflectiveFloor(bool reflective) { m_Reflective = reflective; }
    /// The time, for anything that moves by itself (the ripples).
    void SetTime(uint32_t ms) { m_TimeMs = ms; }
    /// A clock in microseconds, for timing RenderRows' stages (null: not
    /// timed). Each stage's time adds up until TakeStageUs.
    void SetClock(uint64_t (*clock)()) { m_Clock = clock; }
    enum Stage
    {
        kWallsStage,
        kFloorsStage,
        kSpritesStage,  // billboards, shadows' things and the overlay
        kStages
    };
    /// The time spent in `stage` since the last call, then zero again.
    uint64_t TakeStageUs(Stage stage)
    {
        const uint64_t us = m_StageUs[stage];
        m_StageUs[stage] = 0;
        return us;
    }

    /// The picture drawn over the view (none: texture null).
    void SetOverlay(const Overlay& overlay) { m_Overlay = overlay; }

    /// Casts the rays and places the billboards for one frame, seen from
    /// (x, y) in tile units looking along `angle` (0 = +x, 256 = +y).
    void Prepare(const DungeonMap& map, Fix x, Fix y, int angle, const Billboard* billboards, int count);

    /// Fills rows [firstRow, firstRow + rowCount) of the view into `dst`, one
    /// RGB565 row every `strideBytes`; dst points at the first of those rows,
    /// column 0 of the view. Only columns [columnFrom, columnTo) are written
    /// (columnTo -1: to the right edge).
    void RenderRows(uint16_t* dst, int strideBytes, int firstRow, int rowCount, int columnFrom = 0,
                    int columnTo = -1) const;

    /// The distance to the wall seen in column `x`, in tile units (16.16).
    Fix DepthAt(int x) const { return m_Columns ? m_Columns[x / m_Pixel].depth : INT32_MAX; }

    /// Where the point (x, y), `height` wall heights above the floor, shows
    /// in the view after Prepare: false when it is behind the viewer, off
    /// the sides or behind a wall. `depth` how far away it is.
    bool Project(Fix x, Fix y, Fix height, int& screenX, int& screenY, Fix& depth) const;

    static Fix Sin(int angle);
    static Fix Cos(int angle) { return Sin(angle + kAngleSteps / 4); }

private:
    struct Column
    {
        int16_t top = 0;     // first wall row (may be above the view)
        int16_t bottom = 0;  // last wall row + 1
        Fix texStart = 0;    // texture v at row `top`, in texels 16.16
        Fix texStep = 0;     // texture v per row
        Fix depth = 0;
        Fix portalDepth = INT32_MAX;          // where the ray went through an impossible door
        uint8_t portal = 0;                   // which one, 1-based; 0 none
        const uint8_t* texColumn = nullptr;   // the texture's column of indices
        const uint16_t* palette = nullptr;    // and the palette of its shade
        const uint16_t* darker = nullptr;     // the next shade, mixed in by `dither`
        const uint16_t* reflected = nullptr;  // a darker one, for its reflection in water
        uint8_t texSize = 16;
        uint8_t dither = 0;  // how far toward `darker`, in sixteenths
        // The storey on top of it (SetWallTops): its column of indices, null
        // for none; its rows end where the wall's begin.
        const uint8_t* upColumn = nullptr;
        const uint16_t* upPalette = nullptr;
        const uint16_t* upDarker = nullptr;
        int16_t upFrom = 0;  // its first row (may be above the view)
        uint8_t upSize = 16;
    };

    struct Placed
    {
        int16_t left = 0;    // first screen column (may be off screen)
        int16_t width = 0;   // columns across
        int16_t top = 0;     // first row
        int16_t height = 0;  // rows down
        Fix depth = 0;
        Fix texStep = 0;                    // texels per screen pixel, down
        Fix texStepX = 0;                   // and across
        const uint8_t* index = nullptr;     // the texture's indices
        const uint16_t* palette = nullptr;  // the palette of its shade
        uint8_t texSize = 16;
        uint8_t through = 0;   // seen through impossible door n (1-based), 0 directly
        int16_t cut = 0x7FFF;  // rows from here down are drawn a row lower in its picture (a breath out)
    };
    static constexpr int kMaxPlaced = kMaxBillboards * (1 + kMaxPortals);

    struct FloorRow
    {
        Fix startX = 0;  // where the row's leftmost ray meets the floor
        Fix startY = 0;
        Fix stepX = 0;  // and how far along it each column moves
        Fix stepY = 0;
        int paletteOffset = 0;  // the shade level's palette, in colours from a texture's first
        uint8_t dither = 0;     // how far toward the next shade, in sixteenths
        uint8_t sizeShift = 5;  // log2 of the texture's size
        bool valid = false;     // false: the flat colour
        bool isFloor = false;   // below the horizon: the floor's looks apply
        bool canopy = false;    // above it with no ceiling: where the roofed tiles' cloth is drawn
    };

    // The view in its own pixels (half the screen's when they are doubled).
    int m_Width = 320;
    int m_Height = 240;
    int m_Pixel = 1;
    // Doubled pixels: the view's rows are drawn into the scratch first, a
    // few at a time.
    static constexpr int kScratchRows = 20;
    void RenderOwnRows(uint16_t* dst, int strideBytes, int firstRow, int rowCount, int columnFrom, int columnTo) const;
    int m_Horizon = 120;
    int m_HorizonOffset = 0;
    Fix m_LightReach = 7 * kFixOne;
    const Texture* m_Floor = nullptr;
    const Texture* m_Floors[kMaxFloorKinds] = {};
    int m_FloorCount = 0;
    const Texture* m_Walls[kMaxWallKinds] = {};
    int m_WallCount = 0;
    // Per tile: bits 0-2 the floor's turn and mirror, bit 3 the second floor,
    // bits 5-7 the ceiling's turn and mirror.
    uint8_t m_CellStyle[kMapWidth * kMapHeight] = {};
    int m_StyledFloors = -1;  // the floor count m_CellStyle was made for
    const Texture* m_Looks[kFloorLooks] = {};
    const Texture* m_SecretWall = nullptr;
    const Texture* m_Door = nullptr;
    const Texture* m_EchoWall = nullptr;
    const Texture* m_Sky = nullptr;
    Fix m_SkyU0 = 0;  // the sky's texel at the view's left edge, 16.16
    Fix m_SkyDu = 0;  // and per column
    const uint8_t* m_CellWalls = nullptr;
    const Texture* const* m_CellWallTextures = nullptr;
    int m_CellWallCount = 0;
    const Texture* const* m_WallTops = nullptr;
    int m_WallTopCount = 0;
    const Texture* const* m_CellWallTops = nullptr;
    const uint8_t* m_MapLooks = nullptr;  // the map's FloorLook per tile, for this frame
    const uint8_t* m_Gloom = nullptr;
    const Texture* m_Canopy = nullptr;
    Fix m_LightX[kMaxLights] = {};
    Fix m_LightY[kMaxLights] = {};
    int m_LightCount = 0;
    Fix m_LightRadius = kFixOne * 2;
    bool m_Reflective = false;
    uint32_t m_TimeMs = 0;
    Overlay m_Overlay;
    const Texture* m_Ceiling = nullptr;
    // What SetStorage gave, and the view's arrays laid out in it (LayOut):
    // null while there is none, or not enough for the view's size.
    void* m_Storage = nullptr;
    size_t m_StorageBytes = 0;
    void LayOut();
    FloorRow* m_FloorRows = nullptr;
    Column* m_Columns = nullptr;
    Fix* m_CameraX = nullptr;  // -1 .. 1 across the view, per column
    Placed* m_Placed = nullptr;
    uint16_t* m_RowColour = nullptr;  // ceiling or floor colour per row (view height <= width)
    uint16_t* m_Scratch = nullptr;
    uint16_t m_FlatColours[4] = { 0x0000, 0x2104, 0x0000, 0x4208 };  // as SetFloorColours got them
    int m_PlacedCount = 0;

    // Each tile's shadow, 1-based into m_ShadowInput; 0 none.
    const Shadow* m_ShadowInput = nullptr;
    int m_ShadowInputCount = 0;
    uint8_t m_ShadowCell[kMapWidth * kMapHeight] = {};
    // Each tile's light (the nearest that reaches it), 1-based; 0 none. Its
    // pool on the floor and the ceiling is worked out pixel by pixel.
    uint8_t m_LightCell[kMapWidth * kMapHeight] = {};
    int32_t m_LightR2 = 1;     // the lights' radius squared, in 1/256 tiles
    int32_t m_LightScale = 0;  // turns what is left of it into brightness
    // The view, kept from Prepare for placing things on screen.
    Fix m_ViewX = 0;
    Fix m_ViewY = 0;
    Fix m_DirX = 0;
    Fix m_DirY = 0;
    Fix m_PlaneX = 0;
    Fix m_PlaneY = 0;
    uint64_t (*m_Clock)() = nullptr;
    mutable uint64_t m_StageUs[kStages] = {};

    void CastColumn(const DungeonMap& map, int x, Fix posX, Fix posY, Fix dirX, Fix dirY, Fix planeX, Fix planeY);
    void PlaceBillboards(const DungeonMap& map, const Billboard* billboards, int count, Fix posX, Fix posY, Fix dirX,
                         Fix dirY, Fix planeX, Fix planeY);
    void PlaceOne(const Billboard& b, Fix bx, Fix by, uint8_t through, Fix posX, Fix posY, Fix dirX, Fix dirY,
                  Fix planeX, Fix planeY, int64_t invDet);
    int ShadeLevel(Fix depth, int levels) const;
    /// The shade at `depth` in sixteenths of a level: the whole levels are
    /// the palettes, the rest is mixed pixel by pixel (kDither), so the light
    /// fades without bands.
    int Shade16(Fix depth, int levels) const;
    /// How many levels brighter the lights make (x, y): 0 to 3.
    /// How much brighter the lights make (x, y), in 128ths of the way from
    /// dark to bright (sixteenths of a level, of eight).
    int LightBoost(Fix x, Fix y) const;
    void PrepareFloorRows(Fix posX, Fix posY, Fix dirX, Fix dirY, Fix planeX, Fix planeY);
    /// The colour of the way down seen through the floor at (fx, fy): steps
    /// going down into the dark, with the hole's sides round them. `level`
    /// is the floor's shade on this row.
    uint16_t PitColour(Fix fx, Fix fy, int level) const;
};

}  // namespace Dungeon
