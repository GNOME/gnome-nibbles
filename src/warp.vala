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

private class WarpManager: Object
{
    private class Warp : Object
    {
        private bool init_finished = false;

        public int   id         { internal get; protected construct; }

        public uint8 source_x   { internal get; protected construct set; }
        public uint8 source_y   { internal get; protected construct set; }

        public uint8 target_x   { private get; protected construct set; }
        public uint8 target_y   { private get; protected construct set; }

        public bool  bidi       { internal get; protected construct set; }

        internal Warp.from_source (int id, uint8 source_x, uint8 source_y)
        {
            Object (id      : id,
                    source_x: source_x,
                    source_y: source_y,
                    bidi    : true);    // that is a "maybe for now," until init_finished is set
        }

        internal Warp.from_target (int id, uint8 target_x, uint8 target_y)
        {
            Object (id      : id,
                    target_x: target_x,
                    target_y: target_y,
                    bidi    : false);
        }

        internal void set_source (uint8 x, uint8 y)
            requires (init_finished == false)
        {
            if (bidi)   // set to "true" when created from source
            {
                target_x = x;
                target_y = y;
            }
            else
            {
                source_x = x;
                source_y = y;
            }
            init_finished = true;
        }

        internal void set_target (uint8 x, uint8 y)
            requires (init_finished == false)
            requires (bidi == true)     // set to "true" when created from source
        {
            target_x = x;
            target_y = y;
            bidi = false;
            init_finished = true;
        }

        internal bool get_target (uint8 x, uint8 y, bool horizontal, ref uint8 target_x, ref uint8 target_y)
            requires (init_finished == true)
        {
            if ((x != source_x && x != source_x + 1)
             || (y != source_y && y != source_y + 1))
                return false;

            if (!bidi)
            {
                target_x = this.target_x;
                target_y = this.target_y;
            }
            else if (horizontal)
            {
                if (x == source_x)
                    target_x = this.target_x + 2;
                else if (this.target_x == 0)
                    assert_not_reached ();
                else
                    target_x = this.target_x - 1;
                if (y == source_y)
                    target_y = this.target_y;
                else
                    target_y = this.target_y + 1;
            }
            else
            {
                if (x == source_x)
                    target_x = this.target_x;
                else
                    target_x = this.target_x + 1;
                if (y == source_y)
                    target_y = this.target_y + 2;
                else if (this.target_y == 0)
                    assert_not_reached ();
                else
                    target_y = this.target_y - 1;
            }
            return true;
        }
    }

    private const uint8 MAX_WARPS = 200;

    private Gee.LinkedList<Warp> warps = new Gee.LinkedList<Warp> ();

    internal void add_warp_source (int id, uint8 x, uint8 y)
    {
        foreach (Warp warp in warps)
        {
            if (warp.id != id)
                continue;

            warp.set_source (x, y);
            if (warp.bidi)
            {
                Warp bidi_warp = new Warp.from_source (id, x, y);
                bidi_warp.set_source (warp.source_x, warp.source_y);
                warps.add (bidi_warp);
            }
            return;
        }

        if (warps.size >= MAX_WARPS)
            return;

        warps.add (new Warp.from_source (id, x, y));
    }

    internal void add_warp_target (int id, uint8 x, uint8 y)
    {
        foreach (Warp warp in warps)
        {
            if (warp.id != id)
                continue;

            warp.set_target (x, y);
            return;
        }

        if (warps.size >= MAX_WARPS)
            return;

        warps.add (new Warp.from_target (id, x, y));
    }

    internal bool get_warp_target (uint8 x, uint8 y, bool horizontal, out uint8 target_x, out uint8 target_y)
    {
        target_x = 0;   // garbage
        target_y = 0;   // garbage

        foreach (Warp warp in warps)
            if (warp.get_target (x, y, horizontal, ref target_x, ref target_y))
                return true;

        return false;
    }

    internal void clear_warps ()
    {
        warps.clear ();
    }
}
