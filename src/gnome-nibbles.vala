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

private class Nibbles : Gtk.Application
{
    /* Translators: name of the program, as seen in the headerbar, in GNOME Shell, or in the about dialog */
    internal const string PROGRAM_NAME = _("Nibbles");

    private NibblesWindow window;

    private const GLib.ActionEntry action_entries[] =
    {
        {"help", help_cb},
        {"about", about_cb},
        {"quit", quit}
    };

    private static bool disable_fakes   = false;
    private static bool enable_fakes    = false;
    private static bool start           = false;
    private static int level            = int.MIN;
    private static int nibbles          = int.MIN;
    private static int players          = int.MIN;
    private static int speed            = int.MIN;
    private static bool? sound          = null;
    private const OptionEntry[] option_entries =
    {
        /* Translators: command-line option description, see 'gnome-nibbles --help' */
        { "disable-fakes",  'd', OptionFlags.NONE, OptionArg.NONE,  null,           N_("Disable fake bonuses"),                 null },

        /* Translators: command-line option description, see 'gnome-nibbles --help' */
        { "enable-fakes",   'e', OptionFlags.NONE, OptionArg.NONE,  null,           N_("Enable fake bonuses"),                  null },

        /* Translators: command-line option description, see 'gnome-nibbles --help' */
        { "level",          'l', OptionFlags.NONE, OptionArg.INT,   ref level,      N_("Start at given level (1-26)"),

        /* Translators: in the command-line options description, text to indicate the user should specify the start level, see 'gnome-nibbles --help' */
                                                                                    N_("NUMBER") },

        /* Translators: command-line option description, see 'gnome-nibbles --help' */
        { "mute",           0,   OptionFlags.NONE, OptionArg.NONE,  null,           N_("Turn off the sound"),                   null },

        /* Translators: command-line option description, see 'gnome-nibbles --help' */
        { "nibbles",        'n', OptionFlags.NONE, OptionArg.INT,   ref nibbles,    N_("Set number of nibbles (4-6)"),

        /* Translators: in the command-line options description, text to indicate the user should specify number of nibbles, see 'gnome-nibbles --help' */
                                                                                    N_("NUMBER") },

        /* Translators: command-line option description, see 'gnome-nibbles --help' */
        { "players",        'p', OptionFlags.NONE, OptionArg.INT,   ref players,    N_("Set number of players (1-4)"),

        /* Translators: in the command-line options description, text to indicate the user should specify number of players, see 'gnome-nibbles --help' */
                                                                                    N_("NUMBER") },

        /* Translators: command-line option description, see 'gnome-nibbles --help' */
        { "speed",          's', OptionFlags.NONE, OptionArg.INT,   ref speed,      N_("Set worms speed (4-1, 4 for slow)"),

        /* Translators: in the command-line options description, text to indicate the user should specify the worms speed, see 'gnome-nibbles --help' */
                                                                                    N_("NUMBER") },

        /* Translators: command-line option description, see 'gnome-nibbles --help' */
        { "start",          0,   OptionFlags.NONE, OptionArg.NONE,  null,           N_("Start playing"),                        null },

        /* Translators: command-line option description, see 'gnome-nibbles --help' */
        { "unmute",         0,   OptionFlags.NONE, OptionArg.NONE,  null,           N_("Turn on the sound"),                    null },

        /* Translators: command-line option description, see 'gnome-nibbles --help' */
        { "version",        'v', OptionFlags.NONE, OptionArg.NONE,  null,           N_("Show release version"),                 null },
        {}
    };

    internal static int main (string[] args)
    {
        Intl.setlocale (LocaleCategory.ALL, "");
        Intl.bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
        Intl.bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        Intl.textdomain (GETTEXT_PACKAGE);

        Environment.set_prgname ("org.gnome.Nibbles");
        Environment.set_application_name (PROGRAM_NAME);
        Window.set_default_icon_name ("org.gnome.Nibbles");

        return new Nibbles ().run (args);
    }

