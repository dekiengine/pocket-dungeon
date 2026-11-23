// What can be done with the things standing about. Each offers a choice or
// two (drink from the barrel or smash it, knock on the coffin or open it):
// which ones are still open, the menu that offers them, and what each does.

#include "CrawlerGame.h"

#include "Contracts.h"
#include "CrawlerGameData.h"
#include "Village.h"

#include <cstdio>

namespace Dungeon
{

using namespace GameData;

namespace
{

// What becomes of a choice once taken: still there (it failed, or can be
// done again), used up, or the whole thing used up with it.
enum class After : uint8_t
{
    Kept,
    Done,
    All,
};

constexpr uint8_t kNoWeapon = 15;  // an empty place on a weapon rack

}  // namespace

void CrawlerGame::SetUpProp(Prop& prop)
{
    prop.done = 0;
    prop.state = 0;
    switch (PropIndex(prop))
    {
        case kAleBarrel: prop.state = 3; break;                                        // gulps
        case kCoffin: prop.state = m_Depth > 1 && m_Random.Chance(35) ? 1 : 0; break;  // someone is in
        case kWeaponRack:
        {
            // Two weapons the hero could use and does not hold, one to a side.
            uint8_t found[kWeapons];
            int count = 0;
            for (int w = 0; w < kWeapons; ++w)
            {
                if (w != static_cast<int>(m_Weapon) && IsWeaponUnlocked(static_cast<Weapon>(w)))
                {
                    found[count++] = static_cast<uint8_t>(w);
                }
            }
            const int first = count > 0 ? m_Random.Range(0, count - 1) : -1;
            const int second = count > 1 ? (first + m_Random.Range(1, count - 1)) % count : -1;
            prop.state = static_cast<uint8_t>((first >= 0 ? found[first] : kNoWeapon) |
                                              ((second >= 0 ? found[second] : kNoWeapon) << 4));
            break;
        }
        case kFishBasket: prop.state = 2; break;                                         // fish to take
        case kCage: prop.state = m_Depth > 1 && m_Random.Chance(30) ? 1 : 0; break;      // it bites
        case kOldChest: prop.state = m_Depth > 1 && m_Random.Chance(30) ? 1 : 0; break;  // it is no chest
        default: break;
    }
}

Deed CrawlerGame::PropDeed(const Prop& prop, int choice) const
{
    if (!prop.present || prop.fixed || choice < 0 || choice >= kThingDeeds || (prop.done & (1u << choice)) != 0)
    {
        return Deed::None;
    }
    const Deed deed = kPropInfo[PropIndex(prop)].deeds[choice];
    // The tavern's barrels are Barley's: no smashing those.
    if (deed == Deed::Smash && m_InVillage)
    {
        return Deed::None;
    }
    if (deed == Deed::TakeSecond && (prop.state >> 4) >= kWeapons)
    {
        return Deed::None;
    }
    return deed;
}

bool CrawlerGame::HasDeeds(const Prop& prop) const
{
    for (int choice = 0; choice < kThingDeeds; ++choice)
    {
        if (PropDeed(prop, choice) != Deed::None)
        {
            return true;
        }
    }
    return false;
}

Deed CrawlerGame::ThingDeed(int choice) const
{
    if (m_ThingProp >= 0)
    {
        return PropDeed(m_Props[m_ThingProp], choice);
    }
    if (m_ThingFeature >= 0 && m_Features[m_ThingFeature].kind == FeatureKind::Shrine)
    {
        // A stone listener: a prayer (once), a name it has not heard, and
        // with a piece of the heart in hand the name cut into it.
        const bool can = choice == 0 ? !m_Features[m_ThingFeature].used : choice == 1 ? NextName() >= 0 : m_Shards > 0;
        constexpr Deed kDeeds[kThingDeeds] = { Deed::PrayStone, Deed::GiveName, Deed::SayHeartName };
        return can ? kDeeds[choice] : Deed::None;
    }
    if (m_ThingFeature >= 0 && !m_Features[m_ThingFeature].used)
    {
        return choice == 0 ? Deed::DrinkDeep : choice == 1 ? Deed::Wash : Deed::FillSkin;
    }
    if (m_ThingItem >= 0 && choice < 2)
    {
        return choice == 0 ? Deed::Eat : Deed::Throw;
    }
    return Deed::None;
}

const char* CrawlerGame::GetThingName() const
{
    const bool stone = m_ThingFeature >= 0 && m_Features[m_ThingFeature].kind == FeatureKind::Shrine;
    return m_ThingProp >= 0   ? kPropInfo[PropIndex(m_Props[m_ThingProp])].name
           : m_ThingItem >= 0 ? ItemName(static_cast<ItemKind>(m_ThingItem))
           : stone            ? kStoneName
                              : kFountainName;
}

const char* CrawlerGame::GetThingAbout() const
{
    const bool stone = m_ThingFeature >= 0 && m_Features[m_ThingFeature].kind == FeatureKind::Shrine;
    return m_ThingProp >= 0   ? kPropInfo[PropIndex(m_Props[m_ThingProp])].about
           : m_ThingItem >= 0 ? ItemHelp(static_cast<ItemKind>(m_ThingItem))
           : stone            ? kStoneAbout
                              : kFountainAbout;
}

const char* CrawlerGame::DeedHint(const MenuEntry& entry) const
{
    switch (entry.deed)
    {
        case Deed::TakeFirst:
        case Deed::TakeSecond: return ItemHelp(entry.item);
        case Deed::PickLock:
            if (entry.count == 0)
            {
                return "You have no lockpick.";
            }
            break;
        case Deed::Cook:
            if (entry.count == 0)
            {
                return "You have nothing to cook.";
            }
            break;
        case Deed::FeedCage:
            if (entry.count == 0)
            {
                return "You have nothing it would eat.";
            }
            break;
        case Deed::FillSkin:
            if (entry.count == 0)
            {
                return "You can carry no more water.";
            }
            break;
        case Deed::SayHeartName:
            if (entry.count == 0)
            {
                return "The heart is not whole. Four pieces.";
            }
            break;
        default: break;
    }
    return kDeedInfo[static_cast<int>(entry.deed)].hint;
}

void CrawlerGame::UseProp(int index)
{
    const Prop& prop = m_Props[index];
    if (prop.fixed)
    {
        // A stone in the village's yard is read; any other furnishing is
        // only to look at.
        for (int stone = 0; stone < Village::kStones && m_InVillage && m_Inside < 0; ++stone)
        {
            if (Village::StoneCell(stone) == prop.at)
            {
                m_Gained = -1;
                Sound(Cue::Pray);
                // Astrid's, once her doll is home.
                const bool doll = stone == 2 && Contracts::IsKept(m_Legend, Contracts::Id::Doll);
                Say(doll ? "\"Astrid, six.\" A rag doll sits against the stone." : Village::StoneLine(stone));
                ShowResult(Reveal::Now);
                return;
            }
        }
        Say(kPropInfo[PropIndex(prop)].about);
        return;
    }
    if (!HasDeeds(prop))
    {
        Say(kNothingMore);
        return;
    }
    m_ThingProp = index;
    m_ThingFeature = -1;
    m_ThingItem = -1;
    OpenMenu(Menu::Thing);
}

int CrawlerGame::NextName() const
{
    for (int name = 0; name < 3; ++name)
    {
        if ((m_WordsUsed & (1u << name)) == 0)
        {
            return name;
        }
    }
    return Lore::CountBits(m_Legend.ducks) >= Lore::kDucks ? 3 : -1;
}

void CrawlerGame::UseShrine(int feature)
{
    m_ThingProp = -1;
    m_ThingFeature = feature;
    m_ThingItem = -1;
    if (ThingDeed(0) == Deed::None && ThingDeed(1) == Deed::None && ThingDeed(2) == Deed::None)
    {
        Say(kNothingMore);
        return;
    }
    OpenMenu(Menu::Thing);
}

void CrawlerGame::UseFountain(int feature)
{
    if (m_Features[feature].used)
    {
        Say("The basin is dry.");
        return;
    }
    m_ThingProp = -1;
    m_ThingFeature = feature;
    m_ThingItem = -1;
    OpenMenu(Menu::Thing);
}

void CrawlerGame::UseFood(ItemKind kind)
{
    m_ThingProp = -1;
    m_ThingFeature = -1;
    m_ThingItem = static_cast<int>(kind);
    OpenMenu(Menu::Thing);
}

void CrawlerGame::ThrowFood(ItemKind kind)
{
    // To the first thing in line, a few tiles off: it eats, and is busy.
    constexpr int kReach = 5;
    constexpr int kBusy = 4;
    char text[kMessageLength];
    for (int i = 1; i <= kReach; ++i)
    {
        const Cell c = Ahead(m_Hero, m_Facing, i);
        if (!m_Map.IsWalkable(c) || PropAt(c) >= 0 || FeatureAt(c) >= 0)
        {
            break;
        }
        const int index = MonsterAt(c);
        if (index < 0)
        {
            continue;
        }
        Monster& m = m_Monsters[index];
        const char* name = StatsOf(m.kind).name;
        if (m.kind >= MonsterKind::Mimic)
        {
            // The big ones, and the dead, are not hungry for that.
            std::snprintf(text, sizeof(text), "The %s does not even look at it.", name);
            Say(text);
            return;
        }
        --m_Bag[static_cast<int>(kind)];
        m.frozen = m.frozen > kBusy ? m.frozen : kBusy;
        std::snprintf(text, sizeof(text), "The %s falls on it, and forgets you a while.", name);
        Say(text);
        TurnEnds();
        return;
    }
    Say("Nothing out there to throw it to.");
}

void CrawlerGame::EatGlowcap()
{
    char text[kMessageLength];
    const int roll = m_Random.Range(0, 99);
    if (roll < 15)
    {
        m_Map.RevealAll();
        m_KnowsSecrets = true;
        Say("The dark thins. You see the whole floor, and what it hides.");
    }
    else if (roll < 45)
    {
        m_Mp = m_Mp + 4 < m_MaxMp ? m_Mp + 4 : m_MaxMp;
        std::snprintf(text, sizeof(text), "Your teeth buzz. MP %d.", m_Mp);
        Say(text);
    }
    else if (roll < 75)
    {
        m_Hp = m_Hp + 6 < m_MaxHp ? m_Hp + 6 : m_MaxHp;
        std::snprintf(text, sizeof(text), "Earthy, and good. HP %d.", m_Hp);
        Say(text);
    }
    else
    {
        m_Hp = m_Hp > 3 ? m_Hp - 3 : 1;
        std::snprintf(text, sizeof(text), "Bitter. Your stomach turns. HP %d.", m_Hp);
        Say(text);
    }
}

void CrawlerGame::FillThingMenu()
{
    constexpr const char* kTake[kWeapons] = { "Take the sword", "Take the axe", "Take the spear", "Take the dagger",
                                              "Take the hammer" };
    for (int choice = 0; choice < kThingDeeds && m_MenuCount < kMaxMenu; ++choice)
    {
        const Deed deed = ThingDeed(choice);
        if (deed == Deed::None)
        {
            continue;
        }
        MenuEntry& e = m_MenuEntries[m_MenuCount++];
        e = MenuEntry{};
        e.deed = deed;
        e.price = choice;
        e.count = 1;  // it can be done
        e.label = kDeedInfo[static_cast<int>(deed)].label;
        if (deed == Deed::TakeFirst || deed == Deed::TakeSecond)
        {
            const uint8_t state = m_Props[m_ThingProp].state;
            const int weapon = deed == Deed::TakeFirst ? state & 15 : state >> 4;
            e.item = weapon < kWeapons ? WeaponItem(static_cast<Weapon>(weapon)) : ItemKind::Sword;
            e.label = weapon < kWeapons ? kTake[weapon] : "Take the whetstone";
        }
        else if (deed == Deed::PickLock)
        {
            e.count = m_Bag[static_cast<int>(ItemKind::Lockpick)] > 0 ? 1 : 0;
        }
        else if (deed == Deed::Cook)
        {
            e.count = Carried(ItemKind::Snack) + Carried(ItemKind::Fish) + Carried(ItemKind::Glowcap) > 0 ? 1 : 0;
        }
        else if (deed == Deed::FeedCage)
        {
            e.count = Carried(ItemKind::Snack) + Carried(ItemKind::Fish) > 0 ? 1 : 0;
        }
        else if (deed == Deed::FillSkin)
        {
            e.count = Carried(ItemKind::Water) < kMaxCarried ? 1 : 0;
        }
        else if (deed == Deed::SayHeartName)
        {
            e.count = m_Shards >= Lore::kShards ? 1 : 0;  // the heart must be whole
        }
        else if (deed == Deed::Eat || deed == Deed::Throw)
        {
            e.item = static_cast<ItemKind>(m_ThingItem);
        }
    }
}

void CrawlerGame::LootContainer(const char* from, const char* empty)
{
    const int roll = m_Random.Range(0, 99);
    if (roll < 40)
    {
        Gain(ItemKind::Gold, from);
    }
    else if (roll < 55)
    {
        Gain(PickLoot(true), from);
    }
    else if (roll < 65)
    {
        Gain(static_cast<ItemKind>(static_cast<int>(ItemKind::Moss) + GetZone()), from);
    }
    else if (roll < 73)
    {
        Carry(ItemKind::Snack, 1, from);
    }
    else if (roll < 77)
    {
        Carry(ItemKind::Key, 1, from);
    }
    else
    {
        Say(empty);
    }
}

void CrawlerGame::BreakProp(Prop& prop)
{
    // It goes to pieces and the tile is free; what was in it is yours.
    const int kind = PropIndex(prop);
    AddBreak(prop.at, kind);
    prop.present = false;
    m_SwingMs = kSwingMs;
    if (kind == kAleBarrel)
    {
        LootContainer("The staves burst. ", "The staves burst. Ale, and nothing else.");
    }
    else if (kind == kPots)
    {
        LootContainer("It shatters. ", "It shatters. Shards, and old air.");
    }
    else
    {
        LootContainer("The wood gives. ", "The wood gives. Straw. Only straw.");
    }
    m_Map.ComputeDistances(m_Hero);
    ++m_Version;
}

void CrawlerGame::DoDeed(const MenuEntry& entry)
{
    m_Gained = -1;
    // What it sounds like, where the game's changes would not say.
    switch (entry.deed)
    {
        case Deed::Drink:
        case Deed::DrinkDeep:
        case Deed::Wash:
        case Deed::FillSkin: Sound(Cue::Drink); break;
        case Deed::EatBerries:
        case Deed::EatMushroom:
        case Deed::Cook:
        case Deed::FeedCage: Sound(Cue::Eat); break;
        case Deed::Pray:
        case Deed::Bow:
        case Deed::TouchCrystal:
        case Deed::PrayStone:
        case Deed::GiveName:
        case Deed::ReadStone: Sound(Cue::Pray); break;
        case Deed::Dig:
        case Deed::DigBasket:
        case Deed::SearchCart:
        case Deed::BreakCrystal: Sound(Cue::Dig); break;
        case Deed::PickLock:
        case Deed::ForceLock:
        case Deed::OpenChest:
        case Deed::OpenCoffin:
        case Deed::OpenCage:
        case Deed::PryGem: Sound(Cue::Lock); break;
        case Deed::Sharpen:
        case Deed::MendArmour: Sound(Cue::Sharpen); break;
        case Deed::Warm:
        case Deed::Sleep: Sound(Cue::Heal); break;
        default: break;
    }
    DoDeedNow(entry);
    // What a contract asked for turns up where it was said to be: the doll
    // in the Ossuary's coffins and bones (not always the first), the hammer
    // by an anvil in the Foundry.
    if (!m_InVillage && GetZone() == 1 && (entry.deed == Deed::OpenCoffin || entry.deed == Deed::Search) &&
        Contracts::GetState(m_Legend, Contracts::Id::Doll) == Contracts::State::Taken && m_Random.Chance(50))
    {
        ContractFound(static_cast<int>(Contracts::Id::Doll));
    }
    if (!m_InVillage && GetZone() == 3 && (entry.deed == Deed::Sharpen || entry.deed == Deed::MendArmour))
    {
        ContractFound(static_cast<int>(Contracts::Id::Hammer));
    }
    if (ShowContractNews())
    {
        return;
    }
    // What it did is shown, not only said: as a shape first when something
    // shut was opened or reached into, after the blow when something was
    // broken. Food out of the bag is eaten in passing.
    if (m_Menu != Menu::None || IsDead() || entry.deed == Deed::Eat || entry.deed == Deed::Throw)
    {
        return;
    }
    switch (entry.deed)
    {
        case Deed::Search:
        case Deed::OpenCoffin:
        case Deed::Knock:
        case Deed::OpenChest:
        case Deed::PickLock:
        case Deed::ForceLock:
        case Deed::ReachIn:
        case Deed::ReachForPearl:
        case Deed::Dig:
        case Deed::DigBasket:
        case Deed::SearchCart:
        case Deed::SearchShelf:
        case Deed::SearchCamp:
        case Deed::OpenCage:
        case Deed::LookMirror:
        case Deed::DrinkDeep:
        case Deed::EatMushroom: ShowResult(Reveal::Opened); break;
        case Deed::Smash:
        case Deed::BreakCrystal:
        case Deed::BreakMirror: ShowResult(Reveal::Blow); break;
        default: ShowResult(Reveal::Now); break;
    }
}

bool CrawlerGame::LooksSpent(const Prop& prop) const
{
    if (!prop.present || prop.fixed)
    {
        return false;
    }
    switch (PropIndex(prop))
    {
        case kAleBarrel: return false;                 // it only looks like a barrel either way
        case kFishBasket: return prop.state == 0;      // the fish are gone
        case kSentinel: return (prop.state & 1) != 0;  // the gem is out of its eye
        default: return !HasDeeds(prop);
    }
}

void CrawlerGame::DoDeedNow(const MenuEntry& entry)
{
    Prop* prop = m_ThingProp >= 0 ? &m_Props[m_ThingProp] : nullptr;
    Feature* feature = m_ThingFeature >= 0 ? &m_Features[m_ThingFeature] : nullptr;
    const Cell at = prop ? prop->at : feature ? feature->at : m_Hero;
    char text[kMessageLength];
    const auto heal = [this](int amount) { m_Hp = m_Hp + amount < m_MaxHp ? m_Hp + amount : m_MaxHp; };
    const auto mana = [this](int amount) { m_Mp = m_Mp + amount < m_MaxMp ? m_Mp + amount : m_MaxMp; };
    const auto hurt = [this](int amount) { m_Hp = m_Hp > amount ? m_Hp - amount : 1; };
    // Something comes out beside the thing: which monster, or null.
    const auto wake = [this, at]() -> Monster*
    {
        for (int f = 0; f < 4; ++f)
        {
            const Cell c = Ahead(at, f, 1);
            if (c != m_Hero && IsFree(c) && PlaceMonster(PickKind(), c))
            {
                return &m_Monsters[MonsterAt(c)];
            }
        }
        return nullptr;
    };
    After after = After::Done;
    switch (entry.deed)
    {
        case Deed::Search:
            SearchProp(*prop, GetThingName());
            after = After::All;
            break;
        case Deed::Drink:
            heal(4);
            m_Tipsy = m_Tipsy + 10 < 30 ? m_Tipsy + 10 : 30;
            prop->state = prop->state > 0 ? prop->state - 1 : 0;
            std::snprintf(text, sizeof(text),
                          prop->state > 0 ? "Thick and bitter. HP %d. The floor tilts a little."
                                          : "The last of it. HP %d. The barrel rings hollow.",
                          m_Hp);
            Say(text);
            after = prop->state > 0 ? After::Kept : After::Done;
            break;
        case Deed::Smash: BreakProp(*prop); return;
        case Deed::EatBerries:
            if (m_Random.Chance(80))
            {
                heal(6);
                std::snprintf(text, sizeof(text), "Sweet, then sharp. HP %d.", m_Hp);
            }
            else
            {
                heal(2);
                std::snprintf(text, sizeof(text), "Sour enough to cross your eyes. HP %d.", m_Hp);
            }
            Say(text);
            after = After::All;
            break;
        case Deed::PickBerries:
            after = Carry(ItemKind::Snack, 1, "A handful of berries. ") ? After::All : After::Kept;
            break;
        case Deed::Pray:
            m_Mp = m_MaxMp;
            Say("You kneel. The flames lean toward you. MP full.");
            break;
        case Deed::TakeOfferings:
        {
            const int found = m_Random.Range(6, 12) * m_Depth;
            m_Gold += found;
            const Monster* woken = m_Random.Chance(50) ? wake() : nullptr;
            if (woken)
            {
                std::snprintf(text, sizeof(text), "%d gold. The dead do mind: a %s!", found, StatsOf(woken->kind).name);
            }
            else
            {
                std::snprintf(text, sizeof(text), "%d gold. The candles gutter. Nothing comes.", found);
            }
            Say(text);
            break;
        }
        case Deed::OpenCoffin:
        {
            const Monster* inside = prop->state != 0 ? wake() : nullptr;
            prop->state = 0;
            if (inside)
            {
                // What it lay on is still in there, for after.
                std::snprintf(text, sizeof(text), "The lid grinds aside. A %s sits up.", StatsOf(inside->kind).name);
                Say(text);
                after = After::Kept;
                break;
            }
            Gain(PickLoot(true), "Under the shroud: ");
            after = After::All;
            break;
        }
        case Deed::Knock:
            Say(prop->state != 0 ? "You knock. Something inside knocks back." : "You knock. Nothing. Only dust.");
            break;
        case Deed::TouchCrystal:
            mana(6);
            std::snprintf(text, sizeof(text), "Cold runs up your arm and stays. MP %d.", m_Mp);
            Say(text);
            after = After::All;
            break;
        case Deed::BreakCrystal:
            m_SwingMs = kSwingMs;
            AddBreak(at, PropIndex(*prop));
            if (m_Random.Chance(50))
            {
                after = Carry(ItemKind::Gem, 1, "It cracks clean. ") ? After::All : After::Kept;
            }
            else
            {
                Say("It breaks into glitter. Nothing to carry.");
                after = After::All;
            }
            break;
        case Deed::EatMushroom:
            EatGlowcap();
            after = After::All;
            break;
        case Deed::PickMushrooms:
            after = Carry(ItemKind::Glowcap, 1, "Cut at the stem. ") ? After::All : After::Kept;
            break;
        case Deed::ReachForPearl:
            // A dagger's thin blade holds the shell open.
            if (m_Random.Chance(m_Weapon == Weapon::Dagger ? 75 : 55))
            {
                after = Carry(ItemKind::Pearl, 1, "Your fingers close on it. ") ? After::All : After::Kept;
            }
            else
            {
                hurt(3);
                std::snprintf(text, sizeof(text), "It snaps shut on your wrist. HP %d.", m_Hp);
                Say(text);
                after = After::Kept;
            }
            break;
        case Deed::Sharpen:
            ++m_Attack;
            std::snprintf(text, sizeof(text), "Sparks, then a clean edge. Atk %d.", m_Attack);
            Say(text);
            after = After::All;
            break;
        case Deed::MendArmour:
            ++m_Defense;
            std::snprintf(text, sizeof(text), "You beat the dents flat. Def %d.", m_Defense);
            Say(text);
            after = After::All;
            break;
        case Deed::Warm:
            heal(8);
            std::snprintf(text, sizeof(text), "The heat gets into your bones. HP %d.", m_Hp);
            Say(text);
            break;
        case Deed::Cook:
        {
            // As often as there is something to cook: a fish first, then a
            // glowcap (a stew that fills MP too), then a snack.
            after = After::Kept;
            const ItemKind food = Carried(ItemKind::Fish) > 0      ? ItemKind::Fish
                                  : Carried(ItemKind::Glowcap) > 0 ? ItemKind::Glowcap
                                                                   : ItemKind::Snack;
            if (Carried(food) <= 0)
            {
                Say("Nothing to cook. The coals wait.");
                break;
            }
            --m_Bag[static_cast<int>(food)];
            m_Hp = m_MaxHp;
            if (food == ItemKind::Glowcap)
            {
                m_Mp = m_MaxMp;
                Say("It stews down blue, and safe. HP and MP full.");
                break;
            }
            std::snprintf(text, sizeof(text),
                          food == ItemKind::Fish ? "The skin crackles. A real meal. HP %d."
                                                 : "It spits and browns. A real meal. HP %d.",
                          m_Hp);
            Say(text);
            break;
        }
        case Deed::TakeFirst:
        case Deed::TakeSecond:
            if (IsWeaponItem(entry.item))
            {
                after = Carry(entry.item, 1, "Off the rack: ") ? After::All : After::Kept;
            }
            else
            {
                Gain(entry.item, "Off the rack: ");  // a whetstone
                after = After::All;
            }
            break;
        case Deed::Read: ReadBook(); break;
        case Deed::SearchShelf:
        {
            const int roll = m_Random.Range(0, 99);
            if (roll < 30)
            {
                Carry(ItemKind::Key, 1, "Behind the books: ");
            }
            else if (roll < 55)
            {
                Carry(ItemKind::Lockpick, 1, "Behind the books: ");
            }
            else if (roll < 75)
            {
                Gain(ItemKind::Gold, "Behind the books: ");
            }
            else
            {
                Say("Behind the books: dust, and a dead spider.");
            }
            break;
        }
        case Deed::Sleep: Sleep(); break;
        case Deed::SearchCamp:
        {
            const int roll = m_Random.Range(0, 99);
            if (roll < 40)
            {
                Carry(ItemKind::Snack, 1, "In the bedroll: ");
            }
            else if (roll < 65)
            {
                Carry(ItemKind::Potion, 1, "In the bedroll: ");
            }
            else if (roll < 85)
            {
                Gain(ItemKind::Gold, "In the bedroll: ");
            }
            else
            {
                Say("Ashes and a cracked cup. Nothing worth taking.");
            }
            break;
        }
        case Deed::PickLock: after = PickLock() ? After::All : After::Kept; break;
        case Deed::ForceLock:
        {
            // It opens, but the good things in it do not survive.
            const int found = m_Random.Range(8, 14) * m_Depth;
            m_Gold += found;
            m_SwingMs = kSwingMs;
            std::snprintf(text, sizeof(text), "The lid splits. %d gold, and broken glass.", found);
            Say(text);
            after = After::All;
            break;
        }
        case Deed::ReachIn:
            if (m_Random.Chance(15))
            {
                hurt(2);
                std::snprintf(text, sizeof(text), "Something in there bites. HP %d.", m_Hp);
                Say(text);
                after = After::Kept;
            }
            else
            {
                LootContainer("You fish it out: ", "Your hand finds the bottom. Nothing.");
                after = After::All;
            }
            break;
        case Deed::DrinkDeep: DrinkFountain(*feature); break;
        case Deed::Wash:
            heal(8);
            feature->used = true;
            std::snprintf(text, sizeof(text), "Cold and clean. The sting goes out. HP %d.", m_Hp);
            Say(text);
            break;
        case Deed::PryGem:
            if (!Carry(ItemKind::Gem, 1, "It comes free. "))
            {
                after = After::Kept;
                break;
            }
            prop->state = 1;  // the eye is empty
            if (m_Random.Chance(45))
            {
                hurt(4);
                m_ShakeMs = kShakeMs;
                std::snprintf(text, sizeof(text), "The gem comes free. So does his sword arm. HP %d.", m_Hp);
                Say(text);
            }
            after = After::All;
            break;
        case Deed::Bow:
            m_MaxHp += 2;
            m_Hp += 2;
            std::snprintf(text, sizeof(text), "You bow. Stone creaks. You stand taller. Max HP %d.", m_MaxHp);
            Say(text);
            after = After::All;
            break;
        case Deed::ReadStone: Say(kEpitaphs[m_Random.Range(0, kEpitaphCount - 1)]); break;
        case Deed::Dig:
        {
            const Monster* risen = m_Depth > 1 && m_Random.Chance(35) ? wake() : nullptr;
            if (risen)
            {
                std::snprintf(text, sizeof(text), "Your blade hits wood. The wood hits back: a %s!",
                              StatsOf(risen->kind).name);
                Say(text);
            }
            else if (m_Random.Chance(50))
            {
                Gain(PickLoot(true), "In the grave: ");
            }
            else
            {
                Gain(static_cast<ItemKind>(static_cast<int>(ItemKind::Moss) + GetZone()), "In the grave: ");
            }
            after = After::All;
            break;
        }
        case Deed::TakeFish:
            if (!Carry(ItemKind::Fish, 1, "Cold and slippery. "))
            {
                after = After::Kept;
                break;
            }
            prop->state = prop->state > 0 ? prop->state - 1 : 0;
            after = prop->state > 0 ? After::Kept : After::Done;
            break;
        case Deed::DigBasket:
        {
            const int roll = m_Random.Range(0, 99);
            if (roll < 35)
            {
                Gain(ItemKind::Shell, "At the bottom: ");
            }
            else if (roll < 55)
            {
                Carry(ItemKind::Pearl, 1, "At the bottom: ");
            }
            else if (roll < 75)
            {
                Gain(ItemKind::Gold, "At the bottom: ");
            }
            else
            {
                Say("Scales and seaweed. Your hands stink.");
            }
            break;
        }
        case Deed::SearchCart:
        {
            const int roll = m_Random.Range(0, 99);
            if (roll < 60)
            {
                Gain(ItemKind::Ore, "Among the slag: ");
            }
            else if (roll < 80)
            {
                Carry(ItemKind::Gem, 1, "Among the slag: ");
            }
            else
            {
                Say("Slag. All of it slag.");
            }
            break;
        }
        case Deed::PushCart:
        {
            // It rolls off the way the hero faces, into whatever is there.
            constexpr int kRoll = 8;
            int hit = -1;
            for (int i = 1; i <= kRoll && hit < 0; ++i)
            {
                const Cell c = Ahead(at, m_Facing, i);
                if (!m_Map.IsWalkable(c) || PropAt(c) >= 0 || FeatureAt(c) >= 0)
                {
                    break;
                }
                hit = MonsterAt(c);
            }
            AddBreak(at, PropIndex(*prop));
            prop->present = false;
            m_ShakeMs = kShakeMs;
            m_Map.ComputeDistances(m_Hero);
            if (hit >= 0)
            {
                HurtMonster(hit, 10 + 2 * m_Depth, "The cart ploughs into the");
            }
            else
            {
                Say("It rattles off into the dark and hits something solid.");
            }
            ++m_Version;
            return;
        }
        case Deed::FeedCage:
        {
            int& food = Carried(ItemKind::Snack) > 0 ? m_Bag[static_cast<int>(ItemKind::Snack)]
                                                     : m_Bag[static_cast<int>(ItemKind::Fish)];
            if (food <= 0)
            {
                Say("You have nothing it would eat.");
                after = After::Kept;
                break;
            }
            --food;
            Say((prop->state & 1) != 0 ? "It takes the food. Then it tries for the fingers."
                                       : "It eats from your hand. Something like a purr.");
            prop->state = static_cast<uint8_t>(prop->state | 2);  // fed
            break;
        }
        case Deed::OpenCage:
        {
            const bool fed = (prop->state & 2) != 0;
            if ((prop->state & 1) != 0)
            {
                Monster* out = wake();
                if (out)
                {
                    // Fed, it comes out slow.
                    out->frozen = fed ? 3 : 0;
                    std::snprintf(text, sizeof(text),
                                  fed ? "A %s comes out slow, still chewing." : "A %s comes out teeth first.",
                                  StatsOf(out->kind).name);
                    Say(text);
                    after = After::All;
                    break;
                }
            }
            // A friend: it leaves something, more for having been fed.
            constexpr ItemKind kThanks[] = { ItemKind::Heart, ItemKind::Elixir, ItemKind::Gem };
            Gain(fed ? kThanks[m_Random.Range(0, 2)] : PickLoot(true), "It bolts, and leaves a gift: ");
            after = After::All;
            break;
        }
        case Deed::Prod:
            if (prop->state == 0)
            {
                Say("Wood. Only wood.");
                break;
            }
            // No chest: it runs, and drops what it kept.
            AddBreak(at, PropIndex(*prop));
            prop->present = false;
            m_Map.ComputeDistances(m_Hero);
            Gain(PickLoot(true), "It yelps and runs. It dropped: ");
            ++m_Version;
            return;
        case Deed::OpenChest:
            if (prop->state != 0)
            {
                hurt(4 + m_Depth / 3);
                AddBreak(at, PropIndex(*prop));
                prop->present = false;
                m_ShakeMs = kShakeMs;
                m_Map.ComputeDistances(m_Hero);
                std::snprintf(text, sizeof(text), "The lid has teeth. HP %d. It scuttles off.", m_Hp);
                Say(text);
                ++m_Version;
                return;
            }
            Gain(PickLoot(true), "Inside: ");
            after = After::All;
            break;
        case Deed::LookMirror:
        {
            const int roll = m_Random.Range(0, 99);
            if (roll < 30)
            {
                m_Map.RevealAll();
                Say("The glass shows the whole floor, as if from above.");
            }
            else if (roll < 55)
            {
                m_MaxMp += 2;
                m_Mp += 2;
                std::snprintf(text, sizeof(text), "Your reflection nods to you. Max MP %d.", m_MaxMp);
                Say(text);
            }
            else if (roll < 80)
            {
                hurt(3);
                std::snprintf(text, sizeof(text), "Your reflection blinks first. Then it smiles. HP %d.", m_Hp);
                Say(text);
            }
            else if (const Monster* out = wake())
            {
                std::snprintf(text, sizeof(text), "It was not your reflection. A %s steps out.",
                              StatsOf(out->kind).name);
                Say(text);
            }
            else
            {
                Say("Only your own face, older than you remember.");
            }
            after = After::All;
            break;
        }
        case Deed::BreakMirror:
            AddBreak(at, PropIndex(*prop));
            prop->present = false;
            m_SwingMs = kSwingMs;
            m_Map.ComputeDistances(m_Hero);
            hurt(1);
            Gain(ItemKind::Gold, "Silver from behind the glass: ");
            ++m_Version;
            return;
        case Deed::Eat:
        {
            const ItemKind food = static_cast<ItemKind>(m_ThingItem);
            if (UseItem(food) != 0)
            {
                --m_Bag[m_ThingItem];
                TurnEnds();
            }
            return;
        }
        case Deed::Throw: ThrowFood(static_cast<ItemKind>(m_ThingItem)); return;
        case Deed::FillSkin:
            if (!Carry(ItemKind::Water, 1, "It gurgles full. "))
            {
                after = After::Kept;
                break;
            }
            feature->used = true;
            break;
        case Deed::PrayStone: Pray(*feature); break;
        case Deed::GiveName:
        {
            // The names the hero knows, one a time: each gives once a run.
            const int name = NextName();
            m_WordsUsed = static_cast<uint16_t>(m_WordsUsed | (name >= 0 && name < 3 ? 1u << name : 0u));
            if (name == 0)
            {
                Carry(ItemKind::Snack, 1, "\"Puddle.\" The stone gives up a snack. ");
            }
            else if (name == 1)
            {
                m_Mp = m_MaxMp;
                Say("\"Halvard.\" It answers: he is below the below. MP full.");
            }
            else if (name == 2)
            {
                m_MaxMp += 2;
                m_Mp = m_MaxMp;
                std::snprintf(text, sizeof(text), "\"Sigrun.\" Green light, for a moment. Max MP %d.", m_MaxMp);
                Say(text);
            }
            else
            {
                // The three carved ducks' names: the dungeon dances, or stops.
                m_Legend.disco = !m_Legend.disco;
                ++m_LegendVersion;
                Say(m_Legend.disco ? "The ducks' names. The whole dungeon starts to dance." : "The dancing stops.");
            }
            break;
        }
        case Deed::SayHeartName:
            if (m_Shards >= Lore::kShards)
            {
                EnterSeam();
                return;
            }
            Say("The stone shivers: bring it his heart, whole.");
            break;
        case Deed::None: break;
    }
    if (prop && after == After::Done)
    {
        prop->done = static_cast<uint8_t>(prop->done | (1u << entry.price));
    }
    else if (prop && after == After::All)
    {
        prop->done = 0xFF;
    }
    ++m_Version;
}

}  // namespace Dungeon
