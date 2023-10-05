/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Gnome Nibbles: Gnome Worm Game
 * Copyright (C) 2015 Iulian-Gabriel Radu <iulian.radu67@gmail.com>
 * Copyright (C) 2023 Ben Corby <bcorby@new-ms.com>
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

/* designed for Gtk 4, link with libgtk-4-dev or gtk4-devel */
using Gtk;
using Cairo;

internal delegate void YesNoResultFunction (uint yes_no);
internal delegate bool NewGameDialogueActiveFunction (out YesNoResultFunction function);

/* worm colors */
static void get_worm_rgb (int color, bool bright, out double r, out double g, out double b)
{
    switch (color)
    {
        case 0: /* red */
            r = bright ? 1 : 0.75;
            g =  0;
            b =  0;
            break;
        case 1: /* green */
            r = 0;
            g =  bright ? 0.75 : 0.5;
            b =  0;
            break;
        case 2: /* blue */
            r = 0;
            g = bright ? 0.5 : 0.25;
            b = bright ? 1 : 0.75;
            break;
        case 3: /* yellow */
            r = bright ? 0.9 : 0.75;  
            g = bright ? 0.9 : 0.75;
            b = 0;
            break;
        case 4: /* cyan */
            r = 0;
            g = bright ? 1 : 0.75;
            b = bright ? 1 : 0.75;
            break;
        case 5: /* magenta */
            r = bright ? 0.75 : 0.5;
            g = 0;
            b = bright ? 0.75 : 0.5;
            break;
        default:
            r = bright ? 1 : 0.75;
            g = bright ? 1 : 0.75;
            b = bright ? 1 : 0.75;
            break;            
    }
}

static void get_worm_pango_color (int color, bool bright, ref Pango.Color c)
{
    double r, g, b;
    get_worm_rgb (color, bright, out r, out g, out b);
    c.red =   (uint16)(r * 0xffff);
    c.green = (uint16)(g * 0xffff);
    c.blue =  (uint16)(b * 0xffff);
}

internal class NibblesView : DrawingArea
{
    /* classes */
    struct Point3D
    {
        double x;
        double y;
        double z;
    }
    class View3D
    {
        /*
         *                          ' +       view plain y = 3
         *                          ' |               |              * view point (1.5, 4.5, 4)
         *                          ' |              /|           +
         *                          ' z             / |        + 
         *                          '              /  |     +             (3, 0) *  y
         *                (3, 0, 0) *-------------/----  +                      /|  |
         *                         /     /     / /   /+                        / |  |
         *                        /     /     / /  +/                         /  |  +
         *           3D space    /     /     / /+  /                         / ---  
         *                      -------------+/----                         /   /
         *                     /     /    +/ /   /                         /   / 
         *               +    /     /  O  / /   /                         /   /   2D view plain 
         *              /    /     /     / /   /                         /  --
         *             /    --------------|----                         /   /
         *            x    /     /     /  |  /                         /   /    +
         *                /     /     /   | /                         /   /    /
         *               /     /     /    |/                  (0, 0) *  --    /
         *              *------------------                          |  /    x
         *         (0, 0, 0)                                         | /
         *                    y --- +                                |/
         *                                                         ---
         *
         *  The object O at (1.5, 1.5, 0) when viewed from the view point (1.5, 4.5, 4) 
         *  crosses the view plain (y = 3) at the 2 dimensional point (1.5, 1.5).
         *
         *  Example code:
         *    View3D test = new View3D ();
         *    test.set_view_plain (3);    
         *    test.set_view_point ({1.5, 4.5, 4});
         *    double test_x,test_y;
         *    test.to_view_plain ({1.5, 1.5, 0}, out test_x, out test_y);
         *    stdout.printf ("The 3D object at %f, %f, %f is viewed on the 2D plain at %f, %f\n", 1.5, 1.5, 0.0, test_x, test_y);
         *
         */

        /* view plain */
        double view_plain_y;
        bool has_view_plain_been_set = false;
        /* height of view plain */
        double view_plain_z_height;
        bool has_view_plain_z_height_been_set = false;
        /* 3D view point */
        Point3D view_point;
        /* scaling */
        double x_scale = 1;
        double y_scale = 1;

        internal void set_view_plain (double y)
        {
            view_plain_y = y;
            has_view_plain_been_set = true;
        }
#if no_compile
        internal void set_view_plain_z_height (double height)
        {
            view_plain_z_height = height;
            has_view_plain_z_height_been_set = true;
        }

        internal void set_2D_plain_height (double height)
        {
            set_view_plain_z_height (height);
        }
#endif
        internal void set_view_point (Point3D point,
                                      double max_object_height = 1 /* only used to set the view plain height */)
        {
            view_point = point;
            if (has_view_plain_been_set && !has_view_plain_z_height_been_set)
            {
                /* set default view plain height */
                view_plain_z_height = (point.z - max_object_height) * view_plain_y / point.y + max_object_height;
            }
        }

        internal void set_scale_x (double scale)
        {
            x_scale = scale;
        }
 
        internal void set_scale_y (double scale)
        {
            y_scale = scale;
        }

        internal void to_view_plain (Point3D point, out double x_at_plain, out double z_at_plain)
        {
            double base_length = Math.sqrt ((view_point.x - point.x) * (view_point.x - point.x) + (view_point.y - point.y) * (view_point.y - point.y));
            double z_length = view_point.z - point.z;
            double base_length_to_view_plain = (view_plain_y - point.y) / (view_point.y - point.y) * base_length;
            z_at_plain = (view_plain_z_height - (point.z + base_length_to_view_plain / base_length * z_length)) * y_scale;
            double x_length = view_point.x - point.x;
            x_at_plain = (point.x + base_length_to_view_plain / base_length * x_length) * x_scale;
        }
        
        internal double view_point_x ()
        {
            return view_point.x;
        }

        internal double 2D_diff (Point3D a, Point3D b)
        {
            double ax,ay,bx,by;
            to_view_plain (a, out ax, out ay);
            to_view_plain (b, out bx, out by);
            return Math.sqrt ((ax - bx) * (ax - bx) + (ay - by) * (ay - by));
        }
    }

    /* constants */
    internal const uint8 WIDTH = 92;
    internal const uint8 HEIGHT = 66;

    /* Game */
    NibblesGame game;

    /* delegate to nibbles-window */
    internal delegate int CountdownActiveFunction ();
    CountdownActiveFunction countdown_active;
    NewGameDialogueActiveFunction new_game_dialogue_active;
    YesNoResultFunction result_function;

    /* yes no buttons */
    double b0_x;
    double b0_y;
    double b0_width;
    double b0_height;
    double b1_x;
    double b1_y;
    double b1_width;
    double b1_height;
    bool mouse_pressed;
    uint mouse_button;

    /* animation */
    uint64 animate = 0;

