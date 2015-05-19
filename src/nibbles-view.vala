public class NibblesView : GtkClutter.Embed
{
    /* Game being played */
    public NibblesGame game { get; private set; }

    public Clutter.Actor surface;
    public Clutter.Stage stage;
    private Clutter.Actor level;

    Gdk.Pixbuf[] wall_pixmaps = { null, null, null, null, null,
                                  null, null, null, null, null,
                                  null
    };
    Gdk.Pixbuf[] worm_pixmaps = { null, null, null, null, null,
                                   null, null
    };
    Gdk.Pixbuf[] boni_pixmaps = { null, null, null, null, null,
                                   null, null, null, null
    };

    public NibblesView (NibblesGame game)
    {
        this.game = game;

        stage = get_stage () as Clutter.Stage;
        Clutter.Color stage_color = { 0x00, 0x00, 0x00, 0xff };
        stage.set_background_color (stage_color);

        set_size_request (7 * game.width, 7 * game.height);

        try
        {
            var pixbuf = new Gdk.Pixbuf.from_file (Path.build_filename (DATADIR, "pixmaps", "wall-small-empty.svg"));
            surface = new GtkClutter.Texture ();
            ((GtkClutter.Texture) surface).set_from_pixbuf (pixbuf);

            var val = Value (typeof (bool));
            val.set_boolean (true);
            surface.set_opacity (100);

            surface.set_property ("repeat-x", val);
            surface.set_property ("repeat-y", val);

            surface.set_position (0, 0);
            surface.show ();
        }
        catch (Clutter.TextureError e)
        {
            warning ("Failed to load textures: %s", e.message);
        }
        catch (GLib.Error e)
        {
            warning ("Failed to load textures: %s", e.message);
        }

        load_pixmap ();

        stage.add_child (surface);
    }

    public void new_level (int level)
    {
        string level_name;
        string filename;
        string tmpboard;

        level_name = "level%03d.gnl".printf (level);
        filename = Path.build_filename (DATADIR, "levels", level_name, null);

        FileStream file;
        if ((file = FileStream.open (filename, "r")) == null) {
            string message =
                (_("Nibbles couldn't load level file:\n%s\n\n" +
                   "Please check your Nibbles installation")).printf (filename);
            var dialog = new Gtk.MessageDialog (null,
                                                Gtk.DialogFlags.MODAL,
                                                Gtk.MessageType.ERROR,
                                                Gtk.ButtonsType.OK,
                                                message);
            dialog.run ();
            dialog.destroy ();
        }

        for (int i = 0; i < game.height; i++)
        {
            if ((tmpboard = file.read_line ()) == null)
            {
                string message =
                    (_("Level file appears to be damaged:\n%s\n\n" +
                       "Please check your Nibbles installation")).printf (filename);
                var dialog = new Gtk.MessageDialog (null,
                                                    Gtk.DialogFlags.MODAL,
                                                    Gtk.MessageType.ERROR,
                                                    Gtk.ButtonsType.OK,
                                                    message);
                dialog.run ();
                dialog.destroy ();
                break;
            }

            for (int j = 0; j < game.width; j++)
            {
                game.walls[j, i] = tmpboard.@get(j);
                switch (game.walls[j, i])
                {
                    case 'm':
                        game.walls[j, i] = game.EMPTYCHAR;
                        break;
                    case 'n':
                        game.walls[j, i] = game.EMPTYCHAR;
                        break;
                    case 'o':
                        game.walls[j, i] = game.EMPTYCHAR;
                        break;
                    case 'p':
                        game.walls[j, i] = game.EMPTYCHAR;
                        break;
                    default:
                        break;
                }
            }
        }

        load_level ();
    }

    private Gdk.Pixbuf load_pixmap_file (string pixmap, int xsize, int ysize)
    {
        var filename = Path.build_filename (DATADIR, "pixmaps", pixmap, null);

        if (filename == null)
        {
            string message =
                (_("Nibbles couldn't find pixmap file:\n%s\n\n" +
                   "Please check your Nibbles installation")).printf (pixmap);
                var dialog = new Gtk.MessageDialog (null,
                                                    Gtk.DialogFlags.MODAL,
                                                    Gtk.MessageType.ERROR,
                                                    Gtk.ButtonsType.OK,
                                                    message);
                dialog.run ();
                dialog.destroy ();
                Posix.exit (Posix.EXIT_FAILURE);
        }

        Gdk.Pixbuf image = null;
        try
        {
            image = new Gdk.Pixbuf.from_file_at_scale (filename, xsize, ysize, true);
        }
        catch (GLib.Error e)
        {
            warning ("Failed to load pixmap file: %s", e.message);
        }
        return image;
    }

    private void load_pixmap ()
    {
        string[] bonus_files = {
            "diamond.svg",
            "bonus1.svg",
            "bonus2.svg",
            "life.svg",
            "bonus3.svg",
            "bonus4.svg",
            "bonus5.svg",
            "questionmark.svg"
        };

        string[] small_files = {
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

        string[] worm_files = {
            "snake-red.svg",
            "snake-green.svg",
            "snake-blue.svg",
            "snake-yellow.svg",
            "snake-cyan.svg",
            "snake-magenta.svg",
            "snake-grey.svg"
        };

        int tile_size = game.properties.tile_size;
        for (int i = 0; i < 8; i++) {
            boni_pixmaps[i] = load_pixmap_file (bonus_files[i],
                                                2 * tile_size, 2 * tile_size);
        }

        for (int i = 0; i < 11; i++) {
            wall_pixmaps[i] = load_pixmap_file (small_files[i],
                                                2 * tile_size, 2 * tile_size);
        }

        for (int i = 0; i < 7; i++) {
            worm_pixmaps[i] = load_pixmap_file (worm_files[i],
                                                tile_size, tile_size);
        }
    }

    void load_level ()
    {
        int x_pos, y_pos;
        Clutter.Actor tmp = null;
        bool is_wall = true;
        level = new Clutter.Actor ();

        /* Load wall_pixmaps onto the surface */
        for (int i = 0; i < game.height; i++)
        {
            y_pos = i * game.properties.tile_size;
            for (int j = 0; j < game.width; j++)
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
                            ((GtkClutter.Texture) tmp).set_from_pixbuf (wall_pixmaps[0]);
                            break;
                        case 'c': // straight side
                            tmp = new GtkClutter.Texture ();
                            ((GtkClutter.Texture) tmp).set_from_pixbuf (wall_pixmaps[1]);
                            break;
                        case 'd': // corner bottom left
                            tmp = new GtkClutter.Texture ();
                            ((GtkClutter.Texture) tmp).set_from_pixbuf (wall_pixmaps[2]);
                            break;
                        case 'e': // corner bottom right
                            tmp = new GtkClutter.Texture ();
                            ((GtkClutter.Texture) tmp).set_from_pixbuf (wall_pixmaps[3]);
                            break;
                        case 'f': // corner up left
                            tmp = new GtkClutter.Texture ();
                            ((GtkClutter.Texture) tmp).set_from_pixbuf (wall_pixmaps[4]);
                            break;
                        case 'g': // corner up right
                            tmp = new GtkClutter.Texture ();
                            ((GtkClutter.Texture) tmp).set_from_pixbuf (wall_pixmaps[5]);
                            break;
                        case 'h': // tee up
                            tmp = new GtkClutter.Texture ();
                            ((GtkClutter.Texture) tmp).set_from_pixbuf (wall_pixmaps[6]);
                            break;
                        case 'i': // tee right
                            tmp = new GtkClutter.Texture ();
                            ((GtkClutter.Texture) tmp).set_from_pixbuf (wall_pixmaps[7]);
                            break;
                        case 'j': // tee left
                            tmp = new GtkClutter.Texture ();
                            ((GtkClutter.Texture) tmp).set_from_pixbuf (wall_pixmaps[8]);
                            break;
                        case 'k': // tee down
                            tmp = new GtkClutter.Texture ();
                            ((GtkClutter.Texture) tmp).set_from_pixbuf (wall_pixmaps[9]);
                            break;
                        case 'l': // tee cross
                            tmp = new GtkClutter.Texture ();
                            ((GtkClutter.Texture) tmp).set_from_pixbuf (wall_pixmaps[10]);
                            break;
                        default:
                            is_wall = false;
                            break;
                    }
                }
                catch (GLib.Error e)
                {

                }

                if (is_wall)
                {
                    x_pos = j * game.properties.tile_size;

                    ((Clutter.Actor) tmp).set_size (game.properties.tile_size,
                                                    game.properties.tile_size);
                    ((Clutter.Actor) tmp).set_position (x_pos, y_pos);
                    ((Clutter.Actor) tmp).show ();
                    level.add_child ((Clutter.Actor) tmp);
                }
            }
        }

        stage.add_child (level);

        level.set_opacity (0);
        ((Clutter.Actor) level).set_scale (0.2, 0.2);

        level.save_easing_state ();
        level.set_easing_mode (Clutter.AnimationMode.EASE_OUT_BOUNCE);
        level.set_easing_duration (game.properties.GAMEDELAY * game.properties.GAMEDELAY);
        level.set_scale (1.0, 1.0);
        level.set_pivot_point (0.5f, 0.5f);
        level.set_opacity (0xff);
        level.restore_easing_state ();
    }

    public void board_rescale (int tile_size)
    {
        int count;
        int board_width, board_height;
        float x_pos, y_pos;
        Clutter.Actor tmp;

        if (level == null)
            return;
        if (surface == null)
            return;

        board_width = game.width * tile_size;
        board_height = game.height * tile_size;

        surface.set_size (board_width, board_height);

        count = level.get_n_children ();

        for (int i = 0; i < count; i++)
        {
            tmp = level.get_child_at_index (i);
            ((Clutter.Actor) tmp).get_position (out x_pos, out y_pos);
            ((Clutter.Actor) tmp).set_position ((x_pos / game.properties.tile_size) * tile_size,
                                                (y_pos / game.properties.tile_size) * tile_size);
            ((Clutter.Actor) tmp).set_size (tile_size, tile_size);
        }
    }
}
