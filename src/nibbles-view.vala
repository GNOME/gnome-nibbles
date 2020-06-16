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

private class WormView : Object
{
    internal List<Widget> widgets = new List<Widget> ();

    internal void set_opacity (uint8 new_opacity)
    {
        foreach (Widget widget in widgets)
            widget.set_opacity (new_opacity);
    }

    internal void unparent ()
    {
        foreach (Widget widget in widgets)
            widget.unparent ();
    }
//    protected override void show ()
//    {
//        base.show ();

//        set_opacity (0);
//        set_scale (3.0, 3.0);

//        save_easing_state ();
//        set_easing_mode (Clutter.AnimationMode.EASE_OUT_CIRC);
//        set_easing_duration (NibblesGame.GAMEDELAY * 26);
//        set_scale (1.0, 1.0);
//        set_pivot_point (0.5f, 0.5f);
//        set_opacity (0xff);
//        restore_easing_state ();
//    }

    internal void hide ()
    {
        foreach (Widget widget in widgets)
            widget.hide ();

//        save_easing_state ();
//        set_easing_mode (Clutter.AnimationMode.EASE_IN_QUAD);
//        set_easing_duration (NibblesGame.GAMEDELAY * 15);
//        set_scale (0.4f, 0.4f);
//        set_opacity (0);
//        restore_easing_state ();
    }
}

private class NibblesView : Widget
{
    private const int MINIMUM_TILE_SIZE = 7;
    public int tile_size { internal get; protected construct set; }

    /* Pixmaps */
    private Gdk.Pixbuf wall_pixmaps[12];
    private Gdk.Pixbuf worm_pixmaps[6];
    private Gdk.Pixbuf boni_pixmaps[9];

    /* Actors */
//    private Clutter.Stage stage;
//    private Clutter.Actor level;
//    internal Clutter.Actor name_labels { get; private set; }

    private Gee.HashMap<Worm,  WormView>    worm_actors  = new Gee.HashMap<Worm,  WormView> ();
    private Gee.HashMap<Bonus, Image>       bonus_actors = new Gee.HashMap<Bonus, Image> ();
    private Gee.HashSet<Image>              warp_actors  = new Gee.HashSet<Image> ();

    private GridLayout layout;

    /* Game being played */
    private NibblesGame _game;
    public NibblesGame game
    {
        internal get { return _game; }
        protected construct
        {
            if (_game != null)
                SignalHandler.disconnect_matched (_game, SignalMatchType.DATA, 0, 0, null, null, this);

            _game = value;
            _game.bonus_added.connect (bonus_added_cb);
            _game.bonus_removed.connect (bonus_removed_cb);

            _game.bonus_applied.connect (bonus_applied_cb);

            _game.warp_added.connect (warp_added_cb);

            _game.animate_end_game.connect (animate_end_game_cb);
        }
    }

    construct
    {
        layout = new GridLayout ();
        set_layout_manager (layout);
    }

    internal NibblesView (NibblesGame game, int tile_size, bool is_muted)
    {
        Object (game: game, tile_size: tile_size, is_muted: is_muted);

//        stage = (Clutter.Stage) get_stage ();
//        Clutter.Color stage_color = { 0x00, 0x00, 0x00, 0xff };
//        stage.set_background_color (stage_color);

        set_size_request (MINIMUM_TILE_SIZE * NibblesGame.WIDTH,
                          MINIMUM_TILE_SIZE * NibblesGame.HEIGHT);

        load_pixmap ();
    }

//    protected override bool configure_event (Gdk.EventConfigure event)
//    {
//        int new_tile_size, ts_x, ts_y;

//        /* Compute the new tile size based on the size of the
//         * drawing area, rounded down.
//         */
//        ts_x = event.width / NibblesGame.WIDTH;
//        ts_y = event.height / NibblesGame.HEIGHT;
//        if (ts_x * NibblesGame.WIDTH > event.width)
//            ts_x--;
//        if (ts_y * NibblesGame.HEIGHT > event.height)
//            ts_y--;
//        new_tile_size = int.min (ts_x, ts_y);

//        if (new_tile_size == 0 || tile_size == 0)
//            return true;

//        if (tile_size != new_tile_size)
//        {
//            get_stage ().set_size (new_tile_size * NibblesGame.WIDTH, new_tile_size * NibblesGame.HEIGHT);

//            board_rescale (new_tile_size);
//            boni_rescale  (new_tile_size);
//            warps_rescale (new_tile_size);
//            foreach (var worm in game.worms)
//                worm.rescaled (new_tile_size);

//            tile_size = new_tile_size;
//        }

//        return false;
//    }

