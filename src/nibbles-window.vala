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

[GtkTemplate (ui = "/org/gnome/nibbles/ui/nibbles.ui")]
private class NibblesWindow : ApplicationWindow
{
    /* Application and worm settings */
    private GLib.Settings settings;
    private Gee.ArrayList<GLib.Settings> worm_settings;

    /* state */
    private bool window_is_maximized;
    private bool window_is_tiled;
    private int window_width;
    private int window_height;

    /* Main widgets */
    [GtkChild] private Stack main_stack;
    [GtkChild] private Overlay overlay;

    /* HeaderBar */
    [GtkChild] private HeaderBar headerbar;
    [GtkChild] private Button new_game_button;
    [GtkChild] private Button pause_button;

    /* Pre-game screen widgets */
    private Gee.LinkedList<ToggleButton> number_of_players_buttons;
    private Gee.LinkedList<ToggleButton> number_of_ai_buttons;
    [GtkChild] private ToggleButton players1;
    [GtkChild] private ToggleButton players2;
    [GtkChild] private ToggleButton players3;
    [GtkChild] private ToggleButton players4;
    [GtkChild] private ToggleButton ai0;
    [GtkChild] private ToggleButton ai1;
    [GtkChild] private ToggleButton ai2;
    [GtkChild] private ToggleButton ai3;
    [GtkChild] private ToggleButton ai4;
    [GtkChild] private ToggleButton ai5;

    [GtkChild] private Button next_button;
    [GtkChild] private Button start_button;

    [GtkChild] private Box grids_box;
    private Gdk.Pixbuf arrow_pixbuf;
    private Gdk.Pixbuf arrow_key_pixbuf;

    /* Statusbar widgets */
    [GtkChild] private Stack statusbar_stack;
    [GtkChild] private Label countdown;
    [GtkChild] private Scoreboard scoreboard;
    private Gdk.Pixbuf scoreboard_life;

    /* Preferences dialog */
    private PreferencesDialog preferences_dialog = null;

    /* Rendering of the game */
    private NibblesView? view;

    [GtkChild] private Box game_box;
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

    private const GLib.ActionEntry menu_entries[] =
    {
        {"start-game", start_game_cb},
        {"new-game", new_game_cb},
        {"pause", pause_cb},
        {"preferences", preferences_cb},
        {"scores", scores_cb},

        {"show-new-game-screen", show_new_game_screen_cb},
        {"show-controls-screen", show_controls_screen_cb},
        {"back", back_cb}
    };

