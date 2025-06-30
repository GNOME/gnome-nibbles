/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Gnome Nibbles: Gnome Worm Game
 * Copyright (C) 2015 Iulian-Gabriel Radu <iulian.radu67@gmail.com>
 * Copyright (C) 2022-25 Ben Corby <bcorby@new-ms.com>
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
private enum WormDirection
{
    NONE,   // unused, but allows to cast an integer from 1 to 4 into the four directions
    RIGHT,
    EAST = RIGHT,
    DOWN,
    SOUTH = DOWN,
    LEFT,
    WEST = LEFT,
    UP,
    NORTH = UP;

    internal WormDirection turn_left ()
    {
        switch (this)
        {
            case EAST:
                return NORTH;
            case NORTH:
                return WEST;
            case WEST:
                return SOUTH;
            case SOUTH:
                return EAST;
            default:
                assert_not_reached ();
        }
    }

    internal WormDirection turn_right ()
    {
        switch (this)
        {
            case EAST:
                return SOUTH;
            case SOUTH:
                return WEST;
            case WEST:
                return NORTH;
            case NORTH:
                return EAST;
            default:
                assert_not_reached ();
        }
    }

    internal WormDirection[] get_space_fill_array ()
    {
        switch (this)
        {
            case WEST:
                return {SOUTH,WEST,NORTH};
            case NORTH:
                return {EAST,WEST,NORTH};
            case EAST:
                return {EAST,SOUTH,NORTH};
            case SOUTH:
                return {EAST,SOUTH,WEST};
            default:
                assert_not_reached ();
        }
    }

 #if !TEST_COMPILE
    internal WormDirection reverse ()
    {
        switch (this)
        {
            case EAST:
                return WEST;
            case SOUTH:
                return NORTH;
            case WEST:
                return EAST;
            case NORTH:
                return SOUTH;
            default:
                assert_not_reached ();
        }
    }
#endif
}

private struct Position
{
    uint8 x;
    uint8 y;

