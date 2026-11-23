#pragma once

// The rules of the first-person crawler, with no engine in them: the hero's
// moves and turns (each glides over a few frames), monsters that walk toward
// the hero and fight, a bag of things to use, a bog trader who sells more, wishing
// fountains, traps, rooms behind cracked walls and vaults behind locked
// doors, experience and levels, a boss every fifth floor, stairs to the next.
// And the story (Lore.h): echo stones, stone listeners that keep names, the four heart
// shards, the Seam and its Keeper, and the side quests: Puddle's snacks,
// Sigrun, the rubber ducks, the ghost of the last run.
// The Crawler component feeds it keys and time and draws what it says.

#include "DungeonMap.h"
#include "Lore.h"
#include "../render/Raycaster.h"

#include <cstdint>

namespace Dungeon
{

enum class Action : uint8_t
{
    None,
    Forward,
    Back,
    TurnLeft,
    TurnRight,
    StrafeLeft,
    StrafeRight,
    Wait,
    Restart,
    OpenBag,
    MenuUp,
    MenuDown,
    MenuSelect,
    MenuClose,
    OpenMagic,
    Interact,  // use what is ahead: take it, open it, drink it, talk to it
    OpenMenu,  // the menu: the bag, the magic, the way home, the keys
};

// In texture order. The cute ones live near the top, the fierce ones deep
// down; the last three are bosses.
enum class MonsterKind : uint8_t
{
    Droolblob,
    Candleghoul,
    Eyejar,
    Eyebat,
    Flamespider,
    Bonehound,
    Teasnail,
    Stewpot,
    Grimoire,
    Ragmummy,
    Slimewizard,
    Rockstack,
    Glimmerjelly,  // rare, runs away, worth a lot
    Mimic,         // boss of floors 5, 20, 35...
    Grubking,      // boss of floors 10, 25...
    Hydragon,      // boss of floors 15, 30...
    Keeper,        // the mimic in Halvard's hat, in the Seam
    Ghost,         // what the last run left where it died
};
constexpr int kMonsterKinds = 18;

enum class ItemKind : uint8_t
{
    Potion,
    Gold,
    Sword,
    Shield,
    Heart,
    Chest,
    Key,
    Scroll,
    Bomb,
    Wing,
    Elixir,
    Page,  // a page of Halvard's journal: the story, one page at a time
    // Weapons: wield one (from the bag); the one in hand goes back in it.
    Blade,
    Axe,
    Spear,
    Dagger,
    Hammer,
    Shard,     // a piece of Halvard's heart: four, one per zone
    Duck,      // a rubber duck: three, kept over runs
    Snack,     // Puddle's favourite; three earn his Pocket Staff
    Lockpick,  // opens a locked chest, or breaks
    Gem,       // worth a lot to Puddle
    Goblet,    // worth something to Puddle
    Note,      // left by someone before you: read at once
    // Materials for the village, one per zone (Village::kMaterials).
    Moss,
    Bone,
    Shell,
    Ore,
    Water,    // spring water from a fountain: drunk later
    Glowcap,  // a cave mushroom: a gamble to eat, a stew when cooked
    Fish,     // raw; better cooked, or thrown to something hungry
    Pearl,    // out of a giant clam: worth a lot to Puddle
    Vial,     // unmarked: drinking it does anything
};
constexpr int kItemKinds = 33;

enum class Weapon : uint8_t
{
    Blade,   // balanced, more critical hits
    Axe,     // hits hard, sometimes misses
    Spear,   // reaches two tiles
    Dagger,  // two quick stabs
    Hammer,  // stuns now and then, smashes cracked walls
};
constexpr int kWeapons = 5;

enum class Spell : uint8_t
{
    FireBolt,  // a fireball down the corridor at the first monster in line
    Heal,
    Frost,  // freezes everything near for a few turns
    Blink,  // a jump forward of up to four tiles
    Fold,   // the Pocket Staff's: folds the first foe ahead into the wall
};
constexpr int kSpells = 5;

// Halvard's journal, found a page at a time across runs.
constexpr int kLorePages = 12;

// Every five floors the dungeon changes: its walls, floor and ceiling, its
// props, and its boss on the fifth. Then round again.
constexpr int kZones = 4;
constexpr int kZoneFloors = 5;
// Things standing about in each zone's rooms: a barrel, a berry bush...
constexpr int kZoneProps = 4;
// ...and ones found in any zone: a bookshelf, a camp, a locked chest.
// A Prop's kind is one of its zone's below kZoneProps, a common one from it.
constexpr int kCommonProps = 8;  // bookshelf, camp, locked chest, pot, crates, cage, old chest, mirror
constexpr int kPropKinds = kZones * kZoneProps + kCommonProps;

// Things that stand on a tile and are not monsters: walking into one uses it.
enum class FeatureKind : uint8_t
{
    Fountain,
    Merchant,
    Shrine,      // pray once: a blessing, or not
    PortalHome,  // opens where a boss falls: home with everything
    Granny,      // the village's villagers: talk to them
    Kid,
    Keeper,  // a building's keeper, inside it (CrawlerTextures::keepers)
    Fence,   // the man in the village's alley: sells what the dead dropped
    Board,   // the notice board in the square: the contracts
};
constexpr int kFeatureKinds = 9;

enum class Menu : uint8_t
{
    None,
    Bag,
    Shop,
    Page,  // a diary page being read
    Magic,
    Echo,       // an echo stone (or an ending) being heard
    Ending,     // the Keeper beaten: free Halvard, or take the staff
    Book,       // a book or a note being read
    Controls,   // the keys, shown once on the very first start
    Thing,      // a thing standing about: what to do with it
    Talk,       // someone speaking: a line, shut by a key
    Result,     // what a deed did, shown until a key (after a wait, for what was shut)
    Main,       // the menu: bag, magic, the way home, the keys
    Keys,       // the keys, to change them (the Crawler's part)
    Descend,    // standing on a way down: go, or not yet
    Fence,      // the man in the alley: materials, for the village's gold
    Contracts,  // what the town has asked of the hero: taken, done, paid
    Notes,      // what was read: notes, books, the diary's pages, to read again
};

/// How what a deed did comes up (Menu::Result).
enum class Reveal : uint8_t
{
    Now,     // at once
    Opened,  // something shut was opened: a shape first, then what it is
    Blow,    // something was struck: the blow and its pieces first, then the words
};

/// What can be done with a thing standing about: a line of its menu.
enum class Deed : uint8_t
{
    None,
    Search,  // a pack, a pile of bones
    Drink,   // a barrel's ale
    Smash,   // a barrel, a pot, crates
    EatBerries,
    PickBerries,
    Pray,
    TakeOfferings,
    OpenCoffin,
    Knock,
    TouchCrystal,
    BreakCrystal,
    EatMushroom,
    PickMushrooms,
    ReachForPearl,
    Sharpen,
    MendArmour,
    Warm,
    Cook,
    TakeFirst,  // a weapon rack's two weapons
    TakeSecond,
    Read,
    SearchShelf,
    Sleep,
    SearchCamp,
    PickLock,
    ForceLock,
    ReachIn,
    PryGem,  // a statue's eye
    Bow,
    ReadStone,  // a gravestone
    Dig,
    TakeFish,  // a fish basket
    DigBasket,
    SearchCart,  // a mine cart
    PushCart,
    OpenCage,
    FeedCage,
    OpenChest,  // an old chest, which may not be one
    Prod,
    LookMirror,
    BreakMirror,
    Eat,       // food from the bag
    Throw,     // to something hungry, to keep it busy
    FillSkin,  // a fountain
    DrinkDeep,
    Wash,
    PrayStone,     // a stone listener: a blessing, or not
    GiveName,      // and a name it has not heard: it gives something back
    SayHeartName,  // the name cut into Halvard's heart, once it is whole
};
constexpr int kDeeds = static_cast<int>(Deed::SayHeartName) + 1;
// How many choices a thing offers at most (and then to leave it).
constexpr int kThingDeeds = 3;

/// Something that makes a sound and shows no other way: a drink, a lock, a
/// bell. The game says so; whatever plays sounds takes them.
enum class Cue : uint8_t
{
    Drink,
    Eat,
    Pray,
    Dig,
    Lock,
    Sharpen,
    Buy,
    Error,
    Heal,
    Teleport,
    Bell,
    Roar,
    Home,
    Bump,
    Spikes,
    Swing,    // the weapon through the air
    Clang,    // on something that does not give
    Crack,    // on a wall that does
    Crumble,  // and the wall coming down
    Stumble,  // a beer's feet
};

struct Monster
{
    MonsterKind kind = MonsterKind::Droolblob;
    Cell at;
    Cell from;  // where it glides from this turn
    int hp = 1;
    int hurtMs = 0;   // > 0 while it shows being hit
    int lungeMs = 0;  // > 0 while it lunges at the hero
    int phase = 0;    // where its idle bob starts, so they do not bob together
    int frozen = 0;   // turns it cannot act (frost, a hammer's stun)
    int maxHp = 1;
    bool alive = false;
    bool named = false;  // someone a contract is about: Orm
};

struct Item
{
    ItemKind kind = ItemKind::Potion;
    Cell at;
    bool taken = true;
};

/// A torch on a wall: it stands out from the wall a little, and lights it.
struct Torch
{
    Fix x = 0;  // where the flame is, tile units
    Fix y = 0;
    uint8_t facing = 0;  // the way it faces, out from its wall
    bool present = false;
};

/// Something standing in a room that blocks the way: zone dressing.
struct Prop
{
    Cell at;
    uint8_t kind = 0;    // which of its zone's kZoneProps, or with `fixed` a kPropInfo index
    bool fixed = false;  // the same prop in any zone: a building's furnishing
    bool any = false;    // `kind` is a place in the props' table: a themed room's thing, from any zone
    bool present = false;
    uint8_t done = 0;    // which of its choices are used up, a bit each
    uint8_t state = 0;   // a barrel's gulps left, who is in a coffin or a cage, a rack's two weapons
    uint8_t facing = 1;  // the way its front faces, away from the wall behind it
};

struct Feature
{
    FeatureKind kind = FeatureKind::Fountain;
    Cell at;
    bool present = false;
    bool used = false;  // a fountain drunk dry
};

/// Something that just happened in the world, for the screen to show: a
/// number over a monster, sparks, a kill's splash.
enum class EventKind : uint8_t
{
    Hit,
    Critical,
    Miss,
    Kill,
    Break,  // a pot or a crate smashed: its pieces fly
    Chip,   // a blow on a wall: stone flies from the wall ahead
};
struct Event
{
    EventKind kind = EventKind::Hit;
    int amount = 0;  // the damage, for a hit
    Cell at;
    MonsterKind monster = MonsterKind::Droolblob;
    int prop = -1;  // for a Break: the prop's place in the tables (CrawlerTextures::props)
};

/// What the hero faces, for the prompt over the view: its name, what using
/// it does (Action::Interact), a line about it, and for a monster its health.
struct Target
{
    const char* name = nullptr;  // nullptr: nothing there worth a prompt
    const char* verb = nullptr;  // nullptr: nothing to do with it (any more)
    const char* detail = nullptr;
    int hp = -1;  // a monster's, -1 for anything else
    int maxHp = 0;
    // What it costs, for a building to raise: the gold and the material
    // (a Village material) it needs and the village has. -1: no cost.
    int needGold = -1;
    int haveGold = 0;
    int material = 0;
    int needMaterial = 0;
    int haveMaterial = 0;
    const char* gain = nullptr;  // what paying it gives
};

/// One line of the open menu: a thing in the bag (count) or for sale
/// (price, stock).
struct MenuEntry
{
    ItemKind item = ItemKind::Potion;
    int count = 0;
    int price = 0;
    int spell = -1;               // in the Magic menu: which Spell, its MP cost in price
    const char* label = nullptr;  // a line that is not a thing: "Feed Puddle"
    // In a thing's menu: what the line does, which of its choices it is
    // (price) and whether it can be done now (count).
    Deed deed = Deed::None;
};

/// The textures CrawlerGame hands out with its billboards.
struct CrawlerTextures
{
    const Texture* monsters[kMonsterKinds] = {};
    const Texture* items[kItemKinds] = {};
    const Texture* features[kFeatureKinds] = {};
    const Texture* props[kPropKinds] = {};  // the zones' (zone by zone), then the common ones
    const Texture* torch[2] = {};           // two flame frames
    const Texture* bolt = nullptr;          // a fire bolt in flight
    const Texture* spells[kSpells] = {};    // their icons, for the magic menu
    // Things drawn from eight sides, in the order Raycaster::ViewIndex gives
    // (front, front right, right, back right, back, back left, left, front
    // left); empty for things that look the same from everywhere.
    const Texture* propViews[kPropKinds][8] = {};
    const Texture* torchViews[8] = {};
    const Texture* keepers[Village::kBuildings] = {};  // inside each building, by Village::Building
    // A clay pot's other shapes: which one stands where goes by its tile, so
    // pots huddled together are not all alike.
    const Texture* pots[2] = {};
    // What a thing looks like once nothing is left to do with it (an open
    // chest, a bush picked bare), where it has such a picture; and a
    // fountain drunk dry.
    const Texture* spent[kPropKinds] = {};
    const Texture* fountainDry = nullptr;
    // The wall of the place, for the colour of what a blow knocks off it.
    const Texture* rubble = nullptr;
};

class CrawlerGame
{
public:
    static constexpr int kMaxMonsters = 12;
    static constexpr int kMaxItems = 12;
    static constexpr int kMaxFeatures = 5;
    static constexpr int kMaxProps = 20;
    static constexpr int kMaxTorches = 8;
    static constexpr int kMaxMenu = 30;
    static constexpr int kMaxCarried = 9;  // of each kind in the bag
    static constexpr int kMessageLength = 64;
    static constexpr int kSwingMs = 220;