    private inline Nibbles ()
    {
        Object (application_id: "org.gnome.Nibbles", flags: ApplicationFlags.FLAGS_NONE);

        add_main_option_entries (option_entries);
    }

    protected override int handle_local_options (VariantDict options)
    {
        if (options.contains ("version"))
        {
            /* Not translated so can be easily parsed */
            stderr.printf ("gnome-nibbles %s\n", VERSION);
            return Posix.EXIT_SUCCESS;
        }

        if (level   != int.MIN && (level   < 1 || level  > 26))
        {
            /* Translators: command-line error message, displayed for an invalid start level request; see 'gnome-nibbles -l 0' */
            stderr.printf (_("Start level should only be between 1 and 26.") + "\n");
            return Posix.EXIT_FAILURE;
        }
        if (nibbles != int.MIN && (nibbles < 4 || nibbles > 6))
        {
            /* Translators: command-line error message, displayed for an invalid number of nibbles; see 'gnome-nibbles -n 1' */
            stderr.printf (_("There could only be between 4 and 6 nibbles.") + "\n");
            return Posix.EXIT_FAILURE;
        }
        if (players != int.MIN && (players < 1 || players > 4))
        {
            /* Translators: command-line error message, displayed for an invalid number of players; see 'gnome-nibbles -p 5' */
            stderr.printf (_("There could only be between 1 and 4 players.") + "\n");
            return Posix.EXIT_FAILURE;
        }
        if (speed   != int.MIN && (speed   < 1 || speed   > 4))
        {
            /* Translators: command-line error message, displayed for an invalid given worms speed; see 'gnome-nibbles -s 5' */
            stderr.printf (_("Speed should be between 4 (slow) and 1 (fast).") + "\n");
            return Posix.EXIT_FAILURE;
        }

        disable_fakes = options.contains ("disable-fakes");
        enable_fakes  = options.contains ("enable-fakes");
        if (disable_fakes && enable_fakes)
        {
            /* Translators: command-line error message, displayed for an invalid combination of options; see 'gnome-nibbles -d -e' */
            stderr.printf (_("Options --disable-fakes (-d) and --enable-fakes (-e) are mutually exclusive.") + "\n");
            return Posix.EXIT_FAILURE;
        }

        if (options.contains ("mute"))
            sound = false;
        else if (options.contains ("unmute"))
            sound = true;

        if (options.contains ("start"))
            start = true;

        /* Activate */
        return -1;
    }

