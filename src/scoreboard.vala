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

[GtkTemplate (ui = "/org/gnome/Nibbles/ui/scoreboard.ui")]
private class Scoreboard : Box
{
    private Gee.HashMap<PlayerScoreBox, Worm> boxes = new Gee.HashMap<PlayerScoreBox, Worm> ();

    internal void register (Worm worm, string color_name, Image life)
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
        var box = new PlayerScoreBox (_("Worm %d").printf (worm.id + 1), color, worm.score, worm.lives, life);
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

    private Gee.LinkedList<Image> life_images = new Gee.LinkedList<Image> ();

    internal PlayerScoreBox (string name, Pango.Color color, int score, uint8 lives_left, Image _life)
    {
        name_label.set_markup ("<span color=\"" + color.to_string () + "\">" + name + "</span>");
        score_label.set_label (score.to_string ());

        for (uint8 i = 0; i < Worm.MAX_LIVES; i++)
        {
            var life = new Image.from_paintable (_life.get_paintable ());
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