    /*\
    * * Level creation and loading
    \*/

    internal void new_level (int level_id)
    {
        string level_name = "level%03d.gnl".printf (level_id);
        string filename = Path.build_filename (PKGDATADIR, "levels", level_name, null);

        FileStream file;
        if ((file = FileStream.open (filename, "r")) == null)
            error ("Nibbles couldn't find pixmap file: %s", filename);

        foreach (var actor in worm_actors.values)
            actor.unparent ();
        worm_actors.clear ();

        foreach (var actor in bonus_actors.values)
            actor.unparent ();
        bonus_actors.clear ();

        foreach (var actor in warp_actors)
            actor.unparent ();
        warp_actors.clear ();

//        if (level != null)
//        {
//            level.remove_all_children ();
//            stage.remove_child (level);
//        }
//        level = new Clutter.Actor ();

        string? line;
        string [] board = {};
        while ((line = file.read_line ()) != null)
            board += (!) line;
        if (!game.load_board (board))
            error ("Level file appears to be damaged: %s", filename);

        foreach (Worm worm in game.worms)
        {
            var actors = new WormView ();
//            stage.add_child (actors);
            worm_actors.@set (worm, actors);
        }

        /* Load wall_pixmaps onto the surface */
        int x_pos, y_pos;
        Image? tmp;
        for (int i = 0; i < NibblesGame.HEIGHT; i++)
        {
            y_pos = i * tile_size;
            for (int j = 0; j < NibblesGame.WIDTH; j++)
            {
                tmp = null;
                try
                {
                    switch (game.board[j, i])
                    {
                        case 'a':   // the most common thing on top in the switch
                            tmp = new Image.from_pixbuf (wall_pixmaps[11]);
                            break;

                        case 'b': // straight up
                            tmp = new Image.from_pixbuf (wall_pixmaps[0]);
                            break;
                        case 'c': // straight side
                            tmp = new Image.from_pixbuf (wall_pixmaps[1]);
                            break;
                        case 'd': // corner bottom left
                            tmp = new Image.from_pixbuf (wall_pixmaps[2]);
                            break;
                        case 'e': // corner bottom right
                            tmp = new Image.from_pixbuf (wall_pixmaps[3]);
                            break;
                        case 'f': // corner up left
                            tmp = new Image.from_pixbuf (wall_pixmaps[4]);
                            break;
                        case 'g': // corner up right
                            tmp = new Image.from_pixbuf (wall_pixmaps[5]);
                            break;
                        case 'h': // tee up
                            tmp = new Image.from_pixbuf (wall_pixmaps[6]);
                            break;
                        case 'i': // tee right
                            tmp = new Image.from_pixbuf (wall_pixmaps[7]);
                            break;
                        case 'j': // tee left
                            tmp = new Image.from_pixbuf (wall_pixmaps[8]);
                            break;
                        case 'k': // tee down
                            tmp = new Image.from_pixbuf (wall_pixmaps[9]);
                            break;
                        case 'l': // tee cross
                            tmp = new Image.from_pixbuf (wall_pixmaps[10]);
                            break;

                        case 'r': // should have been repleced by NibblesGame.EMPTYCHAR
                        case 's':
                        case 't':
                        case 'u':
                        case 'v':
                        case 'w':
                        case 'x':
                        case 'y':
                        case 'z':
                        case '.': // empty space in files, replaced by an 'a'
                            assert_not_reached ();

                        case 'Q':
                        case 'R':
                        case 'S':
                        case 'T':
                        case 'U':
                        case 'V':
                        case 'W':
                        case 'X':
                        case 'Y':
                        case 'Z':
                        default:
                            break;
                    }
                }
                catch (Error e)
                {
                    error ("Error loading level: %s", e.message);
                }

                if (tmp != null)
                {
                    ((!) tmp).pixel_size = tile_size;
                    ((!) tmp).insert_after (this, /* insert first */ null);
                    GridLayoutChild child_layout = (GridLayoutChild) layout.get_layout_child ((!) tmp);
                    child_layout.set_left_attach (j);
                    child_layout.set_top_attach (i);
                }
            }
        }
//        stage.add_child (level);

//        level.set_opacity (0);
//        level.set_scale (0.2, 0.2);

//        level.save_easing_state ();
//        level.set_easing_mode (Clutter.AnimationMode.EASE_OUT_BOUNCE);
//        level.set_easing_duration (NibblesGame.GAMEDELAY * NibblesGame.GAMEDELAY);
//        level.set_scale (1.0, 1.0);
//        level.set_pivot_point (0.5f, 0.5f);
//        level.set_opacity (0xff);
//        level.restore_easing_state ();
    }

