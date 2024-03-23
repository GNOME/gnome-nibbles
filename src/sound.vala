/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Gnome Nibbles: Gnome Worm Game
 * Copyright (C) 2023 Ben Corby <bcorby@new-ms.com>
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
 *
 */

using GSound;

/*\
* * Sound
\*/

internal class Sound : Object
{
    /* constructor */
    internal Sound (bool is_muted)
    {
        set_muted (is_muted);
    }

    /* signal */
    internal void connect_signal (NibblesGame game)
    {
        game.play_sound.connect (play_sound);
    }

    /* variables */
    bool is_muted;
    bool is_initilised = false;
    bool errored = false;
    Context sound_context;

    /* functions */
    internal void set_muted (bool muted)
    {
        is_muted = muted;
    }

    /* private functions */
    private bool initilise_context ()
    {
        try
        {
            sound_context = new Context ();
            is_initilised = true;
        }
        catch (GSound.Error e)
        {
            warning (e.message);
            errored = true;
        }
        catch (GLib.Error e)
        {
            warning (e.message);
            errored = true;
        }
        return is_initilised;
    }

    private void play_sound (string name)
    {
        if (!is_muted && !errored)
        {
            if (is_initilised || initilise_context ())
            {
                string filename = name + ".ogg";
                try
                {
                    sound_context.play_simple (null,
                        Attribute.MEDIA_NAME, filename,
                        Attribute.MEDIA_FILENAME,
                        Path.build_filename (SOUND_DIRECTORY, filename));
                }
                catch (GSound.Error e)
                {
                    warning (e.message);
                }
                catch (GLib.Error e)
                {
                    warning (e.message);
                }
            }
        }
    }
}

