/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Gnome Nibbles: Gnome Worm Game
 * Copyright (C) 2015 Iulian-Gabriel Radu <iulian.radu67@gmail.com>
 * Copyright (C) 2023-2025 Ben Corby <bcorby@new-ms.com>
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

/* designed for Gtk 4, link with libgtk-4-dev or gtk4-devel */
using Gtk;

/*
 * Remove deprecate warning.
 *
 * The use of show_uri is deprecated since GTK 4.10. The suggested alternative
 * is to use UriLauncher, however for help UriLauncher has a bug that results
 * in a "Document Not Found" message when used in flatpack.
 * There is no intention to fix this bug so we will continue to use show_uri.
 * This stops the deprecate warning and hopefully stops anyone mistakenly
 * coding a switch to UriLauncher.
 *
 */
namespace no_deprecate_warning
{
    [CCode (cheader_filename = "gtk/gtk.h", cname = "gtk_show_uri")]
    extern static void show_uri (Gtk.Window? parent, string uri, uint32 timestamp);
}

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

    private static bool disable_fakes           = false;
    private static bool enable_fakes            = false;
    private static bool start                   = false;
    private static int level                    = int.MIN;
    private static int progress                 = int.MIN;
    private static int nibbles                  = int.MIN;
    private static int players                  = int.MIN;
    private static int speed                    = int.MIN;
    private static bool? three_dimensional_view = null;
    private static bool? sound                  = null;
    private const OptionEntry[] option_entries  =
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
        { "progress",       0, OptionFlags.NONE, OptionArg.INT,     ref progress,   N_("Method of progress through the boards (0 - sequentially, 1 - randomly, 2 - fixed board)"),

        /* Translators: in the command-line options description, text to indicate the user should specify the progres method, see 'gnome-nibbles --help' */
                                                                                    N_("NUMBER") },

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
        { "3D",             '3', OptionFlags.NONE, OptionArg.NONE,  null,           N_("Set three dimensional view"),           null },

        /* Translators: command-line option description, see 'gnome-nibbles --help' */
        { "2D",             '2', OptionFlags.NONE, OptionArg.NONE,  null,           N_("Set two dimensional view"),             null },

        /* Translators: command-line option description, see 'gnome-nibbles --help' */
        { "start",          0,   OptionFlags.NONE, OptionArg.NONE,  null,           N_("Start playing"),                        null },

        /* Translators: command-line option description, see 'gnome-nibbles --help' */
        { "mute",           0,   OptionFlags.NONE, OptionArg.NONE,  null,           N_("Turn off the sound"),                   null },

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

        #if USE_LIBADWAITA
        Adw.init ();
        #else
        Gtk.init ();
        #endif

        var application = new Nibbles ();
        return application.run (args);
    }

    private Nibbles ()
    {
        Object (application_id: "org.gnome.Nibbles", flags: ApplicationFlags.DEFAULT_FLAGS);
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
        if (progress != int.MIN && (progress < 0 || progress > 2))
        {
            /* Translators: command-line error message, displayed for an invalid progress method request; see 'gnome-nibbles --progress' */
            stderr.printf (_("Progress method should be; 0 - sequentially, 1 - randomly, 2 - fixed board.") + "\n");
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

        if (options.contains ("3D") && options.contains ("2D"))
        {
            /* Translators: command-line error message, displayed for an invalid combination of options; see 'gnome-nibbles -3 -2' */
            stderr.printf (_("Options --3D (-3) and --2D (-2) are mutually exclusive.") + "\n");
            return Posix.EXIT_FAILURE;
        }
        else if (options.contains ("3D"))
            three_dimensional_view = true;
        else if (options.contains ("2D"))
            three_dimensional_view = false;

        if (options.contains ("start"))
            start = true;

        /* Activate */
        return -1;
    }

    protected override void startup ()
    {
        base.startup ();
        Environment.set_prgname ("org.gnome.Nibbles");
        Environment.set_application_name (PROGRAM_NAME);
        Window.set_default_icon_name ("org.gnome.Nibbles");

        #if USE_LIBADWAITA
        Adw.StyleManager.get_default ().set_color_scheme (Adw.ColorScheme.FORCE_DARK);
        #else
        Gtk.Settings.get_default ().@set ("gtk-application-prefer-dark-theme", true);
        #endif

        add_action_entries (action_entries, this);

        /*
         * When a const string [] is passed to a C function it is passed as a
         * pointer to a list of pointers without a null terminator at the end
         * of the list. Therefore we must manually put a null at the end of the
         * list if the C function is expecting it (which is usually the case
         * for a list of pointers of unknown length).
         */
        const string [] newgame = {"<Primary>n", null};
        const string [] fullscreen = {"F11", null};
        const string [] pause = {"<Primary>p", "Pause", null};
        const string [] appquit = {"<Primary>q", null};
        const string [] back = {"Escape", null};
        const string [] hamburger = {"F10", "Menu", null};
        // F1 and friends are managed manually
        set_accels_for_action ("win.new-game",  newgame);
        set_accels_for_action ("win.fullscreen",fullscreen);
        set_accels_for_action ("win.pause",     pause);
        set_accels_for_action ("app.quit",      appquit);
        set_accels_for_action ("win.back",      back);
        set_accels_for_action ("win.hamburger", hamburger);
    }

    private void create_window () {
        bool nibbles_changed = nibbles != int.MIN;
        bool players_changed = players != int.MIN;
        if (nibbles_changed
         || players_changed
         || progress != int.MIN
         || speed != int.MIN
         || disable_fakes
         || enable_fakes
         || three_dimensional_view != null
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
            else if (nibbles_changed)
                settings.set_int ("ai", nibbles - settings.get_int ("players"));

            if (progress != int.MIN && progress >= 0 && progress <= 2)
                settings.set_int ("progress", progress);

            if (speed != int.MIN)
                settings.set_int ("speed", speed);

            if (disable_fakes)
                settings.set_boolean ("fakes", false);
            else if (enable_fakes)
                settings.set_boolean ("fakes", true);

            if (three_dimensional_view != null)
                settings.set_boolean ("three-dimensional-view", (!) three_dimensional_view);

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

    protected override void activate ()
    {
        if (window == null)
            create_window ();

        window.present ();
        base.activate ();
    }

    protected override void shutdown ()
    {
        if (window != null)
            window.on_shutdown ();

        base.shutdown ();
    }

    private inline void help_cb ()
    {
        if (window != null && !window.game_paused)
            window.activate_action ("pause", null);
#if CAN_USE_SHOW_URI
        /* See "Remove deprecate warning." comment at the top of this file. */
        no_deprecate_warning.show_uri (window, "help:gnome-nibbles", Gdk.CURRENT_TIME);
#else /* !CAN_USE_SHOW_URI */
        launch_help.begin ((obj,res)=>
        {
            launch_help.end (res);
        });
#endif
    }

#if !CAN_USE_SHOW_URI
    async void launch_help ()
    {
        /* UriLuncher has a bug see https://gitlab.gnome.org/GNOME/gtk/-/issues/6135 */
        var help = new UriLauncher ("help:gnome-nibbles");
        try
        {
            yield help.launch (window, null);
        }
        catch (Error e)
        {
            warning ("Unable to open help: %s", e.message);
        }
    }
#endif

    private inline void about_cb ()
    {
        if (window != null && !window.game_paused)
            window.activate_action ("pause", null);

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
            _("Iulian-Gabriel Radu"),


        /* Translators: text crediting an author, in the about dialog */
            _("Ben Corby")
        };

        /* Translators: text crediting a documenter, in the about dialog */
        string [] documenters = { _("Kevin Breit") };


        /* Translators: text crediting a designer, in the about dialog */
        string [] artists = { _("Allan Day") };

        #if USE_LIBADWAITA
        var about_dialog = new Adw.AboutDialog ();

        about_dialog.set_application_icon ("org.gnome.Nibbles");
        about_dialog.set_application_name (PROGRAM_NAME);
        /* Translators: the name of the app's developers, seen in the About dialog */
        about_dialog.set_developer_name (_("The GNOME Project"));
        about_dialog.set_version (VERSION + " (adwaita)");
        about_dialog.set_developers (authors); /* compile without LIBADWAITA if this line causes a problem */
        about_dialog.set_documenters (documenters); /* compile without LIBADWAITA if this line causes a problem */
        about_dialog.set_artists (artists); /* compile without LIBADWAITA if this line causes a problem */
        about_dialog.set_copyright (
        #else
        show_about_dialog (window,
                           "program-name", PROGRAM_NAME,
                           "version", VERSION,
                           /* Translators: small description of the game, seen in the About dialog */
                           "comments", _("A worm game for GNOME"),
                           "logo-icon-name", "org.gnome.Nibbles",
                           "copyright",
        #endif
            /* Translators: text crediting some maintainers, seen in the About dialog */
             _("Copyright © 1999-2008 – Sean MacIsaac, Ian Peters, Andreas Røsdal") + "\n" +


             /* Translators: text crediting a maintainer, seen in the About dialog */
             _("Copyright © 2009 – Guillaume Beland") + "\n" +


             /* Translators: text crediting a maintainer, seen in the About dialog; the %u is replaced with the years of start and end */
             _("Copyright © %u-%u – Iulian-Gabriel Radu").printf (2015, 2020) + "\n" +


             /* Translators: text crediting a maintainer, seen in the About dialog; the %u is replaced with the years of start and end */
             _("Copyright © %u-%u – Ben Corby").printf (2022, 2025)
        #if USE_LIBADWAITA
        /**/);
        about_dialog.set_license_type (License.GPL_3_0);
        about_dialog.set_translator_credits (_("translator-credits"));
        about_dialog.set_website (WEBSITE);
        about_dialog.present (this.get_active_window ());
        #else
            ,
            "license-type", License.GPL_3_0, // means "GNU General Public License, version 3.0 or later"
            "authors", authors,
            "documenters", documenters,
            "artists", artists,
            /* Translators: about dialog text; this string should be replaced by a text crediting yourselves and your translation team, or should be left empty. Do not translate literally! */
            "translator-credits", _("translator-credits"),
            "website", WEBSITE);
        #endif
    }
}