    /// Starts a new run, on depth 1 unless told otherwise.
    void NewRun(uint32_t seed, int depth = 1);
    /// A new visit to the village, where runs start: the well goes down to
    /// depth 1, the lift (once built) deeper.
    void GoHome(uint32_t seed);
    bool IsInVillage() const { return m_InVillage; }
    /// New weapons turn up in the dungeon as runs get deeper: the deepest
    /// floor ever reached each needs (the sword: none).
    static int WeaponUnlockDepth(Weapon weapon);
    bool IsWeaponUnlocked(Weapon weapon) const { return m_Deepest >= WeaponUnlockDepth(weapon); }
    /// The building the hero is inside (a Village::Building), or -1.
    int GetInside() const { return m_Inside; }
    /// Opens the card with the keys on it, and marks it seen for good.
    void ShowControls();
    /// This run's materials, not yet home.
    int GetMaterial(int material) const { return m_Materials[material]; }

    /// One key's worth of play. Ignored while a move is still gliding, and
    /// while dead (except Restart). With a menu open only the Menu actions
    /// count.
    void Act(Action action);

    /// Moves time on: the glides, the swings, the bobs, the hit flashes.
    void Tick(int ms);

    bool IsGliding() const { return m_GlideMs > 0; }
    /// How far through the current glide, 0 to 1024 (1024 when still).
    int GlideProgress() const;
    /// True while the glide is a step (the view bobs), not a turn.
    bool IsStepping() const { return m_GlideMs > 0 && m_Hero != m_HeroFrom; }
    bool IsDead() const { return m_Hp <= 0; }
    /// What the last death left: where, the gold the bank kept and the echo
    /// holds, the materials that went home.
    struct DeathReport
    {
        int depth = 0;  // 0: the Seam
        int keptGold = 0;
        int echoGold = 0;
        int materials = 0;
        bool record = false;             // deeper than ever before
        const char* unlocked = nullptr;  // what that record opened up ("Axes ..."), or null
        // What to work toward at home: the building nearest being built or
        // bettered, and what it still needs. Empty: nothing left to build.
        char goal[2][48] = {};
        bool goalReady = false;  // there is enough for it already
    };
    const DeathReport& GetDeathReport() const { return m_DeathReport; }
    /// How far through the sword's swing, 0 to 1024, or -1 when not swinging.
    static constexpr int kMaxEvents = 8;
    /// Copies out what happened since the last call (up to `max`, oldest
    /// first) and forgets it.
    int TakeEvents(Event* out, int max);
    /// What TakeEvents would hand out, left where it is.
    int GetEventCount() const { return m_EventCount; }
    const Event& GetEvent(int i) const { return m_Events[i]; }
    int SwingProgress() const { return m_SwingMs > 0 ? 1024 - m_SwingMs * 1024 / kSwingMs : -1; }
    /// > 0 just after a bomb, for the flash.
    int BlastMs() const { return m_BlastMs; }
    /// > 0 just after a spell; `kind` says which (a Spell), for its colour.
    int FlashMs(int& kind) const
    {
        kind = m_FlashKind;
        return m_FlashMs;
    }