    /*\
    * * Pixmaps loading
    \*/

    internal static Gdk.Pixbuf load_pixmap_file (string pixmap, int xsize, int ysize)
    {
        var filename = Path.build_filename (PKGDATADIR, "pixmaps", pixmap, null);
        if (filename == null)
            error ("Nibbles couldn't find pixmap file: %s", filename);

        Gdk.Pixbuf image = null;
        try
        {
            image = new Gdk.Pixbuf.from_file_at_scale (filename, xsize, ysize, true);
        }
        catch (Error e)
        {
            warning ("Failed to load pixmap file: %s", e.message);
        }

        return image;
    }

    private void load_pixmap ()
    {
        string[] bonus_files =
        {
            "bonus1.svg",
            "bonus2.svg",
            "bonus3.svg",
            "life.svg",
            "diamond.svg",
            "questionmark.svg"
        };

        string[] small_files =
        {
            "wall-straight-up.svg",
            "wall-straight-side.svg",
            "wall-corner-bottom-left.svg",
            "wall-corner-bottom-right.svg",
            "wall-corner-top-left.svg",
            "wall-corner-top-right.svg",
            "wall-tee-up.svg",
            "wall-tee-right.svg",
            "wall-tee-left.svg",
            "wall-tee-down.svg",
            "wall-cross.svg",
            "empty.svg"
        };

        string[] worm_files =
        {
            "snake-red.svg",
            "snake-green.svg",
            "snake-blue.svg",
            "snake-yellow.svg",
            "snake-cyan.svg",
            "snake-magenta.svg"
        };

        for (int i = 0; i < bonus_files.length; i++)
        {
            boni_pixmaps[i] = load_pixmap_file (bonus_files[i],
                                                2 * tile_size, 2 * tile_size);
        }

        for (int i = 0; i < small_files.length; i++)
        {
            wall_pixmaps[i] = load_pixmap_file (small_files[i],
                                                2 * tile_size, 2 * tile_size);
        }

        for (int i = 0; i < worm_files.length; i++)
        {
            worm_pixmaps[i] = load_pixmap_file (worm_files[i],
                                                tile_size, tile_size);
        }
    }

    internal void connect_worm_signals ()
    {
        foreach (var worm in game.worms)
        {
            worm.added.connect (worm_added_cb);
            worm.finish_added.connect (worm_finish_added_cb);
            worm.moved.connect (worm_moved_cb);
            worm.rescaled.connect (worm_rescaled_cb);
            worm.died.connect (worm_died_cb);
            worm.tail_reduced.connect (worm_tail_reduced_cb);
            worm.reversed.connect (worm_reversed_cb);
            worm.notify["is-materialized"].connect (() => {
                uint8 opacity;
                opacity = worm.is_materialized ? 0xff : 0x50;

                WormView actors = worm_actors.@get (worm);

//                actors.save_easing_state ();
//                actors.set_easing_duration (NibblesGame.GAMEDELAY * 10);
                actors.set_opacity (opacity);
//                actors.restore_easing_state ();
            });
        }
    }

    private void board_rescale (int new_tile_size)
    {
        int board_width, board_height;
//        float x_pos, y_pos;

//        if (level == null)
//            return;

        board_width = NibblesGame.WIDTH * new_tile_size;
        board_height = NibblesGame.HEIGHT * new_tile_size;

        warning (@"new board size: $board_width, $board_height");

//        foreach (var actor in level.get_children ())
//        {
//            actor.get_position (out x_pos, out y_pos);
//            actor.set_position ((x_pos / tile_size) * new_tile_size,
//                                (y_pos / tile_size) * new_tile_size);
//            actor.set_size (new_tile_size, new_tile_size);
//        }

//        if (!name_labels.visible)
//            return;

//        foreach (var worm in game.worms)
//        {
//            var actor = name_labels.get_child_at_index (worm.id);

//            var middle = worm.length / 2;
//            if (worm.direction == WormDirection.UP || worm.direction == WormDirection.DOWN)
//            {
//                actor.set_x (worm.list[middle].x * new_tile_size - actor.width / 2 + new_tile_size / 2);
//                actor.set_y (worm.list[middle].y * new_tile_size - 5 * new_tile_size);
//            }
//            else if (worm.direction == WormDirection.LEFT || worm.direction == WormDirection.RIGHT)
//            {
//                actor.set_x (worm.list[middle].x * new_tile_size - actor.width / 2 + new_tile_size / 2);
//                actor.set_y (worm.head.y * new_tile_size - 3 * new_tile_size);
//            }
//        }
    }

