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

using Gtk;

[GtkTemplate (ui = "/org/gnome/Nibbles/ui/preferences-dialog.ui")]
private class PreferencesDialog : Window
{
    private GLib.Settings settings;
    private Gee.ArrayList<GLib.Settings> worm_settings;

    [GtkChild] private Stack            stack;
    [GtkChild] private Stack            headerbar_stack;
    [GtkChild] private ComboBoxText     worm_combobox;
    [GtkChild] private Gtk.ListStore    list_store_1;
    [GtkChild] private Gtk.ListStore    list_store_2;
    [GtkChild] private Gtk.ListStore    list_store_3;
    [GtkChild] private Gtk.ListStore    list_store_4;
    [GtkChild] private TreeView         tree_view_1;
    [GtkChild] private TreeView         tree_view_2;
    [GtkChild] private TreeView         tree_view_3;
    [GtkChild] private TreeView         tree_view_4;
    [GtkChild] private ComboBoxText     combo_box_1;
    [GtkChild] private ComboBoxText     combo_box_2;
    [GtkChild] private ComboBoxText     combo_box_3;
    [GtkChild] private ComboBoxText     combo_box_4;

    private Gee.ArrayList<Gtk.ListStore>    list_stores;
    private Gee.ArrayList<TreeView>         tree_views;
    private Gee.ArrayList<ComboBoxText>     combo_boxes;

    private EventControllerKey key_controller;          // for keeping in memory

    public int n_worms { private get; protected construct; }

    construct
    {
        key_controller = new EventControllerKey (this);
        key_controller.key_pressed.connect (on_key_pressed);

        tree_views = new Gee.ArrayList<TreeView> ();
        tree_views.add (tree_view_1);
        tree_views.add (tree_view_2);
        tree_views.add (tree_view_3);
        tree_views.add (tree_view_4);

        list_stores = new Gee.ArrayList<Gtk.ListStore> ();
        list_stores.add (list_store_1);
        list_stores.add (list_store_2);
        list_stores.add (list_store_3);
        list_stores.add (list_store_4);

        combo_boxes = new Gee.ArrayList<ComboBoxText> ();
        combo_boxes.add (combo_box_1);
        combo_boxes.add (combo_box_2);
        combo_boxes.add (combo_box_3);
        combo_boxes.add (combo_box_4);
    }

    internal PreferencesDialog (ApplicationWindow window, GLib.Settings settings, Gee.ArrayList<GLib.Settings> worm_settings, int worm_id, int n_worms)
    {
        Object (transient_for: window, n_worms: n_worms);

        this.settings = settings;
        this.worm_settings = worm_settings;

        if (n_worms == 1)
            headerbar_stack.set_visible_child_name ("preferences-label");
        else if (n_worms != 4)
            for (int i = 3; i > n_worms - 1; i--)
                worm_combobox.remove (i);

        /* Control keys */
        foreach (var list_store in list_stores)
        {
            var id = list_stores.index_of (list_store);
            var tree_view = tree_views[id];

            TreeIter iter;
            list_store.append (out iter);
            var keyval = worm_settings[id].get_int ("key-up");
            /* Translators: in the Preferences dialog, label of an option (available for each playable worm) for changing the key to move the given worm up */
            list_store.@set (iter, 0, "key-up", 1, _("Move up"), 2, keyval);
            list_store.append (out iter);
            keyval = worm_settings[id].get_int ("key-down");
            /* Translators: in the Preferences dialog, label of an option (available for each playable worm) for changing the key to move the given worm down */
            list_store.@set (iter, 0, "key-down", 1, _("Move down"), 2, keyval);
            list_store.append (out iter);
            keyval = worm_settings[id].get_int ("key-left");
            /* Translators: in the Preferences dialog, label of an option (available for each playable worm) for changing the key to move the given worm left */
            list_store.@set (iter, 0, "key-left", 1, _("Move left"), 2, keyval);
            list_store.append (out iter);
            keyval = worm_settings[id].get_int ("key-right");
            /* Translators: in the Preferences dialog, label of an option (available for each playable worm) for changing the key to move the given worm right */
            list_store.@set (iter, 0, "key-right", 1, _("Move right"), 2, keyval);

            var label_renderer = new CellRendererText ();
            /* Translators: in the Preferences dialog, label of a column in a table for changing the keys to move the given worm (available for each playable worm); are listed there all the actions a player can do with its worm; the other column is "Key" */
            tree_view.insert_column_with_attributes (-1, _("Action"), label_renderer, "text", 1);

            var key_renderer = new CellRendererAccel ();
            key_renderer.editable = true;
            key_renderer.accel_mode = CellRendererAccelMode.OTHER;
            key_renderer.accel_edited.connect (accel_edited_cb);
            key_renderer.accel_cleared.connect (accel_cleared_cb);
            /* Translators: in the Preferences dialog, label of a column in a table for changing the keys to move the given worm (available for each playable worm); are listed there all the keys a player can use with its worm; the other column is "Action" */
            tree_view.insert_column_with_attributes (-1, _("Key"), key_renderer, "accel-key", 2);
        }

        /* Worm color */
        foreach (var combo_box in combo_boxes)
        {
            for (int i = 0; i < NibblesView.NUM_COLORS; i++)
                combo_box.append_text (NibblesView.colorval_name_translated (i));

            var id = combo_boxes.index_of (combo_box);

            var color = worm_settings[id].get_enum ("color");
            combo_box.set_active (color);
            combo_box.changed.connect (combo_box_changed_cb);
        }

        /* Choose correct worm */
        worm_combobox.set_active (worm_id - 1);
    }