    Weapon GetWeapon() const { return m_Weapon; }
    /// What is just ahead of the hero, for the prompt.
    Target GetTarget() const;
    /// Turns left of a beer's courage: hits harder, misses more, the view
    /// sways.
    int GetTipsy() const { return m_Tipsy; }
    /// True while there is something left to do with a prop.
    bool HasDeeds(const Prop& prop) const;
    /// True once a prop should show its used-up picture.
    bool LooksSpent(const Prop& prop) const;
    /// What someone is saying (Menu::Talk), and a count that goes up with
    /// each line.
    const char* GetSpeech() const { return m_Speech; }
    uint32_t GetSpeechCount() const { return m_SpeechCount; }
    /// What a deed just did (Menu::Result): the words, the thing it gave
    /// (an ItemKind, or -1), whether it was shut and is opened slowly, and a
    /// count that goes up with each.
    const char* GetResultText() const { return m_ResultText; }
    int GetResultItem() const { return m_ResultItem; }
    Reveal GetReveal() const { return m_Reveal; }
    bool IsResultSlow() const { return m_Reveal == Reveal::Opened; }
    uint32_t GetResultCount() const { return m_ResultCount; }
    /// How loud the sounds are, 0 to 100: a line of the menu steps it.
    int GetSoundVolume() const { return m_SoundVolume; }
    void SetSoundVolume(int percent) { m_SoundVolume = percent < 0 ? 0 : (percent > 100 ? 100 : percent); }
    /// What just made a sound that the game's changes do not show: taken
    /// by whatever plays sounds.
    int TakeCues(Cue* out, int max);
    /// Blows the cracked wall ahead has taken: kWallBlows bring it down.
    static constexpr int kWallBlows = 5;
    int GetWallBlows() const { return m_WallBlows; }
    /// What a line of the contracts' list says to do (Menu::Contracts).
    const char* GetContractHint(const MenuEntry& entry) const;
    /// Where the way down the hero stands on leads (Menu::Descend).
    int GetDescendDepth() const { return m_DescendTo; }
    /// True when the menu's way home is open: out in the dungeon, nothing
    /// hunting nearby, no boss in the way.
    bool CanReturnHome() const;
    /// The thing whose choices are open (Menu::Thing): its name, a line
    /// about it, and what the chosen line would do.
    const char* GetThingName() const;
    const char* GetThingAbout() const;
    const char* DeedHint(const MenuEntry& entry) const;
    /// A prop's place in the tables of kPropKinds (CrawlerTextures::props).
    static int PropKindIndex(int zone, int kind)
    {
        return kind < kZoneProps ? zone * kZoneProps + kind : kZones * kZoneProps + kind - kZoneProps;
    }
    /// The same for a prop standing here.
    int PropIndex(const Prop& p) const { return p.fixed || p.any ? p.kind : PropKindIndex(GetZone(), p.kind); }

