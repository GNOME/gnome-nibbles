public class NibblesGame : Object
{
    public int tile_size;
    public int start_level;

    public int DEFAULTGAMEDELAY = 35;
    public int GAMEDELAY = 35;
    public int NETDELAY = 2;
    public int BONUSDELAY = 100;

    public int width = 92;
    public int height = 66;

    public char EMPTYCHAR = 'a';
    public char WORMCHAR = 'w';

    public int current_level;
    public int[,] walls;

    public NibblesGame (Settings settings)
    {
        walls = new int[width, height];
        load_properties (settings);
    }

    public void load_properties (Settings settings)
    {
        tile_size = settings.get_int ("tile-size");
        start_level = settings.get_int ("start-level");
    }

    public void save_properties (Settings settings)
    {
        tile_size = settings.get_int ("tile-size");
        start_level = settings.get_int ("start-level");
    }
}
