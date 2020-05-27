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

private enum GameStatus
{
    GAMEOVER,
    VICTORY,
    NEWROUND;
}

private class NibblesGame : Object
{
    internal const int MINIMUM_TILE_SIZE = 7;

    internal const int GAMEDELAY = 35;
    private const int BONUSDELAY = 100;

    internal const int MAX_HUMANS = 4;
    internal const int MAX_AI = 5;
    internal const int MAX_WORMS = 6;

    internal const int MAX_SPEED = 4;

    internal const int WIDTH = 92;
    internal const int HEIGHT = 66;
    internal const int CAPACITY = WIDTH * HEIGHT;

    internal const char EMPTYCHAR = 'a';
    internal const char WORMCHAR = 'w';     // only used in worm.vala
    internal const char WARPCHAR = 'W';     // only used in warp.vala

    internal const int MAX_LEVEL = 26;

    public int start_level      { internal get; protected construct; }
    public int current_level    { internal get; protected construct set; }
    public int speed            { internal get; internal construct set; }

    /* Board data */
    public int tile_size        { internal get; internal construct set; }
    internal int[,] board = new int[WIDTH, HEIGHT];

    /* Worms data */
    internal int numhumans      { internal get; internal set; }
    internal int numai          { internal get; internal set; }
    internal int numworms       { internal get; private set; }

    /* Game models */
    public Gee.LinkedList<Worm> worms                       { internal get; default = new Gee.LinkedList<Worm> (); }
    public Boni boni                                        { internal get; default = new Boni (); }
    public WarpManager warp_manager                         { internal get; default = new WarpManager (); }
    public Gee.HashMap<Worm, WormProperties?> worm_props    { internal get; default = new Gee.HashMap<Worm, WormProperties?> (); }

    /* Game controls */
    internal bool is_running    { internal get; private set; default = false; }
    internal bool is_paused     { internal get; private set; default = false; }

    private uint main_id = 0;
    private uint add_bonus_id = 0;

    public bool fakes           { internal get; internal construct set; }

    internal signal void worm_moved (Worm worm);
    internal signal void bonus_applied (Bonus bonus, Worm worm);
    internal signal void log_score (int score, int level_reached);
    internal signal void animate_end_game ();
    internal signal void level_completed ();

    internal NibblesGame (int tile_size, int start_level, int speed, bool fakes)
    {
        Object (tile_size: tile_size, start_level: start_level, current_level: start_level, speed: speed, fakes: fakes);

        Random.set_seed ((uint32) time_t ());
    }

    /*\
    * * Game controls
    \*/

    internal void start (bool add_initial_bonus)
    {
        if (add_initial_bonus)
            add_bonus (true);

        is_running = true;

        main_id = Timeout.add (GAMEDELAY * speed, main_loop_cb);
        Source.set_name_by_id (main_id, "[Nibbles] main_loop_cb");

        add_bonus_id = Timeout.add (BONUSDELAY * speed, add_bonus_cb);
        Source.set_name_by_id (add_bonus_id, "[Nibbles] add_bonus_cb");
    }

    internal void stop ()
    {
        is_running = false;

        if (main_id != 0)
        {
            Source.remove (main_id);
            main_id = 0;
        }

        if (add_bonus_id != 0)
        {
            Source.remove (add_bonus_id);
            add_bonus_id = 0;
        }
    }

    internal void pause ()
    {
        is_paused = true;
        stop ();
    }

    internal void unpause ()
    {
        is_paused = false;
        start (/* add initial bonus */ false);
    }

    internal inline void reset ()
    {
        current_level = start_level;
    }

    private void end ()
    {
        stop ();
        animate_end_game ();
    }

    internal bool main_loop_cb ()
    {
        var status = get_game_status ();

        if (status == GameStatus.GAMEOVER)
        {
            end ();

            log_score (worms.first ().score, current_level);

            return Source.REMOVE;
        }
        else if (status == GameStatus.VICTORY)
        {
            end ();

            var winner = get_winner ();
            if (winner == null)
                return Source.REMOVE;

            log_score (winner.score, current_level);

            return Source.REMOVE;
        }
        else if (status == GameStatus.NEWROUND)
        {
            stop ();

            animate_end_game ();
            level_completed ();

            current_level++;

            if (current_level == MAX_LEVEL + 1)
                log_score (worms.first ().score, current_level);

            return Source.REMOVE;
        }
        move_worms ();

        return Source.CONTINUE;
    }

