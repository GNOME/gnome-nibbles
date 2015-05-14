public class NibblesView : GtkClutter.Embed
{
    /* Game being played */
    public NibblesGame game;

    public Clutter.Actor surface;
    public Clutter.Stage stage;

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

        set_size_request (game.properties.tile_size * game.width, game.properties.tile_size * game.height);

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
            Posix.exit (Posix.EXIT_FAILURE);
        }
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
}
