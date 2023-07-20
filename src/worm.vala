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

private enum WormDirection
{
    NONE,   // unused, but allows to cast an integer from 1 to 4 into the four directions
    RIGHT,
    DOWN,
    LEFT,
    UP;

    internal WormDirection opposite ()
    {
        switch (this)
        {
            case RIGHT: return LEFT;
            case LEFT: return RIGHT;
            case DOWN: return UP;
            case UP: return DOWN;
            default: assert_not_reached ();
        }
    }

    internal WormDirection turn_left ()
    {
        switch (this)
        {
            case RIGHT: return UP;
            case UP: return LEFT;
            case LEFT: return DOWN;
            case DOWN: return RIGHT;
            default: assert_not_reached ();
        }
    }

    internal WormDirection turn_right ()
    {
        switch (this)
        {
            case RIGHT: return DOWN;
            case DOWN: return LEFT;
            case LEFT: return UP;
            case UP: return RIGHT;
            default: assert_not_reached ();
        }
    }
}

private struct Position
{
    uint8 x;
    uint8 y;

    internal void move (WormDirection direction, uint8 width, uint8 height)
    {
        switch (direction)
        {
            case WormDirection.UP:
                if (y == 0)
                    y = height - 1;
                else
                    y--;
                break;

            case WormDirection.DOWN:
                if (y >= height - 1)
                    y = 0;
                else
                    y++;
                break;

            case WormDirection.LEFT:
                if (x == 0)
                    x = width - 1;
                else
                    x--;
                break;

            case WormDirection.RIGHT:
                if (x >= width - 1)
                    x = 0;
                else
                    x++;
                break;

            default:
                assert_not_reached ();
        }
    }
}

private class WormProperties : Object
{
    internal int color  { internal get; internal set; }
    internal uint up    { internal get; internal set; }
    internal uint down  { internal get; internal set; }
    internal uint left  { internal get; internal set; }
    internal uint right { internal get; internal set; }
}

private class Worm : Object
{
    private const int STARTING_LENGTH = 5; /* STARTING_LENGTH must be greater than 0 */
    internal const uint8 STARTING_LIVES = 6;
    internal const uint8 MAX_LIVES = 12;

    internal const int GROW_FACTOR = 4;

    internal Position starting_position { internal get; private set; }

    public int id { internal get; protected construct; }

    internal bool is_human;
    internal bool keypress = false;
    internal bool is_stopped = false;
    internal bool is_materialized { internal get {return rounds_to_stay_dematerialized <= 0;} }
    private int rounds_to_stay_dematerialized;

    internal uint8 lives    { internal get; internal set; default = STARTING_LIVES; }
    internal int change     { internal get; internal set; default = 0; }
    internal int score      { internal get; internal set; default = 0; }

    internal int length
    {
        get { return list.size; }
    }

    internal Position head
    {
        get
        {
            Position head = list.first ();
            return head;
        }
        private set
        {
            list.@set (0, value);
        }
    }

    internal WormDirection direction { internal get; private set; }

    private WormDirection starting_direction;

    private Gee.ArrayQueue<WormDirection> key_queue = new Gee.ArrayQueue<WormDirection> ();

    internal Gee.LinkedList<Position?> list { internal get; private set; default = new Gee.LinkedList<Position?> (); }

    internal signal void added ();
    internal signal void finish_added ();
    internal signal void moved ();
    internal signal void rescaled (int tile_size);
    internal signal void died ();
    internal signal void tail_reduced (int erase_size);
    internal signal void reversed ();

    internal signal void bonus_found ();

    public uint8 width  { private get; protected construct; }
    public uint8 height { private get; protected construct; }
    public int capacity { private get; protected construct; }

    construct
    {
        deadend_board = new uint [width, height];
    }

    internal Worm (int id, uint8 width, uint8 height)
    {
        int capacity = width * height;
        Object (id: id, width: width, height: height, capacity: capacity);
    }

    internal void set_start (uint8 x, uint8 y, WormDirection direction)
    {
        list.clear ();

        starting_position = Position () { x = x, y = y };

        list.add (starting_position);

        starting_direction = direction;
        this.direction     = direction;
        change = 0;
        key_queue.clear ();
    }

