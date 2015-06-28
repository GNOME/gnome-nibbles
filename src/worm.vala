/* Gnome Nibbles: Gnome Worm Game
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

public class Worm : Object
{
    public const int STARTING_LENGTH = 5;
    private const int STARTING_LIVES = 6;

    public Position starting_position { get; private set; }

    public int id { get; private set; }

    public bool human;
    public bool keypress = false;
    public bool is_stopped = false;

    public int lives { get; private set; }
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

    public Worm (int id, WormDirection direction)
    {
        this.id = id;
        human = true;
        starting_direction = direction;
        lives = STARTING_LIVES;
        list = new Gee.LinkedList<Position?> ();
        key_queue = new Gee.ArrayQueue<WormDirection> ();
    }

    public Position head ()
    {
        return list.first ();
    }

    public void set_start (int xhead, int yhead)
    {
        starting_position = Position () {
            x = xhead,
            y = yhead
        };

        list.add (starting_position);

        this.direction = starting_direction;
    }

    public void move (int[,] walls, bool remove)
    {
        if (human)
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
        /* Mark the tile as occupied by the worm's body */
        walls[head ().x, head ().y] = NibblesGame.WORMCHAR + id;

        if (remove)
        {
            walls[list.last ().x, list.last ().y] = NibblesGame.EMPTYCHAR;
            list.poll_tail ();
            moved ();
        }
        else
            added ();

        if (!key_queue.is_empty)
            dequeue_keypress ();
    }

    public bool can_move_to (int[,] walls, int numworms)
    {
        Position position = position_move ();

        if (walls[position.x, position.y] > NibblesGame.EMPTYCHAR &&
            walls[position.x, position.y] < 'z' + numworms)
            return false;

        return true;
    }

    public bool collides_with_head (Position other_head)
    {
        if (head ().x == other_head.x)
            return head ().y - 1 == other_head.y || head ().y + 1 == other_head.y;
        if (head ().y == other_head.y)
            return head ().x - 1 == other_head.x || head ().x + 1 == other_head.x;

        return false;
    }

    public void spawn (int[,] walls)
    {
        for (int i = 0; i < STARTING_LENGTH; i++)
            move (walls, false);
    }

    public void lose_life ()
    {
        lives--;
    }

    public void die (int[,] walls)
    {
        is_stopped = true;
        lose_life ();

        died ();
        foreach (var pos in list)
            walls[pos.x, pos.y] = NibblesGame.EMPTYCHAR;

        list.clear ();
        list.add (starting_position);
        direction = starting_direction;
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
                requires (!key_queue.is_empty)
    {
        /* Ignore duplicates in normal movement mode. This resolves the key
         * repeat issue
         */
        if (!key_queue.is_empty && dir == key_queue.peek ())
            return;

        key_queue.add (dir);
    }

    public void dequeue_keypress ()
    {
        direction = key_queue.poll ();
    }
}
