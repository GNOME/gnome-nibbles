using Gtk;

public class Nibbles : Gtk.Application
{
    private GLib.Settings settings;

    private bool is_maximized;
    private bool is_tiled;
    private int window_width;
    private int window_height;

    private ApplicationWindow window;
    private HeaderBar headerbar;

    private const GLib.ActionEntry action_entries[] =
    {
        {"quit", quit}
    };

    public Nibbles ()
    {
        Object (application_id: "org.gnome.nibbles", flags: ApplicationFlags.FLAGS_NONE);
    }

    protected override void startup ()
    {
        base.startup ();

        add_action_entries (action_entries, this);

        settings = new GLib.Settings ("org.gnome.nibbles");

        set_accels_for_action ("app.quit", {"<Primary>q"});

        var builder = new Builder.from_resource ("/org/gnome/nibbles/ui/gnome-nibbles.ui");
        window = builder.get_object ("nibbles-window") as ApplicationWindow;
        window.size_allocate.connect (size_allocate_cb);
        window.window_state_event.connect (window_state_event_cb);
        window.set_default_size (settings.get_int ("window-width"), settings.get_int ("window-height"));
        if (settings.get_boolean ("window-is-maximized"))
            window.maximize ();

        headerbar = builder.get_object ("headerbar") as HeaderBar;
        window.set_titlebar (headerbar);

        add_window (window);
    }

    protected override void activate ()
    {
        window.present ();
    }

    protected override void shutdown ()
    {
        settings.set_int ("window-width", window_width);
        settings.set_int ("window-height", window_height);
        settings.set_boolean ("window-is-maximized", is_maximized);

        base.shutdown ();
    }

    private void size_allocate_cb (Allocation allocation)
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

    public static int main (string[] args)
    {
        return new Nibbles ().run (args);
    }
}
