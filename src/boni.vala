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

/*
 * Coding style.
 *
 * To help you comply with the coding style in this project use the
 * following greps. Any lines returned should be adjusted so they
 * don't match. The convoluted regular expressions are so they don't
 * match them self.
 *
 * grep -ne '[^][)(_!$ "](' *.vala
 * grep -ne '[(] ' *.vala
 * grep -ne '[ ])' *.vala
 *
 */

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
    public uint8 x              { internal get; protected construct; }
    public uint8 y              { internal get; protected construct; }
    public BonusType bonus_type { internal get; protected construct; }
    public bool fake            { internal get; protected construct; }

    public uint16 countdown     { internal get; internal construct set; }

    internal Bonus (uint8 x, uint8 y, BonusType bonus_type, bool fake, uint16 countdown)
    {
        Object (x: x, y: y, bonus_type: bonus_type, fake: fake, countdown: countdown);
    }
}

private class Boni : Object
{
    private Gee.LinkedList<Bonus> bonuses = new Gee.LinkedList<Bonus> ();

    private uint8 regular_bonus_left = 0;
    private uint8 regular_bonus_maxi = 0;
    private uint8 total_bonus_number = 0;
    public int progress;

    private const uint8 MAX_BONUSES = 100;

    internal signal void bonus_removed (Bonus bonus);

    internal bool add_bonus (owned Bonus bonus)
    {
        if (progress != 2 && total_bonus_number >= MAX_BONUSES)
            return false;

        bonuses.add (bonus);
        total_bonus_number++;
        return true;
    }

    internal void remove_bonus (Bonus bonus)
    {
        bonus_removed (bonus);
        bonuses.remove (bonus);
    }

    internal void reset (uint8 regular_bonus)
    {
        bonuses.clear ();
        reset_missed ();
        regular_bonus_maxi = regular_bonus < MAX_BONUSES ? regular_bonus : MAX_BONUSES;
        regular_bonus_left = regular_bonus_maxi;
        total_bonus_number = 0;
    }

    internal Bonus? get_bonus (uint8 x, uint8 y)
    {
        foreach (Bonus bonus in bonuses)
        {
            if ((x == bonus.x + 0 && y == bonus.y + 0)
             || (x == bonus.x + 1 && y == bonus.y + 0)
             || (x == bonus.x + 0 && y == bonus.y + 1)
             || (x == bonus.x + 1 && y == bonus.y + 1))
            {
                return bonus;
            }
        }

        return null;
    }

    internal Gee.List<Bonus> get_bonuses ()
    {
        return bonuses;
    }

    internal void on_worms_move (out uint8 missed_bonuses_to_replace)
    {
        missed_bonuses_to_replace = 0;

	for (int i = bonuses.size; i > 0; i--)
	{
	    Bonus bonus = bonuses.get (i - 1);
	    if (bonus.countdown > 0)
		bonus.countdown--;
	    else
	    {
		remove_bonus (bonus);
		if (bonus.bonus_type == BonusType.REGULAR && !bonus.fake)
		{
		    increase_missed ();
		    missed_bonuses_to_replace++;
		}
	    }
	}
    }

    internal inline uint8 new_regular_bonus_eaten ()
    {
        reset_missed (); /* Without this reset all scores get set to
                            zero for the rest of the game
                            once too_many_missed () is true. */
        if (regular_bonus_left > 0)
            regular_bonus_left--;
        return regular_bonus_maxi - regular_bonus_left;
    }

    internal inline bool last_regular_bonus ()
    {
        return progress != 2 && regular_bonus_left == 0;
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
