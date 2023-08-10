/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Gnome Nibbles: Gnome Worm Game
 * Copyright (C) 2015 Iulian-Gabriel Radu <iulian.radu67@gmail.com>
 * Copyright (C) 2020 Arnaud Bonatti <arnaud.bonatti@gmail.com>
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

    private Gdk.Pixbuf arrow_pixbuf;

    internal void load_pixmaps (int tile_size)
    {
        arrow_pixbuf = NibblesView.load_pixmap_file ("arrow.svg", 5 * tile_size, 5 * tile_size);
    }

    internal void prepare (Gee.LinkedList<Worm> worms, Gee.HashMap<Worm, WormProperties> worm_props)
    {
        foreach (var grid in grids_box.get_children ())
            grid.destroy ();

        GenericSet<uint> duplicate_keys     = new GenericSet<uint> (direct_hash, direct_equal);
        GenericSet<uint> encountered_keys   = new GenericSet<uint> (direct_hash, direct_equal);
        foreach (var worm in worms)
        {
            if (worm.is_human)
            {
                WormProperties worm_prop = worm_props.@get (worm);

                var grid = new ControlsGrid (worm.id, worm_prop, arrow_pixbuf);
                grids_box.add (grid);
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
                            WormProperties worm_prop = worm_props.@get (worm);

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
private class ControlsGrid : Button
{
    [GtkChild] private unowned Label name_label;
    [GtkChild] private unowned Image arrow_up;
    [GtkChild] private unowned Image arrow_down;
    [GtkChild] private unowned Image arrow_left;
    [GtkChild] private unowned Image arrow_right;
    [GtkChild] private unowned Label move_up_label;
    [GtkChild] private unowned Label move_down_label;
    [GtkChild] private unowned Label move_left_label;
    [GtkChild] private unowned Label move_right_label;

    internal WormProperties worm_props;
    internal ulong external_handler;
    private ulong    up_handler;
    private ulong  down_handler;
    private ulong  left_handler;
    private ulong right_handler;
    private ulong color_handler;

    internal ControlsGrid (int worm_id, WormProperties worm_props, Gdk.Pixbuf arrow)
    {
        this.worm_props = worm_props;

        set_action_target ("i", worm_id + 1);

        /* Translators: text displayed in a screen showing the keys used by the players; the %d is replaced by the number that identifies the player */
        var player_id = _("Player %d").printf (worm_id + 1);
        color_handler = worm_props.notify ["color"].connect (() => {
                var color = Pango.Color ();
                color.parse (NibblesView.colorval_name_untranslated (worm_props.color));
                name_label.set_markup (@"<b><span font-family=\"Sans\" color=\"$(color.to_string ())\">$(player_id)</span></b>");
            });
        var color = Pango.Color ();
        color.parse (NibblesView.colorval_name_untranslated (worm_props.color));
        name_label.set_markup (@"<b><span font-family=\"Sans\" color=\"$(color.to_string ())\">$(player_id)</span></b>");

        arrow_up.set_from_pixbuf    (arrow.rotate_simple (Gdk.PixbufRotation.NONE));
        arrow_down.set_from_pixbuf  (arrow.rotate_simple (Gdk.PixbufRotation.UPSIDEDOWN));
        arrow_left.set_from_pixbuf  (arrow.rotate_simple (Gdk.PixbufRotation.COUNTERCLOCKWISE));
        arrow_right.set_from_pixbuf (arrow.rotate_simple (Gdk.PixbufRotation.CLOCKWISE));

           up_handler = worm_props.notify ["up"].connect    (() => configure_label (worm_props.up,    move_up_label));
         down_handler = worm_props.notify ["down"].connect  (() => configure_label (worm_props.down,  move_down_label));
         left_handler = worm_props.notify ["left"].connect  (() => configure_label (worm_props.left,  move_left_label));
        right_handler = worm_props.notify ["right"].connect (() => configure_label (worm_props.right, move_right_label));

        configure_label (worm_props.up,    move_up_label);
        configure_label (worm_props.down,  move_down_label);
        configure_label (worm_props.left,  move_left_label);
        configure_label (worm_props.right, move_right_label);
    }

    internal void mark_duplicated_keys (GenericSet<uint> duplicate_keys)
    {
        set_duplicate_class (worm_props.up    in duplicate_keys, move_up_label);
        set_duplicate_class (worm_props.down  in duplicate_keys, move_down_label);
        set_duplicate_class (worm_props.left  in duplicate_keys, move_left_label);
        set_duplicate_class (worm_props.right in duplicate_keys, move_right_label);
    }
    private static void set_duplicate_class (bool new_value, Label label)
    {
        if (new_value)
            label.get_style_context ().add_class ("duplicate");
        else
            label.get_style_context ().remove_class ("duplicate");
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
            label.get_style_context ().add_class ("arrow");
            label.set_text ("↑");
        }
        else if (key_name == "Down")
        {
            label.get_style_context ().add_class ("arrow");
            label.set_text ("↓");
        }
        else if (key_name == "Left")
        {
            label.get_style_context ().add_class ("arrow");
            label.set_text ("←");
        }
        else if (key_name == "Right")
        {
            label.get_style_context ().add_class ("arrow");
            label.set_text ("→");
        }
        else if (key_name == null || key_name == "")
        {
            label.get_style_context ().remove_class ("arrow");
            label.set_text ("");
        }
        else
        {
            label.get_style_context ().remove_class ("arrow");
            label.set_text (@"$(accelerator_get_label (key_value, 0))");
        }
    }
}