    /// The book or note open in the Book menu.
    const Lore::Book* GetReading() const { return m_Reading; }
    int GetMp() const { return m_Mp; }
    int GetMaxMp() const { return m_MaxMp; }
    bool KnowsSpell(Spell spell) const { return spell == Spell::Fold ? m_HasStaff : m_Level >= SpellLevel(spell); }
    static const char* SpellName(Spell spell);
    static const char* SpellHelp(Spell spell);
    static int SpellCost(Spell spell);
    static int SpellLevel(Spell spell);  // the level it is learned at

    /// The shadows on the floor under what stands there (monsters, mid-glide
    /// included, things lying about, props). Returns how many.
    int GetShadows(Raycaster::Shadow* out, int max) const;
    /// The viewer's place and heading for the raycaster, mid-glide included.
    void GetView(Fix& x, Fix& y, int& angle) const;
    /// Fills `out` with what stands on the floor, as seen now. Returns how
    /// many.
    int GetBillboards(const CrawlerTextures& textures, Billboard* out, int max) const
    {
        Fix x = 0;
        Fix y = 0;
        int angle = 0;
        GetView(x, y, angle);
        return GetBillboards(textures, out, max, x, y);
    }
    /// The same, for a viewer at (viewX, viewY): things drawn from eight
    /// sides show the side facing it.
    int GetBillboards(const CrawlerTextures& textures, Billboard* out, int max, Fix viewX, Fix viewY) const;
    /// Which of a thing's eight views (CrawlerTextures::propViews) shows from
    /// (viewX, viewY), for a thing at (x, y) facing `facing`.
    static int ViewIndex(Fix x, Fix y, int facing, Fix viewX, Fix viewY);

