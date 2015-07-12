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

public class Nibbles : Gtk.Application
{
    private Settings settings;
    private Gee.ArrayList<Settings> worm_settings;

    private bool is_maximized;
    private bool is_tiled;
    private int window_width;
    private int window_height;

    private Gtk.ApplicationWindow window;
    private Gtk.HeaderBar headerbar;
    private Gtk.Stack main_stack;
    private Games.GridFrame frame;
    private Gtk.Box statusbar;
    private Gtk.Label countdown;
    private Gee.LinkedList<Gtk.ToggleButton> number_of_players_buttons;
    private Gtk.Revealer next_button_revealer;

    private NibblesView? view;
    private NibblesGame? game = null;

    private const int COUNTDOWN_TIME = 5;

    private const ActionEntry action_entries[] =
    {
        {"start-game", start_game_cb},
        {"quit", quit}
    };

    private const ActionEntry menu_entries[] =
    {
        {"show-new-game-screen", show_new_game_screen_cb},
        {"show-controls-screen", show_controls_screen_cb}
    };

    private static const OptionEntry[] option_entries =
    {
        { "version", 'v', 0, OptionArg.NONE, null,
        /* Help string for command line --version flag */
        N_("Show release version"), null},

        { null }
    };

    public Nibbles ()
    {
        Object (application_id: "org.gnome.nibbles", flags: ApplicationFlags.FLAGS_NONE);
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

        var css_provider = new Gtk.CssProvider ();
        css_provider.load_from_resource ("/org/gnome/nibbles/ui/nibbles.css");
        Gtk.StyleContext.add_provider_for_screen (Gdk.Screen.get_default (), css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION);

        add_action_entries (action_entries, this);
        add_action_entries (menu_entries, this);

        settings = new Settings ("org.gnome.nibbles");
        worm_settings = new Gee.ArrayList<Settings> ();
        for (int i = 0; i < NibblesGame.NUMWORMS; i++)
        {
            var name = "org.gnome.nibbles.worm%d".printf(i);
            worm_settings.add (new Settings (name));
        }

        set_accels_for_action ("app.quit", {"<Primary>q"});

        var builder = new Gtk.Builder.from_resource ("/org/gnome/nibbles/ui/nibbles.ui");
        window = builder.get_object ("nibbles-window") as Gtk.ApplicationWindow;
        window.size_allocate.connect (size_allocate_cb);
        window.window_state_event.connect (window_state_event_cb);
        window.set_default_size (settings.get_int ("window-width"), settings.get_int ("window-height"));
        if (settings.get_boolean ("window-is-maximized"))
            window.maximize ();

        headerbar = (Gtk.HeaderBar) builder.get_object ("headerbar");
        main_stack = (Gtk.Stack) builder.get_object ("main_stack");
        statusbar = (Gtk.Box) builder.get_object ("statusbar");
        countdown = (Gtk.Label) builder.get_object ("countdown");
        number_of_players_buttons = new Gee.LinkedList<Gtk.ToggleButton> ();
        for (int i = 0; i < 2; i++)
        {
            var button = (Gtk.ToggleButton) builder.get_object ("players%d".printf (i + 1));
            button.toggled.connect (change_number_of_players_cb);
            number_of_players_buttons.add (button);
        }
        next_button_revealer = (Gtk.Revealer) builder.get_object ("next_button_revealer");

        window.set_titlebar (headerbar);

        add_window (window);

        /* Load game */
        game = new NibblesGame (settings);

        view = new NibblesView (game);
        view.show ();

        frame = new Games.GridFrame (NibblesGame.WIDTH, NibblesGame.HEIGHT);
        main_stack.add_named (frame, "frame");

        frame.add (view);
        frame.show ();
        // frame.show_all ();

        /* TODO Fix problem and remove this call
         * For some reason tile_size gets set to 0 after calling
         * frame.add (view). start_level stays the same
         */
        game.load_properties (settings);
        game.current_level = game.start_level;
        view.new_level (game.current_level);
        view.configure_event.connect (configure_event_cb);

        foreach (var worm in game.worms)
        {
            var actors = view.worm_actors.get (worm);
            if (actors.get_stage () == null) {
                view.stage.add_child (actors);
            }
            actors.show ();
        }
        game.load_worm_properties (worm_settings);

        /* Check wether to display the first run screen */
        var first_run = settings.get_boolean ("first-run");
        if (first_run)
            show_first_run_screen ();
        else
            show_new_game_screen_cb ();
    }

    protected override void activate ()
    {
        window.present ();
    }

