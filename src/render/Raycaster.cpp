#include "Raycaster.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace Dungeon
{

namespace
{

// Field of view: the camera plane is this long either side of the view
// direction (0.66: about 66 degrees across).
constexpr Fix kPlaneLength = 43254;

// Nearer than this a wall is treated as this near, so the division stays in
// range when the viewer stands against it.
constexpr Fix kMinDepth = kFixOne / 64;

// The most cells a ray crosses before giving up: more than the map is wide.
constexpr int kMaxSteps = 64;

Fix Mul(Fix a, Fix b)
{
    return static_cast<Fix>((static_cast<int64_t>(a) * b) >> kFixShift);
}

// |1 / v| in 16.16, kept in range. (2^32 - 1) / |v| is a 32-bit division
// (the RP2040 does those in hardware) and off by less than one step.
Fix Reciprocal(Fix v)
{
    if (v == 0)
    {
        return INT32_MAX;
    }
    const uint32_t r = 0xFFFFFFFFu / (v < 0 ? 0u - static_cast<uint32_t>(v) : static_cast<uint32_t>(v));
    return r > static_cast<uint32_t>(INT32_MAX) ? INT32_MAX : static_cast<Fix>(r);
}

uint16_t Blend565(uint16_t a, uint16_t b, int t, int span)
{
    // t of span of the way from a to b.
    const int ar = a >> 11, ag = (a >> 5) & 63, ab = a & 31;
    const int br = b >> 11, bg = (b >> 5) & 63, bb = b & 31;
    const int r = ar + (br - ar) * t / span;
    const int g = ag + (bg - ag) * t / span;
    const int bl = ab + (bb - ab) * t / span;
    return static_cast<uint16_t>((r << 11) | (g << 5) | bl);
}

}  // namespace

Fix Raycaster::Sin(int angle)
{
    // Built once from the C library: the only floating point in here, and
    // never in a frame.
    static Fix table[kAngleSteps];
    static bool built = false;
    if (!built)
    {
        for (int i = 0; i < kAngleSteps; ++i)
        {
            table[i] = static_cast<Fix>(std::lround(std::sin(i * 6.283185307179586 / kAngleSteps) * kFixOne));
        }
        built = true;
    }
    return table[angle & (kAngleSteps - 1)];
}

Raycaster::Raycaster()
{
    SetSize(m_Width, m_Height);
}

namespace
{

// Each of the view's arrays starts on a boundary good for anything.
size_t Aligned(size_t bytes)
{
    return (bytes + 7) & ~static_cast<size_t>(7);
}

}  // namespace

size_t Raycaster::StorageBytes(int width, int height, int pixelSize)
{
    const int pixel = pixelSize >= 2 ? 2 : 1;
    const size_t w = static_cast<size_t>((width < kMaxWidth ? width : kMaxWidth) / pixel);
    const size_t h = static_cast<size_t>((height < kMaxWidth ? height : kMaxWidth) / pixel);
    return Aligned(w * sizeof(Column)) + Aligned(h * sizeof(FloorRow)) + Aligned(kMaxPlaced * sizeof(Placed)) +
           Aligned(w * sizeof(Fix)) + Aligned(h * sizeof(uint16_t)) +
           (pixel == 2 ? Aligned(kScratchRows * w * sizeof(uint16_t)) : 0);
}

void Raycaster::SetStorage(void* memory, size_t bytes)
{
    m_Storage = memory;
    m_StorageBytes = memory ? bytes : 0;
    LayOut();
}

void Raycaster::LayOut()
{
    const size_t w = static_cast<size_t>(m_Width);
    const size_t h = static_cast<size_t>(m_Height);
    const bool fits = m_Storage && m_StorageBytes >= StorageBytes(m_Width * m_Pixel, m_Height * m_Pixel, m_Pixel);
    m_PlacedCount = 0;
    if (!fits)
    {
        m_Columns = nullptr;
        m_FloorRows = nullptr;
        m_Placed = nullptr;
        m_CameraX = nullptr;
        m_RowColour = nullptr;
        m_Scratch = nullptr;
        return;
    }
    std::memset(m_Storage, 0, m_StorageBytes);
    uint8_t* at = static_cast<uint8_t*>(m_Storage);
    const auto take = [&at](size_t bytes)
    {
        uint8_t* here = at;
        at += Aligned(bytes);
        return here;
    };
    m_Columns = reinterpret_cast<Column*>(take(w * sizeof(Column)));
    m_FloorRows = reinterpret_cast<FloorRow*>(take(h * sizeof(FloorRow)));
    m_Placed = reinterpret_cast<Placed*>(take(kMaxPlaced * sizeof(Placed)));
    m_CameraX = reinterpret_cast<Fix*>(take(w * sizeof(Fix)));
    m_RowColour = reinterpret_cast<uint16_t*>(take(h * sizeof(uint16_t)));
    m_Scratch = m_Pixel == 2 ? reinterpret_cast<uint16_t*>(take(kScratchRows * w * sizeof(uint16_t))) : nullptr;
    for (int x = 0; x < m_Width; ++x)
    {
        // -1 at the left edge to just under 1 at the right.
        m_CameraX[x] = static_cast<Fix>((static_cast<int64_t>(2 * x - m_Width) << kFixShift) / m_Width);
    }
    SetFloorColours(m_FlatColours[0], m_FlatColours[1], m_FlatColours[2], m_FlatColours[3]);
}

void Raycaster::SetSize(int width, int height)
{
    m_Width = (width < kMaxWidth ? width : kMaxWidth) / m_Pixel;
    m_Height = (height < kMaxWidth ? height : kMaxWidth) / m_Pixel;
    LayOut();
}

namespace
{

// Mixes a tile's place into a number that looks random but is the same every
// frame.
uint32_t TileHash(int x, int y)
{
    uint32_t h = static_cast<uint32_t>(x) * 73856093u ^ static_cast<uint32_t>(y) * 19349663u;
    h ^= h >> 13;
    h *= 0x5BD1E995u;
    h ^= h >> 15;
    return h;
}

}  // namespace

void Raycaster::SetLights(const Fix* xs, const Fix* ys, int count, Fix radius)
{
    m_LightCount = 0;
    for (int i = 0; i < count && m_LightCount < kMaxLights; ++i)
    {
        m_LightX[m_LightCount] = xs[i];
        m_LightY[m_LightCount] = ys[i];
        ++m_LightCount;
    }
    m_LightRadius = radius > kFixOne / 4 ? radius : kFixOne / 4;
}

namespace
{

// The order pixels turn to the next shade as it nears: a 4x4 pattern, by row
// then column. A pixel takes the darker shade when its number is below the
// shade's sixteenths.
constexpr uint8_t kDither[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };

// A light's brightness at its middle and the most it adds, in 128ths of
// the way from dark to bright: a torch lifts what is beside it nearly out of
// the dark.
constexpr int kLightStrength = 150;
constexpr int kLightMost = 104;

}  // namespace

DUNGEON_HOT int Raycaster::LightBoost(Fix x, Fix y) const
{
    // In 1/256 tiles, so the squares fit 32 bits (a floor is 24 tiles across).
    const int32_t r = m_LightRadius >> 8;
    const int32_t r2 = r * r;
    int best = 0;
    for (int i = 0; i < m_LightCount; ++i)
    {
        const int32_t dx = (x - m_LightX[i]) >> 8;
        const int32_t dy = (y - m_LightY[i]) >> 8;
        if (dx > r || dx < -r || dy > r || dy < -r)
        {
            continue;
        }
        const int32_t d2 = dx * dx + dy * dy;
        if (d2 < r2)
        {
            const int boost = static_cast<int>(kLightStrength * (r2 - d2) / r2);
            best = boost > best ? boost : best;
        }
    }
    return best > kLightMost ? kLightMost : best;
}

void Raycaster::SetWalls(const Texture* const* walls, int count)
{
    m_WallCount = 0;
    for (int i = 0; i < count && m_WallCount < kMaxWallKinds; ++i)
    {
        if (walls[i])
        {
            m_Walls[m_WallCount++] = walls[i];
        }
    }
}

void Raycaster::SetFloors(const Texture* const* floors, int count, const Texture* ceiling)
{
    m_FloorCount = 0;
    for (int i = 0; i < count && m_FloorCount < kMaxFloorKinds; ++i)
    {
        if (floors[i])
        {
            m_Floors[m_FloorCount++] = floors[i];
        }
    }
    m_Floor = m_FloorCount > 0 ? m_Floors[0] : nullptr;
    m_Ceiling = ceiling;
}

void Raycaster::SetFloorColours(uint16_t ceilingFar, uint16_t ceilingNear, uint16_t floorFar, uint16_t floorNear)
{
    m_FlatColours[0] = ceilingFar;
    m_FlatColours[1] = ceilingNear;
    m_FlatColours[2] = floorFar;
    m_FlatColours[3] = floorNear;
    const int half = m_Height / 2;
    for (int y = 0; y < m_Height && m_RowColour; ++y)
    {
        // How far from the horizon this row is, 0 at the horizon.
        const int fromHorizon = y < half ? half - y : y - half;
        m_RowColour[y] = y < half ? Blend565(ceilingFar, ceilingNear, fromHorizon, half)
                                  : Blend565(floorFar, floorNear, fromHorizon, half);
    }
}

int Raycaster::ShadeLevel(Fix depth, int levels) const
{
    if (levels <= 1)
    {
        return 0;
    }
    // depth * levels / reach, in 32-bit divisions.
    const Fix step = m_LightReach / levels;
    const int level = step > 0 ? depth / step : 0;
    return level < levels - 1 ? level : levels - 1;
}

int Raycaster::Shade16(Fix depth, int levels) const
{
    if (levels <= 1)
    {
        return 0;
    }
    const Fix step = m_LightReach / (levels * 16);
    int shade = step > 0 ? depth / step : 0;
    // A level with only a pixel in eight of the next reads as specks, not
    // as a shade: those go to the whole level.
    const int part = shade & 15;
    shade = part < 3 ? shade - part : (part > 13 ? shade - part + 16 : shade);
    return shade < (levels - 1) * 16 ? shade : (levels - 1) * 16;
}

void Raycaster::Prepare(const DungeonMap& map, Fix x, Fix y, int angle, const Billboard* billboards, int count)
{
    if (!m_Columns)
    {
        return;  // no storage yet
    }
    const Fix dirX = Cos(angle);
    const Fix dirY = Sin(angle);
    // The camera plane points to the viewer's right: with y growing down the
    // screen's map, that is (-dirY, dirX).
    const Fix planeX = Mul(-dirY, kPlaneLength);
    const Fix planeY = Mul(dirX, kPlaneLength);
    m_Horizon = m_Height / 2 + m_HorizonOffset / m_Pixel;
    m_MapLooks = map.GetLooks();
    // Each tile's floor and ceiling: which, and turned how. The same for a
    // tile every frame, worked out again only when the floors change.
    for (int ty = 0; ty < kMapHeight && m_StyledFloors != m_FloorCount; ++ty)
    {
        for (int tx = 0; tx < kMapWidth; ++tx)
        {
            const uint32_t h = TileHash(tx, ty);
            const bool second = m_FloorCount > 1 && ((h >> 3) % 100) < 30;
            m_CellStyle[ty * kMapWidth + tx] =
                static_cast<uint8_t>((h & 7) | (second ? 8 : 0) | (((h >> 12) & 7) << 5));
        }
    }
    m_StyledFloors = m_FloorCount;
    PrepareFloorRows(x, y, dirX, dirY, planeX, planeY);
    // Which tiles a shadow falls on.
    for (uint8_t& cell : m_ShadowCell)
    {
        cell = 0;
    }
    for (int i = 0; i < m_ShadowInputCount; ++i)
    {
        const Shadow& s = m_ShadowInput[i];
        const int x0 = (s.x - s.radius) >> kFixShift;
        const int x1 = (s.x + s.radius) >> kFixShift;
        const int y0 = (s.y - s.radius) >> kFixShift;
        const int y1 = (s.y + s.radius) >> kFixShift;
        for (int ty = y0 < 0 ? 0 : y0; ty <= y1 && ty < kMapHeight; ++ty)
        {
            for (int tx = x0 < 0 ? 0 : x0; tx <= x1 && tx < kMapWidth; ++tx)
            {
                m_ShadowCell[ty * kMapWidth + tx] = static_cast<uint8_t>(i + 1);
            }
        }
    }
    // Which tiles a light reaches: for each, the light nearest its middle.
    for (uint8_t& cell : m_LightCell)
    {
        cell = 0;
    }
    {
        const int32_t r = m_LightRadius >> 8;
        m_LightR2 = r * r;
        m_LightScale = (kLightStrength << 20) / m_LightR2;
    }
    for (int i = 0; i < m_LightCount; ++i)
    {
        const int x0 = (m_LightX[i] - m_LightRadius) >> kFixShift;
        const int x1 = (m_LightX[i] + m_LightRadius) >> kFixShift;
        const int y0 = (m_LightY[i] - m_LightRadius) >> kFixShift;
        const int y1 = (m_LightY[i] + m_LightRadius) >> kFixShift;
        for (int ty = y0 < 0 ? 0 : y0; ty <= y1 && ty < kMapHeight; ++ty)
        {
            for (int tx = x0 < 0 ? 0 : x0; tx <= x1 && tx < kMapWidth; ++tx)
            {
                uint8_t& cell = m_LightCell[ty * kMapWidth + tx];
                const auto away = [&](int light)
                {
                    const int32_t dx = (((tx << kFixShift) + kFixOne / 2) - m_LightX[light]) >> 8;
                    const int32_t dy = (((ty << kFixShift) + kFixOne / 2) - m_LightY[light]) >> 8;
                    return dx * dx + dy * dy;
                };
                if (cell == 0 || away(i) < away(cell - 1))
                {
                    cell = static_cast<uint8_t>(i + 1);
                }
            }
        }
    }
    if (m_Sky && m_Sky->index)
    {
        // The sky goes round six times; the view sees about 66 degrees of
        // the 360 (0.1855 of the way round).
        constexpr int kSkyRepeats = 6;
        const int64_t round = static_cast<int64_t>(m_Sky->size) * kSkyRepeats << kFixShift;
        m_SkyDu = static_cast<Fix>(round * 1855 / 10000 / m_Width);
        // And drifts by itself, a texel every half second.
        const int64_t drift = (static_cast<int64_t>(m_TimeMs) * (kFixOne / 500)) % round;
        m_SkyU0 = static_cast<Fix>(round * (angle & (kAngleSteps - 1)) / kAngleSteps + drift -
                                   static_cast<int64_t>(m_SkyDu) * m_Width / 2);
    }
    for (int column = 0; column < m_Width; ++column)
    {
        CastColumn(map, column, x, y, dirX, dirY, planeX, planeY);
    }
    // A wall seen square on is one shade all across: mixing two there would
    // only show the pattern, so it takes the nearer of the two whole.
    constexpr int kFlatRun = 8;
    for (int from = 0; from < m_Width;)
    {
        int to = from + 1;
        while (to < m_Width && m_Columns[to].palette == m_Columns[from].palette &&
               m_Columns[to].dither == m_Columns[from].dither)
        {
            ++to;
        }
        if (to - from >= kFlatRun && m_Columns[from].texColumn && m_Columns[from].dither != 0)
        {
            for (int c = from; c < to; ++c)
            {
                Column& column = m_Columns[c];
                if (column.dither >= 8)
                {
                    column.palette = column.darker;
                    column.upPalette = column.upDarker;
                }
                column.dither = 0;
            }
        }
        from = to;
    }
    PlaceBillboards(map, billboards, count, x, y, dirX, dirY, planeX, planeY);
    m_ViewX = x;
    m_ViewY = y;
    m_DirX = dirX;
    m_DirY = dirY;
    m_PlaneX = planeX;
    m_PlaneY = planeY;
}

namespace
{

int Log2(int size)
{
    int shift = 0;
    while ((1 << shift) < size)
    {
        ++shift;
    }
    return shift;
}

}  // namespace

DUNGEON_HOT void Raycaster::PrepareFloorRows(Fix posX, Fix posY, Fix dirX, Fix dirY, Fix planeX, Fix planeY)
{
    // The rays at the left and right edges of the view.
    const Fix leftX = dirX - planeX;
    const Fix leftY = dirY - planeY;
    const Fix spanX = 2 * planeX;
    const Fix spanY = 2 * planeY;
    for (int row = 0; row < m_Height; ++row)
    {
        FloorRow& r = m_FloorRows[row];
        const bool below = row > m_Horizon;
        // Under the sky, the rows above the horizon are still worked out
        // when some tiles are roofed: for the cloth over those.
        const bool roofed = !below && !m_Ceiling && m_Canopy;
        const Texture* texture = below ? m_Floor : (roofed ? m_Canopy : m_Ceiling);
        const int fromHorizon = below ? row - m_Horizon : m_Horizon - row;
        r.canopy = false;
        if (!texture || !texture->index || fromHorizon <= 0)
        {
            r.valid = false;
            continue;
        }
        // The camera is half a wall up: a row this far from the horizon sees
        // the floor (or ceiling) this far away.
        const Fix distance = ((m_Height / 2) << kFixShift) / fromHorizon;
        r.startX = posX + Mul(distance, leftX);
        r.startY = posY + Mul(distance, leftY);
        r.stepX = Mul(distance, spanX) / m_Width;
        r.stepY = Mul(distance, spanY) / m_Width;
        const int size = texture->size;
        r.sizeShift = static_cast<uint8_t>(Log2(size));
        const int shade = Shade16(distance, texture->levels);
        r.paletteOffset = (shade >> 4) * texture->colours;
        r.dither = static_cast<uint8_t>(shade & 15);
        r.valid = !roofed;  // the sky is drawn there; the cloth goes over it after
        r.canopy = roofed;
        r.isFloor = below;
    }
}

DUNGEON_HOT void Raycaster::CastColumn(const DungeonMap& map, int x, Fix posX, Fix posY, Fix dirX, Fix dirY, Fix planeX,
                                       Fix planeY)
{
    Fix rayX = dirX + Mul(planeX, m_CameraX[x]);
    Fix rayY = dirY + Mul(planeY, m_CameraX[x]);
    int mapX = posX >> kFixShift;
    int mapY = posY >> kFixShift;
    Fix deltaX = 0;
    Fix deltaY = 0;
    int stepX = 1;
    int stepY = 1;
    int64_t sideX = 0;
    int64_t sideY = 0;

    // Distance along the ray (in units of the ray, so the depth comes out
    // square to the view) to the first vertical and horizontal grid line.
    // `posX`/`posY` need not be inside the map cell: after an impossible
    // door it is where the ray would have started to arrive where it did.
    const auto aim = [&]()
    {
        deltaX = Reciprocal(rayX);
        deltaY = Reciprocal(rayY);
        stepX = rayX < 0 ? -1 : 1;
        stepY = rayY < 0 ? -1 : 1;
        const int64_t toX = rayX < 0 ? static_cast<int64_t>(posX) - (static_cast<int64_t>(mapX) << kFixShift)
                                     : (static_cast<int64_t>(mapX + 1) << kFixShift) - posX;
        const int64_t toY = rayY < 0 ? static_cast<int64_t>(posY) - (static_cast<int64_t>(mapY) << kFixShift)
                                     : (static_cast<int64_t>(mapY + 1) << kFixShift) - posY;
        sideX = (toX * deltaX) >> kFixShift;
        sideY = (toY * deltaY) >> kFixShift;
    };
    aim();

    // Grid line to grid line until a wall. Stepping into an impossible door
    // from its front, the ray comes out of the other side, turned with it,
    // and goes on: the depth keeps counting from the viewer.
    Column& column = m_Columns[x];
    column.portalDepth = INT32_MAX;
    column.portal = 0;
    column.upColumn = nullptr;
    int side = 0;
    bool hit = false;
    int portals = 0;
    for (int steps = 0; steps < kMaxSteps && !hit; ++steps)
    {
        int64_t crossing = 0;
        int heading = 0;
        if (sideX < sideY)
        {
            crossing = sideX;
            sideX += deltaX;
            mapX += stepX;
            side = 0;
            heading = stepX > 0 ? 0 : 2;
        }
        else
        {
            crossing = sideY;
            sideY += deltaY;
            mapY += stepY;
            side = 1;
            heading = stepY > 0 ? 1 : 3;
        }
        const Portal* portal = portals < 4 ? map.PortalEntering({ mapX, mapY }, heading) : nullptr;
        if (portal)
        {
            ++portals;
            const Fix t = static_cast<Fix>(crossing);
            if (column.portalDepth == INT32_MAX)
            {
                column.portalDepth = t;
                column.portal = static_cast<uint8_t>(1 + (portal - &map.GetPortal(0)));
            }
            // Where the ray crosses into the door, from the door's centre,
            // turned as the door turns and put round the other side's centre.
            Fix lx = posX + Mul(rayX, t) - ((mapX << kFixShift) + kFixOne / 2);
            Fix ly = posY + Mul(rayY, t) - ((mapY << kFixShift) + kFixOne / 2);
            const int turns = (portal->toDir - portal->dir) & 3;
            for (int k = 0; k < turns; ++k)
            {
                // A quarter turn clockwise on a map whose y points down.
                const Fix nx = -ly;
                ly = lx;
                lx = nx;
                const Fix rx = -rayY;
                rayY = rayX;
                rayX = rx;
            }
            mapX = portal->to.x;
            mapY = portal->to.y;
            const Fix arriveX = (mapX << kFixShift) + kFixOne / 2 + lx;
            const Fix arriveY = (mapY << kFixShift) + kFixOne / 2 + ly;
            posX = arriveX - Mul(rayX, t);
            posY = arriveY - Mul(rayY, t);
            aim();
            // The ray is on the boundary already: what it steps past next is
            // measured from here, so the crossings stay in order.
            continue;
        }
        hit = map.BlocksSight({ mapX, mapY });
    }

    int64_t depth64 = side == 0 ? sideX - deltaX : sideY - deltaY;
    if (depth64 < kMinDepth)
    {
        depth64 = kMinDepth;
    }
    if (depth64 > INT32_MAX)
    {
        depth64 = INT32_MAX;
    }
    const Fix depth = static_cast<Fix>(depth64);

    const int lineHeight = (m_Height << kFixShift) / depth;
    column.depth = depth;
    column.top = static_cast<int16_t>(m_Horizon - lineHeight / 2);
    column.bottom = static_cast<int16_t>(column.top + lineHeight);
    if (lineHeight > 30000)
    {
        column.top = -15000;
        column.bottom = 15000;
    }

    // Which wall: most the first, the others less and less often, the same
    // for a tile every frame; a hideout's way in its own.
    const Texture* texture = nullptr;
    if (m_WallCount > 0)
    {
        // Out of 100: where each further kind starts, by how many there are.
        static constexpr int kStarts[kMaxWallKinds + 1][kMaxWallKinds] = {
            { 100, 100, 100, 100 }, { 100, 100, 100, 100 }, { 85, 100, 100, 100 },
            { 72, 90, 100, 100 },   { 66, 84, 95, 100 },
        };
        const auto kindAt = [&](int tx, int ty)
        {
            const int roll = static_cast<int>(TileHash(tx, ty) % 100);
            int k = 0;
            while (k < m_WallCount - 1 && roll >= kStarts[m_WallCount][k])
            {
                ++k;
            }
            return k;
        };
        int kind = kindAt(mapX, mapY);
        if (kind > 0)
        {
            // A feature wall (a skull, a waterfall) never stands next to
            // another: of neighbours that would both have one, only the one
            // with the higher roll keeps it.
            const uint32_t mine = TileHash(mapX, mapY);
            constexpr int kNx[4] = { 1, -1, 0, 0 };
            constexpr int kNy[4] = { 0, 0, 1, -1 };
            for (int n = 0; n < 4 && kind > 0; ++n)
            {
                const int tx = mapX + kNx[n];
                const int ty = mapY + kNy[n];
                if (map.BlocksSight({ tx, ty }) && kindAt(tx, ty) > 0 && TileHash(tx, ty) > mine)
                {
                    kind = 0;
                }
            }
        }
        texture = m_Walls[kind];
    }
    const Tile tile = map.At(mapX, mapY);
    if (tile == Tile::SecretWall && m_SecretWall)
    {
        texture = m_SecretWall;
    }
    else if (tile == Tile::Door && m_Door)
    {
        texture = m_Door;
    }
    else if (tile == Tile::EchoWall && m_EchoWall)
    {
        texture = m_EchoWall;
    }
    // The storey on top, where walls have one.
    const Texture* up = m_WallTopCount > 0 ? m_WallTops[(TileHash(mapX, mapY) >> 5) % m_WallTopCount] : nullptr;
    if (m_CellWalls && DungeonMap::InBounds(mapX, mapY))
    {
        const int own = m_CellWalls[mapY * kMapWidth + mapX];
        if (own > 0 && own <= m_CellWallCount)
        {
            texture = m_CellWallTextures[own - 1];
            if (up && m_CellWallTops && m_CellWallTops[own - 1])
            {
                up = m_CellWallTops[own - 1];
            }
        }
    }
    if (!texture || !texture->index)
    {
        column.texColumn = nullptr;
        return;
    }
    const int size = texture->size;

    // Where along the wall the ray struck, 0..1, as a texture column.
    const Fix along = side == 0 ? posY + Mul(depth, rayY) : posX + Mul(depth, rayX);
    int texX = static_cast<int>((static_cast<int64_t>(along & (kFixOne - 1)) * size) >> kFixShift);
    if ((side == 0 && rayX > 0) || (side == 1 && rayY < 0))
    {
        texX = size - 1 - texX;
    }
    // Every other tile, by its place, the picture the other way round: the
    // same wall does not repeat so plainly.
    if ((TileHash(mapX, mapY) >> 9) & 1)
    {
        texX = size - 1 - texX;
    }

    // Walls across the view's y axis a shade darker, so corners read; near
    // a torch, brighter.
    // Steps of shade are counted in eighths of the way to dark, whatever
    // number of levels the texture has.
    const int coarse = texture->levels >= 8 ? texture->levels / 8 : 1;
    int shade = Shade16(depth, texture->levels) + side * 16 * coarse;
    // What stands in shadow is never lit brighter than two thirds of its shadow: a
    // lantern there is a dim pool, not daylight.
    int least = 0;
    if (m_Gloom)
    {
        // In shadow where the floor it stands on is.
        const int fromX = side == 0 ? mapX - stepX : mapX;
        const int fromY = side == 1 ? mapY - stepY : mapY;
        if (DungeonMap::InBounds(fromX, fromY))
        {
            const int gloom = m_Gloom[fromY * kMapWidth + fromX] * 16 * coarse;
            shade += gloom;
            least = gloom * 2 / 3;
        }
    }
    if (m_LightCount > 0)
    {
        shade -= LightBoost(posX + Mul(rayX, depth), posY + Mul(rayY, depth)) * coarse;
        shade = shade < least ? least : shade;
    }
    if (shade > (texture->levels - 1) * 16)
    {
        shade = (texture->levels - 1) * 16;
    }
    const int level = shade >> 4;
    column.texSize = static_cast<uint8_t>(size);
    column.texColumn = texture->index + texX;
    column.palette = texture->Palette(level);
    column.darker = texture->Palette(level + 1 < texture->levels ? level + 1 : level);
    column.dither = static_cast<uint8_t>(shade & 15);
    column.reflected =
        texture->Palette(level + 2 * coarse < texture->levels ? level + 2 * coarse : texture->levels - 1);
    column.texStep = lineHeight > 0 ? (size << kFixShift) / lineHeight : 0;
    column.texStart = 0;
    if (up && up->index && lineHeight > 0 && lineHeight <= 30000)
    {
        const int upLevel = level < up->levels ? level : up->levels - 1;
        column.upSize = static_cast<uint8_t>(up->size);
        column.upColumn = up->index + texX * up->size / size;
        column.upPalette = up->Palette(upLevel);
        column.upDarker = up->Palette(upLevel + 1 < up->levels ? upLevel + 1 : upLevel);
        column.upFrom = static_cast<int16_t>(column.top - lineHeight);
    }
}

const uint16_t kFlashPalette[256] = {
#define DUNGEON_FLASH_16                                                                                               \
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,    \
        0xFFFF, 0xFFFF
    kTransparent,     0xFFFF,           0xFFFF,           0xFFFF,           0xFFFF,           0xFFFF,
    0xFFFF,           0xFFFF,           0xFFFF,           0xFFFF,           0xFFFF,           0xFFFF,
    0xFFFF,           0xFFFF,           0xFFFF,           0xFFFF,           DUNGEON_FLASH_16, DUNGEON_FLASH_16,
    DUNGEON_FLASH_16, DUNGEON_FLASH_16, DUNGEON_FLASH_16, DUNGEON_FLASH_16, DUNGEON_FLASH_16, DUNGEON_FLASH_16,
    DUNGEON_FLASH_16, DUNGEON_FLASH_16, DUNGEON_FLASH_16, DUNGEON_FLASH_16, DUNGEON_FLASH_16, DUNGEON_FLASH_16,
    DUNGEON_FLASH_16,
#undef DUNGEON_FLASH_16
};

bool Raycaster::Project(Fix x, Fix y, Fix height, int& screenX, int& screenY, Fix& depth) const
{
    if (!m_Columns)
    {
        return false;
    }
    const Fix det = Mul(m_PlaneX, m_DirY) - Mul(m_DirX, m_PlaneY);
    if (det == 0)
    {
        return false;
    }
    const int64_t invDet = (static_cast<int64_t>(1) << (2 * kFixShift)) / det;
    const Fix sx = x - m_ViewX;
    const Fix sy = y - m_ViewY;
    const Fix across = static_cast<Fix>((invDet * (Mul(m_DirY, sx) - Mul(m_DirX, sy))) >> kFixShift);
    depth = static_cast<Fix>((invDet * (Mul(-m_PlaneY, sx) + Mul(m_PlaneX, sy))) >> kFixShift);
    if (depth < kFixOne / 8)
    {
        return false;
    }
    screenX = m_Width / 2 + static_cast<int>((static_cast<int64_t>(m_Width / 2) * across) / depth);
    if (screenX < 0 || screenX >= m_Width || depth >= m_Columns[screenX].depth)
    {
        return false;
    }
    screenY = m_Horizon + static_cast<int>(Mul(kFixOne / 2 - height, m_Height << kFixShift) / depth);
    // In screen pixels.
    screenX *= m_Pixel;
    screenY *= m_Pixel;
    return true;
}

void Raycaster::PlaceBillboards(const DungeonMap& map, const Billboard* billboards, int count, Fix posX, Fix posY,
                                Fix dirX, Fix dirY, Fix planeX, Fix planeY)
{
    m_PlacedCount = 0;
    // 1 / the camera matrix's determinant.
    const Fix det = Mul(planeX, dirY) - Mul(dirX, planeY);
    if (det == 0)
    {
        return;
    }
    const int64_t invDet = (static_cast<int64_t>(1) << (2 * kFixShift)) / det;
    // Each billboard once where it stands, and once more for each impossible
    // door, where it shows through that door: its place on the far side
    // brought back to this side. Columns that look through the door draw only
    // those (RenderRows).
    for (int pass = 0; pass <= kMaxPortals; ++pass)
    {
        const Portal* portal = pass > 0 ? &map.GetPortal(pass - 1) : nullptr;
        if (portal && !portal->present)
        {
            continue;
        }
        for (int i = 0; i < count && m_PlacedCount < kMaxPlaced; ++i)
        {
            const Billboard& b = billboards[i];
            if (!b.texture || !b.texture->index)
            {
                continue;
            }
            Fix bx = b.x;
            Fix by = b.y;
            if (portal)
            {
                // From the far side's frame back into this one: undo the
                // door's turn about its two ends.
                Fix lx = bx - ((portal->to.x << kFixShift) + kFixOne / 2);
                Fix ly = by - ((portal->to.y << kFixShift) + kFixOne / 2);
                const int turns = (portal->dir - portal->toDir) & 3;
                for (int k = 0; k < turns; ++k)
                {
                    const Fix nx = -ly;
                    ly = lx;
                    lx = nx;
                }
                bx = (portal->at.x << kFixShift) + kFixOne / 2 + lx;
                by = (portal->at.y << kFixShift) + kFixOne / 2 + ly;
            }
            PlaceOne(b, bx, by, static_cast<uint8_t>(pass), posX, posY, dirX, dirY, planeX, planeY, invDet);
        }
    }
}

DUNGEON_HOT void Raycaster::PlaceOne(const Billboard& b, Fix bx, Fix by, uint8_t through, Fix posX, Fix posY, Fix dirX,
                                     Fix dirY, Fix planeX, Fix planeY, int64_t invDet)
{
    {
        const Fix sx = bx - posX;
        const Fix sy = by - posY;
        const Fix across = static_cast<Fix>((invDet * (Mul(dirY, sx) - Mul(dirX, sy))) >> kFixShift);
        const Fix depth = static_cast<Fix>((invDet * (Mul(-planeY, sx) + Mul(planeX, sy))) >> kFixShift);
        if (depth < kFixOne / 8)
        {
            return;  // behind the viewer, or too close to draw
        }
        const int centreX = m_Width / 2 + static_cast<int>((static_cast<int64_t>(m_Width / 2) * across) / depth);
        const int floorHeight = static_cast<int>((static_cast<int64_t>(m_Height) << kFixShift) / depth);
        const int height = static_cast<int>((static_cast<int64_t>(floorHeight) * b.scale) >> kFixShift);
        if (height <= 0 || centreX + height < 0 || centreX - height > m_Width)
        {
            return;
        }
        Placed p;
        p.height = static_cast<int16_t>(height > 4000 ? 4000 : height);
        const int width =
            static_cast<int>((static_cast<int64_t>(height > 4000 ? 4000 : height) * b.squash) >> kFixShift);
        p.width = static_cast<int16_t>(width > 4000 ? 4000 : (width > 0 ? width : 1));
        p.left = static_cast<int16_t>(centreX - p.width / 2);
        // Standing on the floor: its bottom on the floor line where its base
        // is (nearer than its middle by its foot), raised by its lift.
        const Fix baseDepth = depth - b.foot > kFixOne / 8 ? depth - b.foot : kFixOne / 8;
        const int baseHeight = static_cast<int>((static_cast<int64_t>(m_Height) << kFixShift) / baseDepth);
        const int lift = static_cast<int>((static_cast<int64_t>(p.height) * b.lift) >> kFixShift);
        p.top = static_cast<int16_t>(m_Horizon + baseHeight / 2 - p.height - lift);
        p.depth = depth;
        const Texture& texture = *b.texture;
        p.texSize = static_cast<uint8_t>(texture.size);
        p.texStep = static_cast<Fix>((static_cast<int64_t>(texture.size) << kFixShift) / p.height);
        if (b.breath != 0 && p.height >= 16)
        {
            // Breathed out: the rows above its middle come down one, over
            // the row that goes.
            p.cut = static_cast<int16_t>(p.height * 11 / 20);
            p.top = static_cast<int16_t>(p.top + 1);
            p.height = static_cast<int16_t>(p.height - 1);
        }
        p.texStepX = static_cast<Fix>((static_cast<int64_t>(texture.size) << kFixShift) / p.width);
        const int coarse = texture.levels >= 8 ? texture.levels / 8 : 1;
        int level = ShadeLevel(depth, texture.levels) +
                    (b.shadeBias - (m_LightCount > 0 ? (LightBoost(bx, by) + 8) / 16 : 0)) * coarse;
        if (m_Gloom && DungeonMap::InBounds(bx >> kFixShift, by >> kFixShift) && !b.palette)
        {
            const int gloom = m_Gloom[(by >> kFixShift) * kMapWidth + (bx >> kFixShift)] * coarse;
            level += gloom;
            level = level < gloom * 2 / 3 ? gloom * 2 / 3 : level;
        }
        level = level < 0 ? 0 : (level > texture.levels - 1 ? texture.levels - 1 : level);
        p.index = texture.index;
        p.palette = b.palette ? b.palette : texture.Palette(level);
        p.through = through;

        // Kept far to near: nearer ones draw over farther ones.
        int at = m_PlacedCount++;
        while (at > 0 && m_Placed[at - 1].depth < depth)
        {
            m_Placed[at] = m_Placed[at - 1];
            --at;
        }
        m_Placed[at] = p;
    }
}

DUNGEON_HOT uint16_t Raycaster::PitColour(Fix fx, Fix fy, int level) const
{
    // The ray from the eye through this point of the floor goes on down
    // into the hole: four steps going down toward the south, each a quarter
    // of the hole long, the hole's sides round them. Worked out in 1/256ths
    // of a tile (and of a wall's height), which is finer than a pixel of it.
    constexpr int kRim = 32;  // the frame round the hole
    constexpr int kWide = 256 - 2 * kRim;
    constexpr int kSteps = 4;
    constexpr int kDeep = 236;  // the last step: nearly a wall's height down
    const Texture& stone = *m_Floor;
    const int size = stone.size;
    const int levels = stone.levels;
    // Where in the hole the ray enters, and how far it moves sideways for
    // each 1/256 of depth (the eye is half a wall above the floor), in 8.8.
    const int u0 = ((fx & (kFixOne - 1)) >> 8) - kRim;
    const int v0 = ((fy & (kFixOne - 1)) >> 8) - kRim;
    const int ax = (fx - m_ViewX) >> 7;
    const int ay = (fy - m_ViewY) >> 7;
    // The depth at which it would leave by a side, if it gets that far.
    const int sideAt = ax > 0 ? ((kWide - u0) << 8) / ax : (ax < 0 ? (u0 << 8) / -ax : 0x7FFFFFFF);
    const int perV = ay != 0 ? (1 << 16) / (ay > 0 ? ay : -ay) : 0;  // depth for each 1/256 of v
    int step = v0 * kSteps / kWide;
    step = step < 0 ? 0 : (step > kSteps - 1 ? kSteps - 1 : step);
    int depth = 0;  // where the ray stops
    int u = 0;      // and what of the stone shows there
    int v = 0;
    int darker = 0;  // a side or a riser: a shade down from a tread
    for (int tries = 0; tries <= kSteps; ++tries)
    {
        const int tread = (step + 1) * kDeep / kSteps;
        // Where it leaves this step's part of the hole, toward the next
        // (deeper) step or back toward the last.
        const int edge = ay > 0 ? (step + 1) * kWide / kSteps : step * kWide / kSteps;
        const int edgeAt = ay != 0 ? ((ay > 0 ? edge - v0 : v0 - edge) * perV) >> 8 : 0x7FFFFFFF;
        if (tread <= sideAt && tread <= edgeAt)
        {
            // Down onto the tread: its front edge a shade lighter.
            depth = tread;
            u = u0 + ((ax * tread) >> 8);
            v = v0 + ((ay * tread) >> 8);
            darker = (step + 1) * kWide / kSteps - v < kWide / (kSteps * 4) ? -2 : 0;
            break;
        }
        if (sideAt <= edgeAt)
        {
            // Into the hole's side.
            depth = sideAt;
            u = v0 + ((ay * sideAt) >> 8);
            v = sideAt;
            darker = 1;
            break;
        }
        if (ay > 0 && step == kSteps - 1)
        {
            // Past the last step: the dark it goes on into.
            depth = kDeep * 2;
            break;
        }
        if (ay < 0 && (step == 0 || edgeAt > step * kDeep / kSteps))
        {
            // Back into the hole's end, or the riser of the step before.
            depth = edgeAt;
            u = u0 + ((ax * edgeAt) >> 8);
            v = edgeAt;
            darker = 3;
            break;
        }
        step += ay > 0 ? 1 : -1;
    }
    // Darker the deeper: black a little below the last step.
    const int coarse = levels >= 8 ? levels / 8 : 1;
    int shade = level + depth * levels / 420 + darker * coarse;
    shade = shade < 0 ? 0 : (shade > levels - 1 ? levels - 1 : shade);
    const int tu = ((u * size) >> 8) & (size - 1);
    const int tv = ((v * size) >> 8) & (size - 1);
    return stone.palette[shade * stone.colours + stone.index[tv * size + tu]];
}

DUNGEON_HOT void Raycaster::RenderRows(uint16_t* dst, int strideBytes, int firstRow, int rowCount, int columnFrom,
                                       int columnTo) const
{
    if (!m_Columns)
    {
        return;  // no storage yet
    }
    if (m_Pixel == 1)
    {
        RenderOwnRows(dst, strideBytes, firstRow, rowCount, columnFrom, columnTo);
        return;
    }
    // Doubled pixels: the view's own rows into the scratch, a few at a time,
    // then each written twice across and twice down.
    const int stride = strideBytes / 2;
    const int left = columnFrom > 0 ? columnFrom : 0;
    const int right = columnTo < 0 || columnTo > m_Width * 2 ? m_Width * 2 : columnTo;
    const int end = firstRow + rowCount < m_Height * 2 ? firstRow + rowCount : m_Height * 2;
    int row = firstRow > 0 ? firstRow : 0;
    while (row < end && left < right)
    {
        const int ownFirst = row / 2;
        int ownCount = (end + 1) / 2 - ownFirst;
        ownCount = ownCount < kScratchRows ? ownCount : kScratchRows;
        RenderOwnRows(m_Scratch, m_Width * 2, ownFirst, ownCount, left / 2, (right + 1) / 2);
        const int chunkEnd = (ownFirst + ownCount) * 2 < end ? (ownFirst + ownCount) * 2 : end;
        for (; row < chunkEnd; ++row)
        {
            const uint16_t* in = m_Scratch + (row / 2 - ownFirst) * m_Width;
            uint16_t* out = dst + (row - firstRow) * stride;
            int x = left;
            if (x & 1)
            {
                out[x] = in[x >> 1];
                ++x;
            }
            for (; x + 1 < right; x += 2)
            {
                const uint16_t pixel = in[x >> 1];
                out[x] = pixel;
                out[x + 1] = pixel;
            }
            if (x < right)
            {
                out[x] = in[x >> 1];
            }
        }
    }
}

DUNGEON_HOT void Raycaster::RenderOwnRows(uint16_t* dst, int strideBytes, int firstRow, int rowCount, int columnFrom,
                                          int columnTo) const
{
    const int stride = strideBytes / 2;
    const int left = columnFrom > 0 ? columnFrom : 0;
    const int right = columnTo < 0 || columnTo > m_Width ? m_Width : columnTo;
    // The floor's marks and second floor, where they match the first floor's
    // size and colours (they share its rows' shade).
    const Texture* lookTexture[kFloorLooks] = {};
    for (int i = 1; i < kFloorLooks; ++i)
    {
        const Texture* look = m_Looks[i];
        if (look && look->index && m_Floor && look->size == m_Floor->size && look->colours == m_Floor->colours &&
            look->levels == m_Floor->levels)
        {
            lookTexture[i] = look;
        }
    }
    const Texture* floorB = m_FloorCount > 1 && m_Floors[1]->size == m_Floor->size &&
                                    m_Floors[1]->colours == m_Floor->colours && m_Floors[1]->levels == m_Floor->levels
                                ? m_Floors[1]
                                : m_Floor;
    // The walls first, column by column: each column's slice of its wall,
    // straight down, with nothing to look up again per pixel.
    const uint64_t wallsFrom = m_Clock ? m_Clock() : 0;
    const int rowsFrom = firstRow > 0 ? firstRow : 0;
    const int rowsTo = firstRow + rowCount < m_Height ? firstRow + rowCount : m_Height;
    for (int x = left; x < right; ++x)
    {
        const Column& c = m_Columns[x];
        const int from = c.top > rowsFrom ? c.top : rowsFrom;
        const int to = c.bottom < rowsTo ? c.bottom : rowsTo;
        if (!c.texColumn || from >= to)
        {
            continue;
        }
        const uint8_t* texColumn = c.texColumn;
        const uint16_t* palettes[4];
        for (int r = 0; r < 4; ++r)
        {
            palettes[r] = kDither[r][x & 3] < c.dither ? c.darker : c.palette;
        }
        const int size = c.texSize;
        const Fix step = c.texStep;
        Fix v = c.texStart + (from - c.top) * step;
        uint16_t* out = dst + (from - firstRow) * stride + x;
        for (int row = from; row < to; ++row)
        {
            const int texel = v >> kFixShift;
            *out = palettes[row & 3][texColumn[(texel < size ? texel : size - 1) * size]];
            out += stride;
            v += step;
        }
    }

    const uint64_t floorsFrom = m_Clock ? m_Clock() : 0;
    for (int i = 0; i < rowCount; ++i)
    {
        const int row = firstRow + i;
        if (row < 0 || row >= m_Height)
        {
            continue;
        }
        uint16_t* out = dst + i * stride;
        const uint16_t background = m_RowColour[row];
        const FloorRow& fr = m_FloorRows[row];
        const Texture* surface = fr.isFloor ? m_Floor : m_Ceiling;
        // Floor or ceiling texture coordinates, stepped along the row.
        Fix fx = fr.startX + fr.stepX * left;
        Fix fy = fr.startY + fr.stepY * left;
        const int shift = kFixShift - fr.sizeShift;
        const int mask = (1 << fr.sizeShift) - 1;
        const int sizeShift = fr.sizeShift;
        const int styleShift = fr.isFloor ? 0 : 5;
        // Water ripples: the reflection's rows sway a pixel or two, row by row.
        const int ripple =
            m_Reflective && fr.isFloor
                ? static_cast<int>((static_cast<int64_t>(Sin(row * 40 + static_cast<int>(m_TimeMs / 3))) * 2) >>
                                   kFixShift)
                : 0;
        // What the tile under the last pixel needed, kept while the next
        // pixels stay on it: most of a row's pixels share their tile.
        // Above the horizon with no ceiling: the sky's row, if there is one.
        const uint8_t* skyRow = nullptr;
        if (!fr.valid && m_Sky && m_Sky->index && row < m_Horizon)
        {
            const int size = m_Sky->size;
            const int v = row * size / (m_Horizon > 0 ? m_Horizon : 1);
            skyRow = m_Sky->index + (v < size ? v : size - 1) * size;
        }
        const int skyMask = m_Sky ? m_Sky->size - 1 : 0;
        unsigned lastCell = 0xFFFFFFFFu;
        int turn = 0;
        bool mayReflect = false;
        bool pit = false;
        const int rowLevel = surface && surface->colours > 0 ? fr.paletteOffset / surface->colours : 0;
        const uint8_t* texIndex = nullptr;
        // The row's shade and the next: which of the two each column takes.
        const uint16_t* texPalettes[2] = {};
        unsigned darkerColumns = 0;
        for (int k = 0; k < 4; ++k)
        {
            darkerColumns |= (kDither[row & 3][k] < fr.dither ? 1u : 0u) << k;
        }
        // A shadow on the tile: its middle, the squares of its three rings'
        // radii (in 1/256 tiles), and the palettes one, two and three
        // levels darker.
        const Shadow* shadow = nullptr;
        int32_t ring2[3] = {};
        const uint16_t* shaded[3] = {};
        // A light over the tile: where it is, and the tile's palettes to pick
        // a brighter shade from, by how near each pixel is.
        bool lit = false;
        Fix lightX = 0;
        Fix lightY = 0;
        const uint16_t* tilePalette = nullptr;
        int tileColours = 0;
        int rowShade = 0;
        int least = 0;
        int darkest = 0;  // the tile's last shade: past it is another texture's memory
        int coarse = 1;
        for (int x = left; x < right; ++x, fx += fr.stepX, fy += fr.stepY)
        {
            const Column& c = m_Columns[x];
            if (row >= c.top && row < c.bottom && c.texColumn)
            {
                continue;  // a wall, already drawn
            }
            if (!fr.valid)
            {
                out[x] = skyRow ? m_Sky->palette[skyRow[((m_SkyU0 + x * m_SkyDu) >> kFixShift) & skyMask]] : background;
                continue;
            }
            const unsigned cx = static_cast<unsigned>(fx >> kFixShift);
            const unsigned cy = static_cast<unsigned>(fy >> kFixShift);
            const bool inMap = cx < static_cast<unsigned>(kMapWidth) && cy < static_cast<unsigned>(kMapHeight);
            const unsigned cell = inMap ? cy * kMapWidth + cx : 0xFFFFFFFEu;
            if (cell != lastCell)
            {
                // A new tile: its own turn and mirror, its floor, its mark.
                lastCell = cell;
                const Texture* texture = surface;
                turn = 0;
                mayReflect = false;
                pit = false;
                if (inMap)
                {
                    const int style = m_CellStyle[cell];
                    turn = style >> styleShift;
                    if (fr.isFloor)
                    {
                        const uint8_t look = m_MapLooks[cell];
                        pit = look == static_cast<uint8_t>(FloorLook::StairsHole);
                        if (pit)
                        {
                            // The hole's stone frame, the same way up for
                            // every way down.
                            turn = 0;
                        }
                        if (look != 0 && look < kFloorLooks && lookTexture[look])
                        {
                            texture = lookTexture[look];
                        }
                        else if (style & 8)
                        {
                            texture = floorB;
                        }
                        else
                        {
                            mayReflect = m_Reflective;
                        }
                    }
                }
                texIndex = texture->index;
                // In shadow: this tile's floor some shades darker.
                int offset = fr.paletteOffset;
                if (m_Gloom && inMap && m_Gloom[cell] != 0 && fr.isFloor)
                {
                    const int most = (texture->levels - 1) * texture->colours;
                    offset += m_Gloom[cell] * (texture->levels >= 8 ? texture->levels / 8 : 1) * texture->colours;
                    offset = offset < most ? offset : most;
                }
                texPalettes[0] = texture->palette + offset;
                texPalettes[1] = offset + texture->colours < texture->levels * texture->colours
                                     ? texPalettes[0] + texture->colours
                                     : texPalettes[0];
                lit = inMap && m_LightCell[cell] != 0;
                if (lit)
                {
                    lightX = m_LightX[m_LightCell[cell] - 1];
                    lightY = m_LightY[m_LightCell[cell] - 1];
                    tilePalette = texture->palette;
                    tileColours = texture->colours;
                    // (Its shadow included, and never lit past two thirds of it.)
                    rowShade = offset / texture->colours * 16 + fr.dither;
                    least = (offset - fr.paletteOffset) / texture->colours * 32 / 3;
                    darkest = (texture->levels - 1) * 16;
                    coarse = texture->levels >= 8 ? texture->levels / 8 : 1;
                }
                shadow = nullptr;
                if (inMap && fr.isFloor && m_ShadowCell[cell] != 0)
                {
                    shadow = &m_ShadowInput[m_ShadowCell[cell] - 1];
                    const int32_t r = shadow->radius >> 8;
                    ring2[0] = r * r * 36 / 100;  // the darkest middle
                    ring2[1] = r * r * 66 / 100;
                    ring2[2] = r * r;
                    const int level = fr.paletteOffset / texture->colours;
                    for (int k = 0; k < 3; ++k)
                    {
                        const int coarse = texture->levels >= 8 ? texture->levels / 8 : 1;
                        const int down = level + (4 - k) * coarse;
                        const int darker = down < texture->levels ? down : texture->levels - 1;
                        shaded[k] = texture->palette + darker * texture->colours;
                    }
                }
            }
            if (pit)
            {
                // Inside the frame: down the hole.
                constexpr int kRim = kFixOne / 8;
                const int pu = fx & (kFixOne - 1);
                const int pv = fy & (kFixOne - 1);
                if (pu >= kRim && pu < kFixOne - kRim && pv >= kRim && pv < kFixOne - kRim)
                {
                    out[x] = PitColour(fx, fy, rowLevel);
                    continue;
                }
            }
            if (mayReflect && c.texColumn && ((x + row) & 1))
            {
                // Water: the wall above, mirrored at its foot.
                const int mirrored = 2 * c.bottom - 1 - row + ripple;
                if (mirrored >= c.top && mirrored < c.bottom)
                {
                    int v = static_cast<int>((c.texStart + (mirrored - c.top) * c.texStep) >> kFixShift);
                    v = v < c.texSize ? v : c.texSize - 1;
                    out[x] = c.reflected[c.texColumn[v * c.texSize]];
                    continue;
                }
            }
            int u = (fx >> shift) & mask;
            int w = (fy >> shift) & mask;
            if (turn & 1)
            {
                u = mask - u;
            }
            if (turn & 2)
            {
                w = mask - w;
            }
            if (turn & 4)
            {
                const int t = u;
                u = w;
                w = t;
            }
            const uint8_t index = texIndex[(w << sizeShift) + u];
            if (shadow)
            {
                const int32_t dx = (fx - shadow->x) >> 8;
                const int32_t dy = (fy - shadow->y) >> 8;
                const int32_t d2 = dx * dx + dy * dy;
                if (d2 < ring2[2])
                {
                    out[x] = shaded[d2 < ring2[0] ? 0 : (d2 < ring2[1] ? 1 : 2)][index];
                    continue;
                }
            }
            if (lit)
            {
                const int32_t dx = (fx - lightX) >> 8;
                const int32_t dy = (fy - lightY) >> 8;
                const int32_t d2 = dx * dx + dy * dy;
                if (d2 < m_LightR2)
                {
                    // Brighter the nearer, in sixteenths of a shade, the part
                    // left over mixed by the pattern.
                    int boost = (((m_LightR2 - d2) >> 4) * m_LightScale) >> 16;
                    boost = boost > kLightMost ? kLightMost : boost;
                    int shade = rowShade - boost * coarse;
                    shade = shade < least ? least : (shade > darkest ? darkest : shade);
                    const int level = (shade >> 4) + (kDither[row & 3][x & 3] < (shade & 15) ? 1 : 0);
                    out[x] = tilePalette[level * tileColours + index];
                    continue;
                }
            }
            out[x] = texPalettes[(darkerColumns >> (x & 3)) & 1][index];
        }
    }

    // The storeys on top of the walls, over the sky: column by column, the
    // sky showing where their picture is empty.
    for (int x = left; x < right && m_WallTopCount > 0; ++x)
    {
        const Column& c = m_Columns[x];
        const int from = c.upFrom > rowsFrom ? c.upFrom : rowsFrom;
        const int to = c.top < rowsTo ? c.top : rowsTo;
        if (!c.upColumn || from >= to)
        {
            continue;
        }
        const uint8_t* upColumn = c.upColumn;
        const uint16_t* palettes[4];
        for (int r = 0; r < 4; ++r)
        {
            palettes[r] = kDither[r][x & 3] < c.dither ? c.upDarker : c.upPalette;
        }
        const int size = c.upSize;
        const int rows = c.top - c.upFrom;
        const Fix step = (size << kFixShift) / rows;
        Fix v = (from - c.upFrom) * step;
        uint16_t* out = dst + (from - firstRow) * stride + x;
        for (int row = from; row < to; ++row)
        {
            const int texel = v >> kFixShift;
            const uint8_t index = upColumn[(texel < size ? texel : size - 1) * size];
            if (index != 0)
            {
                *out = palettes[row & 3][index];
            }
            out += stride;
            v += step;
        }
    }

    // The cloth over the roofed tiles: across at the walls' tops, so it
    // goes over the sky and the storeys behind it, and never over a wall.
    for (int i = 0; i < rowCount && m_Canopy && m_Canopy->index && m_Gloom; ++i)
    {
        const int row = firstRow + i;
        if (row < 0 || row >= m_Height || !m_FloorRows[row].canopy)
        {
            continue;
        }
        const FloorRow& fr = m_FloorRows[row];
        const Texture& cloth = *m_Canopy;
        const int shift = kFixShift - fr.sizeShift;
        const int mask = (1 << fr.sizeShift) - 1;
        const int coarse = cloth.levels >= 8 ? cloth.levels / 8 : 1;
        const int rowLevel = fr.paletteOffset / cloth.colours;
        uint16_t* out = dst + i * stride;
        Fix fx = fr.startX + fr.stepX * left;
        Fix fy = fr.startY + fr.stepY * left;
        for (int x = left; x < right; ++x, fx += fr.stepX, fy += fr.stepY)
        {
            const Column& c = m_Columns[x];
            const unsigned cx = static_cast<unsigned>(fx >> kFixShift);
            const unsigned cy = static_cast<unsigned>(fy >> kFixShift);
            if ((c.texColumn && row >= c.top) || cx >= static_cast<unsigned>(kMapWidth) ||
                cy >= static_cast<unsigned>(kMapHeight))
            {
                continue;
            }
            const int gloom = m_Gloom[cy * kMapWidth + cx];
            if (gloom < kCoveredGloom)
            {
                continue;
            }
            int level = rowLevel + gloom * coarse / 2;
            level = level < cloth.levels - 1 ? level : cloth.levels - 1;
            const int u = (fx >> shift) & mask;
            const int w = (fy >> shift) & mask;
            out[x] = cloth.palette[level * cloth.colours + cloth.index[(w << fr.sizeShift) + u]];
        }
    }

    const uint64_t spritesFrom = m_Clock ? m_Clock() : 0;
    for (int i = 0; i < rowCount; ++i)
    {
        const int row = firstRow + i;
        if (row < 0 || row >= m_Height)
        {
            continue;
        }
        uint16_t* out = dst + i * stride;
        for (int s = 0; s < m_PlacedCount; ++s)
        {
            const Placed& p = m_Placed[s];
            if (row < p.top || row >= p.top + p.height)
            {
                continue;
            }
            // 32-bit products: a row or column inside it times its step stays
            // under the texture size in 16.16.
            const int within = row - p.top < p.cut ? row - p.top : row - p.top + 1;
            int v = static_cast<int>((static_cast<uint32_t>(within) * static_cast<uint32_t>(p.texStep)) >> kFixShift);
            v = v < p.texSize ? v : p.texSize - 1;
            const uint8_t* texRow = p.index + v * p.texSize;
            const int from = p.left < left ? left : p.left;
            const int to = p.left + p.width > right ? right : p.left + p.width;
            const int lastTexel = p.texSize - 1;
            uint32_t u16 = static_cast<uint32_t>(from - p.left) * static_cast<uint32_t>(p.texStepX);
            for (int x = from; x < to; ++x, u16 += static_cast<uint32_t>(p.texStepX))
            {
                const Column& c = m_Columns[x];
                if (p.depth >= c.depth ||
                    (p.through == 0 ? p.depth >= c.portalDepth : (c.portal != p.through || p.depth <= c.portalDepth)))
                {
                    continue;  // behind the wall, or on the wrong side of a door, in this column
                }
                int u = static_cast<int>(u16 >> kFixShift);
                u = u < lastTexel ? u : lastTexel;
                const uint16_t pixel = p.palette[texRow[u]];
                if (pixel != kTransparent)
                {
                    out[x] = pixel;
                }
            }
        }

        // The overlay, over everything: given in screen pixels.
        Overlay o = m_Overlay;
        o.x /= m_Pixel;
        o.y /= m_Pixel;
        o.scale = o.scale >= m_Pixel ? o.scale / m_Pixel : (o.scale > 0 ? 1 : 0);
        if (o.texture && o.texture->index && o.scale > 0)
        {
            const int size = o.texture->size;
            const int v = row >= o.y ? (row - o.y) / o.scale : size;
            if (v < size)
            {
                const uint8_t* texRow = o.texture->index + v * size;
                const uint16_t* palette = o.texture->palette;
                const int from = o.x > left ? o.x : left;
                const int to = o.x + size * o.scale < right ? o.x + size * o.scale : right;
                // Each texel `scale` pixels across: count them off instead of
                // dividing for every pixel.
                int u = (from - o.x) / o.scale;
                int within = (from - o.x) - u * o.scale;
                for (int x = from; x < to; ++x)
                {
                    const uint16_t pixel = palette[texRow[u]];
                    if (pixel != kTransparent)
                    {
                        out[x] = pixel;
                    }
                    if (++within == o.scale)
                    {
                        within = 0;
                        ++u;
                    }
                }
            }
        }
    }
    if (m_Clock)
    {
        const uint64_t to = m_Clock();
        m_StageUs[kWallsStage] += floorsFrom - wallsFrom;
        m_StageUs[kFloorsStage] += spritesFrom - floorsFrom;
        m_StageUs[kSpritesStage] += to - spritesFrom;
    }
}

}  // namespace Dungeon