    private void animate_end_game_cb ()
    {
        foreach (var worm in game.worms)
            worm_actors.@get (worm).hide ();

        foreach (var actor in warp_actors)
            actor.hide ();

//        level.save_easing_state ();
//        level.set_easing_mode (Clutter.AnimationMode.EASE_IN_QUAD);
//        level.set_easing_duration (NibblesGame.GAMEDELAY * 20);
//        level.set_scale (0.4f, 0.4f);
//        level.set_pivot_point (0.5f, 0.5f);
//        level.set_opacity (0);
//        level.restore_easing_state ();
    }

//    internal void create_name_labels ()
//    {
//        name_labels = new Clutter.Actor ();
//        foreach (var worm in game.worms)
//        {
//            var color = game.worm_props.@get (worm).color;

//            /* Translators: the player's number, e.g. "Player 1" or "Player 2". */
//            var player_id = _("Player %d").printf (worm.id + 1);
//            var label = new Clutter.Text.with_text ("Monospace 10", @"<b>$(player_id)</b>");
//            label.set_use_markup (true);
//            label.set_color (Clutter.Color.from_string (colorval_name_untranslated (color)));

//            var middle = worm.length / 2;
//            if (worm.direction == WormDirection.UP || worm.direction == WormDirection.DOWN)
//            {
//                label.set_x (worm.list[middle].x * tile_size - label.width / 2 + tile_size / 2);
//                label.set_y (worm.list[middle].y * tile_size - 5 * tile_size);
//            }
//            else if (worm.direction == WormDirection.LEFT || worm.direction == WormDirection.RIGHT)
//            {
//                label.set_x (worm.list[middle].x * tile_size - label.width / 2 + tile_size / 2);
//                label.set_y (worm.head.y * tile_size - 3 * tile_size);
//            }
//            name_labels.add (label);
//        }

//        level.add_child (name_labels);
//    }

    /*\
    * * Worms drawing
    \*/

    private void worm_added_cb (Worm worm)
    {
        var actor = new Image.from_pixbuf (worm_pixmaps[game.worm_props.@get (worm).color]);
        actor.pixel_size = tile_size;
        actor.insert_after (this, /* insert first */ null);

        GridLayoutChild child_layout = (GridLayoutChild) layout.get_layout_child (actor);
        child_layout.set_left_attach (worm.list.first ().x);
        child_layout.set_top_attach (worm.list.first ().y);

        var actors = worm_actors.@get (worm);
        actors.widgets.append (actor);
    }

    private void worm_finish_added_cb (Worm worm)
    {
        WormView actors = worm_actors.@get (worm);

        actors.set_opacity (0);
//        actors.set_scale (3.0, 3.0);

//        actors.save_easing_state ();
//        actors.set_easing_mode (Clutter.AnimationMode.EASE_OUT);
//        actors.set_easing_duration (NibblesGame.GAMEDELAY * 20);
//        actors.set_scale (1.0, 1.0);
//        actors.set_pivot_point (0.5f, 0.5f);
//        actors.set_opacity (0xff);
//        actors.restore_easing_state ();

        worm.dematerialize (game.board, 3);

        Timeout.add (NibblesGame.GAMEDELAY * 27, () => {
            worm.is_stopped = false;
            return Source.REMOVE;
        });
    }

    private void worm_moved_cb (Worm worm)
    {
        var actors = worm_actors.@get (worm);

        var tail_actor = actors.widgets.first ().data;
        actors.widgets.remove (tail_actor);
        tail_actor.hide ();
        tail_actor.unparent ();
        tail_actor.destroy ();
        worm_added_cb (worm);
    }

