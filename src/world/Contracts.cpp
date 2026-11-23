#include "Contracts.h"

#include "CrawlerGame.h"

namespace Dungeon::Contracts
{

namespace
{

constexpr Info kInfo[kContracts] = {
    { "Twelve Meals",
      Giver::Gunnar,
      "Gunnar wants a word",
      "In the tavern.",
      { "Gunnar: The bog trader ate here all winter. Twelve meals.",
        "Gunnar: Never paid. Find him below. Say Gunnar remembers.",
        "Gunnar: Bring back whatever he gives. I am not proud." },
      "Gunnar: The trader. Twelve meals. Go on.",
      { "Gunnar: A pearl and four buttons. That is nine meals.",
        "Gunnar: The pearl goes to the bank. Drink. It is on him." },
      nullptr,
      ItemKind::Pearl,
      { "Find the bog trader below. Talk to him.", "Take what he gave back to Gunnar.", "Paid. Nine meals of twelve." },
      "60 gold to the village.",
      "Leif: Gunnar is smiling. Gran says to count the spoons." },
    { "A Brother's Knife",
      Giver::Ulf,
      "Ulf wants a word",
      "In the alley, west of the square.",
      { "Ulf: My brother Orm went down for water. The dry summer.",
        "Ulf: Something wears his coat on the third floor. End it.",
        "Ulf: Bring me his knife. A wolf's head on the pommel." },
      "Ulf: Third floor. He wears a grey coat. It was grey.",
      { "Ulf: That is it. He carved the wolf himself. Badly.",
        "Ulf: For you, all I sell is cheaper. Do not thank me." },
      "A knife with a wolf's head. Orm's. He rests now.",
      ItemKind::Dagger,
      { "Orm walks the third floor below.", "Bring Orm's knife to Ulf, in the alley.",
        "Orm rests. Ulf sells cheaper." },
      "Ulf sells cheaper.",
      "Leif: Ulf was crying in the alley. He said it was dust." },
    { "Astrid's Doll",
      Giver::Hilde,
      "Old Hilde wants a word",
      "In the square.",
      { "Hilde: Astrid was six. They carried her down with the sick.",
        "Hilde: Her mother sewed her a doll. It went down too.",
        "Hilde: It lies in the Ossuary, in a coffin or the bones." },
      "Hilde: The doll. Look in coffins. Look in the bone piles.",
      { "Hilde: Give it here. Oh. She called it Mouse.", "Hilde: I will lay it at her stone. Take this. It was mine." },
      "A rag doll, one eye a button. Sewn with care.",
      ItemKind::Snack,
      { "In the Ossuary: open coffins, search bones.", "Bring the doll to Old Hilde.",
        "The doll sits at Astrid's stone." },
      "Max HP +5, every run.",
      "Leif: There is a doll at the little stone. Gran put it." },
    { "The Baron's Ledger",
      Giver::Runa,
      "Runa wants a word",
      "In the bank.",
      { "Runa: The baron hid grain while the town buried children.",
        "Runa: He kept a ledger. He kept it on him. He is below.", "Runa: Kill what he became. Bring me the book." },
      "Runa: The carrion king, tenth floor. The ledger.",
      { "Runa: You have it. I will read it aloud at the well.",
        "Runa: Forty sacks. Forty. The bank owes you for this." },
      "In its robes: a ledger. Every sack counted.",
      ItemKind::Page,
      { "The carrion king, on B10, carries it.", "Bring the ledger to Runa, in the bank.",
        "Read aloud at the well. Forty sacks." },
      "150 gold to the village.",
      "Leif: Runa read a book at the well. Nobody would look up." },
    { "The Teacher's Question",
      Giver::Asgeir,
      "Asgeir wants a word",
      "In the wizard tower.",
      { "Asgeir: Halvard was my student. He turned the key.",
        "Asgeir: Did he choose it, or was he told? The stones know.", "Asgeir: Listen to five of them. Then tell me." },
      "Asgeir: Five stones. You have not heard five.",
      { "Asgeir: So. He was told, and he did it, and he stayed.",
        "Asgeir: That is worse. Sit. I will teach you what I can." },
      nullptr,
      ItemKind::Scroll,
      { "Listen to five echo stones.", "Tell Asgeir what the stones said.", "He was told. He did it. He stayed." },
      "Max MP +3, every run.",
      "Leif: The wizard has not slept. His lamp burns all night." },
    { "Fever Herb",
      Giver::Ingrid,
      "Ingrid wants a word",
      "In the apothecary.",
      { "Ingrid: Fever again, two houses. I will not be late twice.",
        "Ingrid: Glowcaps grow in the Sunken Reach. Bring me three.",
        "Ingrid: Carry them home. Do not eat them. You will want to." },
      "Ingrid: Three glowcaps. Carried home, not eaten.",
      { "Ingrid: Three. Whole. You did not even lick one.",
        "Ingrid: Both houses will live. Take a draught each time." },
      "Three glowcaps, whole. Ingrid is waiting.",
      ItemKind::Glowcap,
      { "Cave mushrooms, B11 to B15. Carry 3 home.", "Bring the glowcaps to Ingrid.", "Two houses lived." },
      "One more potion, every run.",
      "Leif: Nobody is coughing at the Larssons' any more." },
    { "Father's Hammer",
      Giver::Torvald,
      "Torvald wants a word",
      "In the smithy.",
      { "Torvald: My father forged the door. He never spoke again.",
        "Torvald: He threw his hammer down the shaft that night.",
        "Torvald: Anvils draw hammers. Look by one in the Foundry." },
      "Torvald: The Foundry. An anvil. His hammer.",
      { "Torvald: That is his. See the thumb, worn into the wood.",
        "Torvald: He made a door. I will make you a better blade." },
      "Under the anvil: a hammer, worn by one hand.",
      ItemKind::Hammer,
      { "Use an anvil in the Cinder Foundry.", "Bring the hammer to Torvald.", "The hammer hangs over the forge." },
      "Atk +1, every run.",
      "Leif: Torvald hung an old hammer over the forge. He bowed." },
};

}  // namespace

State GetState(const Legend& legend, Id id)
{
    return static_cast<State>((legend.contracts >> (2 * static_cast<int>(id))) & 3);
}

void SetState(Legend& legend, Id id, State state)
{
    const int shift = 2 * static_cast<int>(id);
    legend.contracts =
        static_cast<uint16_t>((legend.contracts & ~(3u << shift)) | (static_cast<unsigned>(state) << shift));
}

bool IsKept(const Legend& legend, Id id)
{
    return GetState(legend, id) == State::Kept;
}

const Info& GetInfo(Id id)
{
    return kInfo[static_cast<int>(id) < kContracts ? static_cast<int>(id) : 0];
}

int Of(Giver giver)
{
    for (int c = 0; c < kContracts; ++c)
    {
        if (kInfo[c].giver == giver)
        {
            return c;
        }
    }
    return -1;
}

}  // namespace Dungeon::Contracts
