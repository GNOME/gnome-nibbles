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
    internal const int GAMEDELAY = 35;

    internal const int MAX_WORMS = 6;

    internal const int MAX_SPEED = 4;

    internal const char EMPTYCHAR = 'a';
    internal const char WORMCHAR = 'w';     // only used in worm.vala
    internal const char WARPCHAR = 'W';     // only used in warp.vala

    internal const int MAX_LEVEL = 26;

    public int start_level      { internal get; protected construct; }
    public int current_level    { internal get; protected construct set; }
    public int speed            { internal get; internal construct set; }

    /* Board data */
    internal int[,] board;

    public int width            { internal get; protected construct; }
    public int height           { internal get; protected construct; }

    /* Worms data */
    internal int numhumans      { internal get; internal set; }
    internal int numai          { internal get; internal set; }
    internal int numworms       { internal get; private set; }

    /* Game models */
    public Gee.LinkedList<Worm> worms                   { internal get; default = new Gee.LinkedList<Worm> (); }
    public Gee.HashMap<Worm, WormProperties> worm_props { internal get; default = new Gee.HashMap<Worm, WormProperties> (); }

    private Boni boni = new Boni ();
    private WarpManager warp_manager = new WarpManager ();

    /* Game controls */
    internal bool is_running    { internal get; private set; default = false; }
    internal bool is_paused     { internal get; private set; default = false; }

    private uint main_id = 0;

    public bool fakes           { internal get; internal construct set; }

    internal signal void bonus_applied (Bonus bonus, Worm worm);
    internal signal void log_score (int score, int level_reached);
    internal signal void animate_end_game ();
    internal signal void level_completed ();
    internal signal void warp_added (int x, int y);
    internal signal void bonus_added (Bonus bonus);
    internal signal void bonus_removed (Bonus bonus);

    construct
    {
        board = new int [width, height];
        warp_manager.warp_added.connect ((warp) => warp_added (warp.x, warp.y));
        boni.bonus_removed.connect ((bonus) => bonus_removed (bonus));
    }

    internal NibblesGame (int start_level, int speed, bool fakes, int width, int height, bool no_random = false)
    {
        Object (start_level: start_level, current_level: start_level, speed: speed, fakes: fakes, width: width, height: height);

        Random.set_seed (no_random ? 42 : (uint32) time_t ());
    }

    internal bool load_board (string [] future_board)
    {
        if (future_board.length != height)
            return false;

        boni.reset (numworms);
        warp_manager.warps.clear ();

        string tmpboard;
        int count = 0;
        for (int i = 0; i < height; i++)
        {
            tmpboard = future_board [i];
            if (tmpboard.char_count () != width)
                return false;
            for (int j = 0; j < width; j++)
            {
                unichar char_value = tmpboard.get_char (tmpboard.index_of_nth_char (j));
                switch (char_value)
                {
                    // readable empty tile, but the game internals use an 'a'
                    case '.':
                        board[j, i] = (int) 'a';
                        break;

                    // readable walls, but the game internals use ASCII chars
                    case '┃':
                        board[j, i] = (int) 'b';
                        break;
                    case '━':
                        board[j, i] = (int) 'c';
                        break;
                    case '┗':
                        board[j, i] = (int) 'd';
                        break;
                    case '┛':
                        board[j, i] = (int) 'e';
                        break;
                    case '┏':
                        board[j, i] = (int) 'f';
                        break;
                    case '┓':
                        board[j, i] = (int) 'g';
                        break;
                    case '┻':
                        board[j, i] = (int) 'h';
                        break;
                    case '┣':
                        board[j, i] = (int) 'i';
                        break;
                    case '┫':
                        board[j, i] = (int) 'j';
                        break;
                    case '┳':
                        board[j, i] = (int) 'k';
                        break;
                    case '╋':
                        board[j, i] = (int) 'l';
                        break;

                    // start positions
                    case '▲':
                    case 'm':
                        board[j, i] = (int) NibblesGame.EMPTYCHAR;
                        if (count < numworms)
                        {
                            worms[count].set_start (j, i, WormDirection.UP);
                            count++;
                        }
                        break;
                    case '◀':
                    case 'n':
                        board[j, i] = (int) NibblesGame.EMPTYCHAR;
                        if (count < numworms)
                        {
                            worms[count].set_start (j, i, WormDirection.LEFT);
                            count++;
                        }
                        break;
                    case '▼':
                    case 'o':
                        board[j, i] = (int) NibblesGame.EMPTYCHAR;
                        if (count < numworms)
                        {
                            worms[count].set_start (j, i, WormDirection.DOWN);
                            count++;
                        }
                        break;
                    case '▶':
                    case 'p':
                        board[j, i] = (int) NibblesGame.EMPTYCHAR;
                        if (count < numworms)
                        {
                            worms[count].set_start (j, i, WormDirection.RIGHT);
                            count++;
                        }
                        break;

                    // warps
                    case 'Q':
                    case 'R':
                    case 'S':
                    case 'T':
                    case 'U':
                    case 'V':
                    case 'W':
                    case 'X':
                    case 'Y':
                    case 'Z':
                        board[j, i] = (int) char_value;
                        warp_manager.add_warp (board, j - 1, i - 1, -(board[j, i]), 0);
                        break;

                    case 'r':
                    case 's':
                    case 't':
                    case 'u':
                    case 'v':
                    case 'w':
                    case 'x':
                    case 'y':
                    case 'z':
                        warp_manager.add_warp (board, -(((int) char_value) - 'a' + 'A'), 0, j, i);
                        board[j, i] = (int) NibblesGame.EMPTYCHAR;
                        break;

                    // old walls, kept for compatibility
                    case 'a':
                    case 'b':
                    case 'c':
                    case 'd':
                    case 'e':
                    case 'f':
                    case 'g':
                    case 'h':
                    case 'i':
                    case 'j':
                    case 'k':
                    case 'l':
                        board[j, i] = (int) char_value;
                        break;

                    default:
                        return false;
                }
            }
        }
        return true;
    }

    /*\
    * * Game controls
    \*/

    private uint8 bonus_cycle = 0;
    internal void start (bool add_initial_bonus)
    {
        if (add_initial_bonus)
            add_bonus (true);

        is_running = true;

        main_id = Timeout.add (GAMEDELAY * speed, () => {
                bonus_cycle = (bonus_cycle + 1) % 3;
                if (bonus_cycle == 0)
                    add_bonus (false);
                return main_loop_cb ();
            });
        Source.set_name_by_id (main_id, "[Nibbles] main_loop_cb");
    }

    internal void stop ()
    {
        is_running = false;

        if (main_id == 0)
            return;
        Source.remove (main_id);
        main_id = 0;
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
        is_paused = false;
    }

    private void end ()
    {
        stop ();
        animate_end_game ();
    }

    private bool main_loop_cb ()
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
            var worm = new Worm (i, width, height);
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

    private void move_worms ()
    {
        if (boni.too_many_missed ())
        {
            foreach (var worm in worms)
            {
                if (worm.score > 0)
                    worm.score--;
            }
        }

        uint8 missed_bonuses_to_replace;
        boni.on_worms_move (board, out missed_bonuses_to_replace);
        for (uint8 i = 0; i < missed_bonuses_to_replace; i++)
            add_bonus (true);

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
            x = Random.int_range (0, width  - 1);
            y = Random.int_range (0, height - 1);

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
                _add_bonus (x, y, BonusType.REGULAR, true, 300);

            good = false;
            while (!good)
            {
                good = true;

                x = Random.int_range (0, width  - 1);
                y = Random.int_range (0, height - 1);
                if (board[x, y] != EMPTYCHAR)
                    good = false;
                if (board[x + 1, y] != EMPTYCHAR)
                    good = false;
                if (board[x, y + 1] != EMPTYCHAR)
                    good = false;
                if (board[x + 1, y + 1] != EMPTYCHAR)
                    good = false;
            }
            _add_bonus (x, y, BonusType.REGULAR, false, 300);
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
                    _add_bonus (x, y, BonusType.HALF, good, 200);
                    break;
                case 10:
                case 11:
                case 12:
                case 13:
                case 14:
                    _add_bonus (x, y, BonusType.DOUBLE, good, 150);
                    break;
                case 15:
                    _add_bonus (x, y, BonusType.LIFE, good, 100);
                    break;
                case 16:
                case 17:
                case 18:
                case 19:
                case 20:
                    if (numworms > 1)
                        _add_bonus (x, y, BonusType.REVERSE, good, 150);
                    break;
            }
        }
    }
    private inline void _add_bonus (int x, int y, BonusType bonus_type, bool fake, int countdown)
    {
        Bonus bonus = new Bonus (x, y, bonus_type, fake, countdown);
        if (boni.add_bonus (board, bonus))
            bonus_added (bonus);
    }

    private void apply_bonus (Bonus bonus, Worm worm)
    {
        if (bonus.fake)
        {
            worm.reverse (board);

            return;
        }

        switch (board[worm.head.x, worm.head.y] - 'A')
        {
            case BonusType.REGULAR:
                int nth_bonus = boni.new_regular_bonus_eaten ();
                worm.change += nth_bonus * Worm.GROW_FACTOR;
                worm.score  += nth_bonus * current_level;
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

    private void bonus_found_cb (Worm worm)
    {
        var bonus = boni.get_bonus (board, worm.head.x, worm.head.y);
        if (bonus == null)
            return;
        apply_bonus (bonus, worm);
        bonus_applied (bonus, worm);

        bool real_bonus = board[worm.head.x, worm.head.y] == BonusType.REGULAR + 'A'
                       && !bonus.fake;

        boni.remove_bonus (board, bonus);

        if (real_bonus && !boni.last_regular_bonus ())
            add_bonus (true);
    }

    private void warp_found_cb (Worm worm)
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

        if (boni.last_regular_bonus ())
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
            var properties = new WormProperties ();

            worm_settings[worm.id].bind_with_mapping ("color", properties, "color", SettingsBindFlags.DEFAULT,
                                                      (prop_value, variant) => { prop_value.set_int (get_color_num (variant.get_string ()));
                                                                                 return /* success */ true; },
                                                      (prop_value, variant_type) => { return new Variant.@string (get_color_string (prop_value.get_int ())); },
                                                      null, null);

            worm_settings[worm.id].bind ("key-up",      properties, "up",       SettingsBindFlags.DEFAULT);
            worm_settings[worm.id].bind ("key-down",    properties, "down",     SettingsBindFlags.DEFAULT);
            worm_settings[worm.id].bind ("key-left",    properties, "left",     SettingsBindFlags.DEFAULT);
            worm_settings[worm.id].bind ("key-right",   properties, "right",    SettingsBindFlags.DEFAULT);

            worm_props.@set (worm, properties);
        }
    }
    private static inline string get_color_string (int color)
    {
        switch (color)
        {
            case 0: return "red";
            case 1: return "green";
            case 2: return "blue";
            case 3: return "yellow";
            case 4: return "cyan";
            case 5: return "purple";
            default: assert_not_reached ();
        }
    }
    private static inline int get_color_num (string color)
    {
        switch (color)
        {
            case "red":     return 0;
            case "green":   return 1;
            case "blue":    return 2;
            case "yellow":  return 3;
            case "cyan":    return 4;
            case "purple":  return 5;
            default: assert_not_reached ();
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