    const DungeonMap& GetMap() const { return m_Map; }
    Cell GetHeroCell() const { return m_Hero; }
    int GetFacing() const { return m_Facing; }  // 0 east, 1 south, 2 west, 3 north
    int GetHp() const { return m_Hp; }
    int GetMaxHp() const { return m_MaxHp; }
    int GetDepth() const { return m_Depth; }
    int GetGold() const { return m_Gold; }
    int GetAttack() const { return m_Attack; }
    int GetDefense() const { return m_Defense; }
    int GetLevel() const { return m_Level; }
    int GetXp() const { return m_Xp; }
    /// Experience the next level needs.
    int GetXpNeeded() const { return m_Level * 8; }
    int GetDeepest() const { return m_Deepest; }
    void SetDeepest(int depth) { m_Deepest = depth; }
    bool IsBossFloor() const { return m_Depth % 5 == 0; }
    /// Which zone this floor is in (0 mossy halls, 1 bone catacombs, 2
    /// flooded caves, 3 ember forge).
    /// The Seam looks like the forge.
    int GetZone() const { return m_InSeam ? 3 : ((m_Depth - 1) / kZoneFloors) % kZones; }
    static const char* ZoneName(int zone);
    /// True once a map scroll showed this floor: the minimap shows the
    /// cracked walls and the traps.
    bool KnowsSecrets() const { return m_KnowsSecrets; }
    const char* GetMessage() const { return m_Message; }
    /// Counts up every time a message is said, even the same one again.
    uint32_t GetMessageCount() const { return m_MessageCount; }
    /// Counts up every time something the HUD or a menu shows changes.
    uint32_t GetVersion() const { return m_Version; }

