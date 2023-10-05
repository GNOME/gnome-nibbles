/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Gnome Nibbles: Gnome Worm Game
 * Copyright (C) 2015 Iulian-Gabriel Radu <iulian.radu67@gmail.com>
 * Copyright (C) 2020 Arnaud Bonatti <arnaud.bonatti@gmail.com>
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

using Gtk;

[GtkTemplate (ui = "/org/gnome/Nibbles/ui/controls.ui")]
private class Controls : Box
{
    [GtkChild] private unowned Box grids_box;
    private Gee.LinkedList<ControlsGrid> grids = new Gee.LinkedList<ControlsGrid> ();

    /* nibbles-window */
    internal signal void add_keypress_handler (KeypressHandlerFunction? handler);

    internal Gee.List<GLib.Settings> worm_settings;

    internal void prepare (Gee.LinkedList<Worm> worms, Gee.HashMap<Worm, WormProperties> worms_props, Gee.List<GLib.Settings> worm_settings)
    {
        for (;grids_box.get_last_child () != null;)
        {
            var child = grids_box.get_last_child ();
            grids_box.remove (child);
            child.destroy ();
        }

        GenericSet<uint> duplicate_keys     = new GenericSet<uint> (direct_hash, direct_equal);
        GenericSet<uint> encountered_keys   = new GenericSet<uint> (direct_hash, direct_equal);
        foreach (var worm in worms)
        {
            if (worm.is_human)
            {
                WormProperties worm_prop = worms_props.@get (worm);

                var grid = new ControlsGrid (this, worm.id, worm_prop, worms_props);
                grids_box.append (grid);
                grids.add (grid);

                check_for_duplicates (worm_prop.up,     ref encountered_keys, ref duplicate_keys);
                check_for_duplicates (worm_prop.down,   ref encountered_keys, ref duplicate_keys);
                check_for_duplicates (worm_prop.left,   ref encountered_keys, ref duplicate_keys);
                check_for_duplicates (worm_prop.right,  ref encountered_keys, ref duplicate_keys);
            }
        }
        foreach (ControlsGrid grid in grids)
        {
            grid.external_handler = grid.worm_props.notify.connect (() => {
                    GenericSet<uint> _duplicate_keys    = new GenericSet<uint> (direct_hash, direct_equal);
                    GenericSet<uint> _encountered_keys  = new GenericSet<uint> (direct_hash, direct_equal);
                    foreach (var worm in worms)
                    {
                        if (worm.is_human)
                        {
                            WormProperties worm_prop = worms_props.@get (worm);

                            check_for_duplicates (worm_prop.up,     ref _encountered_keys, ref _duplicate_keys);
                            check_for_duplicates (worm_prop.down,   ref _encountered_keys, ref _duplicate_keys);
                            check_for_duplicates (worm_prop.left,   ref _encountered_keys, ref _duplicate_keys);
                            check_for_duplicates (worm_prop.right,  ref _encountered_keys, ref _duplicate_keys);
                        }
                    }
                    foreach (ControlsGrid _grid in grids)
                        _grid.mark_duplicated_keys (_duplicate_keys);
                });
            grid.mark_duplicated_keys (duplicate_keys);
        }
        this.worm_settings = worm_settings;
    }
    private void check_for_duplicates (uint key, ref GenericSet<uint> encountered_keys, ref GenericSet<uint> duplicate_keys)
    {
        if (encountered_keys.contains (key))
            duplicate_keys.add (key);
        else
            encountered_keys.add (key);
    }

    internal void clean ()
    {
        foreach (ControlsGrid grid in grids)
        {
            grid.worm_props.disconnect (grid.external_handler);
            grid.disconnect_stuff ();
        }
        grids.clear ();
    }
}

[GtkTemplate (ui = "/org/gnome/Nibbles/ui/controls-grid.ui")]
private class ControlsGrid : Frame
{
    [GtkChild] private unowned Overlay overlay;
    //[GtkChild] private unowned Grid grid;
    [GtkChild] private unowned Button name_label;
    [GtkChild] private unowned DrawingArea arrow_up;
    [GtkChild] private unowned DrawingArea arrow_down;
    [GtkChild] private unowned DrawingArea arrow_left;
    [GtkChild] private unowned DrawingArea arrow_right;
    [GtkChild] private unowned Button move_up_button;
    [GtkChild] private unowned Button move_down_button;
    [GtkChild] private unowned Button move_left_button;
    [GtkChild] private unowned Button move_right_button;

