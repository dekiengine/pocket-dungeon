// Fighting: the hero's blows and spells, what a blow does to a monster, and the
// monsters' turn.

#include "CrawlerGame.h"

#include "Contracts.h"
#include "CrawlerGameData.h"
#include "Village.h"

#include <cstdio>
#include <cstring>

namespace Dungeon
{

using namespace GameData;

void CrawlerGame::Blast()
{
    m_BlastMs = kBlastMs;
    m_ShakeMs = kShakeMs;
    const Cell ahead = Ahead(m_Hero, m_Facing, 1);
    const Tile tile = m_Map.At(ahead);
    // Every monster near the hero is hurt, as by any blow: one it kills
    // leaves what it would have left (a boss its chest and the way down,
    // the Keeper the story's end).
    int hit = 0;
    const uint32_t said = m_MessageCount;
    m_Critical = false;
    for (int i = 0; i < kMaxMonsters; ++i)
    {
        const Monster& m = m_Monsters[i];
        const int dx = m.at.x - m_Hero.x;
        const int dy = m.at.y - m_Hero.y;
        if (!m.alive || dx < -2 || dx > 2 || dy < -2 || dy > 2)
        {
            continue;
        }
        ++hit;
        HurtMonster(i, 6 + m_Depth, nullptr);
    }
    if (m_EndingPending)
    {
        return;  // the Keeper fell to it
    }
    char text[kMessageLength];
    if (tile == Tile::SecretWall)
    {
        m_Map.Open(ahead);
        m_Map.ComputeView(m_Hero, 6);
        std::snprintf(text, sizeof(text), "BOOM! The wall comes down. A room behind it.");
    }
    else if (tile == Tile::Door)
    {
        std::snprintf(text, sizeof(text), "BOOM! The smoke clears. The door has not moved.");
    }
    else if (m_MessageCount != said)
    {
        return;  // what fell to it has already been said
    }
    else if (hit > 0)
    {
        std::snprintf(text, sizeof(text), "BOOM! The blast catches %d monster%s.", hit, hit == 1 ? "" : "s");
    }
    else
    {
        std::snprintf(text, sizeof(text), "BOOM! Dust, and ringing ears. Nothing else.");
    }
    Say(text);
}

bool CrawlerGame::CastSpell(Spell spell)
{
    char text[kMessageLength];
    const int cost = SpellCost(spell);
    if (m_Mp < cost)
    {
        std::snprintf(text, sizeof(text), "Not enough MP (%d needed).", cost);
        Say(text);
        return false;
    }
    m_Mp -= cost;
    m_FlashMs = 220;
    m_FlashKind = static_cast<int>(spell);
    m_HeroFrom = m_Hero;
    m_FacingFrom = m_Facing;
    for (Monster& m : m_Monsters)
    {
        m.from = m.at;
    }
    switch (spell)
    {
        case Spell::FireBolt:
        {
            // Down the corridor to the first monster, or the first wall.
            Cell at = m_Hero;
            int target = -1;
            int steps = 0;
            for (int i = 1; i <= 8; ++i)
            {
                const Cell next = Ahead(m_Hero, m_Facing, i);
                if (!m_Map.IsWalkable(next))
                {
                    break;
                }
                at = next;
                steps = i;
                target = MonsterAt(next);
                if (target >= 0)
                {
                    break;
                }
            }
            const Fix half = kFixOne / 2;
            m_Bolt.fromX = Centre(m_Hero.x);
            m_Bolt.fromY = Centre(m_Hero.y);
            const Cell end = target >= 0 ? at : Ahead(m_Hero, m_Facing, steps);
            constexpr int kDx[4] = { 1, 0, -1, 0 };
            constexpr int kDy[4] = { 0, 1, 0, -1 };
            m_Bolt.toX = Centre(end.x) + (target >= 0 ? 0 : kDx[m_Facing & 3] * half);
            m_Bolt.toY = Centre(end.y) + (target >= 0 ? 0 : kDy[m_Facing & 3] * half);
            m_Bolt.total = 90 * (steps > 0 ? steps : 1);
            m_Bolt.ms = m_Bolt.total;
            if (target < 0)
            {
                Say("Fire Bolt! It bursts on the wall.");
                return true;
            }
            HurtMonster(target, m_Random.Range(3, 5) + m_Level + m_Depth / 3, "Fire Bolt! It burns the");
            return true;
        }
        case Spell::Heal:
        {
            // Healing a mired adept ahead brings Sigrun back.
            const int ahead = MonsterAt(Ahead(m_Hero, m_Facing, 1));
            if (ahead >= 0 && m_Monsters[ahead].kind == MonsterKind::Slimewizard)
            {
                Monster& maren = m_Monsters[ahead];
                maren.alive = false;
                PlaceItem(ItemKind::Elixir, maren.at);
                m_MaxMp += 4;
                m_Mp = m_MaxMp;
                ++m_Legend.marenSaved;
                ++m_LegendVersion;
                Say("The gold runs off her. Sigrun: \"I know you.\" Max MP +4");
                return true;
            }
            const int heal = 8 + 2 * m_Level;
            m_Hp = m_Hp + heal < m_MaxHp ? m_Hp + heal : m_MaxHp;
            Sound(Cue::Heal);
            std::snprintf(text, sizeof(text), "Warm light. HP %d/%d.", m_Hp, m_MaxHp);
            Say(text);
            return true;
        }
        case Spell::Frost:
        {
            int frozen = 0;
            for (int i = 0; i < kMaxMonsters; ++i)
            {
                Monster& m = m_Monsters[i];
                const int dx = m.at.x - m_Hero.x;
                const int dy = m.at.y - m_Hero.y;
                if (!m.alive || dx < -2 || dx > 2 || dy < -2 || dy > 2)
                {
                    continue;
                }
                m.frozen = 3;
                ++frozen;
                HurtMonster(i, 2 + m_Level / 2, nullptr);
            }
            std::snprintf(text, sizeof(text), frozen > 0 ? "Frost! %d frozen solid." : "Frost! The air freezes.",
                          frozen);
            Say(text);
            return true;
        }
        case Spell::Blink:
        {
            Cell landing = m_Hero;
            for (int i = 1; i <= 4; ++i)
            {
                const Cell next = Ahead(m_Hero, m_Facing, i);
                if (!m_Map.IsWalkable(next) || PropAt(next) >= 0 || FeatureAt(next) >= 0)
                {
                    break;
                }
                if (MonsterAt(next) < 0)
                {
                    landing = next;
                }
            }
            if (landing == m_Hero)
            {
                m_Mp += cost;
                Say("No room to blink.");
                return false;
            }
            m_Hero = landing;
            PickUp();
            Say("Blink! You flicker forward.");
            return true;
        }
        case Spell::Fold:
        {
            // The first foe ahead is folded into the wall; a boss only hurt.
            for (int i = 1; i <= 6; ++i)
            {
                const Cell next = Ahead(m_Hero, m_Facing, i);
                if (!m_Map.IsWalkable(next))
                {
                    break;
                }
                const int target = MonsterAt(next);
                if (target >= 0)
                {
                    const bool boss = IsBoss(m_Monsters[target].kind);
                    HurtMonster(target, boss ? 12 + m_Level : 999, "Fold! Space crumples round the");
                    return true;
                }
            }
            Say("Fold! Space crinkles. Nothing there.");
            return true;
        }
    }
    return true;
}

void CrawlerGame::HeroAttacks(int index)
{
    Monster& m = m_Monsters[index];
    m_SwingMs = kSwingMs;
    char text[kMessageLength];
    int damage = m_Random.Range(1, m_Attack + 1);
    bool critical = m_Random.Chance(m_Weapon == Weapon::Blade ? 15 : 6);
    const char* how = nullptr;
    switch (m_Weapon)
    {
        case Weapon::Axe:
            if (m_Random.Chance(15))
            {
                AddEvent(EventKind::Miss, 0, m);
                return;
            }
            damage += 2;
            break;
        case Weapon::Dagger: damage = m_Random.Range(1, m_Attack / 2 + 1) + m_Random.Range(1, m_Attack / 2 + 1); break;
        case Weapon::Hammer:
            if (m_Random.Chance(30))
            {
                m.frozen = m.frozen > 1 ? m.frozen : 1;
                how = "Stunned! You bash the";
            }
            break;
        default: break;
    }
    if (m_Tipsy > 0)
    {
        if (m_Random.Chance(20))
        {
            std::snprintf(text, sizeof(text), "The room spins. You miss the %s by a mile.", StatsOf(m.kind).name);
            Say(text);
            AddEvent(EventKind::Miss, 0, m);
            return;
        }
        damage += 2;
    }
    if (critical)
    {
        damage *= 2;
    }
    damage -= StatsOf(m.kind).defense;
    m_Critical = critical;
    HurtMonster(index, damage < 1 ? 1 : damage, how);
    m_Critical = false;
}

void CrawlerGame::HurtMonster(int index, int damage, const char* how)
{
    Monster& m = m_Monsters[index];
    const MonsterStats& stats = StatsOf(m.kind);
    if (m.kind == MonsterKind::Glimmerjelly)
    {
        damage = 1;
    }
    m.hp -= damage;
    m.hurtMs = kHurtMs;
    AddEvent(m_Critical ? EventKind::Critical : EventKind::Hit, damage, m);
    char text[kMessageLength];
    if (m.hp > 0 && m.kind == MonsterKind::Keeper)
    {
        constexpr const char* kTaunts[] = { "Keeper: I counted them tonight. You are not one.",
                                            "Keeper: I am Halvard. I have the hat. I AM.",
                                            "Keeper: No one leaves. I swore it on the door.",
                                            "Keeper: Strike. You will only come back to me." };
        Say(kTaunts[m_Random.Range(0, 3)]);
        if (!m_KeeperCalled && m.hp * 2 < stats.hp)
        {
            // Half gone: it coughs up help.
            m_KeeperCalled = true;
            int called = 0;
            for (int f = 0; f < 4 && called < 2; ++f)
            {
                const Cell c = Ahead(m.at, f, 1);
                if (IsFree(c) && PlaceMonster(called == 0 ? MonsterKind::Droolblob : MonsterKind::Candleghoul, c))
                {
                    ++called;
                }
            }
            Say("Keeper: Come out, all of you. Keep this one!");
        }
        return;
    }
    if (m.hp > 0)
    {
        if (how)
        {
            std::snprintf(text, sizeof(text), "%s %s for %d.", how, stats.name, damage);
            Say(text);
        }
        else
        {
            ++m_Version;
        }
        return;
    }
    m.alive = false;
    AddEvent(EventKind::Kill, 0, m);
    if (m.named)
    {
        ContractFound(static_cast<int>(Contracts::Id::Brother));  // Orm: his knife
    }
    if (m.kind == MonsterKind::Grubking)
    {
        ContractFound(static_cast<int>(Contracts::Id::Ledger));  // the baron: his ledger
    }
    if (m.kind == MonsterKind::Glimmerjelly)
    {
        const int gold = m_Random.Range(20, 40) * m_Depth;
        m_Gold += gold;
        std::snprintf(text, sizeof(text), "Caught the gilt wisp! +%d XP, +%d G", stats.xp + 4 * m_Depth, gold);
        Say(text);
        GainXp(stats.xp + 4 * m_Depth);
        return;
    }
    if (m.kind == MonsterKind::Keeper)
    {
        // The end of the story: a choice, put to the hero once whatever
        // felled it is done with (a bomb out of the bag, a spell out of the
        // magic: their menus close after this).
        GainXp(stats.xp);
        m_EndingPending = true;
        ++m_Version;
        Say("The Keeper falls. Under the hat, a foreman's face.");
        return;
    }
    if (m.kind == MonsterKind::Ghost)
    {
        const int back = m_Legend.ghostGold / 2 > 5 * m_Depth ? m_Legend.ghostGold / 2 : 5 * m_Depth;
        m_Gold += back;
        m_Legend.ghostDepth = 0;
        m_Legend.ghostGold = 0;
        ++m_LegendVersion;
        std::snprintf(text, sizeof(text), "Your echo fades... and hands back %d gold.", back);
        Say(text);
        GainXp(stats.xp);
        return;
    }
    ++m_Version;
    GainXp(stats.xp);
    const bool snacky =
        m.kind == MonsterKind::Droolblob || m.kind == MonsterKind::Teasnail || m.kind == MonsterKind::Stewpot;
    if (m_Random.Chance(12))
    {
        PlaceItem(static_cast<ItemKind>(static_cast<int>(ItemKind::Moss) + GetZone()), m.at);
    }
    else if (snacky && m_Random.Chance(30))
    {
        PlaceItem(ItemKind::Snack, m.at);
    }
    else if (IsBoss(m.kind))
    {
        // A boss leaves a chest and a heart crystal where it stood, and
        // its zone's material round it.
        if (!PlaceChest(m.at))
        {
            PlaceItem(ItemKind::Gold, m.at);
        }
        m_Map.ComputeDistances(m_Hero);
        if (m.kind == MonsterKind::Mimic || m.kind == MonsterKind::Grubking || m.kind == MonsterKind::Hydragon)
        {
            m_Legend.bosses = static_cast<uint8_t>(m_Legend.bosses | (m.kind == MonsterKind::Mimic      ? 1
                                                                      : m.kind == MonsterKind::Grubking ? 2
                                                                                                        : 4));
            ++m_LegendVersion;
        }
        int dropped = 0;
        for (int f = 0; f < 4 && dropped < 2; ++f)
        {
            const Cell c = Ahead(m.at, f, 1);
            if (c != m_Hero && IsFree(c) &&
                PlaceItem(static_cast<ItemKind>(static_cast<int>(ItemKind::Moss) + GetZone()), c))
            {
                ++dropped;
            }
        }
        Gain(ItemKind::Heart, "The way down is open! ");
    }
    else if (m_Random.Chance(18))
    {
        PlaceItem(PickLoot(false), m.at);
    }
}

void CrawlerGame::MonstersAct()
{
    m_Map.ComputeDistances(m_Hero);
    for (Monster& m : m_Monsters)
    {
        if (!m.alive)
        {
            continue;
        }
        if (m.frozen > 0)
        {
            --m.frozen;
            continue;
        }
        const uint16_t distance = m_Map.DistanceAt(m.at);
        constexpr Cell kSteps[4] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
        if (m.kind == MonsterKind::Glimmerjelly)
        {
            // Seen up close it may slip away for good; otherwise it runs.
            if (m_Map.IsVisible(m.at) && distance <= 4 && m_Random.Chance(18))
            {
                m.alive = false;
                Say("The gilt wisp slips away!");
                continue;
            }
            Cell best = m.at;
            uint16_t bestDistance = distance;
            for (const Cell& step : kSteps)
            {
                const Cell n{ m.at.x + step.x, m.at.y + step.y };
                const uint16_t d = m_Map.DistanceAt(n);
                if (d != DungeonMap::kUnreached && (bestDistance == DungeonMap::kUnreached || d > bestDistance) &&
                    IsFree(n))
                {
                    bestDistance = d;
                    best = n;
                }
            }
            m.at = best;
            continue;
        }
        if (distance == 1)
        {
            const MonsterStats& stats = StatsOf(m.kind);
            m.lungeMs = kLungeMs;
            // Never a scratch: at least half of what it can do.
            const int damage = m_Random.Range((stats.attack + 1) / 2, stats.attack + m_Depth / 3) - m_Defense;
            char text[kMessageLength];
            if (damage <= 0)
            {
                continue;
            }
            m_Hp -= damage;
            m_ShakeMs = kShakeMs;
            if (m_Hp <= 0)
            {
                m_Hp = 0;
                std::snprintf(text, sizeof(text), "The %s beats you.", stats.name);
                Say(text);
            }
            else
            {
                ++m_Version;
            }
            if (m_Hp <= 0)
            {
                return;
            }
            continue;
        }
        // Bosses wait by the stairs; the Keeper comes for you.
        if ((IsBoss(m.kind) && m.kind != MonsterKind::Keeper) || distance == DungeonMap::kUnreached ||
            distance > kWakeDistance)
        {
            continue;
        }
        const Cell next = m_Map.StepTowardTarget(m.at);
        if (next != m.at && IsFree(next))
        {
            m.at = next;
        }
    }
}
}  // namespace Dungeon