    /* constructor */
    public NibblesView (NibblesGame game, CountdownActiveFunction countdown_active,
         NewGameDialogueActiveFunction new_game_dialogue_active)
    {
        this.game = game;
        this.countdown_active = (CountdownActiveFunction)countdown_active;
        this.new_game_dialogue_active = (NewGameDialogueActiveFunction)new_game_dialogue_active;

        // connect to signals
        this.realize.connect (()=>
        {
        });
        this.resize.connect ((/*int*/width, /*int*/height)=>
        {
        });

        // set drawing fuction
        set_draw_func ((/*DrawingArea*/ area, /*Cairo.Context*/ c, width, height)=>
        {
            if (game.three_dimensional_view)
            {
                double x2d, y2d;
                double r, g, b;

                View3D v = new View3D ();
                v.set_view_plain (HEIGHT);
                v.set_view_point ({ (double)WIDTH / 2.0, 2.0 * (double)HEIGHT, 2.0 * (double)HEIGHT});
                v.set_scale_x (width / WIDTH);
                v.set_scale_y (height / HEIGHT);

                /* black background */
                v.to_view_plain ({0, 0, 0},out x2d, out y2d);
                c.move_to (x2d,y2d);
                v.to_view_plain ({WIDTH, 0, 0},out x2d, out y2d);
                c.line_to (x2d,y2d);
                v.to_view_plain ({WIDTH, HEIGHT, 0},out x2d, out y2d);
                c.line_to (x2d,y2d);
                v.to_view_plain ({0, HEIGHT, 0},out x2d, out y2d);
                c.line_to (x2d,y2d);
                c.set_source_rgb (0,0,0);
                c.fill ();

                /* map worms */      
                Worm[] dematerialized_worms = {};
                Worm?[,] worm_at = new Worm?[WIDTH, HEIGHT];
                foreach (Worm worm in game.worms)
                    if (!worm.is_stopped)
                        if (worm.is_materialized)
                            foreach (var p in worm.list)
                                worm_at[p>>8, (uint8)p] = worm;
                        else
                            dematerialized_worms += worm;
                foreach (Worm worm in dematerialized_worms)
                    foreach (var p in worm.list)
                        if (worm_at[p>>8, (uint8)p] == null)
                            worm_at[p>>8, (uint8)p] = worm;

                /* map bonuses */
                Bonus?[,] bonus_at = new Bonus?[WIDTH, HEIGHT];
                foreach (var bonus in game.get_bonuses ())
                        bonus_at[bonus.x, bonus.y] = bonus;

                /* draw */      
                for (int y = 0; y < HEIGHT; y++)
                {
                    for (int x = 0; ; x = x < WIDTH / 2 ? WIDTH - 1 - x : WIDTH - x)
                    {
                        if (worm_at[x, y] != null)
                        {
                            get_worm_rgb (game.worm_props.@get (worm_at[x, y]).color, true, out r, out g, out b);
                            worm_at[x, y].was_bonus_eaten_at_this_position ((uint16)(x<<8 | y));
                            draw_sphere (c, v, x, y, r, g, b, worm_at[x, y].was_bonus_eaten_at_this_position ((uint16)(x<<8 | y)) ? 1.25 : 1);
                            Position head = worm_at[x, y].head;
                            if (head.x == x && head.y == y)
                            {
                                switch (worm_at[x, y].direction)
                                {
                                    case WormDirection.SOUTH:
                                        draw_eyes_front (c, v, x, y, animate % 30 / 5 == worm_at[x, y].id);
                                        break;
                                    case WormDirection.EAST:
                                        draw_eyes_right (c, v, x, y, animate % 30 / 5 == worm_at[x, y].id);
                                        break;
                                    case WormDirection.WEST:
                                        draw_eyes_left (c, v, x, y, animate % 30 / 5 == worm_at[x, y].id);
                                        break;
                                    default:
                                        break;
                                }
                            }
                        } 
                        if (game.board[x, y] >= 'b' && game.board[x, y] <= 'l') 
                        {
                            /* draw top of wall */
                            v.to_view_plain ({x, y ,1},out x2d, out y2d);
                            c.move_to (x2d,y2d);
                            v.to_view_plain ({x+1,y,1},out x2d, out y2d);
                            c.line_to (x2d,y2d);
                            v.to_view_plain ({x+1,y+1,1},out x2d, out y2d);
                            c.line_to (x2d,y2d);
                            v.to_view_plain ({x,y+1,1},out x2d, out y2d);
                            c.line_to (x2d,y2d);
                            c.set_source_rgb (0.95,0.95,0.95);
                            c.fill ();

                            /* draw wall inside */
                            if (x < v.view_point_x () && !(game.board[x + 1, y] >= 'b' && game.board[x + 1, y] <= 'l'))
                            {
                                v.to_view_plain ({x+1,y,0},out x2d, out y2d);
                                c.move_to (x2d,y2d);
                                v.to_view_plain ({x+1,y+1,0},out x2d, out y2d);
                                c.line_to (x2d,y2d);
                                v.to_view_plain ({x+1,y+1,1},out x2d, out y2d);
                                c.line_to (x2d,y2d);
                                v.to_view_plain ({x+1,y,1},out x2d, out y2d);
                                c.line_to (x2d,y2d);
                                c.set_source_rgb (0.5,0.5,0.5);
                                c.fill ();
                            }
                            else if (x > v.view_point_x () && !(game.board[x - 1, y] >= 'b' && game.board[x - 1, y] <= 'l'))
                            {
                                v.to_view_plain ({x,y,0},out x2d, out y2d);
                                c.move_to (x2d,y2d);
                                v.to_view_plain ({x,y+1,0},out x2d, out y2d);
                                c.line_to (x2d,y2d);
                                v.to_view_plain ({x,y+1,1},out x2d, out y2d);
                                c.line_to (x2d,y2d);
                                v.to_view_plain ({x,y,1},out x2d, out y2d);
                                c.line_to (x2d,y2d);
                                c.set_source_rgb (0.5,0.5,0.5);
                                c.fill ();
                            }
                            else
                            {
                                /* if we are at the view point we don't need to draw either the left side or the right side of the wall */
                            }
                            /* draw wall front */
                            v.to_view_plain ({x,y+1,0},out x2d, out y2d);
                            c.move_to (x2d,y2d);
                            v.to_view_plain ({x+1,y+1,0},out x2d, out y2d);
                            c.line_to (x2d,y2d);
                            v.to_view_plain ({x+1,y+1,1},out x2d, out y2d);
                            c.line_to (x2d,y2d);
                            v.to_view_plain ({x,y+1,1},out x2d, out y2d);
                            c.line_to (x2d,y2d);
                            c.set_source_rgb (0.75,0.75,0.75);
                            c.fill ();
                        }
                        /* warps */
                        if (game.board[x + 0, y + 0] == NibblesGame.WARPCHAR &&
                            game.board[x + 1, y + 0] == NibblesGame.WARPCHAR &&
                            game.board[x + 0, y + 1] == NibblesGame.WARPCHAR &&
                            game.board[x + 1, y + 1] == NibblesGame.WARPCHAR)
                        {
                            for (int z = 0; z < 6; z++)
                            {
                                uint64 a40 = animate % 40;
                                uint64 a20 = animate % 20;
                                uint64 a20o = (animate + 10) % 20;
                                double a10 = animate % 10;
                                double angle[4] = {
                                    a40 < 20 ?              (a20 < 10 ? a10 : 10.0 - a10) / 5.0 : 0,
                                    a40 >= 10 && a40 < 30 ? (a20o < 10 ? a10 : 10.0 - a10) / 5.0 : 0,
                                    a40 >= 20 && a40 < 40 ? (a20 < 10 ? a10 : 10.0 - a10) / 5.0 : 0,
                                    a40 >= 30 || a40 < 10 ? (a20o < 10 ? a10 : 10.0 - a10) / 5.0 : 0};
                                draw_oval (c, v,
                                     { (double)x + 0.1 * z, (double)y + 0.1 * z, angle[0]},
                                     { (double)x + 2 - 0.1 * z, (double)y + 0.1 * z, angle[1]},
                                     { (double)x + 2 - 0.1 * z, (double)y + 2 - 0.1 * z, angle[2]},
                                     { (double)x + 0.1 * z, (double)y + 2 - 0.1 * z, angle[3]});
                                c.set_source_rgb (0.0,0.0,0.6 - 0.1 * (double)z);
                                c.fill ();
                            }
                        }
                        /* bonus */
                        if (bonus_at [x, y] != null)
                            draw_3D_bonus (c, v, x, y, bonus_at [x, y]);

                        /* have we done all the x positions for this line y */
                        if (x == WIDTH / 2)
                            break;
                    }
                }

                if (countdown_active () > 0)
                {
                    /* count down */
                    string text = seconds_string (countdown_active ());
                    int text_width = (int)v.2D_diff ({WIDTH / 2 - 5, HEIGHT / 2, 0}, {WIDTH / 2 + 5, HEIGHT / 2, 0});
                    double w, h;
                    int font_size = calculate_font_size (c, text, text_width, out w, out h);
                    double center_x, center_y;
                    v.to_view_plain ({WIDTH / 2, HEIGHT / 2, 0}, out center_x, out center_y);
                    c.move_to (center_x - w / 2, center_y + h);
                    set_color (c, -1, true);
                    c.set_font_size (font_size);
                    c.show_text (text);
                    
                    /* draw name labels */
                    foreach (var worm in game.worms)
                    {
                        if (!worm.list.is_empty)
                        {
                            var color = game.worm_props.@get (worm).color;
                            if (worm.direction == WormDirection.UP || worm.direction == WormDirection.DOWN)
                            {
                                /* vertical worm */
                                int middle = worm.length / 2;
                                v.to_view_plain ({(worm.list[middle] >> 8) + 1.5, (uint8)(worm.list[middle]), 0}, out x2d, out y2d);
                                draw_text (c, (int)x2d, (int)y2d, worm_name (worm.id + 1),
                                 (int)v.2D_diff ({(worm.list[0] >> 8), (uint8)(worm.list[0]), 0},
                                             {(worm.list[worm.length - 1] >> 8), (uint8)(worm.list[worm.length - 1]) + 1, 0}),
                                 color);
                            }
                            else if (worm.direction == WormDirection.LEFT || worm.direction == WormDirection.RIGHT)
                            {
                                /* horizontal worm */
                                double x_2d[2], y_2d[2];
                                int x = worm.list[0] >> 8;
                                int x_max = worm.list[worm.length - 1] >> 8;
                                if (x > x_max)
                                {
                                    var swap = x;
                                    x = x_max;
                                    x_max = swap;
                                }
                                v.to_view_plain ({x, (uint8)(worm.list[0]), 2}, out x_2d[0], out y_2d[0]);
                                v.to_view_plain ({x_max + 1, (uint8)(worm.list[0]), 2}, out x_2d[1], out y_2d[1]);
                                draw_text (c, (int)x_2d[0], (int)y_2d[0], worm_name (worm.id + 1), (int)(x_2d[1] - x_2d[0]), color);
                            }
                        }
                    }
                }

                if (new_game_dialogue_active (out result_function))
                {
                    new_game_dialogue_draw (c, width, height, result_function);
                }
            }
            else
            {
                const double max_delta_deviation = 1.15;
                int x_delta = width / WIDTH;
                int y_delta = height / HEIGHT;
                if (x_delta > max_delta_deviation * y_delta)
                    x_delta = (int)(y_delta * max_delta_deviation);
                else if (y_delta > max_delta_deviation * x_delta)
                    y_delta = (int)(x_delta * max_delta_deviation);
                int x_offset = (width - x_delta * WIDTH) / 2;
                int y_offset = (height - y_delta * HEIGHT) / 2;

                /* black background */
                c.rectangle (x_offset, y_offset, x_delta * WIDTH, y_delta * HEIGHT);
                c.set_source_rgb (0,0,0);
                c.fill ();

                /* draw walls & warps */                        
                for (int x = 0; x < WIDTH; x++)
                {
                    for (int y = 0; y < HEIGHT; y++)
                    {
                        /* walls */
                        if (game.board[x, y] >= 'b' && game.board[x, y] <= 'l') 
                            draw_wall_segment (game.board[x, y],
                                c, x_delta * x + x_offset, y_delta * y + y_offset, x_delta, y_delta);
                        /* warps */
                        if (game.board[x + 0, y + 0] == NibblesGame.WARPCHAR &&
                            game.board[x + 1, y + 0] == NibblesGame.WARPCHAR &&
                            game.board[x + 0, y + 1] == NibblesGame.WARPCHAR &&
                            game.board[x + 1, y + 1] == NibblesGame.WARPCHAR)
                        {
                            draw_bonus (c, x_delta * x + x_offset, y_delta * y + y_offset, x_delta + x_delta, y_delta + y_delta, WARP);
                        }
                    }
                }

                /* draw materialized worms */
                var materialized_worm_positions = new Gee.ArrayList<uint16> ();
                int[] dematerialized_worms = {};
                for (int i = 0; i < game.worms.size; i++)
                {
                    if (game.worms[i].is_materialized)
                        foreach (var position in game.worms[i].list)
                        {
                            uint8 x = position >> 8;
                            uint8 y = (uint8)position;
                            draw_worm_segment (c, x_delta * x + x_offset, y_delta * y + y_offset, x_delta, y_delta, game.worm_props.@get (game.worms[i]).color, true, game.worms[i].was_bonus_eaten_at_this_position (position));
                            materialized_worm_positions.add (position);
                        }
                    else
                        dematerialized_worms += i;
                }
                /* draw dematerialized worms */
                for (int i = 0; i < dematerialized_worms.length; i++)
                {
                    foreach (var position in game.worms[i].list)
                    {
                        if (!materialized_worm_positions.contains (position))
                        {
                            uint8 x = position >> 8;
                            uint8 y = (uint8)position;
                            draw_worm_segment (c, x_delta * x + x_offset, y_delta * y + y_offset, x_delta, y_delta, game.worm_props.@get (game.worms[i]).color, false, false);
                        }
                    }
                }
                
                /* draw bonuses */
                foreach (var bonus in game.get_bonuses ())
                {
                    draw_bonus (c, x_delta * bonus.x + x_offset, y_delta * bonus.y + y_offset, x_delta + x_delta, y_delta + y_delta, bonus.bonus_type);
                }
                
                if (countdown_active () > 0)
                {
                    /* count down */
                    string text = seconds_string (countdown_active ());
                    int text_width = x_delta * 10;
                    double w, h;
                    int font_size = calculate_font_size (c, text, text_width, out w, out h);
                    c.move_to (x_offset + x_delta * (WIDTH / 2) - w / 2, y_offset + y_delta * (HEIGHT / 2) + h / 2);
                    set_color (c, -1, true);
                    c.set_font_size (font_size);
                    c.show_text (text);
                    
                    /* draw name labels */
                    foreach (var worm in game.worms)
                    {
                        if (!worm.list.is_empty)
                        {
                            var color = game.worm_props.@get (worm).color;
                            if (worm.direction == WormDirection.UP || worm.direction == WormDirection.DOWN)
                            {
                                /* vertical worm */
                                int middle = worm.length / 2;
                                draw_text (c, x_offset + x_delta * ((worm.list[middle] >> 8) + 1) + x_delta / 2,
                                              y_offset + y_delta * ((uint8)worm.list[middle] + 1),
                                              worm_name (worm.id + 1), x_delta * worm.length, color);
                            }
                            else if (worm.direction == WormDirection.LEFT || worm.direction == WormDirection.RIGHT)
                            {
                                /* horizontal worm */
                                int x = worm.list[0] >> 8;
                                if (x > worm.list[worm.length-1] >> 8)
                                    x = worm.list[worm.length-1] >> 8;
                                draw_text (c, x_offset + x_delta * x,
                                              y_offset + y_delta * ((uint8)worm.list[0]) - y_delta / 2,
                                              worm_name (worm.id + 1), x_delta * worm.length, color);
                            }
                        }
                    }
                }

                if (new_game_dialogue_active (out result_function))
                {
                    new_game_dialogue_draw (c, width, height, result_function);
                }
            }
        });
        connect_game_signals (game);
    }

    string seconds_string (int s)
    {
        switch (s)
        {
            case 1:
                // Translators: One second to go until the game starts.
                return _("1");
            case 2:
                // Translators: Two seconds to go until the game starts.
                return _("2");
            case 3:
                // Translators: Three seconds to go until the game starts.
                return _("3");
            default:
                return "";
        }
    }