    internal Controls controls;
    internal WormProperties worm_props;
    internal ulong external_handler;
    private ulong    up_handler;
    private ulong  down_handler;
    private ulong  left_handler;
    private ulong right_handler;
    private ulong color_handler;

    ColourWheel? colour_wheel = null;
    OverlayMessage key_press_message;

    bool duplicate[4];

    internal ControlsGrid (Controls controls, int worm_id, WormProperties worm_props, Gee.HashMap<Worm, WormProperties> worms_props)
    {
        this.controls = controls;
        this.worm_props = worm_props;
        set_margin_bottom (10);

        /* Translators: text displayed in a screen showing the keys used by the players; the %d is replaced by the number that identifies the player */
        var player_id = _("Player %d").printf (worm_id + 1);
        color_handler = worm_props.notify ["color"].connect (() => {
                var color = Pango.Color ();
                color.parse (NibblesView.colorval_name_untranslated (worm_props.color));
                ((Label)name_label.get_child ()).set_markup (@"<b><span font-family=\"Sans\" color=\"$(color.to_string ())\">$(player_id)</span></b>");
            });
        var color = Pango.Color ();
        color.parse (NibblesView.colorval_name_untranslated (worm_props.color));
        ((Label)name_label.get_child ()).set_markup (@"<b><span font-family=\"Sans\" color=\"$(color.to_string ())\">$(player_id)</span></b>");

        arrow_up.set_draw_func ((/*DrawingArea*/ area, /*Cairo.Context*/ C, width, height)=>{draw_arrow (0, C, width, height);});
        arrow_down.set_draw_func ((/*DrawingArea*/ area, /*Cairo.Context*/ C, width, height)=>{draw_arrow (1, C, width, height);});
        arrow_left.set_draw_func ((/*DrawingArea*/ area, /*Cairo.Context*/ C, width, height)=>{draw_arrow (2, C, width, height);});
        arrow_right.set_draw_func ((/*DrawingArea*/ area, /*Cairo.Context*/ C, width, height)=>{draw_arrow (3, C, width, height);});

           up_handler = worm_props.notify ["up"].connect    (() => configure_label (worm_props.up, (Label)(move_up_button.get_child ())));
         down_handler = worm_props.notify ["down"].connect  (() => configure_label (worm_props.down, (Label)(move_down_button.get_child ())));
         left_handler = worm_props.notify ["left"].connect  (() => configure_label (worm_props.left, (Label)(move_left_button.get_child ())));
        right_handler = worm_props.notify ["right"].connect (() => configure_label (worm_props.right, (Label)(move_right_button.get_child ())));

        configure_label (worm_props.up,    (Label)(move_up_button.get_child ()));
        configure_label (worm_props.down,  (Label)(move_down_button.get_child ()));
        configure_label (worm_props.left,  (Label)(move_left_button.get_child ()));
        configure_label (worm_props.right, (Label)(move_right_button.get_child ()));

        name_label.clicked.connect (()=> 
        {
            if (null == colour_wheel && null == key_press_message)
            {
                colour_wheel = new ColourWheel ((c)=>
                {
                    overlay.remove_overlay (colour_wheel);
                    colour_wheel = null;
                    controls.add_keypress_handler (null);
                    if (this.worm_props.color != (int)c)
                    {
                        for (var i = worms_props.map_iterator ();i.next ();)
                        {
                            if (i.get_value ().color == (int)c)
                            {
                                // swap colors
                                i.get_value ().color = this.worm_props.color;
                                this.worm_props.color = (int)c;
                                return;
                            }
                        }
                        this.worm_props.color = (int)c;
                    }
                });
                overlay.add_overlay (colour_wheel);
                /* to do, catch key presses to allow color selection via the keyboard */
                /*controls.add_keypress_handler ((keyval, keycode, out remove_handler)=>
                {
                    //remove_handler = true;
                    switch (keyval)
                    {
                        case 0xff09: // tab 
                            break;
                        case 0xfe20: // back tab 
                            break;
                        default:
                            break;
                    }
                    return true;
                });*/
            }
        });

        move_up_button.clicked.connect (()=> 
        {
            if (null == colour_wheel && null == key_press_message)
            {
                /* Translators: text displayed in a message box directing the player to press the key they want to use to direct the worm up the screen */
                key_press_message = new OverlayMessage (_("Press a key for up."));
                overlay.add_overlay (key_press_message);
                controls.add_keypress_handler ((keyval, keycode, out remove_handler)=>
                {
                    remove_handler = true;
                    if (keyval != this.worm_props.up)
                        this.worm_props.up = keyval;
                    overlay.remove_overlay (key_press_message);
                    key_press_message = null;
                    return true;
                });
            }
        });
        move_down_button.clicked.connect (()=> 
        {
            if (null == colour_wheel && null == key_press_message)
            {
                /* Translators: text displayed in a message box directing the player to press the key they want to use to direct the worm down the screen */
                key_press_message = new OverlayMessage (_("Press a key for down."));
                overlay.add_overlay (key_press_message);
                controls.add_keypress_handler ((keyval, keycode, out remove_handler)=>
                {
                    remove_handler = true;
                    if (keyval != this.worm_props.down)
                        this.worm_props.down = keyval;
                    overlay.remove_overlay (key_press_message);
                    key_press_message = null;
                    return true;
                });
            }
        });
        move_left_button.clicked.connect (()=> 
        {
            if (null == colour_wheel && null == key_press_message)
            {
                /* Translators: text displayed in a message box directing the player to press the key they want to use to direct the worm left */
                key_press_message = new OverlayMessage (_("Press a key for left."));
                overlay.add_overlay (key_press_message);
                controls.add_keypress_handler ((keyval, keycode, out remove_handler)=>
                {
                    remove_handler = true;
                    if (keyval != this.worm_props.left)
                        this.worm_props.left = keyval;
                    overlay.remove_overlay (key_press_message);
                    key_press_message = null;
                    return true;
                });
            }
        });
        move_right_button.clicked.connect (()=> 
        {
            if (null == colour_wheel && null == key_press_message)
            {
                /* Translators: text displayed in a message box directing the player to press the key they want to use to direct the worm right */
                key_press_message = new OverlayMessage (_("Press a key for right."));
                overlay.add_overlay (key_press_message);
                controls.add_keypress_handler ((keyval, keycode, out remove_handler)=>
                {
                    remove_handler = true;
                    if (keyval != this.worm_props.right)
                        this.worm_props.right = keyval;
                    overlay.remove_overlay (key_press_message);
                    key_press_message = null;
                    return true;
                });
            }
        });
    }

