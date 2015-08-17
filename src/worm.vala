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

public class Worm : Object
{
    public const int STARTING_LENGTH = 5;
    private const int STARTING_LIVES = 6;
    public const int GROW_FACTOR = 4;

    public Position starting_position { get; private set; }

    public int id { get; private set; }

    public bool is_human;
    public bool keypress = false;
    public bool is_stopped = false;

    public int lives { get; set; }
    public int change;
    public int score { get; set; }

    public int length
    {
        get { return list.size; }
        set {}
    }

    public Position head
    {
        get
        {
            Position head = list.first ();
            return head;
        }
        set {}
    }

    public WormDirection direction;

    public WormDirection starting_direction;

    private Gee.ArrayQueue<WormDirection> key_queue;

    public Gee.LinkedList<Position?> list { get; private set; }

    public signal void added ();
    public signal void moved ();
    public signal void rescaled (int tile_size);
    public signal void died ();
    public signal void tail_reduced (int erase_size);
    public signal void reversed ();

    public signal void bonus_found ();

    public Worm (int id)
    {
        this.id = id;
        lives = STARTING_LIVES;
        score = 0;
        change = 0;
        list = new Gee.LinkedList<Position?> ();
        key_queue = new Gee.ArrayQueue<WormDirection> ();
    }

    public void set_start (int xhead, int yhead, WormDirection direction)
    {
        list.clear ();

        starting_position = Position () {
            x = xhead,
            y = yhead
        };

        list.add (starting_position);

        starting_direction = direction;
        this.direction = starting_direction;
        change = 0;
        key_queue.clear ();
    }

    public void move (int[,] walls)
    {
        if (is_human)
            keypress = false;

        var position = head;
        switch (direction)
        {
            case WormDirection.UP:
                position.y = --head.y;
                if (position.y < 0)
                    position.y = NibblesGame.HEIGHT - 1;
                break;
            case WormDirection.DOWN:
                position.y = ++head.y;
                if (position.y >= NibblesGame.HEIGHT)
                    position.y = 0;
                break;
            case WormDirection.LEFT:
                position.x = --head.x;
                if (position.x < 0)
                    position.x = NibblesGame.WIDTH - 1;
                break;
            case WormDirection.RIGHT:
                position.x = ++head.x;
                if (position.x >= NibblesGame.WIDTH)
                    position.x = 0;
                break;
            default:
                break;
        }

        /* Add a new body piece */
        list.offer_head (position);

        if (change > 0)
        {
            change--;
            added ();
        }
        else
        {
            walls[list.last ().x, list.last ().y] = NibblesGame.EMPTYCHAR;
            list.poll_tail ();
            moved ();
        }

        /* Check for bonus before changing tile */
        if (walls[head.x, head.y] != NibblesGame.EMPTYCHAR)
            bonus_found ();

        /* Mark the tile as occupied by the worm's body */
        walls[head.x, head.y] = NibblesGame.WORMCHAR + id;

        if (!key_queue.is_empty)
            dequeue_keypress ();
    }

    public void reduce_tail (int[,] walls, int erase_size)
    {
        if (erase_size > 0)
        {
            if (length <= erase_size)
            {
                reset (walls);
            }

            for (int i = 0; i < erase_size; i++)
            {
                walls[list.last ().x, list.last ().y] = NibblesGame.EMPTYCHAR;
                list.poll_tail ();
            }
            tail_reduced (erase_size);
        }
    }

    public void reverse (int[,] walls)
    {
        var reversed_list = new Gee.LinkedList<Position?> ();
        foreach (var pos in list)
            reversed_list.offer_head (pos);

        reversed ();
        list = reversed_list;

        /* Set new direction as the opposite direction of the last two tail pieces */
        if (list[0].y == list[1].y)
            direction = (list[0].x > list[1].x) ? WormDirection.RIGHT : WormDirection.LEFT;
        else
            direction = (list[0].y > list[1].y) ? WormDirection.DOWN : WormDirection.UP;
    }

    public bool can_move_to (int[,] walls, int numworms)
    {
        var position = position_move ();

        if (walls[position.x, position.y] > NibblesGame.EMPTYCHAR
            && walls[position.x, position.y] < 'z' + numworms)
        {
            return false;
        }

        return true;
    }

    public bool will_collide_with_head (Worm other_worm)
    {
        var worm_pos = position_move ();
        var other_worm_pos = other_worm.position_move ();

        if (worm_pos == other_worm_pos)
            return true;

        return false;
    }

    public void spawn (int[,] walls)
    {
        change = STARTING_LENGTH - 1;
        for (int i = 0; i < STARTING_LENGTH; i++)
            move (walls);
    }

    public void lose_life ()
    {
        lives--;
    }

