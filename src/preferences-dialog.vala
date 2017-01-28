/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Gnome Nibbles: Gnome Worm Game
 * Copyright (C) 2015 Gabriel Ivascu <ivascu.gabriel59@gmail.com>
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

[GtkTemplate (ui = "/org/gnome/nibbles/ui/preferences-dialog.ui")]
private class PreferencesDialog : Gtk.Dialog
{
    private Gtk.ApplicationWindow window;

    private Settings settings;
    private Gee.ArrayList<Settings> worm_settings;

    [GtkChild]
    private Gtk.Notebook notebook;
    [GtkChild]
    private Gtk.RadioButton beginner_radio_button;
    [GtkChild]
    private Gtk.RadioButton slow_radio_button;
    [GtkChild]
    private Gtk.RadioButton medium_radio_button;
    [GtkChild]
    private Gtk.RadioButton fast_radio_button;
    [GtkChild]
    private Gtk.CheckButton sound_check_button;
    [GtkChild]
    private Gtk.CheckButton fakes_check_button;
    [GtkChild]
    private Gtk.ListStore list_store_1;
    [GtkChild]
    private Gtk.ListStore list_store_2;
    [GtkChild]
    private Gtk.ListStore list_store_3;
    [GtkChild]
    private Gtk.ListStore list_store_4;
    [GtkChild]
    private Gtk.TreeView tree_view_1;
    [GtkChild]
    private Gtk.TreeView tree_view_2;
    [GtkChild]
    private Gtk.TreeView tree_view_3;
    [GtkChild]
    private Gtk.TreeView tree_view_4;
    [GtkChild]
    private Gtk.ComboBoxText combo_box_1;
    [GtkChild]
    private Gtk.ComboBoxText combo_box_2;
    [GtkChild]
    private Gtk.ComboBoxText combo_box_3;
    [GtkChild]
    private Gtk.ComboBoxText combo_box_4;

    private Gee.ArrayList<Gtk.RadioButton> radio_buttons;
    private Gee.ArrayList<Gtk.ListStore> list_stores;
    private Gee.ArrayList<Gtk.TreeView> tree_views;
    private Gee.ArrayList<Gtk.ComboBoxText> combo_boxes;

    public PreferencesDialog (Gtk.ApplicationWindow window, Settings settings, Gee.ArrayList<Settings> worm_settings)
    {
        Object (use_header_bar: 1);

        this.settings = settings;
        this.worm_settings = worm_settings;
        this.window = window;

        this.response.connect (() => {
            this.destroy ();
        });

        this.set_transient_for (window);

        /* Speed radio buttons */
        radio_buttons = new Gee.ArrayList<Gtk.RadioButton> ();
        radio_buttons.add (beginner_radio_button);
        radio_buttons.add (slow_radio_button);
        radio_buttons.add (medium_radio_button);
        radio_buttons.add (fast_radio_button);

        foreach (var radio_button in radio_buttons)
        {
            var speed = NibblesGame.MAX_SPEED - radio_buttons.index_of (radio_button);
            radio_button.set_active (speed == settings.get_int ("speed"));
            radio_button.toggled.connect (radio_button_toggled_cb);
        }

        /* Sound check button */
        sound_check_button.set_active (settings.get_boolean ("sound"));
        sound_check_button.toggled.connect (sound_toggled_cb);

        /* Fake bonuses check button */
        fakes_check_button.set_active (settings.get_boolean ("fakes"));
        fakes_check_button.toggled.connect (fakes_toggles_cb);

        /* Control keys */
        tree_views = new Gee.ArrayList<Gtk.TreeView> ();
        tree_views.add (tree_view_1);
        tree_views.add (tree_view_2);
        tree_views.add (tree_view_3);
        tree_views.add (tree_view_4);

        list_stores = new Gee.ArrayList<Gtk.ListStore> ();
        list_stores.add (list_store_1);
        list_stores.add (list_store_2);
        list_stores.add (list_store_3);
        list_stores.add (list_store_4);

        foreach (var list_store in list_stores)
        {
            var id = list_stores.index_of (list_store);
            var tree_view = tree_views[id];

            Gtk.TreeIter iter;
            list_store.append (out iter);
            var keyval = worm_settings[id].get_int ("key-up");
            list_store.set (iter, 0, "key-up", 1, _("Move up"), 2, keyval);
            list_store.append (out iter);
            keyval = worm_settings[id].get_int ("key-down");
            list_store.set (iter, 0, "key-down", 1, _("Move down"), 2, keyval);
            list_store.append (out iter);
            keyval = worm_settings[id].get_int ("key-left");
            list_store.set (iter, 0, "key-left", 1, _("Move left"), 2, keyval);
            list_store.append (out iter);
            keyval = worm_settings[id].get_int ("key-right");
            list_store.set (iter, 0, "key-right", 1, _("Move right"), 2, keyval);

            var label_renderer = new Gtk.CellRendererText ();
            tree_view.insert_column_with_attributes (-1, _("Action"), label_renderer, "text", 1);

            var key_renderer = new Gtk.CellRendererAccel ();
            key_renderer.editable = true;
            key_renderer.accel_mode = Gtk.CellRendererAccelMode.OTHER;
            key_renderer.accel_edited.connect (accel_edited_cb);
            key_renderer.accel_cleared.connect (accel_cleared_cb);
            tree_view.insert_column_with_attributes (-1, _("Key"), key_renderer, "accel-key", 2);

        }

        /* Worm color */
        combo_boxes = new Gee.ArrayList<Gtk.ComboBoxText> ();
        combo_boxes.add (combo_box_1);
        combo_boxes.add (combo_box_2);
        combo_boxes.add (combo_box_3);
        combo_boxes.add (combo_box_4);

        foreach (var combo_box in combo_boxes)
        {
            for (int i = 0; i < NibblesView.NUM_COLORS; i++)
                combo_box.append_text (NibblesView.colorval_name (i));

            var id = combo_boxes.index_of (combo_box);

            var color = worm_settings[id].get_enum ("color");
            combo_box.set_active (color);
            combo_box.changed.connect (combo_box_changed_cb);
        }
    }

