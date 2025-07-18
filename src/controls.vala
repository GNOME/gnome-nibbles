/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Gnome Nibbles: Gnome Worm Game
 * Copyright (C) 2015 Iulian-Gabriel Radu <iulian.radu67@gmail.com>
 * Copyright (C) 2020 Arnaud Bonatti <arnaud.bonatti@gmail.com>
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

using Gtk;
using Gsk;

internal delegate bool CheckDuplicate (uint i);

[GtkTemplate (ui = "/org/gnome/Nibbles/ui/controls.ui")]
private class Controls : Box
{
    [GtkChild] private unowned Box grids_box;
    [GtkChild] private unowned Button button;
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

        #if USE_PILL_BUTTON
        if (button.has_css_class ("play"))
        {
            button.remove_css_class ("play");
            button.add_css_class ("pill");
        }
        #else
        button.has_css_class ("play");
        #endif

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


[GtkTemplate (ui = "/org/gnome/Nibbles/ui/arrow.ui")]
internal class Arrow : Widget
{
    /* direction property */
    internal enum eDirection {up, down, left, right}
    internal eDirection direction {set; get;}
    /* structor used by render function */
    struct xy {double x; double y;}
    /* render function */
    public override void snapshot (Snapshot s)
    {
        var path = new PathBuilder ();
        double width = get_width ();
        double height = get_height ();
        xy a[7];
        switch (direction)
        {
            case up:
            default:
                a = {{0, height / 2},{width / 2, 0},{width, height /2},{width * 2 / 3, height / 2},{width * 2 / 3, height},{width / 3, height},{width / 3, height / 2}};
                break;
            case down:
                a = {{0, height / 2},{width / 2, height},{width, height /2},{width * 2 / 3, height / 2},{width * 2 / 3, 0},{width / 3, 0},{width / 3, height / 2}};
                break;
            case left:
                a = {{width / 2, 0},{0, height / 2},{width/2, height},{width / 2, height * 2 / 3},{width, height * 2 / 3},{width, height / 3},{width / 2, height / 3}};
                break;
            case right:
                a = {{width / 2, 0},{width, height / 2},{width / 2, height},{width / 2, height * 2 / 3},{0, height  * 2 / 3},{0, height / 3},{width / 2, height / 3}};
                break;
        }
        /* draw */
        for (int i = 0; i < a.length; i++)
        {
            if (i == 0)
                path.move_to ((float)a[0].x, (float)a[0].y);
            else
                path.line_to ((float)a[i].x, (float)a[i].y);
        }
        Gdk.RGBA c;
        if (check_duplicate != null && check_duplicate (direction))
            c = {0.75f, 0f, 0f, 1f};
        else
            c = {0.2890625f, 0.5625f, 0.84765625f, 1f};
        s.append_fill (path.to_path (), EVEN_ODD, c);
    }
    /* check for duplicate call back function */
    unowned CheckDuplicate check_duplicate = null;
    internal void SetCheckDuplicate (CheckDuplicate f) {check_duplicate = f;}
}

[GtkTemplate (ui = "/org/gnome/Nibbles/ui/controls-grid.ui")]
private class ControlsGrid : Box
{
    [GtkChild] private unowned Overlay overlay;
    [GtkChild] private unowned Grid grid;
    [GtkChild] private unowned Button name_label;
    [GtkChild] private unowned Arrow  arrow_up;
    [GtkChild] private unowned Arrow  arrow_down;
    [GtkChild] private unowned Arrow  arrow_left;
    [GtkChild] private unowned Arrow  arrow_right;
    [GtkChild] private unowned Button move_up_button;
    [GtkChild] private unowned Button move_down_button;
    [GtkChild] private unowned Button move_left_button;
    [GtkChild] private unowned Button move_right_button;
    [GtkChild] private unowned ColourWheel wheel;

    internal Controls controls;
    internal WormProperties worm_props;
    internal ulong external_handler;
    private ulong    up_handler;
    private ulong  down_handler;
    private ulong  left_handler;
    private ulong right_handler;
    private ulong color_handler;

    OverlayMessage key_press_message;

    bool duplicate[4];