    struct xy
    {
        double x;
        double y;
    }
    private void draw_arrow (uint d /* 0 up, 1 down, 2 left, 3 right */, Cairo.Context C, double width, double height)
    {
        xy a[7];
        
        if (d == 0)
            a = {{0, height / 2},{width / 2, 0},{width, height /2},{width * 2 / 3, height / 2},{width * 2 / 3, height},{width / 3, height},{width / 3, height / 2}};
        else if (d == 1)
            a = {{0, height / 2},{width / 2, height},{width, height /2},{width * 2 / 3, height / 2},{width * 2 / 3, 0},{width / 3, 0},{width / 3, height / 2}};
        else if (d == 2)
            a = {{width / 2, 0},{0, height / 2},{width/2, height},{width / 2, height * 2 / 3},{width, height * 2 / 3},{width, height / 3},{width / 2, height / 3}};
        else
            a = {{width / 2, 0},{width, height / 2},{width / 2, height},{width / 2, height * 2 / 3},{0, height  * 2 / 3},{0, height / 3},{width / 2, height / 3}};

        /* draw */
        for (int i = 0; i < a.length; i++)
        {
            if (i == 0)
                C.move_to (a[0].x, a[0].y);
            else
                C.line_to (a[i].x, a[i].y);
        }
        if (duplicate[d])
            C.set_source_rgba (0.75, 0, 0, 1);
        else
            C.set_source_rgba (0.2890625, 0.5625, 0.84765625, 1); //4a90d9
        C.fill ();       
    }