    /*\
    * * Handling worms
    \*/

    internal void create_worms ()
    {
        worms.clear ();

        numworms = numai + numhumans;
        for (int i = 0; i < numworms; i++)
        {
            var worm = new Worm (i);
            worm.bonus_found.connect (bonus_found_cb);
            worm.warp_found.connect (warp_found_cb);
            worm.is_human = (i < numhumans);
            worms.add (worm);
        }
    }

    internal void add_worms ()
    {
        foreach (var worm in worms)
        {
            /* Required for the first element of the worm added before signals were connected
             * TODO: Try to connect signals before adding the starting position to the worm
             */
            worm.added ();

            worm.spawn (board);
        }
    }

    internal void move_worms ()
    {
        if (boni.too_many_missed ())
        {
            foreach (var worm in worms)
            {
                if (worm.score > 0)
                    worm.score--;
            }
        }

        // FIXME 1/3: Use an iterator instead of a second list and remove
        // from the boni.bonuses list inside boni.remove_bonus ()
        var found = new Gee.LinkedList<Bonus> ();
        foreach (var bonus in boni.bonuses)
        {
            if (bonus.countdown-- == 0)
            {
                bool missed = bonus.bonus_type == BonusType.REGULAR && !bonus.fake;

                found.add (bonus);
                boni.remove_bonus (board, bonus);

                if (missed)
                {
                    boni.increase_missed ();
                    add_bonus (true);
                }
            }
        }
        boni.bonuses.remove_all (found);
        // END FIXME

        var dead_worms = new Gee.LinkedList<Worm> ();
        foreach (var worm in worms)
        {
            if (worm.is_stopped)
                continue;

            if (worm.list.is_empty)
                continue;

            if (!worm.is_human)
                worm.ai_move (board, numworms, worms);

            foreach (var other_worm in worms)
            {
                if (worm != other_worm
                    && !other_worm.is_stopped
                    && worm.will_collide_with_head (other_worm))
                    {
                        if (!dead_worms.contains (worm))
                            dead_worms.add (worm);
                        if (!dead_worms.contains (other_worm))
                            dead_worms.add (other_worm);
                        continue;
                    }
            }

            if (!worm.can_move_to (board, numworms))
            {
                dead_worms.add (worm);
                continue;
            }

            worm.move (board);
        }

        foreach (var worm in dead_worms)
        {
            if (numworms > 1)
                worm.score = worm.score * 7 / 10;

            if (worm.lives > 0)
                worm.reset (board);
        }
    }

    private void reverse_worms (Worm worm)
    {
        foreach (var other_worm in worms)
            if (worm != other_worm)
                other_worm.reverse (board);
    }

    /*\
    * * Handling bonuses
    \*/

    private void add_bonus (bool regular)
    {
        bool good = false;
        int x = 0, y = 0;

        if (!regular)
        {
            if (Random.int_range (0, 50) != 0)
                return;
        }

        do
        {
            good = true;
            x = Random.int_range (0, WIDTH - 1);
            y = Random.int_range (0, HEIGHT - 1);

            if (board[x, y] != EMPTYCHAR)
                good = false;
            if (board[x + 1, y] != EMPTYCHAR)
                good = false;
            if (board[x, y + 1] != EMPTYCHAR)
                good = false;
            if (board[x + 1, y + 1] != EMPTYCHAR)
                good = false;
        } while (!good);

        if (regular)
        {
            if ((Random.int_range (0, 7) == 0) && fakes)
                boni.add_bonus (board, x, y, BonusType.REGULAR, true, 300);

            good = false;
            while (!good)
            {
                good = true;

                x = Random.int_range (0, WIDTH - 1);
                y = Random.int_range (0, HEIGHT - 1);
                if (board[x, y] != EMPTYCHAR)
                    good = false;
                if (board[x + 1, y] != EMPTYCHAR)
                    good = false;
                if (board[x, y + 1] != EMPTYCHAR)
                    good = false;
                if (board[x + 1, y + 1] != EMPTYCHAR)
                    good = false;
            }
            boni.add_bonus (board, x, y, BonusType.REGULAR, false, 300);
        }
        else if (!boni.too_many_missed ())
        {
            if (Random.int_range (0, 7) != 0)
                good = false;
            else
                good = true;

            if (good && !fakes)
                return;

            switch (Random.int_range (0, 21))
            {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                case 9:
                    boni.add_bonus (board, x, y, BonusType.HALF, good, 200);
                    break;
                case 10:
                case 11:
                case 12:
                case 13:
                case 14:
                    boni.add_bonus (board, x, y, BonusType.DOUBLE, good, 150);
                    break;
                case 15:
                    boni.add_bonus (board, x, y, BonusType.LIFE, good, 100);
                    break;
                case 16:
                case 17:
                case 18:
                case 19:
                case 20:
                    if (numworms > 1)
                        boni.add_bonus (board, x, y, BonusType.REVERSE, good, 150);
                    break;
            }
        }
    }

