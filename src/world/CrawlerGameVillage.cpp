// Home: what a fall or the way back brings home, the village and its buildings'
// insides, their keepers and the upgrades.

#include "CrawlerGame.h"

#include "Contracts.h"
#include "CrawlerGameData.h"
#include "Village.h"

#include <cstdio>
#include <cstring>

namespace Dungeon
{

using namespace GameData;

void CrawlerGame::RecordDeath()
{
    m_DeathRecorded = true;
    ++m_Legend.deaths;
    // The bank keeps part of the gold; the ghost holds the rest. The
    // materials always make it home.
    const int kept = m_Gold * Village::KeptOnDeath(m_Legend.buildings[static_cast<int>(Village::Building::Bank)]) / 100;
    m_Legend.bank += kept;
    m_Legend.ghostDepth = m_InSeam ? 0 : m_Depth;
    m_Legend.ghostGold = m_Gold - kept;
    m_DeathReport = DeathReport{};
    m_DeathReport.depth = m_Legend.ghostDepth;
    m_DeathReport.keptGold = kept;
    m_DeathReport.echoGold = m_Gold - kept;
    for (int m : m_Materials)
    {
        m_DeathReport.materials += m;
    }
    BankMaterials();
    // What the run opened, and what to work toward now.
    m_DeathReport.record = !m_InSeam && m_Depth > m_RecordBefore;
    m_DeathReport.unlocked = UnlockedBetween(m_RecordBefore, m_Deepest);
    FillGoal(m_DeathReport);
    ++m_LegendVersion;
}

void CrawlerGame::FillGoal(DeathReport& report) const
{
    // The building nearest its next level: how much of its gold and its
    // material is in already, each counted up to all of it.
    int best = -1;
    int bestScore = -1;
    for (int b = 0; b < Village::kBuildings; ++b)
    {
        const int next = m_Legend.buildings[b] + 1;
        const int boss = Village::BossNeeded(b, next);
        if (next > Village::kMaxLevel || (boss != 0 && (m_Legend.bosses & boss) == 0))
        {
            continue;
        }
        const Village::Cost cost = Village::UpgradeCost(b, next);
        const int gold = m_Legend.bank < cost.gold ? m_Legend.bank : cost.gold;
        const int have =
            m_Legend.materials[cost.material] < cost.amount ? m_Legend.materials[cost.material] : cost.amount;
        const int score =
            gold * 100 / (cost.gold > 0 ? cost.gold : 1) + have * 100 / (cost.amount > 0 ? cost.amount : 1);
        if (score > bestScore)
        {
            bestScore = score;
            best = b;
        }
    }
    report.goal[0][0] = 0;
    report.goal[1][0] = 0;
    report.goalReady = false;
    if (best < 0)
    {
        return;
    }
    const int next = m_Legend.buildings[best] + 1;
    const Village::Cost cost = Village::UpgradeCost(best, next);
    const char* material = Village::MaterialName(cost.material);
    if (bestScore >= 200)
    {
        report.goalReady = true;
        std::snprintf(report.goal[0], sizeof(report.goal[0]),
                      next == 1 ? "The %s can be built now." : "The %s can grow now.", Village::BuildingName(best));
        return;
    }
    std::snprintf(report.goal[0], sizeof(report.goal[0]), "The %s needs %d gold and %d %s.",
                  Village::BuildingName(best), cost.gold, cost.amount, material);
    std::snprintf(report.goal[1], sizeof(report.goal[1]), "The village has %d gold and %d %s.", m_Legend.bank,
                  m_Legend.materials[cost.material], material);
}

void CrawlerGame::BankMaterials()
{
    for (int m = 0; m < Village::kMaterials; ++m)
    {
        m_Legend.materials[m] += m_Materials[m];
        m_Materials[m] = 0;
    }
}

void CrawlerGame::EnterVillage()
{
    m_InVillage = true;
    m_Inside = -1;
    m_InSeam = false;
    m_Depth = 1;
    Village::BuildMap(m_Map);
    m_Hero = Village::StartCell();
    m_HeroFrom = m_Hero;
    m_Facing = Village::StartFacing();
    m_FacingFrom = m_Facing;
    m_GlideMs = 0;
    m_KnowsSecrets = false;
    m_FallPending = false;
    m_Menu = Menu::None;
    m_EchoOnFloor = -1;
    m_ShardHere = false;
    m_WallBlows = 0;
    m_GoodsCount = 0;
    ClearPlace();
    // Two barrels by the tavern.
    for (int i = 0; i < 2; ++i)
    {
        Prop& barrel = m_Props[i];
        barrel.at = Village::BarrelCell(i);
        barrel.kind = kAleBarrel;  // the mossy halls' barrel
        barrel.facing = 1;
        barrel.present = true;
        SetUpProp(barrel);
    }
    // The villagers.
    for (int v = 0; v < Village::kVillagers; ++v)
    {
        Feature& f = m_Features[1 + v];
        f.kind = v == 0 ? FeatureKind::Granny : FeatureKind::Kid;
        f.at = Village::VillagerCell(v);
        f.present = true;
    }
    // What stands about only to look at: a brazier, crates.
    for (int i = 0; i < Village::kDressings; ++i)
    {
        const Village::Furnishing dressing = Village::Dressing(i);
        Prop& prop = m_Props[3 + Village::kStones + i];
        prop.at = dressing.at;
        prop.kind = dressing.prop;
        prop.fixed = true;
        prop.facing = dressing.facing;
        prop.present = true;
    }
    // The notice board, across the square from where a visit starts.
    Feature& board = m_Features[2 + Village::kVillagers];
    board.kind = FeatureKind::Board;
    board.at = Village::BoardCell();
    board.present = true;
    // The man at the alley's dead end.
    Feature& fence = m_Features[1 + Village::kVillagers];
    fence.kind = FeatureKind::Fence;
    fence.at = Village::FenceCell();
    fence.present = true;
    // The yard past him: the stones, and the candles someone keeps lit.
    for (int i = 0; i <= Village::kStones; ++i)
    {
        Prop& prop = m_Props[2 + i];
        prop.at = i < Village::kStones ? Village::StoneCell(i) : Village::CandlesCell();
        prop.kind = static_cast<uint8_t>(i < Village::kStones ? kGravestone : kShrine);
        prop.fixed = true;
        prop.facing = i < Village::kStones ? 1 : 2;
        prop.present = true;
    }
    // The lift's platform shows its rune once it is built.
    if (m_Legend.buildings[static_cast<int>(Village::Building::Lift)] > 0)
    {
        m_Map.SetLook(Village::LiftCell(), FloorLook::Rune);
    }
    // Daylight: no torches, but for the lantern over the man in the alley.
    Torch& lantern = m_Torches[0];
    lantern.x = Centre(Village::FenceCell().x);
    lantern.y = Centre(Village::FenceCell().y) - kFixOne * 40 / 100;
    lantern.facing = 1;  // out from the wall north of him
    lantern.present = true;
    m_Map.RevealAll();
    m_Map.ComputeView(m_Hero, 6);
}

void CrawlerGame::ClearPlace()
{
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
    for (Prop& prop : m_Props)
    {
        prop = Prop{};
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
}

void CrawlerGame::EnterBuilding(int building)
{
    m_Inside = building;
    Village::BuildInterior(m_Map);
    m_Hero = Village::InteriorStart();
    m_HeroFrom = m_Hero;
    m_Facing = Village::InteriorFacing();
    m_FacingFrom = m_Facing;
    m_GlideMs = 0;
    m_Menu = Menu::None;
    ClearPlace();
    Feature& keeper = m_Features[0];
    keeper.kind = FeatureKind::Keeper;
    keeper.at = Village::KeeperCell();
    keeper.present = true;
    const Village::Furnishing* furnishings = nullptr;
    const int count = Village::Furnish(building, furnishings);
    for (int i = 0; i < count && i < kMaxProps; ++i)
    {
        Prop& prop = m_Props[i];
        prop.at = furnishings[i].at;
        prop.kind = furnishings[i].prop;
        prop.fixed = true;
        prop.facing = furnishings[i].facing;
        prop.present = true;
    }
    // A torch on each side wall, the near half of the room.
    constexpr Cell kTorchCells[2] = { { 9, 11 }, { 13, 11 } };
    for (int i = 0; i < 2; ++i)
    {
        const int toWall = i == 0 ? 2 : 0;  // west, then east
        Torch& t = m_Torches[i];
        t.x = Centre(kTorchCells[i].x) + (toWall == 0 ? 1 : -1) * (kFixOne * 40 / 100);
        t.y = Centre(kTorchCells[i].y);
        t.facing = static_cast<uint8_t>((toWall + 2) & 3);  // out from its wall
        t.present = true;
    }
    m_Map.RevealAll();
    m_Map.ComputeView(m_Hero, 6);
    ++m_Version;
}

void CrawlerGame::LeaveBuilding()
{
    const int building = m_Inside;
    EnterVillage();
    m_Hero = Village::OutsideCell(building);
    m_HeroFrom = m_Hero;
    m_Facing = Village::OutsideFacing(building);
    m_FacingFrom = m_Facing;
    m_Map.ComputeView(m_Hero, 6);
    ++m_Version;
}

void CrawlerGame::TalkToKeeper()
{
    // What they have to ask or to pay comes first; then, with enough for
    // it, the next level is built; otherwise a reminder of what was asked,
    // or a chat.
    const int building = m_Inside;
    const int level = m_Legend.buildings[building];
    if (TalkOfContract(building, false))
    {
        return;
    }
    if (level < Village::kMaxLevel && Village::BossNeeded(building, level + 1) == 0)
    {
        const Village::Cost cost = Village::UpgradeCost(building, level + 1);
        if (m_Legend.bank >= cost.gold && m_Legend.materials[cost.material] >= cost.amount)
        {
            TryUpgrade(building);
            return;
        }
    }
    const int talks = m_KeeperTalks[building]++;
    if (!TalkOfContract(building, talks % 3 == 0))
    {
        Speak(Village::KeeperLine(building, talks));
    }
}

void CrawlerGame::ReturnHome()
{
    Sound(Cue::Home);
    // Everything comes home: the gold to the bank, the materials to the
    // store.
    const int gold = m_Gold;
    int materials = 0;
    for (int m : m_Materials)
    {
        materials += m;
    }
    m_Legend.bank += m_Gold;
    BankMaterials();
    ++m_LegendVersion;
    // Three glowcaps carried home whole: Ingrid's contract.
    const bool herbs = Carried(ItemKind::Glowcap) >= Contracts::kGlowcaps;
    GoHome(m_Seed * 1103515245u + 12345u);
    char text[kMessageLength];
    std::snprintf(text, sizeof(text), "Home. %d gold and %d materials go to the village.", gold, materials);
    Say(text);
    if (herbs)
    {
        ContractFound(static_cast<int>(Contracts::Id::Herb));
        ShowContractNews();
    }
    // A record that opened something says so.
    if (const char* news = UnlockedBetween(m_RecordBefore, m_Deepest))
    {
        Say(news);
    }
}

void CrawlerGame::TryUpgrade(int building)
{
    uint8_t& level = m_Legend.buildings[building];
    const char* name = Village::BuildingName(building);
    char text[kMessageLength];
    if (level >= Village::kMaxLevel)
    {
        std::snprintf(text, sizeof(text), "The %s is as grand as it gets.", name);
        Say(text);
        return;
    }
    const int next = level + 1;
    const Village::Cost cost = Village::UpgradeCost(building, next);
    const int boss = Village::BossNeeded(building, next);
    if (boss != 0 && (m_Legend.bosses & boss) == 0)
    {
        constexpr const char* kBosses[3] = { "gilded maw", "carrion king", "threefold wyrm" };
        std::snprintf(text, sizeof(text), "Too dangerous down there. Beat the %s first.",
                      kBosses[boss == 1   ? 0
                              : boss == 2 ? 1
                                          : 2]);
        Say(text);
        return;
    }
    if (m_Legend.bank < cost.gold || m_Legend.materials[cost.material] < cost.amount)
    {
        std::snprintf(text, sizeof(text), "Needs %d gold and %d %s. You have %d and %d.", cost.gold, cost.amount,
                      Village::MaterialName(cost.material), m_Legend.bank, m_Legend.materials[cost.material]);
        Say(text);
        return;
    }
    m_Legend.bank -= cost.gold;
    m_Legend.materials[cost.material] -= cost.amount;
    level = static_cast<uint8_t>(next);
    ++m_LegendVersion;
    ++m_Version;
    if (building == static_cast<int>(Village::Building::Lift))
    {
        m_Map.SetLook(Village::LiftCell(), FloorLook::Rune);
    }
    std::snprintf(text, sizeof(text), next == 1 ? "The %s is built! %s." : "The %s grows! %s.", name,
                  Village::Effect(building, next));
    Say(text);
}
}  // namespace Dungeon