    construct
    {
        add_action_entries (menu_entries, this);
        new_game_action = (SimpleAction) lookup_action ("new-game");
        pause_action    = (SimpleAction) lookup_action ("pause");
        back_action     = (SimpleAction) lookup_action ("back");

        settings = new GLib.Settings ("org.gnome.Nibbles");
        settings.changed.connect (settings_changed_cb);

        worm_settings = new Gee.ArrayList<GLib.Settings> ();
        for (int i = 0; i < NibblesGame.MAX_WORMS; i++)
        {
            var name = "org.gnome.Nibbles.worm%d".printf(i);
            worm_settings.add (new GLib.Settings (name));
            worm_settings[i].changed.connect (worm_settings_changed_cb);
        }

        size_allocate.connect (size_allocate_cb);
        window_state_event.connect (window_state_event_cb);
        set_default_size (settings.get_int ("window-width"), settings.get_int ("window-height"));
        if (settings.get_boolean ("window-is-maximized"))
            maximize ();

        key_controller = new EventControllerKey (this);
        key_controller.key_pressed.connect (key_press_event_cb);

        number_of_players_buttons = new Gee.LinkedList<ToggleButton> ();
        players1.toggled.connect (change_number_of_players_cb);
        number_of_players_buttons.add (players1);
        players2.toggled.connect (change_number_of_players_cb);
        number_of_players_buttons.add (players2);
        players3.toggled.connect (change_number_of_players_cb);
        number_of_players_buttons.add (players3);
        players4.toggled.connect (change_number_of_players_cb);
        number_of_players_buttons.add (players4);

        number_of_ai_buttons = new Gee.LinkedList<ToggleButton> ();
        ai0.toggled.connect (change_number_of_ai_cb);
        number_of_ai_buttons.add (ai0);
        ai1.toggled.connect (change_number_of_ai_cb);
        number_of_ai_buttons.add (ai1);
        ai2.toggled.connect (change_number_of_ai_cb);
        number_of_ai_buttons.add (ai2);
        ai3.toggled.connect (change_number_of_ai_cb);
        number_of_ai_buttons.add (ai3);
        ai4.toggled.connect (change_number_of_ai_cb);
        number_of_ai_buttons.add (ai4);
        ai5.toggled.connect (change_number_of_ai_cb);
        number_of_ai_buttons.add (ai5);

        /* Create game */
        game = new NibblesGame (settings.get_int ("tile-size"),
                                settings.get_int ("start-level"),
                                settings.get_int ("speed"),
                                settings.get_boolean ("fakes"));
        game.log_score.connect (log_score_cb);
        game.level_completed.connect (level_completed_cb);
        game.notify["is-paused"].connect (() => {
            if (game.is_paused)
                statusbar_stack.set_visible_child_name ("paused");
            else
                statusbar_stack.set_visible_child_name ("scoreboard");
        });

        /* Create view */
        view = new NibblesView (game, !settings.get_boolean ("sound"));
        view.show ();

        frame = new Games.GridFrame (NibblesGame.WIDTH, NibblesGame.HEIGHT);
        game_box.pack_start (frame);

        /* Create scoreboard */
        scoreboard_life = view.load_pixmap_file ("scoreboard-life.svg", 2 * game.tile_size, 2 * game.tile_size);

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
    }

    internal void on_shutdown ()
    {
        settings.delay ();
        // window state
        settings.set_int ("window-width", window_width);
        settings.set_int ("window-height", window_height);
        settings.set_boolean ("window-is-maximized", window_is_maximized);

        // game properties
        settings.set_int ("tile-size", game.tile_size);
        settings.set_int ("start-level", game.start_level);
        settings.set_int ("speed", game.speed);
        settings.set_boolean ("fakes", game.fakes);
        settings.apply ();
    }

