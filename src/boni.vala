/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Gnome Nibbles: Gnome Worm Game
 * Copyright (C) 2015 Iulian-Gabriel Radu, Sean MacIsaac, Ian Peters,
 *                    Guillaume Béland
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

public class Boni : Object
{
    public Gee.ArrayList<Bonus> bonuses;
    public int missed;
    public int left;
    public int numbonuses;

    public const int MAX_BONUSES = 100;
    public const int MAX_MISSED = 2;

    public signal void bonus_added ();
    public signal void bonus_removed (Bonus bonus);

    public Boni (int numworms)
    {
        bonuses = new Gee.ArrayList<Bonus> ();
        missed = 0;
        numbonuses = 8 + numworms;
        left = numbonuses;
    }

    public void add_bonus (int[,] walls, int x, int y, BonusType type, bool fake, int countdown)
    {
        if (numbonuses == MAX_BONUSES)
            return;

        var bonus = new Bonus (x, y, type, fake, countdown);
        bonuses.add (bonus);
        walls[x, y] = type + 'A';
        walls[x + 1, y] = type + 'A';
        walls[x, y + 1] = type + 'A';
        walls[x + 1, y + 1] = type + 'A';
        bonus_added ();
        numbonuses++;

        //TODO
        // if (type != BonusType.REGULAR)
        //     play_sound ("appear");
    }

    public void remove_bonus (int[,] walls, Bonus bonus)
    {
        walls[bonus.x, bonus.y] = NibblesGame.EMPTYCHAR;
        walls[bonus.x + 1, bonus.y] = NibblesGame.EMPTYCHAR;
        walls[bonus.x, bonus.y + 1] = NibblesGame.EMPTYCHAR;
        walls[bonus.x + 1, bonus.y + 1] = NibblesGame.EMPTYCHAR;

        bonuses.remove (bonus);
        bonus_removed (bonus);
    }
}

public class Bonus : Object
{
    public int x;
    public int y;
    public BonusType type;
    public bool fake;
    public int countdown;

    public Bonus (int x, int y, BonusType type, bool fake, int countdown)
    {
        this.x = x;
        this.y = y;
        this.type = type;
        this.fake = fake;
        this.countdown = countdown;
    }
}

public enum BonusType
{
    REGULAR,
    HALF,
    DOUBLE,
    LIFE,
    REVERSE,
    CUT,
    SWITCH,
    WARP
}
