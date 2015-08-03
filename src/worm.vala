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
    private const int STARTING_LIVES = 2;
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

    private WormDirection _direction;
    public WormDirection direction
    {
        get { return _direction; }
        set
        {
            if (keypress)
            {
                queue_keypress (value);
                return;
            }

            _direction = value;
            keypress = true;
        }
    }

    public WormDirection starting_direction;

    private Gee.ArrayQueue<WormDirection> key_queue;

    public Gee.LinkedList<Position?> list { get; private set; }

    public signal void added ();
    public signal void moved ();
    public signal void rescaled (int tile_size);
    public signal void died ();
    public signal void tail_reduced (int erase_size);

    public signal void bonus_found ();

    public Worm (int id)
    {
        this.id = id;
        is_human = true;
        lives = STARTING_LIVES;
        score = 0;
        change = 0;
        list = new Gee.LinkedList<Position?> ();
        key_queue = new Gee.ArrayQueue<WormDirection> ();
    }

    public Position head ()
    {
        return list.first ();
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

        var position = head ();
        switch (direction)
        {
            case WormDirection.UP:
                position.y = --head ().y;
                if (position.y < 0)
                    position.y = NibblesGame.HEIGHT - 1;
                break;
            case WormDirection.DOWN:
                position.y = ++head ().y;
                if (position.y >= NibblesGame.HEIGHT)
                    position.y = 0;
                break;
            case WormDirection.LEFT:
                position.x = --head ().x;
                if (position.x < 0)
                    position.x = NibblesGame.WIDTH - 1;
                break;
            case WormDirection.RIGHT:
                position.x = ++head ().x;
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
        if (walls[head ().x, head ().y] != NibblesGame.EMPTYCHAR)
            bonus_found ();

        /* Mark the tile as occupied by the worm's body */
        walls[head ().x, head ().y] = NibblesGame.WORMCHAR + id;

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

    public bool can_move_to (int[,] walls, int numworms)
    {
        var position = position_move ();

        if (walls[position.x, position.y] > NibblesGame.EMPTYCHAR
            && walls[position.x, position.y] < 'z' + numworms
            && position != list.last ()) /* The last position of the worm won't exist in the next frame */
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
        change = STARTING_LENGTH;
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
        direction = starting_direction;
        change = 0;
        spawn (walls);

        key_queue.clear ();

        is_stopped = false;
    }

    private Position position_move ()
    {
        Position position = head ();

        switch (direction)
        {
            case WormDirection.UP:
                position.y = --head ().y;
                if (position.y < 0)
                    position.y = NibblesGame.HEIGHT - 1;
                break;
            case WormDirection.DOWN:
                position.y = ++head ().y;
                if (position.y >= NibblesGame.HEIGHT)
                    position.y = 0;
                break;
            case WormDirection.LEFT:
                position.x = --head ().x;
                if (position.x < 0)
                    position.x = NibblesGame.WIDTH - 1;
                break;
            case WormDirection.RIGHT:
                position.x = ++head ().x;
                if (position.x >= NibblesGame.WIDTH)
                    position.x = 0;
                break;
            default:
                break;
        }

        return position;
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

    private uint upper_key (uint keyval)
    {
        if (keyval > 255)
            return keyval;
        return ((char) keyval).toupper ();
    }

    public void handle_direction (WormDirection dir)
    {
        direction = dir;
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
        direction = key_queue.poll ();
    }
}

public struct Position
{
    int x;
    int y;
}

public enum WormDirection
{
    UP,
    DOWN,
    LEFT,
    RIGHT
}

public struct WormProperties
{
    int color;
    uint left;
    uint right;
    uint up;
    uint down;
}
