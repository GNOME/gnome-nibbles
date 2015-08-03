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

public class NibblesView : GtkClutter.Embed
{
    /* Game being played */
    private NibblesGame _game;
    public NibblesGame game
    {
        get { return _game; }
        set
        {
            if (_game != null)
                SignalHandler.disconnect_matched (_game, SignalMatchType.DATA, 0, 0, null, null, this);

            _game = value;
            _game.boni.bonus_added.connect (bonus_added_cb);
            _game.boni.bonus_removed.connect (bonus_removed_cb);

            _game.bonus_applied.connect (bonus_applied_cb);

            _game.animate_end_game.connect (animate_end_game_cb);
        }
    }

    public Clutter.Stage stage { get; private set; }
    private Clutter.Actor level;
    public Clutter.Actor name_labels { get; private set; }

    private Gdk.Pixbuf wall_pixmaps[11];
    public Gdk.Pixbuf worm_pixmaps[7];
    private Gdk.Pixbuf boni_pixmaps[9];

    public Gee.HashMap<Worm, WormActor> worm_actors;
    public Gee.HashMap<Bonus, BonusTexture> bonus_actors;

    public const int NUM_COLORS = 7;
    public static string[] color_lookup =
    {
      "red",
      "green",
      "blue",
      "orange",
      "cyan",
      "purple",
      "grey"
    };

    public NibblesView (NibblesGame game)
    {
        this.game = game;

        stage = (Clutter.Stage) get_stage ();
        Clutter.Color stage_color = { 0x00, 0x00, 0x00, 0xff };
        stage.set_background_color (stage_color);

        set_size_request (NibblesGame.MINIMUM_TILE_SIZE * NibblesGame.WIDTH,
                          NibblesGame.MINIMUM_TILE_SIZE * NibblesGame.HEIGHT);

        worm_actors = new Gee.HashMap<Worm, WormActor> ();
        bonus_actors = new Gee.HashMap<Bonus, BonusTexture> ();

        load_pixmap ();
    }

    public override bool key_press_event (Gdk.EventKey event)
    {
        return game.handle_keypress (event.keyval);
    }

    public void new_level (int level)
    {
        string level_name;
        string filename;
        string tmpboard;
        int count = 0;

        level_name = "level%03d.gnl".printf (level);
        filename = Path.build_filename (PKGDATADIR, "levels", level_name, null);

        FileStream file;
        if ((file = FileStream.open (filename, "r")) == null)
        {
            /* Fatal console error when the game's data files are missing. */
            error (_("Nibbles couldn't find pixmap file: %s"), filename);
        }

        worm_actors.clear ();
        bonus_actors.clear ();
        game.boni.reset (game.numworms);

        for (int i = 0; i < NibblesGame.HEIGHT; i++)
        {
            if ((tmpboard = file.read_line ()) == null)
            {
                /* Fatal console error when the game's level files are damaged. */
                error (_("Level file appears to be damaged: %s"), filename);
            }

            for (int j = 0; j < NibblesGame.WIDTH; j++)
            {
                game.walls[j, i] = tmpboard.get(j);
                switch (game.walls[j, i])
                {
                    case 'm':
                        game.walls[j, i] = NibblesGame.EMPTYCHAR;
                        if (count < game.numworms)
                        {
                            game.worms[count].set_start (j, i, WormDirection.UP);

                            var actors = new WormActor ();
                            worm_actors.set (game.worms[count], actors);
                            count++;
                        }
                        break;
                    case 'n':
                        game.walls[j, i] = NibblesGame.EMPTYCHAR;
                        if (count < game.numworms)
                        {
                            game.worms[count].set_start (j, i, WormDirection.LEFT);

                            var actors = new WormActor ();
                            worm_actors.set (game.worms[count], actors);
                            count++;
                        }
                        break;
                    case 'o':
                        game.walls[j, i] = NibblesGame.EMPTYCHAR;
                        if (count < game.numworms)
                        {
                            game.worms[count].set_start (j, i, WormDirection.DOWN);

                            var actors = new WormActor ();
                            worm_actors.set (game.worms[count], actors);
                            count++;
                        }
                        break;
                    case 'p':
                        game.walls[j, i] = NibblesGame.EMPTYCHAR;
                        if (count < game.numworms)
                        {
                            game.worms[count].set_start (j, i, WormDirection.RIGHT);

                            var actors = new WormActor ();
                            worm_actors.set (game.worms[count], actors);
                            count++;
                        }
                        break;
                    default:
                        break;
                }
            }
        }

        load_level ();
    }

