/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-

   Gnome Nibbles: Gnome Worm Game
   Copyright (C) 2020 Arnaud Bonatti <arnaud.bonatti@gmail.com>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

using Gtk;

[GtkTemplate (ui = "/org/gnome/Nibbles/ui/speed.ui")]
private class Speed : Box
{
    [GtkChild] private unowned ToggleButton speed4;
    [GtkChild] private unowned ToggleButton speed3;
    [GtkChild] private unowned ToggleButton speed2;
    [GtkChild] private unowned ToggleButton speed1;
    [GtkChild] private unowned ToggleButton enable_fake_bonuses;
    [GtkChild] private unowned Button next;

    private SimpleAction speed_action;
    private SimpleAction fakes_action;

    private const GLib.ActionEntry [] players_action_entries =
    {
        { "change-speed", null, "i",    "4",    change_speed },
        { "toggle-fakes", null, null,   "true", toggle_fakes }
    };

    construct
    {
        setup_label (speed4.get_child ());
        setup_label (speed3.get_child ());
        setup_label (speed2.get_child ());
        setup_label (speed1.get_child ());
        setup_label (enable_fake_bonuses.get_child ());

        next.set_margin_top (14);
        #if USE_PILL_BUTTON
        if (next.has_css_class ("play"))
        {
            next.remove_css_class ("play");
            next.add_css_class ("pill");
        }
        #endif

        SimpleActionGroup action_group = new SimpleActionGroup ();
        action_group.add_action_entries (players_action_entries, this);
        insert_action_group ("speed", action_group);

        speed_action = (SimpleAction) action_group.lookup_action ("change-speed");
        fakes_action = (SimpleAction) action_group.lookup_action ("toggle-fakes");
    }

    void setup_label (Widget w)
    {
        var label = (Label)w;
        label.set_markup (@"<b><span size=\"14.0pt\" font-family=\"Sans\">"+label.get_text ()+"</span></b>");
        label.set_margin_top (14);
        label.set_margin_bottom (14);
        label.set_margin_start (14);
        label.set_halign (Align.START);
    }

    internal inline void set_values (int speed, bool fakes)
    {
        speed_action.set_state (speed);
        fakes_action.set_state (fakes);
        set_frames (speed);
    }

    internal inline void get_values (out int speed, out bool fakes)
    {
        speed = speed_action.get_state ().get_int32 ();
        fakes = fakes_action.get_state ().get_boolean ();
    }

    private inline void change_speed (SimpleAction _speed_action, Variant variant)
    {
        int speed = variant.get_int32 ();
        if (speed < 1 || speed > 4)
            assert_not_reached ();
        _speed_action.set_state (speed);
        set_frames (speed);
    }

    private void set_frames (int speed)
    {
        speed4.set_has_frame (speed == 4);
        speed3.set_has_frame (speed == 3);
        speed2.set_has_frame (speed == 2);
        speed1.set_has_frame (speed == 1);
    }

    private void toggle_fakes (SimpleAction _fakes_action, Variant variant)
    {
        _fakes_action.set_state (!_fakes_action.get_state ().get_boolean ());
    }
}