    internal void mark_duplicated_keys (GenericSet<uint> duplicate_keys)
    {
        bool d;
        d = worm_props.up in duplicate_keys;
        if (d != duplicate[0])
        {
            duplicate[0] = d;
            arrow_up.queue_draw ();
        }
        d = worm_props.down in duplicate_keys;
        if (d != duplicate[1])
        {
            duplicate[1] = d;
            arrow_down.queue_draw ();
        }
        d = worm_props.left in duplicate_keys;
        if (d != duplicate[2])
        {
            duplicate[2] = d;
            arrow_left.queue_draw ();
        }
        d = worm_props.right in duplicate_keys;
        if (d != duplicate[3])
        {
            duplicate[3] = d;
            arrow_right.queue_draw ();
        }
        set_duplicate_class (worm_props.up    in duplicate_keys, (Label)(move_up_button.get_child ()));
        set_duplicate_class (worm_props.down  in duplicate_keys, (Label)(move_down_button.get_child ()));
        set_duplicate_class (worm_props.left  in duplicate_keys, (Label)(move_left_button.get_child ()));
        set_duplicate_class (worm_props.right in duplicate_keys, (Label)(move_right_button.get_child ()));
    }

    private void set_duplicate_class (bool duplicate, Label label)
    {
        label.attributes = new Pango.AttrList ();
        label.attributes.insert (Pango.attr_weight_new (Pango.Weight.BOLD));
        if (duplicate)
            label.attributes.insert (Pango.attr_foreground_new (0xffff,0,0));
        else
            label.attributes.insert (Pango.attr_foreground_new (0xffff,0xffff,0xffff));
    }

    internal void disconnect_stuff ()
    {
        worm_props.disconnect (up_handler);
        worm_props.disconnect (down_handler);
        worm_props.disconnect (left_handler);
        worm_props.disconnect (right_handler);
        worm_props.disconnect (color_handler);
    }

    private static void configure_label (uint key_value, Label label)
    {
        string? key_name = Gdk.keyval_name (key_value);
        if (key_name == "Up")
        {
            if (label.attributes == null)
                label.attributes = new Pango.AttrList ();
            label.attributes.insert (Pango.attr_scale_new (Pango.Scale.X_LARGE));
            label.attributes.insert (Pango.attr_weight_new (Pango.Weight.BOLD));
            label.set_text ("↑");
        }
        else if (key_name == "Down")
        {
            if (label.attributes == null)
                label.attributes = new Pango.AttrList ();
            label.attributes.insert (Pango.attr_scale_new (Pango.Scale.X_LARGE));
            label.attributes.insert (Pango.attr_weight_new (Pango.Weight.BOLD));
            label.set_text ("↓");
        }
        else if (key_name == "Left")
        {
            if (label.attributes == null)
                label.attributes = new Pango.AttrList ();
            label.attributes.insert (Pango.attr_scale_new (Pango.Scale.X_LARGE));
            label.attributes.insert (Pango.attr_weight_new (Pango.Weight.BOLD));
            label.set_text ("←");
        }
        else if (key_name == "Right")
        {
            if (label.attributes == null)
                label.attributes = new Pango.AttrList ();
            label.attributes.insert (Pango.attr_scale_new (Pango.Scale.X_LARGE));
            label.attributes.insert (Pango.attr_weight_new (Pango.Weight.BOLD));
            label.set_text ("→");
        }
        else if (key_name == null || key_name == "")
        {
            label.attributes = null;
            label.set_text ("");
        }
        else
        {
            label.attributes = null;
            label.set_text (@"$(accelerator_get_label (key_value, 0))");
        }
    }
}