    /* worm name */
    string worm_name (int id)
    {
        switch (id)
        {
            case 1:
                // Translators: the first worm's name.
                return _("Worm 1");
            case 2:
                // Translators: the seconds worm's name.
                return _("Worm 2");
            case 3:
                // Translators: the third worm's name.
                return _("Worm 3");
            case 4:
                // Translators: the fourth worm's name.
                return _("Worm 4");
            case 5:
                // Translators: the fifth worm's name.
                return _("Worm 5");
            case 6:
                // Translators: the sixth worm's name.
                return _("Worm 6");
            default:
                return "";
        };
    }

    /* redraw */
    public void redraw (bool AnimateStep = false)
    {
        if (AnimateStep)
            ++animate;
        queue_draw ();
    }
    
    /* signals */
    internal void connect_game_signals (NibblesGame game)
    {
        game.redraw.connect (redraw);
    }

    /* private functions */

    void new_game_dialogue_draw (Context c, double width, double height, YesNoResultFunction result_function)
    {
        /* Translators: message displayed in a Message Dialog, when the player tries to start a new game while one is running, the '\n' is a new line character */
        string text = _("Are you sure you want to start a new game?\nIf you start a new game, the current one will be lost.");
        draw_dialogue (c, width, height, text,
         out b0_x, out b0_y, out b0_width, out b0_height,
         out b1_x, out b1_y, out b1_width, out b1_height);

        var mouse_position = new EventControllerMotion ();
        mouse_position.motion.connect ((x,y)=> {yesno_position (x, y);});
        mouse_position.enter.connect ((x,y)=>  {yesno_position (x, y);});

        var mouse_click = new EventControllerLegacy ();
        mouse_click.event.connect ((event)=>
        {
            switch (event.get_event_type ())
            {
                case Gdk.EventType.BUTTON_PRESS:
                    mouse_pressed = true;
                    redraw ();
                    return true;
                case Gdk.EventType.BUTTON_RELEASE:
                    mouse_pressed = false;
                    if (mouse_button > 1)
                        redraw ();
                    else
                        result_function (mouse_button);
                    return true;
                default:
                    return false;
            }
        });
        add_controller (mouse_click);
        add_controller (mouse_position);
    }

    void yesno_position (double x, double y)
    {
        uint new_button;
        if (x >= b0_x && x <= b0_x + b0_width && y >= b0_y && y <= b0_y + b0_height)
            new_button = 0; /* yes */
        else if (x >= b1_x && x <= b1_x + b1_width && y >= b1_y && y <= b1_y + b1_height)
            new_button = 1; /* no */
        else
            new_button = uint.MAX;

        if (new_button != mouse_button)
        {
            mouse_button = new_button;
            redraw ();
        }
    }

    void draw_oval (Context C, View3D v, Point3D a, Point3D b, Point3D c, Point3D d)
    {
        double x[3];
        double y[3];

        Point3D p[8] = {a,mid_point (a,b),b,mid_point (b,c),c,mid_point (c,d),d,mid_point (d,a)};
        uint lowest_y_midpoint_index = uint.MAX;
        for (int i = 1; i < 8; i += 2)
        {
            if (lowest_y_midpoint_index == uint.MAX || p[lowest_y_midpoint_index].y > p[i].y)
                lowest_y_midpoint_index = i;
        }
        uint second_lowest_y_midpoint_index = uint.MAX;
        for (int i = 1; i < 8; i += 2)
        {
            if (i != lowest_y_midpoint_index && (second_lowest_y_midpoint_index == uint.MAX || p[second_lowest_y_midpoint_index].y > p[i].y))
                second_lowest_y_midpoint_index = i;
        }

        int direction = second_lowest_y_midpoint_index > lowest_y_midpoint_index ? -1 : +1;
        for (uint i = second_lowest_y_midpoint_index;;)
        {
            int between_point = (int)i + direction;
            int next_index = between_point + direction;
            if (between_point < 0)
                between_point += 8;
            else if (between_point > 7)
                between_point -= 8;
            if (next_index < 0)
                next_index += 8;
            else if (next_index > 7)
                next_index -= 8;
            v.to_view_plain (p[i],out x[0], out y[0]);
            v.to_view_plain (p[between_point],out x[1], out y[1]);
            v.to_view_plain (p[next_index],out x[2], out y[2]);
            if (i == second_lowest_y_midpoint_index)
                C.move_to (x[0],y[0]);
            C.curve_to (x[0],y[0],x[1],y[1],x[2],y[2]);
            i = (uint)next_index;
            if (i == second_lowest_y_midpoint_index)
                break;
        }
    }

    Point3D mid_point (Point3D a, Point3D b)
    {
        return {a.x > b.x ? (a.x - b.x) / 2 + b.x : (b.x - a.x) / 2 + a.x,
                a.y > b.y ? (a.y - b.y) / 2 + b.y : (b.y - a.y) / 2 + a.y,
                a.z > b.z ? (a.z - b.z) / 2 + b.z : (b.z - a.z) / 2 + a.z};
    }

    void draw_sphere (Context c, View3D v, int x, int y, double r, double g, double b, double size)
    {
        double increase = (size - 1) / 2;
        draw_oval (c, v, {x + 0 - increase, y + 1.0 + increase, 0 - increase},
                         {x + 0 - increase, y + 0.0 - increase, 1 + increase},
                         {x + 1 + increase, y + 0.0 - increase, 1 + increase},
                         {x + 1 + increase, y + 1.0 + increase, 0 - increase});
        double X, Y, radius, d, top_x, top_y;
        v.to_view_plain ({x + 0.5, y + 0.5, 0.5}, out X, out Y);
        radius = v.2D_diff ({x + 0.5, y + 0.5, 0.5}, {x + 0.5, y + 0.0 - increase, 1 + increase});
        d = v.2D_diff ({x + 0.5, y + 0.5, 0.5}, {x + 0.0 - increase, y + 0.5, 0.5});
        if (d > radius)
            radius = d;
        d = v.2D_diff ({x + 0.5, y + 0.5, 0.5}, {x + 1.0 + increase, y + 0.5, 0.5});
        if (d > radius)
            radius = d;
        d = v.2D_diff ({x + 0.5, y + 0.5, 0.5}, {x + 0.5, y + 1.0 + increase, 0.0 - increase});
        if (d > radius)
            radius = d;

        v.to_view_plain ({x + 0.5, y + 0.5, 1}, out top_x, out top_y);
        Cairo.Pattern pat2 = new Cairo.Pattern.radial (X, Y, radius, top_x, top_y, radius / 20.0);
        pat2.add_color_stop_rgb (0, r / 10, g / 10, b / 10);
        pat2.add_color_stop_rgb (1, r, g, b);
        pat2.add_color_stop_rgb (2, (1 - r) /2 + r, (1 - g) /2 + g, (1 - b) /2 + b);
        c.set_source (pat2);
        c.fill ();
    }

    void draw_eyes_front (Context c, View3D v, int x, int y, bool blink)
    {
        /* eyes look ahead */
        double e = Math.sqrt (0.125);
        double b = blink ? 0.025 : 0.1;
        Point3D centre = {x + 0.5 - e / 2, y + 0.5 + e, 0.5 + e / 2};
        draw_oval (c, v, {centre.x - 0.1, centre.y + b, centre.z - b},
                         {centre.x - 0.1, centre.y - b, centre.z + b},
                         {centre.x + 0.1, centre.y - b, centre.z + b},
                         {centre.x + 0.1, centre.y + b, centre.z - b});
        c.set_source_rgb (0, 0, 0);
        c.fill ();
        centre = {x + 0.5 + e / 2, y + 0.5 + e, 0.5 + e / 2};
        draw_oval (c, v, {centre.x - 0.1, centre.y + b, centre.z - b},
                         {centre.x - 0.1, centre.y - b, centre.z + b},
                         {centre.x + 0.1, centre.y - b, centre.z + b},
                         {centre.x + 0.1, centre.y + b, centre.z - b});
        c.set_source_rgb (0, 0, 0);
        c.fill ();
    }

    void draw_eyes_left (Context c, View3D v, int x, int y, bool blink)
    {
        /* eyes look left */
        double e = Math.sqrt (0.125);
        double b = blink ? 0.025 : 0.1;
        Point3D centre = {x + 0.5 - e, y + 0.5 - e / 2, 0.5 + e / 2};
        draw_oval (c, v, {centre.x - b, centre.y - 0.1, centre.z - b},
                         {centre.x + b, centre.y - 0.1, centre.z + b},
                         {centre.x + b, centre.y + 0.1, centre.z + b},
                         {centre.x - b, centre.y + 0.1, centre.z - b});
        c.set_source_rgb (0, 0, 0);
        c.fill ();
        centre = {x + 0.5 - e, y + 0.5 + e / 2, 0.5 + e / 2};
        draw_oval (c, v, {centre.x - b, centre.y - 0.1, centre.z - b},
                         {centre.x + b, centre.y - 0.1, centre.z + b},
                         {centre.x + b, centre.y + 0.1, centre.z + b},
                         {centre.x - b, centre.y + 0.1, centre.z - b});
        c.set_source_rgb (0, 0, 0);
        c.fill ();
    }

    void draw_eyes_right (Context c, View3D v, int x, int y, bool blink)
    {
        /* eyes look right */
        double e = Math.sqrt (0.125);
        double b = blink ? 0.025 : 0.1;
        Point3D centre = {x + 0.5 + e, y + 0.5 - e / 2, 0.5 + e / 2};
        draw_oval (c, v, {centre.x + b, centre.y - 0.1, centre.z - b},
                         {centre.x - b, centre.y - 0.1, centre.z + b},
                         {centre.x - b, centre.y + 0.1, centre.z + b},
                         {centre.x + b, centre.y + 0.1, centre.z - b});
        c.set_source_rgb (0, 0, 0);
        c.fill ();
        centre = {x + 0.5 + e, y + 0.5 + e / 2, 0.5 + e / 2};
        draw_oval (c, v, {centre.x + b, centre.y - 0.1, centre.z - b},
                         {centre.x - b, centre.y - 0.1, centre.z + b},
                         {centre.x - b, centre.y + 0.1, centre.z + b},
                         {centre.x + b, centre.y + 0.1, centre.z - b});
        c.set_source_rgb (0, 0, 0);
        c.fill ();
    }

    void draw_3D_bonus (Context c, View3D v, int x, int y, Bonus bonus)
    {
        switch (bonus.bonus_type)
        {
            case REGULAR: // apple
                draw_apple (c, v, x, y);
                break;
            case HALF: // cherry
                draw_cherry (c, v, x, y);
                break;
            case DOUBLE: // banana
                draw_banana (c, v, x, y);
                break;
            case LIFE: // heart
                draw_heart (c, v, x, y);
                break;
            case REVERSE: // diamond
                draw_diamond (c, v, x, y);
                break;
            case WARP: // floating hole
                break;
            case 6:  // orange
                break;
            case 7: // red pear
                break;
            default: // blank
                break;
        }
    }

