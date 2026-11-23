#pragma once

// Walking the hero about in tests: settle a glide, face a way, walk to a
// tile round whatever is in the way, find a tile, stand beside a thing.

#include "../src/world/CrawlerGame.h"

namespace Dungeon::TestWalk
{

constexpr int kDx[4] = { 1, 0, -1, 0 };
constexpr int kDy[4] = { 0, 1, 0, -1 };

inline void Settle(CrawlerGame& game)
{
    for (int i = 0; i < 100 && game.IsGliding(); ++i)
    {
        game.Tick(16);
    }
}

/// Lets go of someone speaking, or of what a deed did: the key that takes.
inline void Dismiss(CrawlerGame& game)
{
    if (game.GetMenu() == Menu::Talk || game.GetMenu() == Menu::Result)
    {
        game.Act(Action::MenuClose);
    }
}

/// Closes a page, echo, book or note picked up on the way (any key does),
/// and a result or a line of talk.
inline void CloseReading(CrawlerGame& game)
{
    const Menu menu = game.GetMenu();
    if (menu == Menu::Page || menu == Menu::Echo || menu == Menu::Book)
    {
        game.Act(Action::Wait);
    }
    Dismiss(game);
}

/// Uses the thing ahead and takes the choice that does `deed`. False (and
/// its menu shut again) when it offers none such.
inline bool Choose(CrawlerGame& game, Deed deed)
{
    game.Act(Action::Interact);
    if (game.GetMenu() != Menu::Thing)
    {
        return false;
    }
    for (int i = 0; i < game.GetMenuCount(); ++i)
    {
        if (game.GetMenuEntry(i).deed == deed)
        {
            for (int k = 0; k < i; ++k)
            {
                game.Act(Action::MenuDown);
            }
            game.Act(Action::MenuSelect);
            Dismiss(game);  // what it did was shown: on
            return true;
        }
    }
    game.Act(Action::MenuClose);
    return false;
}

inline void Face(CrawlerGame& game, int way)
{
    CloseReading(game);
    for (int i = 0; i < 4 && game.GetFacing() != way; ++i)
    {
        game.Act(Action::TurnRight);
        Settle(game);
    }
}

inline bool Blocked(const CrawlerGame& game, Cell c)
{
    return !game.GetMap().IsWalkable(c) || game.FeatureAt(c) >= 0 || game.PropAt(c) >= 0;
}

/// Walks the hero onto `goal` (a free floor tile), fighting whatever stands
/// in the way, never onto `avoid` (the village's well, say). False if it
/// cannot.
inline bool WalkTo(CrawlerGame& game, Cell goal, Cell avoid = { -1, -1 })
{
    for (int steps = 0; steps < 400 && game.GetHeroCell() != goal; ++steps)
    {
        CloseReading(game);
        if (game.IsDead() || game.GetMenu() != Menu::None)
        {
            return false;
        }
        // Breadth first from the goal, so each tile knows its way there.
        int distance[kMapHeight][kMapWidth];
        for (auto& row : distance)
        {
            for (int& d : row)
            {
                d = -1;
            }
        }
        Cell queue[kMapWidth * kMapHeight];
        int head = 0;
        int tail = 0;
        distance[goal.y][goal.x] = 0;
        queue[tail++] = goal;
        while (head < tail)
        {
            const Cell c = queue[head++];
            for (int f = 0; f < 4; ++f)
            {
                const Cell n{ c.x + kDx[f], c.y + kDy[f] };
                if (n.x < 0 || n.y < 0 || n.x >= kMapWidth || n.y >= kMapHeight || distance[n.y][n.x] >= 0 ||
                    Blocked(game, n) || n == avoid)
                {
                    continue;
                }
                distance[n.y][n.x] = distance[c.y][c.x] + 1;
                queue[tail++] = n;
            }
        }
        const Cell hero = game.GetHeroCell();
        if (distance[hero.y][hero.x] < 0)
        {
            return false;
        }
        int way = -1;
        for (int f = 0; f < 4; ++f)
        {
            const Cell n{ hero.x + kDx[f], hero.y + kDy[f] };
            if (n.x >= 0 && n.y >= 0 && n.x < kMapWidth && n.y < kMapHeight && distance[n.y][n.x] >= 0 &&
                distance[n.y][n.x] < distance[hero.y][hero.x])
            {
                way = f;
            }
        }
        if (way < 0)
        {
            return false;
        }
        Face(game, way);
        game.Act(Action::Forward);
        Settle(game);
    }
    CloseReading(game);
    return game.GetHeroCell() == goal;
}

/// A free floor tile next to `c`, and the way from it to `c`.
inline bool StandBeside(const CrawlerGame& game, Cell c, Cell& stand, int& way)
{
    for (int f = 0; f < 4; ++f)
    {
        const Cell n{ c.x - kDx[f], c.y - kDy[f] };
        if (!Blocked(game, n) && game.MonsterAt(n) < 0)
        {
            stand = n;
            way = f;
            return true;
        }
    }
    return false;
}

inline bool Find(const CrawlerGame& game, Tile tile, Cell& at)
{
    for (int y = 0; y < kMapHeight; ++y)
    {
        for (int x = 0; x < kMapWidth; ++x)
        {
            if (game.GetMap().At(x, y) == tile)
            {
                at = { x, y };
                return true;
            }
        }
    }
    return false;
}

}  // namespace Dungeon::TestWalk