internal class OverlayMessage : DrawingArea
{
    public OverlayMessage (string text)
    {
        // set drawing fuction
        set_draw_func ((/*DrawingArea*/ area, /*Cairo.Context*/ C, width, height)=>
        {
            const double PI2 = 1.570796326794896619231321691639751442;
            const double border_width = 3;

            double text_width;
            double text_height;
            int font_size = calculate_font_size (C, text, width / 3 * 2, out text_width, out text_height);
            double minimum_dimension = text_width < text_height ? text_width : text_height;
            double background_width = text_width + minimum_dimension * 2;
            double background_height = text_height + minimum_dimension * 2;

            double x = (width - background_width) / 2;
            double y = (height - background_height) / 2;

            double arc_radius = background_width < background_height ? background_width / 3 : background_height / 3;

            /* draw border */
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

            draw_text (C, x + (background_width - text_width) / 2,
                y + background_height / 2 + text_height / 3, text, font_size);
        });
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
            if (width_diff > target_width_diff)
                break;
            else if (width_diff < target_width_diff)
            {
                target_width_diff = width_diff;
                target_font_size = font_size;
            }
        }
        return target_font_size;
    }

    void draw_text (Cairo.Context C, double x, double y, string text, int font_size)
    {
        /* draw using x,y as the bottom left corner of the text */
        C.move_to (x, y);
        C.set_font_size (font_size);
        C.set_source_rgba (0.75, 0.75, 0.75, 1);
        C.show_text (text);
    }
}

internal class ColourWheel : DrawingArea
{
    internal delegate void ResultFunction (uint c);
    internal ResultFunction result;

    double centre_x;
    double centre_y;
    double radius;
    uint mouse_segment = uint.MAX;
    bool mouse_pressed = false;
    