    int GetMonsterCount() const { return kMaxMonsters; }
    const Monster& GetMonster(int i) const { return m_Monsters[i]; }
    const Item& GetItem(int i) const { return m_Items[i]; }
    const Feature& GetFeature(int i) const { return m_Features[i]; }
    /// The monster standing on `c`, or -1.
    int MonsterAt(Cell c) const;
    /// The feature standing on `c`, or -1.
    int FeatureAt(Cell c) const;
    /// The prop standing on `c`, or -1.
    int PropAt(Cell c) const;
    const Prop& GetProp(int i) const { return m_Props[i]; }
    const Torch& GetTorch(int i) const { return m_Torches[i]; }
    /// The trap on `c` the hero knows of (found, or shown by a scroll).
    bool HasKnownTrap(Cell c) const;

    /// How many of `kind` the bag holds.
    int Carried(ItemKind kind) const { return m_Bag[static_cast<int>(kind)]; }
    /// Puts `count` of a thing in the bag, for tests and previews.
    void SetCarried(ItemKind kind, int count) { m_Bag[static_cast<int>(kind)] = count; }

    /// The open menu, its lines and the line chosen. The line after the last
    /// entry is "close".
    Menu GetMenu() const { return m_Menu; }
    int GetMenuCount() const { return m_MenuCount; }
    const MenuEntry& GetMenuEntry(int i) const { return m_MenuEntries[i]; }
    int GetMenuCursor() const { return m_MenuCursor; }

    /// Diary pages read so far, over every run (the Crawler keeps it on the
    /// SD card). The next page found is the one after them.
    int GetPagesKnown() const { return m_PagesKnown; }
    void SetPagesKnown(int pages) { m_PagesKnown = pages < kLorePages ? pages : kLorePages; }
    /// The page open in the Page menu (1-based), and a page's text.
    int GetPageShown() const { return m_PageShown; }
    static const char* PageText(int page);

    /// The story so far, over every run (the Crawler keeps it on the SD
    /// card), and a count that goes up whenever it changes.
    const Legend& GetLegend() const { return m_Legend; }
    void SetLegend(const Legend& legend) { m_Legend = legend; }
    uint32_t GetLegendVersion() const { return m_LegendVersion; }
    /// Heart shards carried this run.
    int GetShards() const { return m_Shards; }
    /// Puts pieces of the heart in hand, for tests and previews.
    void SetShards(int shards) { m_Shards = shards; }
    bool HasStaff() const { return m_HasStaff; }
    int GetSnacksGiven() const { return m_SnacksGiven; }
    /// True in the Seam, the fold under every floor where the Keeper waits.
    bool IsInSeam() const { return m_InSeam; }
    /// True once an ending was chosen: the run is over, its story told.
    bool IsRunWon() const { return m_RunWon; }
    /// The echo open in the Echo menu (Lore::GetEcho), an ending included.
    int GetEchoShown() const { return m_EchoShown; }

