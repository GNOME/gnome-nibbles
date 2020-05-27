/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Gnome Nibbles: Gnome Worm Game
 * Copyright (C) 2015 Iulian-Gabriel Radu <iulian.radu67@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// This is a fairly literal translation of the GPLv2+ original by
// Sean MacIsaac, Ian Peters, Guillaume Béland.

private enum BonusType
{
    REGULAR,
    HALF,
    DOUBLE,
    LIFE,
    REVERSE,
    WARP;
}

private class Bonus : Object
{
    public int x                { internal get; protected construct; }
    public int y                { internal get; protected construct; }
    public BonusType bonus_type { internal get; protected construct; }
    public bool fake            { internal get; protected construct; }
    public int countdown        { internal get; internal construct set; }

    internal Bonus (int x, int y, BonusType bonus_type, bool fake, int countdown)
    {
        Object (x: x, y: y, bonus_type: bonus_type, fake: fake, countdown: countdown);
    }
}

private class Boni : Object
{
    internal Gee.LinkedList<Bonus> bonuses = new Gee.LinkedList<Bonus> ();

    internal int numleft    { internal get; internal set; default = 8; }
    internal int numboni    { internal get; private set; default = 8; }
    private  int numbonuses = 0;

    private const int MAX_BONUSES = 100;

    internal signal void bonus_added ();
    internal signal void bonus_removed (Bonus bonus);

    internal void add_bonus (int[,] board, int x, int y, BonusType bonus_type, bool fake, int countdown)
    {
        if (numbonuses == MAX_BONUSES)
            return;

        var bonus = new Bonus (x, y, bonus_type, fake, countdown);
        bonuses.add (bonus);
        board[x    , y    ] = bonus_type + 'A';
        board[x + 1, y    ] = bonus_type + 'A';
        board[x    , y + 1] = bonus_type + 'A';
        board[x + 1, y + 1] = bonus_type + 'A';
        bonus_added ();
        numbonuses++;
    }

    internal void remove_bonus (int[,] board, Bonus bonus)
    {
        board[bonus.x    , bonus.y    ] = NibblesGame.EMPTYCHAR;
        board[bonus.x + 1, bonus.y    ] = NibblesGame.EMPTYCHAR;
        board[bonus.x    , bonus.y + 1] = NibblesGame.EMPTYCHAR;
        board[bonus.x + 1, bonus.y + 1] = NibblesGame.EMPTYCHAR;

        bonus_removed (bonus);
        bonuses.remove (bonus);
    }

    internal void reset (int numworms)
    {
        bonuses.clear ();
        reset_missed ();
        numboni = 8 + numworms;
        numbonuses = 0;
        numleft = numboni;
    }

    internal Bonus? get_bonus (int[,] board, int x, int y)
    {
        foreach (var bonus in bonuses)
        {
            if ((x == bonus.x     && y == bonus.y    )
             || (x == bonus.x + 1 && y == bonus.y    )
             || (x == bonus.x     && y == bonus.y + 1)
             || (x == bonus.x + 1 && y == bonus.y + 1))
            {
                return bonus;
            }
        }

        return null;
    }

    internal void on_worms_move (int[,] board, out uint8 missed_bonuses_to_replace)
    {
        missed_bonuses_to_replace = 0;

        // FIXME Use an iterator instead of a second list
        var found = new Gee.LinkedList<Bonus> ();
        foreach (var bonus in bonuses)
            if (bonus.countdown-- == 0)
                found.add (bonus);

        foreach (var bonus in found)
        {
            bool real_bonus = bonus.bonus_type == BonusType.REGULAR && !bonus.fake;

            remove_bonus (board, bonus);

            if (real_bonus)
            {
                increase_missed ();
                missed_bonuses_to_replace++;
            }
        }
    }

    /*\
    * * missed
    \*/

    private uint8 missed = 0;
    private const uint8 MAX_MISSED = 2;

    internal inline bool too_many_missed ()
    {
        return missed > MAX_MISSED;
    }

    private inline void increase_missed ()
    {
        missed++;
    }

    private inline void reset_missed ()
    {
        missed = 0;
    }
}
