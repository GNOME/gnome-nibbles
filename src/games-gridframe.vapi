[CCode (cheader_filename = "games-gridframe.h")]
public class GamesGridFrame : Gtk.Bin {
    public int xmult;
    public int ymult;
    public int xpadding;
    public int ypadding;
    public float xalign;
    public float yalign;
    public Gtk.Allocation old_allocation;
    [CCode (cname = "games_grid_frame_new")]
    public GamesGridFrame (int width, int height);
    public void set (int width, int height);
    public void set_padding (int xpadding, int ypadding);
    public void set_alignment (float xalign, float yalign);
}

[CCode (cheader_filename = "games-gridframe.h")]
public enum Prop {
    X_PADDING,
    Y_PADDING,
    WIDTH,
    HEIGHT,
    X_ALIGN,
    Y_ALIGN
}