    static const char* ItemName(ItemKind kind);
    /// What a thing does, in a few words.
    static const char* ItemHelp(ItemKind kind);
    static const char* MonsterName(MonsterKind kind);
    static bool IsBoss(MonsterKind kind);

private:
    DungeonMap m_Map;
    Random m_Random;
    uint32_t m_Seed = 1;
    int m_Depth = 1;
    int m_Deepest = 1;
    Cell m_Hero;
    Cell m_HeroFrom;
    int m_Facing = 0;
    int m_FacingFrom = 0;
    int m_GlideMs = 0;
    int m_GlideLength = 1;
    int m_Hp = 20;
    int m_MaxHp = 20;
    int m_Attack = 2;
    int m_Defense = 0;
    int m_Level = 1;
    int m_Xp = 0;
    int m_Gold = 0;
    int m_ShakeMs = 0;
    int m_SwingMs = 0;
    Event m_Events[kMaxEvents];
    int m_EventCount = 0;
    bool m_Critical = false;  // the hit being dealt is a critical
    void AddEvent(EventKind kind, int amount, const Monster& monster);
    void AddBreak(Cell at, int prop);
    void AddChip(Cell wall);
    int m_BlastMs = 0;
    uint32_t m_TimeMs = 0;
    bool m_KnowsSecrets = false;
    bool m_FallPending = false;
    // The step being glided went through an impossible door: this one.
    bool m_Through = false;
    Portal m_ThroughPortal;
    Monster m_Monsters[kMaxMonsters];
    Item m_Items[kMaxItems];
    Feature m_Features[kMaxFeatures];
    Prop m_Props[kMaxProps];
    Torch m_Torches[kMaxTorches];
    // Traps per tile: 0 none, 1 spikes, 2 trapdoor; found ones have bit 4.
    uint8_t m_Traps[kMapHeight][kMapWidth] = {};
    int m_Bag[kItemKinds] = {};
    int m_PagesKnown = 0;
    int m_PageShown = 1;
    Weapon m_Weapon = Weapon::Blade;
    int m_Mp = 8;
    int m_MaxMp = 8;
    int m_MpSteps = 0;
    int m_FlashMs = 0;
    int m_FlashKind = 0;
    // A fire bolt flying, for the view: from, to, how long.
    struct Bolt
    {
        Fix fromX = 0;
        Fix fromY = 0;
        Fix toX = 0;
        Fix toY = 0;
        int ms = 0;
        int total = 1;
    };
    Bolt m_Bolt;
    // Puddle's goods on this floor.
    MenuEntry m_Goods[4];
    int m_GoodsCount = 0;
    Menu m_Menu = Menu::None;
    MenuEntry m_MenuEntries[kMaxMenu];
    int m_MenuCount = 0;
    int m_MenuCursor = 0;
    char m_Message[kMessageLength] = {};
    uint32_t m_MessageCount = 0;
    uint32_t m_Version = 0;
    // The story: kept over runs, then this run's part.
    Legend m_Legend;
    uint32_t m_LegendVersion = 0;
    int m_Shards = 0;
    uint8_t m_ShardBits = 0;  // which zones' shards were found this run
    bool m_ShardHere = false;
    int m_SnacksGiven = 0;
    bool m_HasStaff = false;
    uint16_t m_WordsUsed = 0;  // names given to a stone listener this run, a bit each
    bool m_InSeam = false;
    bool m_KeeperCalled = false;   // it called for help, once
    bool m_EndingPending = false;  // it fell: the ending's choice comes once the turn is done
    bool m_RunWon = false;
    bool m_DeathRecorded = false;
    int m_EchoOnFloor = -1;  // the echo this floor's stone holds
    int m_EchoShown = 0;
    Cell m_KnockAt;
    int m_WallBlows = 0;
    bool m_HollowSaid = false;  // the cracked wall was bumped into, and said so
    int m_DescendTo = 1;
    bool m_DescendAsked = false;  // since the hero came to stand where they are
    // The thing whose choices are open: a prop, a fountain, or food out of
    // the bag (an ItemKind).
    int m_ThingProp = -1;
    int m_ThingFeature = -1;
    int m_ThingItem = -1;
    char m_Speech[kMessageLength] = {};
    uint32_t m_SpeechCount = 0;
    char m_ResultText[kMessageLength] = {};
    int m_ResultItem = -1;
    Reveal m_Reveal = Reveal::Now;
    uint32_t m_ResultCount = 0;
    int m_Gained = -1;  // the last thing gained, for the result to show
    int m_SoundVolume = 50;

public:
    static constexpr int kMaxCues = 6;

private:
    Cue m_Cues[kMaxCues] = {};
    int m_CueCount = 0;
    void Sound(Cue cue);
    int m_Tipsy = 0;
    bool m_InVillage = false;
    // Someone speaking several lines: those still to come, and what follows
    // the last (0 nothing; 1 + a contract: it is taken; 101 + one: kept).
    const char* const* m_SpeechLines = nullptr;
    int m_SpeechLeft = 0;
    int m_AfterSpeech = 0;
    int m_ContractNews = -1;  // a contract just done below, still to be shown
    uint8_t m_Gossiped = 0;   // which kept contracts the town has spoken of
    void SpeakLines(const char* const* lines, int count, int after);
    bool NextSpeech();
    void AfterSpeech();
    /// True when `giver` (a Contracts::Giver) has a contract to offer or to
    /// pay.
    bool HasWord(int giver) const;
    /// `giver` speaks of their contract: offers it, pays it, or with
    /// `remind` says what is still to do. False when there is nothing to say.
    bool TalkOfContract(int giver, bool remind);
    void ContractFound(int contract);
    bool ShowContractNews();
    int FencePrice(int material) const;
    const char* Gossip();
    void FillContractsMenu();
    void PlaceNamed();
    int m_Talks[Village::kVillagers] = {};  // lines heard from each villager
    int m_FenceTalks = 0;
    int m_Inside = -1;  // the building the hero is in, or -1
    DeathReport m_DeathReport;
    int m_RecordBefore = 0;  // the deepest floor reached before this run
    int m_KeeperTalks[Village::kBuildings] = {};
    int m_Materials[Village::kMaterials] = {};
    // What GetTarget says about the village's buildings, written as it asks.
    mutable char m_TargetName[32] = {};
    mutable char m_TargetDetail[56] = {};
    const Lore::Book* m_Reading = nullptr;
    uint16_t m_BooksRead = 0;  // this run's, a bit each: each teaches once
    // Notes read this run, to read again from the menu.
    static constexpr int kMaxNotes = 6;
    const Lore::Book* m_Notes[kMaxNotes] = {};
    int m_NoteCount = 0;

