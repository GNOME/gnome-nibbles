public class NibblesView : GtkClutter.Embed
{
    /* Game being played */
    public NibblesGame game;

    public Clutter.Actor surface;
    public Clutter.Stage stage;

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
            (surface as GtkClutter.Texture).set_from_pixbuf (pixbuf);

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

        stage.add_child (surface);
    }
}
