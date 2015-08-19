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

public class Warp : Object
{
    public int x;
    public int y;

    public int wx;
    public int wy;

    public Warp (int x, int y, int wx, int wy)
    {
        this.x = x;
        this.y = y;

        this.wx = wx;
        this.wy = wy;
    }
}

public class WarpManager: Object
{
    private const int MAX_WARPS = 200;

    public Gee.LinkedList<Warp> warps;

    public signal void warp_added (Warp warp);

    public WarpManager ()
    {
        warps = new Gee.LinkedList<Warp> ();
    }

    public void add_warp (int[,] walls, int x, int y, int wx, int wy)
    {
        bool add = true;

        if (x < 0)
        {
            foreach (var warp in warps)
            {
                if (warp.wx == x)
                {
                    warp.wx = wx;
                    warp.wy = wy;
                    return;
                }
            }

            if (warps.size == MAX_WARPS)
                return;

            warps.add (new Warp (x, y, wx, wy));
        }
        else
        {
            foreach (var warp in warps)
            {
                if (warp.x == wx)
                {
                    warp.x = x;
                    warp.y = y;
                    add = false;

                    warp_added (warp);
                }
            }

            if (add)
            {
                if (warps.size == MAX_WARPS)
                    return;

                var warp = new Warp (x, y, wx, wy);
                warps.add (warp);

                warp_added (warp);
            }

            walls[x, y] = NibblesGame.WARPCHAR;
            walls[x + 1, y] = NibblesGame.WARPCHAR;
            walls[x, y + 1] = NibblesGame.WARPCHAR;
            walls[x + 1, y + 1] = NibblesGame.WARPCHAR;
        }
    }

    public Warp? get_warp (int x, int y)
    {
        foreach (var warp in warps)
        {
            if ((x == warp.x && y == warp.y)
             || (x == warp.x + 1 && y == warp.y)
             || (x == warp.x && y == warp.y + 1)
             || (x == warp.x + 1 && y == warp.y + 1))
                return warp;
        }

        return null;
    }
}
