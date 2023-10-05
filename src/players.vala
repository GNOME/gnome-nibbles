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

[GtkTemplate (ui = "/org/gnome/Nibbles/ui/players.ui")]
private class Players : Box
{
    [GtkChild] private unowned ToggleButton players1;
    [GtkChild] private unowned ToggleButton players2;
    [GtkChild] private unowned ToggleButton players3;
    [GtkChild] private unowned ToggleButton players4;
    [GtkChild] private unowned ToggleButton worms4;
    [GtkChild] private unowned ToggleButton worms5;
    [GtkChild] private unowned ToggleButton worms6;

    private SimpleAction nibbles_number_action;
    private SimpleAction players_number_action;

    /* Translators: in the "Number of players" configuration screen, label of a button allowing to play game only with humans, appearing when there are 4 humans set */
    private const string ai0_label = _("0");

    /* Translators: in the "Number of players" configuration screen, label of a button allowing to play game with one AI, appearing when there are 3 or 4 humans set */
    private const string ai1_label = _("1");

    /* Translators: in the "Number of players" configuration screen, label of a button allowing to play game with two AIs, appearing when there are 2, 3 or 4 humans set */
    private const string ai2_label = _("2");

    /* Translators: in the "Number of players" configuration screen, label of a button allowing to play game with three AIs, appearing when there are 1, 2 or 3 humans set */
    private const string ai3_label = _("3");

    /* Translators: in the "Number of players" configuration screen, label of a button allowing to play game with four AIs, appearing when there are 1 or 2 humans set */
    private const string ai4_label = _("4");

    /* Translators: in the "Number of players" configuration screen, label of a button allowing to play game with five AIs, appearing when there is only 1 human set  */
    private const string ai5_label = _("5");

    private const GLib.ActionEntry [] players_action_entries =
    {
        { "change-nibbles-number", null, "i", "4", change_nibbles_number },
        { "change-players-number", null, "i", "1", change_players_number }
    };

    construct
    {
        ((Label)players1.get_child ()).set_markup (@"<b><span size=\"30.0pt\" font-family=\"Sans\">1</span></b>");
        ((Label)players2.get_child ()).set_markup (@"<b><span size=\"30.0pt\" font-family=\"Sans\">2</span></b>");
        ((Label)players3.get_child ()).set_markup (@"<b><span size=\"30.0pt\" font-family=\"Sans\">3</span></b>");
        ((Label)players4.get_child ()).set_markup (@"<b><span size=\"30.0pt\" font-family=\"Sans\">4</span></b>");

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
        switch (players_number_action.get_state ().get_int32 ())
        {
            case 1:
                ((Label)worms4.get_child ()).set_markup (@"<b><span size=\"30.0pt\" font-family=\"Sans\">"+_(ai3_label)+"</span></b>");
                ((Label)worms5.get_child ()).set_markup (@"<b><span size=\"30.0pt\" font-family=\"Sans\">"+_(ai4_label)+"</span></b>");
                ((Label)worms6.get_child ()).set_markup (@"<b><span size=\"30.0pt\" font-family=\"Sans\">"+_(ai5_label)+"</span></b>");
                break;
            case 2:
                ((Label)worms4.get_child ()).set_markup (@"<b><span size=\"30.0pt\" font-family=\"Sans\">"+_(ai2_label)+"</span></b>");
                ((Label)worms5.get_child ()).set_markup (@"<b><span size=\"30.0pt\" font-family=\"Sans\">"+_(ai3_label)+"</span></b>");
                ((Label)worms6.get_child ()).set_markup (@"<b><span size=\"30.0pt\" font-family=\"Sans\">"+_(ai4_label)+"</span></b>");
                break;
            case 3:
                ((Label)worms4.get_child ()).set_markup (@"<b><span size=\"30.0pt\" font-family=\"Sans\">"+_(ai1_label)+"</span></b>");
                ((Label)worms5.get_child ()).set_markup (@"<b><span size=\"30.0pt\" font-family=\"Sans\">"+_(ai2_label)+"</span></b>");
                ((Label)worms6.get_child ()).set_markup (@"<b><span size=\"30.0pt\" font-family=\"Sans\">"+_(ai3_label)+"</span></b>");
                break;
            case 4:
                ((Label)worms4.get_child ()).set_markup (@"<b><span size=\"30.0pt\" font-family=\"Sans\">"+_(ai0_label)+"</span></b>");
                ((Label)worms5.get_child ()).set_markup (@"<b><span size=\"30.0pt\" font-family=\"Sans\">"+_(ai1_label)+"</span></b>");
                ((Label)worms6.get_child ()).set_markup (@"<b><span size=\"30.0pt\" font-family=\"Sans\">"+_(ai2_label)+"</span></b>");
                break;
            default:
                assert_not_reached ();
        }
    }

    private inline void change_nibbles_number (SimpleAction _nibbles_number_action, Variant variant)
    {
        int nibbles_number = variant.get_int32 ();
        if (nibbles_number < 4 || nibbles_number > 6)
            assert_not_reached ();
        _nibbles_number_action.set_state (nibbles_number);
    }
}
