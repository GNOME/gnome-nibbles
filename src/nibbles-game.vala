public class NibblesGame : Object
{
    public Properties properties;

    public int width = 92;
    public int height = 66;

    public char EMPTYCHAR = 'a';
    public char WORMCHAR = 'w';

    public int current_level;
    public int[,] walls;

    public NibblesGame ()
    {
        properties = new Properties ();
        walls = new int[width, height];
    }
}
