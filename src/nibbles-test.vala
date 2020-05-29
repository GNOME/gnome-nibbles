/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-

   This file is part of GNOME Nibbles.

   Copyright (C) 2020 – Arnaud Bonatti <arnaud.bonatti@gmail.com>

   GNOME Nibbles is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   GNOME Nibbles is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNOME Nibbles.  If not, see <https://www.gnu.org/licenses/>.
*/

namespace NibblesTest
{
    private static int main (string [] args)
    {
        Test.init (ref args);
        Test.add_func ("/Nibbles/test tests",
                                 test_tests);
        Test.add_func ("/Nibbles/test games",
                                 test_games);
        return Test.run ();
    }

    private static void test_tests ()
    {
        assert_true (1 + 1 == 2);
    }

    /*\
    * * test games
    \*/

    private static void test_games ()
    {
        NibblesGame game = new NibblesGame (/* TODO tile size */ 42, /* start level */ 1, /* speed */ 0, /* fakes */ false, /* no random */ true);

        game.numhumans = 0;
        game.numai = 4;
        game.create_worms ();

        // FIXME adapted from nibbles-view.vala; should be in game.vala

        game.boni.reset (game.numworms);
        game.warp_manager.warps.clear ();

        string tmpboard;
        int count = 0;
        string [] level = level_008.split ("\n");
        for (int i = 0; i < NibblesGame.HEIGHT; i++)
        {
            tmpboard = level [i];
            for (int j = 0; j < NibblesGame.WIDTH; j++)
            {
                game.board[j, i] = tmpboard.@get(j);
                switch (game.board[j, i])
                {
                    case 'm':
                        game.board[j, i] = NibblesGame.EMPTYCHAR;
                        if (count < game.numworms)
                        {
                            game.worms[count].set_start (j, i, WormDirection.UP);
                            count++;
                        }
                        break;
                    case 'n':
                        game.board[j, i] = NibblesGame.EMPTYCHAR;
                        if (count < game.numworms)
                        {
                            game.worms[count].set_start (j, i, WormDirection.LEFT);
                            count++;
                        }
                        break;
                    case 'o':
                        game.board[j, i] = NibblesGame.EMPTYCHAR;
                        if (count < game.numworms)
                        {
                            game.worms[count].set_start (j, i, WormDirection.DOWN);
                            count++;
                        }
                        break;
                    case 'p':
                        game.board[j, i] = NibblesGame.EMPTYCHAR;
                        if (count < game.numworms)
                        {
                            game.worms[count].set_start (j, i, WormDirection.RIGHT);
                            count++;
                        }
                        break;
                    default:
                        break;
                }
            }
        }

        for (int i = 0; i < NibblesGame.HEIGHT; i++)
        {
            for (int j = 0; j < NibblesGame.WIDTH; j++)
            {
                switch (game.board[j, i])
                {
                    case '.': // empty space
                        game.board[j, i] = 'a';
                        break;
                    case 'Q':
                    case 'R':
                    case 'S':
                    case 'T':
                    case 'U':
                    case 'V':
                    case 'W':
                    case 'X':
                    case 'Y':
                    case 'Z':
                        game.warp_manager.add_warp (game.board, j - 1, i - 1, -(game.board[j, i]), 0);
                        break;
                    case 'r':
                    case 's':
                    case 't':
                    case 'u':
                    case 'v':
                    case 'w':
                    case 'x':
                    case 'y':
                    case 'z':
                        game.warp_manager.add_warp (game.board, -(game.board[j, i] - 'a' + 'A'), 0, j, i);
                        game.board[j, i] = NibblesGame.EMPTYCHAR;
                        break;
                    default:
                        break;
                }
            }
        }
        // END FIXME

        assert_true (game.numworms == 4);
        assert_true (game.worms.size == 4);

        uint8 applied_bonus = 0;
        game.bonus_applied.connect ((bonus, worm) => { applied_bonus++; Test.message (@"worm $(worm.id) took bonus at [$(bonus.x), $(bonus.y)]"); });

        game.add_worms ();
        game.start (/* add initial bonus */ true);

        assert_true (game.worms.@get (0).head.x ==  4 && game.worms.@get (0).head.y == 14);
        assert_true (game.worms.@get (1).head.x == 18 && game.worms.@get (1).head.y == 31);
        assert_true (game.worms.@get (2).head.x ==  9 && game.worms.@get (2).head.y == 39);
        assert_true (game.worms.@get (3).head.x == 51 && game.worms.@get (3).head.y == 45);

        // run until game is finished
        bool completed = false;
        game.level_completed.connect (() => { completed = true; });
        MainContext context = MainContext.@default ();
        while (!completed)
            context.iteration (/* may block */ false);

        assert_true (applied_bonus == 17);

        assert_true (game.worms.@get (0).lives == 6);
        assert_true (game.worms.@get (1).lives == 6);
        assert_true (game.worms.@get (2).lives == 6);
        assert_true (game.worms.@get (3).lives == 6);

        assert_true (game.worms.@get (0).score == 14);
        assert_true (game.worms.@get (1).score == 21);
        assert_true (game.worms.@get (2).score == 37);
        assert_true (game.worms.@get (3).score == 16);
    }

