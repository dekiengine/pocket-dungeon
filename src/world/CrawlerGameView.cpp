// What the screen shows of the game: what the hero faces (the prompt), where the
// eye is, and the billboards and shadows of everything standing about.

#include "CrawlerGame.h"

#include "Contracts.h"
#include "CrawlerGameData.h"
#include "Village.h"

#include <cstdio>
#include <cstring>

namespace Dungeon
{

using namespace GameData;

namespace
{

// How far the base of something `scale` of a tile tall reaches toward the
// viewer from its middle: a little under half its width, and never so far
// that a big one's feet leave the view from the next tile.
Fix FootOf(Fix scale)
{
    const Fix foot = scale * 3 / 10;
    return foot < Pct(20) ? foot : Pct(20);
}

}  // namespace

Target CrawlerGame::GetTarget() const
{
    Target t;
    const Cell ahead = Ahead(m_Hero, m_Facing, 1);
    if (m_Inside >= 0)
    {
        if (ahead == Village::InteriorDoor())
        {
            t.name = "Door";
            t.verb = "Go out";
            t.detail = "Back to the village.";
            return t;
        }
        if (ahead == Village::KeeperCell())
        {
            const int level = m_Legend.buildings[m_Inside];
            t.name = Village::KeeperName(m_Inside);
            if (level >= Village::kMaxLevel || Village::BossNeeded(m_Inside, level + 1) != 0)
            {
                t.verb = "Talk";
                t.detail = Village::KeeperAbout(m_Inside);
                return t;
            }
            const Village::Cost cost = Village::UpgradeCost(m_Inside, level + 1);
            t.needGold = cost.gold;
            t.haveGold = m_Legend.bank;
            t.material = cost.material;
            t.needMaterial = cost.amount;
            t.haveMaterial = m_Legend.materials[cost.material];
            t.gain = Village::Effect(m_Inside, level + 1);
            // With a contract to offer or pay, or too little to build with,
            // the key talks.
            const bool enough = t.haveGold >= t.needGold && t.haveMaterial >= t.needMaterial;
            t.verb = enough && !HasWord(m_Inside) ? "Upgrade" : "Talk";
            return t;
        }
    }
    else if (m_InVillage)
    {
        // A front straight ahead shows what it needs from a few steps off;
        // it is built from right before it.
        int building = Village::BuildingAt(ahead);
        bool far = false;
        for (int steps = 2; steps <= 4 && building < 0; ++steps)
        {
            const Cell before = Ahead(m_Hero, m_Facing, steps - 1);
            if (!m_Map.IsWalkable(before) || FeatureAt(before) >= 0 || PropAt(before) >= 0)
            {
                break;
            }
            building = Village::BuildingAt(Ahead(m_Hero, m_Facing, steps));
            far = building >= 0;
        }
        if (far && (m_Legend.buildings[building] >= Village::kMaxLevel ||
                    (m_Legend.buildings[building] > 0 && Village::CanEnter(building))))
        {
            building = -1;  // only what can still be raised from outside calls from afar
        }
        if (building >= 0)
        {
            const int level = m_Legend.buildings[building];
            std::snprintf(m_TargetName, sizeof(m_TargetName), level == 0 ? "%s (ruin)" : "%s  Lv %d/3",
                          Village::BuildingName(building), level);
            t.name = m_TargetName;
            if (level > 0 && Village::CanEnter(building))
            {
                t.verb = "Enter";
                t.detail = Village::Effect(building, level);
                return t;
            }
            if (level >= Village::kMaxLevel)
            {
                t.detail = Village::Effect(building, level);
                return t;
            }
            const int boss = Village::BossNeeded(building, level + 1);
            if (boss != 0 && (m_Legend.bosses & boss) == 0)
            {
                t.detail = "The beast below must fall first.";
                return t;
            }
            const Village::Cost cost = Village::UpgradeCost(building, level + 1);
            t.needGold = cost.gold;
            t.haveGold = m_Legend.bank;
            t.material = cost.material;
            t.needMaterial = cost.amount;
            t.haveMaterial = m_Legend.materials[cost.material];
            t.gain = Village::Effect(building, level + 1);
            const bool enough = t.haveGold >= t.needGold && t.haveMaterial >= t.needMaterial;
            t.verb = far || !enough ? nullptr : (level == 0 ? "Build" : "Upgrade");
            return t;
        }
        const int lift = m_Legend.buildings[static_cast<int>(Village::Building::Lift)];
        if (ahead == Village::LiftCell() && lift > 0)
        {
            std::snprintf(m_TargetDetail, sizeof(m_TargetDetail), "Down to depth %d.", Village::LiftDepth(lift));
            t.name = "Mine lift";
            t.verb = "Ride down";
            t.detail = m_TargetDetail;
            return t;
        }
        if (ahead == m_Map.GetStairs())
        {
            t.name = "The old well";
            t.verb = "Climb down";
            t.detail = "The dungeon. Depth 1.";
            return t;
        }
    }
    const int monster = MonsterAt(ahead);
    if (monster >= 0)
    {
        const Monster& m = m_Monsters[monster];
        t.name = m.named ? "Orm the Thirsty" : StatsOf(m.kind).name;
        t.verb = "Attack";
        t.hp = m.hp;
        t.maxHp = m.maxHp;
        return t;
    }
    const int feature = FeatureAt(ahead);
    if (feature >= 0)
    {
        const Feature& f = m_Features[feature];
        switch (f.kind)
        {
            case FeatureKind::Fountain:
                t.name = kFountainName;
                t.verb = f.used ? nullptr : "Look closer";
                t.detail = f.used ? "The basin is dry." : kFountainAbout;
                break;
            case FeatureKind::Merchant:
                t.name = "Puddle";
                t.verb = "Trade";
                t.detail = "Goods for gold. Snacks welcome.";
                break;
            case FeatureKind::Granny:
            case FeatureKind::Kid:
            {
                const int v = f.kind == FeatureKind::Granny ? 0 : 1;
                t.name = Village::VillagerName(v);
                t.verb = "Talk";
                t.detail = Village::VillagerAbout(v);
                break;
            }
            case FeatureKind::PortalHome:
                t.name = "Portal home";
                t.verb = "Go home";
                t.detail = "Bank your gold and materials.";
                break;
            case FeatureKind::Shrine:
            {
                const bool more = !f.used || NextName() >= 0 || m_Shards > 0;
                t.name = kStoneName;
                t.verb = more ? "Look closer" : nullptr;
                t.detail = more ? kStoneAbout : kNothingMore;
                break;
            }
            case FeatureKind::Keeper:
                t.name = Village::KeeperName(m_Inside);
                t.verb = "Talk";
                t.detail = Village::KeeperAbout(m_Inside);
                break;
            case FeatureKind::Fence:
                t.name = Village::FenceName();
                t.verb = "Trade";
                t.detail = Village::FenceAbout();
                break;
            case FeatureKind::Board:
                t.name = "Notice board";
                t.verb = "Read";
                t.detail = "Who wants what in Last Hearth.";
                break;
        }
        return t;
    }
    const int prop = PropAt(ahead);
    if (prop >= 0)
    {
        const Prop& p = m_Props[prop];
        const int kind = PropIndex(p);
        t.name = kPropInfo[kind].name;
        if (p.fixed)
        {
            // A furnishing: to look at. A stone in the yard: to read.
            t.detail = kPropInfo[kind].about;
            t.verb = m_InVillage && m_Inside < 0 && kind == kGravestone ? "Read" : nullptr;
            return t;
        }
        // E opens its choices.
        const bool more = HasDeeds(p);
        t.verb = more ? "Look closer" : nullptr;
        t.detail = more ? kPropInfo[kind].about : kNothingMore;
        return t;
    }
    for (const Item& item : m_Items)
    {
        if (!item.taken && item.at == ahead)
        {
            t.name = ItemName(item.kind);
            t.verb = item.kind == ItemKind::Chest ? "Open" : "Take";
            t.detail = item.kind == ItemKind::Chest ? "What's inside?" : ItemHelp(item.kind);
            return t;
        }
    }
    if (m_Map.PortalEntering(ahead, m_Facing) != nullptr)
    {
        t.name = "Lone doorway";
        t.verb = "Step in";
        t.detail = "It shouldn't lead anywhere...";
        return t;
    }
    switch (m_Map.At(ahead))
    {
        case Tile::Door:
            t.name = "Vault door";
            t.verb = "Unlock";
            t.detail = Carried(ItemKind::Key) > 0 ? "You have a key." : "Locked. It needs a key.";
            break;
        case Tile::SecretWall:
        {
            const int blows = m_KnockAt == ahead ? m_WallBlows : 0;
            t.name = "Cracked wall";
            t.verb = "Strike";
            t.detail = blows == 0 ? "It sounds hollow." : (blows < 3 ? "Chipped. Keep at it." : "It is giving way.");
            break;
        }
        case Tile::EchoWall:
            t.name = "Echo stone";
            t.verb = "Listen";
            t.detail = "Old voices are caught in it.";
            break;
        case Tile::LooseBrick:
            t.name = "Loose brick";
            t.verb = "Pull";
            t.detail = "One brick sticks out a little.";
            break;
        case Tile::Stairs:
            t.name = m_InSeam ? "The bottom" : "Way down";
            t.verb = m_InSeam ? nullptr : "Descend";
            t.detail = m_InSeam ? "There is no down from here." : "Deeper, darker, better loot.";
            break;
        default: break;
    }
    return t;
}

void CrawlerGame::GetView(Fix& x, Fix& y, int& angle) const
{
    // 0 at the start of the glide, 1024 at its end.
    const int t = GlideProgress();
    x = Lerp(Centre(m_HeroFrom.x), Centre(m_Hero.x), t);
    y = Lerp(Centre(m_HeroFrom.y), Centre(m_Hero.y), t);
    if (m_Through && m_GlideMs > 0)
    {
        // Through an impossible door: the first half of the step on this
        // side, toward the doorway; past its middle line the same place seen
        // from the other side, turned with it. The picture never jumps, as
        // the rays already go through.
        const Portal& p = m_ThroughPortal;
        const int turns = (p.toDir - p.dir) & 3;
        const bool across = t > 512;
        const int tt = across ? (t < 516 ? 516 : t) : (t > 508 ? 508 : t);
        Fix sx = Lerp(Centre(m_HeroFrom.x), Centre(p.at.x), tt);
        Fix sy = Lerp(Centre(m_HeroFrom.y), Centre(p.at.y), tt);
        int facing = m_FacingFrom;
        if (across)
        {
            Fix lx = sx - Centre(p.at.x);
            Fix ly = sy - Centre(p.at.y);
            for (int k = 0; k < turns; ++k)
            {
                const Fix nx = -ly;
                ly = lx;
                lx = nx;
            }
            sx = Centre(p.to.x) + lx;
            sy = Centre(p.to.y) + ly;
            facing = (facing + turns) & 3;
        }
        angle = facing * kAngleSteps / 4;
        // The eye's step back (below) shrinks to nothing at the doorway's
        // middle line, so it is never behind the place it arrives at.
        const Fix back = kEyeBack * (tt > 512 ? tt - 512 : 512 - tt) / 512;
        x = sx - Mul(Raycaster::Cos(angle), back);
        y = sy - Mul(Raycaster::Sin(angle), back);
        if (m_ShakeMs > 0)
        {
            angle += ((m_ShakeMs / 40) % 2 == 0 ? 6 : -6);
        }
        return;
    }
    const int from = m_FacingFrom * kAngleSteps / 4;
    int turn = m_Facing * kAngleSteps / 4 - from;
    if (turn > kAngleSteps / 2)
    {
        turn -= kAngleSteps;
    }
    if (turn < -kAngleSteps / 2)
    {
        turn += kAngleSteps;
    }
    angle = from + turn * t / 1024;
    // The eye is a little behind the tile's middle, so what stands on the
    // next tile is seen whole, its feet and the floor under it included.
    x -= Mul(Raycaster::Cos(angle), kEyeBack);
    y -= Mul(Raycaster::Sin(angle), kEyeBack);
    if (m_ShakeMs > 0)
    {
        // A knock: the view twitches side to side.
        angle += ((m_ShakeMs / 40) % 2 == 0 ? 6 : -6);
    }
}

int CrawlerGame::ViewIndex(Fix x, Fix y, int facing, Fix viewX, Fix viewY)
{
    // Where the viewer is, from the thing: ahead of it (along its facing)
    // and to its right. Eight sides, 45 degrees each, the front first, then
    // round by its right.
    constexpr int kDx[4] = { 1, 0, -1, 0 };
    constexpr int kDy[4] = { 0, 1, 0, -1 };
    const int f = facing & 3;
    const int64_t dx = viewX - x;
    const int64_t dy = viewY - y;
    const int64_t ahead = dx * kDx[f] + dy * kDy[f];
    const int64_t right = -dx * kDy[f] + dy * kDx[f];
    const int64_t a = ahead < 0 ? -ahead : ahead;
    const int64_t r = right < 0 ? -right : right;
    // tan 22.5 degrees is about 0.41: within that of an axis is straight on.
    if (5 * r < 2 * a)
    {
        return ahead >= 0 ? 0 : 4;
    }
    if (5 * a < 2 * r)
    {
        return right >= 0 ? 2 : 6;
    }
    if (ahead >= 0)
    {
        return right >= 0 ? 1 : 7;
    }
    return right >= 0 ? 3 : 5;
}

int CrawlerGame::GetShadows(Raycaster::Shadow* out, int max) const
{
    int count = 0;
    const auto add = [&](Fix x, Fix y, int percent)
    {
        if (count < max)
        {
            out[count++] = { x, y, Pct(percent) };
        }
    };
    const int t = GlideProgress();
    for (const Monster& m : m_Monsters)
    {
        if (m.alive)
        {
            // A little wider than it is, so a rim shows round its feet
            // (FootOf); smaller under a flier.
            const MonsterStats& stats = StatsOf(m.kind);
            int percent = static_cast<int>((static_cast<int64_t>(stats.scale) * 46) >> kFixShift);
            percent = stats.hover > 0 ? percent * 2 / 3 : percent;
            percent = percent < 20 ? 20 : (percent > 42 ? 42 : percent);
            add(Lerp(Centre(m.from.x), Centre(m.at.x), t), Lerp(Centre(m.from.y), Centre(m.at.y), t), percent);
        }
    }
    for (const Item& item : m_Items)
    {
        if (!item.taken)
        {
            add(Centre(item.at.x), Centre(item.at.y), item.kind == ItemKind::Chest ? 34 : 24);
        }
    }
    for (const Feature& f : m_Features)
    {
        if (f.present && f.kind != FeatureKind::PortalHome)
        {
            add(Centre(f.at.x), Centre(f.at.y), 38);
        }
    }
    for (const Prop& p : m_Props)
    {
        if (p.present)
        {
            add(Centre(p.at.x), Centre(p.at.y), 38);
        }
    }
    return count;
}

int CrawlerGame::GetBillboards(const CrawlerTextures& textures, Billboard* out, int max, Fix viewX, Fix viewY) const
{
    int count = 0;
    const int t = GlideProgress();
    const Fix heroX = Centre(m_Hero.x);
    const Fix heroY = Centre(m_Hero.y);
    for (const Monster& m : m_Monsters)
    {
        if (!m.alive || count >= max)
        {
            continue;
        }
        const MonsterStats& stats = StatsOf(m.kind);
        Billboard& b = out[count++];
        b.foot = 0;
        b.x = Lerp(Centre(m.from.x), Centre(m.at.x), t);
        b.y = Lerp(Centre(m.from.y), Centre(m.at.y), t);
        // A lunge: a third of the way toward the hero and back.
        if (m.lungeMs > 0)
        {
            const int phase = (kLungeMs - m.lungeMs) * (kAngleSteps / 2) / kLungeMs;
            const Fix reach = Raycaster::Sin(phase) / 3;
            b.x += static_cast<Fix>((static_cast<int64_t>(heroX - b.x) * reach) >> kFixShift);
            b.y += static_cast<Fix>((static_cast<int64_t>(heroY - b.y) * reach) >> kFixShift);
        }
        b.texture = textures.monsters[static_cast<int>(m.kind)];
        // Someone with a name stands a head taller than their kind.
        b.scale = m.named ? stats.scale * 6 / 5 : stats.scale;
        // A glimmerjelly glitters: a shade brighter now and then.
        b.shadeBias = m.kind == MonsterKind::Glimmerjelly && ((m_TimeMs + m.phase) / 140) % 3 == 0 ? -3 : 0;
        // The ghost flickers in and out of the dark.
        if (m.kind == MonsterKind::Ghost)
        {
            b.shadeBias = ((m_TimeMs + m.phase) / 90) % 4 == 0 ? 3 : -1;
        }
        // Idle: breathing, each monster at its own phase; a flier bobs up
        // and down as well.
        const int idle = m.phase + static_cast<int>(m_TimeMs / 2);
        const Fix wave = Raycaster::Sin(idle);
        b.lift = stats.hover + (stats.hover > 0 ? wave / 8 : 0);
        b.foot = stats.hover > 0 ? 0 : FootOf(stats.scale);
        b.squash = kFixOne;
        b.breath = wave < 0 ? 1 : 0;
        if (m.hurtMs > 0)
        {
            // Hit: knocked back from the hero and springing back, squashed
            // wide by the blow, lit up white at first.
            const int phase = (kHurtMs - m.hurtMs) * (kAngleSteps / 2) / kHurtMs;
            const Fix push = Raycaster::Sin(phase) / 5;
            const int dx = m.at.x > m_Hero.x ? 1 : (m.at.x < m_Hero.x ? -1 : 0);
            const int dy = m.at.y > m_Hero.y ? 1 : (m.at.y < m_Hero.y ? -1 : 0);
            b.x += dx * push;
            b.y += dy * push;
            b.squash += push;
            if (kHurtMs - m.hurtMs < 50)
            {
                b.palette = kFlashPalette;
            }
        }
    }
    for (const Feature& f : m_Features)
    {
        if (!f.present || count >= max)
        {
            continue;
        }
        Billboard& b = out[count++];
        b.foot = 0;
        b.x = Centre(f.at.x);
        b.y = Centre(f.at.y);
        b.texture = f.kind == FeatureKind::Keeper ? (m_Inside >= 0 ? textures.keepers[m_Inside] : nullptr)
                                                  : textures.features[static_cast<int>(f.kind)];
        b.shadeBias = 0;
        if (f.kind == FeatureKind::Granny || f.kind == FeatureKind::Kid || f.kind == FeatureKind::Keeper ||
            f.kind == FeatureKind::Fence)
        {
            // Villagers breathe, each at their own pace; the child hops.
            const Fix wave = Raycaster::Sin(static_cast<int>(m_TimeMs / (f.kind == FeatureKind::Kid ? 2 : 4)));
            b.scale = f.kind == FeatureKind::Kid ? Pct(56) : Pct(68);
            b.lift = f.kind == FeatureKind::Kid ? (wave > 0 ? wave / 12 : 0) : 0;
            b.squash = kFixOne;
            b.breath = wave < 0 ? 1 : 0;
        }
        else if (f.kind == FeatureKind::Merchant)
        {
            // Puddle breathes, slowly.
            const Fix wave = Raycaster::Sin(static_cast<int>(m_TimeMs / 3));
            b.scale = Pct(74);
            b.lift = 0;
            b.squash = kFixOne;
            b.breath = wave < 0 ? 1 : 0;
        }
        else
        {
            b.scale = f.kind == FeatureKind::Shrine                                       ? Pct(66)
                      : f.kind == FeatureKind::PortalHome || f.kind == FeatureKind::Board ? Pct(80)
                                                                                          : Pct(58);
            b.lift = 0;
            b.squash = kFixOne;
            b.shadeBias = f.used ? 2 : 0;
            if (f.kind == FeatureKind::Fountain && f.used && textures.fountainDry)
            {
                b.texture = textures.fountainDry;  // drunk dry: it shows
                b.shadeBias = 0;
            }
        }
        b.foot = f.kind == FeatureKind::PortalHome ? 0 : FootOf(b.scale);
    }
    if (m_Bolt.ms > 0 && textures.bolt && count < max)
    {
        const int t1024 = 1024 - m_Bolt.ms * 1024 / m_Bolt.total;
        Billboard& b = out[count++];
        b.foot = 0;
        b.x = Lerp(m_Bolt.fromX, m_Bolt.toX, t1024);
        b.y = Lerp(m_Bolt.fromY, m_Bolt.toY, t1024);
        b.texture = textures.bolt;
        b.scale = Pct(28);
        b.shadeBias = -8;
        b.lift = Pct(120);
        b.squash = kFixOne;
    }
    for (int i = 0; i < kMaxTorches; ++i)
    {
        const Torch& t = m_Torches[i];
        if (!t.present || !textures.torch[0] || count >= max)
        {
            continue;
        }
        // The flame flickers: two frames, each torch out of step.
        const int frame = static_cast<int>((m_TimeMs / 110 + static_cast<uint32_t>(i) * 3) % 2);
        Billboard& b = out[count++];
        b.foot = 0;
        b.x = t.x;
        b.y = t.y;
        b.texture = textures.torch[frame] ? textures.torch[frame] : textures.torch[0];
        if (textures.torchViews[0])
        {
            b.texture = textures.torchViews[ViewIndex(t.x, t.y, t.facing, viewX, viewY)];
        }
        b.scale = Pct(38);
        b.shadeBias = -2;
        b.lift = Pct(115);   // up on the wall
        b.squash = kFixOne;  // its colours flicker (the Crawler's palettes); its shape holds still
    }
    for (const Prop& prop : m_Props)
    {
        const int kind = PropIndex(prop);
        const Texture* texture =
            textures.propViews[kind][0]
                ? textures.propViews[kind][ViewIndex(Centre(prop.at.x), Centre(prop.at.y), prop.facing, viewX, viewY)]
                : textures.props[kind];
        if (textures.spent[kind] && LooksSpent(prop))
        {
            texture = textures.spent[kind];  // used up: it shows
        }
        if (kind == kPots)
        {
            const int shape = (prop.at.x * 7 + prop.at.y * 13) % 3;
            texture = shape > 0 && textures.pots[shape - 1] ? textures.pots[shape - 1] : texture;
        }
        if (!prop.present || !texture || count >= max)
        {
            continue;
        }
        Billboard& b = out[count++];
        b.foot = 0;
        b.x = Centre(prop.at.x);
        b.y = Centre(prop.at.y);
        b.texture = texture;
        b.scale = Pct(70);
        b.foot = FootOf(b.scale);
        b.shadeBias = 0;
        b.lift = 0;
        b.squash = kFixOne;
    }
    for (const Item& item : m_Items)
    {
        // What stands on it is what shows.
        if (item.taken || count >= max || MonsterAt(item.at) >= 0)
        {
            continue;
        }
        Billboard& b = out[count++];
        b.foot = 0;
        b.x = Centre(item.at.x);
        b.y = Centre(item.at.y);
        b.texture = textures.items[static_cast<int>(item.kind)];
        // Big and a shade brighter than the floor round them, to be spotted
        // from across a room.
        b.scale = item.kind == ItemKind::Chest ? Pct(60) : Pct(50);
        b.foot = FootOf(b.scale);
        b.shadeBias = -1;
        // Lying on the floor, like everything else that stands there.
        b.lift = 0;
        b.squash = kFixOne;
    }
    return count;
}
}  // namespace Dungeon
