// Things to use: the bag, the shop and the other menus, what is picked up, the
// props and what using each does, books, notes, bricks, echo stones and the
// stone listeners' prayers.

#include "CrawlerGame.h"

#include "Contracts.h"
#include "CrawlerGameData.h"
#include "Village.h"

#include <cstdio>
#include <cstring>

namespace Dungeon
{

using namespace GameData;

void CrawlerGame::ShowControls()
{
    m_Legend.controlsShown = true;
    ++m_LegendVersion;
    OpenMenu(Menu::Controls);
}

void CrawlerGame::BuyMaterial(int material)
{
    // Paid from the village's gold, straight into its store.
    const int price = FencePrice(material);
    if (material < 0 || material >= Village::kMaterials || m_Legend.bank < price)
    {
        Sound(Cue::Error);
        Say("Ulf: Come back with coin.");
        return;
    }
    m_Legend.bank -= price;
    ++m_Legend.materials[material];
    ++m_LegendVersion;
    Sound(Cue::Buy);
    char text[kMessageLength];
    std::snprintf(text, sizeof(text), "Ulf hands it over. The village has %d %s now.", m_Legend.materials[material],
                  Village::MaterialName(material));
    Say(text);
    FillMenu();
    ++m_Version;
}

void CrawlerGame::OpenMenu(Menu menu)
{
    m_Menu = menu;
    m_MenuCursor = 0;
    FillMenu();
    ++m_Version;
}

void CrawlerGame::FillMenu()
{
    m_MenuCount = 0;
    if (m_Menu == Menu::Bag)
    {
        for (ItemKind kind : kBagKinds)
        {
            if (m_Bag[static_cast<int>(kind)] > 0 && m_MenuCount < kMaxMenu)
            {
                // A line of its own: nothing left on it from the menu before.
                MenuEntry& e = m_MenuEntries[m_MenuCount++];
                e = MenuEntry{};
                e.item = kind;
                e.count = m_Bag[static_cast<int>(kind)];
            }
        }
    }
    else if (m_Menu == Menu::Shop)
    {
        for (int i = 0; i < m_GoodsCount; ++i)
        {
            m_MenuEntries[m_MenuCount++] = m_Goods[i];
        }
        // Puddle wants snacks: a line to give him one.
        if (m_Bag[static_cast<int>(ItemKind::Snack)] > 0 && !m_HasStaff && m_MenuCount < kMaxMenu)
        {
            MenuEntry& e = m_MenuEntries[m_MenuCount++];
            e = MenuEntry{};
            e.item = ItemKind::Snack;
            e.count = m_SnacksGiven;
            e.label = "Feed Puddle";
        }
        // A word with him.
        if (m_MenuCount < kMaxMenu)
        {
            MenuEntry& e = m_MenuEntries[m_MenuCount++];
            e = MenuEntry{};
            e.item = ItemKind::Note;
            e.label = "Talk";
        }
        // And he buys anything shiny.
        const int shiny = Carried(ItemKind::Gem) + Carried(ItemKind::Goblet) + Carried(ItemKind::Pearl);
        if (shiny > 0 && m_MenuCount < kMaxMenu)
        {
            MenuEntry& e = m_MenuEntries[m_MenuCount++];
            e = MenuEntry{};
            e.item = ItemKind::Gem;
            e.count = shiny;
            e.price = ValuablesWorth();
            e.label = "Sell valuables";
        }
    }
    else if (m_Menu == Menu::Thing)
    {
        FillThingMenu();
    }
    else if (m_Menu == Menu::Contracts)
    {
        FillContractsMenu();
    }
    else if (m_Menu == Menu::Notes)
    {
        // What was read, to read again: this run's notes (price 0) and
        // books (1), then the diary's pages (2), all of them ever found.
        constexpr const char* kPageNames[kLorePages] = { "Diary, page 1",  "Diary, page 2",  "Diary, page 3",
                                                         "Diary, page 4",  "Diary, page 5",  "Diary, page 6",
                                                         "Diary, page 7",  "Diary, page 8",  "Diary, page 9",
                                                         "Diary, page 10", "Diary, page 11", "Diary, page 12" };
        const auto add = [&](const char* label, ItemKind icon, int kind, int which)
        {
            if (m_MenuCount < kMaxMenu)
            {
                MenuEntry& e = m_MenuEntries[m_MenuCount++];
                e = MenuEntry{};
                e.label = label;
                e.item = icon;
                e.price = kind;
                e.count = which;
            }
        };
        for (int i = m_NoteCount - 1; i >= 0; --i)
        {
            add(m_Notes[i]->title, ItemKind::Note, 0, i);
        }
        for (int b = 0; b < Lore::kBooks; ++b)
        {
            if ((m_BooksRead & (1u << b)) != 0)
            {
                add(Lore::GetBook(b).title, ItemKind::Scroll, 1, b);
            }
        }
        for (int page = 1; page <= m_PagesKnown && page <= kLorePages; ++page)
        {
            add(kPageNames[page - 1], ItemKind::Page, 2, page);
        }
    }
    else if (m_Menu == Menu::Descend)
    {
        // One line to go; the line under it stays.
        MenuEntry& e = m_MenuEntries[m_MenuCount++];
        e = MenuEntry{};
        e.label = m_InVillage ? (m_DescendTo > 1 ? "Ride down" : "Climb down") : "Go down";
        e.count = 1;
    }
    else if (m_Menu == Menu::Fence)
    {
        // What he has: the materials of the places reached, a price each;
        // and a word with him.
        for (int m = 0; m < Village::kMaterials; ++m)
        {
            if (m_Deepest >= Village::MaterialDepth(m))
            {
                MenuEntry& e = m_MenuEntries[m_MenuCount++];
                e = MenuEntry{};
                e.item = static_cast<ItemKind>(static_cast<int>(ItemKind::Moss) + m);
                e.price = FencePrice(m);
                e.count = m_Legend.bank >= e.price ? 1 : 0;
            }
        }
        MenuEntry& e = m_MenuEntries[m_MenuCount++];
        e = MenuEntry{};
        e.item = ItemKind::Note;
        e.label = "Talk";
    }
    else if (m_Menu == Menu::Main)
    {
        // The bag, the magic, the way home, the keys: which line in price,
        // whether it can be taken now in count.
        constexpr const char* kLines[7] = { "Bag",       "Magic", "Return to Last Hearth", "Controls", "Sound",
                                            "Contracts", "Notes" };
        constexpr ItemKind kIcons[7] = { ItemKind::Potion, ItemKind::Scroll, ItemKind::Wing, ItemKind::Key,
                                         ItemKind::Duck,   ItemKind::Note,   ItemKind::Page };
        for (int i = 0; i < 7; ++i)
        {
            MenuEntry& e = m_MenuEntries[m_MenuCount++];
            e = MenuEntry{};
            e.label = kLines[i];
            e.item = kIcons[i];
            e.price = i;
            e.count = i != 2 || CanReturnHome() ? 1 : 0;
        }
    }
    else if (m_Menu == Menu::Keys)
    {
        // A line for each key that can be changed, and one to put them
        // back; the Crawler knows the keys and draws them.
        constexpr const char* kLines[] = { "Forward",    "Back", "Turn left", "Turn right", "Step left",
                                           "Step right", "Use",  "Wait",      "Menu",       "Put them back" };
        for (const char* line : kLines)
        {
            MenuEntry& e = m_MenuEntries[m_MenuCount++];
            e = MenuEntry{};
            e.label = line;
            e.item = ItemKind::Key;
            e.count = 1;
        }
    }
    else if (m_Menu == Menu::Ending)
    {
        m_MenuEntries[0] = MenuEntry{};
        m_MenuEntries[0].label = "Free Halvard";
        m_MenuEntries[0].item = ItemKind::Shard;
        m_MenuEntries[1] = MenuEntry{};
        m_MenuEntries[1].label = "Take the staff";
        m_MenuEntries[1].spell = static_cast<int>(Spell::Fold);
        m_MenuCount = 2;
    }
    else if (m_Menu == Menu::Magic)
    {
        for (int s = 0; s < kSpells && m_MenuCount < kMaxMenu; ++s)
        {
            if (KnowsSpell(static_cast<Spell>(s)))
            {
                MenuEntry& e = m_MenuEntries[m_MenuCount++];
                e = MenuEntry{};
                e.spell = s;
                e.price = SpellCost(static_cast<Spell>(s));
            }
        }
    }
    if (m_MenuCursor > m_MenuCount)
    {
        m_MenuCursor = m_MenuCount;
    }
}

bool CrawlerGame::Carry(ItemKind kind, int count, const char* from)
{
    int& held = m_Bag[static_cast<int>(kind)];
    char text[kMessageLength];
    if (held + count > kMaxCarried)
    {
        std::snprintf(text, sizeof(text), "%sNo room for another %s.", from, ItemName(kind));
        Say(text);
        return false;
    }
    held += count;
    m_Gained = static_cast<int>(kind);
    std::snprintf(text, sizeof(text), "%s+1 %s.", from, ItemName(kind));
    Say(text);
    return true;
}

int CrawlerGame::UseItem(ItemKind kind)
{
    char text[kMessageLength];
    switch (kind)
    {
        case ItemKind::Potion:
        {
            if (m_Hp >= m_MaxHp)
            {
                Say("You are already at full HP.");
                return 0;
            }
            const int heal = 10 + m_Depth;
            m_Hp = m_Hp + heal < m_MaxHp ? m_Hp + heal : m_MaxHp;
            Sound(Cue::Drink);
            std::snprintf(text, sizeof(text), "It burns going down. HP %d/%d.", m_Hp, m_MaxHp);
            Say(text);
            return 1;
        }
        case ItemKind::Elixir:
        {
            m_MaxHp += 3;
            m_Hp = m_MaxHp;
            m_Mp = m_MaxMp;
            std::snprintf(text, sizeof(text), "Light under your skin. Full HP, max HP %d.", m_MaxHp);
            Say(text);
            return 1;
        }
        case ItemKind::Bomb:
        {
            Blast();
            return 1;
        }
        case ItemKind::Scroll:
        {
            m_Map.RevealAll();
            m_KnowsSecrets = true;
            for (int y = 0; y < kMapHeight; ++y)
            {
                for (int x = 0; x < kMapWidth; ++x)
                {
                    if (m_Traps[y][x] != 0)
                    {
                        m_Traps[y][x] |= kFound;
                        m_Map.SetLook({ x, y }, (m_Traps[y][x] & ~kFound) == kTrapdoor ? FloorLook::StairsHole
                                                : (m_Traps[y][x] & ~kFound) == kSpikes ? FloorLook::Spikes
                                                                                       : FloorLook::Rune);
                    }
                }
            }
            Say("Ink crawls over the page: this floor, and what it hides.");
            return 1;
        }
        case ItemKind::Key:
        {
            Say("A key is for a locked door.");
            return 0;
        }
        case ItemKind::Lockpick:
        {
            Say("A lockpick is for a locked chest.");
            return 0;
        }
        case ItemKind::Gem:
        case ItemKind::Pearl:
        case ItemKind::Goblet:
        {
            Say("Pretty. Puddle would pay well for it.");
            return 0;
        }
        case ItemKind::Water:
        {
            if (m_Hp >= m_MaxHp && m_Mp >= m_MaxMp)
            {
                Say("You are not thirsty.");
                return 0;
            }
            m_Hp = m_Hp + 12 < m_MaxHp ? m_Hp + 12 : m_MaxHp;
            m_Mp = m_Mp + 4 < m_MaxMp ? m_Mp + 4 : m_MaxMp;
            m_Tipsy = 0;
            std::snprintf(text, sizeof(text), "Cold all the way down. HP %d/%d, MP %d.", m_Hp, m_MaxHp, m_Mp);
            Say(text);
            return 1;
        }
        case ItemKind::Glowcap:
        {
            EatGlowcap();
            return 1;
        }
        case ItemKind::Vial:
        {
            // Nobody wrote on it what it was.
            const int roll = m_Random.Range(0, 99);
            if (roll < 30)
            {
                m_Hp = m_Hp + 15 < m_MaxHp ? m_Hp + 15 : m_MaxHp;
                std::snprintf(text, sizeof(text), "Warm, like broth. HP %d/%d.", m_Hp, m_MaxHp);
            }
            else if (roll < 50)
            {
                m_Mp = m_MaxMp;
                std::snprintf(text, sizeof(text), "It fizzes behind your eyes. MP full.");
            }
            else if (roll < 65)
            {
                ++m_MaxHp;
                ++m_Hp;
                std::snprintf(text, sizeof(text), "Bitter. Then your heart beats harder. Max HP %d.", m_MaxHp);
            }
            else if (roll < 80)
            {
                m_Tipsy = 20;
                std::snprintf(text, sizeof(text), "Strong drink, whatever it was. The walls lean.");
            }
            else
            {
                m_Hp = m_Hp > 5 ? m_Hp - 5 : 1;
                std::snprintf(text, sizeof(text), "It burns all the way down. HP %d/%d.", m_Hp, m_MaxHp);
            }
            Say(text);
            return 1;
        }
        case ItemKind::Fish:
        {
            m_Hp = m_Hp + 4 < m_MaxHp ? m_Hp + 4 : m_MaxHp;
            std::snprintf(text, sizeof(text), "Raw, cold, and it is food. HP %d/%d.", m_Hp, m_MaxHp);
            Say(text);
            return 1;
        }
        case ItemKind::Snack:
        {
            m_Hp = m_Hp + 6 < m_MaxHp ? m_Hp + 6 : m_MaxHp;
            std::snprintf(text, sizeof(text), "Gone in two bites. HP %d/%d. Puddle need not know.", m_Hp, m_MaxHp);
            Say(text);
            return 1;
        }
        case ItemKind::Blade:
        case ItemKind::Axe:
        case ItemKind::Spear:
        case ItemKind::Dagger:
        case ItemKind::Hammer:
        {
            // The one in hand goes back into the bag.
            ++m_Bag[static_cast<int>(WeaponItem(m_Weapon))];
            m_Weapon = static_cast<Weapon>(static_cast<int>(kind) - static_cast<int>(ItemKind::Blade));
            std::snprintf(text, sizeof(text), "You wield the %s.", ItemName(kind));
            Say(text);
            return 1;
        }
        case ItemKind::Wing:
        {
            if (m_InSeam)
            {
                Say("There is no down from here.");
                return 0;
            }
            if (IsBossFloor() && BossAlive())
            {
                Say("Something on this floor will not let you leave.");
                return 0;
            }
            EnterFloor(m_Depth + 1);
            std::snprintf(text, sizeof(text), "The wing lifts you, and lets go. B%d.", m_Depth);
            Say(text);
            return 2;
        }
        default: return 0;
    }
}

void CrawlerGame::ChooseInMenu()
{
    if (m_MenuCursor >= m_MenuCount)
    {
        m_Menu = Menu::None;
        ++m_Version;
        return;
    }
    const MenuEntry entry = m_MenuEntries[m_MenuCursor];
    if (m_Menu == Menu::Main)
    {
        switch (entry.price)
        {
            case 0: OpenMenu(Menu::Bag); break;
            case 1: OpenMenu(Menu::Magic); break;
            case 2:
                if (CanReturnHome())
                {
                    m_Menu = Menu::None;
                    ReturnHome();
                }
                break;
            case 3: OpenMenu(Menu::Keys); break;
            case 5: OpenMenu(Menu::Contracts); break;
            case 6: OpenMenu(Menu::Notes); break;
            default:
            {
                // How loud: a step up, and round to silence.
                constexpr int kSteps[] = { 0, 25, 50, 75, 100 };
                int next = 0;
                for (const int step : kSteps)
                {
                    if (step > m_SoundVolume)
                    {
                        next = step;
                        break;
                    }
                }
                m_SoundVolume = next;
                ++m_Version;
                break;
            }
        }
        return;
    }
    if (m_Menu == Menu::Keys)
    {
        return;  // the Crawler's: it knows the keys
    }
    if (m_Menu == Menu::Contracts)
    {
        // What was said of it, again: the asking while it is open, the
        // thanks once it is kept.
        const Contracts::Info& info = Contracts::GetInfo(static_cast<Contracts::Id>(entry.price));
        const Contracts::State state = static_cast<Contracts::State>(entry.count);
        if (state == Contracts::State::Kept)
        {
            SpeakLines(info.back, Contracts::kBackLines, 0);
        }
        else if (state != Contracts::State::Unasked)
        {
            SpeakLines(info.offer, Contracts::kOfferLines, 0);
        }
        return;
    }
    if (m_Menu == Menu::Notes)
    {
        // Opened again: a note or a book on its sheet, a page of the diary.
        if (entry.price == 2)
        {
            m_PageShown = entry.count;
            m_Menu = Menu::Page;
        }
        else
        {
            m_Reading = entry.price == 0 ? m_Notes[entry.count] : &Lore::GetBook(entry.count);
            m_Menu = Menu::Book;
        }
        ++m_Version;
        return;
    }
    if (m_Menu == Menu::Descend)
    {
        Descend();
        return;
    }
    if (m_Menu == Menu::Fence)
    {
        if (entry.label)
        {
            // What he asks or owes first; now and then what is still to do.
            const int talks = m_FenceTalks++;
            if (!TalkOfContract(static_cast<int>(Contracts::Giver::Ulf), talks % 3 == 1))
            {
                Speak(Village::FenceLine(talks));
            }
            return;
        }
        BuyMaterial(static_cast<int>(entry.item) - static_cast<int>(ItemKind::Moss));
        return;
    }
    if (m_Menu == Menu::Thing)
    {
        // The menu goes first: a deed may open another (a book).
        m_Menu = Menu::None;
        ++m_Version;
        DoDeed(entry);
        return;
    }
    if (m_Menu == Menu::Magic)
    {
        if (!CastSpell(static_cast<Spell>(entry.spell)))
        {
            return;  // not enough MP: the menu stays
        }
        m_Menu = Menu::None;
        ++m_Version;
        TurnEnds();
        return;
    }
    if (m_Menu == Menu::Bag && IsFood(entry.item))
    {
        UseFood(entry.item);  // eat it, or throw it
        return;
    }
    if (m_Menu == Menu::Bag)
    {
        const int used = UseItem(entry.item);
        if (used == 0)
        {
            m_Menu = Menu::None;
            ++m_Version;
            return;
        }
        --m_Bag[static_cast<int>(entry.item)];
        if (used == 2)
        {
            return;  // a new floor: the menu is already gone
        }
        m_Menu = Menu::None;
        ++m_Version;
        TurnEnds();
        return;
    }

    // The shop.
    if (entry.label && entry.item == ItemKind::Note)
    {
        // A word with Puddle: he smells a snack, or passes on what he heard.
        m_Menu = Menu::None;
        if (Contracts::GetState(m_Legend, Contracts::Id::Meals) == Contracts::State::Taken)
        {
            // Gunnar's twelve meals: he pays, after his fashion.
            Contracts::SetState(m_Legend, Contracts::Id::Meals, Contracts::State::Found);
            ++m_LegendVersion;
            Speak("Puddle: Gunnar? Twelve? I counted nine. Here. Take it.");
            return;
        }
        Speak(Carried(ItemKind::Snack) > 0 && !m_HasStaff ? "Puddle: You smell of snacks. Good. Sit."
                                                          : kRumours[m_Random.Range(0, kRumourCount - 1)]);
        return;
    }
    if (entry.label && entry.item == ItemKind::Snack)
    {
        GiveSnack();
        return;
    }
    if (entry.label && entry.item == ItemKind::Gem)
    {
        SellValuables();
        return;
    }
    MenuEntry& good = m_Goods[m_MenuCursor];
    char text[kMessageLength];
    if (good.count <= 0)
    {
        Sound(Cue::Error);
        Say("Puddle: That one is gone. Somebody was quicker.");
        return;
    }
    if (m_Gold < good.price)
    {
        Sound(Cue::Error);
        std::snprintf(text, sizeof(text), "Puddle: That is %d gold. You hold %d.", good.price, m_Gold);
        Say(text);
        return;
    }
    if (GoesInBag(good.item))
    {
        if (!Carry(good.item, 1, "Puddle: Thanks! "))
        {
            return;
        }
    }
    else
    {
        Gain(good.item, "Puddle: Thanks! ");
    }
    m_Gold -= good.price;
    Sound(Cue::Buy);
    --good.count;
    FillMenu();
    ++m_Version;
}

void CrawlerGame::ReadPage()
{
    if (m_PagesKnown >= kLorePages)
    {
        // Every page read: a known one, worth a little experience.
        Say("A page you have read before. +5 XP");
        GainXp(5);
        return;
    }
    ++m_PagesKnown;
    m_PageShown = m_PagesKnown;
    Say("A torn page, in a small neat hand.");
    OpenMenu(Menu::Page);
}

void CrawlerGame::Pray(Feature& shrine)
{
    if (shrine.used)
    {
        Say("The stone is silent.");
        return;
    }
    shrine.used = true;
    char text[kMessageLength];
    const int roll = m_Random.Range(0, 99);
    if (roll < 28)
    {
        ++m_Attack;
        std::snprintf(text, sizeof(text), "The statue smiles. Atk %d!", m_Attack);
    }
    else if (roll < 52)
    {
        ++m_Defense;
        std::snprintf(text, sizeof(text), "The statue nods. Def %d!", m_Defense);
    }
    else if (roll < 78)
    {
        m_MaxHp += 2;
        m_Hp = m_MaxHp;
        std::snprintf(text, sizeof(text), "Warm light. Full HP, max %d!", m_MaxHp);
    }
    else
    {
        const int lost = m_Gold / 3;
        m_Gold -= lost;
        std::snprintf(text, sizeof(text), "The statue frowns. -%d gold...", lost);
    }
    Say(text);
}

void CrawlerGame::DrinkFountain(Feature& fountain)
{
    if (fountain.used)
    {
        Say("The basin is dry.");
        return;
    }
    fountain.used = true;
    char text[kMessageLength];
    const int roll = m_Random.Range(0, 99);
    if (roll < 35)
    {
        m_Hp = m_MaxHp;
        m_Mp = m_MaxMp;
        Say("Cold, sweet water. HP and MP full.");
    }
    else if (roll < 55)
    {
        m_MaxHp += 3;
        m_Hp += 3;
        std::snprintf(text, sizeof(text), "You feel stronger! Max HP %d.", m_MaxHp);
        Say(text);
    }
    else if (roll < 70)
    {
        const int found = m_Random.Range(10, 20) * m_Depth;
        m_Gold += found;
        std::snprintf(text, sizeof(text), "Your hand closes on coins. %d gold.", found);
        Say(text);
    }
    else if (roll < 85)
    {
        m_Hp = m_Hp > 3 ? m_Hp - 3 : 1;
        Say("It tastes of pennies and rot. -3 HP.");
    }
    else
    {
        for (int f = 0; f < 4; ++f)
        {
            const Cell c = Ahead(fountain.at, f, 1);
            if (IsFree(c) && PlaceMonster(MonsterKind::Glimmerjelly, c))
            {
                Say("A gilt wisp slips out of the water!");
                return;
            }
        }
        m_Hp = m_MaxHp;
        Say("Cold, sweet water. HP full.");
    }
}

void CrawlerGame::Gain(ItemKind kind, const char* from)
{
    if (GoesInBag(kind))
    {
        Carry(kind, 1, from);
        return;
    }
    if (kind == ItemKind::Page)
    {
        ReadPage();
        return;
    }
    if (kind == ItemKind::Note)
    {
        ReadNote();
        return;
    }
    char text[kMessageLength];
    m_Gained = static_cast<int>(kind);
    switch (kind)
    {
        case ItemKind::Gold:
        {
            const int found = m_Random.Range(3, 9) * m_Depth;
            m_Gold += found;
            std::snprintf(text, sizeof(text), "%s%d gold.", from, found);
            break;
        }
        case ItemKind::Sword:
        {
            ++m_Attack;
            std::snprintf(text, sizeof(text), "%sA sharper sword! Atk %d.", from, m_Attack);
            break;
        }
        case ItemKind::Shield:
        {
            ++m_Defense;
            std::snprintf(text, sizeof(text), "%sA sturdier shield! Def %d.", from, m_Defense);
            break;
        }
        case ItemKind::Heart:
        {
            m_MaxHp += 5;
            m_Hp += 5;
            std::snprintf(text, sizeof(text), "%sA heart crystal! Max HP %d.", from, m_MaxHp);
            break;
        }
        case ItemKind::Chest:
        {
            Gain(PickLoot(true), "Chest: ");
            return;
        }
        case ItemKind::Moss:
        case ItemKind::Bone:
        case ItemKind::Shell:
        case ItemKind::Ore:
        {
            const int material = static_cast<int>(kind) - static_cast<int>(ItemKind::Moss);
            ++m_Materials[material];
            std::snprintf(text, sizeof(text), "%s+1 %s for the village (%d).", from, Village::MaterialName(material),
                          m_Materials[material]);
            break;
        }
        case ItemKind::Shard:
        {
            m_ShardBits = static_cast<uint8_t>(m_ShardBits | (1 << GetZone()));
            ++m_Shards;
            std::snprintf(text, sizeof(text), "%sA heart shard! It hums a letter: %c (%d of 4)", from,
                          Lore::ShardLetter(m_Shards - 1), m_Shards);
            break;
        }
        case ItemKind::Duck:
        {
            const int duck = (GetZone() + Lore::kDucks - 1) % Lore::kDucks;
            m_Legend.ducks = static_cast<uint8_t>(m_Legend.ducks | (1 << duck));
            ++m_LegendVersion;
            const int found = Lore::CountBits(m_Legend.ducks);
            if (found >= Lore::kDucks)
            {
                std::snprintf(text, sizeof(text), "%sThe third duck. All three. Tell a Listener.", from);
            }
            else
            {
                std::snprintf(text, sizeof(text), "%sA carved duck, worn smooth by small hands. (%d of 3)", from,
                              found);
            }
            break;
        }
        default: return;
    }
    Say(text);
}

void CrawlerGame::PickUpAt(Cell at)
{
    for (Item& item : m_Items)
    {
        if (item.taken || item.at != at)
        {
            continue;
        }
        if (GoesInBag(item.kind) && m_Bag[static_cast<int>(item.kind)] >= kMaxCarried)
        {
            char text[kMessageLength];
            std::snprintf(text, sizeof(text), "No room for another %s.", ItemName(item.kind));
            Say(text);
            continue;
        }
        item.taken = true;
        m_Gained = -1;
        Gain(item.kind, "");
        if (item.kind == ItemKind::Chest && m_Menu == Menu::None)
        {
            ShowResult(Reveal::Opened);
        }
    }
}

void CrawlerGame::SearchProp(Prop& prop, const char* name)
{
    char from[32];
    std::snprintf(from, sizeof(from), "%s: ", name);
    char text[kMessageLength];
    const int roll = m_Random.Range(0, 99);
    if (roll < 12 && m_Depth > 1)
    {
        // Something was living in it.
        for (int f = 0; f < 4; ++f)
        {
            const Cell c = Ahead(prop.at, f, 1);
            if (c != m_Hero && IsFree(c) && PlaceMonster(PickKind(), c))
            {
                const Monster& m = m_Monsters[MonsterAt(c)];
                std::snprintf(text, sizeof(text), "%sit moves. A %s crawls out!", from, StatsOf(m.kind).name);
                Say(text);
                return;
            }
        }
    }
    if (roll < 45)
    {
        Gain(PickLoot(true), from);
    }
    else if (roll < 56)
    {
        Gain(ItemKind::Gold, from);
    }
    else if (roll < 68)
    {
        Gain(static_cast<ItemKind>(static_cast<int>(ItemKind::Moss) + GetZone()), from);
    }
    else if (roll < 80)
    {
        Carry(ItemKind::Snack, 1, from);
    }
    else
    {
        std::snprintf(text, sizeof(text), "%sdust, and old cobwebs.", from);
        Say(text);
    }
}

void CrawlerGame::ReadBook()
{
    // An unread one if there is one: each teaches its lesson once a run.
    const int start = m_Random.Range(0, Lore::kBooks - 1);
    int book = start;
    for (int i = 0; i < Lore::kBooks; ++i)
    {
        const int b = (start + i) % Lore::kBooks;
        if ((m_BooksRead & (1u << b)) == 0)
        {
            book = b;
            break;
        }
    }
    const bool first = (m_BooksRead & (1u << book)) == 0;
    m_BooksRead = static_cast<uint16_t>(m_BooksRead | (1u << book));
    m_Reading = &Lore::GetBook(book);
    char text[kMessageLength];
    const char* learned = "";
    if (first)
    {
        switch (Lore::GetBookGift(book))
        {
            case Lore::BookGift::Attack:
                ++m_Attack;
                learned = " Atk +1!";
                break;
            case Lore::BookGift::Defense:
                ++m_Defense;
                learned = " Def +1!";
                break;
            case Lore::BookGift::MaxHp:
                m_MaxHp += 3;
                m_Hp += 3;
                learned = " Max HP +3!";
                break;
            case Lore::BookGift::MaxMp:
                m_MaxMp += 2;
                m_Mp += 2;
                learned = " Max MP +2!";
                break;
            case Lore::BookGift::Lockpick:
                if (m_Bag[static_cast<int>(ItemKind::Lockpick)] < kMaxCarried)
                {
                    ++m_Bag[static_cast<int>(ItemKind::Lockpick)];
                    learned = " A lockpick fell out!";
                }
                break;
            case Lore::BookGift::None: break;
        }
    }
    std::snprintf(text, sizeof(text), "You read \"%s\".%s", m_Reading->title, learned);
    Say(text);
    OpenMenu(Menu::Book);
}

void CrawlerGame::ReadNote()
{
    // What it hints at: one of this floor's secrets still waiting, if any.
    Lore::NoteHint hints[5];
    int count = 0;
    for (int y = 0; y < kMapHeight && count == 0; ++y)
    {
        for (int x = 0; x < kMapWidth; ++x)
        {
            if (m_Map.At(x, y) == Tile::LooseBrick)
            {
                hints[count++] = Lore::NoteHint::LooseBrick;
                break;
            }
        }
    }
    const Hideout& secret = m_Map.GetSecretRoom();
    if (secret.present && m_Map.At(secret.entrance) == Tile::SecretWall)
    {
        hints[count++] = Lore::NoteHint::Crack;
    }
    for (const Item& item : m_Items)
    {
        if (!item.taken && item.kind == ItemKind::Shard)
        {
            hints[count++] = Lore::NoteHint::Shard;
            break;
        }
    }
    if (m_EchoOnFloor >= 0)
    {
        hints[count++] = Lore::NoteHint::Echo;
    }
    const Hideout& vault = m_Map.GetVault();
    if (vault.present && m_Map.At(vault.entrance) == Tile::Door)
    {
        hints[count++] = Lore::NoteHint::Vault;
    }
    const Lore::NoteHint hint = count > 0 ? hints[m_Random.Range(0, count - 1)] : Lore::NoteHint::Nothing;
    m_Reading = &Lore::GetNote(hint, m_Random.Range(0, 1));
    // Kept, to read again from the menu (the oldest goes when there are
    // too many).
    bool kept = false;
    for (int i = 0; i < m_NoteCount; ++i)
    {
        kept = kept || m_Notes[i] == m_Reading;
    }
    if (!kept)
    {
        if (m_NoteCount == kMaxNotes)
        {
            for (int i = 1; i < kMaxNotes; ++i)
            {
                m_Notes[i - 1] = m_Notes[i];
            }
            --m_NoteCount;
        }
        m_Notes[m_NoteCount++] = m_Reading;
    }
    Say("A note. Someone left it for whoever came next.");
    OpenMenu(Menu::Book);
}

void CrawlerGame::Sleep()
{
    m_Hp = m_MaxHp;
    m_Mp = m_MaxMp;
    m_Tipsy = 0;
    char text[kMessageLength];
    if (m_Depth > 1 && m_Random.Chance(30))
    {
        // Something found you sleeping.
        for (int f = 0; f < 4; ++f)
        {
            const Cell c = Ahead(m_Hero, f, 1);
            if (IsFree(c) && PlaceMonster(PickKind(), c))
            {
                const Monster& m = m_Monsters[MonsterAt(c)];
                std::snprintf(text, sizeof(text), "You wake to drool on your face... a %s!", StatsOf(m.kind).name);
                Say(text);
                return;
            }
        }
    }
    Say("You sleep. Nothing finds you. HP and MP full.");
}

bool CrawlerGame::PickLock()
{
    int& picks = m_Bag[static_cast<int>(ItemKind::Lockpick)];
    char text[kMessageLength];
    if (picks <= 0)
    {
        Say("Locked tight. You need a lockpick.");
        return false;
    }
    // A dagger's thin blade helps.
    if (!m_Random.Chance(m_Weapon == Weapon::Dagger ? 85 : 60))
    {
        --picks;
        std::snprintf(text, sizeof(text), "Snap! The lockpick breaks. (%d left)", picks);
        Say(text);
        return false;
    }
    // Locked chests keep the good things.
    constexpr ItemKind kRich[] = {
        ItemKind::Elixir, ItemKind::Heart,  ItemKind::Gem, ItemKind::Goblet, ItemKind::Gem,
        ItemKind::Wing,   ItemKind::Scroll, ItemKind::Axe, ItemKind::Spear,  ItemKind::Hammer
    };
    constexpr int kRichCount = static_cast<int>(sizeof(kRich) / sizeof(kRich[0]));
    Gain(Unlocked(kRich[m_Random.Range(0, kRichCount - 1)]), "Click! Inside: ");
    return true;
}

void CrawlerGame::PullBrick(Cell at)
{
    m_Map.Set(at.x, at.y, Tile::Wall);
    ++m_Version;
    const Hideout& secret = m_Map.GetSecretRoom();
    const int roll = m_Random.Range(0, 99);
    if (secret.present && m_Map.At(secret.entrance) == Tile::SecretWall && roll < 35)
    {
        // A switch, not a stash.
        m_Map.Open(secret.entrance);
        m_Map.ComputeView(m_Hero, 6);
        Say("Click. Somewhere, stone grinds open...");
        return;
    }
    if (roll >= 90)
    {
        for (int f = 0; f < 4; ++f)
        {
            const Cell c = Ahead(m_Hero, f, 1);
            if (IsFree(c) && PlaceMonster(MonsterKind::Eyebat, c))
            {
                Say("A dusk gazer bursts out of the hole!");
                return;
            }
        }
    }
    if (roll >= 75)
    {
        Carry(ItemKind::Gem, 1, "Behind the brick: ");
        return;
    }
    Gain(PickLoot(true), "Behind the brick: ");
}

int CrawlerGame::ValuablesWorth() const
{
    return Carried(ItemKind::Gem) * (30 + 5 * m_Depth) + Carried(ItemKind::Goblet) * (20 + 3 * m_Depth) +
           Carried(ItemKind::Pearl) * (45 + 6 * m_Depth);
}

void CrawlerGame::SellValuables()
{
    const int worth = ValuablesWorth();
    if (worth <= 0)
    {
        return;
    }
    m_Gold += worth;
    m_Bag[static_cast<int>(ItemKind::Gem)] = 0;
    m_Bag[static_cast<int>(ItemKind::Goblet)] = 0;
    m_Bag[static_cast<int>(ItemKind::Pearl)] = 0;
    char text[kMessageLength];
    std::snprintf(text, sizeof(text), "Puddle: Pretty things. %d gold for the lot.", worth);
    Say(text);
    FillMenu();
    ++m_Version;
}

void CrawlerGame::ReadEcho(Cell at)
{
    // Heard once, the stone goes quiet: a wall again.
    m_Map.Set(at.x, at.y, Tile::Wall);
    if (m_EchoOnFloor < 0)
    {
        return;
    }
    m_EchoShown = m_EchoOnFloor;
    m_Legend.echoes = static_cast<uint16_t>(m_Legend.echoes | (1u << m_EchoOnFloor));
    m_EchoOnFloor = -1;
    ++m_LegendVersion;
    Say("The stone goes quiet.");
    OpenMenu(Menu::Echo);
}

void CrawlerGame::GiveSnack()
{
    int& snacks = m_Bag[static_cast<int>(ItemKind::Snack)];
    if (snacks <= 0 || m_HasStaff)
    {
        return;
    }
    --snacks;
    ++m_SnacksGiven;
    char text[kMessageLength];
    if (m_SnacksGiven >= 3)
    {
        m_HasStaff = true;
        m_Legend.staff = true;
        ++m_LegendVersion;
        Say("Puddle: Fed thrice, as he fed me. Take his staff.");
    }
    else
    {
        std::snprintf(text, sizeof(text), "Puddle: Good. %d more, and I give what I was given.", 3 - m_SnacksGiven);
        Say(text);
    }
    FillMenu();
    ++m_Version;
}

void CrawlerGame::ChooseEnding(int choice)
{
    const bool freed = choice == 0;
    m_Legend.endings = static_cast<uint8_t>(m_Legend.endings | (freed ? 1 : 2));
    m_Legend.lastEnding = static_cast<uint8_t>(freed ? 1 : 2);
    ++m_LegendVersion;
    m_RunWon = true;
    m_EchoShown = freed ? Lore::kEndingFreed : Lore::kEndingStaff;
    m_Menu = Menu::Echo;
    ++m_Version;
    Say(freed ? "Halvard is free. Two hundred shades lie down." : "You put on the hat. It fits. They turn to you.");
}
}  // namespace Dungeon
