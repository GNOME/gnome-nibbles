/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Gnome Nibbles: Gnome Worm Game
 *
 * Rewrite of the original by Sean MacIsaac, Ian Peters, Guillaume Béland
 * Copyright (C) 2015 Iulian-Gabriel Radu <iulian.radu67@gmail.com>
 * Copyright (C) 2020 Arnaud Bonatti <arnaud.bonatti@gmail.com>
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

private class WarpManager: Object
{
    private class Warp : Object
    {
        private bool init_finished = false;

        public int id       { internal get; protected construct; }

        public int source_x { internal get; protected construct set; }
        public int source_y { internal get; protected construct set; }

        public int target_x { internal get; protected construct set; }
        public int target_y { internal get; protected construct set; }

        internal Warp.from_source (int id, int source_x, int source_y)
        {
            Object (id      : id,
                    source_x: source_x,
                    source_y: source_y);
        }

        internal Warp.from_target (int id, int target_x, int target_y)
        {
            Object (id      : id,
                    target_x: target_x,
                    target_y: target_y);
        }

        internal void set_source (int x, int y)
            requires (init_finished == false)
        {
            source_x = x;
            source_y = y;
            init_finished = true;
        }

        internal void set_target (int x, int y)
            requires (init_finished == false)
        {
            target_x = x;
            target_y = y;
            init_finished = true;
        }
    }

    private const int MAX_WARPS = 200;

    private Gee.LinkedList<Warp> warps = new Gee.LinkedList<Warp> ();

    internal void add_warp_source (int id, int x, int y)
    {
        foreach (var warp in warps)
        {
            if (warp.id == id)
            {
                warp.set_source (x, y);
                return;
            }
        }

        if (warps.size == MAX_WARPS)
            return;

        warps.add (new Warp.from_source (id, x, y));
    }

    internal void add_warp_target (int id, int x, int y)
    {
        foreach (var warp in warps)
        {
            if (warp.id == id)
            {
                warp.set_target (x, y);
                return;
            }
        }

        if (warps.size == MAX_WARPS)
            return;

        warps.add (new Warp.from_target (id, x, y));
    }

    internal bool get_warp_target (int x, int y, out int target_x, out int target_y)
    {
        foreach (var warp in warps)
        {
            if ((x == warp.source_x     && y == warp.source_y    )
             || (x == warp.source_x + 1 && y == warp.source_y    )
             || (x == warp.source_x     && y == warp.source_y + 1)
             || (x == warp.source_x + 1 && y == warp.source_y + 1))
            {
                target_x = warp.target_x;
                target_y = warp.target_y;
                return true;
            }
        }
        target_x = 0;   // garbage
        target_y = 0;   // garbage
        return false;
    }

    internal void clear_warps ()
    {
        warps.clear ();
    }
}
