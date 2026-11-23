// The game's core: names and texts, starting a run, looking things up, a key's
// worth of play and the turn after it, time, and the events the screen reads.
// The rest of CrawlerGame is in CrawlerGameFloor, Fight, Things, Village and
// View.

#include "CrawlerGame.h"

#include "Contracts.h"
#include "CrawlerGameData.h"
#include "Village.h"

#include <cstdio>
#include <cstring>

namespace Dungeon
{

using namespace GameData;

const char* CrawlerGame::ItemName(ItemKind kind)
{
    switch (kind)
    {
        case ItemKind::Potion: return "Potion";
        case ItemKind::Gold: return "Gold";
        case ItemKind::Sword: return "Whetstone";
        case ItemKind::Blade: return "Sword";
        case ItemKind::Axe: return "Axe";
        case ItemKind::Spear: return "Spear";
        case ItemKind::Dagger: return "Dagger";
        case ItemKind::Hammer: return "Hammer";
        case ItemKind::Shield: return "Shield";
        case ItemKind::Heart: return "Heart crystal";
        case ItemKind::Chest: return "Chest";
        case ItemKind::Key: return "Key";
        case ItemKind::Scroll: return "Map scroll";
        case ItemKind::Bomb: return "Bomb";
        case ItemKind::Wing: return "Wing";
        case ItemKind::Elixir: return "Elixir";
        case ItemKind::Page: return "Diary page";
        case ItemKind::Shard: return "Heart shard";
        case ItemKind::Duck: return "Carved duck";
        case ItemKind::Snack: return "Snack";
        case ItemKind::Lockpick: return "Lockpick";
        case ItemKind::Gem: return "Gem";
        case ItemKind::Goblet: return "Gold goblet";
        case ItemKind::Note: return "Note";
        case ItemKind::Moss: return "Moss";
        case ItemKind::Bone: return "Bone";
        case ItemKind::Shell: return "Shell";
        case ItemKind::Ore: return "Ore";
        case ItemKind::Water: return "Spring water";
        case ItemKind::Glowcap: return "Glowcap";
        case ItemKind::Fish: return "Fish";
        case ItemKind::Pearl: return "Pearl";
        case ItemKind::Vial: return "Murky vial";
    }
    return "?";
}

const char* CrawlerGame::ItemHelp(ItemKind kind)
{
    switch (kind)
    {
        case ItemKind::Potion: return "Heals 10 HP or more.";
        case ItemKind::Elixir: return "Full HP, +3 max HP.";
        case ItemKind::Bomb: return "Blasts walls & foes.";
        case ItemKind::Scroll: return "Maps the whole floor.";
        case ItemKind::Key: return "Opens a locked door.";
        case ItemKind::Wing: return "Flies down a floor.";
        case ItemKind::Sword: return "Attack +1.";
        case ItemKind::Blade: return "Balanced. Crits more.";
        case ItemKind::Axe: return "Hits hard. Can miss.";
        case ItemKind::Spear: return "Reaches two tiles.";
        case ItemKind::Dagger: return "Two quick stabs.";
        case ItemKind::Hammer: return "Stuns. Breaks cracks.";
        case ItemKind::Shield: return "Defence +1.";
        case ItemKind::Heart: return "Max HP +5.";
        case ItemKind::Snack: return "Puddle loves these.";
        case ItemKind::Lockpick: return "Opens locked chests. Can break.";
        case ItemKind::Gem:
        case ItemKind::Pearl:
        case ItemKind::Goblet: return "Puddle pays well for it.";
        case ItemKind::Water: return "Clean and cold. HP and MP.";
        case ItemKind::Glowcap: return "It glows. Eating it is a gamble.";
        case ItemKind::Fish: return "Raw. Better cooked, or thrown.";
        case ItemKind::Vial: return "Unmarked. It could be anything.";
        case ItemKind::Note: return "Someone left it here.";
        case ItemKind::Moss:
        case ItemKind::Bone:
        case ItemKind::Shell:
        case ItemKind::Ore: return "For building the village.";
        default: return "";
    }
}

const char* CrawlerGame::ZoneName(int zone)
{
    constexpr const char* kNames[kZones] = { "The Weeping Halls", "The Ossuary", "The Sunken Reach",
                                             "The Cinder Foundry" };
    return kNames[((zone % kZones) + kZones) % kZones];
}

const char* CrawlerGame::PageText(int page)
{
    return page >= 1 && page <= kLorePages ? kPages[page - 1] : "";
}

const char* CrawlerGame::SpellName(Spell spell)
{
    constexpr const char* kNames[kSpells] = { "Fire Bolt", "Heal", "Frost", "Blink", "Fold" };
    return kNames[static_cast<int>(spell)];
}

const char* CrawlerGame::SpellHelp(Spell spell)
{
    constexpr const char* kHelp[kSpells] = { "Burns the first foe ahead.", "Restores HP.", "Freezes foes near you.",
                                             "Jumps you forward.", "Folds a foe into the wall." };
    return kHelp[static_cast<int>(spell)];
}

int CrawlerGame::SpellCost(Spell spell)
{
    constexpr int kCost[kSpells] = { 3, 4, 5, 4, 6 };
    return kCost[static_cast<int>(spell)];
}

int CrawlerGame::SpellLevel(Spell spell)
{
    constexpr int kLevel[kSpells] = { 1, 3, 5, 7, 1 };
    return kLevel[static_cast<int>(spell)];
}

const char* CrawlerGame::MonsterName(MonsterKind kind)
{
    return StatsOf(kind).name;
}

bool CrawlerGame::IsBoss(MonsterKind kind)
{
    return kind == MonsterKind::Mimic || kind == MonsterKind::Grubking || kind == MonsterKind::Hydragon ||
           kind == MonsterKind::Keeper;
}

Cell CrawlerGame::Ahead(Cell from, int facing, int steps)
{
    constexpr int kDx[4] = { 1, 0, -1, 0 };
    constexpr int kDy[4] = { 0, 1, 0, -1 };
    const int f = facing & 3;
    return { from.x + kDx[f] * steps, from.y + kDy[f] * steps };
}

void CrawlerGame::Speak(const char* line)
{
    // One line, with nothing after it (SpeakLines says more).
    m_SpeechLeft = 0;
    m_AfterSpeech = 0;
    std::snprintf(m_Speech, sizeof(m_Speech), "%s", line);
    ++m_SpeechCount;
    m_Menu = Menu::Talk;
    ++m_Version;
}

void CrawlerGame::ShowResult(Reveal reveal)
{
    std::snprintf(m_ResultText, sizeof(m_ResultText), "%s", m_Message);
    m_ResultItem = m_Gained;
    m_Reveal = reveal;
    // Shown by the screen as it happens (a thing found is tossed up in the
    // view, as coins are): nothing to shut, play goes on.
    ++m_ResultCount;
    ++m_Version;
}

bool CrawlerGame::CanReturnHome() const
{
    if (m_InVillage || m_InSeam || IsDead() || (IsBossFloor() && BossAlive()))
    {
        return false;
    }
    for (const Monster& m : m_Monsters)
    {
        const int dx = m.at.x > m_Hero.x ? m.at.x - m_Hero.x : m_Hero.x - m.at.x;
        const int dy = m.at.y > m_Hero.y ? m.at.y - m_Hero.y : m_Hero.y - m.at.y;
        if (m.alive && dx <= 5 && dy <= 5)
        {
            return false;  // something hunts nearby
        }
    }
    return true;
}

void CrawlerGame::Sound(Cue cue)
{
    if (m_CueCount < kMaxCues)
    {
        m_Cues[m_CueCount++] = cue;
    }
}

int CrawlerGame::TakeCues(Cue* out, int max)
{
    const int count = m_CueCount < max ? m_CueCount : max;
    for (int i = 0; i < count; ++i)
    {
        out[i] = m_Cues[i];
    }
    m_CueCount = 0;
    return count;
}

void CrawlerGame::Say(const char* text)
{
    std::snprintf(m_Message, sizeof(m_Message), "%s", text);
    ++m_MessageCount;
    ++m_Version;
}

int CrawlerGame::WeaponUnlockDepth(Weapon weapon)
{
    switch (weapon)
    {
        case Weapon::Blade: return 0;
        case Weapon::Axe: return 3;
        case Weapon::Spear: return 5;
        case Weapon::Dagger: return 8;
        case Weapon::Hammer: return 11;
    }
    return 0;
}

const char* CrawlerGame::UnlockedBetween(int before, int now)
{
    // The deepest one counts: a run that opens two says the better.
    // Said as the hero would know it: what was seen that deep.
    constexpr const char* kNews[kWeapons] = { nullptr, "That deep, axes lie about. Look for them.",
                                              "That deep, spears lie about. Look for them.",
                                              "That deep, daggers lie about. Look for them.",
                                              "That deep, hammers lie about. Look for them." };
    const char* news = nullptr;
    for (int w = 1; w < kWeapons; ++w)
    {
        const int depth = WeaponUnlockDepth(static_cast<Weapon>(w));
        if (before < depth && now >= depth)
        {
            news = kNews[w];
        }
    }
    return news;
}

ItemKind CrawlerGame::Unlocked(ItemKind kind) const
{
    if (IsWeaponItem(kind) &&
        !IsWeaponUnlocked(static_cast<Weapon>(static_cast<int>(kind) - static_cast<int>(ItemKind::Blade))))
    {
        return ItemKind::Sword;  // a whetstone instead
    }
    return kind;
}

void CrawlerGame::NewRun(uint32_t seed, int depth)
{
    m_RecordBefore = m_Deepest;
    ResetRun(seed);
    EnterFloor(depth > 0 ? depth : 1);
    SayRunStart();
}

void CrawlerGame::GoHome(uint32_t seed)
{
    ResetRun(seed);
    EnterVillage();
    Say(m_Legend.deaths == 0 && m_Legend.bank == 0 ? "Welcome home. The old well leads down."
                                                   : "Home. Gold builds the village.");
}

void CrawlerGame::StartRun(int depth)
{
    m_RecordBefore = m_Deepest;
    ResetRun(m_Seed * 1103515245u + 12345u);
    EnterFloor(depth);
    if (depth > 1)
    {
        char text[kMessageLength];
        std::snprintf(text, sizeof(text), "The lift shudders down to B%d.", depth);
        Say(text);
        return;
    }
    SayRunStart();
}

void CrawlerGame::ResetRun(uint32_t seed)
{
    m_Seed = seed != 0 ? seed : 1;
    m_Random.Seed(m_Seed);
    m_Hp = 20;
    m_MaxHp = 20;
    m_Attack = 2;
    m_Defense = 0;
    m_Level = 1;
    m_Xp = 0;
    m_Gold = 0;
    for (int& count : m_Bag)
    {
        count = 0;
    }
    m_Bag[static_cast<int>(ItemKind::Potion)] = 1;
    m_Bag[static_cast<int>(ItemKind::Bomb)] = 1;
    m_Weapon = Weapon::Blade;
    m_MaxMp = 8;
    m_Mp = m_MaxMp;
    m_MpSteps = 0;
    m_Bolt = Bolt{};
    m_Menu = Menu::None;
    m_Shards = 0;
    m_ShardBits = 0;
    m_SnacksGiven = 0;
    m_HasStaff = false;
    m_WordsUsed = 0;
    m_InSeam = false;
    m_KeeperCalled = false;
    m_RunWon = false;
    m_DeathRecorded = false;
    m_Tipsy = 0;
    m_EndingPending = false;
    m_BooksRead = 0;
    m_NoteCount = 0;
    m_InVillage = false;
    m_Inside = -1;
    for (int& m : m_Materials)
    {
        m = 0;
    }
    // What the village has built.
    const auto level = [this](Village::Building b)
    { return static_cast<int>(m_Legend.buildings[static_cast<int>(b)]); };
    // The smith: a keener blade, then armour too.
    m_Attack += level(Village::Building::Blacksmith);
    m_Defense += level(Village::Building::Blacksmith) - 1 > 0 ? level(Village::Building::Blacksmith) - 1 : 0;
    m_Bag[static_cast<int>(ItemKind::Potion)] += level(Village::Building::Apothecary);
    m_MaxHp += 8 * level(Village::Building::Tavern);
    m_Hp = m_MaxHp;
    m_MaxMp += 3 * level(Village::Building::Tower);
    m_Mp = m_MaxMp;
    // What kept contracts gave: Hilde's charm, the teacher's lessons,
    // Ingrid's draught, Torvald's blade.
    m_MaxHp += Contracts::IsKept(m_Legend, Contracts::Id::Doll) ? 5 : 0;
    m_Hp = m_MaxHp;
    m_MaxMp += Contracts::IsKept(m_Legend, Contracts::Id::Question) ? 3 : 0;
    m_Mp = m_MaxMp;
    m_Bag[static_cast<int>(ItemKind::Potion)] += Contracts::IsKept(m_Legend, Contracts::Id::Herb) ? 1 : 0;
    m_Attack += Contracts::IsKept(m_Legend, Contracts::Id::Hammer) ? 1 : 0;
    m_ContractNews = -1;
}

void CrawlerGame::SayRunStart()
{
    char text[kMessageLength];
    if (m_Legend.lastEnding == 1)
    {
        Say("Halvard: Down again? Then say a word for him.");
    }
    else if (m_Legend.deaths > 0)
    {
        std::snprintf(text, sizeof(text), "%s: You again. I keep %d of you now.", Lore::VoiceName(m_Legend),
                      m_Legend.deaths);
        Say(text);
    }
    else
    {
        Say("The contract: end what keeps the dead below.");
    }
}

int CrawlerGame::MonsterAt(Cell c) const
{
    for (int i = 0; i < kMaxMonsters; ++i)
    {
        if (m_Monsters[i].alive && m_Monsters[i].at == c)
        {
            return i;
        }
    }
    return -1;
}

int CrawlerGame::PropAt(Cell c) const
{
    for (int i = 0; i < kMaxProps; ++i)
    {
        if (m_Props[i].present && m_Props[i].at == c)
        {
            return i;
        }
    }
    return -1;
}

int CrawlerGame::FeatureAt(Cell c) const
{
    for (int i = 0; i < kMaxFeatures; ++i)
    {
        if (m_Features[i].present && m_Features[i].at == c)
        {
            return i;
        }
    }
    return -1;
}

bool CrawlerGame::HasKnownTrap(Cell c) const
{
    if (!DungeonMap::InBounds(c.x, c.y))
    {
        return false;
    }
    const uint8_t trap = m_Traps[c.y][c.x];
    return (trap & ~kFound) != 0 && ((trap & kFound) != 0 || m_KnowsSecrets);
}

bool CrawlerGame::IsFree(Cell c) const
{
    return m_Map.IsWalkable(c) && c != m_Hero && MonsterAt(c) < 0 && FeatureAt(c) < 0 && PropAt(c) < 0;
}

bool CrawlerGame::IsEmptyFloor(Cell c) const
{
    if (!IsFree(c) || m_Map.At(c) != Tile::Floor || c == m_Map.GetStart())
    {
        return false;
    }
    for (const Item& item : m_Items)
    {
        if (!item.taken && item.at == c)
        {
            return false;
        }
    }
    return m_Traps[c.y][c.x] == 0;
}

void CrawlerGame::Act(Action action)
{
    ActOnce(action);
    if (IsDead() && !m_DeathRecorded)
    {
        RecordDeath();
    }
    if (m_EndingPending && !IsDead())
    {
        // The Keeper fell this turn, however it was felled: the choice.
        m_EndingPending = false;
        m_Menu = Menu::Ending;
        m_MenuCursor = 0;
        FillMenu();
        ++m_Version;
    }
}

void CrawlerGame::ActOnce(Action action)
{
    if (IsDead())
    {
        if (action == Action::Restart)
        {
            GoHome(m_Seed * 1103515245u + 12345u);
        }
        return;
    }

    if (m_Menu == Menu::Page || m_Menu == Menu::Echo || m_Menu == Menu::Book || m_Menu == Menu::Controls)
    {
        if (action != Action::None)
        {
            m_Menu = Menu::None;
            ++m_Version;
        }
        return;
    }
    if (m_Menu == Menu::Talk || m_Menu == Menu::Result)
    {
        // Shut by the key that takes or leaves, not by walking on. Someone
        // with more to say says the next line; after the last, what the
        // talk was for (a contract taken, or paid).
        if (action == Action::MenuSelect || action == Action::MenuClose || action == Action::Interact)
        {
            const bool talk = m_Menu == Menu::Talk;
            if (talk && NextSpeech())
            {
                return;
            }
            m_Menu = Menu::None;
            ++m_Version;
            if (talk)
            {
                AfterSpeech();
            }
        }
        return;
    }
    if (m_RunWon)
    {
        return;  // the story is told: the Crawler goes back to the title
    }
    if (m_Menu == Menu::Ending)
    {
        // A choice that must be made: no closing it.
        if (action == Action::MenuUp || action == Action::MenuDown)
        {
            m_MenuCursor = 1 - (m_MenuCursor & 1);
            ++m_Version;
        }
        else if (action == Action::MenuSelect)
        {
            ChooseEnding(m_MenuCursor);
        }
        return;
    }
    if (m_Menu != Menu::None)
    {
        switch (action)
        {
            case Action::MenuUp:
                m_MenuCursor = m_MenuCursor > 0 ? m_MenuCursor - 1 : m_MenuCount;
                ++m_Version;
                break;
            case Action::MenuDown:
                m_MenuCursor = m_MenuCursor < m_MenuCount ? m_MenuCursor + 1 : 0;
                ++m_Version;
                break;
            case Action::MenuSelect: ChooseInMenu(); break;
            case Action::MenuClose:
            case Action::OpenBag:
            case Action::OpenMagic:
            case Action::OpenMenu:
                m_Menu = Menu::None;
                ++m_Version;
                break;
            default: break;
        }
        return;
    }

    if (IsGliding() || action == Action::None || action == Action::Restart)
    {
        return;
    }
    if (action == Action::OpenBag)
    {
        OpenMenu(Menu::Bag);
        return;
    }
    if (action == Action::OpenMagic)
    {
        OpenMenu(Menu::Magic);
        return;
    }
    if (action == Action::OpenMenu)
    {
        OpenMenu(Menu::Main);
        return;
    }
    if (action == Action::MenuUp || action == Action::MenuDown || action == Action::MenuSelect ||
        action == Action::MenuClose)
    {
        return;
    }
    if (m_Tipsy > 0 && action != Action::Wait && m_Random.Chance(30))
    {
        // A beer's feet: now and then they go their own way.
        constexpr Action kStumbles[6] = { Action::Forward,     Action::Back,     Action::StrafeLeft,
                                          Action::StrafeRight, Action::TurnLeft, Action::TurnRight };
        const Action other = kStumbles[m_Random.Range(0, 5)];
        if (other != action)
        {
            action = other;
            Sound(Cue::Stumble);
            Say("Your feet go their own way.");
        }
    }
    bool swing = false;
    if (action == Action::Interact)
    {
        // Something lying ahead is picked up from here; a cracked wall is
        // struck; anything else is used as walking into it would. Nothing to
        // use: the weapon is swung at whatever is there, or at the air. On
        // a way down, the question again.
        const Target target = GetTarget();
        const Cell ahead = Ahead(m_Hero, m_Facing, 1);
        if (!target.verb)
        {
            if (AskDescend())
            {
                return;
            }
            swing = true;
        }
        else if (MonsterAt(ahead) < 0 && FeatureAt(ahead) < 0 && PropAt(ahead) < 0)
        {
            for (const Item& item : m_Items)
            {
                if (!item.taken && item.at == ahead)
                {
                    PickUpAt(ahead);
                    return;
                }
            }
            swing = m_Map.At(ahead) == Tile::SecretWall;
        }
        action = Action::Forward;
    }

    m_HeroFrom = m_Hero;
    m_FacingFrom = m_Facing;
    m_Through = false;
    for (Monster& m : m_Monsters)
    {
        m.from = m.at;
    }
    if (swing)
    {
        Swing();
        return;
    }

    if (action == Action::TurnLeft || action == Action::TurnRight)
    {
        // Turning takes no turn: monsters do not move for it.
        m_Facing = (m_Facing + (action == Action::TurnLeft ? 3 : 1)) & 3;
        m_GlideMs = kTurnMs;
        m_GlideLength = kTurnMs;
        return;
    }

    if (action != Action::Wait)
    {
        int way = m_Facing;
        if (action == Action::Back)
        {
            way = m_Facing + 2;
        }
        else if (action == Action::StrafeLeft)
        {
            way = m_Facing + 3;
        }
        else if (action == Action::StrafeRight)
        {
            way = m_Facing + 1;
        }
        const Cell to = Ahead(m_Hero, way, 1);
        if (const Portal* portal = m_Map.PortalEntering(to, way & 3))
        {
            // Through an impossible door: out somewhere else, turned with it.
            if (MonsterAt(portal->to) >= 0 || FeatureAt(portal->to) >= 0)
            {
                Say("Something blocks the other side...");
                return;
            }
            m_Through = true;
            m_ThroughPortal = *portal;
            m_Facing = (m_Facing + portal->toDir - portal->dir) & 3;
            const bool inward = m_Map.GetPocket().present && m_Map.GetPocket().room.Contains(portal->to);
            Say(inward ? "Your ears pop. This hall can't fit here..." : "You step back out of the fold.");
            StepTo(portal->to);
            TurnEnds();
            return;
        }
        if (m_Weapon == Weapon::Spear && action == Action::Forward && MonsterAt(to) < 0 && m_Map.IsWalkable(to) &&
            FeatureAt(to) < 0 && PropAt(to) < 0)
        {
            // The spear reaches past an empty tile.
            const int far = MonsterAt(Ahead(m_Hero, way, 2));
            if (far >= 0)
            {
                HeroAttacks(far);
                TurnEnds();
                return;
            }
        }
        const int monster = MonsterAt(to);
        const int feature = FeatureAt(to);
        const int prop = PropAt(to);
        if (action != Action::Forward)
        {
            // Backwards or sideways only walks: things are used, and foes
            // fought, by facing them.
            const bool front = m_InVillage && m_Inside < 0 && Village::BuildingAt(to) >= 0;
            if (monster >= 0 || feature >= 0 || prop >= 0 || front || !m_Map.IsWalkable(to))
            {
                Sound(Cue::Bump);
                return;
            }
            StepTo(to);
            TurnEnds();
            return;
        }
        if (monster < 0 && feature < 0 && prop >= 0)
        {
            UseProp(prop);  // takes no turn
            return;
        }
        const Tile tile = m_Map.At(to);
        if (monster >= 0)
        {
            HeroAttacks(monster);
        }
        else if (feature >= 0)
        {
            // Using a feature takes no turn.
            Feature& f = m_Features[feature];
            if (f.kind == FeatureKind::Merchant)
            {
                OpenMenu(Menu::Shop);
            }
            else if (f.kind == FeatureKind::PortalHome)
            {
                ReturnHome();
            }
            else if (f.kind == FeatureKind::Granny || f.kind == FeatureKind::Kid)
            {
                // Hilde has something to ask; Leif says what the town
                // says; otherwise their lines, in turn.
                const int v = f.kind == FeatureKind::Granny ? 0 : 1;
                const char* gossip = v == 1 ? Gossip() : nullptr;
                const int talks = m_Talks[v]++;
                if (gossip)
                {
                    Speak(gossip);
                }
                else if (v != 0 || !TalkOfContract(static_cast<int>(Contracts::Giver::Hilde), talks % 3 == 1))
                {
                    Speak(Village::VillagerLine(v, talks));
                }
            }
            else if (f.kind == FeatureKind::Keeper)
            {
                TalkToKeeper();
            }
            else if (f.kind == FeatureKind::Fence)
            {
                OpenMenu(Menu::Fence);
            }
            else if (f.kind == FeatureKind::Board)
            {
                OpenMenu(Menu::Contracts);
            }
            else if (f.kind == FeatureKind::Shrine)
            {
                UseShrine(feature);
            }
            else
            {
                UseFountain(feature);
            }
            return;
        }
        else if (tile == Tile::SecretWall)
        {
            // Walked into: it sounds hollow, said once. Blows open it.
            Sound(Cue::Bump);
            if (!m_HollowSaid)
            {
                m_HollowSaid = true;
                Say("This wall sounds hollow.");
            }
            return;
        }
        else if (tile == Tile::EchoWall)
        {
            ReadEcho(to);
            return;
        }
        else if (tile == Tile::LooseBrick)
        {
            PullBrick(to);
            return;
        }
        else if (m_Inside >= 0 && to == Village::InteriorDoor())
        {
            LeaveBuilding();
            return;
        }
        else if (m_InVillage && m_Inside < 0 && Village::BuildingAt(to) >= 0)
        {
            const int building = Village::BuildingAt(to);
            if (m_Legend.buildings[building] > 0 && Village::CanEnter(building))
            {
                EnterBuilding(building);
            }
            else
            {
                TryUpgrade(building);
            }
            return;
        }
        else if (tile == Tile::Door)
        {
            int& keys = m_Bag[static_cast<int>(ItemKind::Key)];
            if (keys <= 0)
            {
                Say("Locked. It needs a key.");
                return;
            }
            --keys;
            m_Map.Open(to);
            Say("The key turns. The vault door swings wide.");
        }
        else if (m_Map.IsWalkable(to))
        {
            StepTo(to);
        }
        else
        {
            Sound(Cue::Bump);
            return;  // a wall: no turn spent
        }
    }
    TurnEnds();
}

void CrawlerGame::TurnEnds()
{
    if (IsDead())
    {
        return;
    }
    // Just stepped onto it: a way down asks before it takes.
    const bool arrived = m_Hero != m_HeroFrom;
    if (m_FallPending)
    {
        m_FallPending = false;
        EnterFloor(m_Depth + 1);
        char text[kMessageLength];
        std::snprintf(text, sizeof(text), "The floor is gone. You land hard on B%d.", m_Depth);
        Say(text);
        return;
    }
    if (arrived && m_Map.At(m_Hero) == Tile::Stairs && !m_InSeam && !m_InVillage && BossAlive())
    {
        // The boss floor's stairs open only once the boss is gone.
        for (const Monster& m : m_Monsters)
        {
            if (m.alive && IsBoss(m.kind))
            {
                char text[kMessageLength];
                Sound(Cue::Roar);
                std::snprintf(text, sizeof(text), "The %s guards the way down!", StatsOf(m.kind).name);
                Say(text);
                break;
            }
        }
    }
    MonstersAct();
    m_Map.ComputeView(m_Hero, 6);
    if (m_Tipsy > 0)
    {
        --m_Tipsy;
    }
    m_GlideMs = kStepMs;
    m_GlideLength = kStepMs;
    // A point of MP every three turns.
    if (++m_MpSteps >= 3)
    {
        m_MpSteps = 0;
        if (m_Mp < m_MaxMp)
        {
            ++m_Mp;
            ++m_Version;
        }
    }
    // A contract just done: what was found, shown before anything else.
    if (ShowContractNews())
    {
        return;
    }
    // Asked once for each time the hero comes to stand there: on arriving,
    // or as soon as the way opens (the boss that guarded it falls).
    m_DescendAsked = m_DescendAsked && !arrived;
    if (!m_DescendAsked && !IsDead())
    {
        AskDescend();
    }
}

bool CrawlerGame::AskDescend()
{
    // Where the tile under the hero leads, if anywhere: the well, the lift,
    // the stairs once nothing guards them.
    int to = 0;
    if (m_Inside >= 0 || m_InSeam || m_Menu != Menu::None)
    {
        return false;
    }
    if (m_InVillage)
    {
        const int lift = m_Legend.buildings[static_cast<int>(Village::Building::Lift)];
        to = m_Hero == m_Map.GetStairs() ? 1
                                         : (lift > 0 && m_Hero == Village::LiftCell() ? Village::LiftDepth(lift) : 0);
    }
    else if (m_Map.At(m_Hero) == Tile::Stairs && !BossAlive())
    {
        to = m_Depth + 1;
    }
    if (to <= 0)
    {
        return false;
    }
    m_DescendTo = to;
    m_DescendAsked = true;
    OpenMenu(Menu::Descend);
    return true;
}

void CrawlerGame::Descend()
{
    m_Menu = Menu::None;
    ++m_Version;
    if (m_InVillage)
    {
        StartRun(m_DescendTo);
    }
    else
    {
        EnterFloor(m_Depth + 1);
    }
}

void CrawlerGame::Swing()
{
    const Cell ahead = Ahead(m_Hero, m_Facing, 1);
    const bool open = m_Map.IsWalkable(ahead) && FeatureAt(ahead) < 0 && PropAt(ahead) < 0;
    if (m_Weapon == Weapon::Spear && open)
    {
        // The spear reaches past an empty tile.
        const int far = MonsterAt(Ahead(m_Hero, m_Facing, 2));
        if (far >= 0)
        {
            HeroAttacks(far);
            TurnEnds();
            return;
        }
    }
    m_SwingMs = kSwingMs;
    if (m_Map.At(ahead) == Tile::SecretWall)
    {
        StrikeWall(ahead);
    }
    else
    {
        Sound(open ? Cue::Swing : Cue::Clang);
        if (!open && !m_Map.IsWalkable(ahead))
        {
            AddChip(ahead);
        }
    }
    TurnEnds();
}

void CrawlerGame::StrikeWall(Cell at)
{
    // Each blow shows: chips, then cracks, then the wall comes down. A
    // hammer goes through at once.
    m_WallBlows = m_KnockAt == at ? m_WallBlows + 1 : 1;
    m_KnockAt = at;
    m_ShakeMs = kShakeMs;
    AddChip(at);
    if (m_WallBlows >= kWallBlows || m_Weapon == Weapon::Hammer)
    {
        const bool hammer = m_WallBlows < kWallBlows;
        m_WallBlows = 0;
        m_Map.Open(at);
        m_Map.ComputeView(m_Hero, 6);
        AddChip(at);
        Sound(Cue::Crumble);
        Say(hammer ? "The hammer goes through. A room was hiding there."
                   : "The wall comes down. A room was hiding there.");
        return;
    }
    Sound(Cue::Crack);
    constexpr const char* kBlows[kWallBlows - 1] = { "Stone chips fly. It sounds hollow behind.",
                                                     "A crack runs up the wall.", "Mortar spills out. It is giving.",
                                                     "It hangs by a few stones. Once more." };
    Say(kBlows[m_WallBlows - 1]);
}

void CrawlerGame::StepTo(Cell to)
{
    m_Hero = to;
    PickUp();
    uint8_t& trap = m_Traps[to.y][to.x];
    if (trap == 0)
    {
        return;
    }
    char text[kMessageLength];
    if ((trap & ~kFound) == kTeleport)
    {
        // The floor folds: somewhere else on it, and the fold is spent.
        trap = 0;
        Sound(Cue::Teleport);
        m_Map.SetLook(to, FloorLook::Rune);
        Cell out;
        const auto good = [&](Cell c) { return IsEmptyFloor(c) && c != to; };
        if (m_Map.RandomFloor(m_Random, good, out))
        {
            m_Hero = out;
            m_HeroFrom = out;
            m_ShakeMs = kShakeMs;
            Say("The floor folds under you. You are somewhere else.");
        }
        return;
    }
    if ((trap & ~kFound) == kAlarm)
    {
        // A bell: two more come looking.
        trap = 0;
        Sound(Cue::Bell);
        m_Map.SetLook(to, FloorLook::Rune);
        int came = 0;
        for (int i = 0; i < 2; ++i)
        {
            Cell at;
            const auto away = [&](Cell c)
            {
                const int dx = c.x - to.x;
                const int dy = c.y - to.y;
                return IsFree(c) && dx * dx + dy * dy >= 9;
            };
            came += m_Map.RandomFloor(m_Random, away, at) && PlaceMonster(PickKind(), at) ? 1 : 0;
        }
        Say(came > 0 ? "A wire. A bell rings in the dark. Something answers." : "A wire. A bell rings. Nothing comes.");
        return;
    }
    if ((trap & ~kFound) == kTrapdoor)
    {
        trap |= kFound;
        m_Hp = m_Hp > 2 ? m_Hp - 2 : 1;
        m_FallPending = true;
        return;
    }
    trap |= kFound;
    Sound(Cue::Spikes);
    m_Map.SetLook(to, FloorLook::Spikes);
    const int damage = m_Random.Range(2, 3 + m_Depth / 2);
    m_Hp -= damage;
    m_ShakeMs = kShakeMs;
    if (m_Hp <= 0)
    {
        m_Hp = 0;
        Say("Spikes! You fall...");
        return;
    }
    std::snprintf(text, sizeof(text), "Spikes come up through the floor. -%d HP.", damage);
    Say(text);
}

int CrawlerGame::TakeEvents(Event* out, int max)
{
    const int count = m_EventCount < max ? m_EventCount : max;
    for (int i = 0; i < count; ++i)
    {
        out[i] = m_Events[i];
    }
    m_EventCount = 0;
    return count;
}

void CrawlerGame::AddEvent(EventKind kind, int amount, const Monster& monster)
{
    if (m_EventCount >= kMaxEvents)
    {
        // Full (nobody reading, as in the tests): the oldest goes.
        for (int i = 1; i < kMaxEvents; ++i)
        {
            m_Events[i - 1] = m_Events[i];
        }
        --m_EventCount;
    }
    Event& e = m_Events[m_EventCount++];
    e.kind = kind;
    e.amount = amount;
    e.at = monster.at;
    e.monster = monster.kind;
}

void CrawlerGame::AddBreak(Cell at, int prop)
{
    Monster nobody;
    nobody.at = at;
    AddEvent(EventKind::Break, 0, nobody);
    m_Events[m_EventCount - 1].prop = prop;
}

void CrawlerGame::AddChip(Cell wall)
{
    Monster nobody;
    nobody.at = wall;
    AddEvent(EventKind::Chip, 0, nobody);
}

void CrawlerGame::GainXp(int xp)
{
    m_Xp += xp;
    while (m_Xp >= GetXpNeeded())
    {
        m_Xp -= GetXpNeeded();
        ++m_Level;
        m_MaxHp += 4;
        m_Hp = m_MaxHp;
        m_MaxMp += 2;
        m_Mp = m_MaxMp;
        if (m_Level % 2 == 1)
        {
            ++m_Attack;
        }
        char text[kMessageLength];
        std::snprintf(text, sizeof(text), "Level %d! Max HP %d, max MP %d.", m_Level, m_MaxHp, m_MaxMp);
        Say(text);
    }
}

void CrawlerGame::Tick(int ms)
{
    m_TimeMs += static_cast<uint32_t>(ms);
    const auto down = [ms](int& v) { v = v > ms ? v - ms : 0; };
    down(m_GlideMs);
    down(m_ShakeMs);
    down(m_SwingMs);
    down(m_BlastMs);
    down(m_FlashMs);
    down(m_Bolt.ms);
    for (Monster& m : m_Monsters)
    {
        down(m.hurtMs);
        down(m.lungeMs);
    }
}

int CrawlerGame::GlideProgress() const
{
    if (m_GlideMs <= 0)
    {
        return 1024;
    }
    // Eased in and out (smoothstep), so a step starts and stops softly.
    const int t = 1024 - m_GlideMs * 1024 / m_GlideLength;
    return static_cast<int>((static_cast<int64_t>(t) * t * (3 * 1024 - 2 * t)) >> 20);
}
}  // namespace Dungeon