    public void reset (int[,] walls)
    {
        is_stopped = true;
        lose_life ();

        died ();
        foreach (var pos in list)
            walls[pos.x, pos.y] = NibblesGame.EMPTYCHAR;

        list.clear ();
        list.add (starting_position);
        added ();

        direction = starting_direction;
        change = 0;
        spawn (walls);

        key_queue.clear ();

        is_stopped = false;
    }

    private Position position_move ()
    {
        Position position = head;

        switch (direction)
        {
            case WormDirection.UP:
                position.y = --head.y;
                if (position.y < 0)
                    position.y = NibblesGame.HEIGHT - 1;
                break;
            case WormDirection.DOWN:
                position.y = ++head.y;
                if (position.y >= NibblesGame.HEIGHT)
                    position.y = 0;
                break;
            case WormDirection.LEFT:
                position.x = --head.x;
                if (position.x < 0)
                    position.x = NibblesGame.WIDTH - 1;
                break;
            case WormDirection.RIGHT:
                position.x = ++head.x;
                if (position.x >= NibblesGame.WIDTH)
                    position.x = 0;
                break;
            default:
                break;
        }

        return position;
    }

    private void direction_set (WormDirection dir)
    {
        if (!is_human)
            return;

        if (dir > 4)
            dir = (WormDirection) 1;
        if (dir < 1)
            dir = (WormDirection) 4;

        if (keypress)
        {
            queue_keypress (dir);
            return;
        }

        direction = (WormDirection) dir;
        keypress = true;
    }

    /*\
    * * Keys and key presses
    \*/
    private uint upper_key (uint keyval)
    {
        if (keyval > 255)
            return keyval;
        return ((char) keyval).toupper ();
    }

    public void handle_direction (WormDirection dir)
    {
        direction_set (dir);
    }

    public bool handle_keypress (uint keyval, Gee.HashMap<Worm, WormProperties?> worm_props)
    {
        WormProperties properties;
        uint propsUp, propsDown, propsLeft, propsRight, keyvalUpper;

        if (lives <= 0)
            return false;

        properties = worm_props.get (this);
        propsUp = upper_key (properties.up);
        propsLeft = upper_key (properties.left);
        propsDown = upper_key (properties.down);
        propsRight = upper_key (properties.right);
        keyvalUpper = upper_key (keyval);

        if ((keyvalUpper == propsUp) && (direction != WormDirection.DOWN))
        {
            handle_direction (WormDirection.UP);
            return true;
        }
        if ((keyvalUpper == propsDown) && (direction != WormDirection.UP))
        {
            handle_direction (WormDirection.DOWN);
            return true;
        }
        if ((keyvalUpper == propsRight) && (direction != WormDirection.LEFT))
        {
            handle_direction (WormDirection.RIGHT);
            return true;
        }
        if ((keyvalUpper == propsLeft) && (direction != WormDirection.RIGHT))
        {
            handle_direction (WormDirection.LEFT);
            return true;
        }

        return false;
    }

    public void queue_keypress (WormDirection dir)
    {
        /* Ignore duplicates in normal movement mode. This resolves the key
         * repeat issue
         */
        if (!key_queue.is_empty && dir == key_queue.peek ())
            return;

        key_queue.add (dir);
    }

    public void dequeue_keypress ()
                requires (!key_queue.is_empty)
    {
        direction_set (key_queue.poll ());
    }

    /*\
    * * AI
    \*/

    /* Check whether the worm will be trapped in a dead end. A location
     * within the dead end and the length of the worm is given. This
     * prevents worms getting trapped in a spiral, or in a corner sharper
     * than 90 degrees.  runnumber is a unique number used to update the
     * deadend board. The principle of the deadend board is that it marks
     * all squares previously checked, so the exact size of the deadend
     * can be calculated in O(n) time; to prevent the need to clear it
     * afterwards, a different number is stored in the board each time
     * (the number will not have been previously used, so the board will
     * appear empty). Although in theory deadend_runnumber may wrap round,
     * after 4 billion steps the entire board is likely to have been
     * overwritten anyway.
     */
    static uint[,] deadend_board = new uint[NibblesGame.WIDTH, NibblesGame.HEIGHT];
    static uint deadend_runnumber = 0;