    internal void move_part_1 ()
    {
        if (is_human)
            keypress = false;

        Position position = head;
        position.move (direction, width, height);

        /* Add a new body piece */
        list.offer_head (position);
    }

    internal void move_part_2 (int[,] board, Position? head_position)
    {
        if (head_position != null)
            head = Position () { x = head_position.x, y = head_position.y };

        if (change > 0)
        {
            /* Add to the worm's size. */
            change--;
            added (); /* signal function in nibbles-view.vala */
        }
        else
        {
            /* Remove a body piece from the tail of the list. */
            board[list.last ().x, list.last ().y] = NibblesGame.EMPTYCHAR;
            list.poll_tail ();
            moved (); /* signal function in nibbles-view.vala */
        }

        /* Check for bonus, do nothing if there isn't a bonus */
        if (board[head.x, head.y] != NibblesGame.EMPTYCHAR)
            bonus_found (); /* signal function in nibble-game.vala */

        /* Mark the tile as occupied by the worm's body, if it is materialized */
        if (rounds_to_stay_dematerialized > 1)
            board[head.x, head.y] = NibblesGame.WORMCHAR + id;
        else if (rounds_to_stay_dematerialized > 1)
            rounds_to_stay_dematerialized -= 1;

        if (!key_queue.is_empty)
            dequeue_keypress ();

        /* Try and dematerialize if our rounds are up. */
        if (rounds_to_stay_dematerialized == 1)
            materialize (board);
    }

    /* This function is only called from nibbles-game.vala */
    internal void reduce_tail (int[,] board, int erase_size)
    {
        if (erase_size <= 0)
            return;

        for (int i = 0; i < erase_size; i++)
        {
            board[list.last ().x, list.last ().y] = NibblesGame.EMPTYCHAR;
            list.poll_tail ();
        }
        tail_reduced (erase_size);
    }

    internal void reverse (int[,] board)
    {
        if (!is_stopped && !list.is_empty)
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
    }

    internal bool can_move_to (int[,] board, int numworms, Position position)
    {
        int next_position = board[position.x, position.y];

        if (next_position > NibblesGame.EMPTYCHAR
            && next_position < NibblesGame.WORMCHAR)
            return false;

        if (next_position >= NibblesGame.WORMCHAR
            && next_position < NibblesGame.WORMCHAR + numworms)
            return !is_materialized;

        return true;
    }

    internal void spawn (int[,] board)
    {
        assert (STARTING_LENGTH > 0);
        change = STARTING_LENGTH - 1;
        rounds_to_stay_dematerialized = STARTING_LENGTH;
        for (int i = 0; i < STARTING_LENGTH; i++)
        {
            move_part_1 ();
            move_part_2 (board, /* no warp */ null);
        }
    }

    private void materialize (int [,] board)
    {
        foreach (var pos in list)
        {
            if (board[pos.x, pos.y] != NibblesGame.EMPTYCHAR)
            {
                rounds_to_stay_dematerialized += 1;
                return;
            }
        }
        foreach (var pos in list)
            board[pos.x, pos.y] = NibblesGame.WORMCHAR + id;
        rounds_to_stay_dematerialized = 0;
    }

    internal void dematerialize (int [,] board, int rounds, int gamedelay)
    {
        rounds_to_stay_dematerialized = rounds;
        foreach (var pos in list)
        {
            if (board[pos.x, pos.y] == NibblesGame.WORMCHAR + id)
                board[pos.x, pos.y] = NibblesGame.EMPTYCHAR;
        }

        Timeout.add (gamedelay * 27, () => {
                is_stopped = false;
                return Source.REMOVE;
            });
    }

    internal void add_life ()
    {
        if (lives > MAX_LIVES)
            return;

        lives++;
    }

    private inline void lose_life ()
    {
        if (lives == 0)
            return;

        lives--;
    }

    internal void reset (int[,] board)
    {
        is_stopped = true;
        rounds_to_stay_dematerialized = 0;

        key_queue.clear ();

        lose_life ();

        died ();
        foreach (var pos in list)
            board[pos.x, pos.y] = NibblesGame.EMPTYCHAR;

        list.clear ();
        if (lives > 0)
        {
            list.add (starting_position);
            added ();

            direction = starting_direction;
            spawn (board);

            finish_added ();
        }
    }