    protected override void startup ()
    {
        base.startup ();

        Gtk.Settings.get_default ().@set ("gtk-application-prefer-dark-theme", true);

        var css_provider = new CssProvider ();
        css_provider.load_from_resource ("/org/gnome/Nibbles/ui/nibbles.css");
        Gdk.Display? gdk_display = Gdk.Display.get_default ();
        if (gdk_display != null) // else..?
            StyleContext.add_provider_for_display ((!) gdk_display, css_provider, STYLE_PROVIDER_PRIORITY_APPLICATION);

        add_action_entries (action_entries, this);

        // F1 and friends are managed manually
        set_accels_for_action ("win.new-game",  { "<Primary>n"      });
        set_accels_for_action ("win.pause",     { "<Primary>p",
                                                           "Pause"  });
        set_accels_for_action ("app.quit",      { "<Primary>q"      });
        set_accels_for_action ("win.back",      {          "Escape" });
        set_accels_for_action ("win.hamburger", {          "F10",
                                                           "Menu"   });
        bool nibbles_changed = nibbles != int.MIN;
        bool players_changed = players != int.MIN;
        if (nibbles_changed
         || players_changed
         || speed != int.MIN
         || disable_fakes
         || enable_fakes
         || sound != null)
        {
            GLib.Settings settings = new GLib.Settings ("org.gnome.Nibbles");
            if (nibbles_changed && players_changed)
            {
                settings.set_int ("players", players);
                settings.set_int ("ai", nibbles - players);
            }
            else if (players_changed)
            {
                int old_ai      = settings.get_int ("ai");
                int old_players = settings.get_int ("players");
                settings.set_int ("players", players);
                int new_ai = ((old_ai + old_players).clamp (4, 6) - players).clamp (0, 5);
                if (old_ai != new_ai)
                    settings.set_int ("ai", new_ai);
            }
            else // (nibbles_changed)
                settings.set_int ("ai", nibbles - settings.get_int ("players"));

            if (speed != int.MIN)
                settings.set_int ("speed", speed);

            if (disable_fakes)
                settings.set_boolean ("fakes", false);
            else if (enable_fakes)
                settings.set_boolean ("fakes", true);

            if (sound != null)
                settings.set_boolean ("sound", (!) sound);
        }

        SetupScreen setup;
        if (start)
            setup = SetupScreen.GAME;
        else if (nibbles_changed && players_changed)
        {
            if (speed != int.MIN && (disable_fakes || enable_fakes))
                setup = SetupScreen.CONTROLS;
            else
                setup = SetupScreen.SPEED;
        }
        else
            setup = SetupScreen.USUAL;  // first-run or nibbles-number

        window = new NibblesWindow (level == int.MIN ? 0 : level, setup);
        add_window (window);
    }
    internal bool on_f1_pressed (Gdk.ModifierType state)
    {
        // TODO close popovers
        if ((state & Gdk.ModifierType.CONTROL_MASK) != 0)
            return false;                           // help overlay
        if ((state & Gdk.ModifierType.SHIFT_MASK) == 0)
        {
            help_cb ();
            return true;
        }
        about_cb ();
        return true;
    }

    protected override void activate ()
    {
        window.present ();
        base.activate ();
    }

    protected override void shutdown ()
    {
        window.on_shutdown ();
        base.shutdown ();
    }

    private inline void help_cb ()
    {
        show_uri (window, "help:gnome-nibbles", Gdk.CURRENT_TIME);
    }

    private inline void about_cb ()
    {
        string [] authors = {
        /* Translators: text crediting an author, in the about dialog */
            _("Sean MacIsaac"),


        /* Translators: text crediting an author, in the about dialog */
            _("Ian Peters"),


        /* Translators: text crediting an author, in the about dialog */
            _("Andreas Røsdal"),


        /* Translators: text crediting an author, in the about dialog */
            _("Guillaume Beland"),


        /* Translators: text crediting an author, in the about dialog */
            _("Iulian-Gabriel Radu")
        };

        /* Translators: text crediting a documenter, in the about dialog */
        string [] documenters = { _("Kevin Breit") };


        /* Translators: text crediting a designer, in the about dialog */
        string [] artists = { _("Allan Day") };

        show_about_dialog (window,
                           "program-name", PROGRAM_NAME,
                           "version", VERSION,
                           /* Translators: small description of the game, seen in the About dialog */
                           "comments", _("A worm game for GNOME"),
                           "logo-icon-name", "org.gnome.Nibbles",
                           "copyright",
                             /* Translators: text crediting some maintainers, seen in the About dialog */
                             _("Copyright © 1999-2008 – Sean MacIsaac, Ian Peters, Andreas Røsdal") + "\n" +


                             /* Translators: text crediting a maintainer, seen in the About dialog */
                             _("Copyright © 2009 – Guillaume Beland") + "\n" +


                             /* Translators: text crediting a maintainer, seen in the About dialog; the %u is replaced with the years of start and end */
                             _("Copyright © %u-%u – Iulian-Gabriel Radu").printf (2015, 2020),
                           "license-type", License.GPL_3_0, // means "GNU General Public License, version 3.0 or later"
                           "authors", authors,
                           "documenters", documenters,
                           "artists", artists,
                           /* Translators: about dialog text; this string should be replaced by a text crediting yourselves and your translation team, or should be left empty. Do not translate literally! */
                           "translator-credits", _("translator-credits"),
                           "website", "https://wiki.gnome.org/Apps/Nibbles/"
                           );
    }
}
