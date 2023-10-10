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

delegate bool KeypressHandlerFunction (uint a, uint b, out bool remove_handler);

private enum GameStatus
{
    GAMEOVER,
    VICTORY,
    NEWROUND;
}

private class NibblesGame : Object
{

    internal const int MAX_WORMS = 6;

    internal const int MAX_SPEED = 4;

    internal const char EMPTYCHAR = 'a';
    internal const char WORMCHAR = 'w';     // only used in worm.vala
    internal const char WARPCHAR = 'W';     // only used in warp.vala

    internal const int MAX_LEVEL = 26;

    public bool skip_score      { internal get; protected construct set; }
    public int current_level    { internal get; protected construct set; }
    public bool three_dimensional_view { internal get; internal construct set; }
    public int speed            { internal get; internal construct set; }
    public int gamedelay        { internal get; protected construct; }

    /* Board data */
    internal int[,] board;

    public uint8 width          { internal get; protected construct; }
    public uint8 height         { internal get; protected construct; }

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
    private bool is_paused      { internal get; private set; default = false; }
    internal bool paused
    {
        get {return is_paused;}
        set
        {
            is_paused = value;
            if (value)
                stop ();
            else
                start (false /* add initial bonus */);
        }
    }

    private uint main_id = 0;

    public bool fakes           { internal get; internal construct set; }

    internal signal void bonus_applied (Bonus bonus, Worm worm);
    internal signal void log_score (int score, int level_reached);
    internal signal void animate_end_game ();
    internal signal void level_completed ();
    internal signal void warp_added (uint8 x, uint8 y);
    internal signal void bonus_added (Bonus bonus);
    internal signal void bonus_removed (Bonus bonus);

    /* nibbles-window */
    internal signal string get_pkgdatadir ();
    internal signal bool add_keypress_handler (KeypressHandlerFunction? keypress_handler);
#if !TEST_COMPILE
    internal bool added_keypress_handler = false;
#endif

    /* nibbles-view */
    internal signal void redraw (bool animate = false);

    /* connected to sound */
    internal signal void play_sound (string sound);

    construct
    {
        board = new int [width, height];
        boni.bonus_removed.connect ((bonus) => bonus_removed (bonus));
    }

    internal NibblesGame (int start_level, int speed, int gamedelay, bool fakes, bool three_dimensional_view, uint8 width, uint8 height, bool no_random = false)
    {
        Object (skip_score: (start_level != 1), current_level: start_level, speed: speed, gamedelay: gamedelay, fakes: fakes, three_dimensional_view: three_dimensional_view, width: width, height: height);

        Random.set_seed (no_random ? 42 : (uint32) time_t ());
    }