    internal void apply_bonus (Bonus bonus, Worm worm)
    {
        if (bonus.fake)
        {
            worm.reverse (board);

            return;
        }

        switch (board[worm.head.x, worm.head.y] - 'A')
        {
            case BonusType.REGULAR:
                boni.numleft--;
                worm.change += (boni.numboni - boni.numleft) * Worm.GROW_FACTOR;
                worm.score += (boni.numboni - boni.numleft) * current_level;
                break;
            case BonusType.DOUBLE:
                worm.score += (worm.length + worm.change) * current_level;
                worm.change += worm.length + worm.change;
                break;
            case BonusType.HALF:
                if (worm.length + worm.change > 2)
                {
                    worm.score += ((worm.length + worm.change / 2) * current_level);
                    worm.reduce_tail (board, worm.length / 2);
                    worm.change -= (worm.length + worm.change) / 2;
                }
                break;
            case BonusType.LIFE:
                worm.add_life ();
                break;
            case BonusType.REVERSE:
                reverse_worms (worm);
                break;
        }
    }

    internal bool add_bonus_cb ()
    {
        add_bonus (false);

        return Source.CONTINUE;
    }

    internal void bonus_found_cb (Worm worm)
    {
        var bonus = boni.get_bonus (board, worm.head.x, worm.head.y);
        if (bonus == null)
            return;
        apply_bonus (bonus, worm);
        bonus_applied (bonus, worm);

        if (board[worm.head.x, worm.head.y] == BonusType.REGULAR + 'A'
            && !bonus.fake)
        {
            // FIXME: 2/3
            boni.remove_bonus (board, bonus);
            boni.bonuses.remove (bonus);

            if (boni.numleft != 0)
                add_bonus (true);
        }
        else
        {
            // FIXME: 3/3
            boni.remove_bonus (board, bonus);
            boni.bonuses.remove (bonus);
        }
    }

    internal void warp_found_cb (Worm worm)
    {
        var warp = warp_manager.get_warp (worm.head.x, worm.head.y);
        if (warp == null)
            return;

        worm.warp (warp);
    }

    internal GameStatus? get_game_status ()
    {
        var worms_left = 0;
        foreach (var worm in worms)
        {
            if (worm.lives > 0)
                worms_left += 1;
            else if (worm.is_human && worm.lives <= 0)
                return GameStatus.GAMEOVER;
        }

        if (worms_left == 1 && numworms > 1)
        {
            /* There were multiple worms but only one is still alive */
            return GameStatus.VICTORY;
        }
        else if (worms_left == 0)
        {
            /* There was only one worm and it died */
            return GameStatus.GAMEOVER;
        }

        if (boni.numleft == 0)
            return GameStatus.NEWROUND;

        return null;
    }

    internal Worm? get_winner ()
    {
        foreach (var worm in worms)
        {
            if (worm.lives > 0)
                return worm;
        }

        return null;
    }

    /*\
    * * Saving / Loading properties
    \*/

    internal void load_worm_properties (Gee.ArrayList<Settings> worm_settings)
    {
        worm_props.clear ();
        foreach (var worm in worms)
        {
            var properties = WormProperties ();
            properties.color = worm_settings[worm.id].get_enum ("color");
            properties.up = worm_settings[worm.id].get_int ("key-up");
            properties.down = worm_settings[worm.id].get_int ("key-down");
            properties.left = worm_settings[worm.id].get_int ("key-left");
            properties.right = worm_settings[worm.id].get_int ("key-right");

            worm_props.@set (worm, properties);
        }
    }

    internal bool handle_keypress (uint keyval)
    {
        if (!is_running)
            return false;

        foreach (var worm in worms)
        {
            if (worm.is_human)
            {
                if (worm.handle_keypress (keyval, worm_props))
                    return true;
            }
        }

        return false;
    }
}
