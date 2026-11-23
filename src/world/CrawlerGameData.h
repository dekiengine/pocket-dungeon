#pragma once

// What the parts of CrawlerGame share: how long things take, the monsters'
// numbers, what goes in the bag and what it costs, Puddle's rumours, the
// diary's pages, the props' names and lines, and a little arithmetic. Only
// for CrawlerGame's own .cpp files.

#include "CrawlerGame.h"

namespace Dungeon::GameData
{

// How long a step and a turn glide, in milliseconds.
inline constexpr int kStepMs = 170;
inline constexpr int kTurnMs = 140;
inline constexpr int kHurtMs = 240;
inline constexpr int kShakeMs = 160;
inline constexpr int kLungeMs = 240;
inline constexpr int kBlastMs = 320;
// Monsters further than this many steps sleep.
inline constexpr int kWakeDistance = 9;

// Trap kinds in CrawlerGame::m_Traps, and the bit for a found one.
inline constexpr uint8_t kSpikes = 1;
inline constexpr uint8_t kTrapdoor = 2;
inline constexpr uint8_t kTeleport = 3;  // the floor folds: somewhere else on it
inline constexpr uint8_t kAlarm = 4;     // a bell: things come
inline constexpr uint8_t kFound = 16;

struct MonsterStats
{
    const char* name;
    int hp;
    int attack;
    int defense;
    int xp;
    Fix scale;      // height against a wall's
    Fix hover;      // how far off the floor it floats
    int fromDepth;  // the first floor it is found on
    int toDepth;    // and the last
};

inline constexpr Fix Pct(int percent)
{
    return kFixOne * percent / 100;
}

inline constexpr MonsterStats kStats[kMonsterKinds] = {
    { "bile spawn", 3, 1, 0, 2, Pct(60), 0, 1, 4 },
    { "tallow wraith", 4, 1, 0, 3, Pct(58), 0, 1, 5 },
    { "glass watcher", 5, 2, 0, 4, Pct(68), 0, 2, 6 },
    { "dusk gazer", 4, 2, 0, 4, Pct(45), Pct(70), 2, 7 },
    { "cinder weaver", 6, 2, 0, 5, Pct(62), 0, 3, 8 },
    { "hollow hound", 8, 3, 0, 7, Pct(66), 0, 4, 10 },
    { "scald crawler", 12, 3, 0, 10, Pct(62), 0, 6, 14 },
    { "cauldron glutton", 16, 3, 1, 14, Pct(78), 0, 7, 99 },
    { "whispering codex", 13, 4, 0, 16, Pct(58), Pct(40), 8, 99 },
    { "shroud walker", 15, 5, 1, 18, Pct(78), 0, 10, 99 },
    { "mired adept", 20, 6, 1, 24, Pct(78), 0, 11, 99 },
    { "cairn warden", 28, 6, 2, 30, Pct(90), 0, 12, 99 },
    { "gilt wisp", 4, 0, 0, 40, Pct(50), 0, 0, 0 },
    { "gilded maw", 22, 4, 1, 25, Pct(85), 0, 0, 0 },
    { "carrion king", 45, 8, 2, 60, Pct(100), 0, 0, 0 },
    { "threefold wyrm", 70, 11, 3, 120, Pct(100), 0, 0, 0 },
    { "keeper", 80, 11, 3, 300, Pct(100), 0, 0, 0 },
    { "echo of you", 10, 3, 0, 20, Pct(70), Pct(12), 0, 0 },
};

inline const MonsterStats& StatsOf(MonsterKind kind)
{
    return kStats[static_cast<int>(kind)];
}

// The bag's kinds, in the order the bag lists them.
inline constexpr ItemKind kBagKinds[] = { ItemKind::Potion, ItemKind::Vial,  ItemKind::Water,   ItemKind::Elixir,
                                          ItemKind::Snack,  ItemKind::Fish,  ItemKind::Glowcap, ItemKind::Bomb,
                                          ItemKind::Scroll, ItemKind::Key,   ItemKind::Wing,    ItemKind::Lockpick,
                                          ItemKind::Gem,    ItemKind::Pearl, ItemKind::Goblet,  ItemKind::Blade,
                                          ItemKind::Axe,    ItemKind::Spear, ItemKind::Dagger,  ItemKind::Hammer };

inline bool IsFood(ItemKind kind)
{
    return kind == ItemKind::Snack || kind == ItemKind::Fish || kind == ItemKind::Glowcap;
}

inline ItemKind WeaponItem(Weapon weapon)
{
    return static_cast<ItemKind>(static_cast<int>(ItemKind::Blade) + static_cast<int>(weapon));
}

inline bool IsWeaponItem(ItemKind kind)
{
    return kind >= ItemKind::Blade && kind <= ItemKind::Hammer;
}

inline bool GoesInBag(ItemKind kind)
{
    for (ItemKind k : kBagKinds)
    {
        if (k == kind)
        {
            return true;
        }
    }
    return false;
}

inline int PriceOf(ItemKind kind, int depth)
{
    switch (kind)
    {
        case ItemKind::Potion: return 6 + 2 * depth;
        case ItemKind::Bomb: return 10 + 2 * depth;
        case ItemKind::Scroll: return 12 + 2 * depth;
        case ItemKind::Key: return 15 + 3 * depth;
        case ItemKind::Wing: return 20 + 3 * depth;
        case ItemKind::Elixir: return 35 + 5 * depth;
        case ItemKind::Lockpick: return 6 + depth;
        case ItemKind::Sword:
        case ItemKind::Shield: return 25 + 10 * depth;
        case ItemKind::Blade:
        case ItemKind::Axe:
        case ItemKind::Spear:
        case ItemKind::Dagger:
        case ItemKind::Hammer: return 30 + 8 * depth;
        default: return 99;
    }
}

inline const char* const kRumours[] = {
    "Puddle: Cracked walls hide rooms. A bomb opens them.",
    "Puddle: A locked vault? Keys are rare. Keep yours.",
    "Puddle: Gilt wisps run. Corner them.",
    "Puddle: They built rooms down there. A chapel, even.",
    "Puddle: Every fifth floor, something big waits.",
    "Puddle: A scroll shows traps, and walls that lie.",
    "Puddle: Some floors give way. Mind your step.",
    "Puddle: Come, look. Gold for goods. Fair is fair.",
    "Puddle: The frogs listen. Try my name on one.",
    "Puddle: Meet a mired adept? Heal her. Trust me.",
    "Puddle: A cracked wall? Strike it. Five times.",
    "Puddle: Snacks. I would trade a great deal for one.",
};
inline constexpr int kRumourCount = static_cast<int>(sizeof(kRumours) / sizeof(kRumours[0]));

// Halvard's journal. Each page: up to four lines of 20 characters.
inline const char* const kPages[kLorePages] = {
    "Hunger Winter, day\n40. The fever is in\nevery house but the\ncouncil's.",
    "They voted. The sick\ngo into the mine and\nthe door is shut. I\nhold the only key.",
    "Sten would not stay\nabove. I turned the\nkey on him myself.\nHe thanked me.",
    "The old walls crack.\nA bomb opens them.\nSo do five blows,\nif you are patient.",
    "Year 12. Knocking\nfrom below, still.\nThe council calls it\nwater in the rock.",
    "They kept working,\nbelow. New rooms. A\nchapel. A store. I\nhear the hammers.",
    "Grave-coins crawl\noff the dead. They\nrun from the living.\nCatch one. Count it.",
    "The baron hid grain\nwhile we starved. He\nis down there too.\nFatter. Crowned.",
    "The frog shrines\nhear names. Miners\ntold them their dead\nso none were lost.",
    "Three signed the\norder. I saw what\nthey are now. One\nbody. Three mouths.",
    "Year 40. I am going\ndown to open what I\nshut. If I do not\ncome up, follow.",
    "You read it all. So:\nhis name. I cannot\nwrite it. Say it for\nme, at the bottom.",
};

// A thing standing about: its name, what you see facing it, and the choices
// it offers.
struct PropInfo
{
    const char* name;
    const char* about;  // at most 37 characters
    Deed deeds[kThingDeeds];
};
inline constexpr PropInfo kPropInfo[kPropKinds] = {
    // The Mossy Halls.
    { "Old pack", "A traveller's pack. Nobody near it.", { Deed::Search, Deed::None } },
    { "Ale barrel", "Old ale, by the smell. It sloshes.", { Deed::Drink, Deed::Smash } },
    { "Berry bush", "Fat red berries. Nothing ate them.", { Deed::EatBerries, Deed::PickBerries } },
    { "Stone sentinel", "A stone sentinel. One eye is a gem.", { Deed::PryGem, Deed::Bow } },
    // The Bone Catacombs.
    { "Bone pile", "Bones, stacked with care. By whom?", { Deed::Search, Deed::None } },
    { "Candle shrine", "Candles, still lit. Coins in the wax.", { Deed::Pray, Deed::TakeOfferings } },
    { "Stone coffin", "The lid is heavy. It is not sealed.", { Deed::OpenCoffin, Deed::Knock } },
    { "Gravestone", "The name is worn off. Soft earth.", { Deed::ReadStone, Deed::Dig } },
    // The Flooded Caves.
    { "Crystal", "It hums. The air near it is cold.", { Deed::TouchCrystal, Deed::BreakCrystal } },
    { "Cave mushrooms", "Blue caps. They light the wall.", { Deed::EatMushroom, Deed::PickMushrooms } },
    { "Giant clam", "Big as a shield. A pearl glints.", { Deed::ReachForPearl, Deed::None } },
    { "Fish basket", "Silver fish, still wet. Whose?", { Deed::TakeFish, Deed::DigBasket } },
    // The Ember Forge.
    { "Anvil", "Still warm. A hammer lies across it.", { Deed::Sharpen, Deed::MendArmour } },
    { "Brazier", "Burning low. Someone still feeds it.", { Deed::Warm, Deed::Cook } },
    { "Weapon rack", "Old steel. Some of it still good.", { Deed::TakeFirst, Deed::TakeSecond } },
    { "Mine cart", "Full of ore. The rails run on.", { Deed::SearchCart, Deed::PushCart } },
    // Anywhere.
    { "Bookshelf", "Swollen with damp. One sticks out.", { Deed::Read, Deed::SearchShelf } },
    { "Camp", "A bedroll by a small fire. Nobody.", { Deed::Sleep, Deed::SearchCamp } },
    { "Locked chest", "Iron bands. A padlock like a fist.", { Deed::PickLock, Deed::ForceLock } },
    { "Clay pot", "Sealed with wax. Something rattles.", { Deed::Smash, Deed::ReachIn } },
    { "Crates", "Nailed shut by someone in a hurry.", { Deed::Smash, Deed::None } },
    { "Cage", "Something small inside. It watches.", { Deed::OpenCage, Deed::FeedCage } },
    { "Old chest", "No lock. The lid sits crooked.", { Deed::OpenChest, Deed::Prod } },
    { "Old mirror", "The glass is cracked. It still looks.", { Deed::LookMirror, Deed::BreakMirror } },
};
// Places in kPropInfo and the other tables.
inline constexpr int kAleBarrel = 1;
inline constexpr int kCoffin = kZoneProps + 2;
inline constexpr int kFishBasket = 2 * kZoneProps + 3;
inline constexpr int kWeaponRack = 3 * kZoneProps + 2;
inline constexpr int kBookshelf = kZones * kZoneProps;
inline constexpr int kCamp = kBookshelf + 1;
inline constexpr int kLockedChest = kBookshelf + 2;
inline constexpr int kPots = kBookshelf + 3;
inline constexpr int kCrate = kBookshelf + 4;
inline constexpr int kCage = kBookshelf + 5;
inline constexpr int kOldChest = kBookshelf + 6;
inline constexpr int kMirror = kBookshelf + 7;
inline constexpr int kSentinel = 3;
inline constexpr int kPack = 0;
inline constexpr int kBush = 2;
inline constexpr int kShrine = kZoneProps + 1;
inline constexpr int kGravestone = kZoneProps + 3;
inline constexpr int kMushrooms = 2 * kZoneProps + 1;
inline constexpr int kAnvil = 3 * kZoneProps;
// As a Prop's kind: a common one comes after its zone's.
inline constexpr int kPotsKind = kZoneProps + 3;
inline constexpr int kCrateKind = kZoneProps + 4;
inline constexpr int kCageKind = kZoneProps + 5;
inline constexpr int kOldChestKind = kZoneProps + 6;
inline constexpr int kMirrorKind = kZoneProps + 7;

inline constexpr const char* kNothingMore = "Nothing more to do here.";
inline constexpr const char* kStoneName = "Stone listener";
inline constexpr const char* kStoneAbout = "It keeps every name it is given.";
inline constexpr const char* kFountainName = "Old fountain";
inline constexpr const char* kFountainAbout = "Clear water. Coins glint in it.";

// What gravestones say.
inline constexpr const char* kEpitaphs[] = {
    "\"Here lies what I could not carry.\"",
    "\"She kept nothing. She is missed.\"",
    "\"He counted his gold twice. Then once more.\"",
    "\"Went down for one more floor.\"",
    "\"Do not dig. I mean it.\"",
    "\"Sealed below in the Hunger Winter. Forgive us.\"",
};
inline constexpr int kEpitaphCount = static_cast<int>(sizeof(kEpitaphs) / sizeof(kEpitaphs[0]));

// What each deed is called in a thing's menu, and what the hero knows of it
// before choosing (at most 37 characters).
struct DeedInfo
{
    const char* label;
    const char* hint;
};
inline constexpr DeedInfo kDeedInfo[kDeeds] = {
    { "", "" },
    { "Search it", "Whatever is in there is yours now." },
    { "Drink", "Warms the belly. Blurs the eyes." },
    { "Smash it", "Quick and loud. Keep what falls out." },
    { "Eat them", "A mouthful now. They look sweet." },
    { "Pick them", "For later. Or for a hungry frog." },
    { "Pray", "Kneel a while. Let the quiet in." },
    { "Take the coins", "Just lying there. The dead may mind." },
    { "Open it", "Push the lid. See who is home." },
    { "Knock", "Polite. And then you will know." },
    { "Touch it", "The hum wants in. Let it." },
    { "Break a piece off", "Worth carrying, if it holds." },
    { "Eat one", "They glow. So might you. Or not." },
    { "Pick them", "Dried, they are only food." },
    { "Reach for the pearl", "It is open a hand's width. Just." },
    { "Sharpen your blade", "A keener edge. It will bite deeper." },
    { "Mend your armour", "Beat the dents out. Blows land soft." },
    { "Warm yourself", "Sit. Let the cold out of your hands." },
    { "Cook a snack", "Over coals, a snack becomes a meal." },
    { "Take it", "" },
    { "Take it", "" },
    { "Read", "Someone marked a page." },
    { "Look behind it", "Things get hidden behind books." },
    { "Sleep", "Wake whole. If nothing finds you." },
    { "Go through it", "They left in a hurry. Or did not." },
    { "Pick the lock", "A lockpick, and patience." },
    { "Break it open", "It opens. Not all inside survives." },
    { "Reach inside", "Careful fingers. Pots can bite." },
    { "Pry out the gem", "It is loose. He will not like it." },
    { "Bow to him", "Old manners. They cost nothing." },
    { "Read it", "Somebody wanted this remembered." },
    { "Dig", "Soft earth. Someone dug before you." },
    { "Take a fish", "Supper. Or bait." },
    { "Dig under them", "Fishers keep their luck underneath." },
    { "Dig through the ore", "Most of it is slag. Not all." },
    { "Give it a shove", "Heavy. It will not stop for anyone." },
    { "Open the cage", "The latch is on your side." },
    { "Feed it", "Through the bars. See how it eats." },
    { "Open it", "A chest wants opening." },
    { "Prod it first", "A real chest will not mind." },
    { "Look into it", "It looks back. It always has." },
    { "Break it", "Silver behind the glass. Bad luck." },
    { "Eat it", "Yours. Now." },
    { "Throw it", "Something hungry may want it more." },
    { "Fill your waterskin", "For later. It keeps." },
    { "Drink deep", "Old water does strange things." },
    { "Wash your wounds", "Only wash. It eases what hurts." },
    { "Pray", "It may bless you. It may not." },
    { "Give it a name", "It gives something back for a name." },
    { "Say the heart's name", "His brother's name. No way back." },
};

inline Fix Centre(int v)
{
    return (v << kFixShift) + kFixOne / 2;
}

inline Fix Mul(Fix a, Fix b)
{
    return static_cast<Fix>((static_cast<int64_t>(a) * b) >> kFixShift);
}

// How far behind its tile's middle the hero's eye is.
inline constexpr Fix kEyeBack = kFixOne * 30 / 100;

inline Fix Lerp(Fix a, Fix b, int t1024)
{
    return a + static_cast<Fix>((static_cast<int64_t>(b - a) * t1024) >> 10);
}

}  // namespace Dungeon::GameData