    internal void move (WormDirection direction, uint8 width, uint8 height)
    {
        switch (direction)
        {
            case WormDirection.NORTH:
                if (y == 0)
                    y = height - 1;
                else
                    y--;
                break;
            case WormDirection.SOUTH:
                if (y >= height - 1)
                    y = 0;
                else
                    y++;
                break;
            case WormDirection.WEST:
                if (x == 0)
                    x = width - 1;
                else
                    x--;
                break;
            case WormDirection.EAST:
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

/*
 * This structure is similar to struct Position except x & y
 * are signed to allow wrapping around the board.
 */
public struct SignedPosition
{
    public int64 x; /* x increases going right (or east) */
    public int64 y; /* y increases going down (or south) */

    /* used by wrap functions */
    public int64 x_max;
    public int64 y_max;

    public uint8 wrap_x ()
    {
        assert (x_max > 0); /* call set_wrapping (x, y) first */
        if (x > x_max)
            return (uint8)(x % x_max);
        else if (x < 0)
            return (uint8)(x + (x.abs () / x_max + 1) * x_max);
        else
            return (uint8)x;
    }

    public uint8 wrap_y ()
    {
        assert (y_max > 0); /* call set_wrapping (x, y) first */
        if (y > y_max)
            return (uint8)(y % y_max);
        else if (y < 0)
            return (uint8)(y + (y.abs () / y_max + 1) * y_max);
        else
            return (uint8)y;
    }

    public uint16 wrap_xy ()
    {
        return ((uint16)wrap_x () << 8) | wrap_y ();
    }
}

/*
 * An array to store the positions of worms.
 * The contains function is the equivalent of the function
 * is_position_clear_of_materialized_worms but much faster.
 */
private class WormMap : Object
{
    /* variables */
    private uint64[,] map;

    /* constants */
    const uint8 bits = (uint8) (sizeof (uint64) * 8);

    /* public functions */

    /* constructor */
    public WormMap (Gee.List<Worm> worms, uint8 map_width, uint8 map_height)
    {
        /*
         * The width of this array is determined by the number of bits
         * in the array type and the width of the map.
         * As a location can only be empty or occupied we need one
         * bit to represent each location on the map.
         *
         * If we had a map width of 64 and an array type of
         * uint64 we would need one uint64 to store the
         * information.
         * The math:
         * (map_width – 1) / bits + 1
         * (64 – 1) / 64 + 1
         * 63 / 64 + 1
         * 0 + 1
         * 1
         *
         * If we had a map width of 65 and an array type of
         * uint64 we would need two uint64s to store the
         * information.
         * The math:
         * (map_width – 1) / bits + 1
         * (65 – 1) / 64 + 1
         * 64 / 64 + 1
         * 1 + 1
         * 2
         */
        map = new uint64 [ (map_width - 1) / bits + 1, map_height];
        add (worms);
    }

    public bool contain (uint16 p)
    {
        /*
         * To test if the position p is occupied we need to locate
         * the position within the array and the position
         * within the unsigned integer.
         * As each unsigned integer contains the same number of
         * bits we can simply divide p.x by this number bits to
         * determine the position within the array.
         * The remainder from the above division is the bit position
         * we want within the unsigned integer.
         * Unfortunately in vala there is no mathematical operator that
         * gives both the quotient and the remainder from a division
         * so we have two use two operators / and %.
         * Once we have the bit we and the result with 1 (to mask out
         * any other bits) and do a greater than 0 test to trick the
         * compiler to not complain about the return type.
         *
         */
        uint8 quotient = (p>>8) / bits;
        uint8 remainder = (p>>8) % bits;

        return (map [quotient, (uint8)p] >> remainder & 1) > 0;
    }

    public bool contain_position (Position p)
    {
        return contain (((uint16)p.x) << 8 | p.y);
    }

    public bool contains (WormPositions position_list)
    {
        foreach (var p in position_list)
            if (contain (p))
                return true;
        return false;
    }

    /* private functions */

    private void add (Gee.List<Worm> worms)
    {
        foreach (Worm worm in worms)
        {
            if (worm.is_materialized && !worm.is_stopped)
            {
                foreach (var p in worm.list)
                {
                    /*
                     * We use the same logic here as in the contain
                     * function. Once we locate the bit it is then set
                     * using the |= operator.
                     *
                     */
                    uint8 x = p>>8;
                    uint8 y = (uint8)p;
                    map [x / bits, y] |= (uint64)1 << (x % bits);
                }
            }
        }
    }
}

/* A cut down 128 bit integer with only a subset of operators. */
struct int128
{
    /* variables */
    uint64 hi;
    uint64 lo;
    bool   negative;
    uint64 remainder;

    /* public functions */

    /* *this <<= shift */
    internal void left_shift_assign (int shift)
    {
        if (shift >= 64)
        {
            hi = lo;
            lo = 0;
            shift -= 64;
        }
        if (shift > 0)
        {
            hi <<= shift;
            hi |= lo >> (64 - shift);
            lo <<= shift;
        }
    }

    /* *this << shift */
    internal int128 left_shift (int shift)
    {
        int128 r = {hi,lo,negative};
        r.left_shift_assign (shift);
        return r;
    }

    /* *this >>= shift */
    internal void right_shift_assign (int shift)
    {
        if (shift >= 64)
        {
            lo = hi;
            hi = 0;
            shift -= 64;
        }
        if (shift > 0)
        {
            lo >>= shift;
            lo |= hi << (64 - shift);
            hi >>= shift;
        }
    }

    /* *this += a */
    internal void add_assign (int128 a)
    {
        hi += a.hi;
        uint64 o = lo;
        lo += a.lo;
        if (lo < o)
            hi += 1;
    }

    /* *this > a */
    internal bool greater_than (int128 a)
    {
        if (!negative && !a.negative)
            return hi > a.hi || hi == a.hi && lo > a.lo;
        else if (!negative && a.negative)
            return true;
        else if (negative && !a.negative)
            return false;
        else
            return hi < a.hi || hi == a.hi && lo < a.lo;
    }

    /* *this <= a */
    internal bool less_than_or_equal_to (int128 a)
    {
        return !greater_than (a);
    }

    /* *this < a */
    internal bool less_than (int128 a)
    {
        if (!negative && !a.negative)
            return hi < a.hi || hi == a.hi && lo < a.lo;
        else if (!negative && a.negative)
            return false;
        else if (negative && !a.negative)
            return true;
        else
            return hi > a.hi || hi == a.hi && lo > a.lo;
    }

    /* *this == a */
    internal bool equal_to (int128 a)
    {
        return negative == a.negative && hi == a.hi && lo == a.lo;
    }

    /* *this / x */
    internal int128 divide_by (int64 x)
    {
        /* This algorithm requires the absolute value of x to be
         * less than or equal to 2^63 (0x8000000000000000).
         */
        uint64 div = x.abs ();
        int128 s = {hi, lo};
        int128 d = {0, 0, negative && x >= 0 || !negative && x < 0};

        /* deal with special cases */
        if (s.hi == 0 && s.lo == 0)
        {
            d = {0, 0, false};
            return d;
        }
        else if (div == 0)
        {
            d.hi = 0xffffffffffffffff;
            d.lo = 0xffffffffffffffff;
            return d;
        }

        /* for performance, repeatedly divide the numerator and denominator by 2 while there is no remainder */
        for (; (s.lo & 1) == 0 && (div & 1) == 0; s.right_shift_assign (1), div >>= 1);

        if (div == 1)
        {
            s.negative = d.negative;
            return s;
        }

        for (;s.hi > 0;)
        {
            int shift = 0;
            int128 a = {s.hi, s.lo};
            if (a.hi < div)
                for (; a.hi > 0; a.right_shift_assign (1), ++shift);
            /* a is assigned the quotient of the division */
            a.hi /= div;
            a.lo /= div;
            a.left_shift_assign (shift);
            /* s is assigned the remainder of the division */
            s.minus (a.multiply (div));
            /* add the quotient to our result */
            d.add_assign (a);
        }
        if (s.lo >= div)
        {
            int128 a = {0, s.lo / div};
            d.add_assign (a);
            d.remainder = s.lo % div;
        }
        else
            d.remainder = s.lo;
        return d;
    }

    /* Is *this small enough to fit in an int64? */
    bool is_valid_int64 ()
    {
        return hi == 0
            && (!negative && (lo & 0x8000000000000000) == 0
            || (negative && lo <= 0x8000000000000000));
    }

    /* get the int64 component from this int128 */
    internal int64 get_int64 ()
    {
        assert (is_valid_int64 ());
        return negative ? (int64) (-lo) : (int64) (lo);
    }

    /* return *this multiplied by a as an int128 */
    int128 multiply (uint64 _a)
    {
        int128 a = {0, _a, false};
        int128 r = {0, 0, false};
        for (int shift = 0; a.hi > 0 || a.lo > 0; a.right_shift_assign (1), shift++)
        {
            if ((a.lo & 1) > 0)
                r.add_assign (left_shift (shift));
        }
        return r;
    }

    /* *this - a */
    void minus (int128 a)
    {
        // assumes that *this >= a */
        hi -= a.hi;
        if (lo < a.lo)
            --hi;
        lo -= a.lo;
    }
}

/* return a multiplied by b as an int128 */
int128 multiply (int64 _a, int64 _b)
{
    int128 a = {0, _a.abs () < _b.abs () ? _b.abs () : _a.abs ()};
    int128 b = {0, _a.abs () >= _b.abs () ? _b.abs () : _a.abs ()};
    int128 r = {0, 0, _a < 0 && _b >= 0 || _a >= 0 && _b < 0};
    for (int shift = 0; b.hi > 0 || b.lo > 0; b.right_shift_assign (1), shift++)
    {
        if ((b.lo & 1) > 0)
            r.add_assign (a.left_shift (shift));
    }
    return r;
}

/* return a multiplied by 2 to the power of n as an int128 */
int128 multiply_by_2n (int64 a, int n)
{
    int128 r = {0, a.abs (), a < 0};
    r.left_shift_assign (n);
    return r;
}

/* Very simple iterator for struct Angle. */
private class AngleIterator
{
    /* variables */
    private Angle pAngle; /* pointer to Angle we are iterating over */
    private uint64 index;

    /* public functions */
    public AngleIterator (Angle p)
    {
        pAngle = p;
        index = 0; /* skip the origin */
    }

    public bool next ()
    {
        ++index;
        return true;
    }

    public SignedPosition @get ()
    {
        return pAngle.@get (index);
    }
}

/* an enumerated type that represents one quarter of the board */
enum Quarter {Q0,Q1,Q2,Q3}

/*
 * Structure to store any angle. e.g 45° (PI / 4) or 2½° (PI / 72).
 * The angle is stored as a ratio of x (opposite) over y (adjacent)
 * so that we only need to use integers in all our operations.
 * To convert to degrees use (Math.atan2 (x,-y) / Math.PI * 180).
 *
 */
private struct Angle
{
    /* variables */
    public int64 x; /* x increases going right (or east) */
    public int64 y; /* y increases going down (or south) */

    /* variables used by @get */
    public const int step_multiplier_2n = 38;
    public const int64 step_multiplier = 274877906944; /* 2 to the power of 38 */
    public int64 origin_x;
    public int64 origin_y;

    /* variables used by wrap functions in struct SignedPosition */
    public int64 x_max; /* one more that the max */
    public int64 y_max; /* one more that the max */


    /* public functions */

    /* this = o */
    public void assign (Angle o)
    {
        x = o.x;
        y = o.y;
        origin_x = o.origin_x;
        origin_y = o.origin_y;
        x_max = o.x_max;
        y_max = o.y_max;
    }

    /* this <= o */
    public bool less_than_or_equal_to (Angle o)
    {
        return !greater_than (o);
    }

    /* this > o */
    public bool greater_than (Angle o)
    {
        Quarter q=o.get_quarter ();
        switch (get_quarter ())
        {
            case Q0:
                if (q == Q3)
                    return true;
                else if (q == Q1)
                    return false;
                else
                    return multiply (x, o.y).less_than (multiply (o.x, y));
            case Q1:
                if (q == Q0)
                    return true;
                else if (q == Q2)
                    return false;
                else
                    return multiply (x, o.y).less_than (multiply (o.x, y));
            case Q2:
                if (q == Q1)
                    return true;
                else if (q == Q3)
                    return false;
                else
                    return multiply (x, o.y).less_than (multiply (o.x, y));
            case Q3:
                if (q == Q2)
                    return true;
                else if (q == Q0)
                    return false;
                else
                    return multiply (x, o.y).less_than (multiply (o.x, y));
            default:
                return false;
        }
    }

    /* this < o */
    public bool less_than (Angle o)
    {
        return !equal_to (o) && less_than_or_equal_to (o);
    }

    /* this >= o */
    public bool greater_than_or_equal_to (Angle o)
    {
        Quarter q=o.get_quarter ();
        switch (get_quarter ())
        {
            case Q0:
                if (q == Q3)
                    return true;
                else if (q == Q1)
                    return false;
                else if (q == Q2)
                    return multiply (x, o.y).less_than (multiply (o.x, y));
                else
                    return multiply (x, o.y).less_than_or_equal_to (multiply (o.x, y));
            case Q1:
                if (q == Q0)
                    return true;
                else if (q == Q2)
                    return false;
                else if (q == Q3)
                    return multiply (x, o.y).less_than (multiply (o.x, y));
                else
                    return multiply (x, o.y).less_than_or_equal_to (multiply (o.x, y));
            case Q2:
                if (q == Q1)
                    return true;
                else if (q == Q3)
                    return false;
                else if (q == Q0)
                    return multiply (x, o.y).less_than (multiply (o.x, y));
                else
                    return multiply (x, o.y).less_than_or_equal_to (multiply (o.x, y));
            case Q3:
                if (q == Q2)
                    return true;
                else if (q == Q0)
                    return false;
                else if (q == Q1)
                    return multiply (x, o.y).less_than (multiply (o.x, y));
                else
                    return multiply (x, o.y).less_than_or_equal_to (multiply (o.x, y));
            default:
                return false;
        }
    }

    /* this == o */
    public bool equal_to (Angle o)
    {
        return (get_quarter () == o.get_quarter ())
            && multiply (x, o.y).equal_to (multiply (o.x, y));
    }

    public void set_origin (Position origin)
    {
        origin_x = (origin.x * 2 + 1) * (step_multiplier / 2);
        origin_y = (origin.y * 2 + 1) * (step_multiplier / 2);
    }

    public void set_wrapping (int x, int y)
    {
        x_max = x;
        y_max = y;
    }

    public bool step_along_x ()
    {
        return x.abs () > y.abs ();
    }

    /* get the i position along the angle */
    public SignedPosition @get (uint64 i)
    {
        /* step along x or y axis depending on which is the larger */
        int64 delta_x = step_along_x () ? set_delta_x_sign (step_multiplier)
            : set_delta_x_sign (multiply_by_2n (x, step_multiplier_2n).divide_by (y).get_int64 ());
        int64 delta_y = step_along_x () ? set_delta_y_sign (multiply_by_2n (y, step_multiplier_2n).divide_by (x).get_int64 ())
            : set_delta_y_sign (step_multiplier);

        /* calculate new x,y position */
        int64 x_i = origin_x + delta_x * (int64)i;
        int64 y_i = origin_y + delta_y * (int64)i;

        return { (x_i >> step_multiplier_2n),
            (y_i >> step_multiplier_2n), x_max, y_max};
    }

    public AngleIterator iterator ()
    {
        return new AngleIterator (this);
    }


    /* private functions */

    /*
     *             |
     *  quarter 3  |  quarter 0
     *             |
     * ------------+-------------
     *             |
     *  quarter 2  |  quarter 1
     *             |
     *
     * + is a x=0,y=0
     */
    Quarter get_quarter ()
    {
        if (x >= 0 && y < 0)
            return Q0;
        else if (x > 0 && y >= 0)
            return Q1;
        if (x <= 0 && y > 0)
            return Q2;
        else
            return Q3;
    }

    /* set the parameter to the same sign as x */
    int64 set_delta_x_sign (int64 _x)
    {
        if (x >= 0 && _x < 0 || x < 0 && _x >= 0)
            return -_x;
        else
            return _x;
    }

    /* set the parameter to the same sign as y */
    int64 set_delta_y_sign (int64 _y)
    {
        if (y >= 0 && _y < 0 || y < 0 && _y >= 0)
            return -_y;
        else
            return _y;
    }
}

/*
 * Class to store a slice (of cake).
 * 0° is the same as north on a compass.
 * 180° is the same as south on a compass.
 * e.g a slice between 45° and 90°
 * would be between north east and east and would be
 * one eighth of the whole (cake).
 *
 *        /
 *       /
 *      /
 *     /  slice
 *    /
 *   o-----
 *
 * We always go clockwise therefore in this example
 * the min angle is 45° and the max angle 90°.
 * If the max angle is less than or equal to the min
 * angle we have an empty slice.
 *
 */
internal class Slice : Object
{
    /* variables */
    public Angle min;
    public Angle max;
    bool min_set;
    bool max_set;

    /* constructor */
    public Slice (Slice? copy = null)
    {
        if (copy == null)
        {
            min_set = false;
            max_set = false;
        }
        else /*copy != null*/
            assign (copy);
    }

    /* public functions */
    public void assign (Slice o)
    {
        min.assign (o.min);
        max.assign (o.max);
        min_set = o.min_set;
        max_set = o.max_set;
    }

    public void set_direction_view (WormDirection d, int[,] board)
    {
        /*
         * For the code to work the ratio x / y should be > 0° and <= 45°
         * 369665159/-14116942878 is approximately 1½° (Math.atan2 (x,-y) / Math.PI * 180))
         */
        const int64 x = 369665159;
        const int64 y = -14116942878;
        switch (d)
        {
            case EAST:
                /*
                 *      .
                 *     ..
                 *    ...
                 *   ....
                 *  o....
                 *   ....
                 *    ...
                 *     ..
                 *      .
                 */
                min = {-y, -x}; // mirror on 0° line (x axis) and rotate +90°
                max = {-y, +x}; // rotate +90°
                break;
            case SOUTH:
                /*
                 *      o
                 *     ...
                 *    .....
                 *   .......
                 *  .........
                 */
                min = {+x, -y}; // mirror on 0° line (x axis) and rotate 180°
                max = {-x, -y}; // rotate 180°
                break;
            case WEST:
                /*
                 *  .
                 *  ..
                 *  ...
                 *  ....
                 *  ....o
                 *  ....
                 *  ...
                 *  ..
                 *  .
                 */
                min = {+y, +x}; // mirror on 0° line (x axis) and rotate -90°
                max = {+y, -x}; // rotate -90°
                break;
            case NORTH:
                /*
                 *  .........
                 *   .......
                 *    .....
                 *     ...
                 *      o
                 */
                min = {-x, +y}; // mirror on 0° line (x axis)
                max = {+x, +y};
                break;
            default:
                break;
        }
        min_set = true;
        max_set = true;
        min.set_wrapping (board.length[0], board.length[1]);
        max.set_wrapping (board.length[0], board.length[1]);
    }

    public void add_angle (Angle a)
    {
        if (!min_set && !max_set)
        {
            min.assign (a);
            min_set = true;
        }
        else if (!min_set)
        {
            if (a.greater_than (max))
            {
                min.assign (max);
                max.assign (a);
            }
            else
                min.assign (a);
            min_set = true;
        }
        else if (!max_set)
        {
            if (a.greater_than (min))
                max.assign (a);
            else
            {
                max.assign (min);
                min.assign (a);
            }
            max_set = true;
        }
        else // min_set && max_set
        {
            if (a.less_than (min))
                min.assign (a);
            else if (a.greater_than (max))
                max.assign (a);
        }
    }

    public void set_from_position (Position origin, int64 x, int64 y, int size)
    {
        /*
         * Set this slice to be the slice created by an object at x,y. When
         * viewed from the origin.
         * For normal objects the size is 1 for bonuses it is 2.
         */

        min_set = false;
        max_set = false;

        /*
         * Example for a bonus.
         *
         *
         *       BB
         *      /BB
         *     / /
         *    //
         *   O
         *
         */

        // our origin is in the centre of position e.g. x + 0.5, y + 0.5
        add_angle ({x * 2 - (origin.x * 2 + 1),  y * 2 - (origin.y * 2 + 1)});
        add_angle ({ (x + 1 * size)* 2  - (origin.x * 2 + 1), y * 2 - (origin.y * 2 + 1)});
        add_angle ({x * 2 - (origin.x * 2 + 1), (y + 1 * size)* 2 - (origin.y * 2 + 1)});
        add_angle ({ (x + 1 * size)* 2 - (origin.x * 2 + 1), (y + 1 * size)* 2 - (origin.y * 2 + 1)});
    }

    public void intersection_by_position (Position origin, uint8 _x, uint8 _y, int size)
    {
        /* Take the slice created by an object at x,y when viewed
         * from the origin and set this to be the overlap with between
         * the created slice and this slice.
         */

        int64 x = _x;
        int64 y = _y;
        if (min.x >= 0 && max.x >= 0)
        {
            if (!(x + (size -1) > origin.x))
                x += min.x_max;
        }
        else if (min.x < 0 && max.x < 0)
        {
            if (!(x < origin.x))
                x -= min.x_max;
        }
        else if (min.y >= 0 && max.y >= 0)
        {
            if (!(y + (size -1) > origin.y))
                y += min.y_max;
        }
        else if (min.y < 0 && max.y < 0)
        {
            if (!(y < origin.y))
                y -= min.y_max;
        }
        Angle old_min = {};
        old_min.assign (min);
        Angle old_max = {};
        old_max.assign (max);
        set_from_position (origin, x, y, size);
        if (old_min.greater_than (min))
            min.assign (old_min);
        if (old_max.less_than (max))
            max.assign (old_max);
    }

    public bool is_empty ()
    {
        /* Return true if the slice is empty (covers 0°). */
        return !min_set || !max_set || min.greater_than_or_equal_to (max);
    }

    bool is_bonus_at (uint8 x, uint8 y, Bonus b)
    {
        return b.x == x && b.y == y
            || b.x + 1 == x && b.y == y
            || b.x == x && b.y + 1 == y
            || b.x + 1 == x && b.y + 1 == y;
    }

    bool is_position_occupied (Position p, int[,] board, WormMap worm_map)
    {
        return board[p.x, p.y] > NibblesGame.EMPTYCHAR || worm_map.contain_position (p);
    }

    public int64 is_visible (Position origin, int[,] board, WormMap worm_map, Bonus bonus)
    {
        /*
         * Return the distance to a bonus if it is possible to see
         * the bonus. Otherwise return int64.MAX.
         */

        /* remember the positions we have already checked in this array */
        var checked_positions = new Gee.ArrayList<uint16> ();

        /* follow the min line, looking for a bonus or a blockage (e.g. wall) */
        for (;!is_empty ();)
        {
            int64 distance = 0;
            min.set_origin (origin);
            min.set_wrapping (board.length[0], board.length[1]);
            foreach (SignedPosition p in min)
            {
                distance++;
                if (!checked_positions.contains (p.wrap_xy ()))
                {
                    if (is_bonus_at (p.wrap_x (), p.wrap_y (), bonus))
                    {
                        return min.step_along_x () ? distance + (origin.y - p.y).abs ()
                         : distance + (origin.x - p.x).abs ();
                    }
                    else if (distance > (min.step_along_x () ? board.length[0] : board.length[1]) * 2
                      || is_position_occupied ({p.wrap_x (), p.wrap_y ()}, board, worm_map))
                    {
                        checked_positions.add (p.wrap_xy ());
                        /* subtract the slice of the blocked position */
                        Slice s = new Slice ();
                        s.set_from_position (origin, p.x, p.y, 1);
                        min.assign (s.max);
                        break;
                    }
                }
            }
        }
        return int64.MAX;
    }
}

private class WormProperties : Object
{
    internal int color     { internal get; internal set; }
    internal uint up       { internal get; internal set; }
    internal uint down     { internal get; internal set; }
    internal uint left     { internal get; internal set; }
    internal uint right    { internal get; internal set; }
    internal int raw_up    { internal get; internal set; }
    internal int raw_down  { internal get; internal set; }
    internal int raw_left  { internal get; internal set; }
    internal int raw_right { internal get; internal set; }
}

internal class WormPositions : Gee.LinkedList<uint16>
{
    public bool append_position (Position p)
    {
        return add (((uint16)p.x) << 8 | p.y);
    }
    public Position get_head ()
    {
        var head = first ();
        return { (uint8)(head >> 8), (uint8)head};
    }
    public void set_head (Position p)
    {
        @set (0, (((uint16)p.x) << 8 | p.y));
    }
    public bool prepend_position (Position p)
    {
        if (size > 0)
            return offer_head (((uint16)p.x) << 8 | p.y);
        else
            return append_position (p);
    }
    public Position remove_tail ()
    {
        var tail = poll_tail ();
        return { (uint8)(tail >> 8), (uint8)tail};
    }
}

/*
 * A simple quick array that stores double bits.
 */
#if !TEST_COMPILE
    #if GENERIC_TYPE_BUG
    class DoubleBitArray
    #else
    class DoubleBitArray <T>
    #endif
    {
        const ulong BITS_MASK = 0x3; /* 2 bits set */
    #if GENERIC_TYPE_BUG
        ulong array;
        internal const ulong size = sizeof (ulong) * 4;
        internal ulong get_at (ulong index)
            requires (index < size)
        {
            return (array >> (2 * index)) & BITS_MASK;
        }
        internal void set_at (ulong index, ulong l)
            requires (index < size)
        {
            array &= ~(BITS_MASK << (2 * index));
            array |= (l & BITS_MASK) << (2 * index);
        }
    #else
        T array;
        internal ulong size = sizeof (T) * 4;
        internal T get_at (ulong index)
            requires (index < size)
        {
            return (array >> (2 * index)) & BITS_MASK;
        }
        internal void set_at (ulong index, T l)
            requires (index < size)
        {
            array &= ~(BITS_MASK << (2 * index));
            array |= (l & BITS_MASK) << (2 * index);
        }
    #endif
    }
#endif

private class Worm : Object
{
    private const int STARTING_LENGTH = 5; /* STARTING_LENGTH must be greater than 0 */
    internal const uint8 STARTING_LIVES = 6;
    internal const uint8 MAX_LIVES = 12;

    internal const int GROW_FACTOR = 4;

    internal Position starting_position { internal get; private set; }

    public int id { internal get; protected construct; }

    internal bool is_human;
    internal int rounds_to_stay_still;
    internal bool is_stopped = false;
    private int rounds_to_stay_dematerialized;
    internal bool is_materialized { internal get {return rounds_to_stay_dematerialized <= 0;}}

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
            Position head = list.get_head ();
            return head;
        }
        private set
        {
            list.set_head (value);
        }
    }

    internal WormDirection direction { internal get; private set; }
    internal Position warp_position;
    internal bool warp_bonus;

    private WormDirection starting_direction;
#if !TEST_COMPILE
/*
 * A queue that allows no adjacent duplicates.
 */
#if GENERIC_TYPE_BUG
    class KeyQueue : DoubleBitArray
#else
    class KeyQueue : DoubleBitArray <ulong>
#endif
    {
        ulong head = 0; /* head points to the next to leave the queue */
        ulong tail = 0; /* tail points to the next slot to join the queue and is always an empty slot */
        WormDirection convert_to_direction (ulong l) {return l + 1;}
        ulong convert_from_direction (WormDirection dir) {return dir - 1;}
        ulong peek_tail ()
            requires (!is_empty ())
        {
            return get_at ((tail > 0 ? tail : size) - 1);
        }
        ulong peek_head ()
            requires (!is_empty ())
        {
            return get_at (head);
        }
        bool is_full ()
        {
            /* the queue is full when there is one slot left, that last slot is never used */
            return tail == (head > 0 ? head : size) - 1;
        }
        void join_queue (ulong d)
        {
            /* join to the end of the queue */
            set_at (tail++, d);
            if (tail >= size)
                tail = 0;
        }
        public void append (WormDirection _direction)
            requires (_direction != NONE)
        {
            /* if _direction is a duplicate or the queue is full don't append */
            var d = convert_from_direction (_direction);
            if (is_empty () || peek_tail () != d && !is_full ())
                join_queue (d);
        }
        public void prepend (WormDirection _direction)
            requires (_direction != NONE)
        {
            /* if _direction is a duplicate or the queue is full don't prepend */
            var d = convert_from_direction (_direction);
            if (is_empty ())
                join_queue (d);
            else if (peek_head () != d && !is_full ())
            {
                if (head > 0)
                    head--;
                else
                    head = size - 1;
                set_at (head, d); /* push in at the front of the queue */
            }
        }
        public void clear ()
        {
            head = tail;
        }
        public bool is_empty ()
        {
            return head == tail;
        }
        public WormDirection remove ()
        {
            /* leave the front of the queue */
            var r = convert_to_direction (peek_head ());
            if (head < size - 1)
                head++;
            else
                head = 0;
            return r;
        }
    }
    private KeyQueue key_queue = new KeyQueue ();
#endif
    internal WormPositions list = new WormPositions ();
    private Gee.ArrayList<uint16> bonus_eaten = new Gee.ArrayList<uint16> ();

    /* connected to nibbles-game */
    internal signal void bonus_found ();

    /* delegates to nibbles-game */
    internal delegate Gee.List<Worm> GetOtherWormsType (Worm self);
    GetOtherWormsType get_other_worms;
    internal delegate Gee.List<Bonus> GetBonusesType ();
    GetBonusesType get_bonuses;

    public uint8 width  { private get; protected construct; }
    public uint8 height { private get; protected construct; }
    public int capacity { private get; protected construct; }

    construct
    {
        deadend_board = new uint [width, height];
    }

    internal Worm (int id, uint8 width, uint8 height, GetOtherWormsType cb0, GetBonusesType cb1)
    {
        int capacity = width * height;
        Object (id: id, width: width, height: height, capacity: capacity);
        get_other_worms = (worm)=> {return cb0 (worm);};
        get_bonuses = ()=> {return cb1 ();};
    }

    internal void set_start (uint8 x, uint8 y, WormDirection direction)
    {
        list.clear ();

        bonus_eaten.clear (); /* forget all the bonuses we have eaten */

        starting_position = {x, y};
        list.append_position (starting_position);

        starting_direction = direction;
        this.direction     = direction;
        change = 0;
#if !TEST_COMPILE
        key_queue.clear ();
#endif
    }

    internal void human_move (int[,] board, Gee.LinkedList<Worm> worms)
    {
#if !TEST_COMPILE
        if (!key_queue.is_empty ())
            dequeue_keypress (board, worms); /* change the worm's direction */
#endif
    }

    internal void move_part_1 (int[,] board)
    {
        Position position = head;
        position.move (direction, width, height);

        /* Add a new body piece to the head of the list. */
        list.prepend_position (position);
    }

    internal void move_part_2 (int[,] board, Position? head_position)
    {
        if (head_position != null)
            head = Position () { x = head_position.x, y = head_position.y };
        if (change > 0)
        {
            /* Add to the worm's size. */
            change--;
        }
        else
        {
            /* Remove a body piece from the tail of the list. */
            assert (list.size > 0);
            remove_bonus_eaten_position (list.remove_tail ());
        }
        /* Check for bonus, do nothing if there isn't a bonus */
        bonus_found (); /* signal function in nibble-game.vala */
        /* If we are dematerialized reduce the rounds dematerialized by one. */
        if (rounds_to_stay_dematerialized > 1)
            rounds_to_stay_dematerialized -= 1;
        /* Try and dematerialize if our rounds are up. */
        if (rounds_to_stay_dematerialized == 1)
            materialize (board);
    }

    internal void remove_bonus_eaten_position (Position p)
    {
        if (bonus_eaten.size > 0)
        {
            uint16 a = ((uint16)p.x) << 8 | p.y;
            if (bonus_eaten.contains (a))
                bonus_eaten.remove (a);
        }
    }

    /* This function is only called from nibbles-game.vala */
    internal void add_bonus_eaten_position (uint8 x, uint8 y)
    {
        /* add new position */
        bonus_eaten.add (((uint16)x) << 8 | y);
    }
#if !TEST_COMPILE
    internal bool was_bonus_eaten_at_this_position (uint16 position)
    {
        return bonus_eaten.contains (position);
    }
#endif
    /* This function is only called from nibbles-game.vala */
    internal void reduce_tail (int erase_size)
    {
        if (erase_size > 0)
        {
            for (int i = 0; i < erase_size; i++)
            {
                /* Remove a body piece from the tail of the list. */
                assert (list.size > 0);
                remove_bonus_eaten_position (list.remove_tail ());
            }
        }
    }

    internal void reverse ()
    {
        if (!is_stopped && !list.is_empty)
        {
            var reversed_list = new WormPositions ();
            foreach (var pos in list)
            {
                if (reversed_list.size > 0)
                    reversed_list.offer_head (pos);
                else
                    reversed_list.add (pos);
            }
            list = reversed_list;

            /* Set new direction as the opposite direction of the last two tail pieces */
            if (((uint8)list[0]) == ((uint8)list[1]))
                direction = ((list[0]>>8) > (list[1]>>8)) ? WormDirection.RIGHT : WormDirection.LEFT;
            else
                direction = (((uint8)list[0]) > ((uint8)list[1])) ? WormDirection.DOWN : WormDirection.UP;
        }
    }

    private static bool does_list_contain_position (WormPositions position_list, uint16 position)
    {
        if (position_list != null)
            foreach (var p in position_list)
                if (p == position)
                    return true;
        return false;
    }

    internal bool is_position_clear_of_materialized_worms (Gee.List<Worm> worms, Position position)
    {
        foreach (Worm worm in worms)
            if (worm.is_materialized && does_list_contain_position (worm.list, ((uint16)position.x)<<8 | position.y))
                return false;
        return true;
    }

    static bool is_board_position_occupied (Position p, int[,] board)
    {
        return board[p.x, p.y] > NibblesGame.EMPTYCHAR;
    }

    internal bool can_move_to (int[,] board, Gee.List<Worm> worms, Position position)
    {
        if (is_board_position_occupied (position, board))
            return false;
        else if (!is_position_clear_of_materialized_worms (worms,position))
            return !is_materialized;
        else
            return true;
    }

    internal bool can_move_to_map (int[,] board, WormMap worm_map, Position position)
    {
        if (is_board_position_occupied (position, board))
            return false;
        else if (worm_map.contain_position (position))
            return !is_materialized;
        else
            return true;
    }

    internal bool can_move_direction (int[,] board, Gee.List<Worm> worms, WormDirection direction)
    {
        Position position = list.get_head (); /* head position */
        position.move (direction,width,height);
        return can_move_to (board,worms,position);
    }

    internal void spawn (int[,] board)
    {
        assert (STARTING_LENGTH > 0);
        change = STARTING_LENGTH - 1;
        rounds_to_stay_dematerialized = STARTING_LENGTH;
        for (int i = 0; i < STARTING_LENGTH; i++)
        {
            move_part_1 (board);
            move_part_2 (board, /* no warp */ null);
        }
    }

    private void materialize (int[,] board)
    {
        /*
         * A worm can only materialise if it is not crossing another worm and
         * the next 12 locations in front of it don’t contain a materialised
         * worm. Stop checking the locations for materialised worms if we find
         * an obstacle on the board.
         */
        WormMap worm_map = new WormMap (get_other_worms (this), width, height);
        if (worm_map.contains (list))
        {
            rounds_to_stay_dematerialized += 1; /* wait until to next round to try to materialise */
            return;
        }
        Position position = head;
        for (int i = 12; i > 0 ; i--)
        {
            position = get_position_after_direction_move (position, direction);
            if (is_board_position_occupied (position, board))
            {
                rounds_to_stay_dematerialized = 0; /* materialise now */
                return;
            }
            if (worm_map.contain_position (position))
            {
                rounds_to_stay_dematerialized += 1; /* wait until to next round to try to materialise */
                return;
            }
        }
        rounds_to_stay_dematerialized = 0; /* materialise now */
    }

    internal void dematerialize (int rounds, int gamedelay)
    {
        /* rounds_to_stay_dematerialized must be greater than 0 to dematerialize */
        rounds_to_stay_dematerialized = rounds > 1 ? rounds : 1;

        rounds_to_stay_still = 2;
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
#if !TEST_COMPILE
        key_queue.clear ();
#endif
        lose_life ();

        list.clear ();

        bonus_eaten.clear (); /* forget all the bonuses we have eaten */

        if (lives > 0)
        {
            list.append_position (starting_position);

            direction = starting_direction;
            spawn (board);

            dematerialize (/* number of rounds */ 3, 35);
        }
    }

    internal Position position_move ()
    {
        Position position = {head.x, head.y};
        position.move (direction, width, height);
        return position;
    }

    internal Position get_position_after_direction_move (Position origin, WormDirection direction)
    {
        Position position = {origin.x, origin.y};
        position.move (direction, width, height);
        return position;
    }
    /*\
    * * Keys and key presses
    \*/
#if !TEST_COMPILE
    private bool LastUturnA = false;

    private WormDirection uturn (int [,] board, Gee.List<Worm> worms, WormDirection direction)
    {
        /* player has reversed direction */
        Position tmp;
        WormDirection dirA,dirB;
        int length_posA,length_posB;
        length_posA=0; length_posB=0;
        if (direction==WormDirection.DOWN || direction==WormDirection.UP)
        {
            /* calculate space when we step to the left */
            tmp = {list[0] >> 8, (uint8)list[0]};
            dirA = WormDirection.LEFT;
            tmp.move (dirA,width,height);
            for (length_posA=0; length_posA<height && can_move_to (board,worms,tmp); length_posA++,tmp.move (direction, width, height));
            /* calculate space when we step to the right */
            tmp = {list[0] >> 8, (uint8)list[0]};
            dirB = WormDirection.RIGHT;
            tmp.move (dirB,width,height);
            for (length_posB=0; length_posB<height && can_move_to (board,worms,tmp); length_posB++,tmp.move (direction, width, height));
        }
        else /* direction==WormDirection.LEFT || direction==WormDirection.RIGHT */
        {
            /* calculate space when we step up */
            tmp = {list[0] >> 8, (uint8)list[0]};
            dirA = WormDirection.UP;
            tmp.move (dirA,width,height);
            for (length_posA=0; length_posA<width && can_move_to (board,worms,tmp); length_posA++,tmp.move (direction, width, height));
            /* calculate space when we step down */
            tmp = {list[0] >> 8, (uint8)list[0]};
            dirB = WormDirection.DOWN;
            tmp.move (dirB,width,height);
            for (length_posB=0; length_posB<width && can_move_to (board,worms,tmp); length_posB++,tmp.move (direction, width, height));
        }
        if (length_posA > length_posB)
        {
            LastUturnA=true;
            return dirA;
        }
        else if (length_posA < length_posB)
        {
            LastUturnA=false;
            return dirB;
        }
        else if (length_posA > 0 /*|| length_posB > 0*/)
        {
            if (LastUturnA)
                return dirA;
            else
                return dirB;
        }
        else /* length_posA==0 && length_posB==0 */
            return direction.reverse ();
    }

    private int get_raw_key (uint keyval)
    {
        Gdk.KeymapKey[] keys;
        if (Gdk.Display.get_default ().map_keyval (keyval, out keys))
        {
            if (keys.length > 0)
                return (int)keys[0].keycode;
        }
        return -1;
    }

    internal bool handle_keypress (uint keycode, Gee.HashMap<Worm, WormProperties> worm_props)
    {
        if (lives == 0 || is_stopped || list.is_empty)
            return false;
        else
        {
            WormProperties properties = worm_props.@get (this);
            if (properties.raw_up < 0)
                properties.raw_up = get_raw_key (properties.up);
            if (keycode == properties.raw_up)
            {
                queue_keypress (UP);
                return true;
            }
            if (properties.raw_down < 0)
                properties.raw_down = get_raw_key (properties.down);
            if (keycode == properties.raw_down)
            {
                queue_keypress (DOWN);
                return true;
            }
            if (properties.raw_left < 0)
                properties.raw_left = get_raw_key (properties.left);
            if (keycode == properties.raw_left)
            {
                queue_keypress (LEFT);
                return true;
            }
            if (properties.raw_right < 0)
                properties.raw_right = get_raw_key (properties.right);
            if (keycode == properties.raw_right)
            {
                queue_keypress (RIGHT);
                return true;
            }
            return false;
        }
    }

    private void queue_keypress (WormDirection dir)
    {
        key_queue.append (dir);
    }

    private void dequeue_keypress (int [,] board, Gee.List<Worm> worms)
        requires (!key_queue.is_empty ())
    {
        switch (key_queue.remove ())
        {
            case UP:
                if (direction == DOWN)
                {
                    direction = uturn (board,worms,UP);
                    if (direction != UP && direction != DOWN)
                        key_queue.prepend (UP);
                }
                else if (can_move_direction (board,worms,UP))
                    direction = UP;
                break;
            case DOWN:
                if (direction == UP)
                {
                    direction = uturn (board,worms,DOWN);
                    if (direction != DOWN && direction != UP)
                        key_queue.prepend (DOWN);
                }
                else if (can_move_direction (board,worms,DOWN))
                    direction = DOWN;
                break;
            case LEFT:
                if (direction == RIGHT)
                {
                    direction = uturn (board,worms,LEFT);
                    if (direction != LEFT && direction != RIGHT)
                        key_queue.prepend (LEFT);
                }
                else if (can_move_direction (board,worms,LEFT))
                    direction = LEFT;
                break;
            case RIGHT:
                if (direction == LEFT)
                {
                    direction = uturn (board,worms,RIGHT);
                    if (direction != RIGHT && direction != LEFT)
                        key_queue.prepend (RIGHT);
                }
                else if (can_move_direction (board,worms,RIGHT))
                    direction = RIGHT;
                break;
            default: /* NONE */
                break;
        }
    }
#endif
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
     * appear empty).
     */
    private static uint[,] deadend_board; /* must be an unsigned integer of any size*/
    private static uint deadend_runnumber = 0; /* must be an unsigned integer of the same size as above */

