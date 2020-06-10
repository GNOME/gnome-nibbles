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
        NibblesGame game = new NibblesGame (/* start level */ 1, /* speed */ 0, /* fakes */ false, /* no random */ true);

        game.numhumans = 0;
        game.numai = 4;
        game.create_worms ();

        game.load_board (level_008);

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

    private const string [] level_008 = {
            "fccccccccccccccccccccccccccccccccccccccce........dcccccccccccccccccccccccccccccccccccccccccg",
            "b..........................................................................................b",
            "b..........................................................................................b",
            "b..........................................................................................b",
            "b...R...............................fg...........................fg.....................S..b",
            "b...u...............................dlg..........................bb....................t...b",
            "b....................................dlg.........................bb........................b",
            "b...........fg........................dlg........................bb........................b",
            "b...........bb.........................dlg.......................bb........................b",
            "b...o.......bb..........................dlg......................bb........................b",
            "b...........bb...........................dlg.....................bb...fccccccccg...........b",
            "b...........bb............................dlg....................bb...dcccccccclg..........b",
            "b...........bb.............................dlg...................bb............dlg.........b",
            "b...........bb..............................dlg..................bb.............dlg........b",
            "b...........bb............fg.................dlg.................bb..............dlccg.....b",
            "b...........bb...........fle..................dlg................bb...............dcce.....b",
            "b...........bb..........fle....................dlg...............bb........................b",
            "e...........de.........fle......................dlg..............bb........................d",
            "......................fle........................dlg.............bb.........................",
            ".....................fle.........................fle.............bb.........................",
            "....................fle.........................fle..............bb.........................",
            "...................fle.........................fle...............bb.........................",
            "..................fle.........................fle................bb......fg.................",
            "..................de.........................fle.................bb......dlg................",
            "............................................fle..................bb.......dlg...............",
            "............................................dlg..................bb........dlg..............",
            "g.................o..........................dlg.................bb.........dlg............f",
            "b.............................................dlg................bb..........dlg...........b",
            "b..............................................dlg...............bb...........dlg..........b",
            "b........fg.....................fg..............dlg..............bb............de..........b",
            "b........bb.....................dlg..............dlg.............bb........................b",
            "b........bb......................dlg..............dlg............bb........................b",
            "b........bb.......................dlg..............dlg...........bb........................b",
            "b........bb........................dlg..............de...........bb........................b",
            "b........bb.........................dlg..........................bb........................b",
            "b........bdcccccccg..................dlg.........................bb........................b",
            "b........dcccccccce...................dlg........................bb........................b",
            "b......................................dlg.......................bb........................b",
            "b.......................................dlg......................bb....fcccccccccccccg.....b",
            "b...p....................................dlg.....................bb....dccccccccccccce.....b",
            "b........................................fle.....................bb........................b",
            "b.......................................fle......................bb........................b",
            "b.................fg...................fle.......................bb........................b",
            "b.................bb..................fle........................bb........................b",
            "b...........fg....bb.................fle.........................bb........................b",
            "b..........fle....bb................fle..........................bb........................b",
            "b.........fle.....bb...............fle...........................bb........................b",
            "b........fle......bb...............de............................bb........................b",
            "b.......fle.......bb.............................................bb........................b",
            "b......fle........bb.............................................bb........................b",
            "b......de.........bb...............................m.............bb........................b",
            "b.................bb.............................................bb........................b",
            "b.................bb.............................................bb........................b",
            "b.................bb.............................................bdcccccccccccccg..........b",
            "b.................bb............fccccccccccccccccccccccg.........dcccccccccccccce..........b",
            "b.................bb............dcccccccccccccccccccccce...................................b",
            "b.................bb.......................................................................b",
            "b.................bb...................................................................n...b",
            "b.................bb.......................................................................b",
            "b.................bb......................n................................................b",
            "b...s.............bb...................................................................r...b",
            "b.................de.......................................................................b",
            "b...T...................................................................................U..b",
            "b..........................................................................................b",
            "b..........................................................................................b",
            "dcccccccccccccccccccccccccccccccccccccccg........fccccccccccccccccccccccccccccccccccccccccce"
        };
}