    private const string level_008 = "fccccccccccccccccccccccccccccccccccccccce........dcccccccccccccccccccccccccccccccccccccccccg"
                            + "\n" + "b..........................................................................................b"
                            + "\n" + "b..........................................................................................b"
                            + "\n" + "b..........................................................................................b"
                            + "\n" + "b...R...............................fg...........................fg.....................S..b"
                            + "\n" + "b...u...............................dlg..........................bb....................t...b"
                            + "\n" + "b....................................dlg.........................bb........................b"
                            + "\n" + "b...........fg........................dlg........................bb........................b"
                            + "\n" + "b...........bb.........................dlg.......................bb........................b"
                            + "\n" + "b...o.......bb..........................dlg......................bb........................b"
                            + "\n" + "b...........bb...........................dlg.....................bb...fccccccccg...........b"
                            + "\n" + "b...........bb............................dlg....................bb...dcccccccclg..........b"
                            + "\n" + "b...........bb.............................dlg...................bb............dlg.........b"
                            + "\n" + "b...........bb..............................dlg..................bb.............dlg........b"
                            + "\n" + "b...........bb............fg.................dlg.................bb..............dlccg.....b"
                            + "\n" + "b...........bb...........fle..................dlg................bb...............dcce.....b"
                            + "\n" + "b...........bb..........fle....................dlg...............bb........................b"
                            + "\n" + "e...........de.........fle......................dlg..............bb........................d"
                            + "\n" + "......................fle........................dlg.............bb........................."
                            + "\n" + ".....................fle.........................fle.............bb........................."
                            + "\n" + "....................fle.........................fle..............bb........................."
                            + "\n" + "...................fle.........................fle...............bb........................."
                            + "\n" + "..................fle.........................fle................bb......fg................."
                            + "\n" + "..................de.........................fle.................bb......dlg................"
                            + "\n" + "............................................fle..................bb.......dlg..............."
                            + "\n" + "............................................dlg..................bb........dlg.............."
                            + "\n" + "g.................o..........................dlg.................bb.........dlg............f"
                            + "\n" + "b.............................................dlg................bb..........dlg...........b"
                            + "\n" + "b..............................................dlg...............bb...........dlg..........b"
                            + "\n" + "b........fg.....................fg..............dlg..............bb............de..........b"
                            + "\n" + "b........bb.....................dlg..............dlg.............bb........................b"
                            + "\n" + "b........bb......................dlg..............dlg............bb........................b"
                            + "\n" + "b........bb.......................dlg..............dlg...........bb........................b"
                            + "\n" + "b........bb........................dlg..............de...........bb........................b"
                            + "\n" + "b........bb.........................dlg..........................bb........................b"
                            + "\n" + "b........bdcccccccg..................dlg.........................bb........................b"
                            + "\n" + "b........dcccccccce...................dlg........................bb........................b"
                            + "\n" + "b......................................dlg.......................bb........................b"
                            + "\n" + "b.......................................dlg......................bb....fcccccccccccccg.....b"
                            + "\n" + "b...p....................................dlg.....................bb....dccccccccccccce.....b"
                            + "\n" + "b........................................fle.....................bb........................b"
                            + "\n" + "b.......................................fle......................bb........................b"
                            + "\n" + "b.................fg...................fle.......................bb........................b"
                            + "\n" + "b.................bb..................fle........................bb........................b"
                            + "\n" + "b...........fg....bb.................fle.........................bb........................b"
                            + "\n" + "b..........fle....bb................fle..........................bb........................b"
                            + "\n" + "b.........fle.....bb...............fle...........................bb........................b"
                            + "\n" + "b........fle......bb...............de............................bb........................b"
                            + "\n" + "b.......fle.......bb.............................................bb........................b"
                            + "\n" + "b......fle........bb.............................................bb........................b"
                            + "\n" + "b......de.........bb...............................m.............bb........................b"
                            + "\n" + "b.................bb.............................................bb........................b"
                            + "\n" + "b.................bb.............................................bb........................b"
                            + "\n" + "b.................bb.............................................bdcccccccccccccg..........b"
                            + "\n" + "b.................bb............fccccccccccccccccccccccg.........dcccccccccccccce..........b"
                            + "\n" + "b.................bb............dcccccccccccccccccccccce...................................b"
                            + "\n" + "b.................bb.......................................................................b"
                            + "\n" + "b.................bb...................................................................n...b"
                            + "\n" + "b.................bb.......................................................................b"
                            + "\n" + "b.................bb......................n................................................b"
                            + "\n" + "b...s.............bb...................................................................r...b"
                            + "\n" + "b.................de.......................................................................b"
                            + "\n" + "b...T...................................................................................U..b"
                            + "\n" + "b..........................................................................................b"
                            + "\n" + "b..........................................................................................b"
                            + "\n" + "dcccccccccccccccccccccccccccccccccccccccg........fccccccccccccccccccccccccccccccccccccccccce";
}
