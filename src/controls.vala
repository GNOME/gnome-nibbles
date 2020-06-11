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

using Gtk;

[GtkTemplate (ui = "/org/gnome/nibbles/ui/controls.ui")]
private class Controls : Box
{
    [GtkChild] private Box grids_box;

    private Gdk.Pixbuf arrow_pixbuf;
    private Gdk.Pixbuf arrow_key_pixbuf;

    internal void load_pixmaps (int tile_size)
    {
        arrow_pixbuf     = NibblesView.load_pixmap_file ("arrow.svg",     5 * tile_size, 5 * tile_size);
        arrow_key_pixbuf = NibblesView.load_pixmap_file ("arrow-key.svg", 5 * tile_size, 5 * tile_size);
    }

    internal void prepare (Gee.LinkedList<Worm> worms, Gee.HashMap<Worm, WormProperties?> worm_props)
    {
        foreach (var grid in grids_box.get_children ())
            grid.destroy ();

        foreach (var worm in worms)
        {
            if (worm.is_human)
            {
                var grid = new ControlsGrid (worm.id, worm_props.@get (worm), arrow_pixbuf, arrow_key_pixbuf);
                grids_box.add (grid);
            }
        }
    }
}

[GtkTemplate (ui = "/org/gnome/nibbles/ui/controls-grid.ui")]
private class ControlsGrid : Grid
{
    [GtkChild] private Label name_label;
    [GtkChild] private Image arrow_up;
    [GtkChild] private Image arrow_down;
    [GtkChild] private Image arrow_left;
    [GtkChild] private Image arrow_right;
    [GtkChild] private Label move_up_label;
    [GtkChild] private Label move_down_label;
    [GtkChild] private Label move_left_label;
    [GtkChild] private Label move_right_label;

    internal ControlsGrid (int worm_id, WormProperties worm_props, Gdk.Pixbuf arrow, Gdk.Pixbuf arrow_key)
    {
        var color = Pango.Color ();
        color.parse (NibblesView.colorval_name_untranslated (worm_props.color));

        /* Translators: text displayed in a screen showing the keys used by the players; the %d is replaced by the number that identifies the player */
        var player_id = _("Player %d").printf (worm_id + 1);
        name_label.set_markup (@"<b><span font-family=\"Sans\" color=\"$(color.to_string ())\">$(player_id)</span></b>");

        arrow_up.set_from_pixbuf    (arrow.rotate_simple (Gdk.PixbufRotation.NONE));
        arrow_down.set_from_pixbuf  (arrow.rotate_simple (Gdk.PixbufRotation.UPSIDEDOWN));
        arrow_left.set_from_pixbuf  (arrow.rotate_simple (Gdk.PixbufRotation.COUNTERCLOCKWISE));
        arrow_right.set_from_pixbuf (arrow.rotate_simple (Gdk.PixbufRotation.CLOCKWISE));

        configure_label (Gdk.keyval_name (worm_props.up),    ref move_up_label);
        configure_label (Gdk.keyval_name (worm_props.down),  ref move_down_label);
        configure_label (Gdk.keyval_name (worm_props.left),  ref move_left_label);
        configure_label (Gdk.keyval_name (worm_props.right), ref move_right_label);
    }

    private static void configure_label (string? key_name, ref Label label)
    {
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
            label.set_text ("");
        else
            label.set_text (@"$(key_name.up ())");
    }
}