    static int ai_deadend (int[,] walls, int numworms, int x, int y, int length_left)
    {
        int cdir, cx, cy;

        if (x >= NibblesGame.WIDTH)
            x = 0;
        if (x < 0)
            x = NibblesGame.WIDTH - 1;
        if (y >= NibblesGame.HEIGHT)
            y = 0;
        if (y < 0)
            y = NibblesGame.HEIGHT - 1;

        if (length_left <= 0)
            return 0;

        cdir = 5;
        while (--cdir > 0)
        {
            cx = x;
            cy = y;
            switch (cdir)
            {
                case WormDirection.UP:
                    cy -= 1;
                    break;
                case WormDirection.DOWN:
                    cy += 1;
                    break;
                case WormDirection.LEFT:
                    cx -= 1;
                    break;
                case WormDirection.RIGHT:
                    cx += 1;
                    break;
            }

            if (cx >= NibblesGame.WIDTH)
                cx = 0;
            if (cx < 0)
                cx = NibblesGame.WIDTH - 1;
            if (cy >= NibblesGame.HEIGHT)
                cy = 0;
            if (cy < 0)
                cy = NibblesGame.HEIGHT - 1;

            if ((walls[cx, cy] <= NibblesGame.EMPTYCHAR
                || walls[x, y] >= 'z' + numworms)
                && deadend_board[cx, cy] != deadend_runnumber)
            {
                deadend_board[cx, cy] = deadend_runnumber;
                length_left = ai_deadend (walls, numworms, cx, cy, length_left - 1);
                if (length_left <= 0)
                    return 0;
            }
        }

        return length_left;
    }

    /* Check a deadend starting from the next square in this direction,
     * rather than from this square. Also block off the squares near worm
     * heads, so that humans can't kill AI players by trapping them
     * against a wall.  The given length is quartered and squared; this
     * allows for the situation where the worm has gone round in a square
     * and is about to get trapped in a spiral. However, it's set to at
     * least BOARDWIDTH, so that on the levels with long thin paths a worm
     * won't start down the path if it'll crash at the other end.
     */
    private static int ai_deadend_after (int[,] walls, Gee.LinkedList<Worm> worms, int numworms, int x, int y, int dir, int length)
    {
        int cx, cy, cl, i;

        if (x < 0 || x >= NibblesGame.WIDTH || y < 0 || y >= NibblesGame.HEIGHT)
            return 0;

        ++deadend_runnumber;

        if (dir > 4)
            dir = 1;
        if (dir < 1)
            dir = 4;

        i = numworms;
        while (i-- > 0)
        {
            cx = worms[i].head.x;
            cy = worms[i].head.y;
            if (cx != x || cy != y) {
                if (cx > 0)
                    deadend_board[cx-1, cy] = deadend_runnumber;
                if (cy > 0)
                    deadend_board[cx, cy-1] = deadend_runnumber;
                if (cx < NibblesGame.WIDTH-1)
                    deadend_board[cx+1, cy] = deadend_runnumber;
                if (cy < NibblesGame.HEIGHT-1)
                    deadend_board[cx, cy+1] = deadend_runnumber;
            }
        }

        cx = x;
        cy = y;
        switch (dir)
        {
            case WormDirection.UP:
                cy -= 1;
                break;
            case WormDirection.DOWN:
                cy += 1;
                break;
            case WormDirection.LEFT:
                cx -= 1;
                break;
            case WormDirection.RIGHT:
                cx += 1;
                break;
        }

        if (cx >= NibblesGame.WIDTH)
            cx = 0;
        if (cx < 0)
            cx = NibblesGame.WIDTH - 1;
        if (cy >= NibblesGame.HEIGHT)
            cy = 0;
        if (cy < 0)
            cy = NibblesGame.HEIGHT - 1;

        deadend_board[x, y] = deadend_runnumber;
        deadend_board[cx, cy] = deadend_runnumber;

        cl = (length * length) / 16;
        if (cl < NibblesGame.WIDTH)
            cl = NibblesGame.WIDTH;
        return Worm.ai_deadend (walls, numworms, cx, cy, cl);
    }

    /* Check to see if another worm's head is too close in front of us;
     * that is, that it's within 3 in the direction we're going and within
     * 1 to the side.
     */
    private bool ai_too_close (Gee.LinkedList<Worm> worms, int numworms)
    {
        int i = numworms;
        int dx, dy;

        while (i-- > 0)
        {
            dx = head.x - worms[i].head.x;
            dy = head.y - worms[i].head.y;
            switch (direction)
            {
                case WormDirection.UP:
                    if (dy > 0 && dy <= 3 && dx >= -1 && dx <= 1)
                        return true;
                    break;
                case WormDirection.DOWN:
                    if (dy < 0 && dy >= -3 && dx >= -1 && dx <= 1)
                        return true;
                    break;
                case WormDirection.LEFT:
                    if (dx > 0 && dx <= 3 && dy >= -1 && dy <= 1)
                        return true;
                    break;
                case WormDirection.RIGHT:
                    if (dx < 0 && dx >= -3 && dy >= -1 && dy <= 1)
                        return true;
                    break;
            }
        }

        return false;
    }

