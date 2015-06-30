/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Gnome Nibbles: Gnome Worm Game
 * Copyright (C) 2015 Iulian-Gabriel Radu
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

public class NibblesGame : Object
{
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

    public signal void worm_moved (Worm worm);

    public Gee.HashMap<Worm, WormProperties?> worm_props;

    public NibblesGame (Settings settings)
    {
        walls = new int[WIDTH, HEIGHT];
        worms = new Gee.LinkedList<Worm> ();
        worm_props = new Gee.HashMap<Worm, WormProperties?> ();
        load_properties (settings);
    }

    public void start ()
    {
        add_worms ();
        var id = Timeout.add (game_speed * GAMEDELAY, main_loop_cb);
        Source.set_name_by_id (id, "[Nibbles] main_loop_cb");
    }

    public void add_worms ()
    {
        stderr.printf("[Debug] Loading worms\n");
        foreach (var worm in worms)
            worm.spawn (walls);
    }

    public void move_worms ()
    {
        foreach (var worm in worms)
        {
            if (worm.is_stopped)
                continue;

            foreach (var other_worm in worms)
                if (worm != other_worm
                    && worm.collides_with_head (other_worm.head ()))
                {
                    worm.die (walls);
                    other_worm.die (walls);
                    continue;
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
