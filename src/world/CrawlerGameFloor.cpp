// Making a floor: what monsters and things it gets and where they stand, its torches,
// props, traps, shop and secrets, and the Seam.

#include "CrawlerGame.h"

#include "CrawlerGameData.h"
#include "Village.h"

#include <cstdio>
#include <initializer_list>
#include <cstring>

namespace Dungeon
{

using namespace GameData;

MonsterKind CrawlerGame::PickKind()
{
    // Any kind that lives at this depth; deeper than the last list, the
    // fiercest ones.
    MonsterKind choices[kMonsterKinds];
    int count = 0;
    for (int k = 0; k < kMonsterKinds; ++k)
    {
        if (kStats[k].fromDepth > 0 && kStats[k].fromDepth <= m_Depth && m_Depth <= kStats[k].toDepth)
        {
            choices[count++] = static_cast<MonsterKind>(k);
        }
    }
    if (count == 0)
    {
        return MonsterKind::Rockstack;
    }
    return choices[m_Random.Range(0, count - 1)];
}

ItemKind CrawlerGame::PickLoot(bool chest)
{
    struct Weight
    {
        ItemKind kind;
        int weight;
    };
    static constexpr Weight kFloor[] = { { ItemKind::Gold, 18 },    { ItemKind::Vial, 4 },   { ItemKind::Snack, 4 },
                                         { ItemKind::Lockpick, 3 }, { ItemKind::Gem, 1 },    { ItemKind::Potion, 20 },
                                         { ItemKind::Chest, 14 },   { ItemKind::Bomb, 12 },  { ItemKind::Scroll, 6 },
                                         { ItemKind::Wing, 4 },     { ItemKind::Sword, 4 },  { ItemKind::Shield, 4 },
                                         { ItemKind::Heart, 2 },    { ItemKind::Elixir, 2 }, { ItemKind::Axe, 1 },
                                         { ItemKind::Spear, 1 } };
    static constexpr Weight kChest[] = { { ItemKind::Gold, 6 },    { ItemKind::Vial, 4 },   { ItemKind::Snack, 4 },
                                         { ItemKind::Gem, 4 },     { ItemKind::Goblet, 3 }, { ItemKind::Lockpick, 2 },
                                         { ItemKind::Potion, 16 }, { ItemKind::Bomb, 12 },  { ItemKind::Scroll, 8 },
                                         { ItemKind::Key, 6 },     { ItemKind::Wing, 6 },   { ItemKind::Elixir, 6 },
                                         { ItemKind::Sword, 6 },   { ItemKind::Shield, 6 }, { ItemKind::Heart, 4 },
                                         { ItemKind::Axe, 3 },     { ItemKind::Spear, 3 },  { ItemKind::Dagger, 2 },
                                         { ItemKind::Hammer, 2 } };
    const Weight* table = chest ? kChest : kFloor;
    const int count = chest ? static_cast<int>(sizeof(kChest) / sizeof(kChest[0]))
                            : static_cast<int>(sizeof(kFloor) / sizeof(kFloor[0]));
    int roll = m_Random.Range(0, 99);
    for (int i = 0; i < count; ++i)
    {
        if (roll < table[i].weight)
        {
            return Unlocked(table[i].kind);
        }
        roll -= table[i].weight;
    }
    return ItemKind::Gold;
}

bool CrawlerGame::PlaceMonster(MonsterKind kind, Cell at)
{
    for (Monster& m : m_Monsters)
    {
        if (m.alive)
        {
            continue;
        }
        m = Monster{};
        m.kind = kind;
        m.at = at;
        m.from = at;
        m.hp = StatsOf(kind).hp * 5 / 4 + m_Depth / 2;
        if (IsBoss(kind))
        {
            // Bosses come round again every fifteen floors, tougher.
            m.hp += (m_Depth / 15) * 25;
        }
        if (kind == MonsterKind::Ghost)
        {
            m.hp += 2 * m_Depth;
        }
        m.phase = m_Random.Range(0, kAngleSteps - 1);
        m.maxHp = m.hp;
        m.alive = true;
        return true;
    }
    return false;
}

bool CrawlerGame::PlaceItem(ItemKind kind, Cell at)
{
    for (Item& item : m_Items)
    {
        if (!item.taken)
        {
            continue;
        }
        item.kind = kind;
        item.at = at;
        item.taken = false;
        return true;
    }
    return false;
}

bool CrawlerGame::InTheWay(Cell c) const
{
    // In a room, and not the tile a corridor opens onto (the one way through
    // would be shut, or look it), nor the tile before a cracked wall or a
    // locked door.
    const auto inRoom = [&](Cell at)
    {
        for (int i = 0; i < m_Map.GetRoomCount(); ++i)
        {
            if (m_Map.GetRoom(i).Contains(at))
            {
                return true;
            }
        }
        return (m_Map.GetSecretRoom().present && m_Map.GetSecretRoom().room.Contains(at)) ||
               (m_Map.GetVault().present && m_Map.GetVault().room.Contains(at));
    };
    if (!inRoom(c))
    {
        return true;
    }
    for (int f = 0; f < 4; ++f)
    {
        const Cell n = DungeonMap::Step(c, f);
        const Tile tile = m_Map.At(n);
        if (tile == Tile::SecretWall || tile == Tile::Door || (m_Map.IsWalkable(n) && !inRoom(n)))
        {
            return true;
        }
    }
    return false;
}

bool CrawlerGame::CutsNothingOff(Cell c)
{
    // Features and props already standing are walls for walking. With them
    // as walls, blocking `c` too must cost the start only that one tile.
    Cell standing[kMaxFeatures + kMaxProps];
    int count = 0;
    for (const Feature& f : m_Features)
    {
        if (f.present)
        {
            standing[count++] = f.at;
        }
    }
    for (const Prop& p : m_Props)
    {
        if (p.present)
        {
            standing[count++] = p.at;
        }
    }
    const auto reachable = [&]()
    {
        m_Map.ComputeDistances(m_Map.GetStart());
        int reached = 0;
        for (int y = 0; y < kMapHeight; ++y)
        {
            for (int x = 0; x < kMapWidth; ++x)
            {
                reached += m_Map.DistanceAt({ x, y }) != DungeonMap::kUnreached ? 1 : 0;
            }
        }
        return reached;
    };
    for (int i = 0; i < count; ++i)
    {
        m_Map.Set(standing[i].x, standing[i].y, Tile::Wall);
    }
    const int before = reachable();
    m_Map.Set(c.x, c.y, Tile::Wall);
    const int after = reachable();
    m_Map.Set(c.x, c.y, Tile::Floor);
    for (int i = 0; i < count; ++i)
    {
        m_Map.Set(standing[i].x, standing[i].y, Tile::Floor);
    }
    m_Map.ComputeDistances(m_Hero);
    return after == before - 1;
}

void CrawlerGame::PlaceTorches()
{
    // On room walls: a floor tile with a plain wall beside it, the torch a
    // little out from that wall, never two on one wall tile.
    const int wanted = m_Random.Range(4, kMaxTorches);
    int placed = 0;
    Cell used[kMaxTorches];
    for (int tries = 0; tries < 80 && placed < wanted; ++tries)
    {
        const Room& room = m_Map.GetRoom(m_Random.Range(0, m_Map.GetRoomCount() - 1));
        const Cell c{ m_Random.Range(room.x, room.x + room.w - 1), m_Random.Range(room.y, room.y + room.h - 1) };
        const int f = m_Random.Range(0, 3);
        const Cell wall = DungeonMap::Step(c, f);
        if (!m_Map.IsWalkable(c) || m_Map.At(wall) != Tile::Wall)
        {
            continue;
        }
        bool taken = false;
        for (int i = 0; i < placed; ++i)
        {
            taken = taken || used[i] == wall;
        }
        if (taken)
        {
            continue;
        }
        constexpr int kDx[4] = { 1, 0, -1, 0 };
        constexpr int kDy[4] = { 0, 1, 0, -1 };
        used[placed] = wall;
        Torch& t = m_Torches[placed++];
        t.x = Centre(c.x) + kDx[f] * (kFixOne * 40 / 100);
        t.y = Centre(c.y) + kDy[f] * (kFixOne * 40 / 100);
        t.facing = static_cast<uint8_t>((f + 2) & 3);  // out from its wall
        t.present = true;
    }
}

void CrawlerGame::PlaceProps()
{
    // Along room walls (a wall beside them), not the start's room, never
    // cutting a way off. Spread over the rooms: a small room takes one
    // thing, a big one two, and never the same thing twice in a room or
    // more than twice on a floor.
    const int rooms = m_Map.GetRoomCount();
    if (rooms < 2)
    {
        return;
    }
    int inRoom[kMaxRooms] = {};
    uint32_t kindsInRoom[kMaxRooms] = {};
    int ofKind[kZoneProps + kCommonProps] = {};
    const auto roomTakes = [&](int room)
    {
        const Room& r = m_Map.GetRoom(room);
        return r.w * r.h >= 24 ? 2 : 1;
    };
    // The rooms in a shuffled order, gone round and round.
    int order[kMaxRooms];
    for (int i = 1; i < rooms; ++i)
    {
        order[i - 1] = i;
    }
    for (int i = rooms - 2; i > 0; --i)
    {
        const int j = m_Random.Range(0, i);
        const int swap = order[i];
        order[i] = order[j];
        order[j] = swap;
    }
    int next = 0;
    const auto place = [&](int kind, int extra)
    {
        for (int tries = 0; tries < rooms - 1; ++tries)
        {
            const int room = order[next];
            next = (next + 1) % (rooms - 1);
            if (inRoom[room] < roomTakes(room) + extra && (kindsInRoom[room] & (1u << kind)) == 0 &&
                TryPlaceProp(kind, room))
            {
                ++inRoom[room];
                kindsInRoom[room] |= 1u << kind;
                ++ofKind[kind];
                return room;
            }
        }
        return -1;
    };
    const int wanted = m_Random.Range(3, 5);
    for (int i = 0; i < wanted; ++i)
    {
        int kind = m_Random.Range(0, kZoneProps - 1);
        for (int k = 0; k < kZoneProps && ofKind[kind] >= 2; ++k)
        {
            kind = (kind + 1) % kZoneProps;
        }
        place(kind, 0);
    }
    // Now and then something anyone could have left.
    constexpr int kChance[3] = { 35, 25, 30 };  // bookshelf, camp, locked chest
    for (int common = 0; common < 3; ++common)
    {
        if (m_Random.Chance(kChance[common]))
        {
            place(kZoneProps + common, 1);
        }
    }
    // A cage with something in it, a chest nobody locked.
    if (m_Random.Chance(20))
    {
        place(kCageKind, 1);
    }
    if (m_Random.Chance(30))
    {
        place(kOldChestKind, 1);
    }
    if (m_Random.Chance(15))
    {
        place(kMirrorKind, 1);
    }
    // Things to smash: pots stand in twos and threes in a corner of one or
    // two rooms, a crate or two alone somewhere else.
    const int potRooms = m_Random.Range(1, 2);
    for (int i = 0; i < potRooms; ++i)
    {
        const int room = place(kPotsKind, 1);
        if (room < 0)
        {
            continue;
        }
        Cell first{ -1, -1 };
        for (const Prop& p : m_Props)
        {
            if (p.present && !p.any && p.kind == kPotsKind && m_Map.GetRoom(room).Contains(p.at))
            {
                first = p.at;
            }
        }
        const int more = m_Random.Range(1, 2);
        for (int k = 0; k < more; ++k)
        {
            TryPlaceProp(kPotsKind, room, first);
        }
    }
    const int crates = m_Random.Range(1, 2);
    for (int i = 0; i < crates; ++i)
    {
        place(kCrateKind, 1);
    }
    // And a room or two that is something: a library, a crypt, a store.
    if (m_Random.Chance(65))
    {
        PlaceThemedRoom();
    }
    if (m_Random.Chance(25))
    {
        PlaceThemedRoom();
    }
}

bool CrawlerGame::PlaceChest(Cell at)
{
    int slot = -1;
    for (int i = 0; i < kMaxProps && slot < 0; ++i)
    {
        slot = m_Props[i].present ? -1 : i;
    }
    if (slot < 0 || at == m_Hero || !IsEmptyFloor(at) || InTheWay(at))
    {
        return false;
    }
    // Not right beside another thing: each stands on its own.
    for (int f = 0; f < 4; ++f)
    {
        if (PropAt(DungeonMap::Step(at, f)) >= 0)
        {
            return false;
        }
    }
    m_Map.ComputeDistances(m_Map.GetStart());
    if (m_Map.DistanceAt(at) == DungeonMap::kUnreached || !CutsNothingOff(at))
    {
        return false;
    }
    Prop& prop = m_Props[slot];
    prop = Prop{};
    prop.at = at;
    prop.kind = static_cast<uint8_t>(kOldChestKind);
    // Its front to the open floor.
    for (int f = 0; f < 4; ++f)
    {
        if (m_Map.IsWalkable(DungeonMap::Step(at, f)))
        {
            prop.facing = static_cast<uint8_t>(f);
            break;
        }
    }
    prop.present = true;
    SetUpProp(prop);
    return true;
}

void CrawlerGame::PlacePillars()
{
    // A pillar a step in from each corner of some big rooms, where it cuts
    // nothing off: a hall, not a box.
    for (int i = 1; i < m_Map.GetRoomCount(); ++i)
    {
        const Room& r = m_Map.GetRoom(i);
        if (r.w < 5 || r.h < 5 || !m_Random.Chance(45))
        {
            continue;
        }
        for (const Cell c : { Cell{ r.x + 1, r.y + 1 }, Cell{ r.x + r.w - 2, r.y + 1 }, Cell{ r.x + 1, r.y + r.h - 2 },
                              Cell{ r.x + r.w - 2, r.y + r.h - 2 } })
        {
            m_Map.ComputeDistances(m_Map.GetStart());
            if (IsEmptyFloor(c) && c != m_Map.GetStairs() && m_Map.DistanceAt(c) != DungeonMap::kUnreached &&
                CutsNothingOff(c))
            {
                m_Map.Set(c.x, c.y, Tile::Wall);
            }
        }
    }
}

void CrawlerGame::PlaceThemedRoom()
{
    // What a room can be, by the things in it (their places in the props'
    // table): the sealed ones went on living down here.
    struct Theme
    {
        uint8_t things[5];
        uint8_t count;
    };
    constexpr Theme kThemes[] = {
        { { kBookshelf, kBookshelf, kBookshelf }, 3 },                   // a library
        { { kCrate, kCrate, kAleBarrel, kPots, kPots }, 5 },             // a storeroom
        { { kCoffin, kCoffin, kGravestone, kGravestone, kShrine }, 5 },  // a crypt
        { { kShrine, kShrine, kSentinel, kSentinel }, 4 },               // a chapel
        { { kWeaponRack, kAnvil, kSentinel }, 3 },                       // an armoury
        { { kCamp, kPack, kAleBarrel }, 3 },                             // a camp
        { { kBush, kBush, kMushrooms, kMushrooms }, 4 },                 // a garden
        { { kOldChest, kOldChest, kSentinel }, 3 },                      // a treasury
    };
    constexpr int kThemeCount = static_cast<int>(sizeof(kThemes) / sizeof(kThemes[0]));
    const int rooms = m_Map.GetRoomCount();
    for (int tries = 0; tries < 12; ++tries)
    {
        const int room = m_Random.Range(1, rooms - 1);
        const Room& r = m_Map.GetRoom(room);
        if (rooms < 2 || r.w * r.h < 12)
        {
            continue;
        }
        const Theme& theme = kThemes[m_Random.Range(0, kThemeCount - 1)];
        for (int i = 0; i < theme.count; ++i)
        {
            TryPlaceProp(theme.things[i], room, { -1, -1 }, true);
        }
        return;
    }
}

bool CrawlerGame::TryPlaceProp(int kind, int roomIndex, Cell near, bool any)
{
    int slot = -1;
    for (int i = 0; i < kMaxProps && slot < 0; ++i)
    {
        slot = m_Props[i].present ? -1 : i;
    }
    if (slot < 0 || roomIndex < 0 || roomIndex >= m_Map.GetRoomCount())
    {
        return false;
    }
    const Room& room = m_Map.GetRoom(roomIndex);
    const bool beside = near.x >= 0;
    for (int tries = 0; tries < 20; ++tries)
    {
        // Anywhere in the room, or one of the tiles round `near`.
        const Cell c =
            beside ? Cell{ near.x + m_Random.Range(-1, 1), near.y + m_Random.Range(-1, 1) }
                   : Cell{ m_Random.Range(room.x, room.x + room.w - 1), m_Random.Range(room.y, room.y + room.h - 1) };
        bool byWall = false;
        bool crowded = false;
        for (int f = 0; f < 4; ++f)
        {
            const Cell n = DungeonMap::Step(c, f);
            byWall = byWall || m_Map.At(n) == Tile::Wall;
            // Things stand apart, but for pots huddled together.
            const int neighbour = PropAt(n);
            crowded =
                crowded || (neighbour >= 0 && !(beside && !m_Props[neighbour].any && m_Props[neighbour].kind == kind));
        }
        m_Map.ComputeDistances(m_Map.GetStart());
        if (!room.Contains(c) || !byWall || crowded || !IsEmptyFloor(c) || InTheWay(c) ||
            m_Map.DistanceAt(c) == DungeonMap::kUnreached || !CutsNothingOff(c))
        {
            continue;
        }
        Prop& prop = m_Props[slot];
        prop.at = c;
        prop.kind = static_cast<uint8_t>(kind);
        // Its back to a wall, facing into the room.
        for (int f = 0; f < 4; ++f)
        {
            if (m_Map.At(DungeonMap::Step(c, (f + 2) & 3)) == Tile::Wall)
            {
                prop.facing = static_cast<uint8_t>(f);
                break;
            }
        }
        prop.present = true;
        prop.fixed = false;
        prop.any = any;
        SetUpProp(prop);
        return true;
    }
    return false;
}

bool CrawlerGame::PlaceFeature(FeatureKind kind)
{
    int slot = -1;
    for (int i = 0; i < kMaxFeatures; ++i)
    {
        if (!m_Features[i].present)
        {
            slot = i;
            break;
        }
    }
    if (slot < 0 || m_Map.GetRoomCount() < 2)
    {
        return false;
    }
    m_Map.ComputeDistances(m_Map.GetStart());
    for (int tries = 0; tries < 60; ++tries)
    {
        const Room& room = m_Map.GetRoom(m_Random.Range(1, m_Map.GetRoomCount() - 1));
        const Cell c{ m_Random.Range(room.x, room.x + room.w - 1), m_Random.Range(room.y, room.y + room.h - 1) };
        // Puddle sets up against a wall, for the first tries at least; no
        // one stands in a doorway or a corridor.
        bool byWall = kind != FeatureKind::Merchant || tries >= 25;
        for (int f = 0; f < 4 && !byWall; ++f)
        {
            byWall = m_Map.At(DungeonMap::Step(c, f)) == Tile::Wall;
        }
        if (!byWall || !IsEmptyFloor(c) || InTheWay(c) || m_Map.DistanceAt(c) == DungeonMap::kUnreached ||
            !CutsNothingOff(c))
        {
            m_Map.ComputeDistances(m_Map.GetStart());
            continue;
        }
        Feature& f = m_Features[slot];
        f.kind = kind;
        f.at = c;
        f.present = true;
        f.used = false;
        return true;
    }
    return false;
}

void CrawlerGame::PlaceTraps()
{
    if (m_Depth < 2)
    {
        return;
    }
    int wanted = m_Random.Range(0, 1 + m_Depth / 4);
    wanted = wanted < 4 ? wanted : 4;
    const Hideout& secret = m_Map.GetSecretRoom();
    const Hideout& vault = m_Map.GetVault();
    for (int i = 0; i < wanted; ++i)
    {
        Cell at;
        const auto good = [&](Cell c)
        {
            const Hideout& pocket = m_Map.GetPocket();
            return IsEmptyFloor(c) && !(secret.present && secret.room.Contains(c)) &&
                   !(vault.present && vault.room.Contains(c)) && !(pocket.present && pocket.room.Contains(c));
        };
        if (!m_Map.RandomFloor(m_Random, good, at))
        {
            return;
        }
        // Mostly spikes; deeper, a floor that gives way, one that folds, a
        // wire with a bell on it.
        const int roll = m_Random.Range(0, 99);
        m_Traps[at.y][at.x] = m_Depth >= 3 && roll < 22   ? kTrapdoor
                              : m_Depth >= 3 && roll < 40 ? kTeleport
                              : roll < 58                 ? kAlarm
                                                          : kSpikes;
    }
}

void CrawlerGame::StockShop()
{
    m_GoodsCount = 0;
    const auto add = [&](ItemKind kind)
    {
        for (int i = 0; i < m_GoodsCount; ++i)
        {
            if (m_Goods[i].item == kind)
            {
                return;
            }
        }
        if (m_GoodsCount < 4)
        {
            MenuEntry& e = m_Goods[m_GoodsCount++];
            e.item = kind;
            e.price = PriceOf(kind, m_Depth);
            e.count = kind == ItemKind::Sword || kind == ItemKind::Shield || kind == ItemKind::Key || IsWeaponItem(kind)
                          ? 1
                          : 3;
        }
    };
    add(ItemKind::Potion);
    add(ItemKind::Bomb);
    // A vault on this floor: Puddle has heard, and has a key.
    bool keyOnFloor = false;
    for (const Item& item : m_Items)
    {
        keyOnFloor = keyOnFloor || (!item.taken && item.kind == ItemKind::Key);
    }
    if (m_Map.GetVault().present && !keyOnFloor)
    {
        add(ItemKind::Key);
    }
    constexpr ItemKind kOthers[] = { ItemKind::Lockpick, ItemKind::Scroll, ItemKind::Wing,  ItemKind::Elixir,
                                     ItemKind::Sword,    ItemKind::Shield, ItemKind::Key,   ItemKind::Axe,
                                     ItemKind::Spear,    ItemKind::Dagger, ItemKind::Hammer };
    constexpr int kOtherCount = static_cast<int>(sizeof(kOthers) / sizeof(kOthers[0]));
    for (int tries = 0; tries < 20 && m_GoodsCount < 4; ++tries)
    {
        const ItemKind kind = Unlocked(kOthers[m_Random.Range(0, kOtherCount - 1)]);
        if (!IsWeaponItem(kind) || kind != WeaponItem(m_Weapon))
        {
            add(kind);
        }
    }
}

bool CrawlerGame::BossAlive() const
{
    for (const Monster& m : m_Monsters)
    {
        if (m.alive && IsBoss(m.kind))
        {
            return true;
        }
    }
    return false;
}

void CrawlerGame::EnterFloor(int depth)
{
    m_Depth = depth;
    if (depth > m_Deepest)
    {
        m_Deepest = depth;
    }
    // The Seam is another map: the same floor, folded.
    m_Map.Generate(m_InSeam ? m_Seed ^ 0x5EA3u : m_Seed, depth);
    m_EchoOnFloor = -1;
    m_WallBlows = 0;
    m_HollowSaid = false;
    m_ShardHere = false;
    m_Hero = m_Map.GetStart();
    m_HeroFrom = m_Hero;
    m_GlideMs = 0;
    m_KnowsSecrets = false;
    m_FallPending = false;
    m_Menu = Menu::None;
    // Facing the way out of the start, not a wall.
    m_Facing = 0;
    for (int f = 0; f < 4; ++f)
    {
        if (m_Map.IsWalkable(Ahead(m_Hero, f, 1)))
        {
            m_Facing = f;
            break;
        }
    }
    m_FacingFrom = m_Facing;

    for (Monster& m : m_Monsters)
    {
        m = Monster{};
    }
    for (Item& item : m_Items)
    {
        item = Item{};
    }
    for (Feature& f : m_Features)
    {
        f = Feature{};
    }
    for (Prop& p : m_Props)
    {
        p = Prop{};
    }
    for (Torch& t : m_Torches)
    {
        t = Torch{};
    }
    for (auto& row : m_Traps)
    {
        for (uint8_t& trap : row)
        {
            trap = 0;
        }
    }

    if (m_InSeam)
    {
        // The Seam: the Keeper by the stairs, torches, nothing else.
        m_GoodsCount = 0;
        const Cell stairs = m_Map.GetStairs();
        bool placed = false;
        for (int f = 0; f < 4 && !placed; ++f)
        {
            const Cell c = Ahead(stairs, f, 1);
            placed = IsFree(c) && PlaceMonster(MonsterKind::Keeper, c);
        }
        if (!placed)
        {
            PlaceMonster(MonsterKind::Keeper, stairs);
        }
        PlaceTorches();
        m_Map.ComputeView(m_Hero, 6);
        Say("Keeper: That name. That was mine. Who told you?");
        return;
    }

    // The hideouts' treasure first, so it always has room.
    const Hideout& secret = m_Map.GetSecretRoom();
    const Hideout& vault = m_Map.GetVault();
    if (secret.present)
    {
        const Room& r = secret.room;
        if (!PlaceChest({ r.x, r.y }))
        {
            PlaceItem(ItemKind::Gold, { r.x, r.y });
        }
        const ItemKind second = m_PagesKnown < kLorePages ? ItemKind::Page
                                : m_Random.Chance(30)     ? ItemKind::Heart
                                                          : ItemKind::Gold;
        PlaceItem(second, { r.x + r.w - 1, r.y + r.h - 1 });
    }
    if (vault.present)
    {
        const Room& r = vault.room;
        for (const Cell corner : { Cell{ r.x, r.y }, Cell{ r.x + r.w - 1, r.y } })
        {
            if (!PlaceChest(corner))
            {
                PlaceItem(ItemKind::Gold, corner);
            }
        }
        PlaceItem(ItemKind::Elixir, { r.x, r.y + r.h - 1 });
    }

    // The hall behind the impossible door: treasure, a page, and guards.
    const Hideout& pocket = m_Map.GetPocket();
    if (pocket.present)
    {
        const Room& r = pocket.room;
        const Cell far{ pocket.entrance.x < r.x + r.w / 2 ? r.x + r.w - 1 : r.x,
                        pocket.entrance.y < r.y + r.h / 2 ? r.y + r.h - 1 : r.y };
        if (!PlaceChest(far))
        {
            PlaceItem(ItemKind::Gold, far);
        }
        PlaceItem(m_PagesKnown < kLorePages ? ItemKind::Page : ItemKind::Elixir, { r.x + r.w / 2, r.y + r.h / 2 });
        PlaceItem(ItemKind::Gold, { far.x == r.x ? r.x + 1 : r.x + r.w - 2, far.y });
    }

    PlaceLore();

    // Features: Puddle with his goods, a fountain.
    if (depth >= 2 && (m_Random.Chance(45) || depth % 5 == 1))
    {
        PlaceFeature(FeatureKind::Merchant);
    }
    if (depth >= 2 && m_Random.Chance(35))
    {
        PlaceFeature(FeatureKind::Fountain);
    }
    if (depth >= 3 && m_Random.Chance(30))
    {
        PlaceFeature(FeatureKind::Shrine);
    }
    m_Map.ComputeDistances(m_Hero);
    PlacePillars();
    PlaceProps();
    // A loose brick now and then, and a note that may tell of a secret.
    if (m_Random.Chance(55))
    {
        PlaceLooseBrick();
    }
    if (m_Random.Chance(45))
    {
        Cell at;
        const auto empty = [&](Cell c) { return IsEmptyFloor(c) && m_Map.DistanceAt(c) != DungeonMap::kUnreached; };
        if (m_Map.RandomFloor(m_Random, empty, at))
        {
            PlaceItem(ItemKind::Note, at);
        }
    }
    // An echo stone now and then; always on the very first floor ever.
    if (m_Legend.echoes == 0 || m_Random.Chance(55))
    {
        PlaceEchoStone();
    }
    PlaceTorches();

    // Monsters: more and tougher the deeper, never next to the start.
    m_Map.ComputeDistances(m_Hero);
    const auto farEnough = [&](Cell c)
    {
        const uint16_t d = m_Map.DistanceAt(c);
        return d != DungeonMap::kUnreached && d > 3 && IsFree(c);
    };
    // The boss guards the stairs.
    if (IsBossFloor())
    {
        constexpr MonsterKind kBosses[3] = { MonsterKind::Mimic, MonsterKind::Grubking, MonsterKind::Hydragon };
        const MonsterKind boss = kBosses[(depth / 5 - 1) % 3];
        const Cell stairs = m_Map.GetStairs();
        for (int f = 0; f < 4; ++f)
        {
            const Cell c = Ahead(stairs, f, 1);
            if (IsFree(c) && PlaceMonster(boss, c))
            {
                break;
            }
        }
    }
    // Two guards in the folded hall, away from where you arrive.
    if (pocket.present)
    {
        for (int i = 0, tries = 0; i < 2 && tries < 30; ++tries)
        {
            const Room& r = pocket.room;
            const Cell c{ m_Random.Range(r.x, r.x + r.w - 1), m_Random.Range(r.y, r.y + r.h - 1) };
            const int dx = c.x - pocket.entrance.x;
            const int dy = c.y - pocket.entrance.y;
            if (dx * dx + dy * dy >= 4 && IsFree(c) && PlaceMonster(PickKind(), c))
            {
                ++i;
            }
        }
    }
    const int monsterCount = 3 + depth < kMaxMonsters - 4 ? 3 + depth : kMaxMonsters - 4;
    for (int i = 0; i < monsterCount; ++i)
    {
        Cell at;
        if (!m_Map.RandomFloor(m_Random, farEnough, at) || !PlaceMonster(PickKind(), at))
        {
            break;
        }
    }
    PlaceNamed();
    // Now and then a glimmerjelly: rare, quick to flee, worth a lot.
    if (depth >= 2 && m_Random.Chance(14))
    {
        Cell at;
        if (m_Map.RandomFloor(m_Random, farEnough, at))
        {
            PlaceMonster(MonsterKind::Glimmerjelly, at);
        }
    }
    // Where the last run died, its ghost waits, with its gold.
    bool ghostHere = false;
    if (m_Legend.ghostDepth == depth)
    {
        Cell at;
        if (m_Map.RandomFloor(m_Random, farEnough, at))
        {
            ghostHere = PlaceMonster(MonsterKind::Ghost, at);
        }
    }

    // Loose things on the floor.
    const int itemCount = m_Random.Range(2, 4);
    for (int i = 0; i < itemCount; ++i)
    {
        Cell at;
        const auto empty = [&](Cell c) { return IsEmptyFloor(c) && m_Map.DistanceAt(c) != DungeonMap::kUnreached; };
        if (!m_Map.RandomFloor(m_Random, empty, at))
        {
            break;
        }
        // A chest is a thing that stands there, not something to pocket.
        const ItemKind loot = PickLoot(false);
        if (loot != ItemKind::Chest || !PlaceChest(at))
        {
            PlaceItem(loot == ItemKind::Chest ? ItemKind::Gold : loot, at);
        }
    }
    // Now and then a diary page lies about.
    if (m_PagesKnown < kLorePages && m_Random.Chance(30))
    {
        Cell at;
        const auto empty = [&](Cell c) { return IsEmptyFloor(c) && m_Map.DistanceAt(c) != DungeonMap::kUnreached; };
        if (m_Map.RandomFloor(m_Random, empty, at))
        {
            PlaceItem(ItemKind::Page, at);
        }
    }
    // A vault's key lies somewhere on the floor, more often than not.
    if (vault.present && m_Random.Chance(60))
    {
        Cell at;
        const auto empty = [&](Cell c) { return IsEmptyFloor(c) && m_Map.DistanceAt(c) != DungeonMap::kUnreached; };
        if (m_Map.RandomFloor(m_Random, empty, at))
        {
            PlaceItem(ItemKind::Key, at);
        }
    }
    PlaceTraps();
    StockShop();

    m_Map.ComputeView(m_Hero, 6);
    char text[kMessageLength];
    const bool keeperTalks = m_Legend.lastEnding != 1;
    if (depth > 1 && (depth - 1) % kZoneFloors == 0)
    {
        // A new zone.
        std::snprintf(text, sizeof(text), "Depth %d: %s.", depth, ZoneName(GetZone()));
    }
    else if (IsBossFloor())
    {
        std::snprintf(text, sizeof(text),
                      keeperTalks ? "%s: Meet my pet. Do scream, it likes that."
                                  : "%s: Careful, a big one by the stairs!",
                      Lore::VoiceName(m_Legend));
    }
    else if (ghostHere)
    {
        std::snprintf(text, sizeof(text), "%s: Someone you know is down here...", Lore::VoiceName(m_Legend));
    }
    else if (m_ShardHere)
    {
        std::snprintf(text, sizeof(text),
                      keeperTalks ? "%s: Something of mine is on this floor. LEAVE IT."
                                  : "%s: I feel a piece of me here!",
                      keeperTalks ? Lore::VoiceName(m_Legend) : "Halvard");
    }
    else if (m_Shards > 0 && keeperTalks && m_Random.Chance(40))
    {
        std::snprintf(text, sizeof(text), "%s: Those pieces are mine. Put them down.", Lore::VoiceName(m_Legend));
    }
    else if (m_EchoOnFloor >= 0 && m_Random.Chance(50))
    {
        std::snprintf(text, sizeof(text), "A stone hums somewhere nearby...");
    }
    else if (m_Random.Chance(30))
    {
        std::snprintf(text, sizeof(text), "%s", Lore::VoiceLine(m_Legend, m_Random.Range(0, 99)));
    }
    else if (pocket.present && m_Random.Chance(60))
    {
        std::snprintf(text, sizeof(text), "The halls here feel folded...");
    }
    else if (vault.present)
    {
        std::snprintf(text, sizeof(text), "A lock rattles somewhere...");
    }
    else if (secret.present && m_Random.Chance(50))
    {
        std::snprintf(text, sizeof(text), "A cold draft blows from a wall...");
    }
    else if (m_Features[0].present && m_Features[0].kind == FeatureKind::Merchant)
    {
        std::snprintf(text, sizeof(text), "Someone is humming nearby.");
    }
    else
    {
        text[0] = 0;
    }
    if (text[0] != 0)
    {
        Say(text);
    }
    else
    {
        ++m_Version;
    }
}

bool CrawlerGame::PlaceLooseBrick()
{
    const Hideout& pocket = m_Map.GetPocket();
    for (int tries = 0; tries < 60; ++tries)
    {
        const Room& room = m_Map.GetRoom(m_Random.Range(0, m_Map.GetRoomCount() - 1));
        const Cell c{ m_Random.Range(room.x, room.x + room.w - 1), m_Random.Range(room.y, room.y + room.h - 1) };
        const Cell wall = DungeonMap::Step(c, m_Random.Range(0, 3));
        if (!m_Map.IsWalkable(c) || m_Map.At(wall) != Tile::Wall || wall.x <= 0 || wall.y <= 0 ||
            wall.x >= kMapWidth - 1 || wall.y >= kMapHeight - 1 || (pocket.present && pocket.room.Contains(c)))
        {
            continue;
        }
        m_Map.Set(wall.x, wall.y, Tile::LooseBrick);
        return true;
    }
    return false;
}

void CrawlerGame::EnterSeam()
{
    // The stone listener takes the shards; the floor folds open.
    m_Shards = 0;
    m_InSeam = true;
    m_KeeperCalled = false;
    EnterFloor(m_Depth);
}

Cell CrawlerGame::FarthestFloor()
{
    m_Map.ComputeDistances(m_Map.GetStart());
    Cell best = m_Map.GetStairs();
    uint16_t bestDistance = 0;
    for (int y = 0; y < kMapHeight; ++y)
    {
        for (int x = 0; x < kMapWidth; ++x)
        {
            const uint16_t d = m_Map.DistanceAt({ x, y });
            if (d != DungeonMap::kUnreached && d > bestDistance && IsEmptyFloor({ x, y }) &&
                m_Map.At(x, y) == Tile::Floor)
            {
                bestDistance = d;
                best = { x, y };
            }
        }
    }
    return best;
}

void CrawlerGame::PlaceLore()
{
    // A thing in a hideout (not its chests) turned into `kind`.
    const auto swapIn = [&](const Hideout& hideout, ItemKind kind)
    {
        if (!hideout.present)
        {
            return false;
        }
        for (Item& item : m_Items)
        {
            if (!item.taken && hideout.room.Contains(item.at) && item.kind != ItemKind::Chest &&
                item.kind != ItemKind::Shard && item.kind != ItemKind::Duck)
            {
                item.kind = kind;
                return true;
            }
        }
        return false;
    };
    // A heart shard on the third floor of each zone, in its best hiding
    // place: the folded hall, the secret room, or the far end.
    const int zone = GetZone();
    if (m_Depth % kZoneFloors == 3 && (m_ShardBits & (1 << zone)) == 0)
    {
        m_ShardHere = swapIn(m_Map.GetPocket(), ItemKind::Shard) || swapIn(m_Map.GetSecretRoom(), ItemKind::Shard) ||
                      PlaceItem(ItemKind::Shard, FarthestFloor());
    }
    // A rubber duck in a hideout, one in each of the last three zones.
    if (zone >= 1 && (m_Legend.ducks & (1 << (zone - 1))) == 0 && m_Random.Chance(50))
    {
        if (!swapIn(m_Map.GetSecretRoom(), ItemKind::Duck))
        {
            swapIn(m_Map.GetVault(), ItemKind::Duck);
        }
    }
}

int CrawlerGame::NextEcho() const
{
    const int zone = GetZone();
    for (int i = 0; i < Lore::kEchoesPerZone; ++i)
    {
        const int echo = zone * Lore::kEchoesPerZone + i;
        if ((m_Legend.echoes & (1u << echo)) == 0)
        {
            return echo;
        }
    }
    return -1;
}

bool CrawlerGame::PlaceEchoStone()
{
    const int echo = NextEcho();
    if (echo < 0)
    {
        return false;
    }
    const Hideout& pocket = m_Map.GetPocket();
    for (int tries = 0; tries < 60; ++tries)
    {
        const Room& room = m_Map.GetRoom(m_Random.Range(0, m_Map.GetRoomCount() - 1));
        const Cell c{ m_Random.Range(room.x, room.x + room.w - 1), m_Random.Range(room.y, room.y + room.h - 1) };
        const Cell wall = DungeonMap::Step(c, m_Random.Range(0, 3));
        // The tile in front of it stays clear, so it can be reached.
        if (!IsEmptyFloor(c) || m_Map.At(wall) != Tile::Wall || wall.x <= 0 || wall.y <= 0 || wall.x >= kMapWidth - 1 ||
            wall.y >= kMapHeight - 1 || (pocket.present && pocket.room.Contains(c)))
        {
            continue;
        }
        m_Map.Set(wall.x, wall.y, Tile::EchoWall);
        m_EchoOnFloor = echo;
        return true;
    }
    return false;
}
}  // namespace Dungeon
