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

    private const OptionEntry[] option_entries =
    {
        /* Translators: command-line option description, see 'gnome-nibbles --help' */
        { "version", 'v', OptionFlags.NONE, OptionArg.NONE, null, N_("Show release version"), null },
        {}
    };

    internal static int main (string[] args)
    {
        Intl.setlocale (LocaleCategory.ALL, "");
        Intl.bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
        Intl.bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        Intl.textdomain (GETTEXT_PACKAGE);

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

        /* Activate */
        return -1;
    }

    protected override void startup ()
    {
        base.startup ();

        unowned string[]? argv = null;
        GtkClutter.init (ref argv);

        Environment.set_prgname ("org.gnome.Nibbles");
        Environment.set_application_name (PROGRAM_NAME);

        Window.set_default_icon_name ("org.gnome.Nibbles");

        Gtk.Settings.get_default ().@set ("gtk-application-prefer-dark-theme", true);

        var css_provider = new CssProvider ();
        css_provider.load_from_resource ("/org/gnome/nibbles/ui/nibbles.css");
        Gdk.Display? gdk_display = Gdk.Display.get_default ();
        if (gdk_display != null) // else..?
            StyleContext.add_provider_for_display ((!) gdk_display, css_provider, STYLE_PROVIDER_PRIORITY_APPLICATION);

        add_action_entries (action_entries, this);

        set_accels_for_action ("win.new-game", {"<Primary>n"});
        set_accels_for_action ("app.quit", {"<Primary>q"});
        set_accels_for_action ("win.back", {"Escape"});
        set_accels_for_action ("app.help", {"F1"});

        window = new NibblesWindow ();
        add_window (window);
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
