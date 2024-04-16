/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-

   Gnome Nibbles: Gnome Worm Game
   Copyright (C) 2024 Ben Corby

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

[GtkTemplate (ui = "/org/gnome/Nibbles/ui/board-progress.ui")]
private class BoardProgress : Box
{
    [GtkChild] private unowned ToggleButton linier;
    [GtkChild] private unowned ToggleButton random;
    [GtkChild] private unowned ToggleButton fixed;
    [GtkChild] private unowned Button next;
    [GtkChild] private unowned Overlay overlay;

    private SpinButton spin = new SpinButton.with_range (1, NibblesGame.MAX_LEVEL, 1);
    bool spin_in_use = false;

    private SimpleAction progress_action;

    private const GLib.ActionEntry [] players_action_entries =
    {
        { "change-progress", null, "i",    "3", change_progress }
    };

    construct
    {
        setup_label (linier.get_child ());
        setup_label (random.get_child ());
        setup_label (fixed.get_child ());

        next.set_margin_top (14);
        #if USE_PILL_BUTTON
        if (next.has_css_class ("play"))
        {
            next.remove_css_class ("play");
            next.add_css_class ("pill");
        }
        #endif

        spin.set_halign (END);

        SimpleActionGroup action_group = new SimpleActionGroup ();
        action_group.add_action_entries (players_action_entries, this);
        insert_action_group ("progress", action_group);

        progress_action = (SimpleAction) action_group.lookup_action ("change-progress");
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

    internal void set_values (int progress, int level)
    {
        if (progress < 0 || progress > 2)
            progress = 0;
        if (level < 1 || level > NibblesGame.MAX_LEVEL)
            level = 1;
        progress_action.set_state (progress);
        spin.value = level;
        set_frames (progress);
    }

    internal void get_values (out int progress, out int level)
    {
        progress = progress_action.get_state ().get_int32 ();
        level = spin.get_value_as_int ();
    }

    private void change_progress (SimpleAction _action, Variant variant)
    {
        int progress = variant.get_int32 ();
        assert (progress >= 0 && progress <= 2);
        _action.set_state (progress);
        set_frames (progress);
    }

    private void set_frames (int progress)
    {
        linier.set_has_frame (progress == 0);
        random.set_has_frame (progress == 1);
        fixed.set_has_frame (progress == 2);
        if (progress == 2)
        {
            if (!spin_in_use)
                overlay.add_overlay (spin);
            spin_in_use = true;
        }
        else
        {
            if (spin_in_use)
                overlay.remove_overlay (spin);
            spin_in_use = false;
        }
    }
}