    private void worm_rescaled_cb (Worm worm, int new_tile_size)
    {
//        float x_pos, y_pos;
        var actors = worm_actors.@get (worm);
        if (actors == null)
            return;

//        foreach (var actor in actors.get_children ())
//        {
//            actor.get_position (out x_pos, out y_pos);
//            actor.set_position ((x_pos / tile_size) * new_tile_size,
//                                (y_pos / tile_size) * new_tile_size);
//            actor.set_size (new_tile_size, new_tile_size);
//        }
    }

    private void worm_died_cb (Worm worm)
    {
//        float x, y;
//        var group = new Clutter.Actor ();
//        var actors = worm_actors.@get (worm);
//        foreach (var actor in actors.get_children ())
//        {
//            GtkClutter.Texture texture = new GtkClutter.Texture ();
//            var color = game.worm_props.@get (worm).color;
//            try
//            {
//                texture.set_from_pixbuf (worm_pixmaps[color]);
//            }
//            catch (Clutter.TextureError e)
//            {
//                error ("Nibbles failed to set texture: %s", e.message);
//            }
//            catch (Error e)
//            {
//                error ("Nibbles failed to set texture: %s", e.message);
//            }

//            actor.get_position (out x, out y);

//            texture.set_position (x, y);
//            texture.set_size (tile_size, tile_size);
//            group.add_child (texture);
//        }

//        actors.remove_all_children ();

//        level.add_child (group);

//        group.save_easing_state ();
//        group.set_easing_mode (Clutter.AnimationMode.EASE_OUT_QUAD);
//        group.set_easing_duration (NibblesGame.GAMEDELAY * 9);
//        group.set_scale (2.0f, 2.0f);
//        group.set_pivot_point (0.5f, 0.5f);
//        group.set_opacity (0);
//        group.restore_easing_state ();

        play_sound ("crash");
    }

    private void worm_tail_reduced_cb (Worm worm, int erase_size)
    {
//        float x, y;
//        var group = new Clutter.Actor ();
//        var worm_actors = worm_actors.@get (worm);
//        var color = game.worm_props.@get (worm).color;
//        for (int i = 0; i < erase_size; i++)
//        {
//            var texture = new GtkClutter.Texture ();
//            try
//            {
//                texture.set_from_pixbuf (worm_pixmaps[color]);
//            }
//            catch (Clutter.TextureError e)
//            {
//                error ("Nibbles failed to set texture: %s", e.message);
//            }
//            catch (Error e)
//            {
//                error ("Nibbles failed to set texture: %s", e.message);
//            }

//            worm_actors.first_child.get_position (out x, out y);
//            worm_actors.remove_child (worm_actors.first_child);

//            texture.set_position (x, y);
//            texture.set_size (tile_size, tile_size);
//            group.add_child (texture);
//        }
//        level.add_child (group);

//        group.save_easing_state ();
//        group.set_easing_mode (Clutter.AnimationMode.EASE_OUT_EXPO);
//        group.set_easing_duration (NibblesGame.GAMEDELAY * 25);
//        group.set_opacity (0);
//        group.restore_easing_state ();
    }

    private void worm_reversed_cb (Worm worm)
    {
//        var actors = worm_actors.@get (worm);

//        var count = 0;
//        foreach (var actor in actors.get_children ())
//        {
//            actor.set_position (worm.list[count].x * tile_size, worm.list[count].y * tile_size);
//            count++;
//        }
    }

    /*\
    * * Bonuses drawing
    \*/

    private void bonus_added_cb (Bonus bonus)
    {
        var actor = new Image.from_pixbuf (boni_pixmaps [bonus.bonus_type]);
        actor.pixel_size = 2 * tile_size;
        actor.insert_after (this, /* insert first */ null);

        GridLayoutChild child_layout = (GridLayoutChild) layout.get_layout_child (actor);
        child_layout.set_left_attach (bonus.x);
        child_layout.set_top_attach (bonus.y);
        child_layout.set_column_span (2);
        child_layout.set_row_span (2);

        if (bonus.bonus_type != BonusType.REGULAR)
            play_sound ("appear");

        bonus_actors.@set (bonus, actor);
    }

    private void bonus_removed_cb (Bonus bonus)
    {
        var bonus_actor = bonus_actors.@get (bonus);
        bonus_actors.unset (bonus);
        bonus_actor.hide ();
        bonus_actor.unparent ();
        bonus_actor.destroy ();
    }