    public ColourWheel (ResultFunction result)
    {
        this.result = (ResultFunction)result;
        focusable = true;
        can_focus = true;
        sensitive = true;
        focus_on_click = true;
        grab_focus ();
        
        // set drawing fuction
        set_draw_func ((/*DrawingArea*/ area, /*Cairo.Context*/ C, width, height)=>
        {
            const double PI2 = 1.570796326794896619231321691639751442;
            const double PI3 = 1.047197551196597746154214461093167628;
            const double border_width = 10;
            const double sixty_degrees = 2642885282.0/1525870529.0;
            radius = width < height ? width / 2 : height / 2;
            if (radius > border_width)
                radius -= border_width;
            centre_x = width / 2;
            centre_y = height / 2;

            // draw coloured pie
            C.move_to (centre_x + (mouse_pressed && mouse_segment == 0 ? border_width/sixty_degrees/2.0 : 0), centre_y - (mouse_pressed && mouse_segment == 0 ? border_width*sixty_degrees/2.0 : 0));
            C.arc (centre_x + (mouse_pressed && mouse_segment == 0 ? border_width/sixty_degrees/2.0 : 0), centre_y - (mouse_pressed && mouse_segment == 0 ? border_width*sixty_degrees/2.0 : 0), radius, -PI2, -PI2 + PI3);
            C.set_source_rgba (1 , 0, 0, 1);
            C.fill ();                
            C.move_to (centre_x + (mouse_pressed && mouse_segment == 1 ? border_width : 0), centre_y);
            C.arc (centre_x + (mouse_pressed && mouse_segment == 1 ? border_width : 0), centre_y, radius, -PI2 + PI3, -PI2 + PI3 + PI3);
            C.set_source_rgba (0, 0.75 , 0, 1);
            C.fill ();                
            C.move_to (centre_x + (mouse_pressed && mouse_segment == 2 ? border_width/sixty_degrees/2.0 : 0), centre_y + (mouse_pressed && mouse_segment == 2 ? border_width*sixty_degrees/2.0 : 0));
            C.arc (centre_x + (mouse_pressed && mouse_segment == 2 ? border_width/sixty_degrees/2.0 : 0), centre_y + (mouse_pressed && mouse_segment == 2 ? border_width*sixty_degrees/2.0 : 0), radius, PI2 - PI3, PI2);
            C.set_source_rgba (0, 0.5 , 1, 1);
            C.fill ();                
            C.move_to (centre_x - (mouse_pressed && mouse_segment == 3 ? border_width/sixty_degrees/2.0 : 0), centre_y+ (mouse_pressed && mouse_segment == 3 ? border_width*sixty_degrees/2.0 : 0));
            C.arc (centre_x - (mouse_pressed && mouse_segment == 3 ? border_width/sixty_degrees/2.0 : 0), centre_y+ (mouse_pressed && mouse_segment == 3 ? border_width*sixty_degrees/2.0 : 0), radius, PI2, PI2 + PI3);
            C.set_source_rgba (1, 1, 0, 1);
            C.fill ();                
            C.move_to (centre_x - (mouse_pressed && mouse_segment == 4 ? border_width : 0), centre_y);
            C.arc (centre_x- (mouse_pressed && mouse_segment == 4 ? border_width : 0), centre_y, radius, PI2 + PI3 , PI2 + PI3 + PI3);
            C.set_source_rgba (0, 1 , 1 , 1);
            C.fill ();                
            C.move_to (centre_x - (mouse_pressed && mouse_segment == 5 ? border_width/sixty_degrees/2.0 : 0), centre_y - (mouse_pressed && mouse_segment == 5 ? border_width*sixty_degrees/2.0 : 0));
            C.arc (centre_x - (mouse_pressed && mouse_segment == 5 ? border_width/sixty_degrees/2.0 : 0), centre_y - (mouse_pressed && mouse_segment == 5 ? border_width*sixty_degrees/2.0 : 0), radius, PI2 + PI3 + PI3, -PI2);
            C.set_source_rgba (0.75 , 0 , 0.75 , 1);
            C.fill ();           

            // instruction label
            /* Translators: text displayed in a message box directing the player to select the color they want for the worm */
            draw_label (C,width,height, _("Select your color"));
        });
        var mouse_position = new EventControllerMotion ();
        mouse_position.motion.connect ((x,y)=> {new_position (x, y);});
        mouse_position.enter.connect ((x,y)=>  {new_position (x, y);});

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
                    if (mouse_segment > 5)
                        redraw ();
                    else
                        result (mouse_segment);
                    return true;
                default:
                    return false;
            }
        });
        add_controller (mouse_click);
        add_controller (mouse_position);
    }

    void new_position (double x, double y)
    {
        var new_segment = segment (x, y);
        if (new_segment != mouse_segment)
        {
            mouse_segment = new_segment;
            redraw ();
        }
    }

    uint segment (double x, double y)
    {
        const double sixty_degrees = 2642885282.0/1525870529.0;
        x -= centre_x;
        y -= centre_y;
        y = - y;
        if (x * x + y * y > radius * radius)
            return uint.MAX;
        else if (x >= 0)
        {
            /* right half */
            if (y > 0  && x/y < sixty_degrees)
                return 0;
            else if (y < 0 && x/y > -sixty_degrees)
                return 2;
            else
                return 1;
        }
        else
        {
            /* left half */
            if (y > 0  && -x/y < sixty_degrees)
                return 5;
            else if (y < 0 && -x/y > -sixty_degrees)
                return 3;
            else
                return 4;
        }
    }

    void redraw ()
    {
        queue_draw ();
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
            if (width_diff > target_width_diff)
                break;
            else if (width_diff < target_width_diff)
            {
                target_width_diff = width_diff;
                target_font_size = font_size;
            }
        }
        return target_font_size;
    }

    void draw_text (Cairo.Context C, double x, double y, string text, int font_size)
    {
        /* draw using x,y as the bottom left corner of the text */
        C.move_to (x, y);
        C.set_font_size (font_size);
        C.set_source_rgba (0.75, 0.75, 0.75, 1);
        C.show_text (text);
    }

    void draw_label (Cairo.Context C, double width, double height, string text)
    {
            const double PI2 = 1.570796326794896619231321691639751442;
            const double border_width = 3;
        
            double text_width;
            double text_height;
            int font_size = calculate_font_size (C, text, (int)(width / 2), out text_width, out text_height);
            double minimum_dimension = text_width < text_height ? text_width : text_height;
            double background_width = text_width + minimum_dimension * 2;
            double background_height = text_height + minimum_dimension * 2;
            
            double x = (width - background_width) / 2;
            double y = 0;

            double arc_radius = background_width < background_height ? background_width / 3 : background_height / 3;
            
            /* draw border */
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

            draw_text (C, x + (background_width - text_width) / 2,
                y + background_height / 2 + text_height / 3, text, font_size);
    }
}

