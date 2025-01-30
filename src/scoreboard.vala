/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Gnome Nibbles: Gnome Worm Game
 * Copyright (C) 2015 Iulian-Gabriel Radu <iulian.radu67@gmail.com>
 * Copyright (C) 2024-2025 Ben Corby <bcorby@new-ms.com>
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
 * grep -ne ' $' *.vala
 *
 */

using Gtk;
using Gsk;

[GtkTemplate (ui = "/org/gnome/Nibbles/ui/scoreboard.ui")]
private class Scoreboard : Box
{
    private Gee.HashMap<PlayerScoreBox, Worm> boxes = new Gee.HashMap<PlayerScoreBox, Worm> ();

    internal void register (Worm worm, string color_name)
    {
        var color = Pango.Color ();
        if (color_name == "red")
            get_worm_pango_color (0, true, ref color);
        else if (color_name == "green")
            get_worm_pango_color (1, true, ref color);
        else if (color_name == "blue")
            get_worm_pango_color (2, true, ref color);
        else if (color_name == "yellow")
            get_worm_pango_color (3, true, ref color);
        else if (color_name == "cyan")
            get_worm_pango_color (4, true, ref color);
        else if (color_name == "purple")
            get_worm_pango_color (5, true, ref color);
        else
            get_worm_pango_color (-1, true, ref color);

        /* Translators: text displayed under the game view, presenting the number of remaining lives; the %d is replaced by the number that identifies the player */
        var box = new PlayerScoreBox (_("Worm %d").printf (worm.id + 1), color, worm.score, worm.lives);
        boxes.@set (box, worm);
        append (box);
    }

    internal void update ()
    {
        foreach (var entry in boxes.entries)
        {
            var box = entry.key;
            var worm = entry.@value;

            box.update (worm.score, worm.lives);
        }
    }

    internal void clear ()
    {
        foreach (var entry in boxes.entries)
            remove (entry.key);
        boxes.clear ();
    }
}

[GtkTemplate (ui = "/org/gnome/Nibbles/ui/player-score-box.ui")]
private class PlayerScoreBox : Box
{
    [GtkChild] private unowned Label name_label;
    [GtkChild] private unowned Label score_label;
    [GtkChild] private unowned Grid lives_grid;

    private Gee.LinkedList<Life> life_images = new Gee.LinkedList<Life> ();

    internal PlayerScoreBox (string name, Pango.Color color, int score, uint8 lives_left)
    {
        name_label.set_markup ("<span color=\"" + color.to_string () + "\">" + name + "</span>");
        score_label.set_label (score.to_string ());

        for (uint8 i = 0; i < Worm.MAX_LIVES; i++)
        {
            var life = new Life ();
            if (i >= Worm.STARTING_LIVES)
                life.set_opacity (0);
            life_images.add (life);
            lives_grid.attach (life, i % 6, i / 6);
        }
    }

    internal void update (int score, uint8 lives_left)
    {
        update_score (score);
        update_lives (lives_left);
    }

    internal inline void update_score (int score)
    {
        score_label.set_label (score.to_string ());
    }

    internal void update_lives (uint8 lives_left)
    {
        /* Remove lost lives - if any */
        for (uint8 i = (uint8) life_images.size; i > lives_left; i--)
            life_images[i - 1].set_opacity (0);

        /* Add new lives - if any */
        for (uint8 i = 0; i < lives_left; i++)
            life_images[i].set_opacity (1);
    }
}

internal class Life : Widget
{
    /* initilise the widget */
    construct
    {
        width_request = 16;
        height_request = 16;
    }
    /* draw the heart */
    public override void snapshot (Snapshot snapshot)
    {
        base.snapshot (snapshot);
        var path = new PathBuilder ();
        float x_m = get_width () / 16;
        float y_m = get_height () / 16;
        const float x = 0;
        const float y = 0;
        path.move_to (x + x_m * 4.753906f, y + y_m * 1.828125f);
        path.cubic_to (x + x_m * 2.652344f, y + y_m * 1.851563f, x + x_m * 1.019531f, y + y_m * 3.648438f, x + x_m * 1.0f, y + y_m * 5.8125f);
        path.cubic_to (x + x_m * 0.972656f, y + y_m * 8.890625f, x + x_m * 2.808594f, y + y_m * 9.882813f, x + x_m * 8.015625f, y + y_m * 14.171875f);
        path.cubic_to (x + x_m * 12.992188f, y + y_m * 9.558594f, x + x_m * 14.976563f, y + y_m * 8.316406f, x + x_m * 15.0f, y + y_m * 5.722656f);
        path.cubic_to (x + x_m * 15.027344f, y + y_m * 2.886719f, x + x_m * 10.90625f, y + y_m * 0.128906f, x + x_m * 7.910156f, y + y_m * 3.121094f);
        path.cubic_to (x + x_m * 6.835938f, y + y_m * 2.199219f, x + x_m * 5.742188f, y + y_m * 1.816406f, x + x_m * 4.753906f, y + y_m * 1.828125f);
        snapshot.append_fill (path.to_path (), EVEN_ODD, {1.0f, 0.0f, 0.0f, 1.0f});
    }
}
