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
    /* Application and worm settings */
    private Settings settings;
    private Gee.ArrayList<Settings> worm_settings;

    /* Main window */
    private Gtk.ApplicationWindow window;
    private bool is_maximized;
    private bool is_tiled;
    private int window_width;
    private int window_height;

    private Gtk.Stack main_stack;
    private Gtk.Overlay overlay;

    /* HeaderBar */
    private Gtk.HeaderBar headerbar;
    private Gtk.Button new_game_button;
    private Gtk.Button pause_button;

    /* Pre-game screen widgets */
    private Gee.LinkedList<Gtk.ToggleButton> number_of_players_buttons;
    private Gee.LinkedList<Gtk.ToggleButton> number_of_ai_buttons;

    private Gtk.Box grids_box;
    private Gdk.Pixbuf arrow_pixbuf;
    private Gdk.Pixbuf arrow_key_pixbuf;

    /* Statusbar widgets */
    private Gtk.Stack statusbar_stack;
    private Gtk.Label countdown;
    private Scoreboard scoreboard;
    private Gdk.Pixbuf scoreboard_life;

    /* Preferences dialog */
    private PreferencesDialog preferences_dialog = null;

    /* Rendering of the game */
    private NibblesView? view;

    private Gtk.Box game_box;
    private Games.GridFrame frame;

    /* Game being played */
    private NibblesGame? game = null;

    /* Used for handling the game's scores */
    private Games.Scores.Context scores_context;
    private Gee.LinkedList<Games.Scores.Category> scorecats;

    /* HeaderBar actions */
    private SimpleAction new_game_action;
    private SimpleAction pause_action;
    private SimpleAction back_action;

    private uint countdown_id = 0;
    private const int COUNTDOWN_TIME = 3;
    private int seconds = 0;

    private const ActionEntry action_entries[] =
    {
        {"start-game", start_game_cb},
        {"new-game", new_game_cb},
        {"pause", pause_cb},
        {"preferences", preferences_cb},
        {"scores", scores_cb},
        {"help", help_cb},
        {"about", about_cb},
        {"quit", quit}
    };

    private const ActionEntry menu_entries[] =
    {
        {"show-new-game-screen", show_new_game_screen_cb},
        {"show-controls-screen", show_controls_screen_cb},
        {"back", back_cb}
    };

    private const OptionEntry[] option_entries =
    {
        { "version", 'v', 0, OptionArg.NONE, null,
        /* Help string for command line --version flag */
        N_("Show release version"), null },

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

        unowned string[]? argv = null;
        GtkClutter.init (ref argv);

        Environment.set_prgname ("org.gnome.Nibbles");
        Environment.set_application_name (_("Nibbles"));

        Gtk.Window.set_default_icon_name ("gnome-nibbles");

        Gtk.Settings.get_default ().set ("gtk-application-prefer-dark-theme", true);

        var css_provider = new Gtk.CssProvider ();
        css_provider.load_from_resource ("/org/gnome/nibbles/ui/nibbles.css");
        Gtk.StyleContext.add_provider_for_screen (Gdk.Screen.get_default (), css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION);

        add_action_entries (action_entries, this);
        add_action_entries (menu_entries, this);

        settings = new Settings ("org.gnome.nibbles");
        settings.changed.connect (settings_changed_cb);

        worm_settings = new Gee.ArrayList<Settings> ();
        for (int i = 0; i < NibblesGame.MAX_WORMS; i++)
        {
            var name = "org.gnome.nibbles.worm%d".printf(i);
            worm_settings.add (new Settings (name));
            worm_settings[i].changed.connect (worm_settings_changed_cb);
        }

        set_accels_for_action ("app.quit", {"<Primary>q"});
        set_accels_for_action ("app.back", {"Escape"});
        set_accels_for_action ("app.help", {"F1"});
        new_game_action = (SimpleAction) lookup_action ("new-game");
        pause_action = (SimpleAction) lookup_action ("pause");
        back_action = (SimpleAction) lookup_action ("back");

        var builder = new Gtk.Builder.from_resource ("/org/gnome/nibbles/ui/nibbles.ui");
        window = builder.get_object ("nibbles-window") as Gtk.ApplicationWindow;
        window.size_allocate.connect (size_allocate_cb);
        window.window_state_event.connect (window_state_event_cb);
        window.key_press_event.connect (key_press_event_cb);
        window.set_default_size (settings.get_int ("window-width"), settings.get_int ("window-height"));
        if (settings.get_boolean ("window-is-maximized"))
            window.maximize ();

        headerbar = (Gtk.HeaderBar) builder.get_object ("headerbar");
        overlay = (Gtk.Overlay) builder.get_object ("main_overlay");
        new_game_button = (Gtk.Button) builder.get_object ("new_game_button");
        pause_button = (Gtk.Button) builder.get_object ("pause_button");
        main_stack = (Gtk.Stack) builder.get_object ("main_stack");
        game_box = (Gtk.Box) builder.get_object ("game_box");
        statusbar_stack = (Gtk.Stack) builder.get_object ("statusbar_stack");
        countdown = (Gtk.Label) builder.get_object ("countdown");
        number_of_players_buttons = new Gee.LinkedList<Gtk.ToggleButton> ();
        for (int i = 0; i < NibblesGame.MAX_HUMANS; i++)
        {
            var button = (Gtk.ToggleButton) builder.get_object ("players%d".printf (i + 1));
            button.toggled.connect (change_number_of_players_cb);
            number_of_players_buttons.add (button);
        }
        number_of_ai_buttons = new Gee.LinkedList<Gtk.ToggleButton> ();
        for (int i = 0; i <= NibblesGame.MAX_AI; i++)
        {
            var button = (Gtk.ToggleButton) builder.get_object ("ai%d".printf (i));
            button.toggled.connect (change_number_of_ai_cb);
            number_of_ai_buttons.add (button);
        }
        grids_box = (Gtk.Box) builder.get_object ("grids_box");
        window.set_titlebar (headerbar);

        add_window (window);

        /* Create game */
        game = new NibblesGame (settings);
        game.log_score.connect (log_score_cb);
        game.level_completed.connect (level_completed_cb);
        game.notify["is-paused"].connect (() => {
            if (game.is_paused)
                statusbar_stack.set_visible_child_name ("paused");
            else
                statusbar_stack.set_visible_child_name ("scoreboard");
        });

        /* Create view */
        view = new NibblesView (game);
        view.configure_event.connect (configure_event_cb);
        view.is_muted = !settings.get_boolean ("sound");
        view.show ();

        frame = new Games.GridFrame (NibblesGame.WIDTH, NibblesGame.HEIGHT);
        game_box.pack_start (frame);

        /* Create scoreboard */
        scoreboard = new Scoreboard ();
        scoreboard_life = view.load_pixmap_file ("scoreboard-life.svg", 2 * game.tile_size, 2 * game.tile_size);
        scoreboard.show ();
        statusbar_stack.add_named (scoreboard, "scoreboard");

        frame.add (view);
        frame.show ();

        /* Number of worms */
        game.numhumans = settings.get_int ("players");
        game.numai = settings.get_int ("ai");

        /* Controls screen */
        arrow_pixbuf = view.load_pixmap_file ("arrow.svg", 5 * game.tile_size, 5 * game.tile_size);
        arrow_key_pixbuf = view.load_pixmap_file ("arrow-key.svg", 5 * game.tile_size, 5 * game.tile_size);

        /* Check whether to display the first run screen */
        var first_run = settings.get_boolean ("first-run");
        if (first_run)
            show_first_run_screen ();
        else
            show_new_game_screen_cb ();

        /* Create scores */
        create_scores ();

        window.show ();
    }

    protected override void activate ()
    {
        base.activate ();

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

    private bool countdown_cb ()
    {
        seconds--;

        if (seconds == 0)
        {
            statusbar_stack.set_visible_child_name ("scoreboard");
            view.name_labels.hide ();

            game.add_bonus (true);
            game.start ();

            pause_action.set_enabled (true);
            back_action.set_enabled (true);

            countdown_id = 0;
            return Source.REMOVE;
        }

        countdown.set_label (seconds.to_string ());
        return Source.CONTINUE;
    }

    /*\
    * * Window events
    \*/

    /* The reason this event handler is found here (and not in nibbles-view.vala
     * which would be a more suitable place) is to avoid a weird behavior of having
     * your first key press ignored everytime by the start of a new level, thus
     * making your worm unresponsive to your command.
     */
    private bool key_press_event_cb (Gtk.Widget widget, Gdk.EventKey event)
    {
        return game.handle_keypress (event.keyval);
    }

    private void size_allocate_cb (Gtk.Allocation allocation)
    {
        if (is_maximized || is_tiled)
            return;
        window.get_size (out window_width, out window_height);
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

    private bool configure_event_cb (Gdk.EventConfigure event)
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
            view.get_stage ().set_size (tile_size * NibblesGame.WIDTH, tile_size * NibblesGame.HEIGHT);

            view.board_rescale (tile_size);
            view.boni_rescale (tile_size);
            view.warps_rescale (tile_size);
            foreach (var worm in game.worms)
                worm.rescaled (tile_size);

            game.tile_size = tile_size;
        }

        return false;
    }

    private void start_game_cb ()
    {
        settings.set_boolean ("first-run", false);

        game.reset ();

        view.new_level (game.current_level);
        view.connect_worm_signals ();

        scoreboard.clear ();
        foreach (var worm in game.worms)
        {
            var color = game.worm_props.get (worm).color;
            scoreboard.register (worm, NibblesView.colorval_name (color), scoreboard_life);
            worm.notify["lives"].connect (scoreboard.update);
            worm.notify["score"].connect (scoreboard.update);
        }
        game.add_worms ();

        view.create_name_labels ();

        show_game_view ();

        start_game_with_countdown ();
    }

    private void start_game_with_countdown ()
    {
        statusbar_stack.set_visible_child_name ("countdown");

        new_game_action.set_enabled (true);

        seconds = COUNTDOWN_TIME;
        view.name_labels.show ();

        countdown.set_label (COUNTDOWN_TIME.to_string ());
        countdown_id = Timeout.add_seconds (1, countdown_cb);
    }

    private void restart_game ()
    {
        view.new_level (game.current_level);

        game.add_worms ();
        start_game_with_countdown ();
    }

    private void new_game_cb ()
    {
        if (countdown_id != 0)
        {
            Source.remove (countdown_id);
            countdown_id = 0;
        }

        if (game.is_running)
            game.stop ();

        var dialog = new Gtk.MessageDialog (window,
                                            Gtk.DialogFlags.MODAL,
                                            Gtk.MessageType.WARNING,
                                            Gtk.ButtonsType.OK_CANCEL,
                                            _("Are you sure you want to start a new game?"));
        dialog.secondary_text = _("If you start a new game, the current one will be lost.");

        var button = (Gtk.Button) dialog.get_widget_for_response (Gtk.ResponseType.OK);
        button.set_label (_("_New Game"));
        dialog.response.connect ((response_id) => {
            if (response_id == Gtk.ResponseType.OK)
                show_new_game_screen_cb ();
            if ((response_id == Gtk.ResponseType.CANCEL || response_id == Gtk.ResponseType.DELETE_EVENT)
                && !game.is_paused)
            {
                if (seconds == 0)
                    game.start ();
                else
                    countdown_id = Timeout.add_seconds (1, countdown_cb);

                view.grab_focus ();
            }

            dialog.destroy ();
        });

        dialog.show ();
    }

    private void pause_cb ()
    {
        if (game != null)
        {
            if (game.is_running)
            {
                game.pause ();
                pause_button.set_label (_("_Resume"));
            }
            else
            {
                game.unpause ();
                pause_button.set_label (_("_Pause"));
                view.grab_focus ();
            }
        }
    }

    /*\
    * * Settings changed events
    \*/

    private void settings_changed_cb (string key)
    {
        switch (key)
        {
            case "speed":
                game.speed = settings.get_int (key);
                break;
            case "sound":
                view.is_muted = !settings.get_boolean (key);
                break;
            case "fakes":
                game.fakes = settings.get_boolean (key);
                break;
        }
    }

    private void worm_settings_changed_cb (Settings changed_worm_settings, string key)
    {
        /* Empty worm properties means game has not started yet */
        if (game.worm_props.size == 0)
            return;

        var id = worm_settings.index_of (changed_worm_settings);

        if (id >= game.numworms)
            return;

        var worm = game.worms[id];
        var properties = game.worm_props.get (worm);

        switch (key)
        {
            case "color":
                properties.color = changed_worm_settings.get_enum ("color");
                break;
            case "key-up":
                properties.up = changed_worm_settings.get_int ("key-up");
                break;
            case "key-down":
                properties.down = changed_worm_settings.get_int ("key-down");
                break;
            case "key-left":
                properties.left = changed_worm_settings.get_int ("key-left");
                break;
            case "key-right":
                properties.right = changed_worm_settings.get_int ("key-right");
                break;
        }

        game.worm_props.set (worm, properties);
    }

    /*\
    * * Switching the stack
    \*/

    private void show_first_run_screen ()
    {
        main_stack.set_visible_child_name ("first_run");
    }

    private void show_new_game_screen_cb ()
    {
        if (countdown_id != 0)
        {
            Source.remove (countdown_id);
            countdown_id = 0;
        }

        if (game.is_running)
            game.stop ();

        headerbar.set_title (_("Nibbles"));

        new_game_action.set_enabled (false);
        pause_action.set_enabled (false);
        back_action.set_enabled (true);

        new_game_button.hide ();
        pause_button.hide ();

        var type = main_stack.get_transition_type ();
        main_stack.set_transition_type (Gtk.StackTransitionType.NONE);
        main_stack.set_visible_child_name ("number_of_players");
        main_stack.set_transition_type (type);

        number_of_players_buttons[game.numhumans - 1].set_active (true);
        number_of_ai_buttons[game.numai].set_active (true);
    }

    private void show_controls_screen_cb ()
    {
        /* Save selected number of players before changing the screen */
        foreach (var button in number_of_players_buttons)
        {
            if (button.get_active ())
            {
                int numhumans = -1;
                button.get_label ().scanf ("_%d", &numhumans);
                game.numhumans = numhumans;

                /* Remember the option for the following runs */
                settings.set_int ("players", game.numhumans);
                break;
            }
        }

        /* Save selected number of computer players before changing the screen */
        foreach (var button in number_of_ai_buttons)
        {
            if (button.get_active ())
            {
                int numai = -1;
                button.get_label ().scanf ("_%d", &numai);
                game.numai = numai;

                /* Remember the option for the following runs */
                settings.set_int ("ai", game.numai);
                break;
            }
        }

        /* Create worms and load properties */
        game.create_worms ();
        game.load_worm_properties (worm_settings);

        foreach (var grid in grids_box.get_children ())
            grid.destroy ();

        foreach (var worm in game.worms)
        {
            if (worm.is_human)
            {
                var grid = new ControlsGrid (worm.id, game.worm_props.get (worm), arrow_pixbuf, arrow_key_pixbuf);
                grids_box.add (grid);
            }
        }

        main_stack.set_visible_child_name ("controls");
    }

    private void show_game_view ()
    {
        new_game_button.show ();
        pause_button.show ();

        back_action.set_enabled (false);

        headerbar.set_title (_("Level %d").printf (game.current_level));
        main_stack.set_visible_child_name ("game_box");
    }

    private void back_cb ()
    {
        main_stack.set_transition_type (Gtk.StackTransitionType.SLIDE_DOWN);

        var child_name = main_stack.get_visible_child_name ();
        switch (child_name)
        {
            case "first_run":
                break;
            case "number_of_players":
                break;
            case "controls":
                main_stack.set_visible_child_name ("number_of_players");
                break;
            case "game_box":
                new_game_cb ();
                break;
        }

        main_stack.set_transition_type (Gtk.StackTransitionType.SLIDE_UP);
    }

    private void change_number_of_players_cb (Gtk.ToggleButton button)
    {
        foreach (var other_button in number_of_players_buttons)
        {
            if (button != other_button)
            {
                if (other_button.get_active ())
                {
                    /* We are blocking the signal to prevent another callback when setting the previous
                     * checked button to inactive
                     */
                    SignalHandler.block_matched (other_button, SignalMatchType.DATA, 0, 0, null, null, this);
                    other_button.set_active (false);
                    SignalHandler.unblock_matched (other_button, SignalMatchType.DATA, 0, 0, null, null, this);
                    break;
                }
            }
        }
        button.set_active (true);

        int numhumans = -1;
        button.get_label ().scanf ("_%d", &numhumans);

        int min_ai = 4 - numhumans;
        int max_ai = NibblesGame.MAX_WORMS - numhumans;
        for (int i = 0; i < min_ai; i++)
        {
            number_of_ai_buttons[i].hide ();
        }
        for (int i = min_ai; i <= max_ai; i++)
        {
            number_of_ai_buttons[i].show ();
        }
        for (int i = max_ai + 1; i < number_of_ai_buttons.size; i++)
        {
            number_of_ai_buttons[i].hide ();
        }

        if (numhumans == 4)
        {
            number_of_ai_buttons[0].show ();
        }

        number_of_ai_buttons[min_ai].set_active (true);
    }

    private void change_number_of_ai_cb (Gtk.ToggleButton button)
    {
        foreach (var other_button in number_of_ai_buttons)
        {
            if (button != other_button)
            {
                if (other_button.get_active ())
                {
                    /* We are blocking the signal to prevent another callback when setting the previous
                     * checked button to inactive
                     */
                    SignalHandler.block_matched (other_button, SignalMatchType.DATA, 0, 0, null, null, this);
                    other_button.set_active (false);
                    SignalHandler.unblock_matched (other_button, SignalMatchType.DATA, 0, 0, null, null, this);
                    break;
                }
            }
        }
        button.set_active (true);
    }

    /*\
    * * Scoring
    \*/

    private Games.Scores.Category? category_request (string key)
    {
        foreach (var cat in scorecats)
        {
            if (key == cat.key)
                return cat;
        }
        return null;
    }

    private string? get_new_scores_key (string old_key)
    {
        switch (old_key)
        {
            case "1.0":
                return "fast";
            case "2.0":
                return "medium";
            case "3.0":
                return "slow";
            case "4.0":
                return "beginner";
            case "1.1":
                return "fast-fakes";
            case "2.1":
                return "medium-fakes";
            case "3.1":
                return "slow-fakes";
            case "4.1":
                return "beginner-fakes";
        }
        return null;
    }

    private void create_scores ()
    {
        scorecats = new Gee.LinkedList<Games.Scores.Category> ();
        /* Translators: Difficulty level displayed on the scores dialog */
        scorecats.add (new Games.Scores.Category ("beginner", _("Beginner")));
        /* Translators: Difficulty level displayed on the scores dialog */
        scorecats.add (new Games.Scores.Category ("slow", _("Slow")));
        /* Translators: Difficulty level displayed on the scores dialog */
        scorecats.add (new Games.Scores.Category ("medium", _("Medium")));
        /* Translators: Difficulty level displayed on the scores dialog */
        scorecats.add (new Games.Scores.Category ("fast", _("Fast")));
        /* Translators: Difficulty level with fake bonuses, displayed on the scores dialog */
        scorecats.add (new Games.Scores.Category ("beginner-fakes", _("Beginner with Fakes")));
        /* Translators: Difficulty level with fake bonuses, displayed on the scores dialog */
        scorecats.add (new Games.Scores.Category ("slow-fakes", _("Slow with Fakes")));
        /* Translators: Difficulty level with fake bonuses, displayed on the scores dialog */
        scorecats.add (new Games.Scores.Category ("medium-fakes", _("Medium with Fakes")));
        /* Translators: Difficulty level with fake bonuses, displayed on the scores dialog */
        scorecats.add (new Games.Scores.Category ("fast-fakes", _("Fast with Fakes")));

        scores_context = new Games.Scores.Context.with_importer (
            "gnome-nibbles",
            /* Displayed on the scores dialog, preceeding a difficulty. */
            _("Difficulty Level:"),
            window,
            category_request,
            Games.Scores.Style.POINTS_GREATER_IS_BETTER,
            new Games.Scores.DirectoryImporter.with_convert_func (get_new_scores_key));
    }

    private Games.Scores.Category get_scores_category (int speed, bool fakes)
    {
        string key = null;
        switch (speed)
        {
            case 1:
                key = "fast";
                break;
            case 2:
                key = "medium";
                break;
            case 3:
                key = "slow";
                break;
            case 4:
                key = "beginner";
                break;
        }

        if (fakes)
            key = key + "-fakes";

        foreach (var cat in scorecats)
        {
            if (key == cat.key)
                return cat;
        }

        return scorecats.first ();
    }

    private void log_score_cb (int score, int level_reached)
    {
        /* Disable these here to prevent the user clicking the buttons before the score is saved */
        new_game_action.set_enabled (false);
        pause_action.set_enabled (false);

        var scores = scores_context.get_high_scores (get_scores_category (game.speed, game.fakes));
        var lowest_high_score = (scores.size == 10 ? scores.last ().score : -1);

        if (game.numhumans != 1)
        {
            game_over (score, lowest_high_score, level_reached);
            return;
        }

        if (game.start_level != 1)
        {
            game_over (score, lowest_high_score, level_reached);
            return;
        }

        scores_context.add_score.begin (score,
                                        get_scores_category (game.speed, game.fakes),
                                        null,
                                        (object, result) => {
            try
            {
                scores_context.add_score.end (result);
            }
            catch (GLib.Error e)
            {
                warning ("Failed to add score: %s", e.message);
            }

            game_over (score, lowest_high_score, level_reached);
        });
    }

    private void scores_cb ()
    {
        var should_unpause = false;
        if (game.is_running)
        {
            pause_action.activate (null);
            should_unpause = true;
        }

        scores_context.run_dialog ();

        // Be quite careful about whether to unpause. Don't unpause if the game has not started.
        if (should_unpause)
            pause_action.activate (null);
    }

    private void level_completed_cb ()
    {
        if (game.current_level == NibblesGame.MAX_LEVEL)
            return;

        new_game_action.set_enabled (false);
        pause_action.set_enabled (false);

        // Translators: the %d is the number of the level that was completed.
        var label = new Gtk.Label (_("Level %d Completed!").printf (game.current_level));
        label.halign = Gtk.Align.CENTER;
        label.valign = Gtk.Align.START;
        label.set_margin_top (150);
        label.get_style_context ().add_class ("menu-title");
        label.show ();

        var button = new Gtk.Button.with_label (_("_Next Level"));
        button.set_use_underline (true);
        button.halign = Gtk.Align.CENTER;
        button.valign = Gtk.Align.END;
        button.set_margin_bottom (100);
        button.get_style_context ().add_class ("suggested-action");
        button.clicked.connect (() => {
            label.destroy ();
            button.destroy ();

            headerbar.set_title (_("Level %d").printf (game.current_level));

            restart_game ();
        });

        overlay.add_overlay (label);
        overlay.add_overlay (button);

        overlay.show ();

        Timeout.add (500, () => {
            button.show ();
            button.grab_focus ();

            return Source.REMOVE;
        });
    }

    private void preferences_cb ()
    {
        var should_unpause = false;
        if (game.is_running)
        {
            pause_action.activate (null);
            should_unpause = true;
        }

        if (preferences_dialog != null)
        {
            preferences_dialog.present ();

            if (should_unpause)
                pause_action.activate (null);

            return;
        }

        preferences_dialog = new PreferencesDialog (window, settings, worm_settings);

        preferences_dialog.destroy.connect (() => {
            preferences_dialog = null;

            if (should_unpause)
                pause_action.activate (null);
        });

        preferences_dialog.run ();
    }

    private void game_over (int score, long lowest_high_score, int level_reached)
    {
        var is_high_score = (score > lowest_high_score);
        var is_game_won = (level_reached == NibblesGame.MAX_LEVEL + 1);

        var game_over_label = new Gtk.Label (is_game_won ? _("Congratulations!") : _("Game Over!"));
        game_over_label.halign = Gtk.Align.CENTER;
        game_over_label.valign = Gtk.Align.START;
        game_over_label.set_margin_top (150);
        game_over_label.get_style_context ().add_class ("menu-title");
        game_over_label.show ();

        var msg_label = new Gtk.Label (_("You have completed the game."));
        msg_label.halign = Gtk.Align.CENTER;
        msg_label.valign = Gtk.Align.START;
        msg_label.set_margin_top (window_height / 3);
        msg_label.get_style_context ().add_class ("menu-title");
        msg_label.show ();

        var score_string = ngettext ("%d Point".printf (score), "%d Points".printf (score), score);
        var score_label = new Gtk.Label (@"<b>$(score_string)</b>");
        score_label.set_use_markup (true);
        score_label.halign = Gtk.Align.CENTER;
        score_label.valign = Gtk.Align.START;
        score_label.set_margin_top (window_height / 3 + 80);
        score_label.show ();

        var points_left = lowest_high_score - score;
        var points_left_label = new Gtk.Label (_("(%d more points to reach the leaderboard)").printf (points_left));
        points_left_label.halign = Gtk.Align.CENTER;
        points_left_label.valign = Gtk.Align.START;
        points_left_label.set_margin_top (window_height / 3 + 100);
        points_left_label.show ();

        var button = new Gtk.Button.with_label (_("_Play Again"));
        button.set_use_underline (true);
        button.halign = Gtk.Align.CENTER;
        button.valign = Gtk.Align.END;
        button.set_margin_bottom (100);
        button.get_style_context ().add_class ("suggested-action");
        button.clicked.connect (() => {
            game_over_label.destroy ();
            score_label.destroy ();
            points_left_label.destroy ();
            button.destroy ();
            msg_label.destroy ();

            new_game_action.set_enabled (true);
            pause_action.set_enabled (true);

            show_new_game_screen_cb ();
        });
        button.show ();

        overlay.add_overlay (game_over_label);
        if (is_game_won)
            overlay.add_overlay (msg_label);
        if (game.numhumans == 1)
            overlay.add_overlay (score_label);
        if (game.numhumans == 1 && !is_high_score)
            overlay.add_overlay (points_left_label);
        overlay.add_overlay (button);

        button.grab_focus ();

        overlay.show ();
    }

    private void help_cb ()
    {
        try
        {
            Gtk.show_uri (window.get_screen (), "help:gnome-nibbles", Gtk.get_current_event_time ());
        }
        catch (Error e)
        {
            warning ("Unable to open help: %s", e.message);
        }
    }

    private void about_cb ()
    {
        const string authors[] = { "Sean MacIsaac",
                                   "Ian Peters",
                                   "Andreas Røsdal",
                                   "Guillaume Beland",
                                   "Iulian-Gabriel Radu",
                                   null };

        const string documenters[] = { "Kevin Breit",
                                       null };

        const string artists[] = { "Allan Day",
                                  null };

        Gtk.show_about_dialog (window,
                               "program-name", _("Nibbles"),
                               "logo-icon-name", "gnome-nibbles",
                               "version", VERSION,
                               "comments", _("A worm game for GNOME"),
                               "copyright",
                               "Copyright © 1999–2008 Sean MacIsaac, Ian Peters, Andreas Røsdal\n" +
                               "Copyright © 2009 Guillaume Beland\n" +
                               "Copyright © 2015 Iulian-Gabriel Radu",
                               "license-type", Gtk.License.GPL_2_0,
                               "authors", authors,
                               "documenters", documenters,
                               "artists", artists,
                               "translator-credits", _("translator-credits"),
                               "website", "https://wiki.gnome.org/Apps/Nibbles/"
                               );
    }

    public static int main (string[] args)
    {
        Intl.setlocale (LocaleCategory.ALL, "");
        Intl.bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
        Intl.bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        Intl.textdomain (GETTEXT_PACKAGE);

        return new Nibbles ().run (args);
    }
}