    public Gdk.Pixbuf load_pixmap_file (string pixmap, int xsize, int ysize)
    {
        var filename = Path.build_filename (PKGDATADIR, "pixmaps", pixmap, null);

        if (filename == null)
        {
            /* Fatal console error when the game's data files are missing. */
            error (_("Nibbles couldn't find pixmap file: %s"), filename);
        }

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
            "diamond.svg",
            "bonus1.svg",
            "bonus2.svg",
            "life.svg",
            "bonus3.svg",
            "bonus4.svg",
            "bonus5.svg",
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
            "wall-cross.svg"
        };

        string[] worm_files =
        {
            "snake-red.svg",
            "snake-green.svg",
            "snake-blue.svg",
            "snake-yellow.svg",
            "snake-cyan.svg",
            "snake-magenta.svg",
            "snake-grey.svg"
        };

        for (int i = 0; i < 8; i++)
        {
            boni_pixmaps[i] = load_pixmap_file (bonus_files[i],
                                                2 * game.tile_size, 2 * game.tile_size);
        }

        for (int i = 0; i < 11; i++)
        {
            wall_pixmaps[i] = load_pixmap_file (small_files[i],
                                                2 * game.tile_size, 2 * game.tile_size);
        }

        for (int i = 0; i < 7; i++)
        {
            worm_pixmaps[i] = load_pixmap_file (worm_files[i],
                                                game.tile_size, game.tile_size);
        }
    }

    void load_level ()
    {
        int x_pos, y_pos;
        GtkClutter.Texture tmp = null;
        bool is_wall = true;
        level = new Clutter.Actor ();

        /* Load wall_pixmaps onto the surface */
        for (int i = 0; i < NibblesGame.HEIGHT; i++)
        {
            y_pos = i * game.tile_size;
            for (int j = 0; j < NibblesGame.WIDTH; j++)
            {
                is_wall = true;
                try
                {
                    switch (game.walls[j, i])
                    {
                        case 'a': // empty space
                            is_wall = false;
                            break;
                        case 'b': // straight up
                            tmp = new GtkClutter.Texture ();
                            tmp.set_from_pixbuf (wall_pixmaps[0]);
                            break;
                        case 'c': // straight side
                            tmp = new GtkClutter.Texture ();
                            tmp.set_from_pixbuf (wall_pixmaps[1]);
                            break;
                        case 'd': // corner bottom left
                            tmp = new GtkClutter.Texture ();
                            tmp.set_from_pixbuf (wall_pixmaps[2]);
                            break;
                        case 'e': // corner bottom right
                            tmp = new GtkClutter.Texture ();
                            tmp.set_from_pixbuf (wall_pixmaps[3]);
                            break;
                        case 'f': // corner up left
                            tmp = new GtkClutter.Texture ();
                            tmp.set_from_pixbuf (wall_pixmaps[4]);
                            break;
                        case 'g': // corner up right
                            tmp = new GtkClutter.Texture ();
                            tmp.set_from_pixbuf (wall_pixmaps[5]);
                            break;
                        case 'h': // tee up
                            tmp = new GtkClutter.Texture ();
                            tmp.set_from_pixbuf (wall_pixmaps[6]);
                            break;
                        case 'i': // tee right
                            tmp = new GtkClutter.Texture ();
                            tmp.set_from_pixbuf (wall_pixmaps[7]);
                            break;
                        case 'j': // tee left
                            tmp = new GtkClutter.Texture ();
                            tmp.set_from_pixbuf (wall_pixmaps[8]);
                            break;
                        case 'k': // tee down
                            tmp = new GtkClutter.Texture ();
                            tmp.set_from_pixbuf (wall_pixmaps[9]);
                            break;
                        case 'l': // tee cross
                            tmp = new GtkClutter.Texture ();
                            tmp.set_from_pixbuf (wall_pixmaps[10]);
                            break;
                        default:
                            is_wall = false;
                            break;
                    }
                }
                catch (Error e)
                {
                    /* Error message when a level cannot be loaded. */
                    error (_("Error loading level: %s"), e.message);
                }

                if (is_wall)
                {
                    x_pos = j * game.tile_size;

                    tmp.set_size (game.tile_size, game.tile_size);
                    tmp.set_position (x_pos, y_pos);
                    level.add_child (tmp);
                }
            }
        }
        stage.add_child (level);

        level.set_opacity (0);
        level.set_scale (0.2, 0.2);

        level.save_easing_state ();
        level.set_easing_mode (Clutter.AnimationMode.EASE_OUT_BOUNCE);
        level.set_easing_duration (NibblesGame.GAMEDELAY * NibblesGame.GAMEDELAY);
        level.set_scale (1.0, 1.0);
        level.set_pivot_point (0.5f, 0.5f);
        level.set_opacity (0xff);
        level.restore_easing_state ();
    }

    public void create_name_labels ()
    {
        name_labels = new Clutter.Actor ();
        foreach (var worm in game.worms)
        {
            var color = game.worm_props.get (worm).color;

            var label = new Clutter.Text.with_text ("Source Pro 10", _(@"<b>PLAYER $(worm.id + 1)</b>"));
            label.set_use_markup (true);
            label.set_color (Clutter.Color.from_string (colorval_name (color)));
            // TODO: Better aligb these
            switch (worm.direction)
            {
                case WormDirection.UP:
                    label.x = (worm.head ().x - 4) * game.tile_size;
                    label.y = (worm.head ().y - 8) * game.tile_size;
                    break;
                case WormDirection.DOWN:
                    label.x = (worm.head ().x - 4) * game.tile_size;
                    label.y = (worm.head ().y - 2) * game.tile_size;
                    break;
                case WormDirection.LEFT:
                    label.x = (worm.head ().x - 6) * game.tile_size;
                    label.y = (worm.head ().y - 4) * game.tile_size;
                    break;
                case WormDirection.RIGHT:
                    label.x = (worm.head ().x - 0) * game.tile_size;
                    label.y = (worm.head ().y - 4) * game.tile_size;
                    break;
                default:
                    break;
            }
            name_labels.add (label);
        }

        stage.add_child (name_labels);
    }

    public void connect_worm_signals ()
    {
        foreach (var worm in game.worms)
        {
            worm.added.connect (worm_added_cb);
            worm.moved.connect (worm_moved_cb);
            worm.rescaled.connect (worm_rescaled_cb);
            worm.died.connect (worm_died_cb);
            worm.tail_reduced.connect (worm_tail_reduced_cb);
        }
    }

    public void board_rescale (int tile_size)
    {
        int board_width, board_height;
        float x_pos, y_pos;

        if (level == null)
            return;

        board_width = NibblesGame.WIDTH * tile_size;
        board_height = NibblesGame.HEIGHT * tile_size;

        foreach (var actor in level.get_children ())
        {
            actor.get_position (out x_pos, out y_pos);
            actor.set_position ((x_pos / game.tile_size) * tile_size,
                                (y_pos / game.tile_size) * tile_size);
            actor.set_size (tile_size, tile_size);
        }

        if (!name_labels.visible)
            return;

        foreach (var worm in game.worms)
        {
            var actor = name_labels.get_child_at_index (worm.id);
            actor.get_position (out x_pos, out y_pos);
            actor.x = ((x_pos / game.tile_size) * tile_size);
            actor.y = ((y_pos / game.tile_size) * tile_size);
        }
    }

    public void animate_end_game_cb ()
    {
        foreach (var worm in game.worms)
        {
            var actors = worm_actors.get (worm);

            actors.save_easing_state ();
            actors.set_easing_mode (Clutter.AnimationMode.EASE_IN_QUAD);
            actors.set_easing_duration (NibblesGame.GAMEDELAY * 15);
            actors.set_scale (0.4f, 0.4f);
            actors.set_opacity (0);
            actors.restore_easing_state ();
        }

        foreach (var bonus in game.boni.bonuses)
        {
            var actor = bonus_actors.get (bonus);

            actor.save_easing_state ();
            actor.set_easing_mode (Clutter.AnimationMode.EASE_IN_QUAD);
            actor.set_easing_duration (NibblesGame.GAMEDELAY * 15);
            actor.set_scale (0.4f, 0.4f);
            actor.set_pivot_point (0.5f, 0.5f);
            actor.set_opacity (0);
            actor.restore_easing_state ();
        }

        level.save_easing_state ();
        level.set_easing_mode (Clutter.AnimationMode.EASE_IN_QUAD);
        level.set_easing_duration (NibblesGame.GAMEDELAY * 20);
        level.set_scale (0.4f, 0.4f);
        level.set_pivot_point (0.5f, 0.5f);
        level.set_opacity (0);
        level.restore_easing_state ();
    }

    public void worm_added_cb (Worm worm)
    {
        var actor = new GtkClutter.Texture ();
        try
        {
            actor.set_from_pixbuf (worm_pixmaps[game.worm_props.get (worm).color]);
        }
        catch (Clutter.TextureError e)
        {
            /* Fatal console error when a worm's texture could not be set. */
            error (_("Nibbles failed to set texture: %s"), e.message);
        }
        catch (Error e)
        {
            /* Fatal console error when a worm's texture could not be set. */
            error (_("Nibbles failed to set texture: %s"), e.message);
        }
        actor.set_size (game.tile_size, game.tile_size);
        actor.set_position (worm.list.first ().x * game.tile_size, worm.list.first ().y * game.tile_size);

        var actors = worm_actors.get (worm);
        actors.add_child (actor);
    }

    public void worm_moved_cb (Worm worm)
    {
        var actors = worm_actors.get (worm);

        var tail_actor = actors.first_child;
        actors.remove_child (tail_actor);
        worm_added_cb (worm);
    }

    public void worm_rescaled_cb (Worm worm, int tile_size)
    {
        float x_pos, y_pos;
        var actors = worm_actors.get (worm);
        if (actors == null)
            return;

        foreach (var actor in actors.get_children ())
        {
            actor.get_position (out x_pos, out y_pos);
            actor.set_position ((x_pos / game.tile_size) * tile_size,
                                (y_pos / game.tile_size) * tile_size);
            actor.set_size (tile_size, tile_size);
        }
    }

    public void worm_died_cb (Worm worm)
    {
        float x, y;
        var group = new Clutter.Actor ();
        var actors = worm_actors.get (worm);
        foreach (var actor in actors.get_children ())
        {
            GtkClutter.Texture texture = new GtkClutter.Texture ();
            var color = game.worm_props.get (worm).color;
            try
            {
                texture.set_from_pixbuf (worm_pixmaps[color]);
            }
            catch (Clutter.TextureError e)
            {
                /* Fatal console error when a worm's texture could not be set. */
                error (_("Nibbles failed to set texture: %s"), e.message);
            }
            catch (Error e)
            {
                /* Fatal console error when a worm's texture could not be set. */
                error (_("Nibbles failed to set texture: %s"), e.message);
            }

            actor.get_position (out x, out y);

            texture.set_position (x, y);
            texture.set_size (game.tile_size, game.tile_size);
            group.add_child (texture);
        }

        actors.remove_all_children ();

        stage.add_child (group);

        group.save_easing_state ();
        group.set_easing_mode (Clutter.AnimationMode.EASE_OUT_QUAD);
        group.set_easing_duration (NibblesGame.GAMEDELAY * 9);
        group.set_scale (2.0f, 2.0f);
        group.set_pivot_point (0.5f, 0.5f);
        group.set_opacity (0);
        group.restore_easing_state ();
    }

    public void worm_tail_reduced_cb (Worm worm, int erase_size)
    {
        float x, y;
        var group = new Clutter.Actor ();
        var worm_actors = worm_actors.get (worm);
        var color = game.worm_props.get (worm).color;
        for (int i = 0; i < erase_size; i++)
        {
            var texture = new GtkClutter.Texture ();
            try
            {
                texture.set_from_pixbuf (worm_pixmaps[color]);
            }
            catch (Clutter.TextureError e)
            {
                /* Fatal console error when a worm's texture could not be set. */
                error (_("Nibbles failed to set texture: %s"), e.message);
            }
            catch (Error e)
            {
                /* Fatal console error when a worm's texture could not be set. */
                error (_("Nibbles failed to set texture: %s"), e.message);
            }

            worm_actors.first_child.get_position (out x, out y);
            worm_actors.remove_child (worm_actors.first_child);

            texture.set_position (x, y);
            texture.set_size (game.tile_size, game.tile_size);
            group.add_child (texture);
        }
        stage.add_child (group);

        group.save_easing_state ();
        group.set_easing_mode (Clutter.AnimationMode.EASE_OUT_EXPO);
        group.set_easing_duration (NibblesGame.GAMEDELAY * 25);
        group.set_opacity (0);
        group.restore_easing_state ();
    }

    public void bonus_added_cb ()
    {
        stderr.printf("[Debug] Bonus ADDED\n");
        /* Last bonus added to the list is the one that needs a texture */
        var bonus = game.boni.bonuses.last ();
        var actor = new BonusTexture ();
        try
        {
            actor.set_from_pixbuf (boni_pixmaps[bonus.type]);
        }
        catch (Clutter.TextureError e)
        {
            /* Fatal console error when a texture could not be set. */
            error (_("Nibbles failed to set texture: %s"), e.message);
        }
        catch (Error e)
        {
            /* Fatal console error when a texture could not be set. */
            error (_("Nibbles failed to set texture: %s"), e.message);
        }

        actor.set_position (bonus.x * game.tile_size, bonus.y * game.tile_size);

        stage.add_child (actor);

        bonus_actors.set (bonus, actor);
    }

    public void bonus_removed_cb (Bonus bonus)
    {
        var bonus_actor = bonus_actors.get (bonus);
        bonus_actors.unset (bonus);
        bonus_actor.hide ();
        stage.remove_child (bonus_actor);
    }

    public void bonus_applied_cb (Worm worm)
    {
        var actors = worm_actors.get (worm);
        var actor = actors.last_child;

        actor.save_easing_state ();
        actor.set_easing_mode (Clutter.AnimationMode.EASE_OUT_QUINT);
        actor.set_easing_duration (NibblesGame.GAMEDELAY * 15);
        actor.set_scale (1.45f, 1.45f);
        actor.set_pivot_point (0.5f, 0.5f);
        actor.restore_easing_state ();
    }

    public void boni_rescale (int tile_size)
    {
        float x_pos, y_pos;

        foreach (var bonus in game.boni.bonuses)
        {
            var actor = bonus_actors.get (bonus);
            actor.get_position (out x_pos, out y_pos);
            actor.set_position ((x_pos / game.tile_size) * tile_size,
                                (y_pos / game.tile_size) * tile_size);

            try
            {
                actor.set_from_pixbuf (boni_pixmaps[bonus.type]);
            }
            catch (Clutter.TextureError e)
            {
                /* Fatal console error when a texture could not be set. */
                error (_("Nibbles failed to set texture: %s"), e.message);
            }
            catch (Error e)
            {
                /* Fatal console error when a texture could not be set. */
                error (_("Nibbles failed to set texture: %s"), e.message);
            }
        }
    }

    public static int colorval_from_name (string name)
    {
        for (int i = 0; i < NUM_COLORS; i++)
        {
            if (color_lookup[i] == name)
                return i;
        }

        return 0;
    }

    public static string colorval_name (int colorval)
    {
        return color_lookup[colorval];
    }
}

public class WormActor : Clutter.Actor
{
    public override void show ()
    {
        base.show ();

        set_opacity (0);
        set_scale (3.0, 3.0);

        save_easing_state ();
        set_easing_mode (Clutter.AnimationMode.EASE_OUT_CIRC);
        set_easing_duration (NibblesGame.GAMEDELAY * 26);
        set_scale (1.0, 1.0);
        set_pivot_point (0.5f, 0.5f);
        set_opacity (0xff);
        restore_easing_state ();
    }
}

public class BonusTexture : GtkClutter.Texture
{
    public override void show ()
    {
        base.show ();

        set_opacity (0);
        set_scale (3.0, 3.0);

        save_easing_state ();
        set_easing_mode (Clutter.AnimationMode.EASE_OUT_BOUNCE);
        set_easing_duration (NibblesGame.GAMEDELAY * 20);
        set_scale (1.0, 1.0);
        set_pivot_point (0.5f, 0.5f);
        set_opacity (0xff);
        restore_easing_state ();
    }
}
