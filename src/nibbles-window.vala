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
 */

/* designed for Gtk 4, link with libgtk-4-dev or gtk4-devel */
using Gtk;

private enum SetupScreen
{
    USUAL,
    SPEED,
    CONTROLS,
    GAME
}

[GtkTemplate (ui = "/org/gnome/Nibbles/ui/are-you-sure.ui")]
private class AreYouSureWindow : Window
{
    [GtkChild] private unowned Label line_one;
    [GtkChild] private unowned Label line_two;
    [GtkChild] private unowned Button button_no;
    [GtkChild] private unowned Button button_yes;

    internal delegate void AreYouSureResultFunction (bool yes);
    AreYouSureResultFunction result_function;

    public AreYouSureWindow (Window w, string line1, string line2, AreYouSureResultFunction result_function)
    {
        set_transient_for (w);
        line_one.label = line1;
        line_two.label = line2;
        this.result_function = (AreYouSureResultFunction)result_function;
        close_request.connect (()=>{result_function (false); return false;});
        button_no.clicked.connect (()=>{result_function (false); destroy ();});
        button_yes.clicked.connect (()=>{result_function (true); destroy ();});
    }
}

#if USE_LIBADWAITA
[GtkTemplate (ui = "/org/gnome/Nibbles/ui/nibbles-adw.ui")]
private class NibblesWindow : Adw.ApplicationWindow
#else
[GtkTemplate (ui = "/org/gnome/Nibbles/ui/nibbles.ui")]
private class NibblesWindow : ApplicationWindow
#endif
{
    /* Application and worm settings */
    private GLib.Settings settings;
    private Gee.ArrayList<GLib.Settings> worm_settings;

    private bool fullscreen_mode = false;

    /* Main widgets */
    [GtkChild] private unowned Stack main_stack;
    [GtkChild] private unowned Overlay overlay;

    /* HeaderBar */
#if USE_LIBADWAITA
    [GtkChild] private unowned Adw.ToolbarView toolbar;
    [GtkChild] private unowned Adw.HeaderBar headerbar;
#else
    [GtkChild] private unowned HeaderBar headerbar;
#endif
    [GtkChild] private unowned Button new_game_button;
    [GtkChild] private unowned Button pause_button;
    [GtkChild] private unowned MenuButton hamburger_menu;

    /* Pre-game screen widgets */
    [GtkChild] private unowned Players players;
    [GtkChild] private unowned Speed speed;
    [GtkChild] private unowned BoardProgress board_progress;
    [GtkChild] private unowned Controls controls;

    /* Statusbar widgets */
    [GtkChild] private unowned Stack statusbar_stack;
    [GtkChild] private unowned Scoreboard scoreboard;

    /* Rendering of the game */
    private NibblesView? view;

    [GtkChild] private unowned Box game_box;

    /* sound interface */
    private Sound sound;

    /* keyboard interface */
    class DelegateStack
    {
        internal class DelegateStackIterator
        {
            /* variables */
            private Node? pIterator; /* pointer to next node */
            bool first_next;

            /* public functions */
            public DelegateStackIterator (DelegateStack p)
            {
                pIterator = p.pHead;
                first_next = true;
            }

            public bool next ()
            {
                if (pIterator == null)
                    return false;
                else if (first_next)
                {
                    first_next = !first_next;
                    return true;
                }
                else
                {
                    pIterator = pIterator.pNext;
                    return pIterator != null;
                }
            }

            public unowned KeypressHandlerFunction @get ()
            {
                return pIterator.keypress_handler;
            }
        }

        struct Node
        {
            unowned KeypressHandlerFunction keypress_handler;
            Node? pNext;
        }
        Node? pHead = null;

        internal void push (KeypressHandlerFunction handler)
        {
            if (pHead == null)
                pHead = { handler, null};
            else
                pHead = { handler, pHead};
        }

        internal bool pop ()
        {
            if (pHead == null)
                return false;
            else
            {
                pHead = pHead.pNext;
                return true;
            }
        }

        internal void remove (KeypressHandlerFunction handler)
        {
            if (pHead != null && pHead.keypress_handler == handler)
                pHead = pHead.pNext;
            else if (pHead != null && pHead.pNext != null)
            {
                var pTrail = pHead;
                for (var p = pTrail.pNext; p != null;)
                {
                    if (p.keypress_handler == handler)
                    {
                        pTrail.pNext = p.pNext;
                        break;
                    }
                    else
                    {
                         pTrail = p;
                         p = p.pNext;
                    }
                }
            }
        }

        public DelegateStackIterator iterator ()
        {
            return new DelegateStackIterator (this);
        }
    }
    DelegateStack keypress_handlers = new DelegateStack ();

    /* Game being played */
    private NibblesGame? game = null;
    public  int cli_start_level { private get; internal construct; }
    private int start_level { private get { return cli_start_level == 0 ? settings.get_int ("start-level") : cli_start_level; }}
    public int progress { internal get; internal construct set; }
    public  SetupScreen start_screen { private get; internal construct; }
    public bool game_paused
    {
        get {return game != null && game.paused;}
        private set {}
    }

    /* Used for handling the game's scores */
    private Games.Scores.Context scores_context;

    /* HeaderBar actions */
    private SimpleAction hamburger_action;
    private SimpleAction new_game_action;
    private SimpleAction fullscreen_action;
    private SimpleAction pause_action;
    private SimpleAction back_action;
    private SimpleAction start_game_action;

    /* count down variables */
    private uint countdown_id = 0;
    private const int COUNTDOWN_TIME = 3;
    private int seconds = 0;

    #if USE_LIBADWAITA
    bool dialog_visible = false;
    #endif

    private const GLib.ActionEntry menu_entries[] =
    {
        { "hamburger",      hamburger_cb    },
        { "new-game",       new_game_cb     },  // the "New Game" button (during game), or the ctrl-N shortcut (mostly all the time)
        { "fullscreen",     fullscreen_cb   },
        { "pause",          pause_cb        },
        { "scores",         scores_cb       },

        { "next-screen",    next_screen_cb  },  // called from first-run, players, board-progress and speed
        { "start-game",     start_game      },  // called from controls
        { "back",           back_cb         }   // called on Escape pressed; disabled only during countdown (TODO pause?)
    };

    Gee.ArrayList<Widget> overlay_members = new Gee.ArrayList<Widget> ();
    void overlay_add (Widget widget)
    {
        overlay_members.add (widget);
        overlay.add_overlay (widget);
    }

    bool overlay_remove_all ()
    {
        if (overlay_members.size == 0)
            return false;
        else
        {
            for (int i = overlay_members.size; i > 0 ; --i)
            {
                overlay.remove_overlay (overlay_members[i - 1]);
                overlay_members.remove_at (i - 1);
            }
            return true;
        }
    }

    internal NibblesWindow (int cli_start_level, SetupScreen start_screen)
    {
        Object (cli_start_level: cli_start_level, start_screen: start_screen);
    }

    construct
    {
        add_action_entries (menu_entries, this);
        hamburger_action    = (SimpleAction) lookup_action ("hamburger");
        new_game_action     = (SimpleAction) lookup_action ("new-game");
        fullscreen_action   = (SimpleAction) lookup_action ("fullscreen");
        pause_action        = (SimpleAction) lookup_action ("pause");
        back_action         = (SimpleAction) lookup_action ("back");
        start_game_action   = (SimpleAction) lookup_action ("start-game");

        settings = new GLib.Settings ("org.gnome.Nibbles");
        settings.changed.connect (settings_changed_cb);
        add_action (settings.create_action ("sound"));
        add_action (settings.create_action ("three-dimensional-view"));

        worm_settings = new Gee.ArrayList<GLib.Settings> ();
        for (int i = 0; i < NibblesGame.MAX_WORMS; i++)
        {
            var name = "org.gnome.Nibbles.worm%d".printf (i);
            worm_settings.add (new GLib.Settings (name));
            worm_settings[i].changed.connect (worm_settings_changed_cb);
        }

        hamburger_menu.get_popover ().closed.connect (() =>
        {
            if (null != view)
                set_focus (view);
        });

        set_default_size (settings.get_int ("window-width"), settings.get_int ("window-height"));
        if (settings.get_boolean ("window-is-maximized"))
            maximize ();

        /* create keyboard interface */
        EventControllerKey key_controller = new EventControllerKey ();
        key_controller.key_pressed.connect ((/*EventControllerKey*/controller,/*uint*/keyval,/*uint*/keycode,/*Gdk.ModifierType*/state)=>
        {
            /* The reason this event handler is found here (and not in nibbles-view.vala
             * which would be a more suitable place) is to avoid a weird behavior of having
             * your first key press ignored everytime by the start of a new level, thus
             * making your worm unresponsive to your command.
             */
            if ((!) (Gdk.keyval_name (keyval) ?? "") == "F1")
            {
                if ((state & Gdk.ModifierType.CONTROL_MASK) > 0)
                    activate_action ("show-help-overlay", null);
                else if ((state & Gdk.ModifierType.SHIFT_MASK) > 0)
                    application.activate_action ("about", null); //about_cb ();
                else if ((state & Gdk.ModifierType.SHIFT_MASK) == 0)
                    application.activate_action ("help", null); //help_cb ();
                else
                    return false;
                return true;
            }
            else
            {
                DelegateStack handlers_to_remove = new DelegateStack ();
                foreach (var handler in keypress_handlers)
                {
                    bool remove_handler;
                    bool r = handler (keyval, keycode, out remove_handler);
                    if (remove_handler)
                        handlers_to_remove.push (handler);
                    if (r)
                    {
                        /* remove any handlers that need to be removed before we return */
                        foreach (var h in handlers_to_remove)
                            keypress_handlers.remove (h);
                        return r;
                    }
                }
                /* remove any handlers that need to be removed before we return */
                foreach (var handler in handlers_to_remove)
                    keypress_handlers.remove (handler);
                return false;
            }
        });
        key_controller.im_update.connect (()=>
        {
            assert (false);
        });
        ((Widget)(this)).add_controller (key_controller);

        /* view type */
        bool three_dimensional_view = settings.get_boolean ("three-dimensional-view");

        /* create sound interface */
        sound = new Sound (!settings.get_boolean ("sound"));

        /* Create game */
        game = new NibblesGame (start_level,
                                settings.get_int ("speed"),
                                /* GAMEDELAY, */ 35,
                                settings.get_boolean ("fakes"),
                                three_dimensional_view,
                                NibblesView.WIDTH,
                                NibblesView.HEIGHT);
        game.log_score.connect (log_score_cb);
        game.level_completed.connect (level_completed_cb);
        sound.connect_signal (game);
        game.get_pkgdatadir.connect (()=> {return PKGDATADIR;});
        game.add_keypress_handler.connect ((handler)=>
        {
            if (handler != null)
                keypress_handlers.push (handler);
            else
                keypress_handlers.pop ();
            return true;
        });

        /* Create board view */
        view = new NibblesView (game, countdown_active, fullscreen_active);
        view.set_visible (true);
        view.vexpand = true;
        game_box.prepend (view);
        set_focus (view);

        /* Number of worms */
        game.numhumans = settings.get_int ("players");
        int numai = settings.get_int ("ai");
        if (numai + game.numhumans > NibblesGame.MAX_WORMS)
        {
            game.numhumans = 1;
            numai = 5;
            settings.set_int ("players", game.numhumans);
            settings.set_int ("ai", numai);
        }
        game.numai = numai;
        // NOTE: set numai value to 0 here
        players.set_values (game.numhumans, numai);

        /* Board Progress screen */
        board_progress.set_values (settings.get_int ("progress"), settings.get_int ("start-level"));

        /* Speed screen */
        speed.set_values (settings.get_int ("speed"),
                          settings.get_boolean ("fakes"));

        /* Controls screen */
        controls.add_keypress_handler.connect ((handler)=>
        {
            if (handler != null)
                keypress_handlers.push (handler);
            else
                keypress_handlers.pop ();
        });

        /* Check whether to display the first run screen */
        if (start_screen == SetupScreen.GAME)
        {
            game.numhumans = settings.get_int ("players");
            game.numai     = settings.get_int ("ai");
            game.speed     = settings.get_int ("speed");
            game.fakes     = settings.get_boolean ("fakes");
            game.create_worms (worm_settings);

            start_game ();
        }
        else if (start_screen == SetupScreen.CONTROLS)
        {
            game.numhumans = settings.get_int ("players");
            game.numai     = settings.get_int ("ai");
            game.speed     = settings.get_int ("speed");
            game.fakes     = settings.get_boolean ("fakes");

            show_controls_screen ();
        }
        else if (start_screen == SetupScreen.SPEED)
        {
            game.numhumans = settings.get_int ("players");
            game.numai     = settings.get_int ("ai");

            main_stack.set_visible_child_name ("speed");
        }
        else if (settings.get_boolean ("first-run"))
        {
            FirstRun first_run_panel = new FirstRun ();
            first_run_panel.set_visible (true);
            main_stack.add_named (first_run_panel, "first-run");

            new_game_action.set_enabled (true);
            pause_action.set_enabled (false);
            back_action.set_enabled (false);

            main_stack.set_visible_child (first_run_panel);
        }
        else
        {
            show_new_game_screen ();
        }

        /* Create scores */
        create_scores ();
    }

    internal void on_shutdown ()
    {
        settings.delay ();
        // window state
        int window_width;
        int window_height;
        get_default_size (out window_width, out window_height);
        settings.set_int ("window-width", window_width);
        settings.set_int ("window-height", window_height);
        settings.set_boolean ("window-is-maximized", maximized);

        // game properties
        settings.set_int ("speed", game.speed);
        settings.set_boolean ("fakes", game.fakes);
        settings.apply ();
    }

    private bool countdown_cb ()
    {
        seconds--;
        view.redraw ();
        game.play_sound ("gobble");

        if (seconds == 0)
        {
            game.start (/* add initial bonus */ true);

            pause_action.set_enabled (true);

            countdown_id = 0;
            return Source.REMOVE;
        }

        return Source.CONTINUE;
    }

    /*\
    * * Window events
    \*/

    private void start_game ()
    {
        settings.set_boolean ("first-run", false);

        if (game.paused)
            set_pause_button_label (/* paused */ false);
        game.reset (start_level);

        game.new_level (game.current_level);

        scoreboard.clear ();
        foreach (var worm in game.worms)
        {
            var color = game.worm_props.@get (worm).color;
            scoreboard.register (worm, NibblesView.colorval_name_untranslated (color));
            worm.notify["lives"].connect (scoreboard.update);
            worm.notify["score"].connect (scoreboard.update);
        }
        game.add_worms ();

        show_game_view ();

        start_game_with_countdown ();
    }

    private void start_game_with_countdown ()
    {
        new_game_action.set_enabled (true);
        back_action.set_enabled (true);

        seconds = COUNTDOWN_TIME;
        view.redraw ();
        game.play_sound ("gobble");

        countdown_id = Timeout.add_seconds (1, countdown_cb);
    }

    private void restart_game ()
    {
        game.new_level (game.current_level);

        game.add_worms ();
        start_game_with_countdown ();
    }

    private void hamburger_cb ()
    {
        if (!fullscreen_active () && !hamburger_menu.get_active())
            hamburger_menu.popup ();
    }
    
    private void new_game_cb ()
    {
        var child_name = main_stack.get_visible_child_name ();
        switch (child_name)
        {
            case "first-run":
            case "number_of_players":
            case "board-progress":
            case "speed":
                next_screen_cb ();
                break;
            case "controls":
                start_game ();
                break;
            case "game_box":
                if ((game.is_running || countdown_active () > 0) && fullscreen_active ())
                    fullscreen_cb (); /* escape key switches off full screen mode */
                else
                {
                    overlay_remove_all ();
                    if (end_of_game)    // TODO better
                    {
                        view.set_visible (true);
                        end_of_game = false;

                        show_new_game_screen ();
                    }
                    else
                    #if USE_LIBADWAITA
                        if (!this.dialog_visible)
                    #endif
                        show_new_game_dialog ();
                }
                break;
        }
    }

    private void show_new_game_dialog ()
    {
        if (countdown_id != 0)
        {
            Source.remove (countdown_id);
            countdown_id = 0;
        }

        if (game.is_running)
            game.stop ();

        #if USE_LIBADWAITA
        var dialog = new Adw.AlertDialog (
            /* Translators: first line of message displayed in an alert dialog, when the player tries to start a game while one is running */
            _("New Game?"),
            /* Translators: second line of message displayed in an alert dialog, when the player tries to start a game while one is running */
            _("If you start a new game, the current one will be lost"));

        dialog.add_response ("cancel", _("_Cancel"));
        dialog.add_response ("new-game", _("_New Game"));
        dialog.set_response_appearance ("new-game", Adw.ResponseAppearance.DESTRUCTIVE);

        dialog.response.connect ((response) => {
            this.dialog_visible = false;

            if (response == "new-game")
              show_new_game_screen ();
            if (response != "new-game" && !game.paused)
            {
                if (seconds == 0)
                    game.start (/* add initial bonus */ false);
                else
                    countdown_id = Timeout.add_seconds (1, countdown_cb);

                view.grab_focus ();
            }
        });

        dialog.choose.begin (this, null, (obj,res)=>{dialog.choose.end (res);});
        this.dialog_visible = true;
        #else
        var dialog = new AreYouSureWindow (this,
            /* Translators: first line of message displayed in a modal dialog, when the player tries to start a game while one is running */
            _("Are you sure you want to start a new game?"),
            /* Translators: second line of message displayed in a modal dialog, when the player tries to start a game while one is running */
            _("If you start a new game, the current one will be lost."),
            (/* bool */yes)=>
            {
                if (yes)
                  show_new_game_screen ();
                if (!yes && !game.paused)
                {
                    if (seconds == 0)
                        game.start (/* add initial bonus */ false);
                    else
                        countdown_id = Timeout.add_seconds (1, countdown_cb);

                    view.grab_focus ();
                }
            });
        dialog.set_visible (true);
        #endif
    }

    private void fullscreen_cb ()
    {
        if (game != null)
        {
            if (fullscreen_mode)
            {
                /* restore original window */
                #if USE_LIBADWAITA
                toolbar.reveal_top_bars = true;
                toolbar.reveal_bottom_bars = true;
                #endif
                statusbar_stack.visible = true;
                unfullscreen ();
            }
            else
            {
                /* set to full screen mode */
                fullscreen ();
                #if USE_LIBADWAITA
                toolbar.reveal_top_bars = false;
                toolbar.reveal_bottom_bars = false;
                #endif
                statusbar_stack.visible = !(game.is_running || countdown_active () > 0);
            }
            fullscreen_mode = !fullscreen_mode;
        }
    }

    private void pause_cb ()
    {
        if (game != null)
        {
            game.paused = game.is_running;
            set_pause_button_label (game.paused);
            if (!fullscreen_active ())
                statusbar_stack.set_visible_child_name (game.paused ? "paused" : "scoreboard");
            if (!game.paused)
                view.grab_focus ();
        }
    }

    private void set_pause_button_label (bool paused)
    {
        if (paused)
        {
            #if USE_LIBADWAITA
            /* Translators: tooltip of the pause button, when the game is paused */
            pause_button.set_tooltip_text (_("Resume"));
            pause_button.set_icon_name ("media-playback-start-symbolic");
            #else
            /* Translators: label of the Pause button, when the game is paused */
            pause_button.set_label (_("_Resume"));
            #endif
        }
        else
        {
            #if USE_LIBADWAITA
            /* Translators: tooltip of the pause button, when the game is running */
            pause_button.set_tooltip_text (_("Pause"));   // duplicated in nibbles.ui
            pause_button.set_icon_name ("media-playback-pause-symbolic");   // duplicated in nibbles.ui
            #else
            /* Translators: label of the Pause button, when the game is running */
            pause_button.set_label (_("_Pause"));   // duplicated in nibbles.ui
            #endif
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
                sound.set_muted (!settings.get_boolean (key));
                break;
            case "fakes":
                game.fakes = settings.get_boolean (key);
                break;
            case "three-dimensional-view":
                game.three_dimensional_view = settings.get_boolean (key);
                if (null != view)
                    view.redraw_all ();
                break;
        }
    }

    private void worm_settings_changed_cb (GLib.Settings changed_worm_settings, string key)
    {
        /* Empty worm properties means game has not started yet */
        if (game.worm_props.size == 0)
            return;

        assert (worm_settings != null);
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
            case "key-up-raw":
                properties.raw_up = changed_worm_settings.get_int ("key-up-raw");
                if (properties.raw_up < 0)
                    properties.raw_up = get_raw_key (properties.up);
                break;
            case "key-down-raw":
                properties.raw_down = changed_worm_settings.get_int ("key-down-raw");
                if (properties.raw_down < 0)
                    properties.raw_down= get_raw_key (properties.down);
                break;
            case "key-left-raw":
                properties.raw_left = changed_worm_settings.get_int ("key-left-raw");
                if (properties.raw_left < 0)
                    properties.raw_left = get_raw_key (properties.left);
                break;
            case "key-right-raw":
                properties.raw_right = changed_worm_settings.get_int ("key-right-raw");
                if (properties.raw_right < 0)
                    properties.raw_right = get_raw_key (properties.right);
                break;
        }

        game.worm_props.@set (worm, properties);

        if (id < game.numhumans)
            update_start_game_action ();
    }

    private int get_raw_key (uint keyval)
    {
        Gdk.KeymapKey[] keys;
        if (Gdk.Display.get_default ().map_keyval (keyval, out keys))
        {
            if (keys.length > 0)
                return (int)keys[0].keycode;
        }
        return -1;
    }

    /*\
    * * Switching the stack
    \*/

    private inline void next_screen_cb ()
    {
        var child_name = main_stack.get_visible_child_name ();
        switch (child_name)
        {
            case "first-run":
                show_new_game_screen (/* after first run */ true);
                break;
            case "number_of_players":
                show_board_progress_screen ();
                break;
            case "board_progress":
                show_speed_screen ();
                break;
            case "speed":
                leave_speed_screen ();
                show_controls_screen ();
                break;
            case "controls":
            default:
                return;
        }
    }

    void set_headerbar_title (string title)
    {
        if (headerbar.get_title_widget () == null)
            headerbar.set_title_widget (
        #if USE_LIBADWAITA
                new Adw.WindowTitle (title, ""));
        #else
                new Label (title));
        #endif
        else
            (
        #if USE_LIBADWAITA
                (Adw.WindowTitle)
        #else
                (Label)
        #endif
                headerbar.get_title_widget ()).
        #if USE_LIBADWAITA
                set_title (title);
        #else
                set_label (title);
        #endif
    }

    private void show_new_game_screen (bool after_first_run = false)
    {
        if (countdown_id != 0)
        {
            Source.remove (countdown_id);
            countdown_id = 0;
        }

        if (game.is_running)
            game.stop ();

        set_headerbar_title (Nibbles.PROGRAM_NAME);

        new_game_action.set_enabled (true);
        pause_action.set_enabled (false);
        back_action.set_enabled (true);

        new_game_button.set_visible (false);
        pause_button.set_visible (false);

        if (after_first_run)
            main_stack.set_transition_type (StackTransitionType.SLIDE_UP);
        else
            main_stack.set_transition_type (StackTransitionType.NONE);
        main_stack.set_visible_child_name ("number_of_players");
        main_stack.set_transition_type (StackTransitionType.SLIDE_UP);
    }

    private void show_speed_screen ()
    {
        int progress, level;
        board_progress.get_values (out progress, out level);
        game.progress = progress;
        if (progress == 2)
        {
            settings.set_int ("start-level", level);
            game.start_level = level;
        }
        settings.set_int ("progress", progress);

        main_stack.set_visible_child_name ("speed");
    }

    private void show_board_progress_screen ()
    {
        int numhumans, numai;
        players.get_values (out numhumans, out numai);
        game.numhumans = numhumans;
        game.numai     = numai;
        settings.set_int ("players", numhumans);
        settings.set_int ("ai",      numai);

        main_stack.set_visible_child_name ("board_progress");
    }

    private void leave_speed_screen ()
    {
        int game_speed;
        bool fakes;
        speed.get_values (out game_speed, out fakes);
        game.speed = game_speed;
        game.fakes = fakes;
        settings.set_int ("speed", game_speed);
        settings.set_boolean ("fakes", fakes);
    }

    private void show_controls_screen ()
    {
        controls.clean ();
        game.create_worms (worm_settings);
        update_start_game_action ();

        controls.prepare (game.worms, game.worm_props, worm_settings);

        main_stack.set_visible_child_name ("controls");
    }

    private void update_start_game_action ()
    {
        GenericSet<uint> keys = new GenericSet<uint> (direct_hash, direct_equal);
        for (int i = 0; i < game.numhumans; i++)
        {
            WormProperties worm_prop = game.worm_props.@get (game.worms.@get (i));
            if (worm_prop.up    == 0
             || worm_prop.down  == 0
             || worm_prop.left  == 0
             || worm_prop.right == 0
             // other keys of the same worm
             || worm_prop.up    == worm_prop.down
             || worm_prop.up    == worm_prop.left
             || worm_prop.up    == worm_prop.right
             || worm_prop.down  == worm_prop.left
             || worm_prop.down  == worm_prop.right
             || worm_prop.right == worm_prop.left
             // keys of already checked worms
             || keys.contains (worm_prop.up)
             || keys.contains (worm_prop.down)
             || keys.contains (worm_prop.left)
             || keys.contains (worm_prop.right))
            {
                start_game_action.set_enabled (false);
                return;
            }
            keys.add (worm_prop.up);
            keys.add (worm_prop.down);
            keys.add (worm_prop.left);
            keys.add (worm_prop.right);
        }
        start_game_action.set_enabled (true);
    }

    private void show_game_view ()
    {
        new_game_button.set_visible (true);
        pause_button.set_visible (true);

        /* Translators: title of the headerbar, while a game is running; the %d is replaced by the level number */
        set_headerbar_title (_("Level %d").printf (game.current_level));
        main_stack.set_visible_child_name ("game_box");

        main_stack.set_transition_type (StackTransitionType.SLIDE_UP);
        statusbar_stack.visible = !fullscreen_active ();
        view.set_visible (true);
    }

    private void back_cb ()
    {
        main_stack.set_transition_type (StackTransitionType.SLIDE_DOWN);

        var child_name = main_stack.get_visible_child_name ();
        switch (child_name)
        {
            case "first-run":
                assert_not_reached ();
            case "number_of_players":
                break;
            case "board_progress":
                main_stack.set_visible_child_name ("number_of_players");
                break;
            case "speed":
                main_stack.set_visible_child_name ("board_progress");
                break;
            case "controls":
                main_stack.set_visible_child_name ("speed");
                break;
            case "game_box":
                new_game_cb ();
                break;
        }

        main_stack.set_transition_type (StackTransitionType.SLIDE_UP);
    }

    /*\
    * * Scoring
    \*/

    string[,] category_descriptions = {
    /* Translators: The game type displayed on the scores dialog */
        {"beginner",               _("Beginner")},
        {"beginner-random",        _("Beginner with random levels")},
        {"beginner-fixed1",        _("Beginner, fixed on level 1")},
        {"beginner-fixed2",        _("Beginner, fixed on level 2")},
        {"beginner-fixed3",        _("Beginner, fixed on level 3")},
        {"beginner-fixed4",        _("Beginner, fixed on level 4")},
        {"beginner-fixed5",        _("Beginner, fixed on level 5")},
        {"beginner-fixed6",        _("Beginner, fixed on level 6")},
        {"beginner-fixed7",        _("Beginner, fixed on level 7")},
        {"beginner-fixed8",        _("Beginner, fixed on level 8")},
        {"beginner-fixed9",        _("Beginner, fixed on level 9")},
        {"beginner-fixed10",       _("Beginner, fixed on level 10")},
        {"beginner-fixed11",       _("Beginner, fixed on level 11")},
        {"beginner-fixed12",       _("Beginner, fixed on level 12")},
        {"beginner-fixed13",       _("Beginner, fixed on level 13")},
        {"beginner-fixed14",       _("Beginner, fixed on level 14")},
        {"beginner-fixed15",       _("Beginner, fixed on level 15")},
        {"beginner-fixed16",       _("Beginner, fixed on level 16")},
        {"beginner-fixed17",       _("Beginner, fixed on level 17")},
        {"beginner-fixed18",       _("Beginner, fixed on level 18")},
        {"beginner-fixed19",       _("Beginner, fixed on level 19")},
        {"beginner-fixed20",       _("Beginner, fixed on level 20")},
        {"beginner-fixed21",       _("Beginner, fixed on level 21")},
        {"beginner-fixed22",       _("Beginner, fixed on level 22")},
        {"beginner-fixed23",       _("Beginner, fixed on level 23")},
        {"beginner-fixed24",       _("Beginner, fixed on level 24")},
        {"beginner-fixed25",       _("Beginner, fixed on level 25")},
        {"beginner-fixed26",       _("Beginner, fixed on level 26")},
        {"beginner-fakes-random",  _("Beginner with fakes and random levels")},
        {"beginner-fakes-fixed1",  _("Beginner with fakes, fixed on level 1")},
        {"beginner-fakes-fixed2",  _("Beginner with fakes, fixed on level 2")},
        {"beginner-fakes-fixed3",  _("Beginner with fakes, fixed on level 3")},
        {"beginner-fakes-fixed4",  _("Beginner with fakes, fixed on level 4")},
        {"beginner-fakes-fixed5",  _("Beginner with fakes, fixed on level 5")},
        {"beginner-fakes-fixed6",  _("Beginner with fakes, fixed on level 6")},
        {"beginner-fakes-fixed7",  _("Beginner with fakes, fixed on level 7")},
        {"beginner-fakes-fixed8",  _("Beginner with fakes, fixed on level 8")},
        {"beginner-fakes-fixed9",  _("Beginner with fakes, fixed on level 9")},
        {"beginner-fakes-fixed10", _("Beginner with fakes, fixed on level 10")},
        {"beginner-fakes-fixed11", _("Beginner with fakes, fixed on level 11")},
        {"beginner-fakes-fixed12", _("Beginner with fakes, fixed on level 12")},
        {"beginner-fakes-fixed13", _("Beginner with fakes, fixed on level 13")},
        {"beginner-fakes-fixed14", _("Beginner with fakes, fixed on level 14")},
        {"beginner-fakes-fixed15", _("Beginner with fakes, fixed on level 15")},
        {"beginner-fakes-fixed16", _("Beginner with fakes, fixed on level 16")},
        {"beginner-fakes-fixed17", _("Beginner with fakes, fixed on level 17")},
        {"beginner-fakes-fixed18", _("Beginner with fakes, fixed on level 18")},
        {"beginner-fakes-fixed19", _("Beginner with fakes, fixed on level 19")},
        {"beginner-fakes-fixed20", _("Beginner with fakes, fixed on level 20")},
        {"beginner-fakes-fixed21", _("Beginner with fakes, fixed on level 21")},
        {"beginner-fakes-fixed22", _("Beginner with fakes, fixed on level 22")},
        {"beginner-fakes-fixed23", _("Beginner with fakes, fixed on level 23")},
        {"beginner-fakes-fixed24", _("Beginner with fakes, fixed on level 24")},
        {"beginner-fakes-fixed25", _("Beginner with fakes, fixed on level 25")},
        {"beginner-fakes-fixed26", _("Beginner with fakes, fixed on level 26")},
        {"slow",               _("Slow")},
        {"slow-random",        _("Slow with random levels")},
        {"slow-fixed1",        _("Slow, fixed on level 1")},
        {"slow-fixed2",        _("Slow, fixed on level 2")},
        {"slow-fixed3",        _("Slow, fixed on level 3")},
        {"slow-fixed4",        _("Slow, fixed on level 4")},
        {"slow-fixed5",        _("Slow, fixed on level 5")},
        {"slow-fixed6",        _("Slow, fixed on level 6")},
        {"slow-fixed7",        _("Slow, fixed on level 7")},
        {"slow-fixed8",        _("Slow, fixed on level 8")},
        {"slow-fixed9",        _("Slow, fixed on level 9")},
        {"slow-fixed10",       _("Slow, fixed on level 10")},
        {"slow-fixed11",       _("Slow, fixed on level 11")},
        {"slow-fixed12",       _("Slow, fixed on level 12")},
        {"slow-fixed13",       _("Slow, fixed on level 13")},
        {"slow-fixed14",       _("Slow, fixed on level 14")},
        {"slow-fixed15",       _("Slow, fixed on level 15")},
        {"slow-fixed16",       _("Slow, fixed on level 16")},
        {"slow-fixed17",       _("Slow, fixed on level 17")},
        {"slow-fixed18",       _("Slow, fixed on level 18")},
        {"slow-fixed19",       _("Slow, fixed on level 19")},
        {"slow-fixed20",       _("Slow, fixed on level 20")},
        {"slow-fixed21",       _("Slow, fixed on level 21")},
        {"slow-fixed22",       _("Slow, fixed on level 22")},
        {"slow-fixed23",       _("Slow, fixed on level 23")},
        {"slow-fixed24",       _("Slow, fixed on level 24")},
        {"slow-fixed25",       _("Slow, fixed on level 25")},
        {"slow-fixed26",       _("Slow, fixed on level 26")},
        {"slow-fakes-random",  _("Slow with fakes and random levels")},
        {"slow-fakes-fixed1",  _("Slow with fakes, fixed on level 1")},
        {"slow-fakes-fixed2",  _("Slow with fakes, fixed on level 2")},
        {"slow-fakes-fixed3",  _("Slow with fakes, fixed on level 3")},
        {"slow-fakes-fixed4",  _("Slow with fakes, fixed on level 4")},
        {"slow-fakes-fixed5",  _("Slow with fakes, fixed on level 5")},
        {"slow-fakes-fixed6",  _("Slow with fakes, fixed on level 6")},
        {"slow-fakes-fixed7",  _("Slow with fakes, fixed on level 7")},
        {"slow-fakes-fixed8",  _("Slow with fakes, fixed on level 8")},
        {"slow-fakes-fixed9",  _("Slow with fakes, fixed on level 9")},
        {"slow-fakes-fixed10", _("Slow with fakes, fixed on level 10")},
        {"slow-fakes-fixed11", _("Slow with fakes, fixed on level 11")},
        {"slow-fakes-fixed12", _("Slow with fakes, fixed on level 12")},
        {"slow-fakes-fixed13", _("Slow with fakes, fixed on level 13")},
        {"slow-fakes-fixed14", _("Slow with fakes, fixed on level 14")},
        {"slow-fakes-fixed15", _("Slow with fakes, fixed on level 15")},
        {"slow-fakes-fixed16", _("Slow with fakes, fixed on level 16")},
        {"slow-fakes-fixed17", _("Slow with fakes, fixed on level 17")},
        {"slow-fakes-fixed18", _("Slow with fakes, fixed on level 18")},
        {"slow-fakes-fixed19", _("Slow with fakes, fixed on level 19")},
        {"slow-fakes-fixed20", _("Slow with fakes, fixed on level 20")},
        {"slow-fakes-fixed21", _("Slow with fakes, fixed on level 21")},
        {"slow-fakes-fixed22", _("Slow with fakes, fixed on level 22")},
        {"slow-fakes-fixed23", _("Slow with fakes, fixed on level 23")},
        {"slow-fakes-fixed24", _("Slow with fakes, fixed on level 24")},
        {"slow-fakes-fixed25", _("Slow with fakes, fixed on level 25")},
        {"slow-fakes-fixed26", _("Slow with fakes, fixed on level 26")},
        {"medium",               _("Medium")},
        {"medium-random",        _("Medium with random levels")},
        {"medium-fixed1",        _("Medium, fixed on level 1")},
        {"medium-fixed2",        _("Medium, fixed on level 2")},
        {"medium-fixed3",        _("Medium, fixed on level 3")},
        {"medium-fixed4",        _("Medium, fixed on level 4")},
        {"medium-fixed5",        _("Medium, fixed on level 5")},
        {"medium-fixed6",        _("Medium, fixed on level 6")},
        {"medium-fixed7",        _("Medium, fixed on level 7")},
        {"medium-fixed8",        _("Medium, fixed on level 8")},
        {"medium-fixed9",        _("Medium, fixed on level 9")},
        {"medium-fixed10",       _("Medium, fixed on level 10")},
        {"medium-fixed11",       _("Medium, fixed on level 11")},
        {"medium-fixed12",       _("Medium, fixed on level 12")},
        {"medium-fixed13",       _("Medium, fixed on level 13")},
        {"medium-fixed14",       _("Medium, fixed on level 14")},
        {"medium-fixed15",       _("Medium, fixed on level 15")},
        {"medium-fixed16",       _("Medium, fixed on level 16")},
        {"medium-fixed17",       _("Medium, fixed on level 17")},
        {"medium-fixed18",       _("Medium, fixed on level 18")},
        {"medium-fixed19",       _("Medium, fixed on level 19")},
        {"medium-fixed20",       _("Medium, fixed on level 20")},
        {"medium-fixed21",       _("Medium, fixed on level 21")},
        {"medium-fixed22",       _("Medium, fixed on level 22")},
        {"medium-fixed23",       _("Medium, fixed on level 23")},
        {"medium-fixed24",       _("Medium, fixed on level 24")},
        {"medium-fixed25",       _("Medium, fixed on level 25")},
        {"medium-fixed26",       _("Medium, fixed on level 26")},
        {"medium-fakes-random",  _("Medium with fakes and random levels")},
        {"medium-fakes-fixed1",  _("Medium with fakes, fixed on level 1")},
        {"medium-fakes-fixed2",  _("Medium with fakes, fixed on level 2")},
        {"medium-fakes-fixed3",  _("Medium with fakes, fixed on level 3")},
        {"medium-fakes-fixed4",  _("Medium with fakes, fixed on level 4")},
        {"medium-fakes-fixed5",  _("Medium with fakes, fixed on level 5")},
        {"medium-fakes-fixed6",  _("Medium with fakes, fixed on level 6")},
        {"medium-fakes-fixed7",  _("Medium with fakes, fixed on level 7")},
        {"medium-fakes-fixed8",  _("Medium with fakes, fixed on level 8")},
        {"medium-fakes-fixed9",  _("Medium with fakes, fixed on level 9")},
        {"medium-fakes-fixed10", _("Medium with fakes, fixed on level 10")},
        {"medium-fakes-fixed11", _("Medium with fakes, fixed on level 11")},
        {"medium-fakes-fixed12", _("Medium with fakes, fixed on level 12")},
        {"medium-fakes-fixed13", _("Medium with fakes, fixed on level 13")},
        {"medium-fakes-fixed14", _("Medium with fakes, fixed on level 14")},
        {"medium-fakes-fixed15", _("Medium with fakes, fixed on level 15")},
        {"medium-fakes-fixed16", _("Medium with fakes, fixed on level 16")},
        {"medium-fakes-fixed17", _("Medium with fakes, fixed on level 17")},
        {"medium-fakes-fixed18", _("Medium with fakes, fixed on level 18")},
        {"medium-fakes-fixed19", _("Medium with fakes, fixed on level 19")},
        {"medium-fakes-fixed20", _("Medium with fakes, fixed on level 20")},
        {"medium-fakes-fixed21", _("Medium with fakes, fixed on level 21")},
        {"medium-fakes-fixed22", _("Medium with fakes, fixed on level 22")},
        {"medium-fakes-fixed23", _("Medium with fakes, fixed on level 23")},
        {"medium-fakes-fixed24", _("Medium with fakes, fixed on level 24")},
        {"medium-fakes-fixed25", _("Medium with fakes, fixed on level 25")},
        {"medium-fakes-fixed26", _("Medium with fakes, fixed on level 26")},
        {"fast",               _("Fast")},
        {"fast-random",        _("Fast with random levels")},
        {"fast-fixed1",        _("Fast, fixed on level 1")},
        {"fast-fixed2",        _("Fast, fixed on level 2")},
        {"fast-fixed3",        _("Fast, fixed on level 3")},
        {"fast-fixed4",        _("Fast, fixed on level 4")},
        {"fast-fixed5",        _("Fast, fixed on level 5")},
        {"fast-fixed6",        _("Fast, fixed on level 6")},
        {"fast-fixed7",        _("Fast, fixed on level 7")},
        {"fast-fixed8",        _("Fast, fixed on level 8")},
        {"fast-fixed9",        _("Fast, fixed on level 9")},
        {"fast-fixed10",       _("Fast, fixed on level 10")},
        {"fast-fixed11",       _("Fast, fixed on level 11")},
        {"fast-fixed12",       _("Fast, fixed on level 12")},
        {"fast-fixed13",       _("Fast, fixed on level 13")},
        {"fast-fixed14",       _("Fast, fixed on level 14")},
        {"fast-fixed15",       _("Fast, fixed on level 15")},
        {"fast-fixed16",       _("Fast, fixed on level 16")},
        {"fast-fixed17",       _("Fast, fixed on level 17")},
        {"fast-fixed18",       _("Fast, fixed on level 18")},
        {"fast-fixed19",       _("Fast, fixed on level 19")},
        {"fast-fixed20",       _("Fast, fixed on level 20")},
        {"fast-fixed21",       _("Fast, fixed on level 21")},
        {"fast-fixed22",       _("Fast, fixed on level 22")},
        {"fast-fixed23",       _("Fast, fixed on level 23")},
        {"fast-fixed24",       _("Fast, fixed on level 24")},
        {"fast-fixed25",       _("Fast, fixed on level 25")},
        {"fast-fixed26",       _("Fast, fixed on level 26")},
        {"fast-fakes-random",  _("Fast with fakes and random levels")},
        {"fast-fakes-fixed1",  _("Fast with fakes, fixed on level 1")},
        {"fast-fakes-fixed2",  _("Fast with fakes, fixed on level 2")},
        {"fast-fakes-fixed3",  _("Fast with fakes, fixed on level 3")},
        {"fast-fakes-fixed4",  _("Fast with fakes, fixed on level 4")},
        {"fast-fakes-fixed5",  _("Fast with fakes, fixed on level 5")},
        {"fast-fakes-fixed6",  _("Fast with fakes, fixed on level 6")},
        {"fast-fakes-fixed7",  _("Fast with fakes, fixed on level 7")},
        {"fast-fakes-fixed8",  _("Fast with fakes, fixed on level 8")},
        {"fast-fakes-fixed9",  _("Fast with fakes, fixed on level 9")},
        {"fast-fakes-fixed10", _("Fast with fakes, fixed on level 10")},
        {"fast-fakes-fixed11", _("Fast with fakes, fixed on level 11")},
        {"fast-fakes-fixed12", _("Fast with fakes, fixed on level 12")},
        {"fast-fakes-fixed13", _("Fast with fakes, fixed on level 13")},
        {"fast-fakes-fixed14", _("Fast with fakes, fixed on level 14")},
        {"fast-fakes-fixed15", _("Fast with fakes, fixed on level 15")},
        {"fast-fakes-fixed16", _("Fast with fakes, fixed on level 16")},
        {"fast-fakes-fixed17", _("Fast with fakes, fixed on level 17")},
        {"fast-fakes-fixed18", _("Fast with fakes, fixed on level 18")},
        {"fast-fakes-fixed19", _("Fast with fakes, fixed on level 19")},
        {"fast-fakes-fixed20", _("Fast with fakes, fixed on level 20")},
        {"fast-fakes-fixed21", _("Fast with fakes, fixed on level 21")},
        {"fast-fakes-fixed22", _("Fast with fakes, fixed on level 22")},
        {"fast-fakes-fixed23", _("Fast with fakes, fixed on level 23")},
        {"fast-fakes-fixed24", _("Fast with fakes, fixed on level 24")},
        {"fast-fakes-fixed25", _("Fast with fakes, fixed on level 25")},
        {"fast-fakes-fixed26", _("Fast with fakes, fixed on level 26")},
    };

    ulong get_category_index (string key)
    {
        ulong i;
        for (i = 0; i < category_descriptions.length [0] && category_descriptions [i, 0] != key; i++);
        return i;
    }

    Games.Scores.Category[] created_categories = {}; /* only accessed by the following function */
    private Games.Scores.Category? get_category (string key)
    {
        foreach (var c in created_categories)
            if (c.key == key)
                return c; /* return an already created category */
        var i = get_category_index (key);
        if (i < category_descriptions.length [0])
        {
            /* create the category requested */
            created_categories += new Games.Scores.Category (category_descriptions [i, 0], category_descriptions [i, 1]);
            return created_categories [created_categories.length - 1];
        }
        else
            return null;
    }

    private void create_scores ()
    {
        scores_context = new Games.Scores.Context (
            "gnome-nibbles",
            /* Translators: label displayed on the scores dialog, preceding a difficulty. */
            _("Difficulty Level:"),
            this,
            get_category,
            Games.Scores.Style.POINTS_GREATER_IS_BETTER,
            "org.gnome.Nibbles",
            (a, b)=>{return get_category_index (a.key) < get_category_index (b.key);});
    }

    private string get_scores_category_key (int speed, bool fakes, int board_progress, int start_level)
    {
        string key;
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
            default:
                key = "beginner";
                break;
        }
        if (fakes)
            key += "-fakes";
        if (board_progress == 1)
            key += "-random";
        else if (board_progress == 2)
            key += "-fixed" + start_level.to_string ();
        return key;
    }

    private void log_score_cb (int score, int level_reached)
    {
        /* Disable these here to prevent the user clicking the buttons before the score is saved */
        new_game_action.set_enabled (false);
        pause_action.set_enabled (false);
        back_action.set_enabled (false);

        var category = get_category (get_scores_category_key (game.speed, game.fakes, game.progress, game.start_level));
        assert (null != category);
        const int max_high_score_count = 10;
        var scores = scores_context.get_high_scores (category, max_high_score_count);
        var lowest_high_score = (scores.size < max_high_score_count ? -1 : scores.last ().score);

        if (game.numhumans < 1)
        {
            game_over (score, lowest_high_score, level_reached);
            return;
        }

        scores_context.add_score.begin (score, category, null, (object, result) =>
        {
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

        scores_context.present_dialog ();

        ulong id = 0;
        id = scores_context.dialog_closed.connect (() => {
            // Be quite careful about whether to unpause. Don't unpause if the game has not started.
            if (should_unpause)
                pause_action.activate (null);

            scores_context.disconnect (id);
        });
    }

    private void level_completed_cb ()
    {
        if (game.progress == 0 && game.current_level == (game.start_level > 1 ? game.start_level - 1 : NibblesGame.MAX_LEVEL) ||
            game.progress == 1 && game.levels_uncompleated.length == 0)
            return;

        statusbar_stack.visible = true;
        view.set_visible (false);
        pause_action.set_enabled (false);
        back_action.set_enabled (false);

        /* Translators: label that appears at the end of a level; the %d is the number of the level that was completed */
        var label = new Label (_("Level %d Completed!").printf (game.current_level));
        label.halign = Align.CENTER;
        label.valign = Align.START;
        label.set_margin_top (150);
        if (label.attributes == null)
            label.attributes = new Pango.AttrList ();
        label.attributes.insert (Pango.attr_scale_new (Pango.Scale.X_LARGE));
        label.attributes.insert (Pango.attr_weight_new (Pango.Weight.BOLD));
        label.set_visible (true);

        /* Translators: label of a button that appears at the end of a level; starts next level */
        var button = new Button.with_label (_("_Next Level"));
        button.set_use_underline (true);
        button.width_request = 116;
        button.height_request = 34;
        button.halign = Align.CENTER;
        button.valign = Align.END;
        button.set_margin_bottom (100);
        button.add_css_class ("suggested-action");
        #if USE_PILL_BUTTON
        button.add_css_class ("pill");
        #else
        button.add_css_class ("play");
        #endif
        button.clicked.connect (()=>
        {
            overlay_remove_all ();

            /* Translators: title of the headerbar, while a game is running; the %d is replaced by the level number */
            set_headerbar_title (_("Level %d").printf (game.current_level));

            view.set_visible (true);

            restart_game ();
        });

        overlay_add (label);
        overlay_add (button);

        button.grab_focus ();

        Timeout.add (500, () => {
            button.set_visible (true);
            //button.grab_default ();

            return Source.REMOVE;
        });
    }

    private bool end_of_game = false;
    private void game_over (int score, long lowest_high_score, int level_reached)
    {
        statusbar_stack.visible = true;

        var is_high_score = (score > lowest_high_score);
        bool is_game_won;
        if (game.progress == 0)
            is_game_won = level_reached > NibblesGame.MAX_LEVEL;
        else if (game.progress == 1)
            is_game_won = game.levels_uncompleated.length == 0;
        else /* game.progress == 2 */
            is_game_won = game.get_game_status () == VICTORY;

        /* Translators: label displayed at the end of a level, if the player finished all the levels */
        var game_over_label = new Label (is_game_won ? _("Congratulations!")

        /* Translators: label displayed at the end of a level, if the player did not finished all the levels */
                                                     : _("Game Over!"));
        game_over_label.halign = Align.CENTER;
        game_over_label.valign = Align.START;
        game_over_label.set_margin_top (150);
        if (game_over_label.attributes == null)
            game_over_label.attributes = new Pango.AttrList ();
        game_over_label.attributes.insert (Pango.attr_scale_new (Pango.Scale.XX_LARGE * 2));
        game_over_label.attributes.insert (Pango.attr_weight_new (Pango.Weight.BOLD));
        game_over_label.set_visible (true);

        /* Translators: label displayed at the end of a level, if the player finished all the levels */
        var msg_label = new Label (_("You have completed the game."));
        msg_label.halign = Align.CENTER;
        msg_label.valign = Align.START;
        msg_label.set_margin_top (get_height () / 3);
        if (msg_label.attributes == null)
            msg_label.attributes = new Pango.AttrList ();
        msg_label.attributes.insert (Pango.attr_scale_new (Pango.Scale.X_LARGE));
        msg_label.attributes.insert (Pango.attr_weight_new (Pango.Weight.BOLD));
        msg_label.set_visible (true);

        var score_string = ngettext ("%d Point", "%d Points", score);
        score_string = score_string.printf (score);
        var score_label = new Label (@"<b>$(score_string)</b>");
        score_label.set_use_markup (true);
        score_label.halign = Align.CENTER;
        score_label.valign = Align.START;
        score_label.set_margin_top (get_height () / 3 + 80);
        score_label.set_visible (true);

        var points_left = lowest_high_score + 1 - score;
        /* Translators: label displayed at the end of a level, if the player did not score enough to have its score saved */
        var points_left_label = new Label (_("(%ld more points to reach the leaderboard)").printf (points_left));
        points_left_label.halign = Align.CENTER;
        points_left_label.valign = Align.START;
        points_left_label.set_margin_top (get_height () / 3 + 100);
        points_left_label.set_visible (true);

        /* Translators: label of a button displayed at the end of a level; restarts the game */
        var play_again_button = new Button.with_label (_("_Play Again"));
        play_again_button.set_use_underline (true);
        play_again_button.halign = Align.CENTER;
        play_again_button.valign = Align.END;
        play_again_button.set_margin_bottom (100);
        play_again_button.set_action_name ("win.new-game");
        play_again_button.add_css_class ("suggested-action");
        #if USE_PILL_BUTTON
        play_again_button.add_css_class ("pill");
        #endif
        play_again_button.set_visible (true);

        overlay_add (game_over_label);
        if (is_game_won)
            overlay_add (msg_label);
        if (game.numhumans == 1)
            overlay_add (score_label);
        if (game.numhumans == 1 && !is_high_score)
            overlay_add (points_left_label);
        overlay_add (play_again_button);

        play_again_button.grab_focus ();

        view.set_visible (false);
        end_of_game = true;
        new_game_action.set_enabled (true);
        pause_action.set_enabled (false);
        back_action.set_enabled (false);
    }

    /*\
    * * Delegates
    \*/

    int countdown_active ()
    {
        return seconds;
    }
    
    bool fullscreen_active ()
    {
        return fullscreen_mode;
    }
}

#if USE_LIBADWAITA
[GtkTemplate (ui = "/org/gnome/Nibbles/ui/first-run-adw.ui")]
#else
[GtkTemplate (ui = "/org/gnome/Nibbles/ui/first-run.ui")]
#endif
private class FirstRun : Box
{
    [GtkChild] private unowned Button button;
    construct
    {
        #if USE_PILL_BUTTON
        if (button.has_css_class ("play"))
        {
            button.remove_css_class ("play");
            button.add_css_class ("pill");
        }
        #else
        button.has_css_class ("play");
        #endif
    }
}