[GtkTemplate (ui = "/org/gnome/nibbles/ui/scoreboard.ui")]
private class Scoreboard : Gtk.Box
{
    private Gee.HashMap<PlayerScoreBox, Worm> boxes;

    public Scoreboard ()
    {
        boxes = new Gee.HashMap<PlayerScoreBox, Worm> ();
    }

    public void register (Worm worm, string color_name, Gdk.Pixbuf life_pixbuf)
    {
        var color = Pango.Color ();
        color.parse (color_name);

        var box = new PlayerScoreBox (@"Worm $(worm.id + 1)", color, worm.score, worm.lives, life_pixbuf);
        boxes.set (box, worm);
        add (box);
    }

    public void update ()
    {
        foreach (var entry in boxes.entries)
        {
            var box = entry.key;
            var worm = entry.value;

            box.update (worm.score, worm.lives);
        }
    }

    public void clear ()
    {
        foreach (var entry in boxes.entries)
        {
            var box = entry.key;
            box.destroy ();
        }
        boxes.clear ();
    }
}

[GtkTemplate (ui = "/org/gnome/nibbles/ui/player-score-box.ui")]
private class PlayerScoreBox : Gtk.Box
{
    [GtkChild]
    private Gtk.Label name_label;
    [GtkChild]
    private Gtk.Label score_label;
    [GtkChild]
    private Gtk.Grid lives_grid;