    private static bool ai_wander (int[,] walls, int numworms, int x, int y, int dir, int ox, int oy)
    {
        if (dir > 4)
            dir = 1;
        if (dir < 1)
            dir = 4;

        switch (dir)
        {
            case WormDirection.UP:
                y -= 1;
                break;
            case WormDirection.DOWN:
                y += 1;
                break;
            case WormDirection.LEFT:
                x -= 1;
                break;
            case WormDirection.RIGHT:
                x += 1;
                break;
        }

        if (x >= NibblesGame.WIDTH)
            x = 0;
        if (x < 0)
            x = NibblesGame.WIDTH - 1;
        if (y >= NibblesGame.HEIGHT)
            y = 0;
        if (y < 0)
            y = NibblesGame.HEIGHT - 1;

        switch (walls[x, y] - 'A')
        {
            case BonusType.REGULAR:
                return true;
            case BonusType.DOUBLE:
                return true;
            case BonusType.LIFE:
                return true;
            case BonusType.REVERSE:
                return true;
            case BonusType.HALF:
                return false;
            default:
                if (walls[x, y] > NibblesGame.EMPTYCHAR
                    && walls[x, y] < 'z' + numworms)
                {
                        return false;
                }
                else
                {
                    if (ox == x && oy == y)
                        return false;

                    return Worm.ai_wander (walls, numworms, x, y, dir, ox, oy);
                }
        }
    }

    /* Determines the direction of the AI worm. */
    public void ai_move (int[,] walls, int numworms, Gee.LinkedList<Worm> worms)
    {
        var opposite = (direction + 1) % 4 + 1;

        var front = Worm.ai_wander (walls, numworms, head.x, head.y, direction, head.x, head.y);
        var left = Worm.ai_wander (walls, numworms, head.x, head.y, direction - 1, head.x, head.y);
        var right = Worm.ai_wander (walls, numworms, head.x, head.y, direction + 1, head.x, head.y);

        int dir;
        if (!front)
        {
            if (left)
            {
                /* Found a bonus to the left */
                dir = direction - 1;
                if (dir < 1)
                    dir = 4;

                direction = (WormDirection) dir;
            }
            else if (right)
            {
                /* Found a bonus to the right */
                dir = direction + 1;
                if (dir > 4)
                    dir = 1;

                direction = (WormDirection) dir;
            }
            else
            {
                /* Else move in random direction at random time intervals */
                if (Random.int_range (0, 30) == 1)
                {
                    dir = direction + (Random.boolean () ? 1 : -1);
                    if (dir != opposite)
                    {
                        if (dir > 4)
                            dir = 1;
                        if (dir < 1)
                            dir = 4;

                        direction = (WormDirection) dir;
                    }
                }
            }
        }

        /* Avoid walls, dead-ends and other worm's heads. This is done using
         * an evalution function which is CAPACITY for a wall, 4 if another
         * worm's head is in the tooclose area, 4 if another worm's head
         * could move to the same location as ours, plus 0 if there's no
         * dead-end, or the amount that doesn't fit for a deadend. olddir's
         * score is reduced by 100, to favour it, but only if its score is 0
         * otherwise; this is so that if we're currently trapped in a dead
         * end, the worm will move in a space-filling manner in the hope
         * that the dead end will disappear (e.g. if it's made from the tail
         * of some worm, as often happens).
         */
        var old_dir = direction;
        var best_yet = NibblesGame.CAPACITY * 2;
        var best_dir = -1;

        int this_len;
        for (dir = 1; dir <= 4; dir++)
        {
            direction = (WormDirection) dir;

            if (dir == opposite)
                continue;
            this_len = 0;

            if (!can_move_to (walls, numworms))
                this_len += NibblesGame.CAPACITY;

            if (ai_too_close (worms, numworms))
                this_len += 4;

            this_len += ai_deadend_after (walls, worms, numworms, head.x, head.y, dir, length + change);

            if (dir == old_dir && this_len <= 0)
                this_len -= 100;

            /* If the favoured direction isn't appropriate, then choose
             * another direction at random rather than favouring one in
             * particular, to stop the worms bunching in the bottom-
             * right corner of the board.
             */
            if (this_len <= 0)
                this_len -= Random.int_range (0, 100);
            if (this_len < best_yet)
            {
                best_yet = this_len;
                best_dir = dir;
            }
        }

        direction = (WormDirection) best_dir;

        /* Make sure we are at least avoiding walls.
         * Mostly other snakes should avoid our head.
         */
        for (dir = 1; dir <= 4; dir++)
        {
            if (dir == opposite)
                continue;

            if (!can_move_to (walls, numworms))
                direction = (WormDirection) dir;
            else
                continue;
        }
    }
}

public struct Position
{
    int x;
    int y;
}

public enum WormDirection
{
    NONE,
    RIGHT,
    DOWN,
    LEFT,
    UP
}

public struct WormProperties
{
    int color;
    uint up;
    uint down;
    uint left;
    uint right;
}