    /*\
    * * Level creation and loading
    \*/

#if !TEST_COMPILE
    internal void new_level (int level_id)
    {
        /* add the keypress handler if we haven't done so yet */
        if (!added_keypress_handler)
            added_keypress_handler = add_keypress_handler (keypress);
            
        string level_name = "level%03d.gnl".printf (level_id);
        string filename = GLib.Path.build_filename (get_pkgdatadir (), "levels", level_name, null);

        FileStream file;
        if ((file = FileStream.open (filename, "r")) == null)
            error ("Nibbles couldn't find pixmap file: %s", filename);

        string? line;
        string [] board = {};
        while ((line = file.read_line ()) != null)
            board += (!) line;
        if (!load_board (board, 8 + numworms))
            error ("Level file appears to be damaged: %s", filename);
    }
#endif
    internal bool load_board (string [] future_board, uint8 regular_bonus)
    {
        if (future_board.length != (int) height)
            return false;

        boni.reset (regular_bonus);
        warp_manager.clear_warps ();

        string tmpboard;
        int count = 0;
        for (uint8 i = 0; i < height; i++)
        {
            tmpboard = future_board [i];
            if (tmpboard.char_count () != (int) width)
                return false;
            for (uint8 j = 0; j < width; j++)
            {
                unichar char_value = tmpboard.get_char (tmpboard.index_of_nth_char (j));
                switch (char_value)
                {
                    // readable empty tile, but the game internals use an 'a'
                    case '.':
                    case '+':
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
                        if (j == 0 || i == 0)
                            return false;

                        board[j, i] = (int) char_value;
                        warp_manager.add_warp_source (board[j, i], j - 1, i - 1);

                        board[j - 1, i - 1] = NibblesGame.WARPCHAR;
                        board[j    , i - 1] = NibblesGame.WARPCHAR;
                        board[j - 1, i    ] = NibblesGame.WARPCHAR;
                        board[j    , i    ] = NibblesGame.WARPCHAR;

                        warp_added (j - 1, i - 1);
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
                        // do not use the up () method: it depends on the locale, and that could have some weird results ("i".up () is either I or İ, for example)
                        warp_manager.add_warp_target ((int) char_value - (int) 'a' + (int) 'A', j, i);
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

        main_id = Timeout.add (gamedelay * speed, () => {
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
#if !TEST_COMPILE
    internal inline void reset (int start_level)
    {
        skip_score = start_level != 1;
        current_level = start_level;
        is_paused = false;
    }
#endif
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

    internal void create_worms (Gee.ArrayList<Settings>? worm_settings = null)
    {
        worms.clear ();

        numworms = numai + numhumans;
        for (int i = 0; i < numworms; i++)
        {
            var worm = new Worm (i, width, height, get_other_worms, get_bonuses);
            worm.bonus_found.connect (bonus_found_cb);
            worm.is_human = (i < numhumans);
            worms.add (worm);
        }
        if (worm_settings != null)
            load_worm_properties (worm_settings);

    }

    internal void add_worms ()
    {
        foreach (var worm in worms)
            worm.spawn ();
    }

    private void move_worms ()
    {
        foreach (var worm in worms)
        {
            if (worm.rounds_to_stay_still == 1)
                worm.is_stopped = false;
            if (worm.rounds_to_stay_still > 0)
                --worm.rounds_to_stay_still;
        }

        if (boni.too_many_missed ())
        {
            foreach (var worm in worms)
            {
                if (worm.score > 0)
                    worm.score--;
            }
        }

        uint8 missed_bonuses_to_replace;
        boni.on_worms_move (out missed_bonuses_to_replace);
        for (uint8 i = 0; i < missed_bonuses_to_replace; i++)
            add_bonus (true);

        var dead_worms = new Gee.LinkedList<Worm> ();

        /* make AIs decide what they will do */
        foreach (var worm in worms)
        {
            if (worm.is_stopped
             || worm.list.is_empty)
                continue;

            if (!worm.is_human)
                worm.ai_move (board, worms);
        }

        /* kill worms which are hitting wall */
        foreach (var worm in worms)
        {
            if (worm.is_stopped
             || worm.list.is_empty)
                continue;

            Position position = worm.position_move ();
            uint8 target_x;
            uint8 target_y;
            if (warp_manager.get_warp_target (position.x, position.y,
                             /* horizontal */ worm.direction == WormDirection.LEFT || worm.direction == WormDirection.RIGHT,
                                              out target_x, out target_y))
                position = Position () { x = target_x, y = target_y };

            if (!worm.can_move_to (board, worms, position))
            {
                dead_worms.add (worm);
                play_sound ("crash");
            }
        }

        /* move worms */
        foreach (var worm in worms)
        {
            if (worm.is_stopped
             || worm.list.is_empty
             || worm in dead_worms)
                continue;

            worm.move_part_1 ();
            if (board[worm.head.x, worm.head.y] == NibblesGame.WARPCHAR)
            {
                uint8 target_x;
                uint8 target_y;
                if (!warp_manager.get_warp_target (worm.head.x, worm.head.y,
                                  /* horizontal */ worm.direction == WormDirection.LEFT || worm.direction == WormDirection.RIGHT,
                                                   out target_x, out target_y))
                    assert_not_reached ();

                worm.move_part_2 (Position () { x = target_x, y = target_y });
            }
            else
                worm.move_part_2 (null);

            /* kill worms on heads collision */
            foreach (var other_worm in worms)
            {
                if (worm != other_worm
                 && !other_worm.is_stopped
                 && !other_worm.list.is_empty
                 && worm.head.x == other_worm.head.x
                 && worm.head.y == other_worm.head.y)
                {
                    if (!dead_worms.contains (worm))
                        dead_worms.add (worm);
                    if (!dead_worms.contains (other_worm))
                        dead_worms.add (other_worm);
                }
            }
        }

        /* remove dead worms */
        foreach (var worm in dead_worms)
        {
            if (numworms > 1)
                worm.score = worm.score * 7 / 10;

            if (worm.lives > 0)
                worm.reset ();
        }

        /* refresh the screen */
        redraw (true);
    }

    private void reverse_worms (Worm worm)
    {
        foreach (var other_worm in worms)
            if (worm != other_worm)
                other_worm.reverse ();
    }

    /*\
    * * Handling bonuses
    \*/

    private bool is_space_empty (uint8 x, uint8 y, bool[,] worms_at)
    {
        return EMPTYCHAR == board [x, y] && EMPTYCHAR == board [x + 1, y + 1]
            && EMPTYCHAR == board [x + 1, y] && EMPTYCHAR == board [x, y + 1]
            && boni.get_bonus (x, y) == null && boni.get_bonus (x + 1, y + 1) == null
            && boni.get_bonus (x + 1, y) == null && boni.get_bonus (x, y + 1) == null
            && !worms_at [x, y] && !worms_at [x + 1, y + 1] 
            && !worms_at [x + 1, y] && !worms_at [x, y + 1];
    }

    private void add_bonus (bool regular)
    {
        bool good = false;
        uint8 x = 0;
        uint8 y = 0;
        bool[,] worms_at = new bool[width, height];

        if (!regular)
        {
            if (Random.int_range (0, 50) != 0)
                return;
        }

        foreach (Worm worm in worms)
            if (!worm.is_stopped)
                foreach (var p in worm.list)
                    worms_at[p>>8, (uint8)p] = true;

        do
        {
            x = (uint8) Random.int_range (0, width  - 1);
            y = (uint8) Random.int_range (0, height - 1);
        } while (!is_space_empty (x, y, worms_at));

        if (regular)
        {
            if ((Random.int_range (0, 7) == 0) && fakes)
                _add_bonus (x, y, BonusType.REGULAR, true, 300);

            do
            {
                x = (uint8) Random.int_range (0, width  - 1);
                y = (uint8) Random.int_range (0, height - 1);
            } while (!is_space_empty (x, y, worms_at));
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
    private inline void _add_bonus (uint8 x, uint8 y, BonusType bonus_type, bool fake, uint16 countdown)
    {
        Bonus bonus = new Bonus (x, y, bonus_type, fake, countdown);
        if (boni.add_bonus (bonus))
            if (bonus.bonus_type != BonusType.REGULAR)
                play_sound ("appear");
    }

    private void apply_bonus (Bonus bonus, Worm worm)
    {
        if (bonus.fake)
        {
            worm.reverse ();

            return;
        }

        switch (bonus.bonus_type)
        {
            case BonusType.REGULAR:
                uint8 nth_bonus = boni.new_regular_bonus_eaten ();
                worm.change += (int) nth_bonus * Worm.GROW_FACTOR;
                worm.score  += (int) nth_bonus * current_level;
                play_sound ("gobble");
                break;
            case BonusType.DOUBLE:
                worm.score += (worm.length + worm.change) * current_level;
                worm.change += worm.length + worm.change;
                play_sound ("bonus");
                break;
            case BonusType.HALF:
                if (worm.length + worm.change > 2)
                {
                    worm.score += ((worm.length + worm.change / 2) * current_level);
                    worm.reduce_tail (worm.length / 2);
                    worm.change -= (worm.length + worm.change) / 2;
                }
                play_sound ("bonus");
                break;
            case BonusType.LIFE:
                worm.add_life ();
                play_sound ("life");
                break;
            case BonusType.REVERSE:
                reverse_worms (worm);
                play_sound ("reverse");
                break;
            case BonusType.WARP:
                break;
        }
    }

    private void bonus_found_cb (Worm worm)
    {
        var bonus = boni.get_bonus (worm.head.x, worm.head.y);
        if (bonus == null)
            return;
        worm.add_bonus_eaten_position (worm.head.x, worm.head.y);
        apply_bonus (bonus, worm);
        bonus_applied (bonus, worm);

        bool real_bonus = bonus.bonus_type == BonusType.REGULAR && !bonus.fake;

        boni.remove_bonus (bonus);

        if (real_bonus && !boni.last_regular_bonus ())
            add_bonus (true);
    }

    internal GameStatus? get_game_status ()
    {
        var worms_left = 0;
        foreach (var worm in worms)
        {
            if (worm.lives > 0)
                worms_left += 1;
            else if (worm.is_human && worm.lives == 0)
                return GameStatus.GAMEOVER;
            else if (numhumans == 0 && worm.lives == 0)
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

            worm_settings[worm.id].bind ("key-up",        properties, "up",        SettingsBindFlags.DEFAULT);
            worm_settings[worm.id].bind ("key-down",      properties, "down",      SettingsBindFlags.DEFAULT);
            worm_settings[worm.id].bind ("key-left",      properties, "left",      SettingsBindFlags.DEFAULT);
            worm_settings[worm.id].bind ("key-right",     properties, "right",     SettingsBindFlags.DEFAULT);
            worm_settings[worm.id].bind ("key-up-raw",    properties, "raw-up",    SettingsBindFlags.DEFAULT);
            worm_settings[worm.id].bind ("key-down-raw",  properties, "raw-down",  SettingsBindFlags.DEFAULT);
            worm_settings[worm.id].bind ("key-left-raw",  properties, "raw-left",  SettingsBindFlags.DEFAULT);
            worm_settings[worm.id].bind ("key-right-raw", properties, "raw-right", SettingsBindFlags.DEFAULT);
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
#if !TEST_COMPILE
    internal bool keypress (uint keyval, uint keycode, out bool remove_handler)
    {
        remove_handler = false;
        return handle_keypress (keyval, keycode);
    }

    internal bool handle_keypress (uint keyval, uint keycode)
    {
        if (!is_running)
            return false;

        foreach (var worm in worms)
        {
            if (worm.is_human)
                if (worm.handle_keypress (keycode, worm_props, board, worms))
                    return true;
        }

        return false;
    }
#endif    
    /*\
    * * Delegates
    \*/

    private Gee.List<Worm> get_other_worms (Worm self)
    {
        var result = new Gee.ArrayList<Worm> ();
        foreach (Worm worm in worms)
        {
            if (worm != self)
                result.add (worm);
        }
        return result;
    }
    
    internal Gee.List<Bonus> get_bonuses ()
    {
        return boni.get_bonuses ();
    }
}