    private Gee.LinkedList<Gtk.Image> life_images;

    public PlayerScoreBox (string name, Pango.Color color, int score, int lives_left, Gdk.Pixbuf life_pixbuf)
    {
        name_label.set_markup ("<span color=\"" + color.to_string () + "\">" + name + "</span>");
        score_label.set_label (score.to_string ());

        life_images = new Gee.LinkedList<Gtk.Image> ();

        for (int i = 0; i < Worm.MAX_LIVES; i++)
        {
            var life = new Gtk.Image.from_pixbuf (life_pixbuf);
            life.show ();

            if (i >= Worm.STARTING_LIVES)
                life.set_opacity (0);

            life_images.add (life);
            lives_grid.attach (life, i % 6, i / 6);
        }
    }

    public void update (int score, int lives_left)
    {
        update_score (score);
        update_lives (lives_left);
    }

    public void update_score (int score)
    {
        score_label.set_label (score.to_string ());
    }

    public void update_lives (int lives_left)
    {
        /* Remove lost lives - if any */
        for (int i = life_images.size - 1; i >= lives_left; i--)
            life_images[i].set_opacity (0);

        /* Add new lives - if any */
        for (int i = 0; i < lives_left; i++)
            life_images[i].set_opacity (1);
    }
}

[GtkTemplate (ui = "/org/gnome/nibbles/ui/controls-grid.ui")]
private class ControlsGrid : Gtk.Grid
{
    [GtkChild]
    private Gtk.Label name_label;
    [GtkChild]
    private Gtk.Image arrow_up;
    [GtkChild]
    private Gtk.Image arrow_down;
    [GtkChild]
    private Gtk.Image arrow_left;
    [GtkChild]
    private Gtk.Image arrow_right;
    [GtkChild]
    private Gtk.Overlay move_up;
    [GtkChild]
    private Gtk.Label move_up_label;
    [GtkChild]
    private Gtk.Overlay move_down;
    [GtkChild]
    private Gtk.Label move_down_label;
    [GtkChild]
    private Gtk.Overlay move_left;
    [GtkChild]
    private Gtk.Label move_left_label;
    [GtkChild]
    private Gtk.Overlay move_right;
    [GtkChild]
    private Gtk.Label move_right_label;