    void ActOnce(Action action);
    void ResetRun(uint32_t seed);
    void SayRunStart();
    void EnterVillage();
    /// Empties the place of monsters, things, props, torches and traps.
    void ClearPlace();
    void EnterBuilding(int building);
    void LeaveBuilding();
    void TalkToKeeper();
    void StartRun(int depth);
    void ReturnHome();
    void TryUpgrade(int building);
    void BankMaterials();
    void EnterFloor(int depth);
    void EnterSeam();
    void RecordDeath();
    void PlaceLore();
    bool PlaceEchoStone();
    int NextEcho() const;
    void ReadEcho(Cell at);
    void GiveSnack();
    void ChooseEnding(int choice);
    Cell FarthestFloor();
    void Say(const char* text);
    void StepTo(Cell to);
    void HeroAttacks(int monster);
    void HurtMonster(int index, int damage, const char* how);
    bool CastSpell(Spell spell);
    void MonstersAct();
    void PickUp() { PickUpAt(m_Hero); }
    void PickUpAt(Cell at);
    void UseProp(int index);
    void UseFountain(int feature);
    void UseShrine(int feature);
    /// Which name a stone listener would be given next (0 Puddle's, 1
    /// Halvard's, 2 Sigrun's, 3 the ducks' once all are found), or -1.
    int NextName() const;
    /// Blocking things never stand there: in a corridor, in a doorway, or
    /// before a cracked wall or a locked door.
    bool InTheWay(Cell c) const;
    /// Food out of the bag: eat it, or throw it.
    void UseFood(ItemKind kind);
    void ThrowFood(ItemKind kind);
    /// What eating a cave mushroom does: anything.
    void EatGlowcap();
    /// A new prop's choices all open, and what it holds decided.
    void SetUpProp(Prop& prop);
    /// The deed a prop's choice stands for, or None once it is used up.
    Deed PropDeed(const Prop& prop, int choice) const;
    Deed ThingDeed(int choice) const;
    void FillThingMenu();
    void DoDeed(const MenuEntry& entry);
    void DoDeedNow(const MenuEntry& entry);
    /// Someone says `line`: shown until a key.
    void Speak(const char* line);
    /// Shows what was just said as what happened.
    void ShowResult(Reveal reveal);
    /// The weapon swung at whatever is ahead, or at the air: a turn.
    void Swing();
    void StrikeWall(Cell at);
    /// Asks whether to go down, when the hero stands on a way down that is
    /// open. False when it is not one.
    bool AskDescend();
    void Descend();
    void BuyMaterial(int material);
    /// What a pot, a crate or a barrel held: said after `from`, or `empty`.
    void LootContainer(const char* from, const char* empty);
    void SearchProp(Prop& prop, const char* name);
    void ReadBook();
    void ReadNote();
    void Sleep();
    bool PickLock();
    void PullBrick(Cell at);
    void SellValuables();
    bool PlaceLooseBrick();
    /// Stands a prop of `kind` by a wall of room `room`, beside `near` when
    /// that is a tile (a second pot by the first). False when it found no
    /// place.
    bool TryPlaceProp(int kind, int room, Cell near = { -1, -1 }, bool any = false);
    /// Stands a chest (a thing that stays, and may not be a chest) on `at`.
    /// False when it would be in the way there.
    bool PlaceChest(Cell at);
    /// Pillars in the corners of some big rooms.
    void PlacePillars();
    /// One room dressed as something: a library, a crypt, a storeroom.
    void PlaceThemedRoom();
    void BreakProp(Prop& prop);
    int ValuablesWorth() const;
    void Gain(ItemKind kind, const char* from);
    void GainXp(int xp);
    bool Carry(ItemKind kind, int count, const char* from);
    bool IsFree(Cell c) const;
    bool IsEmptyFloor(Cell c) const;
    bool PlaceMonster(MonsterKind kind, Cell at);
    bool PlaceItem(ItemKind kind, Cell at);
    bool PlaceFeature(FeatureKind kind);
    bool CutsNothingOff(Cell c);
    void PlaceProps();
    void PlaceTorches();
    void PlaceTraps();
    void StockShop();
    MonsterKind PickKind();
    ItemKind PickLoot(bool chest);
    /// `kind`, or a whetstone in place of a weapon not unlocked yet.
    ItemKind Unlocked(ItemKind kind) const;
    /// What a record of `now` opened that `before` had not, or null.
    static const char* UnlockedBetween(int before, int now);
    void FillGoal(DeathReport& report) const;
    bool BossAlive() const;
    void DrinkFountain(Feature& fountain);
    void Pray(Feature& shrine);
    void ReadPage();
    void OpenMenu(Menu menu);
    void FillMenu();
    void ChooseInMenu();
    // 0: not used, 1: used (a turn passes), 2: used and the floor changed.
    int UseItem(ItemKind kind);
    void Blast();
    void TurnEnds();
    static Cell Ahead(Cell from, int facing, int steps);
};

}  // namespace Dungeon
