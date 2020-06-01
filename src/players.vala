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

[GtkTemplate (ui = "/org/gnome/nibbles/ui/players.ui")]
private class Players : Box
{
    [GtkChild] private ToggleButton worms4;
    [GtkChild] private ToggleButton worms5;
    [GtkChild] private ToggleButton worms6;

    private SimpleAction nibbles_number_action;
    private SimpleAction players_number_action;

    private const GLib.ActionEntry [] players_action_entries =
    {
        { "change-nibbles-number", null, "i", "4", change_nibbles_number },
        { "change-players-number", null, "i", "1", change_players_number }
    };

    construct
    {
        SimpleActionGroup action_group = new SimpleActionGroup ();
        action_group.add_action_entries (players_action_entries, this);
        insert_action_group ("players", action_group);

        nibbles_number_action = (SimpleAction) action_group.lookup_action ("change-nibbles-number");
        players_number_action = (SimpleAction) action_group.lookup_action ("change-players-number");
    }

    internal void set_values (int players_number, int number_of_ais)
    {
        nibbles_number_action.set_state (players_number + number_of_ais);
        players_number_action.set_state (players_number);
        update_buttons_labels ();
    }

    internal void get_values (out int players_number, out int number_of_ais)
    {
        players_number = players_number_action.get_state ().get_int32 ();
        number_of_ais  = nibbles_number_action.get_state ().get_int32 () - players_number;
    }

    private inline void change_players_number (SimpleAction _players_number_action, Variant variant)
    {
        int players_number = variant.get_int32 ();
        if (players_number < 1 || players_number > 4)
            assert_not_reached ();
        _players_number_action.set_state (players_number);

        update_buttons_labels ();
    }

    private void update_buttons_labels ()
    {
        int min_ai = 4 - players_number_action.get_state ().get_int32 ();

        worms4.set_label (@"_$min_ai");
        worms5.set_label (@"_$(min_ai + 1)");
        worms6.set_label (@"_$(min_ai + 2)");
    }

    private inline void change_nibbles_number (SimpleAction _nibbles_number_action, Variant variant)
    {
        int nibbles_number = variant.get_int32 ();
        if (nibbles_number < 4 || nibbles_number > 6)
            assert_not_reached ();
        _nibbles_number_action.set_state (nibbles_number);
    }
}