    public ControlsGrid (int worm_id, WormProperties worm_props, Gdk.Pixbuf arrow, Gdk.Pixbuf arrow_key)
    {
        var color = Pango.Color ();
        color.parse (NibblesView.colorval_name (worm_props.color));

        /* Translators: the player's number, e.g. "Player 1" or "Player 2". */
        var player_id = _("Player %d").printf (worm_id + 1);
        name_label.set_markup (@"<b><span font-family=\"Sans\" color=\"$(color.to_string ())\">$(player_id)</span></b>");

        arrow_up.set_from_pixbuf (arrow.rotate_simple (Gdk.PixbufRotation.NONE));
        arrow_down.set_from_pixbuf (arrow.rotate_simple (Gdk.PixbufRotation.UPSIDEDOWN));
        arrow_left.set_from_pixbuf (arrow.rotate_simple (Gdk.PixbufRotation.COUNTERCLOCKWISE));
        arrow_right.set_from_pixbuf (arrow.rotate_simple (Gdk.PixbufRotation.CLOCKWISE));

        string upper_key;
        upper_key = Gdk.keyval_name (worm_props.up).up ();
        if (upper_key == "UP")
        {
            var rotated_pixbuf = arrow_key.rotate_simple (Gdk.PixbufRotation.NONE);
            move_up.add_overlay (new Gtk.Image.from_pixbuf (rotated_pixbuf));
            move_up.show_all ();
        }
        else
            move_up_label.set_markup (@"<b>$(upper_key)</b>");

        upper_key = Gdk.keyval_name (worm_props.down).up ();
        if (upper_key == "DOWN")
        {
            var rotated_pixbuf = arrow_key.rotate_simple (Gdk.PixbufRotation.UPSIDEDOWN);
            move_down.add_overlay (new Gtk.Image.from_pixbuf (rotated_pixbuf));
            move_down.show_all ();
        }
        else
            move_down_label.set_markup (@"<b>$(upper_key)</b>");

        upper_key = Gdk.keyval_name (worm_props.left).up ();
        if (upper_key == "LEFT")
        {
            var rotated_pixbuf = arrow_key.rotate_simple (Gdk.PixbufRotation.COUNTERCLOCKWISE);
            move_left.add_overlay (new Gtk.Image.from_pixbuf (rotated_pixbuf));
            move_left.show_all ();
        }
        else
            move_left_label.set_markup (@"<b>$(upper_key)</b>");

        upper_key = Gdk.keyval_name (worm_props.right).up ();
        if (upper_key == "RIGHT")
        {
            var rotated_pixbuf = arrow_key.rotate_simple (Gdk.PixbufRotation.CLOCKWISE);
            move_right.add_overlay (new Gtk.Image.from_pixbuf (rotated_pixbuf));
            move_right.show_all ();
        }
        else
            move_right_label.set_markup (@"<b>$(upper_key)</b>");
    }
}