    private bool countdown_cb ()
    {
        seconds--;

        if (seconds == 0)
        {
            statusbar_stack.set_visible_child_name ("scoreboard");
            view.name_labels.hide ();

            game.start (/* add initial bonus */ true);

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
    private EventControllerKey key_controller;          // for keeping in memory
    private bool key_press_event_cb (EventControllerKey _key_controller, uint keyval, uint keycode, Gdk.ModifierType state)
    {
        return game.handle_keypress (keyval);
    }

    private void size_allocate_cb (Allocation allocation)
    {
        if (window_is_maximized || window_is_tiled)
            return;
        get_size (out window_width, out window_height);
    }

    private bool window_state_event_cb (Gdk.EventWindowState event)
    {
        if ((event.changed_mask & Gdk.WindowState.MAXIMIZED) != 0)
            window_is_maximized = (event.new_window_state & Gdk.WindowState.MAXIMIZED) != 0;
        /* We don’t save this state, but track it for saving size allocation */
        if ((event.changed_mask & Gdk.WindowState.TILED) != 0)
            window_is_tiled = (event.new_window_state & Gdk.WindowState.TILED) != 0;
        return false;
    }

    private void start_game_cb ()
    {
        settings.set_boolean ("first-run", false);

        if (game.is_paused)
            set_pause_button_label (/* paused */ false);
        game.reset ();

        view.new_level (game.current_level);
        view.connect_worm_signals ();

        scoreboard.clear ();
        foreach (var worm in game.worms)
        {
            var color = game.worm_props.@get (worm).color;
            scoreboard.register (worm, NibblesView.colorval_name_untranslated (color), scoreboard_life);
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

        var dialog = new MessageDialog (this,
                                        DialogFlags.MODAL,
                                        MessageType.WARNING,
                                        ButtonsType.OK_CANCEL,
                                        /* Translators: message displayed in a MessageDialog, when the player tries to start a game while one is running */
                                        _("Are you sure you want to start a new game?"));


        /* Translators: message displayed in a MessageDialog, when the player tries to start a game while one is running */
        dialog.secondary_text = _("If you start a new game, the current one will be lost.");

        var button = (Button) dialog.get_widget_for_response (ResponseType.OK);
        /* Translators: label of a button displayed in a MessageDialog, when the player tries to start a game while one is running */
        button.set_label (_("_New Game"));
        dialog.response.connect ((response_id) => {
            if (response_id == ResponseType.OK)
                show_new_game_screen_cb ();
            if ((response_id == ResponseType.CANCEL || response_id == ResponseType.DELETE_EVENT)
                && !game.is_paused)
            {
                if (seconds == 0)
                    game.start (/* add initial bonus */ false);
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
                set_pause_button_label (/* paused */ true);
            }
            else
            {
                game.unpause ();
                set_pause_button_label (/* paused */ false);
                view.grab_focus ();
            }
        }
    }

    private void set_pause_button_label (bool paused)
    {
        if (paused)
        {
            /* Translators: label of the Pause button, when the game is paused */
            pause_button.set_label (_("_Resume"));
        }
        else
        {
            /* Translators: label of the Pause button, when the game is running */
            pause_button.set_label (_("_Pause"));
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

    private void worm_settings_changed_cb (GLib.Settings changed_worm_settings, string key)
    {
        /* Empty worm properties means game has not started yet */
        if (game.worm_props.size == 0)
            return;

        var id = worm_settings.index_of (changed_worm_settings);

        if (id >= game.numworms)
            return;

        var worm = game.worms[id];
        var properties = game.worm_props.@get (worm);

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

        game.worm_props.@set (worm, properties);
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

        headerbar.set_title (Nibbles.PROGRAM_NAME);

        new_game_action.set_enabled (false);
        pause_action.set_enabled (false);
        back_action.set_enabled (true);

        new_game_button.hide ();
        pause_button.hide ();

        number_of_players_buttons[game.numhumans - 1].set_active (true);
        number_of_ai_buttons[game.numai].set_active (true);

        set_default (next_button);

        main_stack.set_transition_type (StackTransitionType.NONE);
        main_stack.set_visible_child_name ("number_of_players");
        main_stack.set_transition_type (StackTransitionType.SLIDE_UP);
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
                var grid = new ControlsGrid (worm.id, game.worm_props.@get (worm), arrow_pixbuf, arrow_key_pixbuf);
                grids_box.add (grid);
            }
        }

        set_default (start_button);

        main_stack.set_visible_child_name ("controls");
    }

    private void show_game_view ()
    {
        /* FIXME: If there's a transition set, on Wayland, the ClutterEmbed
         * will show outside the game's window. Don't change the transition
         * type when that's no longer a problem.
         */
        main_stack.set_transition_type (StackTransitionType.NONE);
        new_game_button.show ();
        pause_button.show ();

        back_action.set_enabled (false);

        /* Translators: title of the headerbar, while a game is running; the %d is replaced by the level number */
        headerbar.set_title (_("Level %d").printf (game.current_level));        // TODO unduplicate, 1/2
        main_stack.set_visible_child_name ("game_box");

        main_stack.set_transition_type (StackTransitionType.SLIDE_UP);
    }

    private void back_cb ()
    {
        main_stack.set_transition_type (StackTransitionType.SLIDE_DOWN);

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

        main_stack.set_transition_type (StackTransitionType.SLIDE_UP);
    }

    private void change_number_of_players_cb (ToggleButton button)
    {
        var is_same_button = true;
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
                    is_same_button = false;
                    break;
                }
            }
        }

        /* Ignore clicks on the same button. */
        if (is_same_button)
        {
            SignalHandler.block_matched (button, SignalMatchType.DATA, 0, 0, null, null, this);
            button.set_active (true);
            SignalHandler.unblock_matched (button, SignalMatchType.DATA, 0, 0, null, null, this);
            return;
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

    private void change_number_of_ai_cb (ToggleButton button)
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
            /* Translators: label displayed on the scores dialog, preceeding a difficulty. */
            _("Difficulty Level:"),
            this,
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

        view.hide ();

        new_game_action.set_enabled (false);
        pause_action.set_enabled (false);

        /* Translators: label that appears at the end of a level; the %d is the number of the level that was completed */
        var label = new Label (_("Level %d Completed!").printf (game.current_level));
        label.halign = Align.CENTER;
        label.valign = Align.START;
        label.set_margin_top (150);
        label.get_style_context ().add_class ("menu-title");
        label.show ();

        /* Translators: label of a button that appears at the end of a level; starts next level */
        var button = new Button.with_label (_("_Next Level"));
        button.set_use_underline (true);
        button.halign = Align.CENTER;
        button.valign = Align.END;
        button.set_margin_bottom (100);
        button.get_style_context ().add_class ("suggested-action");
        button.set_can_default (true);
        button.clicked.connect (() => {
            label.destroy ();
            button.destroy ();

            /* Translators: title of the headerbar, while a game is running; the %d is replaced by the level number */
            headerbar.set_title (_("Level %d").printf (game.current_level));    // TODO unduplicate, 2/2

            view.show ();

            restart_game ();
        });

        overlay.add_overlay (label);
        overlay.add_overlay (button);

        overlay.grab_default ();

        Timeout.add (500, () => {
            button.show ();
            button.grab_default ();

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

        preferences_dialog = new PreferencesDialog (this, settings, worm_settings);

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

        /* Translators: label displayed at the end of a level, if the player finished all the levels */
        var game_over_label = new Label (is_game_won ? _("Congratulations!")


        /* Translators: label displayed at the end of a level, if the player did not finished all the levels */
                                                     : _("Game Over!"));
        game_over_label.halign = Align.CENTER;
        game_over_label.valign = Align.START;
        game_over_label.set_margin_top (150);
        game_over_label.get_style_context ().add_class ("menu-title");
        game_over_label.show ();

        /* Translators: label displayed at the end of a level, if the player finished all the levels */
        var msg_label = new Label (_("You have completed the game."));
        msg_label.halign = Align.CENTER;
        msg_label.valign = Align.START;
        msg_label.set_margin_top (window_height / 3);
        msg_label.get_style_context ().add_class ("menu-title");
        msg_label.show ();

        var score_string = ngettext ("%d Point", "%d Points", score);
        score_string = score_string.printf (score);
        var score_label = new Label (@"<b>$(score_string)</b>");
        score_label.set_use_markup (true);
        score_label.halign = Align.CENTER;
        score_label.valign = Align.START;
        score_label.set_margin_top (window_height / 3 + 80);
        score_label.show ();

        var points_left = lowest_high_score - score;
        /* Translators: label displayed at the end of a level, if the player did not score enough to have its score saved */
        var points_left_label = new Label (_("(%ld more points to reach the leaderboard)").printf (points_left));
        points_left_label.halign = Align.CENTER;
        points_left_label.valign = Align.START;
        points_left_label.set_margin_top (window_height / 3 + 100);
        points_left_label.show ();

        /* Translators: label of a button displayed at the end of a level; restarts the game */
        var button = new Button.with_label (_("_Play Again"));
        button.set_use_underline (true);
        button.halign = Align.CENTER;
        button.valign = Align.END;
        button.set_margin_bottom (100);
        button.get_style_context ().add_class ("suggested-action");
        button.clicked.connect (() => {
            game_over_label.destroy ();
            score_label.destroy ();
            points_left_label.destroy ();
            button.destroy ();
            msg_label.destroy ();

            view.show ();

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

        view.hide ();
        new_game_action.set_enabled (false);
        pause_action.set_enabled (false);
        back_action.set_enabled (false);
    }
}