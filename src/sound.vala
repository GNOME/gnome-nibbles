/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Gnome Nibbles: Gnome Worm Game
 * Copyright (C) 2023, 2025 Ben Corby <bcorby@new-ms.com>
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
using Gtk; /* designed for Gtk 4, link with libgtk-4-dev or gtk4-devel */
internal class Sound : Object
{
    internal bool mute {get; set;}
    internal Sound (bool mute)
    {
        Object (mute: mute);
    }
    internal void set_muted (bool mute)
    {
        this.mute = mute;
    }
    internal void connect_signal (NibblesGame game)
    {
        game.play_sound.connect (play_sound);
    }
    Gee.TreeMap<string, MediaFile> sound_map = new Gee.TreeMap<string, MediaFile> ();
    private void play_sound (string name)
    {
        if (!mute)
        {
            if (!sound_map.has_key (name))
            {
                sound_map[name] = MediaFile.for_filename (Path.build_filename (SOUND_DIRECTORY, name + ".ogg"));
                if (null != sound_map.@get (name))
                    sound_map.@get (name).set_volume (1);
            }
            if (null != sound_map.@get (name))
                sound_map.@get (name).play ();
        }
    }
}
