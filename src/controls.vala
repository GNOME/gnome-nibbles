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

using Gtk;

[GtkTemplate (ui = "/org/gnome/nibbles/ui/controls-grid.ui")]
private class ControlsGrid : Grid
{
    [GtkChild] private Label name_label;
    [GtkChild] private Image arrow_up;
    [GtkChild] private Image arrow_down;
    [GtkChild] private Image arrow_left;
    [GtkChild] private Image arrow_right;
    [GtkChild] private Overlay move_up;
    [GtkChild] private Label move_up_label;
    [GtkChild] private Overlay move_down;
    [GtkChild] private Label move_down_label;
    [GtkChild] private Overlay move_left;
    [GtkChild] private Label move_left_label;
    [GtkChild] private Overlay move_right;
    [GtkChild] private Label move_right_label;

    internal ControlsGrid (int worm_id, WormProperties worm_props, Gdk.Pixbuf arrow, Gdk.Pixbuf arrow_key)
    {
        var color = Pango.Color ();
        color.parse (NibblesView.colorval_name_untranslated (worm_props.color));

        /* Translators: text displayed in a screen showing the keys used by the players; the %d is replaced by the number that identifies the player */
        var player_id = _("Player %d").printf (worm_id + 1);
        name_label.set_markup (@"<b><span font-family=\"Sans\" color=\"$(color.to_string ())\">$(player_id)</span></b>");

        arrow_up.set_from_pixbuf (arrow.rotate_simple (Gdk.PixbufRotation.NONE));
        arrow_down.set_from_pixbuf (arrow.rotate_simple (Gdk.PixbufRotation.UPSIDEDOWN));
        arrow_left.set_from_pixbuf (arrow.rotate_simple (Gdk.PixbufRotation.COUNTERCLOCKWISE));
        arrow_right.set_from_pixbuf (arrow.rotate_simple (Gdk.PixbufRotation.CLOCKWISE));

        string upper_key;
        upper_key = Gdk.keyval_name (worm_props.up).up ();
        if (upper_key == "UP")
        {
            var rotated_pixbuf = arrow_key.rotate_simple (Gdk.PixbufRotation.NONE);
            move_up.add_overlay (new Image.from_pixbuf (rotated_pixbuf));
            move_up.show_all ();
        }
        else
            move_up_label.set_markup (@"<b>$(upper_key)</b>");

        upper_key = Gdk.keyval_name (worm_props.down).up ();
        if (upper_key == "DOWN")
        {
            var rotated_pixbuf = arrow_key.rotate_simple (Gdk.PixbufRotation.UPSIDEDOWN);
            move_down.add_overlay (new Image.from_pixbuf (rotated_pixbuf));
            move_down.show_all ();
        }
        else
            move_down_label.set_markup (@"<b>$(upper_key)</b>");

        upper_key = Gdk.keyval_name (worm_props.left).up ();
        if (upper_key == "LEFT")
        {
            var rotated_pixbuf = arrow_key.rotate_simple (Gdk.PixbufRotation.COUNTERCLOCKWISE);
            move_left.add_overlay (new Image.from_pixbuf (rotated_pixbuf));
            move_left.show_all ();
        }
        else
            move_left_label.set_markup (@"<b>$(upper_key)</b>");

        upper_key = Gdk.keyval_name (worm_props.right).up ();
        if (upper_key == "RIGHT")
        {
            var rotated_pixbuf = arrow_key.rotate_simple (Gdk.PixbufRotation.CLOCKWISE);
            move_right.add_overlay (new Image.from_pixbuf (rotated_pixbuf));
            move_right.show_all ();
        }
        else
            move_right_label.set_markup (@"<b>$(upper_key)</b>");
    }
}