    private static int ai_deadend (int[,] board, WormMap worm_map, Position position, int length)
    {
        Position [] p = {position};
        for (uint i = 0; p.length > i && (p.length - 1) < length; i++)
        {
            for (uint dir = 4; dir > 0 && (p.length - 1) < length; dir--)
            {
                Position new_position = p[i];
                new_position.move ((WormDirection) dir, (uint8) board.length [0], (uint8) board.length [1]);
                if (deadend_board [new_position.x, new_position.y] != deadend_runnumber
                    && !is_board_position_occupied (new_position, board)
                    && !worm_map.contain_position (new_position))
                {
                    deadend_board [new_position.x, new_position.y] = deadend_runnumber;
                    p+=new_position;
                }
            }
        }
        return p.length - 1 > length ? 0 : length - (p.length - 1);
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
    internal static int ai_deadend_after (int[,] board, Gee.LinkedList<Worm> worms, WormMap worm_map, Position old_position, WormDirection direction, int length)
    {
        uint8 width  = (uint8) /* int */ board.length [0];
        uint8 height = (uint8) /* int */ board.length [1];

        if (++deadend_runnumber == 0)
        {
            for (int x = 0; x < deadend_board.length[0]; x++)
                for (int y = 0; y < deadend_board.length[1]; y++)
                    deadend_board [x, y] = deadend_runnumber;
            deadend_runnumber++;
        }

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

        return ai_deadend (board, worm_map, new_position, cl);
    }

    /* Check to see if another worm's head is too close in front of us;
     * that is, that it's within 3 in the direction we're going and within
     * 1 to the side.
     */
    private inline bool ai_too_close (Gee.LinkedList<Worm> worms, WormDirection direction)
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

    internal bool ai_is_bonus_more_attractive (Bonus.eType b0, int64 d0, Bonus.eType b1, int64 d1)
    {
        /* A LIFE bonus is a more attractive bonus than any other bonus. */
        return (b0 == LIFE && b1 == LIFE
            || b0 != LIFE && b1 != LIFE) && d0 < d1
            || b0 == LIFE && b1 != LIFE;
    }

    internal bool ai_can_see_bonus (int[,] board, Position origin, Bonus bonus, WormDirection direction)
    {
        if (origin.x == bonus.x && origin.y == bonus.y
            || origin.x == bonus.x + 1 && origin.y == bonus.y
            || origin.x == bonus.x && origin.y == bonus.y + 1
            || origin.x == bonus.x + 1 && origin.y == bonus.y + 1)
            return true;
        Slice slice = new Slice ();
        /* our initial view is set by our direction */
        slice.set_direction_view (direction, board);
        /* narrow our view to this bonus (or set an empty view if the bonus is not within our view) */
        slice.intersection_by_position (origin, bonus.x, bonus.y, 2 /* always 2 for bonus */);
        return !slice.is_empty ();
    }

    internal int64 ai_count_distance_to_a_bonus_in_direction (int[,] board, WormMap worm_map,
         Position origin, WormDirection direction, out Bonus.eType bonus_type)
    {
        /*
         * Return the distance to a bonus if it is possible to head in
         * this direction to find a bonus. Otherwise return int64.MAX.
         */

        /*
         * Look for a bonus in direction.
         *
         *       .
         *     . .
         *   . . .
         * o . . .
         *   . b b
         *     b b
         *       .
         *
         */

        int64 bonus_distance = int64.MAX;
        bonus_type = (Bonus.eType)(-1);

        Slice slice = new Slice ();
        foreach (Bonus b in get_bonuses ())
        {
            if (bonus_type == LIFE && b.etype == LIFE ||
                bonus_type != LIFE && (
                    b.etype == REGULAR || b.etype == DOUBLE
                    || b.etype == LIFE || b.etype == REVERSE))
            {
                /* our initial view is set by our direction */
                slice.set_direction_view (direction, board);
                /* narrow our view to this bonus (or set an empty view if the bonus is not within our view) */
                slice.intersection_by_position (origin, b.x, b.y, 2 /* always 2 for bonus */);
                if (!slice.is_empty ())
                {
                    /* we have found a bonus within our field of view, check that nothing is in the way */
                    int64 distance = slice.is_visible (origin, board, worm_map, b);
                    /* If the bonus is visible, its nearer than previous bonuses and it can still be see
                       if we move in this direction choose it. */
                    if (distance < bonus_distance &&
                        ai_can_see_bonus (board, get_position_after_direction_move (origin, direction), b, direction))
                    {
                        bonus_distance = distance;
                        bonus_type = b.etype;
                    }
                }
            }
        }

        return bonus_distance;
    }

    /* Determines the direction of the AI worm. */
    internal void ai_move (int[,] board, Gee.LinkedList<Worm> worms)
    {
        WormMap worm_map = new WormMap (worms, width, height);

        /* We have a look in all directions except behind us for a bonus. */
        Bonus.eType bonus_type;
        int64 shortest_distance = int64.MAX;
        WormDirection shortest_dir = direction;
        Bonus.eType shortest_bonus_type = (Bonus.eType)(-1);

        WormDirection dir[] = {direction, direction.turn_left (), direction.turn_right ()};
        foreach (WormDirection direction in dir)
        {
            int64 d = ai_count_distance_to_a_bonus_in_direction (board, worm_map, head, direction, out bonus_type);
            if (ai_is_bonus_more_attractive (bonus_type, d, shortest_bonus_type, shortest_distance)
                && can_move_direction (board, worms, direction))
            {
                shortest_distance = d;
                shortest_dir = direction;
                shortest_bonus_type = bonus_type;
            }
        }

        if (shortest_distance >= int64.MAX)
        {
            // check next step positions
            WormDirection start_direction[] = {direction, direction, direction.turn_right (), direction.turn_left ()};
            WormDirection look_direction[]  = {direction.turn_left (), direction.turn_right (), direction, direction};
            int64 d;
            for (int i = 0; i < start_direction.length; i++)
            {
                d = ai_count_distance_to_a_bonus_in_direction (board, worm_map,
                    get_position_after_direction_move (head, start_direction[i]),  look_direction[i], out bonus_type);
                if (ai_is_bonus_more_attractive (bonus_type, d, shortest_bonus_type, shortest_distance))
                {
                    shortest_distance = d + 1; /* +1 for the step we have already taken in our logic */
                    shortest_dir = start_direction[i];
                    shortest_bonus_type = bonus_type;
                }
            }
        }

        bonus_type = shortest_bonus_type;
        if (shortest_distance >= int64.MAX)
        {
            // no bonus is visible, one in thirty chance of turning left or right
            if (Random.int_range (0, 30) == 1)
                shortest_dir = Random.boolean () ? direction.turn_right () : direction.turn_left ();
        }

        /* Avoid walls, dead-ends and other worm's heads. This is done using
         * an evaluation function which is CAPACITY for a wall, 4 if another
         * worm's head is in the too close area, 4 if another worm's head
         * could move to the same location as ours, plus 0 if there's no
         * dead-end, or the amount that doesn't fit for a deadend. olddir's
         * score is reduced by 100, to favour it, but only if its score is 0
         * otherwise; this is so that if we're currently trapped in a dead
         * end, the worm will move in a space-filling manner in the hope
         * that the dead end will disappear (e.g. if it's made from the tail
         * of some worm, as often happens).
         */
        WormDirection bonus_dir = shortest_dir;
        WormDirection best_dir = NONE;
        int64 best_yet = int64.MAX;
        foreach (WormDirection direction in dir[0].get_space_fill_array ())
        {
            int this_len = 0;
            /* if we are heading for a LIFE bonus don't worry about being trapped */
            if (!(direction == bonus_dir && bonus_type == LIFE))
            {
#if TEST_COMPILE
                assert (can_move_to (board, worms, get_position_after_direction_move (head, direction)) ==
                    can_move_to_map (board, worm_map, get_position_after_direction_move (head, direction)));
#endif
                if (!can_move_to_map (board, worm_map, get_position_after_direction_move (head, direction)))
                    this_len += capacity;

                if (ai_too_close (worms, direction))
                    this_len += 4;

                this_len += ai_deadend_after (board, worms, worm_map, head, direction, length + change);;
            }
            if (direction == bonus_dir && this_len <= 0)
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

        // set the class variable direction to our desired direction */
        direction = best_dir;
    }
}