    protected override void shutdown ()
    {
        settings.set_int ("window-width", window_width);
        settings.set_int ("window-height", window_height);
        settings.set_boolean ("window-is-maximized", is_maximized);
        game.save_properties (settings);

        base.shutdown ();
    }

    /*\
    * * Window events
    \*/

    private void size_allocate_cb (Gtk.Allocation allocation)
    {
        if (is_maximized || is_tiled)
            return;
        window_width = allocation.width;
        window_height = allocation.height;
    }

    private bool window_state_event_cb (Gdk.EventWindowState event)
    {
        if ((event.changed_mask & Gdk.WindowState.MAXIMIZED) != 0)
            is_maximized = (event.new_window_state & Gdk.WindowState.MAXIMIZED) != 0;
        /* We don’t save this state, but track it for saving size allocation */
        if ((event.changed_mask & Gdk.WindowState.TILED) != 0)
            is_tiled = (event.new_window_state & Gdk.WindowState.TILED) != 0;
        return false;
    }

    public bool configure_event_cb (Gdk.EventConfigure event)
    {
        int tile_size, ts_x, ts_y;

        /* Compute the new tile size based on the size of the
         * drawing area, rounded down.
         */
        ts_x = event.width / NibblesGame.WIDTH;
        ts_y = event.height / NibblesGame.HEIGHT;
        if (ts_x * NibblesGame.WIDTH > event.width)
            ts_x--;
        if (ts_y * NibblesGame.HEIGHT > event.height)
            ts_y--;
        tile_size = int.min (ts_x, ts_y);

        if (tile_size == 0 || game.tile_size == 0)
            return true;

        if (game.tile_size != tile_size)
        {
            view.stage.set_size (tile_size * NibblesGame.WIDTH, tile_size * NibblesGame.HEIGHT);

            view.board_rescale (tile_size);
            view.boni_rescale (tile_size);
            foreach (var worm in game.worms)
                worm.rescaled (tile_size);

            game.tile_size = tile_size;
        }

        return false;
    }

    private void start_game_cb ()
    {
        settings.set_boolean ("first-run", false);
        game.add_worms ();
        show_game_view ();

        var seconds = COUNTDOWN_TIME;
        Timeout.add (1000, () => {
            countdown.set_label ("%d".printf (seconds));
            if (seconds == 0)
            {
                countdown.hide ();
                countdown.set_label ("GO!");
                game.start ();
                return Source.REMOVE;
            }
            seconds--;
            return Source.CONTINUE;
        });
    }

    private void show_first_run_screen ()
    {
        main_stack.set_visible_child_name ("first_run");
    }

    private void show_new_game_screen_cb ()
    {
        main_stack.set_visible_child_name ("number_of_players");
    }

    private void show_controls_screen_cb ()
    {
        main_stack.set_visible_child_name ("controls");
    }

    private void show_game_view ()
    {
        main_stack.set_visible_child_name ("frame");
        statusbar.set_visible (true);
    }

    private void change_number_of_players_cb (Gtk.ToggleButton button)
    {
        if (!button.get_active () && button.get_style_context ().has_class ("suggested-action"))
        {
            button.set_active (true);
        }
        else if (button.get_active () && !button.get_style_context ().has_class ("suggested-action"))
        {
            next_button_revealer.set_reveal_child (true);
            button.get_style_context ().add_class ("suggested-action");
            foreach (var other_button in number_of_players_buttons)
            {
                if (button != other_button)
                {
                    if (other_button.get_active ())
                    {
                        other_button.get_style_context ().remove_class ("suggested-action");
                        other_button.set_active (false);
                        break;
                    }
                }
            }
        }
    }

    public static int main (string[] args)
    {
        var context = new OptionContext ("");

        context.add_group (Gtk.get_option_group (false));
        context.add_group (Clutter.get_option_group_without_init ());

        try
        {
            context.parse (ref args);
        }
        catch (Error e)
        {
            stderr.printf ("%s\n", e.message);
            return Posix.EXIT_FAILURE;
        }

        Environment.set_application_name (_("Nibbles"));

        try
        {
            GtkClutter.init_with_args (ref args, "", new OptionEntry[0], null);
        }
        catch (Error e)
        {
            var dialog = new Gtk.MessageDialog (null,
                                                Gtk.DialogFlags.MODAL,
                                                Gtk.MessageType.ERROR,
                                                Gtk.ButtonsType.NONE,
                                                "Unable to initialize Clutter:\n%s", e.message);
            dialog.set_title (Environment.get_application_name ());
            dialog.run ();
            dialog.destroy ();
            return Posix.EXIT_FAILURE;
        }

        return new Nibbles ().run (args);
    }
}