    private void bonus_applied_cb (Bonus bonus, Worm worm)
    {
        var actors = worm_actors.@get (worm);
//        var actor = actors.last_child;

//        actor.save_easing_state ();
//        actor.set_easing_mode (Clutter.AnimationMode.EASE_OUT_QUINT);
//        actor.set_easing_duration (NibblesGame.GAMEDELAY * 15);
//        actor.set_scale (1.45f, 1.45f);
//        actor.set_pivot_point (0.5f, 0.5f);
//        actor.restore_easing_state ();

        switch (bonus.bonus_type)
        {
            case BonusType.REGULAR:
                play_sound ("gobble");
                break;
            case BonusType.DOUBLE:
                play_sound ("bonus");
                break;
            case BonusType.HALF:
                play_sound ("bonus");
                break;
            case BonusType.LIFE:
                play_sound ("life");
                break;
            case BonusType.REVERSE:
                play_sound ("reverse");
                break;
            default:
                assert_not_reached ();
        }
    }

    private void boni_rescale (int new_tile_size)
    {
        foreach (var bonus in bonus_actors.keys)
        {
            var actor = bonus_actors.@get (bonus);
            actor.pixel_size = 2 * new_tile_size;
        }
    }

    /*\
    * * Warps drawing
    \*/

    private void warp_added_cb (int x, int y)
    {
        var actor = new Image.from_pixbuf (boni_pixmaps [BonusType.WARP]);
        actor.pixel_size = 2 * tile_size;
        actor.insert_after (this, /* insert first */ null);

        GridLayoutChild child_layout = (GridLayoutChild) layout.get_layout_child (actor);
        child_layout.set_left_attach (x);
        child_layout.set_top_attach (y);
        child_layout.set_column_span (2);
        child_layout.set_row_span (2);

        warp_actors.add (actor);
    }

    private void warps_rescale (int new_tile_size)
    {
        foreach (var actor in warp_actors)
            actor.pixel_size = 2 * new_tile_size;
    }

    /*\
    * * Sound
    \*/

    public bool is_muted { private get; internal construct set; }

    private GSound.Context sound_context;
    private SoundContextState sound_context_state = SoundContextState.INITIAL;

    private enum SoundContextState
    {
        INITIAL,
        WORKING,
        ERRORED;
    }

    private void init_sound ()
     // requires (sound_context_state == SoundContextState.INITIAL)
    {
        try
        {
            sound_context = new GSound.Context ();
            sound_context_state = SoundContextState.WORKING;
        }
        catch (Error e)
        {
            warning (e.message);
            sound_context_state = SoundContextState.ERRORED;
        }
    }

    private void play_sound (string name)
    {
        if (!is_muted)
        {
            if (sound_context_state == SoundContextState.INITIAL)
                init_sound ();
            if (sound_context_state == SoundContextState.WORKING)
                _play_sound (name, sound_context);
        }
    }

    private static void _play_sound (string _name, GSound.Context sound_context)
     // requires (sound_context_state == SoundContextState.WORKING)
    {
        string name = _name + ".ogg";
        string path = Path.build_filename (SOUND_DIRECTORY, name);
        try
        {
            sound_context.play_simple (null, GSound.Attribute.MEDIA_NAME, name,
                                             GSound.Attribute.MEDIA_FILENAME, path);
        }
        catch (Error e)
        {
            warning (e.message);
        }
    }

    /*\
    * * Colors
    \*/

    internal const int NUM_COLORS = 6;      // only used in preferences-dialog.vala
    private static string[,] color_lookup =
    {
        /* Translators: possible color of a worm, as displayed in the Preferences dialog combobox */
        { "red",    N_("red")    },
        /* Translators: possible color of a worm, as displayed in the Preferences dialog combobox */
        { "green",  N_("green")  },
        /* Translators: possible color of a worm, as displayed in the Preferences dialog combobox */
        { "blue",   N_("blue")   },
        /* Translators: possible color of a worm, as displayed in the Preferences dialog combobox */
        { "yellow", N_("yellow") },
        /* Translators: possible color of a worm, as displayed in the Preferences dialog combobox */
        { "cyan",   N_("cyan")   },
        /* Translators: possible color of a worm, as displayed in the Preferences dialog combobox */
        { "purple", N_("purple") }
    };

    internal static string colorval_name_untranslated (int colorval)
    {
        return color_lookup[colorval, 0];
    }

    internal static string colorval_name_translated (int colorval)
    {
        return _(color_lookup[colorval, 1]);
    }
}