    void draw_banana (Context c, View3D v, double x, double y)
    {
        double cx[3], cy[3];

        v.to_view_plain ({x + 0, y + 1 + 0.25, 1}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 0, y + 1 + 0.25, 0}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1, y + 1 + 0.25, 0}, out cx[2], out cy[2]);
        c.move_to (cx[0], cy[0]);
        c.curve_to (cx[0], cy[0], cx[1], cy[1], cx[2], cy[2]);
        v.to_view_plain ({x + 1, y + 1 + 0.25, 0}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 2, y + 1 + 0.25, 0}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 2, y + 1 + 0.25, 1}, out cx[2], out cy[2]);
        c.curve_to (cx[0], cy[0], cx[1], cy[1], cx[2], cy[2]);
        v.to_view_plain ({x + 2 - 0.1, y + 1 + 0.25, 1 + 0.5}, out cx[0], out cy[0]);
        c.line_to (cx[0], cy[0]);
        v.to_view_plain ({x + 2 - 0.2, y + 1 + 0.25, 1 + 0.5}, out cx[0], out cy[0]);
        c.line_to (cx[0], cy[0]);

        v.to_view_plain ({x + 2 - 0.2, y + 1 + 0.25, 1 + 0.5}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 2 - 0.2, y + 1 + 0.25, 0.5 }, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1, y + 1 + 0.25, 0.5}, out cx[2], out cy[2]);
        c.curve_to (cx[0], cy[0], cx[1], cy[1], cx[2], cy[2]);

        v.to_view_plain ({x + 1, y + 1 + 0.25, 0.5}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 0.2, y + 1 + 0.25, 0.5}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 0.1, y + 1 + 0.25, 1 + 0.1}, out cx[2], out cy[2]);
        c.curve_to (cx[0], cy[0], cx[1], cy[1], cx[2], cy[2]);
        c.set_source_rgb (0.6, 0.6, 0); /* yellow */
        c.fill ();

        v.to_view_plain ({x + 0.1, y + 1 + 0.25, 1 + 0.1}, out cx[0], out cy[0]);
        c.move_to (cx[0], cy[0]);
        v.to_view_plain ({x + 0.1, y + 1 + 0.15, 1 + 0.1}, out cx[0], out cy[0]);
        c.line_to (cx[0], cy[0]);

        v.to_view_plain ({x + 0.1, y + 1 + 0.15, 1 + 0.1}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 0.2, y + 1 + 0.15, 0.5}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1, y + 1 + 0.15, 0.5}, out cx[2], out cy[2]);
        c.curve_to (cx[0], cy[0], cx[1], cy[1], cx[2], cy[2]);

        v.to_view_plain ({x + 1, y + 1 + 0.15, 0.5}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 2 - 0.2, y + 1 + 0.15, 0.5 }, out cx[1], out cy[1]);
        v.to_view_plain ({x + 2 - 0.2, y + 1 + 0.15, 1 + 0.5}, out cx[2], out cy[2]);
        c.curve_to (cx[0], cy[0], cx[1], cy[1], cx[2], cy[2]);

        v.to_view_plain ({x + 2 - 0.2, y + 1 + 0.15, 1 + 0.5}, out cx[0], out cy[0]);
        c.line_to (cx[0], cy[0]);
        v.to_view_plain ({x + 2 - 0.1, y + 1 + 0.15, 1 + 0.5}, out cx[0], out cy[0]);
        c.line_to (cx[0], cy[0]);
        v.to_view_plain ({x + 2 - 0.1, y + 1 + 0.25, 1 + 0.5}, out cx[0], out cy[0]);
        c.line_to (cx[0], cy[0]);
        v.to_view_plain ({x + 2 - 0.2, y + 1 + 0.25, 1 + 0.5}, out cx[0], out cy[0]);
        c.line_to (cx[0], cy[0]);

        v.to_view_plain ({x + 2 - 0.2, y + 1 + 0.25, 1 + 0.5}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 2 - 0.2, y + 1 + 0.25, 0.5 }, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1, y + 1 + 0.25, 0.5}, out cx[2], out cy[2]);
        c.curve_to (cx[0], cy[0], cx[1], cy[1], cx[2], cy[2]);

        v.to_view_plain ({x + 1, y + 1 + 0.25, 0.5}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 0.2, y + 1 + 0.25, 0.5}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 0.1, y + 1 + 0.25, 1 + 0.1}, out cx[2], out cy[2]);
        c.curve_to (cx[0], cy[0], cx[1], cy[1], cx[2], cy[2]);

        c.set_source_rgb (0.8, 0.8, 0); /* yellow */
        c.fill ();

        v.to_view_plain ({x + 0, y + 1 + 0.25, 1}, out cx[0], out cy[0]);
        c.move_to (cx[0], cy[0]);
        v.to_view_plain ({x + 0.1, y + 1 + 0.25, 1 + 0.1}, out cx[0], out cy[0]);
        c.line_to (cx[0], cy[0]);
        v.to_view_plain ({x + 0.1, y + 1 + 0.15, 1 + 0.1}, out cx[0], out cy[0]);
        c.line_to (cx[0], cy[0]);
        v.to_view_plain ({x + 0, y + 1 + 0.15, 1}, out cx[0], out cy[0]);
        c.line_to (cx[0], cy[0]);
        c.set_source_rgb (0.3, 0.3, 0.4);
        c.fill ();
    }

    void draw_diamond (Context c, View3D v, double x, double y)
    {
        const double cos60 = 0.5;
        const double sin60 = 0.866025403784;
        Point3D top[6] = {{x + 0, y + 1, 1.5}, {x + cos60, y + 1.0 - sin60, 1.5}, {x + cos60 + 1, y + 1.0 - sin60, 1.5},
            {x + 2, y + 1, 1.5}, {x + cos60 + 1, y + 1.0 + sin60, 1.5}, {x + cos60, y + 1.0 + sin60, 1.5}};
        Point3D middle[6] = {{x + 0.5, y + 1, 2}, {x + (1.0 - 0.5 * cos60), y + 1.0 - 0.5 * sin60, 2}, {x + 0.5 * cos60 + 1, y + 1.0 - 0.5 * sin60, 2},
            {x + 1.5, y + 1, 2}, {x + 0.5 * cos60 + 1, y + 1.0 + 0.5 * sin60, 2}, {x + (1.0 - 0.5 * cos60), y + 1.0 + 0.5 * sin60, 2}};
        double X, Y;
        v.to_view_plain (top[0], out X, out Y);
        c.move_to (X, Y);
        v.to_view_plain (top[1], out X, out Y);
        c.line_to (X, Y);
        v.to_view_plain (top[2], out X, out Y);
        c.line_to (X, Y);
        v.to_view_plain (top[3], out X, out Y);
        c.line_to (X, Y);
        v.to_view_plain (top[4], out X, out Y);
        c.line_to (X, Y);
        v.to_view_plain (top[5], out X, out Y);
        c.line_to (X, Y);
        c.set_source_rgb (0.8, 0.9, 1); /* almost white */
        c.fill ();
        
        v.to_view_plain (middle[1], out X, out Y);
        c.move_to (X, Y);
        v.to_view_plain (middle[2], out X, out Y);
        c.line_to (X, Y);
        v.to_view_plain (top[2], out X, out Y);
        c.line_to (X, Y);
        v.to_view_plain (top[1], out X, out Y);
        c.line_to (X, Y);
        c.set_source_rgb (0.618, 0.708, 0.802); /* light blue back */
        c.fill ();

        v.to_view_plain (middle[1], out X, out Y);
        c.move_to (X, Y);
        v.to_view_plain (middle[0], out X, out Y);
        c.line_to (X, Y);
        v.to_view_plain (top[0], out X, out Y);
        c.line_to (X, Y);
        v.to_view_plain (top[1], out X, out Y);
        c.line_to (X, Y);
        c.set_source_rgb (0.347, 0.524, 0.712); /* darker blue back */
        c.fill ();

        v.to_view_plain (middle[2], out X, out Y);
        c.move_to (X, Y);
        v.to_view_plain (middle[3], out X, out Y);
        c.line_to (X, Y);
        v.to_view_plain (top[3], out X, out Y);
        c.line_to (X, Y);
        v.to_view_plain (top[2], out X, out Y);
        c.line_to (X, Y);
        c.set_source_rgb (0.347, 0.524, 0.712); /* darker blue back */
        c.fill ();

        v.to_view_plain (middle[3], out X, out Y);
        c.move_to (X, Y);
        v.to_view_plain (middle[4], out X, out Y);
        c.line_to (X, Y);
        v.to_view_plain (top[4], out X, out Y);
        c.line_to (X, Y);
        v.to_view_plain (top[3], out X, out Y);
        c.line_to (X, Y);
        c.set_source_rgb (0.447, 0.624, 0.812); /* darker blue front */
        c.fill ();

        v.to_view_plain (middle[5], out X, out Y);
        c.move_to (X, Y);
        v.to_view_plain (middle[0], out X, out Y);
        c.line_to (X, Y);
        v.to_view_plain (top[0], out X, out Y);
        c.line_to (X, Y);
        v.to_view_plain (top[5], out X, out Y);
        c.line_to (X, Y);
        c.set_source_rgb (0.447, 0.624, 0.812); /* darker blue front */
        c.fill ();

        v.to_view_plain (middle[4], out X, out Y);
        c.move_to (X, Y);
        v.to_view_plain (middle[5], out X, out Y);
        c.line_to (X, Y);
        v.to_view_plain (top[5], out X, out Y);
        c.line_to (X, Y);
        v.to_view_plain (top[4], out X, out Y);
        c.line_to (X, Y);
        c.set_source_rgb (0.718, 0.808, 0.902); /* light blue front */
        c.fill ();
    }

    void draw_heart (Context c, View3D v, double x, double y)
    {
        double H = Math.sqrt (0.125);
        double h = 0.5 - H;
        double cx[3], cy[3];
        //v.to_view_plain ({x + h, y + 1 - h, 1 + h}, out cx[0], out cy[0]);
        //v.to_view_plain ({x - 0.207106781187, y + 0.5, 1.5}, out cx[1], out cy[1]);
        //v.to_view_plain ({x + h, y + h, 2.0 - h}, out cx[2], out cy[2]);
        v.to_view_plain ({x + h, y + 1, 1 + h}, out cx[0], out cy[0]);
        v.to_view_plain ({x - 0.207106781187, y + 1, 1.5}, out cx[1], out cy[1]);
        v.to_view_plain ({x + h, y + 1, 2.0 - h}, out cx[2], out cy[2]);
        c.move_to (cx[0], cy[0]);
        c.curve_to (cx[0], cy[0], cx[1], cy[1], cx[2], cy[2]);
        //v.to_view_plain ({x + h, y + h, 2.0 - h}, out cx[0], out cy[0]);
        //v.to_view_plain ({x + 0.5, y - 0.207106781187, 2.0 + 0.207106781187}, out cx[1], out cy[1]);
        //v.to_view_plain ({x + 1 - h, y + h, 2.0 - h}, out cx[2], out cy[2]);
        v.to_view_plain ({x + h, y + 1, 2.0 - h}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 0.5, y + 1, 2.0 + 0.207106781187}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1 - h, y + 1, 2.0 - h}, out cx[2], out cy[2]);
        c.curve_to (cx[0], cy[0], cx[1], cy[1], cx[2], cy[2]);
        //v.to_view_plain ({x + 1 - h, y + h, 2.0 - h}, out cx[0], out cy[0]);
        //v.to_view_plain ({x + 1, y + h, ((2.0 - h) - 1.5) / 2 + 1.5}, out cx[1], out cy[1]);
        //v.to_view_plain ({x + 1, y + 0.5, 1.5}, out cx[2], out cy[2]);
        v.to_view_plain ({x + 1 - h, y + 1, 2.0 - h}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1, y + 1, ((2.0 - h) - 1.5) / 2 + 1.5}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1, y + 1, 1.5}, out cx[2], out cy[2]);
        c.curve_to (cx[0], cy[0], cx[1], cy[1], cx[2], cy[2]);
        //v.to_view_plain ({x + 1, y + 0.5, 1.5}, out cx[0], out cy[0]);
        //v.to_view_plain ({x + 1, y + h, ((2.0 - h) - 1.5) / 2 + 1.5}, out cx[1], out cy[1]);
        //v.to_view_plain ({x + 1 + h, y + h, 2.0 - h}, out cx[2], out cy[2]);
        v.to_view_plain ({x + 1, y + 1, 1.5}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1, y + 1, ((2.0 - h) - 1.5) / 2 + 1.5}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1 + h, y + 1, 2.0 - h}, out cx[2], out cy[2]);
        c.curve_to (cx[0], cy[0], cx[1], cy[1], cx[2], cy[2]);
        //v.to_view_plain ({x + 1 + h, y + h, 2.0 - h}, out cx[0], out cy[0]);
        //v.to_view_plain ({x + 1.5, y - 0.207106781187, 2.0 + 0.207106781187}, out cx[1], out cy[1]);
        //v.to_view_plain ({x + 2 - h, y + h, 2.0 - h}, out cx[2], out cy[2]);
        v.to_view_plain ({x + 1 + h, y + 1, 2.0 - h}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1.5, y + 1, 2.0 + 0.207106781187}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 2 - h, y + 1, 2.0 - h}, out cx[2], out cy[2]);
        c.curve_to (cx[0], cy[0], cx[1], cy[1], cx[2], cy[2]);
        //v.to_view_plain ({x + 2 - h, y + h, 2.0 - h}, out cx[0], out cy[0]);
        //v.to_view_plain ({x + 2 + 0.207106781187, y + 0.5, 1.5}, out cx[1], out cy[1]);
        //v.to_view_plain ({x + 2 - h, y + 1 - h, 1 + h}, out cx[2], out cy[2]);
        v.to_view_plain ({x + 2 - h, y + 1, 2.0 - h}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 2 + 0.207106781187, y + 1, 1.5}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 2 - h, y + 1, 1 + h}, out cx[2], out cy[2]);
        c.curve_to (cx[0], cy[0], cx[1], cy[1], cx[2], cy[2]);
        //v.to_view_plain ({x + 1, y + 2, 0}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1, y + 1, 0}, out cx[0], out cy[0]);
        c.line_to (cx[0],cy[0]);
        //v.to_view_plain ({x + h, y + 1 - h, 1 + h}, out cx[0], out cy[0]);
        v.to_view_plain ({x + h, y + 1, 1 + h}, out cx[0], out cy[0]);
        c.line_to (cx[0],cy[0]);
        //v.to_view_plain ({x + 1, y + 1, 1 + h}, out cx[0], out cy[0]);
        //v.to_view_plain ({x + 1, y + 0.5, 1.5}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1, y + 1, 1 + h}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 0.875, y + 1, 1.5}, out cx[1], out cy[1]);
        //double radius = v.2D_diff ({x + 1, y + 1, 1 + h}, {x + 1, y + 2, 0.0});
        double radius = v.2D_diff ({x + 1, y + 1, 1 + h}, {x + 1, y + 1, 0.0});
        Cairo.Pattern pat2 = new Cairo.Pattern.radial (cx[0], cy[0], radius, cx[1], cy[1], radius / 40.0);
        pat2.add_color_stop_rgb (0, 1 / 10, 0, 0);
        pat2.add_color_stop_rgb (1, 1, 0, 0);
        pat2.add_color_stop_rgb (2, 1, 0.5, 0.5);
        c.set_source (pat2);
        c.fill ();
    }

    void draw_cherry (Context c, View3D v, double x, double y)
    {
        // draw left cherry */
        draw_oval (c, v, {x, 1.5 + y, 0},
                         {x, 0.5 + y, 1},
                         {1.0 + x, 0.5 + y, 1},
                         {1.0 + x, 1.5 + y, 0});
        double X, Y, radius, d, top_x, top_y;
        v.to_view_plain ({x + 0.5, y + 1.0, 0.5}, out X, out Y);
        radius = v.2D_diff ({x + 0.5, y + 1.0, 0.5}, {x + 0.5, y + 0.5, 1.0});
        d = v.2D_diff ({x + 0.5, y + 1.0, 0.5}, {x + 0.0, y + 1.0, 0.5});
        if (d > radius)
            radius = d;
        d = v.2D_diff ({x + 0.5, y + 1.0, 0.5}, {x + 1.0, y + 1.0, 0.5});
        if (d > radius)
            radius = d;
        d = v.2D_diff ({x + 0.5, y + 1.0, 0.5}, {x + 0.5, y + 1.5, 0.0});
        if (d > radius)
            radius = d;

        v.to_view_plain ({x + 0.5, y + 1.0, 1.0}, out top_x, out top_y);
        Cairo.Pattern pat2 = new Cairo.Pattern.radial (X, Y, radius, top_x, top_y, radius / 20.0);
        pat2.add_color_stop_rgb (0, 1 / 10, 0, 0);
        pat2.add_color_stop_rgb (1, 1, 0, 0);
        pat2.add_color_stop_rgb (2, 1, 0.5, 0.5);
        c.set_source (pat2);
        c.fill ();
        // draw left stalk */
        double cx[3];
        double cy[3];
        v.to_view_plain ({x + 0.45, y + 1.0, 1.0}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 0.45, y + 1.0, 2.0}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1.15, y + 1.0, 2.0}, out cx[2], out cy[2]);
        c.move_to (cx[0],cy[0]);
        c.curve_to (cx[0],cy[0],cx[1],cy[1],cx[2],cy[2]);
        v.to_view_plain ({x + 1.25, y + 1.0, 2.0}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 0.55, y + 1.0, 2.0}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 0.55, y + 1.0, 1.0}, out cx[2], out cy[2]);
        c.curve_to (cx[0],cy[0],cx[1],cy[1],cx[2],cy[2]);
        c.set_source_rgb (0.5, 0.5, 0);
        c.fill ();        
        // draw right cherry */
        draw_oval (c, v, {1.0 + x, 1.5 + y, 0},
                         {1.0 + x, 0.5 + y, 1},
                         {2.0 + x, 0.5 + y, 1},
                         {2.0 + x, 1.5 + y, 0});
        v.to_view_plain ({x + 1.5, y + 1.0, 0.5}, out X, out Y);
        radius = v.2D_diff ({x + 1.5, y + 1.0, 0.5}, {x + 1.5, y + 0.5, 1.0});
        d = v.2D_diff ({x + 1.5, y + 1.0, 0.5}, {x + 1.0, y + 1.0, 0.5});
        if (d > radius)
            radius = d;
        d = v.2D_diff ({x + 1.5, y + 1.0, 0.5}, {x + 2.0, y + 1.0, 0.5});
        if (d > radius)
            radius = d;
        d = v.2D_diff ({x + 1.5, y + 1.0, 0.5}, {x + 1.5, y + 1.5, 0.0});
        if (d > radius)
            radius = d;

        v.to_view_plain ({x + 1.5, y + 1.0, 1.0}, out top_x, out top_y);
        pat2 = new Cairo.Pattern.radial (X, Y, radius, top_x, top_y, radius / 20.0);
        pat2.add_color_stop_rgb (0, 1 / 10, 0, 0);
        pat2.add_color_stop_rgb (1, 1, 0, 0);
        pat2.add_color_stop_rgb (2, 1, 0.5, 0.5);
        c.set_source (pat2);
        c.fill ();
        // draw right stalk */
        v.to_view_plain ({x + 1.45, y + 1.0, 1.0}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1.45, y + 1.0, 2.0}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1.15, y + 1.0, 2.0}, out cx[2], out cy[2]);
        c.move_to (cx[0],cy[0]);
        c.curve_to (cx[0],cy[0],cx[1],cy[1],cx[2],cy[2]);
        v.to_view_plain ({x + 1.25, y + 1.0, 2.0}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1.55, y + 1.0, 2.0}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1.55, y + 1.0, 1.0}, out cx[2], out cy[2]);
        c.curve_to (cx[0],cy[0],cx[1],cy[1],cx[2],cy[2]);
        c.set_source_rgb (0.5, 0.5, 0);
        c.fill ();        
        /* draw leaf */
        v.to_view_plain ({x + 1.20, y + 1.0, 2.0}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1.20 - 0.5, y + 1.0, 2.0}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1.20 - 0.5, y + 1.0 - 0.5, 2.0}, out cx[2], out cy[2]);
        c.move_to (cx[0],cy[0]);
        c.curve_to (cx[0],cy[0],cx[1],cy[1],cx[2],cy[2]);
        v.to_view_plain ({x + 1.20 - 0.5, y + 1.0 - 0.5, 2.0}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1.20, y + 1.0 - 0.5, 2.0}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1.20, y + 1.0, 2.0}, out cx[2], out cy[2]);
        c.curve_to (cx[0],cy[0],cx[1],cy[1],cx[2],cy[2]);
        c.set_source_rgb (0, 0.75, 0);
        c.fill ();   
    }

    void draw_apple (Context c, View3D v, double x, double y)
    {
        double cx[3];
        double cy[3];
        double top_x, top_y;

        v.to_view_plain ({x + 0, y + 1, 1}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 0, y + 1, 0}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1, y + 1, 0}, out cx[2], out cy[2]);
        c.move_to (cx[0],cy[0]);
        c.curve_to (cx[0],cy[0],cx[1],cy[1],cx[2],cy[2]);
        v.to_view_plain ({x + 1, y + 1, 0}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 2, y + 1, 0}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 2, y + 1, 1}, out cx[2], out cy[2]);
        c.curve_to (cx[0],cy[0],cx[1],cy[1],cx[2],cy[2]);
        v.to_view_plain ({x + 2, y + 1, 1}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 2, y + 1.0, 2}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1.0, y + 1, 2}, out cx[2], out cy[2]);
        c.curve_to (cx[0],cy[0],cx[1],cy[1],cx[2],cy[2]);
        v.to_view_plain ({x + 1.0, y + 1, 2}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 0, y + 1, 2}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 0, y + 1, 1}, out cx[2], out cy[2]);
        c.curve_to (cx[0],cy[0],cx[1],cy[1],cx[2],cy[2]);

        v.to_view_plain ({x + 1, y + 1.0, 1.5}, out top_x, out top_y);
        double radius = v.2D_diff ({x + 1, y + 1.0, 1.5}, {x + 1, y + 1.0, 0});
        Cairo.Pattern pat2 = new Cairo.Pattern.radial (top_x, top_y, radius, top_x, top_y, radius / 20.0);
        pat2.add_color_stop_rgb (0, 0, 1 / 10, 0);
        pat2.add_color_stop_rgb (1, 0, 1, 0);
        pat2.add_color_stop_rgb (2, 0.5, 1, 0.5);
        c.set_source (pat2);
        c.fill ();

        
        for (double 3D_radius = 0.2;3D_radius > 0.01;3D_radius-=0.01)
        {
            v.to_view_plain ({x + (1 - 3D_radius), y + 1.0, 1.75}, out cx[0], out cy[0]);
            v.to_view_plain ({x + (1 - 3D_radius), y + (1 - 3D_radius), 1.75}, out cx[1], out cy[1]);
            v.to_view_plain ({x + 1.00, y + (1 - 3D_radius), 1.75}, out cx[2], out cy[2]);
            c.move_to (cx[0],cy[0]);
            c.curve_to (cx[0],cy[0],cx[1],cy[1],cx[2],cy[2]);
            v.to_view_plain ({x + 1.00, y + (1 - 3D_radius), 1.75}, out cx[0], out cy[0]);
            v.to_view_plain ({x + (1 + 3D_radius), y + (1 - 3D_radius), 1.75}, out cx[1], out cy[1]);
            v.to_view_plain ({x + (1 + 3D_radius), y + 1.0, 1.75}, out cx[2], out cy[2]);
            c.curve_to (cx[0],cy[0],cx[1],cy[1],cx[2],cy[2]);
            v.to_view_plain ({x + (1 + 3D_radius), y + 1.0, 1.75}, out cx[0], out cy[0]);
            v.to_view_plain ({x + (1 + 3D_radius), y + (1 + 3D_radius), 1.75}, out cx[1], out cy[1]);
            v.to_view_plain ({x + 1, y + (1 + 3D_radius), 1.75}, out cx[2], out cy[2]);
            c.curve_to (cx[0],cy[0],cx[1],cy[1],cx[2],cy[2]);
            v.to_view_plain ({x + 1, y + (1 + 3D_radius), 1.75}, out cx[0], out cy[0]);
            v.to_view_plain ({x + (1 - 3D_radius), y + (1 + 3D_radius), 1.75}, out cx[1], out cy[1]);
            v.to_view_plain ({x + (1 - 3D_radius), y + 1.0, 1.75}, out cx[2], out cy[2]);
            c.curve_to (cx[0],cy[0],cx[1],cy[1],cx[2],cy[2]);
            c.set_source_rgb (0.4, 0.8 - (0.2 - 3D_radius) * 3, 0.4 - (0.2 - 3D_radius) * 2);
            c.fill ();
        }        

        /* draw left leaf */
        v.to_view_plain ({x + 1.0, y + 1.0, 1.75}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1.0 - 0.5, y + 1.0, 1.75}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1.0 - 0.5, y + 1.0 - 0.5, 1.75}, out cx[2], out cy[2]);
        c.move_to (cx[0],cy[0]);
        c.curve_to (cx[0],cy[0],cx[1],cy[1],cx[2],cy[2]);
        v.to_view_plain ({x + 1.0 - 0.5, y + 1.0 - 0.5, 1.75}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1.0, y + 1.0 - 0.5, 1.75}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1.0, y + 1.0, 1.75}, out cx[2], out cy[2]);
        c.curve_to (cx[0],cy[0],cx[1],cy[1],cx[2],cy[2]);
        c.set_source_rgb (0.25, 0.5, 0);
        c.fill ();        
        /* draw right leaf */
        v.to_view_plain ({x + 1.0, y + 1.0, 1.75}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1.0 + 0.5, y + 1.0, 1.75}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1.0 + 0.5, y + 1.0 - 0.5, 1.75}, out cx[2], out cy[2]);
        c.move_to (cx[0],cy[0]);
        c.curve_to (cx[0],cy[0],cx[1],cy[1],cx[2],cy[2]);
        v.to_view_plain ({x + 1.0 + 0.5, y + 1.0 - 0.5, 1.75}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1.0, y + 1.0 - 0.5, 1.75}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1.0, y + 1.0, 1.75}, out cx[2], out cy[2]);
        c.curve_to (cx[0],cy[0],cx[1],cy[1],cx[2],cy[2]);
        c.set_source_rgb (0, 0.5, 0.25);
        c.fill ();        
    }

    void draw_wall_segment (int i,
                            Context C, int x, int y, int x_size, int y_size)
    {
        int x_s13 = x_size / 3;
        int x_remainder = x_size - x_s13 * 3;
        int y_s13 = y_size / 3;
        int y_remainder = y_size - y_s13 * 3;
        if (i >= 'b' && i <= 'l')
        {
            /* center square */
            C.rectangle (x_remainder == 2 ? x_s13 + x + x_remainder : x_s13 + x,
                     y_remainder == 2 ? y_s13 + y + y_remainder : y_s13 + y,
                     x_remainder == 2 ? x_s13 : x_s13 + x_remainder,
                     y_remainder == 2 ? y_s13 : y_s13 + y_remainder);
            C.set_source_rgb (0.5,0.5,0.5);
            C.fill ();
        }
        if (i == 'b' || i == 'd' || i == 'e' || i == 'h' || i == 'i' || i == 'j' || i == 'l')
        {
            /* top square */
            C.rectangle (x_remainder == 2 ? x_s13 + x + x_remainder : x_s13 + x,
                         y,
                         x_remainder == 2 ? x_s13 : x_s13 + x_remainder,
                         y_remainder == 2 ? y_s13 + y_remainder : y_s13);
            C.set_source_rgb (0.5,0.5,0.5);
            C.fill ();
        }
        if (i == 'c' || i == 'd' || i == 'f' || i == 'h' || i == 'i' || i == 'k' || i == 'l')
        {
            /* right square */
            C.rectangle (x_s13 + x_s13 + x_remainder + x,
                         y_remainder == 2 ? y_s13 + y_remainder + y : y_s13 + y,
                         x_remainder == 2 ? x_s13 + x_remainder : x_s13,
                         y_remainder == 2 ? y_s13 : y_s13 + y_remainder);
            C.set_source_rgb (0.5,0.5,0.5);
            C.fill ();
        }
        if (i == 'b' || i == 'f' || i == 'g' || i == 'i' || i == 'j' || i == 'k' || i == 'l')
        {
            /* bottom square */
            C.rectangle (x_remainder == 2 ? x_s13 + x + x_remainder : x_s13 + x,
                y_s13 + y_s13 + y_remainder + y,
                x_remainder == 2 ? x_s13 : x_s13 + x_remainder,
                y_remainder == 2 ? y_s13 + y_remainder : y_s13);
            C.set_source_rgb (0.5,0.5,0.5);
            C.fill ();
        }
        if (i == 'c' || i == 'e' || i == 'g' || i == 'h' || i == 'j' || i == 'k' || i == 'l')
        {
            /* left square */
            C.rectangle (x,
                y_remainder == 2 ? y_s13 + y + y_remainder : y_s13 + y,
                x_remainder == 2 ? x_s13 + x_remainder : x_s13,
                y_remainder == 2 ? y_s13 : y_s13 + y_remainder);
            C.set_source_rgb (0.5,0.5,0.5);
            C.fill ();
        }
    }

    void draw_worm_segment (Context C, int x, int y, int x_size, int y_size, int color, bool is_materialized, bool eaten_bonus)
    {
        if (eaten_bonus)
        {
            int a = x_size + x_size / 5;
            if (a < x_size + 1)
                x_size += 1;
            else
            {
                x -= (a - x_size) / 2;
                x_size = a;
            }
            a = y_size + y_size / 5;
            if (a < y_size + 1)
                y_size += 1;
            else
            {
                y -= (a - y_size) / 2;
                y_size = a;
            }
        }
        else
        {
            /* leave a one pixel border */
            ++x;
            ++y;
            x_size -= 1;
            y_size -= 1;
        }
        
        const double PI2 = 1.570796326794896619231321691639751442;
        double x_s13 = x_size / 3.0;
        double x_s23 = x_s13 + x_s13;
        double y_s13 = y_size / 3.0;
        double y_s23 = y_s13 + y_s13;
        
        C.arc (x + x_s23, y + y_s13, x_s13 < y_s13 ? x_s13 : y_s13, -PI2, 0);
        C.arc (x + x_s23, y + y_s23, x_s13 < y_s13 ? x_s13 : y_s13, 0, PI2);
        C.arc (x + x_s13, y + y_s23, x_s13 < y_s13 ? x_s13 : y_s13, PI2, PI2 * 2);
        C.arc (x + x_s13, y + y_s13, x_s13 < y_s13 ? x_s13 : y_s13, PI2 * 2, -PI2);
        
        set_color (C, color, is_materialized);
        C.fill ();                
    }

    void draw_bonus (Context C, int x, int y, int x_size, int y_size, BonusType type)
    {
        double x_m = x_size;
        double y_m = y_size;
        switch (type)
        {
            case REGULAR:
                x_m /= 18;
                y_m /= 18;
                C.move_to (x + x_m * 15, y + y_m * 8);
                C.curve_to (x + x_m * 15.023438, y + y_m * 10.035156, x + x_m * 13.953125, y + y_m * 17.1875, x + x_m * 8, y + y_m * 14.429688);
                C.curve_to (x + x_m * 1.90625, y + y_m * 17.109375, x + x_m * 1.03125, y + y_m * 9.921875, x + x_m * 1, y + y_m * 8);
                C.curve_to (x + x_m * 1.007813, y + y_m * 5.109375, x + x_m * 3.300781, y + y_m * 1.355469, x + x_m * 8, y + y_m * 4.3125);
                C.curve_to (x + x_m * 12.933594, y + y_m * 1.394531, x + x_m * 15.0625, y + y_m * 5, x + x_m * 15, y + y_m * 8);
                C.close_path ();
                C.set_source_rgba (0,1,0,1);
                C.fill ();                
                C.move_to (x + x_m * 9.65625, y + y_m * 1.34375);
                C.curve_to (x + x_m * 8, y + y_m * 2, x + x_m * 8, y + y_m * 3.667969, x + x_m * 8, y + y_m * 5);
                C.close_path ();
                C.set_source_rgba (0,1,0,1);
                C.fill ();
                break;
            case HALF:
                x_m /= 16;
                y_m /= 16;
                C.move_to (x + x_m * 10.253906, y + y_m * 1.3125);
                C.curve_to (x + x_m * 9.472656, y + y_m * 4.730469, x + x_m * 9.445313, y + y_m * 8.015625, x + x_m * 11.625, y + y_m * 10.683594);
                C.set_source_rgba (0.305882,0.603922,0.0235294,1);
                C.fill ();
                C.move_to (x + x_m * 10.296875, y + y_m * 1.152344);
                C.curve_to (x + x_m * 9.046875, y + y_m * 7.132813, x + x_m * 6.023438, y + y_m * 7.765625, x + x_m * 3.84375, y + y_m * 10.429688);
                C.set_source_rgba (0.305882,0.603922,0.0235294,1);
                C.fill ();
                C.move_to (x + x_m * 7, y + y_m * 10);
                C.curve_to (x + x_m * 7, y + y_m * 11.65625, x + x_m * 5.65625, y + y_m * 13, x + x_m * 4, y + y_m * 13);
                C.curve_to (x + x_m * 2.34375, y + y_m * 13, x + x_m * 1, y + y_m * 11.65625, x + x_m * 1, y + y_m * 10);
                C.curve_to (x + x_m * 1, y + y_m * 8.34375, x + x_m * 2.34375, y + y_m * 7, x + x_m * 4, y + y_m * 7);
                C.curve_to (x + x_m * 5.65625, y + y_m * 7, x + x_m * 7, y + y_m * 8.34375, x + x_m * 7, y + y_m * 10);
                C.set_source_rgba (0.8,0,0,1);
                C.fill ();
                C.move_to (x + x_m * 15, y + y_m * 12);
                C.curve_to (x + x_m * 15, y + y_m * 13.65625, x + x_m * 13.65625, y + y_m * 15, x + x_m * 12, y + y_m * 15);
                C.curve_to (x + x_m * 10.34375, y + y_m * 15, x + x_m * 9, y + y_m * 13.65625, x + x_m * 9, y + y_m * 12);
                C.curve_to (x + x_m * 9, y + y_m * 10.34375, x + x_m * 10.34375, y + y_m * 9, x + x_m * 12, y + y_m * 9);
                C.curve_to (x + x_m * 13.65625, y + y_m * 9, x + x_m * 15, y + y_m * 10.34375, x + x_m * 15, y + y_m * 12);
                C.set_source_rgba (0.8,0,0,1);
                C.fill ();
                break;
            case DOUBLE:
                x_m /= 18;
                y_m /= 18;
                C.move_to (x + x_m * 0.695313, y + y_m * 8.425781);
                C.curve_to (x + x_m * 8.914063, y + y_m * 11.246094, x + x_m * 13.257813, y + y_m * 5.894531, x + x_m * 13.847656, y + y_m * 4.394531);
                C.curve_to (x + x_m * 14.285156, y + y_m * 3.351563, x + x_m * 14.308594, y + y_m * 3.082031, x + x_m * 14.402344, y + y_m * 2.535156);
                C.curve_to (x + x_m * 14.941406, y + y_m * 2.433594, x + x_m * 15.613281, y + y_m * 2.71875, x + x_m * 16, y + y_m * 3.0625);
                C.curve_to (x + x_m * 15.566406, y + y_m * 3.535156, x + x_m * 15.261719, y + y_m * 4.246094, x + x_m * 15.167969, y + y_m * 4.984375);
                C.curve_to (x + x_m * 15.675781, y + y_m * 11.316406, x + x_m * 7.71875, y + y_m * 17.683594, x + x_m * 0, y + y_m * 9.972656);
                C.curve_to (x + x_m * 0.03125, y + y_m * 9.433594, x + x_m * 0.210938, y + y_m * 8.84375, x + x_m * 0.695313, y + y_m * 8.425781);
                C.set_source_rgba (0.988235,0.913725,0.309804,1);
                C.fill ();                
                break;
            case LIFE:
                x_m /= 16;
                y_m /= 16;
                C.move_to (x + x_m * 4.753906, y + y_m * 1.828125);
                C.curve_to (x + x_m * 2.652344, y + y_m * 1.851563, x + x_m * 1.019531, y + y_m * 3.648438, x + x_m * 1, y + y_m * 5.8125);
                C.curve_to (x + x_m * 0.972656, y + y_m * 8.890625, x + x_m * 2.808594, y + y_m * 9.882813, x + x_m * 8.015625, y + y_m * 14.171875);
                C.curve_to (x + x_m * 12.992188, y + y_m * 9.558594, x + x_m * 14.976563, y + y_m * 8.316406, x + x_m * 15, y + y_m * 5.722656);
                C.curve_to (x + x_m * 15.027344, y + y_m * 2.886719, x + x_m * 10.90625, y + y_m * 0.128906, x + x_m * 7.910156, y + y_m * 3.121094);
                C.curve_to (x + x_m * 6.835938, y + y_m * 2.199219, x + x_m * 5.742188, y + y_m * 1.816406, x + x_m * 4.753906, y + y_m * 1.828125);
                C.set_source_rgba (1,0,0,1);
                C.fill ();                
                break;
            case REVERSE:
                x_m /= 16;
                y_m /= 16;
                C.move_to (x + x_m * 4, y + y_m * 2);
                C.line_to (x + x_m * 12, y + y_m * 2);
                C.line_to (x + x_m * 15, y + y_m * 6);
                C.line_to (x + x_m * 8, y + y_m * 15);
                C.line_to (x + x_m * 1, y + y_m * 6);
                C.set_source_rgba (0.717647,0.807843,0.901961,1);
                C.fill ();   
                C.move_to (x + x_m * 11, y + y_m * 6);
                C.line_to (x + x_m * 8, y + y_m * 15);
                C.line_to (x + x_m * 5, y + y_m * 6);
                C.set_source_rgba (0.447059,0.623529,0.811765,1);
                C.fill ();   
                C.move_to (x + x_m * 4, y + y_m * 2);
                C.line_to (x + x_m * 8, y + y_m * 2);
                C.line_to (x + x_m * 5, y + y_m * 6);
                C.line_to (x + x_m * 1, y + y_m * 6);
                C.set_source_rgba (0.447059,0.623529,0.811765,1);
                C.fill ();   
                C.move_to (x + x_m * 12, y + y_m * 2);
                C.line_to (x + x_m * 8, y + y_m * 2);
                C.line_to (x + x_m * 11, y + y_m * 6);
                C.line_to (x + x_m * 15, y + y_m * 6);
                C.set_source_rgba (0.447059,0.623529,0.811765,1);
                C.fill ();   
                break;
            case WARP:
                x_m /= 16;
                y_m /= 16;
                C.move_to (x + x_m * 8.664063, y + y_m * 0.621094);
                C.curve_to (x + x_m * 6.179688, y + y_m * 0.761719, x + x_m * 4.265625, y + y_m * 2.679688, x + x_m * 4.40625, y + y_m * 5.164063);
                C.line_to (x + x_m * 7.433594, y + y_m * 5.164063);
                C.curve_to (x + x_m * 7.386719, y + y_m * 4.3125, x + x_m * 8.003906, y + y_m * 3.699219, x + x_m * 8.855469, y + y_m * 3.652344);
                C.curve_to (x + x_m * 9.707031, y + y_m * 3.601563, x + x_m * 10.417969, y + y_m * 4.21875, x + x_m * 10.464844, y + y_m * 5.070313);
                C.line_to (x + x_m * 10.464844, y + y_m * 5.117188);
                C.curve_to (x + x_m * 10.46875, y + y_m * 5.316406, x + x_m * 10.417969, y + y_m * 5.609375, x + x_m * 10.273438, y + y_m * 5.78125);
                C.curve_to (x + x_m * 9.929688, y + y_m * 6.191406, x + x_m * 9.542969, y + y_m * 6.53125, x + x_m * 9.234375, y + y_m * 6.773438);
                C.curve_to (x + x_m * 8.890625, y + y_m * 7.035156, x + x_m * 8.515625, y + y_m * 7.351563, x + x_m * 8.144531, y + y_m * 7.816406);
                C.curve_to (x + x_m * 7.773438, y + y_m * 8.28125, x + x_m * 7.433594, y + y_m * 8.949219, x + x_m * 7.433594, y + y_m * 9.710938);
                C.curve_to (x + x_m * 7.425781, y + y_m * 10.507813, x + x_m * 8.148438, y + y_m * 11.222656, x + x_m * 8.949219, y + y_m * 11.222656);
                C.curve_to (x + x_m * 9.75, y + y_m * 11.222656, x + x_m * 10.476563, y + y_m * 10.507813, x + x_m * 10.464844, y + y_m * 9.710938);
                C.curve_to (x + x_m * 10.464844, y + y_m * 9.710938, x + x_m * 10.4375, y + y_m * 9.753906, x + x_m * 10.511719, y + y_m * 9.664063);
                C.curve_to (x + x_m * 10.585938, y + y_m * 9.566406, x + x_m * 10.789063, y + y_m * 9.40625, x + x_m * 11.078125, y + y_m * 9.1875);
                C.curve_to (x + x_m * 12.921875, y + y_m * 7.792969, x + x_m * 13.492188, y + y_m * 7.003906, x + x_m * 13.492188, y + y_m * 4.882813);
                C.curve_to (x + x_m * 13.355469, y + y_m * 2.394531, x + x_m * 11.152344, y + y_m * 0.484375, x + x_m * 8.664063, y + y_m * 0.621094);
                //animate color C.set_source_rgba (0.678431,0.498039,0.658824,1);
                C.set_source_rgba (animate%30 < 10 ? (animate%30 / 10.0) : (animate%30 >= 20 ? 0 : ((20 - animate%30) / 10.0)),
                                   (animate+10)%30 < 10 ? ((animate+10)%30 / 10.0) : ((animate+10)%30 >= 20 ? 0 : ((20 - (animate+10)%30) / 10.0)),
                                   (animate+20)%30 < 10 ? ((animate+20)%30 / 10.0) : ((animate+20)%30 >= 20 ? 0 : ((20 - (animate+20)%30) / 10.0)),
                                   1);
                C.fill ();   
                C.move_to (x + x_m * 8.949219, y + y_m * 12.738281);
                C.curve_to (x + x_m * 8.113281, y + y_m * 12.738281, x + x_m * 7.433594, y + y_m * 13.417969, x + x_m * 7.433594, y + y_m * 14.253906);
                C.curve_to (x + x_m * 7.433594, y + y_m * 15.089844, x + x_m * 8.113281, y + y_m * 15.769531, x + x_m * 8.949219, y + y_m * 15.769531);
                C.curve_to (x + x_m * 9.785156, y + y_m * 15.769531, x + x_m * 10.464844, y + y_m * 15.089844, x + x_m * 10.464844, y + y_m * 14.253906);
                C.curve_to (x + x_m * 10.464844, y + y_m * 13.417969, x + x_m * 9.785156, y + y_m * 12.738281, x + x_m * 8.949219, y + y_m * 12.738281);
                C.fill ();   
                break;
            case 6:
                x_m /= 16;
                y_m /= 16;
                C.move_to (x + x_m * 8.902344, y + y_m * 0.160156);
                C.curve_to (x + x_m * 6.953125, y + y_m * 1.15625, x + x_m * 7.480469, y + y_m * 3.089844, x + x_m * 7.453125, y + y_m * 5.019531);
                C.line_to (x + x_m * 8.257813, y + y_m * 4.8125);
                C.curve_to (x + x_m * 8.144531, y + y_m * 3.507813, x + x_m * 9.359375, y + y_m * 1.511719, x + x_m * 10.742188, y + y_m * 1.675781);

                C.set_source_rgba (0.305882,0.603922,0.0235294,1);
                C.fill ();   
                C.move_to (x + x_m * 14, y + y_m * 9);
                C.curve_to (x + x_m * 14, y + y_m * 5.6875, x + x_m * 11.3125, y + y_m * 3, x + x_m * 8, y + y_m * 3);
                C.curve_to (x + x_m * 4.6875, y + y_m * 3, x + x_m * 2, y + y_m * 5.6875, x + x_m * 2, y + y_m * 9);
                C.curve_to (x + x_m * 2, y + y_m * 12.3125, x + x_m * 4.6875, y + y_m * 15, x + x_m * 8, y + y_m * 15);
                C.curve_to (x + x_m * 11.3125, y + y_m * 15, x + x_m * 14, y + y_m * 12.3125, x + x_m * 14, y + y_m * 9);

                C.set_source_rgba (0.960784,0.47451,0,1);
                C.fill ();   
                break;
            case 7:
                x_m /= 16;
                y_m /= 16;
                C.move_to (x + x_m * 4.585938, y + y_m * 0.96875);
                C.curve_to (x + x_m * 3.914063, y + y_m * 3.050781, x + x_m * 5.65625, y + y_m * 4.042969, x + x_m * 7, y + y_m * 5.429688);
                C.line_to (x + x_m * 7.421875, y + y_m * 4.710938);
                C.curve_to (x + x_m * 6.417969, y + y_m * 3.871094, x + x_m * 5.867188, y + y_m * 1.597656, x + x_m * 6.960938, y + y_m * 0.738281);
                C.set_source_rgba (0.305882,0.603922,0.0235294,1);
                C.fill ();   
                C.move_to (x + x_m * 12.933594, y + y_m * 5.347656);
                C.curve_to (x + x_m * 13.652344, y + y_m * 7.882813, x + x_m * 12.867188, y + y_m * 8.753906, x + x_m * 12.871094, y + y_m * 10.476563);
                C.curve_to (x + x_m * 12.875, y + y_m * 12.890625, x + x_m * 13.015625, y + y_m * 14.386719, x + x_m * 11.148438, y + y_m * 15.089844);
                C.curve_to (x + x_m * 9.941406, y + y_m * 15.492188, x + x_m * 8.785156, y + y_m * 15.382813, x + x_m * 6.539063, y + y_m * 12.617188);
                C.curve_to (x + x_m * 5.886719, y + y_m * 11.765625, x + x_m * 4.117188, y + y_m * 11.683594, x + x_m * 3.226563, y + y_m * 10.214844);
                C.curve_to (x + x_m * 2.117188, y + y_m * 8.375, x + x_m * 2.902344, y + y_m * 5.152344, x + x_m * 6.707031, y + y_m * 4.464844);
                C.curve_to (x + x_m * 8.609375, y + y_m * 2.308594, x + x_m * 11.933594, y + y_m * 3.136719, x + x_m * 12.933594, y + y_m * 5.347656);
                C.set_source_rgba (0.937255,0.160784,0.160784,1);
                C.fill ();   
                break;
            default:
                break;
        }
    }

    void draw_text (Context C, int x, int y, string text, int target_width, int color)
    {
        /* draw using x,y as the bottom left corner of the text */
        int target_font_size = 1;
        uint target_width_diff = uint.MAX;
        
        for (int font_size = 1;font_size < 200;font_size++)
        {
            Context c = new Context (C.get_target ());
            c.move_to (0, 0);
            c.set_font_size (font_size);
            Cairo.TextExtents extents;
            c.text_extents (text, out extents);
            uint width_diff = (target_width - (int)extents.width).abs ();
            if (width_diff > target_width_diff && width_diff - target_width_diff > 2)
                break;
            else if (width_diff < target_width_diff)
            {
                target_width_diff = width_diff;
                target_font_size = font_size;
            }
        }
        C.move_to (x, y);
        set_color (C, color, true);
        C.set_font_size (target_font_size);
        C.show_text (text);
    }

    void set_color (Context C, int color, bool bright)
    {
        double r;
        double g;
        double b;
        get_worm_rgb (color, bright, out r, out g, out b);
        C.set_source_rgba (r, g, b, 1);
    }

    void draw_dialogue (Context C, double width, double height, string text,
                 out double b0_x, out double b0_y, out double b0_width, out double b0_height,
                 out double b1_x, out double b1_y, out double b1_width, out double b1_height)
    {
            const double PI2 = 1.570796326794896619231321691639751442;
            const double border_width = 3;
            
            var lines = 0;
            int font_size = int.MAX;
            for (var s=text; s.length > 0;)
            {
                uint new_line;
                for (new_line = 0; new_line < s.length && s[new_line] != '\n'; ++new_line);
                ++lines;
                if (new_line > 0)
                {
                    var _text = s[0:new_line];
                    double _text_width;
                    double _text_height;
                    int size = calculate_font_size (C, _text, (int)(width *0.8), out _text_width, out _text_height);
                    if (size < font_size)
                        font_size = size;
                }
                if (s.length > new_line+1)
                    s = s[new_line+1:s.length];
                else
                    break;
            }

            double [] line_width = {};
            double [] line_height = {};
            double text_width=0;
            double text_height=0;
            for (var s=text; s.length > 0;)
            {
                uint new_line;
                for (new_line = 0; new_line < s.length && s[new_line] != '\n'; ++new_line);
                if (new_line > 0)
                {
                    var _text = s[0:new_line];
                    Cairo.Context c = new Cairo.Context (C.get_target ());
                    c.move_to (0, 0);
                    c.set_font_size (font_size);
                    Cairo.TextExtents extents;
                    c.text_extents (_text, out extents);
                    line_width += extents.width;
                    line_height += extents.height + 5;
                    if (extents.width > text_width)
                        text_width = extents.width;
                    text_height += extents.height + 5;
                }
                if (s.length > new_line+1)
                    s = s[new_line+1:s.length];
                else
                    break;
            }

            double button_height = text_height / lines * 1.5;
            double minimum_dimension = 10;
            double background_width = text_width + minimum_dimension * 2;
            double background_height = text_height + minimum_dimension * 5 + button_height;
            
            double x = (width - background_width) / 2;
            double y = 0;

            double arc_radius = background_width < background_height ? background_width / 3 : background_height / 3;
            
            /* draw border */
            C.move_to (x + background_width, y);
            C.arc (x + background_width - arc_radius, y + arc_radius, arc_radius, -PI2, 0);
            C.arc (x + background_width - arc_radius, y + background_height - arc_radius, arc_radius, 0, PI2);
            C.arc (x + arc_radius, y + background_height - arc_radius, arc_radius, PI2, PI2 * 2);
            C.arc (x + arc_radius, y + arc_radius, arc_radius, PI2 * 2, -PI2);
            
            C.set_source_rgba (0.5, 0.5, 0.5, 1);
            C.fill ();                

            /* draw background */
            C.arc (x + background_width - arc_radius, y + arc_radius, arc_radius - border_width, -PI2, 0);
            C.arc (x + background_width - arc_radius, y + background_height - arc_radius, arc_radius - border_width, 0, PI2);
            C.arc (x + arc_radius, y + background_height - arc_radius, arc_radius - border_width, PI2, PI2 * 2);
            C.arc (x + arc_radius, y + arc_radius, arc_radius - border_width, PI2 * 2, -PI2);
            
            C.set_source_rgba (0.125, 0.125, 0.125, 1);
            C.fill ();                

            var line = 0;
            double y_line = 0;
            for (var s=text; s.length > 0; ++line)
            {
                uint new_line;
                for (new_line = 0; new_line < s.length && s[new_line] != '\n'; ++new_line);
                if (new_line > 0)
                {
                    y_line += line_height[line];
                    var _text = s[0:new_line];
                    draw_dialogue_text (C, x + (background_width - line_width[line]) / 2,
                        y + minimum_dimension + y_line, _text, font_size);
                }
                if (s.length > new_line+1)
                    s = s[new_line+1:s.length];
                else
                    break;
            }

            /* draw buttons */
            double button_width = background_width / 5 > 100 ? background_width / 5 : 100;
            b0_x = x + (background_width - button_width * 2) / 3;
            b0_y = y + background_height - minimum_dimension - button_height;
            double b0_radius = button_width < button_height ? button_width / 3 : button_height / 3;
            C.move_to (b0_x + button_width, b0_y);
            C.arc (b0_x + button_width - b0_radius, b0_y + b0_radius, b0_radius, -PI2, 0);
            C.arc (b0_x + button_width - b0_radius, b0_y + button_height - b0_radius, b0_radius, 0, PI2);
            C.arc (b0_x + b0_radius, b0_y + button_height - b0_radius, b0_radius, PI2, PI2 * 2);
            C.arc (b0_x + b0_radius, b0_y + b0_radius, b0_radius, PI2 * 2, -PI2);
            C.set_source_rgba (0.5, 0.5, 0.5, 1);
            C.fill ();                
            C.arc (b0_x + button_width - b0_radius, b0_y + b0_radius, b0_radius- border_width, -PI2, 0);
            C.arc (b0_x + button_width - b0_radius, b0_y + button_height - b0_radius, b0_radius- border_width, 0, PI2);
            C.arc (b0_x + b0_radius, b0_y + button_height - b0_radius, b0_radius - border_width, PI2, PI2 * 2);
            C.arc (b0_x + b0_radius, b0_y + b0_radius, b0_radius - border_width, PI2 * 2, -PI2);
            if (mouse_pressed && mouse_button == 0)
                C.set_source_rgba (0.063, 0.243, 0.459, 1);
            else
                C.set_source_rgba (0.082, 0.322, 0.612, 1);
            C.fill ();             
            /* Translators: message displayed in a Button of a Message Dialog to confirm a positive response */
            string Yes = _("Yes");
            font_size = calculate_font_size_from_max (C, Yes, (int)(button_width - border_width * 2), (int)(button_height / 3) , out b0_width, out b0_height);
            draw_dialogue_text (C, b0_x + (button_width - b0_width) / 2 , b0_y + b0_height + button_height / 3, Yes, font_size);

            b1_x = x + (background_width - button_width * 2) / 3 * 2 + button_width;
            b1_y = b0_y;
            double b1_radius = b0_radius;
            C.move_to (b1_x + button_width, b1_y);
            C.arc (b1_x + button_width - b1_radius, b1_y + b1_radius, b1_radius, -PI2, 0);
            C.arc (b1_x + button_width - b1_radius, b1_y + button_height - b1_radius, b1_radius, 0, PI2);
            C.arc (b1_x + b1_radius, b1_y + button_height - b1_radius, b1_radius, PI2, PI2 * 2);
            C.arc (b1_x + b1_radius, b1_y + b1_radius, b1_radius, PI2 * 2, -PI2);
            C.set_source_rgba (0.5, 0.5, 0.5, 1);
            C.fill ();                
            C.arc (b1_x + button_width - b1_radius, b1_y + b1_radius, b1_radius- border_width, -PI2, 0);
            C.arc (b1_x + button_width - b1_radius, b1_y + button_height - b1_radius, b1_radius- border_width, 0, PI2);
            C.arc (b1_x + b1_radius, b1_y + button_height - b1_radius, b1_radius- border_width, PI2, PI2 * 2);
            C.arc (b1_x + b1_radius, b1_y + b1_radius, b1_radius- border_width, PI2 * 2, -PI2);
            if (mouse_pressed && mouse_button == 1)
                C.set_source_rgba (0.063, 0.243, 0.459, 1);
            else
                C.set_source_rgba (0.082, 0.322, 0.612, 1);
            C.fill ();                
            /* Translators: message displayed in a Button of a Message Dialog to confirm a nagative response */
            string No = _("No");
            font_size = calculate_font_size_from_max (C, No, (int)(button_width - border_width * 2), (int)(button_height / 3) , out b1_width, out b1_height);
            draw_dialogue_text (C, b1_x + (button_width - b1_width) / 2 , b1_y + b1_height + button_height / 3, No, font_size);

            /* set width and height to button's width and height not the text within the button */
            b0_width = button_width;
            b1_width = button_width;
            b0_height = button_height;            
            b1_height = button_height;            
    }

    int calculate_font_size (Cairo.Context C, string text, int target_width, out double width, out double height)
    {
        int target_font_size = 1;
        uint target_width_diff = uint.MAX;
        width = 0;
        height = 0;
        
        for (int font_size = 1;font_size < 200;font_size++)
        {
            Cairo.Context c = new Cairo.Context (C.get_target ());
            c.move_to (0, 0);
            c.set_font_size (font_size);
            Cairo.TextExtents extents;
            c.text_extents (text, out extents);
            width = extents.width;
            height = extents.height;
            uint width_diff = (target_width - (int)width).abs ();
            if (width_diff > target_width_diff && width_diff - target_width_diff > 2)
                break;
            else if (width_diff < target_width_diff)
            {
                target_width_diff = width_diff;
                target_font_size = font_size;
            }
        }
        return target_font_size;
    }

    int calculate_font_size_from_max (Cairo.Context C, string text, int max_width, int max_height, out double width, out double height)
    {
        int font_size_result = 1;
        width = 0;
        height = 0;
        
        for (int font_size = 1;font_size < 200;font_size++)
        {
            Cairo.Context c = new Cairo.Context (C.get_target ());
            c.move_to (0, 0);
            c.set_font_size (font_size);
            Cairo.TextExtents extents;
            c.text_extents (text, out extents);
            if (extents.width < max_width && extents.height < max_height)
            {
                width = extents.width;
                height = extents.height;
                font_size_result = font_size;
            }
            else
                break;
        }
        return font_size_result;
    }

    void draw_dialogue_text (Cairo.Context C, double x, double y, string text, int font_size)
    {
        /* draw using x,y as the bottom left corner of the text */
        C.move_to (x, y);
        C.set_font_size (font_size);
        C.set_source_rgba (0.75, 0.75, 0.75, 1);
        C.show_text (text);
    }

    /*\
    * * Colors
    \*/

    internal const int NUM_COLORS = 6;      // only used in preferences-dialog.vala
    private static string[,] color_lookup =
    {
        /* Translators: possible color of a worm, as displayed in the Preferences dialog combobox */
        { "red",    N_("red")    },
        /* Translators: possible color of a worm, as displayed in the Preferences dialog combobox */
        { "green",  N_("green")  },
        /* Translators: possible color of a worm, as displayed in the Preferences dialog combobox */
        { "blue",   N_("blue")   },
        /* Translators: possible color of a worm, as displayed in the Preferences dialog combobox */
        { "yellow", N_("yellow") },
        /* Translators: possible color of a worm, as displayed in the Preferences dialog combobox */
        { "cyan",   N_("cyan")   },
        /* Translators: possible color of a worm, as displayed in the Preferences dialog combobox */
        { "purple", N_("purple") }
    };

    internal static string colorval_name_untranslated (int colorval)
    {
        return color_lookup[colorval, 0];
    }

    /*\
    * * Image loading
    \*/

    internal static Image load_image_file (string pixmap, int xsize, int ysize)
    {
        var filename = GLib.Path.build_filename (PKGDATADIR, "pixmaps", pixmap, null);
        if (filename == null)
            error ("Nibbles couldn't find image file: %s", filename);

        Image image = new Image.from_file (filename);

        return image;
    }
}