    internal Position position_move ()
    {
        Position position = head;
        position.move (direction, width, height);
        return position;
    }

    private void direction_set (WormDirection dir)
        requires (dir != WormDirection.NONE)
    {
        if (!is_human)
            return;

        if (keypress)
        {
            queue_keypress (dir);
            return;
        }

        direction = dir;
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

    internal bool handle_keypress (uint keyval, Gee.HashMap<Worm, WormProperties> worm_props)
    {
        if (lives == 0 || is_stopped || list.is_empty)
            return false;

        WormProperties properties;
        uint propsUp, propsDown, propsLeft, propsRight, keyvalUpper;

        properties = worm_props.@get (this);
        propsUp = upper_key (properties.up);
        propsLeft = upper_key (properties.left);
        propsDown = upper_key (properties.down);
        propsRight = upper_key (properties.right);
        keyvalUpper = upper_key (keyval);

        if ((keyvalUpper == propsUp) && (direction != WormDirection.DOWN))
        {
            direction_set (WormDirection.UP);
            return true;
        }
        if ((keyvalUpper == propsDown) && (direction != WormDirection.UP))
        {
            direction_set (WormDirection.DOWN);
            return true;
        }
        if ((keyvalUpper == propsRight) && (direction != WormDirection.LEFT))
        {
            direction_set (WormDirection.RIGHT);
            return true;
        }
        if ((keyvalUpper == propsLeft) && (direction != WormDirection.RIGHT))
        {
            direction_set (WormDirection.LEFT);
            return true;
        }

        return false;
    }

    private void queue_keypress (WormDirection dir)
    {
        /* Ignore duplicates in normal movement mode. This resolves the key
         * repeat issue
         */
        if (!key_queue.is_empty && dir == key_queue.peek ())
            return;

        key_queue.add (dir);
    }

    private void dequeue_keypress ()
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
    private static uint[,] deadend_board;
    private static uint deadend_runnumber = 0;

    private static int ai_deadend (int[,] board, int numworms, Position old_position, int length_left)
    {
        uint8 width  = (uint8) /* int */ board.length [0];
        uint8 height = (uint8) /* int */ board.length [1];

        if (length_left <= 0)
            return 0;

        for (int dir = 4; dir > 0; dir--)
        {
            Position new_position = old_position;
            new_position.move ((WormDirection) dir, width, height);

            if ((board [new_position.x, new_position.y] <= NibblesGame.EMPTYCHAR
              || board [old_position.x, old_position.y] >= 'z' + numworms)
             && (deadend_board [new_position.x, new_position.y] != deadend_runnumber))
            {
                deadend_board [new_position.x, new_position.y] = deadend_runnumber;
                length_left = ai_deadend (board, numworms, new_position, length_left - 1);
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
    private static int ai_deadend_after (int[,] board, Gee.LinkedList<Worm> worms, int numworms, Position old_position, WormDirection direction, int length)
    {
        uint8 width  = (uint8) /* int */ board.length [0];
        uint8 height = (uint8) /* int */ board.length [1];

        if (old_position.x >= width
         || old_position.y >= height)
            return 0;

        deadend_runnumber++;

        for (int i = worms.size - 1; i >= 0; i--)
        {
            if (!worms[i].is_stopped && !worms[i].list.is_empty)
            {
                uint8 target_x = worms [i].head.x;
                uint8 target_y = worms [i].head.y;
                if (target_x == old_position.x
                 && target_y == old_position.y)
                    continue;

                if (target_x > 0)           deadend_board [target_x - 1, target_y    ] = deadend_runnumber;
                else                        deadend_board [width    - 1, target_y    ] = deadend_runnumber;
                if (target_y > 0)           deadend_board [target_x    , target_y - 1] = deadend_runnumber;
                else                        deadend_board [target_x    , height   - 1] = deadend_runnumber;
                if (target_x < width - 1)   deadend_board [target_x + 1, target_y    ] = deadend_runnumber;
                else                        deadend_board [0           , target_y    ] = deadend_runnumber;
                if (target_y < height - 1)  deadend_board [target_x    , target_y + 1] = deadend_runnumber;
                else                        deadend_board [target_x    , 0           ] = deadend_runnumber;
            }
        }

        Position new_position = old_position;
        new_position.move (direction, width, height);

        deadend_board [old_position.x, old_position.y] = deadend_runnumber;
        deadend_board [new_position.x, new_position.y] = deadend_runnumber;

        int cl = (length * length) / 16;
        if (cl < (int) width)
            cl = width;
        return ai_deadend (board, numworms, new_position, cl);
    }

    /* Check to see if another worm's head is too close in front of us;
     * that is, that it's within 3 in the direction we're going and within
     * 1 to the side.
     */
    private inline bool ai_too_close (Gee.LinkedList<Worm> worms)
    {
        foreach (Worm worm in worms)
        {
            if (worm == this || worm.is_stopped || worm.list.is_empty)
                continue;

            int16 dx = (int16) this.head.x - (int16) worm.head.x;
            int16 dy = (int16) this.head.y - (int16) worm.head.y;
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

                default:
                    assert_not_reached ();
            }
        }
        return false;
    }

    private static bool ai_wander (int[,] board, int numworms, Position updated_position, WormDirection direction, Position initial_position)
    {
        uint8 width  = (uint8) /* int */ board.length [0];
        uint8 height = (uint8) /* int */ board.length [1];

        updated_position.move (direction, width, height);

        switch (board [updated_position.x, updated_position.y] - 'A')
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
                if (board [updated_position.x, updated_position.y] > NibblesGame.EMPTYCHAR
                 && board [updated_position.x, updated_position.y] < 'z' + numworms)
                    return false;
                if (updated_position.x == initial_position.x
                 && updated_position.y == initial_position.y)
                    return false;
                return ai_wander (board, numworms, updated_position, direction, initial_position);
        }
    }

    /* Determines the direction of the AI worm. */
    internal void ai_move (int[,] board, int numworms, Gee.LinkedList<Worm> worms)
    {
        WormDirection opposite = direction.opposite ();

        /* if no bonus in front */
        if (!ai_wander (board, numworms, head, direction, head))
        {
            /* FIXME worms will prefer to turn left than right */

            /* if bonus found to the left */
            if (ai_wander (board, numworms, head, direction.turn_left (), head))
                direction = direction.turn_left ();

            /* if bonus found to the right */
            else if (ai_wander (board, numworms, head, direction.turn_right (), head))
                direction = direction.turn_right ();

            /* if no bonus found, move in random direction at random time intervals */
            else if (Random.int_range (0, 30) == 1)
                direction = Random.boolean () ? direction.turn_right () : direction.turn_left ();
        }

        /* Avoid walls, dead-ends and other worm's heads. This is done using
         * an evaluation function which is CAPACITY for a wall, 4 if another
         * worm's head is in the tooclose area, 4 if another worm's head
         * could move to the same location as ours, plus 0 if there's no
         * dead-end, or the amount that doesn't fit for a deadend. olddir's
         * score is reduced by 100, to favour it, but only if its score is 0
         * otherwise; this is so that if we're currently trapped in a dead
         * end, the worm will move in a space-filling manner in the hope
         * that the dead end will disappear (e.g. if it's made from the tail
         * of some worm, as often happens).
         */
        WormDirection prev_dir = direction;
        WormDirection best_dir = NONE;
        int best_yet = capacity * 2;

        int this_len;
        for (int dir = 1; dir <= 4; dir++)
        {
            /* TODO make method static, and do not make tests with the class direction variable */
            direction = (WormDirection) dir;

            if (direction == opposite)
                continue;
            this_len = 0;

            if (!can_move_to (board, numworms, position_move ()))
                this_len += capacity;

            if (ai_too_close (worms))
                this_len += 4;

            this_len += ai_deadend_after (board, worms, numworms, head, direction, length + change);

            if (direction == prev_dir && this_len <= 0)
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
                best_dir = direction;
            }
        }

        if (best_dir == NONE)
            assert_not_reached ();

        direction = best_dir;

        /* Make sure we are at least avoiding walls.
         * Mostly other snakes should avoid our head.
         */
        for (uint8 dir = 1; dir <= 4; dir++)
        {
            if (opposite == (WormDirection) dir)
                continue;

            if (!can_move_to (board, numworms, position_move ()))
                direction = (WormDirection) dir;
        }
    }
}
