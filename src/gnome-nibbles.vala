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
    private Gtk.Box game_box;
    private Games.GridFrame frame;
    private Gtk.Stack statusbar_stack;
    private Gtk.Label countdown;
    private Scoreboard scoreboard;
    private Gdk.Pixbuf scoreboard_life;
    private Gee.LinkedList<Gtk.ToggleButton> number_of_players_buttons;
    private Gtk.Revealer next_button_revealer;

    /* Controls screen grids and pixbufs */
    private Gtk.Box grids_box;
    private Gee.LinkedList<ControlsGrid> controls_grids;
    private Gdk.Pixbuf arrow_pixbuf;
    private Gdk.Pixbuf arrow_key_pixbuf;

    /* Used for handling the game's scores */
    private Games.Scores.Context scores_context;
    private Gee.LinkedList<Games.Scores.Category> scorecats;

    private NibblesView? view;
    private NibblesGame? game = null;

    private const int COUNTDOWN_TIME = 3;

    private const ActionEntry action_entries[] =
    {
        {"start-game", start_game_cb},
        {"scores", scores_cb},
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
        stderr.printf("[Debug] Startup\n");
        base.startup ();

        var css_provider = new Gtk.CssProvider ();
        css_provider.load_from_resource ("/org/gnome/nibbles/ui/nibbles.css");
        Gtk.StyleContext.add_provider_for_screen (Gdk.Screen.get_default (), css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION);

        add_action_entries (action_entries, this);
        add_action_entries (menu_entries, this);

        settings = new Settings ("org.gnome.nibbles");
        worm_settings = new Gee.ArrayList<Settings> ();
        for (int i = 0; i < NibblesGame.NUMHUMANS; i++)
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
        game_box = (Gtk.Box) builder.get_object ("game_box");
        statusbar_stack = (Gtk.Stack) builder.get_object ("statusbar_stack");
        countdown = (Gtk.Label) builder.get_object ("countdown");
        number_of_players_buttons = new Gee.LinkedList<Gtk.ToggleButton> ();
        for (int i = 0; i < NibblesGame.NUMHUMANS; i++)
        {
            var button = (Gtk.ToggleButton) builder.get_object ("players%d".printf (i + 1));
            button.toggled.connect (change_number_of_players_cb);
            number_of_players_buttons.add (button);
        }
        next_button_revealer = (Gtk.Revealer) builder.get_object ("next_button_revealer");
        grids_box = (Gtk.Box) builder.get_object ("grids_box");
        window.set_titlebar (headerbar);

        add_window (window);

        /* Create game */
        game = new NibblesGame (settings);
        game.log_score.connect (log_score_cb);
        game.restart_game.connect (restart_game_cb);
        game.level_completed.connect (level_completed_cb);

        view = new NibblesView (game);
        view.configure_event.connect (configure_event_cb);

        frame = new Games.GridFrame (NibblesGame.WIDTH, NibblesGame.HEIGHT);
        game_box.pack_start (frame);

        scoreboard = new Scoreboard ();
        scoreboard_life = view.load_pixmap_file ("scoreboard-life.svg", 2 * game.tile_size, 2 * game.tile_size);
        scoreboard.show ();
        statusbar_stack.add_named (scoreboard, "scoreboard");

        frame.add (view);
        frame.show ();

        /* Controls screen */
        controls_grids = new Gee.LinkedList<ControlsGrid> ();
        arrow_pixbuf = view.load_pixmap_file ("arrow.svg", 5 * game.tile_size, 5 * game.tile_size);
        arrow_key_pixbuf = view.load_pixmap_file ("arrow-key.svg", 5 * game.tile_size, 5 * game.tile_size);

        /* Check wether to display the first run screen */
        var first_run = settings.get_boolean ("first-run");
        if (first_run)
            show_first_run_screen ();
        else
            show_new_game_screen_cb ();

        window.show_all ();

        create_scores ();
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

        game.current_level = game.start_level;

        view.new_level (game.current_level);
        view.create_name_labels ();
        view.connect_worm_signals ();

        foreach (var worm in game.worms)
        {
            var color = game.worm_props.get (worm).color;
            scoreboard.register (worm, NibblesView.colorval_name (color), scoreboard_life);
            worm.notify["lives"].connect (scoreboard.update);
            worm.notify["score"].connect (scoreboard.update);

            var actors = view.worm_actors.get (worm);
            if (actors.get_stage () == null)
                view.stage.add_child (actors);

            actors.show ();
        }
        game.add_worms ();
        show_game_view ();

        start_game_with_countdown ();
    }

    public void start_game_with_countdown ()
    {
        statusbar_stack.set_visible_child_name ("countdown");

        var seconds = COUNTDOWN_TIME;
        view.name_labels.show ();
        Timeout.add (1000, () => {
            countdown.set_label (seconds.to_string ());
            if (seconds == 0)
            {
                statusbar_stack.set_visible_child_name ("scoreboard");
                view.name_labels.hide ();
                countdown.set_label (COUNTDOWN_TIME.to_string ());
                game.start ();
                return Source.REMOVE;
            }
            seconds--;
            return Source.CONTINUE;
        });
    }

    private void restart_game_cb ()
    {
        view.new_level (game.current_level);

        foreach (var worm in game.worms)
        {
            var actors = view.worm_actors.get (worm);
            if (actors.get_stage () == null) {
                view.stage.add_child (actors);
            }
            actors.show ();
        }

        game.add_worms ();
        start_game_with_countdown ();
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
        /* Save selected number of players before changing the screen */
        foreach (var button in number_of_players_buttons)
        {
            if (button.get_active ())
            {
                var label = button.get_label ();
                game.numhumans = int.parse (label.replace ("_", ""));
            }
        }

        /* Create worms and load properties */
        game.create_worms ();
        game.load_worm_properties (worm_settings);

        foreach (var worm in game.worms)
        {
            if (worm.is_human)
            {
                var grid = new ControlsGrid (worm.id, game.worm_props.get (worm), arrow_pixbuf, arrow_key_pixbuf);
                grids_box.add (grid);
                controls_grids.add (grid);
            }
        }

        main_stack.set_visible_child_name ("controls");
    }

    private void show_game_view ()
    {
        main_stack.set_visible_child_name ("game_box");
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

    /*\
    * * Scoring
    \*/

    public void create_scores ()
    {
        scores_context = new Games.Scores.Context ("gnome-nibbles", "", window, Games.Scores.Style.PLAIN_DESCENDING);

        scorecats = new Gee.LinkedList<Games.Scores.Category> ();
        scorecats.add (new Games.Scores.Category ("beginner", "Beginner"));
        scorecats.add (new Games.Scores.Category ("slow", "Slow"));
        scorecats.add (new Games.Scores.Category ("medium", "Medium"));
        scorecats.add (new Games.Scores.Category ("fast", "Fast"));
        scorecats.add (new Games.Scores.Category ("beginner-fakes", "Beginner with Fakes"));
        scorecats.add (new Games.Scores.Category ("slow-fakes", "Slow with Fakes"));
        scorecats.add (new Games.Scores.Category ("medium-fakes", "Medium with Fakes"));
        scorecats.add (new Games.Scores.Category ("fast-fakes", "Fast with Fakes"));

        scores_context.category_request.connect ((s, key) => {

            foreach (var cat in scorecats)
            {
                if (key == cat.key)
                    return cat;
            }
            return null;
        });
    }

    public Games.Scores.Category get_scores_category (int speed, bool fakes)
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

    public void log_score_cb (int score)
    {
        if (game.numhumans != 1)
            return;

        if (game.start_level != 1)
            return;

        if (score <= 0)
            return;

        try
        {
            if (scores_context.add_score (score, get_scores_category (game.speed, game.fakes)))
                scores_context.run_dialog ();
            else
                game_over_cb ();
        }
        catch (GLib.Error e)
        {
            // Translators: This warning is displayed when adding a score fails
            // just before displaying the score dialog
            warning (_("Failed to add score: %s"), e.message);
        }
    }

    private void scores_cb ()
    {
        try
        {
            scores_context.run_dialog ();
        }
        catch (GLib.Error e)
        {
            // Translators: This error is displayed when the scores dialog fails to load
            error (_("Failed to run scores dialog: %s"), e.message);
        }
    }

    public void level_completed_cb ()
    {
        // TODO: Fix extremely big title bar
        var dialog = new Gtk.MessageDialog (window,
                                            Gtk.DialogFlags.MODAL,
                                            Gtk.MessageType.INFO,
                                            Gtk.ButtonsType.NONE,
                                            _("Level %d Completed!").printf (game.current_level));

        dialog.add_button (_("Next level"), Gtk.ResponseType.OK);
        dialog.response.connect ((response_id) => {
            if (response_id == Gtk.ResponseType.OK)
                restart_game_cb ();

            dialog.destroy ();
        });

        dialog.show ();
    }

    public void game_over_cb ()
    {
        var dialog = new Gtk.MessageDialog (window,
                                            Gtk.DialogFlags.MODAL,
                                            Gtk.MessageType.INFO,
                                            Gtk.ButtonsType.NONE,
                                            _("Game Over!").printf (game.current_level));

        dialog.add_button (_("_Quit"), Gtk.ResponseType.CLOSE);
        dialog.add_button (_("_Play Again"), Gtk.ResponseType.OK);
        dialog.response.connect ((response_id) => {
            if (response_id == Gtk.ResponseType.OK)
                show_new_game_screen_cb ();
            if (response_id == Gtk.ResponseType.CLOSE)
                quit ();

            dialog.destroy ();
        });

        dialog.show ();
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

        Gtk.Window.set_default_icon_name ("gnome-nibbles");

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

[GtkTemplate (ui = "/org/gnome/nibbles/ui/scoreboard.ui")]
public class Scoreboard : Gtk.Box
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

        var box = new PlayerScoreBox ("Worm %d".printf (worm.id + 1), color, worm.score, worm.lives, life_pixbuf);
        boxes.set (box, worm);
        add (box);
    }

    public void update ()
    {
        foreach (var entry in boxes.entries)
        {
            entry.key.update (entry.value.score, entry.value.lives);
        }
    }
}

[GtkTemplate (ui = "/org/gnome/nibbles/ui/player-score-box.ui")]
public class PlayerScoreBox : Gtk.Box
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

        for (int i = 0; i < lives_left; i++)
        {
            var life = new Gtk.Image.from_pixbuf (life_pixbuf);
            life.show ();

            life_images.add (life);
            lives_grid.attach (life, i % 6, i/6);
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
        for (int i = life_images.size; i > lives_left; i--)
        {
            var life = life_images.poll ();
            life.hide ();
        }

        /* Add new lives - if any */
        for (int i = life_images.size; i < lives_left; i++)
        {
            var life = new Gtk.Image.from_pixbuf (life_images.first ().get_pixbuf ());
            life.show ();

            life_images.add (life);
            lives_grid.attach (life, i % 6, i/6);
        }
    }
}

[GtkTemplate (ui = "/org/gnome/nibbles/ui/controls-grid.ui")]
public class ControlsGrid : Gtk.Grid
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

        name_label.set_markup (@"<b><span color=\"$(color.to_string ())\">Player $(worm_id + 1)</span></b>");

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