    private inline bool on_key_pressed (EventControllerKey _key_controller, uint keyval, uint keycode, Gdk.ModifierType state)
    {
        string name = (!) (Gdk.keyval_name (keyval) ?? "");
        if (name == "Escape")
            destroy ();
        return false;
    }

    [GtkCallback]
    private inline void on_worm_change (ComboBox _worm_combobox)
    {
        stack.set_visible_child_name (((ComboBoxText) _worm_combobox).get_active_id ());
    }

    private void accel_edited_cb (CellRendererAccel cell, string path_string, uint keyval,
                                  Gdk.ModifierType mask, uint hardware_keycode)
    {
        var path = new TreePath.from_string (path_string);
        if (path == null)
            return;

        var id = worm_combobox.get_active ();
        var list_store = list_stores[id];

        TreeIter it;
        if (!list_store.get_iter (out it, path))
            return;

        string? key = null;
        list_store.@get (it, 0, out key);
        if (key == null)
            return;

        if (keyval == worm_settings [id].get_int (key))
            return;

        /* Duplicate key check */
        bool valid = true;
        for (int i = 0; i < n_worms; i++)
        {
            if (keyval == worm_settings [i].get_int ("key-up")
             || keyval == worm_settings [i].get_int ("key-down")
             || keyval == worm_settings [i].get_int ("key-left")
             || keyval == worm_settings [i].get_int ("key-right"))
            {
                var dialog = new MessageDialog (this,
                                                DialogFlags.DESTROY_WITH_PARENT | DialogFlags.MODAL,
                                                MessageType.WARNING,
                                                ButtonsType.CANCEL,
                                                /* Translators: label of a MessageDialog that appears when one tries to assign an already assigned key */
                                                _("The key you selected is already assigned!"));

                /* Translators: label of one of the buttons of a MessageDialog that appears when one tries to assign an already assigned key (with a mnemonic that appears when pressing Alt) */
                dialog.add_button (_("_Set anyway"), 42);

                dialog.response.connect ((_dialog, response) => {
                        _dialog.destroy ();
                        if (response == 42)
                        {
                            list_store.@set (it, 2, keyval);
                            worm_settings[id].set_int (key, (int) keyval);
                        }
                    });
                dialog.present ();
                return;
            }
        }

        if (valid)
        {
            list_store.@set (it, 2, keyval);
            worm_settings[id].set_int (key, (int) keyval);
        }
    }

    private void accel_cleared_cb (CellRendererAccel cell, string path_string)
    {
        var path = new TreePath.from_string (path_string);
        if (path == null)
            return;

        var id = worm_combobox.get_active ();
        var list_store = list_stores[id];

        TreeIter it;
        if (!list_store.get_iter (out it, path))
            return;

        string? key = null;
        list_store.@get (it, 0, out key);
        if (key == null)
            return;

        list_store.@set (it, 2, 0);
        worm_settings[id].set_int (key, 0);
    }

    private void combo_box_changed_cb (ComboBox combo_box)
    {
        var id = combo_boxes.index_of ((ComboBoxText) combo_box);
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
                if (i < n_worms)
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
