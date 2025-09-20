/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Gnome Nibbles: Gnome Worm Game
 * Copyright (C) 2015 Iulian-Gabriel Radu <iulian.radu67@gmail.com>
 * Copyright (C) 2023-2025 Ben Corby <bcorby@new-ms.com>
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
 * grep -ne '[^][~)(}{_!$ "-](' *.vala
 * grep -ne '[(] ' *.vala
 * grep -ne '[ ])' *.vala
 * grep -ne ' $' *.vala
 *
 */
using Gtk; /* designed for Gtk 4 (version 4.14), link with libgtk-4-dev or gtk4-devel */
using Gsk;

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

internal class NibblesView : TransparentContainer
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
    internal NibblesGame game;

    /* delegate to nibbles-window */
    internal delegate int CountdownActiveFunction ();
    unowned CountdownActiveFunction countdown_active;
    internal delegate bool FullscreenActiveFunction ();
    unowned FullscreenActiveFunction fullscreen_active;

    /* constructor */
    public NibblesView (NibblesGame game,
        CountdownActiveFunction countdown_active,
        FullscreenActiveFunction fullscreen_active)
    {
        this.game = game;
        this.countdown_active = countdown_active;
        this.fullscreen_active = fullscreen_active;
        /* views */
        static_view = new StaticView (this);
        active_view = new ActiveView (this);
        /* overlay */
        Overlay overlay = new Overlay ();
        overlay.set_child (static_view);
        overlay.add_overlay (active_view);
        child = overlay;

        focusable = true;

        /*
        // connect to signals
        this.realize.connect (()=>
        {
        });

        add_tick_callback ((widget, frame_clock)=>
        {
            //var frame_clock = get_frame_clock ();
            var times = frame_clock.get_timings (frame_clock.get_frame_counter ());
            var display_time = times.get_predicted_presentation_time () != 0 ?
                times.get_predicted_presentation_time () : times.get_frame_time ();

            return true;
        });
        */

        connect_game_signals (game);
    }

    /* sub-views */
    StaticView static_view = null;
    internal class StaticView : Widget
    {
        public void redraw ()
        {
            queue_draw ();
        }
        unowned NibblesView view;
        public StaticView (NibblesView view)
        {
            this.view = view;
        }
        Gsk.Path rectangle (uint x, uint y, uint width, uint height)
        {
            var r = new PathBuilder ();
            r.add_rect ({{x, y}, {width, height}});
            return r.to_path ();
        }
        public override void snapshot (Snapshot s)
        {
            if (view.game.three_dimensional_view)
            {
                double x2d, y2d;
                View3D v = new View3D ();
                v.set_view_plain (HEIGHT);
                v.set_view_point ({ (double)WIDTH / 2.0, 2.0 * (double)HEIGHT, 2.0 * (double)HEIGHT});
                v.set_scale_x (get_width () / WIDTH);
                v.set_scale_y (get_height () / HEIGHT);

                /* black background */
                if (view.fullscreen_active ())
                    s.append_fill (rectangle (0, 0, get_width (), get_height ()),
                        EVEN_ODD, {0.0f, 0.0f, 0.0f, 1.0f});
                else
                {
                    var background = new PathBuilder ();
                    v.to_view_plain ({0, 0, 0},out x2d, out y2d);
                    background.move_to ((float)x2d, (float)y2d);
                    v.to_view_plain ({WIDTH, 0, 0},out x2d, out y2d);
                    background.line_to ((float)x2d, (float)y2d);
                    v.to_view_plain ({WIDTH, HEIGHT, 0},out x2d, out y2d);
                    background.line_to ((float)x2d, (float)y2d);
                    v.to_view_plain ({0, HEIGHT, 0},out x2d, out y2d);
                    background.line_to ((float)x2d, (float)y2d);
                    s.append_fill (background.to_path (), EVEN_ODD, {0.0f, 0.0f, 0.0f, 1.0f});
                }
            }
            else
            {
                const double max_delta_deviation = 1.15;
                int x_delta = get_width () / WIDTH;
                int y_delta = get_height () / HEIGHT;
                if (x_delta > max_delta_deviation * y_delta)
                    x_delta = (int)(y_delta * max_delta_deviation);
                else if (y_delta > max_delta_deviation * x_delta)
                    y_delta = (int)(x_delta * max_delta_deviation);
                int x_offset = (get_width () - x_delta * WIDTH) / 2;
                int y_offset = (get_height () - y_delta * HEIGHT) / 2;

                /* black background */
                if (view.fullscreen_active ())
                    s.append_fill (rectangle (0, 0, get_width (), get_height ()),
                        EVEN_ODD, {0.0f, 0.0f, 0.0f, 1.0f});
                else
                    s.append_fill (rectangle (x_offset, y_offset, x_delta * WIDTH, y_delta * HEIGHT),
                        EVEN_ODD, {0.0f, 0.0f, 0.0f, 1.0f});

                /* draw walls */
                for (int x = 0; x < WIDTH; x++)
                {
                    for (int y = 0; y < HEIGHT; y++)
                    {
                        /* walls */
                        if (view.game.board[x, y] >= 'b' && view.game.board[x, y] <= 'l')
                            view.draw_wall_segment (view.game.board[x, y],
                                s, x_delta * x + x_offset, y_delta * y + y_offset, x_delta, y_delta);
                    }
                }
            }
        }
    }
    ActiveView active_view = null;
    internal class ActiveView : Widget
    {
        /* animation */
        uint64 animate = 0;
        /* redraw */
        public void redraw (bool AnimateStep = false)
        {
            if (AnimateStep)
                ++animate;
            queue_draw ();
        }
        unowned NibblesView view;
        public ActiveView (NibblesView view)
        {
            this.view = view;
        }
        public override void snapshot (Snapshot s)
        {
            if (view.game.three_dimensional_view)
            {
                double x2d, y2d;
                double r, g, b;

                View3D v = new View3D ();
                v.set_view_plain (HEIGHT);
                v.set_view_point ({ (double)WIDTH / 2.0, 2.0 * (double)HEIGHT, 2.0 * (double)HEIGHT});
                v.set_scale_x (get_width () / WIDTH);
                v.set_scale_y (get_height () / HEIGHT);

                /* map worms */
                Worm[] dematerialized_worms = {};
                Worm?[,] worm_at = new Worm?[WIDTH, HEIGHT];
                foreach (Worm worm in view.game.worms)
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
                foreach (var bonus in view.game.get_bonuses ())
                        bonus_at[bonus.x, bonus.y] = bonus;

                /* draw */
                for (int y = 0; y < HEIGHT; y++)
                {
                    for (int x = 0; ; x = x < WIDTH / 2 ? WIDTH - 1 - x : WIDTH - x)
                    {
                        if (worm_at[x, y] != null)
                        {
                            get_worm_rgb (view.game.worm_props.@get (worm_at[x, y]).color, true, out r, out g, out b);
                            worm_at[x, y].was_bonus_eaten_at_this_position ((uint16)(x<<8 | y));
                            view.draw_sphere (s, v, x, y, r, g, b, worm_at[x, y].was_bonus_eaten_at_this_position ((uint16)(x<<8 | y)) ? 1.25 : 1);
                            Position head = worm_at[x, y].head;
                            if (head.x == x && head.y == y)
                            {
                                switch (worm_at[x, y].direction)
                                {
                                    case WormDirection.SOUTH:
                                        view.draw_eyes_front (s, v, x, y, animate % 30 / 5 == worm_at[x, y].id);
                                        break;
                                    case WormDirection.EAST:
                                        view.draw_eyes_right (s, v, x, y, animate % 30 / 5 == worm_at[x, y].id);
                                        break;
                                    case WormDirection.WEST:
                                        view.draw_eyes_left (s, v, x, y, animate % 30 / 5 == worm_at[x, y].id);
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                        if (view.game.board[x, y] >= 'b' && view.game.board[x, y] <= 'l')
                        {
                            /* draw top of wall */
                            var wall_top_path = new PathBuilder ();
                            v.to_view_plain ({x, y ,1},out x2d, out y2d);
                            wall_top_path.move_to ( (float)x2d, (float)y2d);
                            v.to_view_plain ({x+1,y,1},out x2d, out y2d);
                            wall_top_path.line_to ( (float)x2d, (float)y2d);
                            v.to_view_plain ({x+1,y+1,1},out x2d, out y2d);
                            wall_top_path.line_to ( (float)x2d, (float)y2d);
                            v.to_view_plain ({x,y+1,1},out x2d, out y2d);
                            wall_top_path.line_to ( (float)x2d, (float)y2d);
                            s.append_fill (wall_top_path.to_path (), EVEN_ODD, {0.95f, 0.95f, 0.95f, 1.0f});

                            /* draw wall inside */
                            var wall_inside_path = new PathBuilder ();
                            if (x < v.view_point_x () && !(view.game.board[x + 1, y] >= 'b' && view.game.board[x + 1, y] <= 'l'))
                            {
                                v.to_view_plain ({x+1,y,0},out x2d, out y2d);
                                wall_inside_path.move_to ( (float)x2d, (float)y2d);
                                v.to_view_plain ({x+1,y+1,0},out x2d, out y2d);
                                wall_inside_path.line_to ( (float)x2d, (float)y2d);
                                v.to_view_plain ({x+1,y+1,1},out x2d, out y2d);
                                wall_inside_path.line_to ( (float)x2d, (float)y2d);
                                v.to_view_plain ({x+1,y,1},out x2d, out y2d);
                                wall_inside_path.line_to ( (float)x2d, (float)y2d);
                                s.append_fill (wall_inside_path.to_path (), EVEN_ODD, {0.5f, 0.5f, 0.5f, 1.0f});
                            }
                            else if (x > v.view_point_x () && !(view.game.board[x - 1, y] >= 'b' && view.game.board[x - 1, y] <= 'l'))
                            {
                                v.to_view_plain ({x,y,0},out x2d, out y2d);
                                wall_inside_path.move_to ( (float)x2d, (float)y2d);
                                v.to_view_plain ({x,y+1,0},out x2d, out y2d);
                                wall_inside_path.line_to ( (float)x2d, (float)y2d);
                                v.to_view_plain ({x,y+1,1},out x2d, out y2d);
                                wall_inside_path.line_to ( (float)x2d, (float)y2d);
                                v.to_view_plain ({x,y,1},out x2d, out y2d);
                                wall_inside_path.line_to ( (float)x2d, (float)y2d);
                                s.append_fill (wall_inside_path.to_path (), EVEN_ODD, {0.5f, 0.5f, 0.5f, 1.0f});
                            }
                            else
                            {
                                /* if we are at the view point we don't need to draw either the left side or the right side of the wall */
                            }
                            /* draw wall front */
                            var wall_front_path = new PathBuilder ();
                            v.to_view_plain ({x,y+1,0},out x2d, out y2d);
                            wall_front_path.move_to ( (float)x2d, (float)y2d);
                            v.to_view_plain ({x+1,y+1,0},out x2d, out y2d);
                            wall_front_path.line_to ( (float)x2d, (float)y2d);
                            v.to_view_plain ({x+1,y+1,1},out x2d, out y2d);
                            wall_front_path.line_to ( (float)x2d, (float)y2d);
                            v.to_view_plain ({x,y+1,1},out x2d, out y2d);
                            wall_front_path.line_to ( (float)x2d, (float)y2d);
                            s.append_fill (wall_front_path.to_path (), EVEN_ODD, {0.95f, 0.95f, 0.95f, 1.0f});
                        }
                        /* warps */
                        if (view.game.board[x + 0, y + 0] == NibblesGame.WARPCHAR &&
                            view.game.board[x + 1, y + 0] == NibblesGame.WARPCHAR &&
                            view.game.board[x + 0, y + 1] == NibblesGame.WARPCHAR &&
                            view.game.board[x + 1, y + 1] == NibblesGame.WARPCHAR)
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
                                s.append_fill (
                                    view.draw_oval (v,
                                         { (double)x + 0.1 * z, (double)y + 0.1 * z, angle[0]},
                                         { (double)x + 2 - 0.1 * z, (double)y + 0.1 * z, angle[1]},
                                         { (double)x + 2 - 0.1 * z, (double)y + 2 - 0.1 * z, angle[2]},
                                         { (double)x + 0.1 * z, (double)y + 2 - 0.1 * z, angle[3]}),
                                     EVEN_ODD, {0.0f, 0.0f, 0.6f - 0.1f * z, 1});
                            }
                        }
                        /* bonus */
                        if (bonus_at [x, y] != null)
                            view.draw_3D_bonus (s, v, x, y, bonus_at [x, y]);

                        /* have we done all the x positions for this line y */
                        if (x == WIDTH / 2)
                            break;
                    }
                }

                if (view.countdown_active () > 0)
                {
                    /* count down */
                    string text = view.seconds_string (view.countdown_active ());
                    double w, h;
                    int font_size = 252;
                    view.calculate_text_size (text, font_size, out w, out h);
                    double center_x, center_y;
                    v.to_view_plain ({WIDTH / 2, HEIGHT / 2, 0}, out center_x, out center_y);
                    view.draw_text_font_size (s, (int)(center_x - w / 2), (int)(center_y - h / 2), text, font_size);

                    /* draw name labels */
                    foreach (var worm in view.game.worms)
                    {
                        if (!worm.list.is_empty)
                        {
                            var color = view.game.worm_props.@get (worm).color;
                            if (worm.direction == WormDirection.UP || worm.direction == WormDirection.DOWN)
                            {
                                /* vertical worm */
                                int middle = worm.length / 2;
                                v.to_view_plain ({ (worm.list[middle] >> 8) + 1.5, (uint8)(worm.list[middle]), 0}, out x2d, out y2d);
                                view.draw_text_target_width (s, (int)x2d, (int)y2d, view.worm_name (worm.id + 1),
                                 (int)v.2D_diff ({ (worm.list[0] >> 8), (uint8)(worm.list[0]), 0},
                                             { (worm.list[worm.length - 1] >> 8), (uint8)(worm.list[worm.length - 1]) + 1, 0}),
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
                                v.to_view_plain ({x, (uint8)(worm.list[0]), 3}, out x_2d[0], out y_2d[0]);
                                v.to_view_plain ({x_max + 1, (uint8)(worm.list[0]), 3}, out x_2d[1], out y_2d[1]);
                                view.draw_text_target_width (s, (int)x_2d[0], (int)y_2d[0], view.worm_name (worm.id + 1), (int)(x_2d[1] - x_2d[0]), color);
                            }
                        }
                    }
                }
            }
            else /* 2D view */
            {
                const double max_delta_deviation = 1.15;
                int x_delta = get_width () / WIDTH;
                int y_delta = get_height () / HEIGHT;
                if (x_delta > max_delta_deviation * y_delta)
                    x_delta = (int)(y_delta * max_delta_deviation);
                else if (y_delta > max_delta_deviation * x_delta)
                    y_delta = (int)(x_delta * max_delta_deviation);
                int x_offset = (get_width () - x_delta * WIDTH) / 2;
                int y_offset = (get_height () - y_delta * HEIGHT) / 2;

                /* draw warps */
                for (int x = 0; x < WIDTH; x++)
                {
                    for (int y = 0; y < HEIGHT; y++)
                    {
                        /* warps */
                        if (view.game.board[x + 0, y + 0] == NibblesGame.WARPCHAR &&
                            view.game.board[x + 1, y + 0] == NibblesGame.WARPCHAR &&
                            view.game.board[x + 0, y + 1] == NibblesGame.WARPCHAR &&
                            view.game.board[x + 1, y + 1] == NibblesGame.WARPCHAR)
                        {
                            view.draw_bonus (s, x_delta * x + x_offset, y_delta * y + y_offset, x_delta + x_delta, y_delta + y_delta, WARP, animate);
                        }
                    }
                }

                /* draw materialized worms */
                var materialized_worm_positions = new Gee.ArrayList<uint16> ();
                int[] dematerialized_worms = {};
                for (int i = 0; i < view.game.worms.size; i++)
                {
                    if (view.game.worms[i].is_materialized)
                        foreach (var position in view.game.worms[i].list)
                        {
                            uint8 x = position >> 8;
                            uint8 y = (uint8)position;
                            view.draw_worm_segment (s, x_delta * x + x_offset, y_delta * y + y_offset, x_delta, y_delta, view.game.worm_props.@get (view.game.worms[i]).color, true, view.game.worms[i].was_bonus_eaten_at_this_position (position));
                            materialized_worm_positions.add (position);
                        }
                    else
                        dematerialized_worms += i;
                }
                /* draw dematerialized worms */
                for (int i = 0; i < dematerialized_worms.length; i++)
                {
                    foreach (var position in view.game.worms[i].list)
                    {
                        if (!materialized_worm_positions.contains (position))
                        {
                            uint8 x = position >> 8;
                            uint8 y = (uint8)position;
                            view.draw_worm_segment (s, x_delta * x + x_offset, y_delta * y + y_offset, x_delta, y_delta, view.game.worm_props.@get (view.game.worms[i]).color, false, false);
                        }
                    }
                }

                /* draw bonuses */
                foreach (var bonus in view.game.get_bonuses ())
                {
                    view.draw_bonus (s, x_delta * bonus.x + x_offset, y_delta * bonus.y + y_offset, x_delta + x_delta, y_delta + y_delta, bonus.etype, animate);
                }

                if (view.countdown_active () > 0)
                {
                    /* count down */
                    string text = view.seconds_string (view.countdown_active ());
                    double w, h;
                    int font_size = 252;
                    view.calculate_text_size (text, font_size, out w, out h);
                    view.draw_text_font_size (s, (int)(x_offset + x_delta * (WIDTH / 2) - w / 2), (int)(y_offset + y_delta * (HEIGHT / 2) - h / 2), text, font_size);

                    /* draw name labels */
                    foreach (var worm in view.game.worms)
                    {
                        if (!worm.list.is_empty)
                        {
                            var color = view.game.worm_props.@get (worm).color;
                            if (worm.direction == WormDirection.UP || worm.direction == WormDirection.DOWN)
                            {
                                /* vertical worm */
                                int middle = worm.length / 2;
                                view.draw_text_target_width (s, x_offset + x_delta * ((worm.list[middle] >> 8) + 1) + x_delta / 2,
                                              y_offset + y_delta * ((uint8)worm.list[middle]),
                                              view.worm_name (worm.id + 1), x_delta * worm.length, color);
                            }
                            else if (worm.direction == WormDirection.LEFT || worm.direction == WormDirection.RIGHT)
                            {
                                /* horizontal worm */
                                int x = worm.list[0] >> 8;
                                if (x > worm.list[worm.length-1] >> 8)
                                    x = worm.list[worm.length-1] >> 8;
                                view.draw_text_target_width (s, x_offset + x_delta * x,
                                              y_offset + y_delta * ((uint8)worm.list[0]) - y_delta,
                                              view.worm_name (worm.id + 1), x_delta * worm.length, color);
                            }
                        }
                    }
                }
            }
        }
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
        if (null != active_view)
            active_view.redraw (AnimateStep);
    }
    public void redraw_all ()
    {
        if (null != active_view)
            active_view.redraw (false);
        if (null != static_view)
            static_view.redraw ();
    }

    /* signals */
    internal void connect_game_signals (NibblesGame game)
    {
        game.redraw.connect (redraw);
    }

    /* private functions */

    Gsk.Path draw_oval (View3D v, Point3D a, Point3D b, Point3D c, Point3D d)
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
        var path = new PathBuilder ();
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
                path.move_to ( (float)x[0], (float)y[0]);
            path.cubic_to ( (float)x[0], (float)y[0], (float)x[1], (float)y[1], (float)x[2], (float)y[2]);
            i = (uint)next_index;
            if (i == second_lowest_y_midpoint_index)
                break;
        }
        return path.to_path ();
    }

    Point3D mid_point (Point3D a, Point3D b)
    {
        return {a.x > b.x ? (a.x - b.x) / 2 + b.x : (b.x - a.x) / 2 + a.x,
                a.y > b.y ? (a.y - b.y) / 2 + b.y : (b.y - a.y) / 2 + a.y,
                a.z > b.z ? (a.z - b.z) / 2 + b.z : (b.z - a.z) / 2 + a.z};
    }

    void draw_sphere (Snapshot s, View3D v, int x, int y, double r, double g, double b, double size)
    {
        double increase = (size - 1) / 2;
        var path = draw_oval (v, {x + 0 - increase, y + 1.0 + increase, 0 - increase},
                      {x + 0 - increase, y + 0.0 - increase, 1 + increase},
                      {x + 1 + increase, y + 0.0 - increase, 1 + increase},
                      {x + 1 + increase, y + 1.0 + increase, 0 - increase});
        double radius_x, radius_y, d, top_x, top_y;;
        radius_y = v.2D_diff ({x + 0.5, y + 0.5, 0.5}, {x + 0.5, y + 0.0 - increase, 1 + increase});
        d = v.2D_diff ({x + 0.5, y + 0.5, 0.5}, {x + 0.5, y + 1.0 + increase, 0.0 - increase});
        if (d > radius_y)
            radius_y = d;
        radius_x = v.2D_diff ({x + 0.5, y + 0.5, 0.5}, {x + 0.0 - increase, y + 0.5, 0.5});
        d = v.2D_diff ({x + 0.5, y + 0.5, 0.5}, {x + 1.0 + increase, y + 0.5, 0.5});
        if (d > radius_x)
            radius_x = d;
        v.to_view_plain ({x + 0.5, y + 0.5, 1}, out top_x, out top_y);
        s.push_fill (path, EVEN_ODD);
        s.append_radial_gradient (get_bounds (path),
            {(float)top_x, (float)top_y},
            (float)radius_x / 80.0f, (float)radius_y / 80.0f,
            0, 100,
            {
                {0, {(1 - (float)r) / 2 + (float)r, (1 - (float)g) / 2 + (float)g, (1 - (float)b) / 2 + (float)b, 1}},
                {0.1f, {(float)r, (float)g, (float)b, 1}},
                {1, {(float)r / 10.0f, (float)g / 10.0f, (float)b / 10.0f, 1}},
            });
        s.pop ();
    }

    void draw_eyes_front (Snapshot s, View3D v, int x, int y, bool blink)
    {
        /* eyes look ahead */
        double e = Math.sqrt (0.125);
        double b = blink ? 0.025 : 0.1;
        Point3D centre = {x + 0.5 - e / 2, y + 0.5 + e, 0.5 + e / 2};
        s.append_fill (draw_oval (v, {centre.x - 0.1, centre.y + b, centre.z - b},
                         {centre.x - 0.1, centre.y - b, centre.z + b},
                         {centre.x + 0.1, centre.y - b, centre.z + b},
                         {centre.x + 0.1, centre.y + b, centre.z - b}),
                       EVEN_ODD, {0.0f, 0.0f, 0.0f, 1.0f});                         
        centre = {x + 0.5 + e / 2, y + 0.5 + e, 0.5 + e / 2};
        s.append_fill (draw_oval (v, {centre.x - 0.1, centre.y + b, centre.z - b},
                         {centre.x - 0.1, centre.y - b, centre.z + b},
                         {centre.x + 0.1, centre.y - b, centre.z + b},
                         {centre.x + 0.1, centre.y + b, centre.z - b}),
                       EVEN_ODD, {0.0f, 0.0f, 0.0f, 1.0f});                         
    }

    void draw_eyes_left (Snapshot s, View3D v, int x, int y, bool blink)
    {
        /* eyes look left */
        double e = Math.sqrt (0.125);
        double b = blink ? 0.025 : 0.1;
        Point3D centre = {x + 0.5 - e, y + 0.5 - e / 2, 0.5 + e / 2};
        s.append_fill (draw_oval (v, {centre.x - b, centre.y - 0.1, centre.z - b},
                         {centre.x + b, centre.y - 0.1, centre.z + b},
                         {centre.x + b, centre.y + 0.1, centre.z + b},
                         {centre.x - b, centre.y + 0.1, centre.z - b}),
                       EVEN_ODD, {0.0f, 0.0f, 0.0f, 1.0f});                         
        centre = {x + 0.5 - e, y + 0.5 + e / 2, 0.5 + e / 2};
        s.append_fill (draw_oval (v, {centre.x - b, centre.y - 0.1, centre.z - b},
                         {centre.x + b, centre.y - 0.1, centre.z + b},
                         {centre.x + b, centre.y + 0.1, centre.z + b},
                         {centre.x - b, centre.y + 0.1, centre.z - b}),
                       EVEN_ODD, {0.0f, 0.0f, 0.0f, 1.0f});                         
    }

    void draw_eyes_right (Snapshot s, View3D v, int x, int y, bool blink)
    {
        /* eyes look right */
        double e = Math.sqrt (0.125);
        double b = blink ? 0.025 : 0.1;
        Point3D centre = {x + 0.5 + e, y + 0.5 - e / 2, 0.5 + e / 2};
        s.append_fill (draw_oval (v, {centre.x + b, centre.y - 0.1, centre.z - b},
                         {centre.x - b, centre.y - 0.1, centre.z + b},
                         {centre.x - b, centre.y + 0.1, centre.z + b},
                         {centre.x + b, centre.y + 0.1, centre.z - b}),
                       EVEN_ODD, {0.0f, 0.0f, 0.0f, 1.0f});                         
        centre = {x + 0.5 + e, y + 0.5 + e / 2, 0.5 + e / 2};
        s.append_fill (draw_oval (v, {centre.x + b, centre.y - 0.1, centre.z - b},
                         {centre.x - b, centre.y - 0.1, centre.z + b},
                         {centre.x - b, centre.y + 0.1, centre.z + b},
                         {centre.x + b, centre.y + 0.1, centre.z - b}),
                       EVEN_ODD, {0.0f, 0.0f, 0.0f, 1.0f});                         
    }

    void draw_3D_bonus (Snapshot s, View3D v, int x, int y, Bonus bonus)
    {
        switch (bonus.etype)
        {
            case REGULAR: // apple
                draw_apple (s, v, x, y);
                break;
            case HALF: // cherry
                draw_cherry (s, v, x, y);
                break;
            case DOUBLE: // banana
                draw_banana (s, v, x, y);
                break;
            case LIFE: // heart
                draw_heart (s, v, x, y);
                break;
            case REVERSE: // diamond
                draw_diamond (s, v, x, y);
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

    void draw_banana (Snapshot s, View3D v, double x, double y)
    {
        double cx[3], cy[3];
        var dark_side = new PathBuilder ();
        v.to_view_plain ({x + 0, y + 1 + 0.25, 1}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 0, y + 1 + 0.25, 0}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1, y + 1 + 0.25, 0}, out cx[2], out cy[2]);
        dark_side.move_to ( (float)cx[0], (float)cy[0]);
        dark_side.cubic_to ( (float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        v.to_view_plain ({x + 1, y + 1 + 0.25, 0}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 2, y + 1 + 0.25, 0}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 2, y + 1 + 0.25, 1}, out cx[2], out cy[2]);
        dark_side.cubic_to ( (float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        v.to_view_plain ({x + 2 - 0.1, y + 1 + 0.25, 1 + 0.5}, out cx[0], out cy[0]);
        dark_side.line_to ( (float)cx[0], (float)cy[0]);
        v.to_view_plain ({x + 2 - 0.2, y + 1 + 0.25, 1 + 0.5}, out cx[0], out cy[0]);
        dark_side.line_to ( (float)cx[0], (float)cy[0]);

        v.to_view_plain ({x + 2 - 0.2, y + 1 + 0.25, 1 + 0.5}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 2 - 0.2, y + 1 + 0.25, 0.5 }, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1, y + 1 + 0.25, 0.5}, out cx[2], out cy[2]);
        dark_side.cubic_to ( (float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);

        v.to_view_plain ({x + 1, y + 1 + 0.25, 0.5}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 0.2, y + 1 + 0.25, 0.5}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 0.1, y + 1 + 0.25, 1 + 0.1}, out cx[2], out cy[2]);
        dark_side.cubic_to ( (float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        s.append_fill (dark_side.to_path (), EVEN_ODD, {0.6f, 0.6f, 0.0f, 1.0f}/* yellow */);

        var light_side = new PathBuilder ();
        v.to_view_plain ({x + 0.1, y + 1 + 0.25, 1 + 0.1}, out cx[0], out cy[0]);
        light_side.move_to ( (float)cx[0], (float)cy[0]);
        v.to_view_plain ({x + 0.1, y + 1 + 0.15, 1 + 0.1}, out cx[0], out cy[0]);
        light_side.line_to ( (float)cx[0], (float)cy[0]);

        v.to_view_plain ({x + 0.1, y + 1 + 0.15, 1 + 0.1}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 0.2, y + 1 + 0.15, 0.5}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1, y + 1 + 0.15, 0.5}, out cx[2], out cy[2]);
        light_side.cubic_to ( (float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);

        v.to_view_plain ({x + 1, y + 1 + 0.15, 0.5}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 2 - 0.2, y + 1 + 0.15, 0.5 }, out cx[1], out cy[1]);
        v.to_view_plain ({x + 2 - 0.2, y + 1 + 0.15, 1 + 0.5}, out cx[2], out cy[2]);
        light_side.cubic_to ( (float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);

        v.to_view_plain ({x + 2 - 0.2, y + 1 + 0.15, 1 + 0.5}, out cx[0], out cy[0]);
        light_side.line_to ( (float)cx[0], (float)cy[0]);
        v.to_view_plain ({x + 2 - 0.1, y + 1 + 0.15, 1 + 0.5}, out cx[0], out cy[0]);
        light_side.line_to ( (float)cx[0], (float)cy[0]);
        v.to_view_plain ({x + 2 - 0.1, y + 1 + 0.25, 1 + 0.5}, out cx[0], out cy[0]);
        light_side.line_to ( (float)cx[0], (float)cy[0]);
        v.to_view_plain ({x + 2 - 0.2, y + 1 + 0.25, 1 + 0.5}, out cx[0], out cy[0]);
        light_side.line_to ( (float)cx[0], (float)cy[0]);

        v.to_view_plain ({x + 2 - 0.2, y + 1 + 0.25, 1 + 0.5}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 2 - 0.2, y + 1 + 0.25, 0.5 }, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1, y + 1 + 0.25, 0.5}, out cx[2], out cy[2]);
        light_side.cubic_to ( (float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);

        v.to_view_plain ({x + 1, y + 1 + 0.25, 0.5}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 0.2, y + 1 + 0.25, 0.5}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 0.1, y + 1 + 0.25, 1 + 0.1}, out cx[2], out cy[2]);
        light_side.cubic_to ( (float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        s.append_fill (light_side.to_path (), EVEN_ODD, {0.8f, 0.8f, 0.0f, 1.0f}/* yellow */);

        var path = new PathBuilder ();
        v.to_view_plain ({x + 0, y + 1 + 0.25, 1}, out cx[0], out cy[0]);
        path.move_to ( (float)cx[0], (float)cy[0]);
        v.to_view_plain ({x + 0.1, y + 1 + 0.25, 1 + 0.1}, out cx[0], out cy[0]);
        path.line_to ( (float)cx[0], (float)cy[0]);
        v.to_view_plain ({x + 0.1, y + 1 + 0.15, 1 + 0.1}, out cx[0], out cy[0]);
        path.line_to ( (float)cx[0], (float)cy[0]);
        v.to_view_plain ({x + 0, y + 1 + 0.15, 1}, out cx[0], out cy[0]);
        path.line_to ( (float)cx[0], (float)cy[0]);
        s.append_fill (path.to_path (), EVEN_ODD, {0.3f, 0.3f, 0.4f, 1.0f});
    }

    void draw_diamond (Snapshot s, View3D v, double x, double y)
    {
        const double cos60 = 0.5;
        const double sin60 = 0.866025403784;
        Point3D top[6] = {{x + 0, y + 1, 1.5}, {x + cos60, y + 1.0 - sin60, 1.5}, {x + cos60 + 1, y + 1.0 - sin60, 1.5},
            {x + 2, y + 1, 1.5}, {x + cos60 + 1, y + 1.0 + sin60, 1.5}, {x + cos60, y + 1.0 + sin60, 1.5}};
        Point3D middle[6] = {{x + 0.5, y + 1, 2}, {x + (1.0 - 0.5 * cos60), y + 1.0 - 0.5 * sin60, 2}, {x + 0.5 * cos60 + 1, y + 1.0 - 0.5 * sin60, 2},
            {x + 1.5, y + 1, 2}, {x + 0.5 * cos60 + 1, y + 1.0 + 0.5 * sin60, 2}, {x + (1.0 - 0.5 * cos60), y + 1.0 + 0.5 * sin60, 2}};
        double X, Y;

        var p0 = new PathBuilder ();
        v.to_view_plain (top[0], out X, out Y);
        p0.move_to ( (float)X, (float)Y);
        v.to_view_plain (top[1], out X, out Y);
        p0.line_to ( (float)X, (float)Y);
        v.to_view_plain (top[2], out X, out Y);
        p0.line_to ( (float)X, (float)Y);
        v.to_view_plain (top[3], out X, out Y);
        p0.line_to ( (float)X, (float)Y);
        v.to_view_plain (top[4], out X, out Y);
        p0.line_to ( (float)X, (float)Y);
        v.to_view_plain (top[5], out X, out Y);
        p0.line_to ( (float)X, (float)Y);
        s.append_fill (p0.to_path (), EVEN_ODD, {0.8f, 0.9f, 1.0f, 1.0f}/* almost white */);

        var p1 = new PathBuilder ();
        v.to_view_plain (middle[1], out X, out Y);
        p1.move_to ( (float)X, (float)Y);
        v.to_view_plain (middle[2], out X, out Y);
        p1.line_to ( (float)X, (float)Y);
        v.to_view_plain (top[2], out X, out Y);
        p1.line_to ( (float)X, (float)Y);
        v.to_view_plain (top[1], out X, out Y);
        p1.line_to ( (float)X, (float)Y);
        s.append_fill (p1.to_path (), EVEN_ODD, {0.618f, 0.708f, 0.802f, 1.0f}/* light blue back */);

        var p2 = new PathBuilder ();
        v.to_view_plain (middle[1], out X, out Y);
        p2.move_to ( (float)X, (float)Y);
        v.to_view_plain (middle[0], out X, out Y);
        p2.line_to ( (float)X, (float)Y);
        v.to_view_plain (top[0], out X, out Y);
        p2.line_to ( (float)X, (float)Y);
        v.to_view_plain (top[1], out X, out Y);
        p2.line_to ( (float)X, (float)Y);
        s.append_fill (p2.to_path (), EVEN_ODD, {0.347f, 0.524f, 0.712f, 1.0f}/* darker blue back */);

        var p3 = new PathBuilder ();
        v.to_view_plain (middle[2], out X, out Y);
        p3.move_to ( (float)X, (float)Y);
        v.to_view_plain (middle[3], out X, out Y);
        p3.line_to ( (float)X, (float)Y);
        v.to_view_plain (top[3], out X, out Y);
        p3.line_to ( (float)X, (float)Y);
        v.to_view_plain (top[2], out X, out Y);
        p3.line_to ( (float)X, (float)Y);
        s.append_fill (p3.to_path (), EVEN_ODD, {0.347f, 0.524f, 0.712f, 1.0f}/* darker blue back */);

        var p4 = new PathBuilder ();
        v.to_view_plain (middle[3], out X, out Y);
        p4.move_to ( (float)X, (float)Y);
        v.to_view_plain (middle[4], out X, out Y);
        p4.line_to ( (float)X, (float)Y);
        v.to_view_plain (top[4], out X, out Y);
        p4.line_to ( (float)X, (float)Y);
        v.to_view_plain (top[3], out X, out Y);
        p4.line_to ( (float)X, (float)Y);
        s.append_fill (p4.to_path (), EVEN_ODD, {0.447f, 0.624f, 0.812f, 1.0f}/* darker blue front */);

        var p5 = new PathBuilder ();
        v.to_view_plain (middle[5], out X, out Y);
        p5.move_to ( (float)X, (float)Y);
        v.to_view_plain (middle[0], out X, out Y);
        p5.line_to ( (float)X, (float)Y);
        v.to_view_plain (top[0], out X, out Y);
        p5.line_to ( (float)X, (float)Y);
        v.to_view_plain (top[5], out X, out Y);
        p5.line_to ( (float)X, (float)Y);
        s.append_fill (p5.to_path (), EVEN_ODD, {0.447f, 0.624f, 0.812f, 1.0f}/* darker blue front */);

        var p6 = new PathBuilder ();
        v.to_view_plain (middle[4], out X, out Y);
        p6.move_to ( (float)X, (float)Y);
        v.to_view_plain (middle[5], out X, out Y);
        p6.line_to ( (float)X, (float)Y);
        v.to_view_plain (top[5], out X, out Y);
        p6.line_to ( (float)X, (float)Y);
        v.to_view_plain (top[4], out X, out Y);
        p6.line_to ( (float)X, (float)Y);
        s.append_fill (p6.to_path (), EVEN_ODD, {0.718f, 0.808f, 0.902f, 1.0f}/* light blue front */);
    }

    void draw_heart (Snapshot s, View3D v, double x, double y)
    {
        double H = Math.sqrt (0.125);
        double h = 0.5 - H;
        double cx[3], cy[3];
        var path = new PathBuilder ();
        v.to_view_plain ({x + h, y + 1, 1 + h}, out cx[0], out cy[0]);
        v.to_view_plain ({x - 0.207106781187, y + 1, 1.5}, out cx[1], out cy[1]);
        v.to_view_plain ({x + h, y + 1, 2.0 - h}, out cx[2], out cy[2]);
        path.move_to ((float)cx[0], (float)cy[0]);
        path.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        v.to_view_plain ({x + h, y + 1, 2.0 - h}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 0.5, y + 1, 2.0 + 0.207106781187}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1 - h, y + 1, 2.0 - h}, out cx[2], out cy[2]);
        path.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        v.to_view_plain ({x + 1 - h, y + 1, 2.0 - h}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1, y + 1, ((2.0 - h) - 1.5) / 2 + 1.5}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1, y + 1, 1.5}, out cx[2], out cy[2]);
        path.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        v.to_view_plain ({x + 1, y + 1, 1.5}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1, y + 1, ((2.0 - h) - 1.5) / 2 + 1.5}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1 + h, y + 1, 2.0 - h}, out cx[2], out cy[2]);
        path.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        v.to_view_plain ({x + 1 + h, y + 1, 2.0 - h}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1.5, y + 1, 2.0 + 0.207106781187}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 2 - h, y + 1, 2.0 - h}, out cx[2], out cy[2]);
        path.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        v.to_view_plain ({x + 2 - h, y + 1, 2.0 - h}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 2 + 0.207106781187, y + 1, 1.5}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 2 - h, y + 1, 1 + h}, out cx[2], out cy[2]);
        path.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        v.to_view_plain ({x + 1, y + 1, 0}, out cx[0], out cy[0]);
        path.line_to ((float)cx[0], (float)cy[0]);
        v.to_view_plain ({x + h, y + 1, 1 + h}, out cx[0], out cy[0]);
        path.line_to ((float)cx[0], (float)cy[0]);
        v.to_view_plain ({x + 1, y + 1, 1 + h}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 0.875, y + 1, 1.5}, out cx[1], out cy[1]);
        double radius = v.2D_diff ({x + 1, y + 1, 1 + h}, {x + 1, y + 1, 0.0});
        
        s.push_fill (path.to_path (), EVEN_ODD);
        s.append_radial_gradient (get_bounds (path.to_path ()),
            {(float)cx[0], (float)cy[0]},
            (float)radius / 40.0f, (float)radius / 40.0f,
            0, 100,
            {
                {0, {1, 0.5f, 0.5f, 1}},
                {0.1f, {1, 0, 0, 1}},
                {1, {1 / 10, 0, 0, 1}}
            });
        s.pop ();
    }

    void draw_cherry (Snapshot s, View3D v, double x, double y)
    {
        // draw left cherry */
        var left_cherry = draw_oval (v, {x, 1.5 + y, 0},
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
        s.push_fill (left_cherry, EVEN_ODD);
        s.append_radial_gradient (get_bounds (left_cherry),
            {(float)top_x, (float)top_y},
            (float)radius / 80.0f, (float)radius / 80.0f,
            0, 100,
            {
                {0,    {1,    0.5f, 0.5f, 1}},
                {0.1f, {1,    0,    0,    1}},
                {1,    {0.1f, 0,    0,    1}}
            });
        s.pop ();
        // draw left stalk */
        var left_stalk = new PathBuilder ();
        double cx[3];
        double cy[3];
        v.to_view_plain ({x + 0.45, y + 1.0, 1.0}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 0.45, y + 1.0, 2.0}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1.15, y + 1.0, 2.0}, out cx[2], out cy[2]);
        left_stalk.move_to ((float)cx[0], (float)cy[0]);
        left_stalk.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        v.to_view_plain ({x + 1.25, y + 1.0, 2.0}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 0.55, y + 1.0, 2.0}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 0.55, y + 1.0, 1.0}, out cx[2], out cy[2]);
        left_stalk.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        s.append_fill (left_stalk.to_path (), EVEN_ODD, {0.5f, 0.5f, 0.0f, 1.0f});
        // draw right cherry */
        var right_cherry = draw_oval (v, {1.0 + x, 1.5 + y, 0},
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
        s.push_fill (right_cherry, EVEN_ODD);
        s.append_radial_gradient (get_bounds (right_cherry),
            {(float)top_x, (float)top_y},
            (float)radius / 80.0f, (float)radius / 80.0f,
            0, 100,
            {
                {0,    {1,    0.5f, 0.5f, 1}},
                {0.1f, {1,    0,    0,    1}},
                {1,    {0.1f, 0,    0,    1}}
            });
        s.pop ();
        // draw right stalk */
        var right_stalk = new PathBuilder ();
        v.to_view_plain ({x + 1.45, y + 1.0, 1.0}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1.45, y + 1.0, 2.0}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1.15, y + 1.0, 2.0}, out cx[2], out cy[2]);
        right_stalk.move_to ((float)cx[0], (float)cy[0]);
        right_stalk.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        v.to_view_plain ({x + 1.25, y + 1.0, 2.0}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1.55, y + 1.0, 2.0}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1.55, y + 1.0, 1.0}, out cx[2], out cy[2]);
        right_stalk.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        s.append_fill (right_stalk.to_path (), EVEN_ODD, {0.5f, 0.5f, 0.0f, 1.0f});
        /* draw leaf */
        var leaf = new PathBuilder ();
        v.to_view_plain ({x + 1.20, y + 1.0, 2.0}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1.20 - 0.5, y + 1.0, 2.0}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1.20 - 0.5, y + 1.0 - 0.5, 2.0}, out cx[2], out cy[2]);
        leaf.move_to ((float)cx[0], (float)cy[0]);
        leaf.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        v.to_view_plain ({x + 1.20 - 0.5, y + 1.0 - 0.5, 2.0}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1.20, y + 1.0 - 0.5, 2.0}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1.20, y + 1.0, 2.0}, out cx[2], out cy[2]);
        leaf.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        s.append_fill (leaf.to_path (), EVEN_ODD, {0.0f, 0.75f, 0.0f, 1.0f});
    }

    void draw_apple (Snapshot s, View3D v, double x, double y)
    {
        double cx[3];
        double cy[3];
        double top_x, top_y;
        var apple = new PathBuilder ();
        v.to_view_plain ({x + 0, y + 1, 1}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 0, y + 1, 0}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1, y + 1, 0}, out cx[2], out cy[2]);
        apple.move_to ((float)cx[0], (float)cy[0]);
        apple.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        v.to_view_plain ({x + 1, y + 1, 0}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 2, y + 1, 0}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 2, y + 1, 1}, out cx[2], out cy[2]);
        apple.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        v.to_view_plain ({x + 2, y + 1, 1}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 2, y + 1.0, 2}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1.0, y + 1, 2}, out cx[2], out cy[2]);
        apple.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        v.to_view_plain ({x + 1.0, y + 1, 2}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 0, y + 1, 2}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 0, y + 1, 1}, out cx[2], out cy[2]);
        apple.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);

        v.to_view_plain ({x + 1, y + 1.0, 1.5}, out top_x, out top_y);
        double radius = v.2D_diff ({x + 1, y + 1.0, 1.5}, {x + 1, y + 1.0, 0});
        var path = apple.to_path ();
        s.push_fill (path, EVEN_ODD);
        s.append_radial_gradient (get_bounds (path),
            {(float)top_x, (float)top_y},
            (float)radius / 80.0f, (float)radius / 80.0f,
            0, 100,
            {
                {0,    {0.5f, 1,    0.5f, 1}},
                {0.1f, {0,    1,    0,    1}},
                {1,    {0,    0.1f, 0,    1}}
            });
        s.pop ();

        for (double 3D_radius = 0.2;3D_radius > 0.01;3D_radius-=0.01)
        {
            apple = new PathBuilder ();
            v.to_view_plain ({x + (1 - 3D_radius), y + 1.0, 1.75}, out cx[0], out cy[0]);
            v.to_view_plain ({x + (1 - 3D_radius), y + (1 - 3D_radius), 1.75}, out cx[1], out cy[1]);
            v.to_view_plain ({x + 1.00, y + (1 - 3D_radius), 1.75}, out cx[2], out cy[2]);
            apple.move_to ((float)cx[0], (float)cy[0]);
            apple.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
            v.to_view_plain ({x + 1.00, y + (1 - 3D_radius), 1.75}, out cx[0], out cy[0]);
            v.to_view_plain ({x + (1 + 3D_radius), y + (1 - 3D_radius), 1.75}, out cx[1], out cy[1]);
            v.to_view_plain ({x + (1 + 3D_radius), y + 1.0, 1.75}, out cx[2], out cy[2]);
            apple.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
            v.to_view_plain ({x + (1 + 3D_radius), y + 1.0, 1.75}, out cx[0], out cy[0]);
            v.to_view_plain ({x + (1 + 3D_radius), y + (1 + 3D_radius), 1.75}, out cx[1], out cy[1]);
            v.to_view_plain ({x + 1, y + (1 + 3D_radius), 1.75}, out cx[2], out cy[2]);
            apple.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
            v.to_view_plain ({x + 1, y + (1 + 3D_radius), 1.75}, out cx[0], out cy[0]);
            v.to_view_plain ({x + (1 - 3D_radius), y + (1 + 3D_radius), 1.75}, out cx[1], out cy[1]);
            v.to_view_plain ({x + (1 - 3D_radius), y + 1.0, 1.75}, out cx[2], out cy[2]);
            apple.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
            s.append_fill (apple.to_path (), EVEN_ODD, {0.4f, 0.8f - (0.2f - (float)3D_radius) * 3, 0.4f - (0.2f - (float)3D_radius) * 2, 1.0f});
        }

        /* draw left leaf */
        var left_leaf = new PathBuilder ();
        v.to_view_plain ({x + 1.0, y + 1.0, 1.75}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1.0 - 0.5, y + 1.0, 1.75}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1.0 - 0.5, y + 1.0 - 0.5, 1.75}, out cx[2], out cy[2]);
        left_leaf.move_to ((float)cx[0], (float)cy[0]);
        left_leaf.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        v.to_view_plain ({x + 1.0 - 0.5, y + 1.0 - 0.5, 1.75}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1.0, y + 1.0 - 0.5, 1.75}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1.0, y + 1.0, 1.75}, out cx[2], out cy[2]);
        left_leaf.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        s.append_fill (left_leaf.to_path (), EVEN_ODD, {0.25f, 0.5f, 0.0f, 1});
        /* draw right leaf */
        var right_leaf = new PathBuilder ();
        v.to_view_plain ({x + 1.0, y + 1.0, 1.75}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1.0 + 0.5, y + 1.0, 1.75}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1.0 + 0.5, y + 1.0 - 0.5, 1.75}, out cx[2], out cy[2]);
        right_leaf.move_to ((float)cx[0], (float)cy[0]);
        right_leaf.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        v.to_view_plain ({x + 1.0 + 0.5, y + 1.0 - 0.5, 1.75}, out cx[0], out cy[0]);
        v.to_view_plain ({x + 1.0, y + 1.0 - 0.5, 1.75}, out cx[1], out cy[1]);
        v.to_view_plain ({x + 1.0, y + 1.0, 1.75}, out cx[2], out cy[2]);
        right_leaf.cubic_to ((float)cx[0], (float)cy[0], (float)cx[1], (float)cy[1], (float)cx[2], (float)cy[2]);
        s.append_fill (right_leaf.to_path (), EVEN_ODD, {0.0f, 0.5f, 0.25f, 1});
    }

    void draw_wall_segment (int i, Snapshot s, int x, int y, int x_size, int y_size)
    {
        int x_s13 = x_size / 3;
        int x_remainder = x_size - x_s13 * 3;
        int y_s13 = y_size / 3;
        int y_remainder = y_size - y_s13 * 3;
        if (i >= 'b' && i <= 'l')
        {
            /* center square */
            var center_square = new PathBuilder ();
            center_square.add_rect ({{x_remainder == 2 ? x_s13 + x + x_remainder : x_s13 + x,
                y_remainder == 2 ? y_s13 + y + y_remainder : y_s13 + y},
                {x_remainder == 2 ? x_s13 : x_s13 + x_remainder,
                y_remainder == 2 ? y_s13 : y_s13 + y_remainder}});
            s.append_fill (center_square.to_path (), EVEN_ODD, {0.5f, 0.5f, 0.5f, 1.0f});
        }
        if (i == 'b' || i == 'd' || i == 'e' || i == 'h' || i == 'i' || i == 'j' || i == 'l')
        {
            /* top square */
            var top_square = new PathBuilder ();
            top_square.add_rect ({{x_remainder == 2 ? x_s13 + x + x_remainder : x_s13 + x,
                y},
                {x_remainder == 2 ? x_s13 : x_s13 + x_remainder,
                y_remainder == 2 ? y_s13 + y_remainder : y_s13}});
            s.append_fill (top_square.to_path (), EVEN_ODD, {0.5f, 0.5f, 0.5f, 1.0f});
        }
        if (i == 'c' || i == 'd' || i == 'f' || i == 'h' || i == 'i' || i == 'k' || i == 'l')
        {
            /* right square */
            var right_square = new PathBuilder ();
            right_square.add_rect ({{x_s13 + x_s13 + x_remainder + x,
                y_remainder == 2 ? y_s13 + y_remainder + y : y_s13 + y},
                {x_remainder == 2 ? x_s13 + x_remainder : x_s13,
                y_remainder == 2 ? y_s13 : y_s13 + y_remainder}});
            s.append_fill (right_square.to_path (), EVEN_ODD, {0.5f, 0.5f, 0.5f, 1.0f});
        }
        if (i == 'b' || i == 'f' || i == 'g' || i == 'i' || i == 'j' || i == 'k' || i == 'l')
        {
            /* bottom square */
            var bottom_square = new PathBuilder ();
            bottom_square.add_rect ({{x_remainder == 2 ? x_s13 + x + x_remainder : x_s13 + x,
                y_s13 + y_s13 + y_remainder + y},
                {x_remainder == 2 ? x_s13 : x_s13 + x_remainder,
                y_remainder == 2 ? y_s13 + y_remainder : y_s13}});
            s.append_fill (bottom_square.to_path (), EVEN_ODD, {0.5f, 0.5f, 0.5f, 1.0f});
        }
        if (i == 'c' || i == 'e' || i == 'g' || i == 'h' || i == 'j' || i == 'k' || i == 'l')
        {
            /* left square */
            var left_square = new PathBuilder ();
            left_square.add_rect ({{x,
                y_remainder == 2 ? y_s13 + y + y_remainder : y_s13 + y},
                {x_remainder == 2 ? x_s13 + x_remainder : x_s13,
                y_remainder == 2 ? y_s13 : y_s13 + y_remainder}});
            s.append_fill (left_square.to_path (), EVEN_ODD, {0.5f, 0.5f, 0.5f, 1.0f});
        }
    }

    void draw_worm_segment (Snapshot s, int x, int y, int x_size, int y_size, int color, bool is_materialized, bool eaten_bonus)
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

        const float PI2 = 1.570796326794896619231321691639751442f;
        float x_s13 = x_size / 3.0f;
        float x_s23 = x_s13 + x_s13;
        float y_s13 = y_size / 3.0f;
        float y_s23 = y_s13 + y_s13;
        var path = new PathBuilder ();
        /* top right corner */
        path.move_to (x + x_s23, y + 0);
        path.svg_arc_to (x_s13, y_s13, PI2, false, true, x + x_size, y + y_s13);
        /* bottom right corner */
        path.line_to (x + x_size, y + y_s23);
        path.svg_arc_to (x_s13, y_s13, PI2, false, true, x + x_s23, y + y_size);
        /* bottom left corner */
        path.line_to (x + x_s13, y + y_size);
        path.svg_arc_to (x_s13, y_s13, PI2, false, true, x + 0, y + y_s23);
        /* top left corner */
        path.line_to (x + 0, y + y_s13);
        path.svg_arc_to (x_s13, y_s13, PI2, false, true, x + x_s13, y + 0);
        /* fill */
        double r,g,b;
        get_worm_rgb (color, is_materialized, out r, out g, out b);
        s.append_fill (path.to_path (), EVEN_ODD, {(float)r, (float)g, (float)b, 1.0f});
    }

    void draw_bonus (Snapshot s, int x, int y, int x_size, int y_size, Bonus.eType type, uint64 animate)
    {
        float x_m = x_size;
        float y_m = y_size;
        switch (type)
        {
            case REGULAR:
                x_m /= 18;
                y_m /= 18;
                var p0 = new PathBuilder ();
                p0.move_to (x + x_m * 15, y + y_m * 8);
                p0.cubic_to (x + x_m * 15.023438f, y + y_m * 10.035156f, x + x_m * 13.953125f, y + y_m * 17.1875f, x + x_m * 8, y + y_m * 14.429688f);
                p0.cubic_to (x + x_m * 1.90625f, y + y_m * 17.109375f, x + x_m * 1.03125f, y + y_m * 9.921875f, x + x_m * 1, y + y_m * 8);
                p0.cubic_to (x + x_m * 1.007813f, y + y_m * 5.109375f, x + x_m * 3.300781f, y + y_m * 1.355469f, x + x_m * 8, y + y_m * 4.3125f);
                p0.cubic_to (x + x_m * 12.933594f, y + y_m * 1.394531f, x + x_m * 15.0625f, y + y_m * 5, x + x_m * 15, y + y_m * 8);
                s.append_fill (p0.to_path (), EVEN_ODD, {0.0f, 1.0f, 0.0f, 1.0f});
                var p1 = new PathBuilder ();
                p1.move_to (x + x_m * 9.65625f, y + y_m * 1.34375f);
                p1.cubic_to (x + x_m * 8, y + y_m * 2, x + x_m * 8, y + y_m * 3.667969f, x + x_m * 8, y + y_m * 5);
                s.append_fill (p1.to_path (), EVEN_ODD, {0.0f, 1.0f, 0.0f, 1.0f});
                break;
            case HALF:
                x_m /= 16;
                y_m /= 16;
                var p0 = new PathBuilder ();
                p0.move_to (x + x_m * 10.253906f, y + y_m * 1.3125f);
                p0.cubic_to (x + x_m * 9.472656f, y + y_m * 4.730469f, x + x_m * 9.445313f, y + y_m * 8.015625f, x + x_m * 11.625f, y + y_m * 10.683594f);
                s.append_fill (p0.to_path (), EVEN_ODD, {0.305882f, 0.603922f, 0.0235294f, 1.0f});
                var p1 = new PathBuilder ();
                p1.move_to (x + x_m * 10.296875f, y + y_m * 1.152344f);
                p1.cubic_to (x + x_m * 9.046875f, y + y_m * 7.132813f, x + x_m * 6.023438f, y + y_m * 7.765625f, x + x_m * 3.84375f, y + y_m * 10.429688f);
                s.append_fill (p1.to_path (), EVEN_ODD, {0.305882f, 0.603922f, 0.0235294f, 1.0f});
                var p2 = new PathBuilder ();
                p2.move_to (x + x_m * 7, y + y_m * 10);
                p2.cubic_to (x + x_m * 7, y + y_m * 11.65625f, x + x_m * 5.65625f, y + y_m * 13, x + x_m * 4, y + y_m * 13);
                p2.cubic_to (x + x_m * 2.34375f, y + y_m * 13, x + x_m * 1, y + y_m * 11.65625f, x + x_m * 1, y + y_m * 10);
                p2.cubic_to (x + x_m * 1, y + y_m * 8.34375f, x + x_m * 2.34375f, y + y_m * 7, x + x_m * 4, y + y_m * 7);
                p2.cubic_to (x + x_m * 5.65625f, y + y_m * 7, x + x_m * 7, y + y_m * 8.34375f, x + x_m * 7, y + y_m * 10);
                s.append_fill (p2.to_path (), EVEN_ODD, {0.8f, 0.0f, 0.0f, 1.0f});
                var p3 = new PathBuilder ();
                p3.move_to (x + x_m * 15, y + y_m * 12);
                p3.cubic_to (x + x_m * 15, y + y_m * 13.65625f, x + x_m * 13.65625f, y + y_m * 15, x + x_m * 12, y + y_m * 15);
                p3.cubic_to (x + x_m * 10.34375f, y + y_m * 15, x + x_m * 9, y + y_m * 13.65625f, x + x_m * 9, y + y_m * 12);
                p3.cubic_to (x + x_m * 9, y + y_m * 10.34375f, x + x_m * 10.34375f, y + y_m * 9, x + x_m * 12, y + y_m * 9);
                p3.cubic_to (x + x_m * 13.65625f, y + y_m * 9, x + x_m * 15, y + y_m * 10.34375f, x + x_m * 15, y + y_m * 12);
                s.append_fill (p3.to_path (), EVEN_ODD, {0.8f, 0.0f, 0.0f, 1.0f});
                break;
            case DOUBLE:
                x_m /= 18;
                y_m /= 18;
                var p0 = new PathBuilder ();
                p0.move_to (x + x_m * 0.695313f, y + y_m * 8.425781f);
                p0.cubic_to (x + x_m * 8.914063f, y + y_m * 11.246094f, x + x_m * 13.257813f, y + y_m * 5.894531f, x + x_m * 13.847656f, y + y_m * 4.394531f);
                p0.cubic_to (x + x_m * 14.285156f, y + y_m * 3.351563f, x + x_m * 14.308594f, y + y_m * 3.082031f, x + x_m * 14.402344f, y + y_m * 2.535156f);
                p0.cubic_to (x + x_m * 14.941406f, y + y_m * 2.433594f, x + x_m * 15.613281f, y + y_m * 2.71875f, x + x_m * 16, y + y_m * 3.0625f);
                p0.cubic_to (x + x_m * 15.566406f, y + y_m * 3.535156f, x + x_m * 15.261719f, y + y_m * 4.246094f, x + x_m * 15.167969f, y + y_m * 4.984375f);
                p0.cubic_to (x + x_m * 15.675781f, y + y_m * 11.316406f, x + x_m * 7.71875f, y + y_m * 17.683594f, x + x_m * 0, y + y_m * 9.972656f);
                p0.cubic_to (x + x_m * 0.03125f, y + y_m * 9.433594f, x + x_m * 0.210938f, y + y_m * 8.84375f, x + x_m * 0.695313f, y + y_m * 8.425781f);
                s.append_fill (p0.to_path (), EVEN_ODD, {0.988235f, 0.913725f, 0.309804f, 1.0f});
                break;
            case LIFE:
                x_m /= 16;
                y_m /= 16;
                var p0 = new PathBuilder ();
                p0.move_to (x + x_m * 4.753906f, y + y_m * 1.828125f);
                p0.cubic_to (x + x_m * 2.652344f, y + y_m * 1.851563f, x + x_m * 1.019531f, y + y_m * 3.648438f, x + x_m * 1, y + y_m * 5.8125f);
                p0.cubic_to (x + x_m * 0.972656f, y + y_m * 8.890625f, x + x_m * 2.808594f, y + y_m * 9.882813f, x + x_m * 8.015625f, y + y_m * 14.171875f);
                p0.cubic_to (x + x_m * 12.992188f, y + y_m * 9.558594f, x + x_m * 14.976563f, y + y_m * 8.316406f, x + x_m * 15, y + y_m * 5.722656f);
                p0.cubic_to (x + x_m * 15.027344f, y + y_m * 2.886719f, x + x_m * 10.90625f, y + y_m * 0.128906f, x + x_m * 7.910156f, y + y_m * 3.121094f);
                p0.cubic_to (x + x_m * 6.835938f, y + y_m * 2.199219f, x + x_m * 5.742188f, y + y_m * 1.816406f, x + x_m * 4.753906f, y + y_m * 1.828125f);
                s.append_fill (p0.to_path (), EVEN_ODD, {1.0f, 0.0f, 0.0f, 1.0f});
                break;
            case REVERSE:
                x_m /= 16;
                y_m /= 16;
                var p0 = new PathBuilder ();
                p0.move_to (x + x_m * 4, y + y_m * 2);
                p0.line_to (x + x_m * 12, y + y_m * 2);
                p0.line_to (x + x_m * 15, y + y_m * 6);
                p0.line_to (x + x_m * 8, y + y_m * 15);
                p0.line_to (x + x_m * 1, y + y_m * 6);
                s.append_fill (p0.to_path (), EVEN_ODD, {0.717647f, 0.807843f, 0.901961f, 1.0f});
                var p1 = new PathBuilder ();
                p1.move_to (x + x_m * 11, y + y_m * 6);
                p1.line_to (x + x_m * 8, y + y_m * 15);
                p1.line_to (x + x_m * 5, y + y_m * 6);
                s.append_fill (p1.to_path (), EVEN_ODD, {0.447059f, 0.623529f, 0.811765f, 1.0f});
                var p2 = new PathBuilder ();
                p2.move_to (x + x_m * 4, y + y_m * 2);
                p2.line_to (x + x_m * 8, y + y_m * 2);
                p2.line_to (x + x_m * 5, y + y_m * 6);
                p2.line_to (x + x_m * 1, y + y_m * 6);
                s.append_fill (p2.to_path (), EVEN_ODD, {0.447059f ,0.623529f ,0.811765f, 1.0f});
                var p3 = new PathBuilder ();
                p3.move_to (x + x_m * 12, y + y_m * 2);
                p3.line_to (x + x_m * 8, y + y_m * 2);
                p3.line_to (x + x_m * 11, y + y_m * 6);
                p3.line_to (x + x_m * 15, y + y_m * 6);
                s.append_fill (p3.to_path (), EVEN_ODD, {0.447059f, 0.623529f, 0.811765f, 1.0f});
                break;
            case WARP:
                x_m /= 16;
                y_m /= 16;
                var p0 = new PathBuilder ();
                p0.move_to (x + x_m * 8.664063f, y + y_m * 0.621094f);
                p0.cubic_to (x + x_m * 6.179688f, y + y_m * 0.761719f, x + x_m * 4.265625f, y + y_m * 2.679688f, x + x_m * 4.40625f, y + y_m * 5.164063f);
                p0.line_to (x + x_m * 7.433594f, y + y_m * 5.164063f);
                p0.cubic_to (x + x_m * 7.386719f, y + y_m * 4.3125f, x + x_m * 8.003906f, y + y_m * 3.699219f, x + x_m * 8.855469f, y + y_m * 3.652344f);
                p0.cubic_to (x + x_m * 9.707031f, y + y_m * 3.601563f, x + x_m * 10.417969f, y + y_m * 4.21875f, x + x_m * 10.464844f, y + y_m * 5.070313f);
                p0.line_to (x + x_m * 10.464844f, y + y_m * 5.117188f);
                p0.cubic_to (x + x_m * 10.46875f, y + y_m * 5.316406f, x + x_m * 10.417969f, y + y_m * 5.609375f, x + x_m * 10.273438f, y + y_m * 5.78125f);
                p0.cubic_to (x + x_m * 9.929688f, y + y_m * 6.191406f, x + x_m * 9.542969f, y + y_m * 6.53125f, x + x_m * 9.234375f, y + y_m * 6.773438f);
                p0.cubic_to (x + x_m * 8.890625f, y + y_m * 7.035156f, x + x_m * 8.515625f, y + y_m * 7.351563f, x + x_m * 8.144531f, y + y_m * 7.816406f);
                p0.cubic_to (x + x_m * 7.773438f, y + y_m * 8.28125f, x + x_m * 7.433594f, y + y_m * 8.949219f, x + x_m * 7.433594f, y + y_m * 9.710938f);
                p0.cubic_to (x + x_m * 7.425781f, y + y_m * 10.507813f, x + x_m * 8.148438f, y + y_m * 11.222656f, x + x_m * 8.949219f, y + y_m * 11.222656f);
                p0.cubic_to (x + x_m * 9.75f, y + y_m * 11.222656f, x + x_m * 10.476563f, y + y_m * 10.507813f, x + x_m * 10.464844f, y + y_m * 9.710938f);
                p0.cubic_to (x + x_m * 10.464844f, y + y_m * 9.710938f, x + x_m * 10.4375f, y + y_m * 9.753906f, x + x_m * 10.511719f, y + y_m * 9.664063f);
                p0.cubic_to (x + x_m * 10.585938f, y + y_m * 9.566406f, x + x_m * 10.789063f, y + y_m * 9.40625f, x + x_m * 11.078125f, y + y_m * 9.1875f);
                p0.cubic_to (x + x_m * 12.921875f, y + y_m * 7.792969f, x + x_m * 13.492188f, y + y_m * 7.003906f, x + x_m * 13.492188f, y + y_m * 4.882813f);
                p0.cubic_to (x + x_m * 13.355469f, y + y_m * 2.394531f, x + x_m * 11.152344f, y + y_m * 0.484375f, x + x_m * 8.664063f, y + y_m * 0.621094f);
                float r,g,b;
                r = animate%30 < 10 ? (animate%30 / 10.0f) : (animate%30 >= 20 ? 0 : ((20 - animate%30) / 10.0f));
                g = (animate+10)%30 < 10 ? ((animate+10)%30 / 10.0f) : ((animate+10)%30 >= 20 ? 0 : ((20 - (animate+10)%30) / 10.0f));
                b = (animate+20)%30 < 10 ? ((animate+20)%30 / 10.0f) : ((animate+20)%30 >= 20 ? 0 : ((20 - (animate+20)%30) / 10.0f));
                s.append_fill (p0.to_path (), EVEN_ODD, {r, g, b, 1.0f});
                var p1 = new PathBuilder ();
                p1.move_to (x + x_m * 8.949219f, y + y_m * 12.738281f);
                p1.cubic_to (x + x_m * 8.113281f, y + y_m * 12.738281f, x + x_m * 7.433594f, y + y_m * 13.417969f, x + x_m * 7.433594f, y + y_m * 14.253906f);
                p1.cubic_to (x + x_m * 7.433594f, y + y_m * 15.089844f, x + x_m * 8.113281f, y + y_m * 15.769531f, x + x_m * 8.949219f, y + y_m * 15.769531f);
                p1.cubic_to (x + x_m * 9.785156f, y + y_m * 15.769531f, x + x_m * 10.464844f, y + y_m * 15.089844f, x + x_m * 10.464844f, y + y_m * 14.253906f);
                p1.cubic_to (x + x_m * 10.464844f, y + y_m * 13.417969f, x + x_m * 9.785156f, y + y_m * 12.738281f, x + x_m * 8.949219f, y + y_m * 12.738281f);
                s.append_fill (p1.to_path (), EVEN_ODD, {r, g, b, 1.0f});
                break;
            case 6:
                x_m /= 16;
                y_m /= 16;
                var p0 = new PathBuilder ();
                p0.move_to (x + x_m * 8.902344f, y + y_m * 0.160156f);
                p0.cubic_to (x + x_m * 6.953125f, y + y_m * 1.15625f, x + x_m * 7.480469f, y + y_m * 3.089844f, x + x_m * 7.453125f, y + y_m * 5.019531f);
                p0.line_to (x + x_m * 8.257813f, y + y_m * 4.8125f);
                p0.cubic_to (x + x_m * 8.144531f, y + y_m * 3.507813f, x + x_m * 9.359375f, y + y_m * 1.511719f, x + x_m * 10.742188f, y + y_m * 1.675781f);
                s.append_fill (p0.to_path (), EVEN_ODD, {0.305882f, 0.603922f, 0.0235294f, 1.0f});
                var p1 = new PathBuilder ();
                p1.move_to (x + x_m * 14, y + y_m * 9);
                p1.cubic_to (x + x_m * 14, y + y_m * 5.6875f, x + x_m * 11.3125f, y + y_m * 3, x + x_m * 8, y + y_m * 3);
                p1.cubic_to (x + x_m * 4.6875f, y + y_m * 3, x + x_m * 2, y + y_m * 5.6875f, x + x_m * 2, y + y_m * 9);
                p1.cubic_to (x + x_m * 2, y + y_m * 12.3125f, x + x_m * 4.6875f, y + y_m * 15, x + x_m * 8, y + y_m * 15);
                p1.cubic_to (x + x_m * 11.3125f, y + y_m * 15, x + x_m * 14, y + y_m * 12.3125f, x + x_m * 14, y + y_m * 9);
                s.append_fill (p1.to_path (), EVEN_ODD, {0.960784f, 0.47451f, 0.0f, 1.0f});
                break;
            case 7:
                x_m /= 16;
                y_m /= 16;
                var p0 = new PathBuilder ();
                p0.move_to (x + x_m * 4.585938f, y + y_m * 0.96875f);
                p0.cubic_to (x + x_m * 3.914063f, y + y_m * 3.050781f, x + x_m * 5.65625f, y + y_m * 4.042969f, x + x_m * 7, y + y_m * 5.429688f);
                p0.line_to (x + x_m * 7.421875f, y + y_m * 4.710938f);
                p0.cubic_to (x + x_m * 6.417969f, y + y_m * 3.871094f, x + x_m * 5.867188f, y + y_m * 1.597656f, x + x_m * 6.960938f, y + y_m * 0.738281f);
                s.append_fill (p0.to_path (), EVEN_ODD, {0.305882f, 0.603922f, 0.0235294f, 1.0f});
                var p1 = new PathBuilder ();
                p1.move_to (x + x_m * 12.933594f, y + y_m * 5.347656f);
                p1.cubic_to (x + x_m * 13.652344f, y + y_m * 7.882813f, x + x_m * 12.867188f, y + y_m * 8.753906f, x + x_m * 12.871094f, y + y_m * 10.476563f);
                p1.cubic_to (x + x_m * 12.875f, y + y_m * 12.890625f, x + x_m * 13.015625f, y + y_m * 14.386719f, x + x_m * 11.148438f, y + y_m * 15.089844f);
                p1.cubic_to (x + x_m * 9.941406f, y + y_m * 15.492188f, x + x_m * 8.785156f, y + y_m * 15.382813f, x + x_m * 6.539063f, y + y_m * 12.617188f);
                p1.cubic_to (x + x_m * 5.886719f, y + y_m * 11.765625f, x + x_m * 4.117188f, y + y_m * 11.683594f, x + x_m * 3.226563f, y + y_m * 10.214844f);
                p1.cubic_to (x + x_m * 2.117188f, y + y_m * 8.375f, x + x_m * 2.902344f, y + y_m * 5.152344f, x + x_m * 6.707031f, y + y_m * 4.464844f);
                p1.cubic_to (x + x_m * 8.609375f, y + y_m * 2.308594f, x + x_m * 11.933594f, y + y_m * 3.136719f, x + x_m * 12.933594f, y + y_m * 5.347656f);
                s.append_fill (p1.to_path (), EVEN_ODD, {0.937255f, 0.160784f, 0.160784f, 1.0f});
                break;
            default:
                break;
        }
    }

    /*\
    * * Text
    \*/

    void draw_text_target_width (Snapshot snapshot, int x, int y, string text, int target_width, int color)
    {
        /* draw using x,y as the top left corner of the text */
        int target_font_size = 1;
        uint target_width_diff = uint.MAX;
        Pango.Rectangle a = {0,0,0,0};

        for (int font_size = 1;font_size < 200;font_size++)
        {
            var layout = create_pango_layout (text);
            var font = null == layout.get_font_description () ?
                Pango.FontDescription.from_string ("Sans Bold 1pt") :
                layout.get_font_description ().copy ();
            font.set_size (Pango.SCALE * font_size);
            layout.set_font_description (font);
            layout.set_text (text, -1);
            Pango.Rectangle b;
            layout.get_extents (out a, out b);
            uint width_diff = (target_width - (int)a.width / Pango.SCALE).abs ();
            if (width_diff > target_width_diff && width_diff - target_width_diff > 2)
                break;
            else if (width_diff < target_width_diff)
            {
                target_width_diff = width_diff;
                target_font_size = font_size;
            }
        }
        snapshot.translate ({x - a.x / Pango.SCALE, y - a.y / Pango.SCALE});
        var layout = create_pango_layout (text);
        var font = null == layout.get_font_description () ?
            Pango.FontDescription.from_string ("Sans Bold 1pt") :
            layout.get_font_description ().copy ();
        font.set_size (Pango.SCALE * target_font_size);
        layout.set_font_description (font);
        layout.set_text (text, -1);
        snapshot.append_layout (layout, {1, 1, 1, 1});
        snapshot.translate ({ -(x - a.x / Pango.SCALE), -(y - a.y / Pango.SCALE)});
    }
    /* calculate the width & height of the text */
    void calculate_text_size (string text, int font_size, out double width, out double height)
    {
        var layout = create_pango_layout (text);
        var font = null == layout.get_font_description () ?
            Pango.FontDescription.from_string ("Sans Bold 1pt") :
            layout.get_font_description ().copy ();
        font.set_size (Pango.SCALE * font_size);
        layout.set_font_description (font);
        layout.set_text (text, -1);
        Pango.Rectangle a,b;
        layout.get_extents (out a, out b);
        width = a.width / Pango.SCALE;
        height = a.height / Pango.SCALE;
    }
    /* draw the text */
    void draw_text_font_size (Snapshot snapshot, int x, int y, string text, int font_size)
    {
        int x_offset, y_offset;
        get_text_offsets (text, font_size, out x_offset, out y_offset);
        snapshot.translate ({x - x_offset, y - y_offset});
        var layout = create_pango_layout (text);
        layout.set_alignment (1);
        var font = null == layout.get_font_description () ?
            Pango.FontDescription.from_string ("Sans Bold 1pt") :
            layout.get_font_description ().copy ();
        font.set_size (Pango.SCALE * font_size);
        layout.set_font_description (font);
        layout.set_text (text, -1);
        snapshot.append_layout (layout, {1, 1, 1, 1});
        snapshot.translate ({ -(x - x_offset), -(y - y_offset)});
    }
    void get_text_offsets (string text, int font_size, out int x_offset, out int y_offset)
    {
        var layout = create_pango_layout (text);
        var font = null == layout.get_font_description () ?
            Pango.FontDescription.from_string ("Sans Bold 1pt") :
            layout.get_font_description ().copy ();
        font.set_size (Pango.SCALE * font_size);
        layout.set_font_description (font);
        layout.set_text (text, -1);
        Pango.Rectangle a,b;
        layout.get_extents (out a, out b);
        x_offset = a.x / Pango.SCALE;
        y_offset = a.y / Pango.SCALE;
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
    * * utility functions
    \*/
    
    Graphene.Rect get_bounds (Gsk.Path path)
    {
        Graphene.Rect bounds;
        path.get_bounds (out bounds);
        return bounds;
    }
}
