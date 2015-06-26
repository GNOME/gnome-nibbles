public class NibblesGame : Object
{
    public int tile_size;
    public int start_level;

    public const int MINIMUM_TILE_SIZE = 7;

    public const int DEFAULTGAMEDELAY = 35;
    public const int GAMEDELAY = 35;
    public const int NETDELAY = 2;
    public const int BONUSDELAY = 100;

    public const int NUMWORMS = 1;

    public const int WIDTH = 92;
    public const int HEIGHT = 66;

    public const char EMPTYCHAR = 'a';
    public const char WORMCHAR = 'w';

    public int current_level;
    public int[,] walls;

    public Gee.LinkedList<Worm> worms;

    public int numworms = NUMWORMS;

    public int game_speed = 4;

    public signal void worm_moved (Worm worm);

    public HashTable<Worm, WormProperties?> worm_props;

    public NibblesGame (Settings settings)
    {
        walls = new int[WIDTH, HEIGHT];
        worms = new Gee.LinkedList<Worm> ();
        worm_props = new HashTable<Worm, WormProperties?> (direct_hash, direct_equal);
        load_properties (settings);
    }

    public void start ()
    {
        add_worms ();
        Timeout.add (game_speed * (GAMEDELAY + NETDELAY), main_loop_cb);
    }

    public void add_worms ()
    {
        stderr.printf("[Debug] Loading worms\n");
        stderr.printf("[Debug] worms: %d\n", worms.size);
        foreach (var worm in worms) {
            stderr.printf("[Debug] worm size %d\n", worm.list.size);
            worm.spawn (walls);
        }
    }

    public void move_worms ()
    {
        foreach (var worm in worms)
        {
            if (worm.stop)
                continue;
            if (!worm.can_move_to (walls, numworms)) {
                stderr.printf("[Debug] died\n");
                worm.die (walls);
                continue;
            }

            worm.move (walls, true);
        }
    }

    public bool main_loop_cb ()
    {
        move_worms ();
        return Source.CONTINUE;
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

    public void load_worm_properties (Gee.ArrayList<Settings> worm_settings)
    {
        foreach (var worm in worms)
        {
            var properties = WormProperties ();
            properties.up = worm_settings[worm.id].get_int ("key-up");
            properties.down = worm_settings[worm.id].get_int ("key-down");
            properties.left = worm_settings[worm.id].get_int ("key-left");
            properties.right = worm_settings[worm.id].get_int ("key-right");

            worm_props.insert (worm, properties);
        }
    }

    public bool handle_keypress (uint keyval)
    {
        foreach (var worm in worms)
            if (worm.human)
                if (worm.handle_keypress (keyval, worm_props))
                    return true;

        return false;
    }
}