    internal ControlsGrid (Controls controls, int worm_id, WormProperties worm_props, Gee.HashMap<Worm, WormProperties> worms_props)
    {
        this.controls = controls;
        this.worm_props = worm_props;
        set_margin_bottom (10);

        /* Translators: text displayed in a screen showing the keys used by the players; the %d is replaced by the number that identifies the player */
        var player_id = _("Player %d").printf (worm_id + 1);
        color_handler = worm_props.notify ["color"].connect (() =>
        {
            var color = Pango.Color ();
            get_worm_pango_color (worm_props.color, true, ref color);
            ((Label)name_label.get_child ()).set_markup (@"<b><span font-family=\"Sans\" color=\"$(color.to_string ())\" size=\"x-large\">$(player_id)</span></b>");
        });
        var color = Pango.Color ();
        get_worm_pango_color (worm_props.color, true, ref color);
        ((Label)name_label.get_child ()).set_markup (@"<b><span font-family=\"Sans\" color=\"$(color.to_string ())\" size=\"x-large\">$(player_id)</span></b>");

        arrow_up.SetCheckDuplicate ((/*uint*/ i)=>{return duplicate[i];});
        arrow_down.SetCheckDuplicate ((/*uint*/ i)=>{return duplicate[i];});
        arrow_left.SetCheckDuplicate ((/*uint*/ i)=>{return duplicate[i];});
        arrow_right.SetCheckDuplicate ((/*uint*/ i)=>{return duplicate[i];});

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
            if (null == key_press_message)
            {
                if (overlay.visible)
                {
                    overlay.visible = false;
                    wheel.visible = true;
                    wheel.do_select_segment (this.worm_props.color, (/*int*/c)=>
                    {
                        if (c != -1)
                            swap_color (worms_props, c);
                        overlay.visible = true;
                        wheel.visible = false;
                    });
                }
                else
                {
                    overlay.visible = true;
                    wheel.visible = false;
                }
            }
        });

