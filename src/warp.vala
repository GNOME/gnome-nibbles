/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Gnome Nibbles: Gnome Worm Game
 *
 * Rewrite of the original by Sean MacIsaac, Ian Peters, Guillaume Béland
 * Copyright (C) 2015 Iulian-Gabriel Radu <iulian.radu67@gmail.com>
 * Copyright (C) 2020 Arnaud Bonatti <arnaud.bonatti@gmail.com>
 * Copyright (C) 2022-24 Ben Corby <bcorby@new-ms.com>
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
 * grep -ne ' $' *.vala
 *
 */

private class WarpManager : Object
{
    private class Warp : Object
    {
        private bool init_finished = false;

        public WarpManager manager { internal get; protected construct; }

        public int   id         { internal get; protected construct; }

        public uint8 source_x   { internal get; protected construct set; }
        public uint8 source_y   { internal get; protected construct set; }

        public uint8 target_x   { private get; protected construct set; }
        public uint8 target_y   { private get; protected construct set; }

        private bool _random;
        public bool  random     {
                                    internal get { return _random; }
                                    protected construct set
                                    {
                                        _random = value;
                                        if (value)
                                            init_finished = true;
                                    }
                                }

        public bool  bidi       { internal get; protected construct set; }

        internal Warp.from_source (WarpManager manager, int id, uint8 source_x, uint8 source_y, bool random = false)
        {
            Object (manager : manager,
                    id      : id,
                    source_x: source_x,
                    source_y: source_y,
                    random  : random,
                    bidi    : true);    // that is a "maybe for now," until init_finished is set
        }

        internal Warp.from_target (WarpManager manager, int id, uint8 target_x, uint8 target_y)
        {
            Object (manager : manager,
                    id      : id,
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

        internal bool get_target (uint8 x, uint8 y, WormDirection direction, int length, bool ai_worm,
                                  Gee.LinkedList<Worm> worms, ref uint8 target_x, ref uint8 target_y, out bool bonus)
            requires (init_finished == true)
        {
            bonus = false;

            if ((x != source_x && x != source_x + 1)
             || (y != source_y && y != source_y + 1))
                return false;

            if (random)
            {
                WormMap worm_map = new WormMap (worms, manager.board_max_x, manager.board_max_y);
                uint16[] P = manager.all_empty_board_positions;
                Position p = {0 ,0};
                int lowest_deadend = int.MAX;
                uint16 lowest_deadend_position = 0;
                int i, count, clear_count;
#if TEST_COMPILE
                /* the test are done assuming the worm is a player */
                clear_count = 12;
#else
                /* ai worm's don't need a long clear streatch to help them stay alive */
                clear_count = ai_worm?2:12;
#endif
                var array_length = P.length;
                for (;array_length > 0;)
                {
                    i = Random.int_range (0, array_length);
                    for (count = 0, p.x = P[i] >> 8, p.y = (uint8)P[i];
                        count < clear_count && manager.board[p.x, p.y] == NibblesGame.EMPTYCHAR && !worm_map.contain_position (p);
                        count++, p.move (direction, manager.board_max_x, manager.board_max_y));
                    if (count >= clear_count)
                    {
                        if (ai_worm)
                        {
                            target_x = P[i] >> 8;
                            target_y = (uint8)P[i];
                            break; /* exit for loop */
                        }
                        else /* human worm */
                        {
                            var deadend = Worm.ai_deadend_after (manager.board, worms, worm_map, {P[i] >> 8, (uint8)P[i]}, direction, length);
                            if (deadend <= 0)
                            {
                                target_x = P[i] >> 8;
                                target_y = (uint8)P[i];
                                break; /* exit for loop */
                            }
                            if (deadend < lowest_deadend)
                            {
                                lowest_deadend = deadend;
                                lowest_deadend_position = P[i];
                            }
                        }
                    }
                    /* remove P[i] from the array */
                    if (i < array_length - 1)
                        P.move (i + 1, i, array_length - (i + 1));
                    array_length--;
                }
                if (array_length <= 0)
                {
                    /* If we have searched the whole map and did not find a deadend
                     * of zero use the best position we found (lowest deadend).
                     */
                    if (lowest_deadend < int.MAX)
                    {
                        target_x = lowest_deadend_position >> 8;
                        target_y = (uint8)lowest_deadend_position;
                    }
                    else
                    {
                        return false;
                    }
                }
                bonus = true;
            }
            else if (!bidi)
            {
                target_x = this.target_x;
                target_y = this.target_y;
            }
            else if (direction == LEFT || direction == RIGHT)
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

    internal uint16[] all_empty_board_positions;
    internal unowned int[,] board;
    internal uint8 board_max_x;
    internal uint8 board_max_y;
    private const uint8 MAX_WARPS = 200;
    private Gee.LinkedList<Warp> warps = new Gee.LinkedList<Warp> ();

    internal void add_warp_source (int id, uint8 x, uint8 y, bool random = false)
    {
        foreach (Warp warp in warps)
        {
            if (warp.id != id)
                continue;

            warp.set_source (x, y);
            if (warp.bidi)
            {
                Warp bidi_warp = new Warp.from_source (this, id, x, y);
                bidi_warp.set_source (warp.source_x, warp.source_y);
                warps.add (bidi_warp);
            }
            return;
        }

        if (warps.size >= MAX_WARPS)
            return;

        warps.add (new Warp.from_source (this, id, x, y, random));
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

        warps.add (new Warp.from_target (this, id, x, y));
    }

    internal bool get_warp_target (uint8 x, uint8 y, WormDirection worm_direction, int worm_length, bool ai_worm, Gee.LinkedList<Worm> worms, out uint8 target_x, out uint8 target_y, out bool bonus)
    {
        target_x = 0;   // garbage
        target_y = 0;   // garbage

        bonus = false;
        foreach (Warp warp in warps)
            if (warp.get_target (x, y, worm_direction, worm_length, ai_worm, worms, ref target_x, ref target_y, out bonus))
                return true;

        return false;
    }

    internal void clear_warps ()
    {
        warps.clear ();
        all_empty_board_positions = {};
    }

    internal void initilise (int[,] board)
    {
        this.board = board;
        board_max_x = (uint8)board.length[0];
        board_max_y = (uint8)board.length[1];
        Position p = {0, 0};
        for (;p.y < board_max_y; p.y++)
        {
            for (p.x = 0; p.x < board_max_x; p.x++)
            {
                if (board[p.x, p.y] == NibblesGame.EMPTYCHAR)
                    all_empty_board_positions += ((uint16)p.x) << 8 | p.y;
            }
        }
    }
}
