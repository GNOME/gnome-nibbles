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

// This is a fairly literal translation of the LGPLv2+ original by
// Sean MacIsaac, Ian Peters, Guillaume Béland.

public class NibblesGame : Object
{
    private Boni _boni;
    public Boni boni
    {
        get { return _boni; }
        set
        {
            if (_boni != null)
                SignalHandler.disconnect_matched (_boni, SignalMatchType.DATA, 0, 0, null, null, this);

            _boni = value;
        }
    }

    public int tile_size;
    public int start_level;

    public const int MINIMUM_TILE_SIZE = 7;

    public const int GAMEDELAY = 35;

    public const int NUMWORMS = 2;

    public const int WIDTH = 92;
    public const int HEIGHT = 66;

    public const char EMPTYCHAR = 'a';
    public const char WORMCHAR = 'w';

    public int current_level;
    public int[,] walls;

    public Gee.LinkedList<Worm> worms;

    public int numworms = NUMWORMS;

    public int game_speed = 4;

    public bool fakes = false;

    public signal void worm_moved (Worm worm);

    public Gee.HashMap<Worm, WormProperties?> worm_props;

    public NibblesGame (Settings settings)
    {
        boni = new Boni (numworms);
        walls = new int[WIDTH, HEIGHT];
        worms = new Gee.LinkedList<Worm> ();
        worm_props = new Gee.HashMap<Worm, WormProperties?> ();

        Random.set_seed ((uint32) time_t ());
        load_properties (settings);
    }

    public void start ()
    {
        add_worms ();
        add_bonus (true);
        var id = Timeout.add (game_speed * GAMEDELAY, main_loop_cb);
        Source.set_name_by_id (id, "[Nibbles] main_loop_cb");
    }

    public void add_worms ()
    {
        stderr.printf("[Debug] Loading worms\n");
        foreach (var worm in worms)
            worm.spawn (walls);
    }

    public void add_bonus (bool regular)
    {
        bool good = false;
        int x = 0, y = 0;

        stderr.printf("[Debug] Adding bonus\n");
        if (!regular)
        {
            if (Random.int_range (0, 50) != 0)
                return;
        }

        stderr.printf("[Debug] Adding bonus2\n");
        do
        {
            good = true;
            x = Random.int_range (0, WIDTH - 1);
            y = Random.int_range (0, HEIGHT - 1);

            stderr.printf("[Debug] %d %d\n", x, y);
            if (walls[x, y] != EMPTYCHAR)
                good = false;
            if (walls[x + 1, y] != EMPTYCHAR)
                good = false;
            if (walls[x, y + 1] != EMPTYCHAR)
                good = false;
            if (walls[x + 1, y + 1] != EMPTYCHAR)
                good = false;
        } while (!good);

        stderr.printf("[Debug] Adding bonus3\n");
        if (regular)
        {
            if ((Random.int_range (0, 7) == 0) && fakes)
                boni.add_bonus (walls, x, y, BonusType.REGULAR, true, 300);

            good = false;
            while (!good)
            {
                good = true;

                x = Random.int_range (0, WIDTH - 1);
                y = Random.int_range (0, HEIGHT - 1);
                if (walls[x, y] != EMPTYCHAR)
                    good = false;
                if (walls[x + 1, y] != EMPTYCHAR)
                    good = false;
                if (walls[x, y + 1] != EMPTYCHAR)
                    good = false;
                if (walls[x + 1, y + 1] != EMPTYCHAR)
                    good = false;
            }
            stderr.printf("[Debug] Called add_bonus\n");
            boni.add_bonus (walls, x, y, BonusType.REGULAR, false, 300);
            stderr.printf("[Debug] Done add_bonus\n");
        }
        else if (boni.missed <= Boni.MAX_MISSED)
        {
            stderr.printf("[Debug] Else if\n");
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
                    boni.add_bonus (walls, x, y, BonusType.HALF, good, 200);
                    break;
                case 10:
                case 11:
                case 12:
                case 13:
                case 14:
                    boni.add_bonus (walls, x, y, BonusType.DOUBLE, good, 150);
                    break;
                case 15:
                    boni.add_bonus (walls, x, y, BonusType.LIFE, good, 100);
                    break;
                case 16:
                case 17:
                case 18:
                case 19:
                case 20:
                    if (numworms > 1)
                        boni.add_bonus (walls, x, y, BonusType.REVERSE, good, 150);
                    break;
            }
        }

        stderr.printf("[Debug] Finished adding bonus\n");
    }

    public void move_worms ()
    {
        foreach (var worm in worms)
        {
            if (worm.is_stopped)
                continue;

            foreach (var other_worm in worms)
            {
                if (worm.will_collide_with_head (other_worm)
                    && worm != other_worm)
                    {
                        stderr.printf("[Debug] Head collision\n");
                        worm.die (walls);
                        other_worm.die (walls);
                        continue;
                    }
            }

            if (!worm.can_move_to (walls, numworms))
            {
                worm.die (walls);
                continue;
            }

            worm.move (walls, true);
        }
    }

    public bool main_loop_cb ()
    {
        move_worms ();
        return Source.CONTINUE;
    }

    public void load_properties (Settings settings)
    {
        tile_size = settings.get_int ("tile-size");
        start_level = settings.get_int ("start-level");
    }

    public void save_properties (Settings settings)
    {
        settings.set_int ("tile-size", tile_size);
        settings.set_int ("start-level", start_level);
    }

    public void load_worm_properties (Gee.ArrayList<Settings> worm_settings)
    {
        foreach (var worm in worms)
        {
            var properties = WormProperties ();
            properties.color = NibblesView.colorval_from_name (worm_settings[worm.id].get_string ("color"));
            properties.up = worm_settings[worm.id].get_int ("key-up");
            properties.down = worm_settings[worm.id].get_int ("key-down");
            properties.left = worm_settings[worm.id].get_int ("key-left");
            properties.right = worm_settings[worm.id].get_int ("key-right");

            worm_props.set (worm, properties);
        }
    }

    public void save_worm_properties (Gee.ArrayList<Settings> worm_settings)
    {
        foreach (var worm in worms)
        {
            var properties = worm_props.get (worm);

            worm_settings[worm.id].set_string ("color", NibblesView.colorval_name (properties.color));
            worm_settings[worm.id].set_int ("key-up", (int) properties.up);
            worm_settings[worm.id].set_int ("key-down", (int) properties.down);
            worm_settings[worm.id].set_int ("key-left", (int) properties.left);
            worm_settings[worm.id].set_int ("key-right", (int) properties.right);
        }
    }

    public bool handle_keypress (uint keyval)
    {
        foreach (var worm in worms)
        {
            if (worm.human)
            {
                if (worm.handle_keypress (keyval, worm_props))
                    return true;
            }
        }

        return false;
    }
}
