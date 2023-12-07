/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Gnome Nibbles: Gnome Worm Game
 * Copyright (C) 2015 Iulian-Gabriel Radu <iulian.radu67@gmail.com>
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

[GtkTemplate (ui = "/org/gnome/Nibbles/ui/nibbles.ui")]
private class NibblesWindow : ApplicationWindow
{
    /* Application and worm settings */
    private GLib.Settings settings;
    private Gee.ArrayList<GLib.Settings> worm_settings;

    /* Main widgets */
    [GtkChild] private unowned Stack main_stack;
    [GtkChild] private unowned Overlay overlay;

    /* HeaderBar */
    [GtkChild] private unowned HeaderBar headerbar;
    [GtkChild] private unowned Button new_game_button;
    [GtkChild] private unowned Button pause_button;
    [GtkChild] private unowned MenuButton hamburger_menu;

    /* Pre-game screen widgets */
    [GtkChild] private unowned Players players;
    [GtkChild] private unowned Speed speed;
    [GtkChild] private unowned Controls controls;

    /* Statusbar widgets */
    [GtkChild] private unowned Stack statusbar_stack;
    [GtkChild] private unowned Scoreboard scoreboard;
    private Image scoreboard_life;

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
            
            public KeypressHandlerFunction @get ()
            {
                return (KeypressHandlerFunction)(pIterator.keypress_handler);
            }
        }

        struct Node
        {
            KeypressHandlerFunction keypress_handler; /* to do, circumnavigate compiler warning message */
            Node? pNext;
        }
        Node? pHead = null;

        internal void push (KeypressHandlerFunction handler)
        {
            if (pHead == null)
                pHead = { (KeypressHandlerFunction)handler, null};
            else
                pHead = { (KeypressHandlerFunction)handler, pHead};
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
    public  SetupScreen start_screen { private get; internal construct; }
    public bool game_paused 
    {
        get {return game != null && game.paused;}
        private set {}
    }

    /* Used for handling the game's scores */
    private Games.Scores.Context scores_context;
    private Gee.LinkedList<Games.Scores.Category> scorecats;

    /* HeaderBar actions */
    private SimpleAction new_game_action;
    private SimpleAction pause_action;
    private SimpleAction back_action;
    private SimpleAction start_game_action;

    /* count down variables */
    private uint countdown_id = 0;
    private const int COUNTDOWN_TIME = 3;
    private int seconds = 0;

    bool show_dialogue = false;

    private const GLib.ActionEntry menu_entries[] =
    {
        { "new-game",       new_game_cb     },  // the "New Game" button (during game), or the ctrl-N shortcut (mostly all the time)
        { "pause",          pause_cb        },
        { "scores",         scores_cb       },

        { "next-screen",    next_screen_cb  },  // called from first-run, players and speed
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
        new_game_action     = (SimpleAction) lookup_action ("new-game");
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
        view = new NibblesView (game,
                                countdown_active,
                                new_game_dialogue_active);
        view.show ();
        view.vexpand = true;
        game_box.prepend (view);
        set_focus (view);

        /* Create scoreboard */
        /* to do, bring this image in to the code, its the last image we load with load_image_file */
        scoreboard_life = NibblesView.load_image_file ("scoreboard-life.svg", 14, 14);

        /* Number of worms */
        game.numhumans = settings.get_int ("players");
        int numai = settings.get_int ("ai");
        if (numai + game.numhumans > NibblesGame.MAX_WORMS)
        {
            assert_not_reached ();
        }
        game.numai = numai;
        // NOTE: set numai value to 0 here
        players.set_values (game.numhumans, numai);

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
            first_run_panel.show ();
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

        if (seconds == 0)
        {
            statusbar_stack.set_visible_child_name ("scoreboard");

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
            scoreboard.register (worm, NibblesView.colorval_name_untranslated (color), scoreboard_life);
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

        countdown_id = Timeout.add_seconds (1, countdown_cb);
    }

    private void restart_game ()
    {
        game.new_level (game.current_level);

        game.add_worms ();
        start_game_with_countdown ();
    }

    private void new_game_cb ()
    {
        var child_name = main_stack.get_visible_child_name ();
        switch (child_name)
        {
            case "first-run":
            case "number_of_players":
            case "speed":
                next_screen_cb ();
                break;
            case "controls":
                start_game ();
                break;
            case "game_box":
                overlay_remove_all ();
                if (end_of_game)    // TODO better
                {
                    view.show ();
                    end_of_game = false;

                    show_new_game_screen ();
                }
                else
                    show_new_game_dialog ();
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
        dialog.show ();
    }

    bool new_game_dialogue_active (out YesNoResultFunction result_function)
    {
        result_function = (yes_no)=>
        {
            show_dialogue = false;
            view.redraw ();

            if (yes_no == 0) /* yes */
                show_new_game_screen ();
            else /* no */
            {
                if (seconds == 0)
                    game.start (/* add initial bonus */ false);
                else
                    countdown_id = Timeout.add_seconds (1, countdown_cb);
            }
        };
        return show_dialogue;
    }

    private void pause_cb ()
    {
        if (game != null)
        {
            game.paused = game.is_running;
            set_pause_button_label (game.paused);
            if (game.paused)
                statusbar_stack.set_visible_child_name ("paused");
            else
            {
                statusbar_stack.set_visible_child_name ("scoreboard");
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
            pause_button.set_label (_("_Pause"));   // duplicated in nibbles.ui
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
            headerbar.set_title_widget (new Label (title));
        else
            ((Label)headerbar.get_title_widget ()).set_label (title);
    }

    private void    show_new_game_screen (bool after_first_run = false)
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

        new_game_button.hide ();
        pause_button.hide ();

        if (after_first_run)
            main_stack.set_transition_type (StackTransitionType.SLIDE_UP);
        else
            main_stack.set_transition_type (StackTransitionType.NONE);
        main_stack.set_visible_child_name ("number_of_players");
        main_stack.set_transition_type (StackTransitionType.SLIDE_UP);
    }

    private void show_speed_screen ()
    {
        int numhumans, numai;
        players.get_values (out numhumans, out numai);
        game.numhumans = numhumans;
        game.numai     = numai;
        settings.set_int ("players", numhumans);
        settings.set_int ("ai",      numai);

        main_stack.set_visible_child_name ("speed");
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
        /* FIXME: If there's a transition set, on Wayland, the ClutterEmbed
         * will show outside the game's window. Don't change the transition
         * type when that's no longer a problem.
         */
        //main_stack.set_transition_type (StackTransitionType.NONE);
        new_game_button.show ();
        pause_button.show ();

        /* Translators: title of the headerbar, while a game is running; the %d is replaced by the level number */
        set_headerbar_title (_("Level %d").printf (game.current_level));
        main_stack.set_visible_child_name ("game_box");

        main_stack.set_transition_type (StackTransitionType.SLIDE_UP);
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
            case "speed":
                main_stack.set_visible_child_name ("number_of_players");
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

        scores_context = new Games.Scores.Context.with_importer_and_icon_name (
            "gnome-nibbles",
            /* Translators: label displayed on the scores dialog, preceding a difficulty. */
            _("Difficulty Level:"),
            this,
            category_request,
            Games.Scores.Style.POINTS_GREATER_IS_BETTER,
            new Games.Scores.DirectoryImporter.with_convert_func (get_new_scores_key),
            "org.gnome.Nibbles");
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
        back_action.set_enabled (false);

        var scores = scores_context.get_high_scores (get_scores_category (game.speed, game.fakes));
        var lowest_high_score = (scores.size == 10 ? scores.last ().score : -1);

        if (game.numhumans != 1)
        {
            game_over (score, lowest_high_score, level_reached);
            return;
        }

        if (game.skip_score)
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
        label.show ();

        /* Translators: label of a button that appears at the end of a level; starts next level */
        var button = new Button.with_label (_("_Next Level"));
        button.set_use_underline (true);
        button.halign = Align.CENTER;
        button.valign = Align.END;
        button.set_margin_bottom (100);
        button.clicked.connect (()=>
        {
            overlay_remove_all ();

            /* Translators: title of the headerbar, while a game is running; the %d is replaced by the level number */
            set_headerbar_title (_("Level %d").printf (game.current_level));

            view.show ();

            restart_game ();
        });

        overlay_add (label);
        overlay_add (button);

        button.grab_focus ();

        Timeout.add (500, () => {
            button.show ();
            //button.grab_default ();

            return Source.REMOVE;
        });
    }

    private bool end_of_game = false;
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
        if (game_over_label.attributes == null)
            game_over_label.attributes = new Pango.AttrList ();
        game_over_label.attributes.insert (Pango.attr_scale_new (Pango.Scale.X_LARGE));
        game_over_label.attributes.insert (Pango.attr_weight_new (Pango.Weight.BOLD));

        game_over_label.show ();

        /* Translators: label displayed at the end of a level, if the player finished all the levels */
        var msg_label = new Label (_("You have completed the game."));
        msg_label.halign = Align.CENTER;
        msg_label.valign = Align.START;
        msg_label.set_margin_top (get_height () / 3);
        if (msg_label.attributes == null)
            msg_label.attributes = new Pango.AttrList ();
        msg_label.attributes.insert (Pango.attr_scale_new (Pango.Scale.X_LARGE));
        msg_label.attributes.insert (Pango.attr_weight_new (Pango.Weight.BOLD));
        msg_label.show ();

        var score_string = ngettext ("%d Point", "%d Points", score);
        score_string = score_string.printf (score);
        var score_label = new Label (@"<b>$(score_string)</b>");
        score_label.set_use_markup (true);
        score_label.halign = Align.CENTER;
        score_label.valign = Align.START;
        score_label.set_margin_top (get_height () / 3 + 80);
        score_label.show ();

        var points_left = lowest_high_score - score;
        /* Translators: label displayed at the end of a level, if the player did not score enough to have its score saved */
        var points_left_label = new Label (_("(%ld more points to reach the leaderboard)").printf (points_left));
        points_left_label.halign = Align.CENTER;
        points_left_label.valign = Align.START;
        points_left_label.set_margin_top (get_height () / 3 + 100);
        points_left_label.show ();

        /* Translators: label of a button displayed at the end of a level; restarts the game */
        var play_again_button = new Button.with_label (_("_Play Again"));
        play_again_button.set_use_underline (true);
        play_again_button.halign = Align.CENTER;
        play_again_button.valign = Align.END;
        play_again_button.set_margin_bottom (100);
        play_again_button.set_action_name ("win.new-game");
        play_again_button.show ();

        overlay_add (game_over_label);
        if (is_game_won)
            overlay_add (msg_label);
        if (game.numhumans == 1)
            overlay_add (score_label);
        if (game.numhumans == 1 && !is_high_score)
            overlay_add (points_left_label);
        overlay_add (play_again_button);

        play_again_button.grab_focus ();

        view.hide ();
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
}

[GtkTemplate (ui = "/org/gnome/Nibbles/ui/first-run.ui")]
private class FirstRun : Box
{
}