        move_up_button.clicked.connect (()=>
        {
            if (!wheel.visible && null == key_press_message)
            {
                /* Translators: text displayed in a message box directing the player to press the key they want to use to direct the worm up the screen */
                key_press_message = new OverlayMessage (_("Press a key for up."), grid.get_width ());
                overlay.add_overlay (key_press_message);
                controls.add_keypress_handler ((keyval, keycode, out remove_handler)=>
                {
                    remove_handler = true;
                    if (keyval != this.worm_props.up)
                        this.worm_props.up = keyval;
                    if (keycode != this.worm_props.raw_up)
                        this.worm_props.raw_up = (int)keycode;
                    overlay.remove_overlay (key_press_message);
                    key_press_message = null;
                    return true;
                });
            }
        });
        move_down_button.clicked.connect (()=>
        {
            if (!wheel.visible && null == key_press_message)
            {
                /* Translators: text displayed in a message box directing the player to press the key they want to use to direct the worm down the screen */
                key_press_message = new OverlayMessage (_("Press a key for down."), grid.get_width ());
                overlay.add_overlay (key_press_message);
                controls.add_keypress_handler ((keyval, keycode, out remove_handler)=>
                {
                    remove_handler = true;
                    if (keyval != this.worm_props.down)
                        this.worm_props.down = keyval;
                    if (keycode != this.worm_props.raw_down)
                        this.worm_props.raw_down = (int)keycode;
                    overlay.remove_overlay (key_press_message);
                    key_press_message = null;
                    return true;
                });
            }
        });
        move_left_button.clicked.connect (()=>
        {
            if (!wheel.visible && null == key_press_message)
            {
                /* Translators: text displayed in a message box directing the player to press the key they want to use to direct the worm left */
                key_press_message = new OverlayMessage (_("Press a key for left."), grid.get_width ());
                overlay.add_overlay (key_press_message);
                controls.add_keypress_handler ((keyval, keycode, out remove_handler)=>
                {
                    remove_handler = true;
                    if (keyval != this.worm_props.left)
                        this.worm_props.left = keyval;
                    if (keycode != this.worm_props.raw_left)
                        this.worm_props.raw_left = (int)keycode;
                    overlay.remove_overlay (key_press_message);
                    key_press_message = null;
                    return true;
                });
            }
        });
        move_right_button.clicked.connect (()=>
        {
            if (!wheel.visible && null == key_press_message)
            {
                /* Translators: text displayed in a message box directing the player to press the key they want to use to direct the worm right */
                key_press_message = new OverlayMessage (_("Press a key for right."), grid.get_width ());
                overlay.add_overlay (key_press_message);
                controls.add_keypress_handler ((keyval, keycode, out remove_handler)=>
                {
                    remove_handler = true;
                    if (keyval != this.worm_props.right)
                        this.worm_props.right = keyval;
                    if (keycode != this.worm_props.raw_right)
                        this.worm_props.raw_right = (int)keycode;
                    overlay.remove_overlay (key_press_message);
                    key_press_message = null;
                    return true;
                });
            }
        });
    }

    void swap_color (Gee.HashMap<Worm, WormProperties> worms_props, int c)
    {
        if (c != worm_props.color)
        {
            for (var i = worms_props.map_iterator ();i.next ();)
            {
                if (i.get_value ().color == c)
                {
                    // swap colors
                    i.get_value ().color = worm_props.color;
                    worm_props.color = c;
                    return;
                }
            }
            worm_props.color = (int)c;
        }
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

internal class OverlayMessage : Widget
{
    public string text { internal get; protected construct set; }
    internal int width {get;set;}

    public OverlayMessage (string text, int width)
    {
        Object (text: text, width: width);
    }

    public override void snapshot (Snapshot snapshot)
    {
        const float PIby2 = 1.570796326794896619231321691639751442f; /* Pi / 2 */
        const int border = 20;
        double text_width;
        double text_height;
        int font_size = calculate_font_size (text, width - border, out text_width, out text_height);
        float background_width = (float)text_width + border;
        float background_height = (float)text_height + border;
        float x = (get_width () - background_width) / 2;
        float y = (get_height () - background_height) / 2;
        #if USE_PILL_BUTTON
        float arc_radius = background_width < background_height ? background_width / 2 : background_height / 2;
        #else
        float arc_radius = background_width < background_height ? background_width / 3 : background_height / 3;
        #endif
        /* draw background */
        var background = new PathBuilder ();
        /* top right corner */
        background.move_to (x + background_width - arc_radius, y + 0);
        background.svg_arc_to (arc_radius, arc_radius, PIby2, false, true, x + background_width, y + arc_radius);
        /* bottom right corner */
        background.line_to (x + background_width, y + background_height - arc_radius);
        background.svg_arc_to (arc_radius, arc_radius, PIby2, false, true, x + background_width - arc_radius, y + background_height);
        /* bottom left corner */
        background.line_to (x + arc_radius, y + background_height);
        background.svg_arc_to (arc_radius, arc_radius, PIby2, false, true, x + 0, y + background_height - arc_radius);
        /* top left corner */
        background.line_to (x + 0, y + arc_radius);
        background.svg_arc_to (arc_radius, arc_radius, PIby2, false, true, x + arc_radius, y + 0);
        /* fill with colour */
        snapshot.append_fill (background.to_path (), EVEN_ODD, {0.0f, 0.0f, 0.0f, 0.9f});
        /* draw the text */
        draw_text_font_size (snapshot, (int)(x + (background_width - text_width) / 2),
            (int)(y + (background_height - text_height) / 2), text, font_size);
    }
    /* calculate the font size that fits in the space */
    int calculate_font_size (string text, int target_width, out double width, out double height)
    {
        bool rush_size_steps = true;
        int fail_count = 0;
        int last_font_size = 1;
        int target_font_size = 1;
        width = 0;
        height = 0;
        uint target_width_diff = uint.MAX;

        for (int font_size = 1;font_size < 128;)
        {
            var layout = create_pango_layout (text)  	;
            var font = null == layout.get_font_description () ?
                Pango.FontDescription.from_string ("Sans Bold 1pt") :
                layout.get_font_description ().copy ();
            font.set_size (Pango.SCALE * font_size);
            layout.set_font_description (font);
            layout.set_text (text, -1);
            Pango.Rectangle a,b;
            layout.get_extents (out a, out b);
            int width_diff = target_width - (int)(a.width / Pango.SCALE);
            if (width_diff > 0 && width_diff < target_width_diff)
            {
                target_width_diff = width_diff;
                target_font_size = font_size;
                width = a.width / Pango.SCALE;
                height = a.height / Pango.SCALE;
                if (!rush_size_steps)
                    fail_count = 0;
            }
            else
            {
                if (rush_size_steps)
                {
                    rush_size_steps = false;
                    font_size = last_font_size + 1;
                    fail_count = 0;
                }
                else if (fail_count > 2)
                    break;
                else
                    fail_count++;
            }
            if (rush_size_steps)
            {
                last_font_size = font_size;
                font_size *= 2;
            }
            else
                font_size++;
        }
        return target_font_size;
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
}
