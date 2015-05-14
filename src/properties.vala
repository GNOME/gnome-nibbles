public class Properties : Object
{
    public int tile_size;

    public Properties ()
    {

    }

    public void update_settings (GLib.Settings settings)
    {
        settings.set_int ("tile-size", tile_size);
    }

    public void update_properties (GLib.Settings settings)
    {
        tile_size = settings.get_int ("tile-size");
    }
}