    private void radio_button_toggled_cb (Gtk.ToggleButton button)
    {
        if (button.get_active ())
        {
            var speed = NibblesGame.MAX_SPEED - radio_buttons.index_of ((Gtk.RadioButton) button);
            settings.set_int ("speed", speed);
        }
    }

    private void sound_toggled_cb ()
    {
        var play_sound = sound_check_button.get_active ();
        settings.set_boolean ("sound", play_sound);
    }

    private void fakes_toggles_cb ()
    {
        var has_fakes = fakes_check_button.get_active ();
        settings.set_boolean ("fakes", has_fakes);
    }

    private void accel_edited_cb (Gtk.CellRendererAccel cell, string path_string, uint keyval,
                                  Gdk.ModifierType mask, uint hardware_keycode)
    {
        var path = new Gtk.TreePath.from_string (path_string);
        if (path == null)
            return;

        var id = notebook.get_current_page () - 1;
        var list_store = list_stores[id];

        Gtk.TreeIter it;
        if (!list_store.get_iter (out it, path))
            return;

        string? key = null;
        list_store.get (it, 0, out key);
        if (key == null)
            return;

        if (worm_settings[id].get_int (key) == keyval)
            return;

        /* Duplicate key check */
        bool valid = true;
        for (int i = 0; i < NibblesGame.MAX_HUMANS; i++)
        {
            if (worm_settings[i].get_int ("key-up") == keyval ||
                worm_settings[i].get_int ("key-down") == keyval ||
                worm_settings[i].get_int ("key-left") == keyval ||
                worm_settings[i].get_int ("key-right") == keyval)
            {
                valid = false;

                var dialog = new Gtk.MessageDialog (window,
                                                    Gtk.DialogFlags.DESTROY_WITH_PARENT,
                                                    Gtk.MessageType.WARNING,
                                                    Gtk.ButtonsType.OK,
                                                    /* Translators: This string appears when one tries to assign an already assigned key */
                                                    _("The key you selected is already assigned!"));

                dialog.run ();
                dialog.destroy ();
                break;
            }
        }

        if (valid)
        {
            list_store.set (it, 2, keyval);
            worm_settings[id].set_int (key, (int) keyval);
        }
    }

    private void accel_cleared_cb (Gtk.CellRendererAccel cell, string path_string)
    {
        var path = new Gtk.TreePath.from_string (path_string);
        if (path == null)
            return;

        var id = notebook.get_current_page () - 1;
        var list_store = list_stores[id];

        Gtk.TreeIter it;
        if (!list_store.get_iter (out it, path))
            return;

        string? key = null;
        list_store.get (it, 0, out key);
        if (key == null)
            return;

        list_store.set (it, 2, 0);
        worm_settings[id].set_int (key, 0);
    }

    private void combo_box_changed_cb (Gtk.ComboBox combo_box)
    {
        var id = combo_boxes.index_of ((Gtk.ComboBoxText) combo_box);
        var color_new = combo_box.get_active ();
        var color_old = worm_settings[id].get_enum ("color");

        if (color_new == color_old)
            return;

        /* Swap the colors if the new color is already set for another worm */
        for (int i = 0; i < NibblesGame.MAX_WORMS; i++)
        {
            if (i != id && worm_settings[i].get_enum ("color") == color_new)
            {
                worm_settings[i].set_enum ("color", color_old);

                /* Update swapped colors in UI */
                if (i < NibblesGame.MAX_HUMANS)
                {
                    foreach (var cbox in combo_boxes)
                    {
                        var index = combo_boxes.index_of (cbox);
                        if (index == i)
                        {
                            cbox.set_active (color_old);
                            break;
                        }
                    }
                }

                break;
            }
        }

        worm_settings[id].set_enum ("color", color_new);
    }
}
