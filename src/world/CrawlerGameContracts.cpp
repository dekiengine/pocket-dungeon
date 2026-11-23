// The contracts of Last Hearth in play: someone asking (a few lines, one at
// a time), the thing turning up below, word of it brought back and paid, and
// the list of them on the notice board.

#include "CrawlerGame.h"

#include "Contracts.h"
#include "CrawlerGameData.h"
#include "Village.h"

#include <cstdio>

namespace Dungeon
{

using namespace GameData;
using Contracts::Id;
using Contracts::State;

void CrawlerGame::SpeakLines(const char* const* lines, int count, int after)
{
    Speak(lines[0]);
    m_SpeechLines = lines + 1;
    m_SpeechLeft = count - 1;
    m_AfterSpeech = after;
}

bool CrawlerGame::NextSpeech()
{
    if (m_SpeechLeft <= 0)
    {
        return false;
    }
    const char* const* lines = m_SpeechLines;
    const int left = m_SpeechLeft;
    const int after = m_AfterSpeech;
    Speak(lines[0]);
    m_SpeechLines = lines + 1;
    m_SpeechLeft = left - 1;
    m_AfterSpeech = after;
    return true;
}

void CrawlerGame::AfterSpeech()
{
    const int after = m_AfterSpeech;
    m_AfterSpeech = 0;
    if (after == 0)
    {
        return;
    }
    // A contract taken, or kept and paid: said on a card of its own.
    const Id id = static_cast<Id>((after - 1) % 100);
    const Contracts::Info& info = Contracts::GetInfo(id);
    char text[kMessageLength];
    if (after < 100)
    {
        Contracts::SetState(m_Legend, id, State::Taken);
        if (id == Id::Ledger && (m_Legend.bosses & 2) != 0)
        {
            Contracts::SetState(m_Legend, id, State::Found);  // the baron fell already: the book is in hand
        }
        Sound(Cue::Pray);
        m_Gained = static_cast<int>(ItemKind::Note);
        std::snprintf(text, sizeof(text), "Contract: %s.", info.name);
    }
    else
    {
        Contracts::SetState(m_Legend, id, State::Kept);
        m_Legend.bank += id == Id::Meals ? 60 : (id == Id::Ledger ? 150 : 0);
        Sound(Cue::Home);
        m_Gained = static_cast<int>(info.foundItem);
        std::snprintf(text, sizeof(text), "Contract kept: %s. %s", info.name, info.reward);
    }
    ++m_LegendVersion;
    Say(text);
    ShowResult(Reveal::Now);
}

bool CrawlerGame::HasWord(int giver) const
{
    const int contract = Contracts::Of(static_cast<Contracts::Giver>(giver));
    if (contract < 0)
    {
        return false;
    }
    const Id id = static_cast<Id>(contract);
    const State state = Contracts::GetState(m_Legend, id);
    return state == State::Unasked || state == State::Found ||
           (state == State::Taken && id == Id::Question && Lore::CountBits(m_Legend.echoes) >= Contracts::kEchoesAsked);
}

bool CrawlerGame::TalkOfContract(int giver, bool remind)
{
    const int contract = Contracts::Of(static_cast<Contracts::Giver>(giver));
    if (contract < 0)
    {
        return false;
    }
    const Id id = static_cast<Id>(contract);
    const Contracts::Info& info = Contracts::GetInfo(id);
    const State state = Contracts::GetState(m_Legend, id);
    if (state == State::Unasked)
    {
        SpeakLines(info.offer, Contracts::kOfferLines, 1 + contract);
        return true;
    }
    if (state == State::Found || (state == State::Taken && HasWord(giver)))
    {
        SpeakLines(info.back, Contracts::kBackLines, 101 + contract);
        return true;
    }
    if (state == State::Taken && remind)
    {
        Speak(info.remind);
        return true;
    }
    return false;
}

void CrawlerGame::ContractFound(int contract)
{
    const Id id = static_cast<Id>(contract);
    if (Contracts::GetState(m_Legend, id) != State::Taken)
    {
        return;
    }
    Contracts::SetState(m_Legend, id, State::Found);
    ++m_LegendVersion;
    m_ContractNews = contract;
}

bool CrawlerGame::ShowContractNews()
{
    if (m_ContractNews < 0 || m_Menu != Menu::None || IsDead())
    {
        return false;
    }
    const Contracts::Info& info = Contracts::GetInfo(static_cast<Id>(m_ContractNews));
    m_ContractNews = -1;
    if (!info.found)
    {
        return false;
    }
    m_Gained = static_cast<int>(info.foundItem);
    Sound(Cue::Pray);
    Say(info.found);
    ShowResult(Reveal::Opened);
    return true;
}

int CrawlerGame::FencePrice(int material) const
{
    // Cheaper once his brother rests.
    const int price = Village::MaterialPrice(material);
    return Contracts::IsKept(m_Legend, Id::Brother) ? price * 6 / 10 : price;
}

const char* CrawlerGame::Gossip()
{
    // What the town says of a contract kept, each once.
    for (int c = 0; c < Contracts::kContracts; ++c)
    {
        if (Contracts::IsKept(m_Legend, static_cast<Id>(c)) && (m_Gossiped & (1u << c)) == 0)
        {
            m_Gossiped = static_cast<uint8_t>(m_Gossiped | (1u << c));
            return Contracts::GetInfo(static_cast<Id>(c)).gossip;
        }
    }
    return nullptr;
}

void CrawlerGame::FillContractsMenu()
{
    // Those taken, done or paid; and who wants a word, of those who can be
    // reached (a keeper's house must stand).
    for (int c = 0; c < Contracts::kContracts && m_MenuCount < kMaxMenu; ++c)
    {
        const Id id = static_cast<Id>(c);
        const Contracts::Info& info = Contracts::GetInfo(id);
        const State state = Contracts::GetState(m_Legend, id);
        const int giver = static_cast<int>(info.giver);
        const bool reachable = giver >= Village::kBuildings || m_Legend.buildings[giver] > 0;
        if (state == State::Unasked && !reachable)
        {
            continue;
        }
        MenuEntry& e = m_MenuEntries[m_MenuCount++];
        e = MenuEntry{};
        e.label = state == State::Unasked ? info.wants : info.name;
        e.item = state == State::Unasked ? ItemKind::Scroll : ItemKind::Note;
        e.price = c;
        e.count = static_cast<int>(state);
    }
}

const char* CrawlerGame::GetContractHint(const MenuEntry& entry) const
{
    const Id id = static_cast<Id>(entry.price);
    const Contracts::Info& info = Contracts::GetInfo(id);
    const State state = static_cast<State>(entry.count);
    if (state == State::Unasked)
    {
        return info.where;
    }
    if (state == State::Taken && id == Id::Question)
    {
        std::snprintf(m_TargetDetail, sizeof(m_TargetDetail), "Listen to five echo stones. Heard %d.",
                      Lore::CountBits(m_Legend.echoes));
        return m_TargetDetail;
    }
    return info.hints[static_cast<int>(state) - 1];
}

void CrawlerGame::PlaceNamed()
{
    // Orm, on his floor, while his brother's contract is open: twice as
    // hard to put down as what he became.
    if (m_InSeam || m_Depth != Contracts::kOrmFloor || Contracts::GetState(m_Legend, Id::Brother) != State::Taken)
    {
        return;
    }
    Cell at;
    const auto away = [&](Cell c)
    {
        const int dx = c.x - m_Hero.x;
        const int dy = c.y - m_Hero.y;
        return IsFree(c) && dx * dx + dy * dy >= 16;
    };
    if (m_Map.RandomFloor(m_Random, away, at) && PlaceMonster(MonsterKind::Ragmummy, at))
    {
        Monster& orm = m_Monsters[MonsterAt(at)];
        orm.named = true;
        orm.hp = orm.hp * 2 + 6;
        orm.maxHp = orm.hp;
    }
}

}  // namespace Dungeon
