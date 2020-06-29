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
        Test.add_func ("/Nibbles/test heads",
                                 test_heads);
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
        NibblesGame game = new NibblesGame (/* start level */ 1, /* speed */ 0, /* fakes */ false, level_008_width, level_008_height, /* no random */ true);

        game.numhumans = 0;
        game.numai = 4;
        game.create_worms ();

        game.load_board (level_008, /* regular bonus = 8 + numworms */ 12);

        ulong [] worms_handlers = new ulong [game.worms.size];
        foreach (Worm worm in game.worms)
            // FIXME we should not have to connect to anything 1/2
            worms_handlers [worm.id] = worm.finish_added.connect (() => { worm.dematerialize (game.board, 3); worm.is_stopped = false; });

        assert_true (game.numworms == 4);
        assert_true (game.worms.size == 4);

        uint8 applied_bonus = 0;
        ulong game_handler_1 = game.bonus_applied.connect ((bonus, worm) => { applied_bonus++; Test.message (@"worm $(worm.id) took bonus at [$(bonus.x), $(bonus.y)]"); });

        game.add_worms ();
        game.start (/* add initial bonus */ true);

        assert_true (game.worms.@get (0).head.x ==  4 && game.worms.@get (0).head.y == 14);
        assert_true (game.worms.@get (1).head.x == 18 && game.worms.@get (1).head.y == 31);
        assert_true (game.worms.@get (2).head.x ==  9 && game.worms.@get (2).head.y == 39);
        assert_true (game.worms.@get (3).head.x == 51 && game.worms.@get (3).head.y == 45);

        // run until game is finished
        bool completed = false;
        ulong game_handler_2 = game.level_completed.connect (() => { completed = true; });
        MainContext context = MainContext.@default ();
        while (!completed)
            context.iteration (/* may block */ false);

        assert_true (applied_bonus == 15);

        assert_true (game.worms.@get (0).lives == 6);
        assert_true (game.worms.@get (1).lives == 5);
        assert_true (game.worms.@get (2).lives == 6);
        assert_true (game.worms.@get (3).lives == 6);

        assert_true (game.worms.@get (0).score ==  11);
        assert_true (game.worms.@get (1).score ==  14);
        assert_true (game.worms.@get (2).score == 119);
        assert_true (game.worms.@get (3).score ==  19);

        foreach (Worm worm in game.worms)
            worm.disconnect (worms_handlers [worm.id]);
        game.disconnect (game_handler_1);
        game.disconnect (game_handler_2);
    }

    private const int level_008_width  = 92;
    private const int level_008_height = 66;
    private const string [] level_008  = {
            "┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛........┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓",
            "┃..........................................................................................┃",
            "┃..........................................................................................┃",
            "┃..........................................................................................┃",
            "┃...R...............................┏┓...........................┏┓.....................S..┃",
            "┃...u...............................┗╋┓..........................┃┃....................t...┃",
            "┃....................................┗╋┓.........................┃┃........................┃",
            "┃...........┏┓........................┗╋┓........................┃┃........................┃",
            "┃...........┃┃.........................┗╋┓.......................┃┃........................┃",
            "┃...▼.......┃┃..........................┗╋┓......................┃┃........................┃",
            "┃...........┃┃...........................┗╋┓.....................┃┃...┏━━━━━━━━┓...........┃",
            "┃...........┃┃............................┗╋┓....................┃┃...┗━━━━━━━━╋┓..........┃",
            "┃...........┃┃.............................┗╋┓...................┃┃............┗╋┓.........┃",
            "┃...........┃┃..............................┗╋┓..................┃┃.............┗╋┓........┃",
            "┃...........┃┃............┏┓.................┗╋┓.................┃┃..............┗╋━━┓.....┃",
            "┃...........┃┃...........┏╋┛..................┗╋┓................┃┃...............┗━━┛.....┃",
            "┃...........┃┃..........┏╋┛....................┗╋┓...............┃┃........................┃",
            "┛...........┗┛.........┏╋┛......................┗╋┓..............┃┃........................┗",
            "......................┏╋┛........................┗╋┓.............┃┃.........................",
            ".....................┏╋┛.........................┏╋┛.............┃┃.........................",
            "....................┏╋┛.........................┏╋┛..............┃┃.........................",
            "...................┏╋┛.........................┏╋┛...............┃┃.........................",
            "..................┏╋┛.........................┏╋┛................┃┃......┏┓.................",
            "..................┗┛.........................┏╋┛.................┃┃......┗╋┓................",
            "............................................┏╋┛..................┃┃.......┗╋┓...............",
            "............................................┗╋┓..................┃┃........┗╋┓..............",
            "┓.................▼..........................┗╋┓.................┃┃.........┗╋┓............┏",
            "┃.............................................┗╋┓................┃┃..........┗╋┓...........┃",
            "┃..............................................┗╋┓...............┃┃...........┗╋┓..........┃",
            "┃........┏┓.....................┏┓..............┗╋┓..............┃┃............┗┛..........┃",
            "┃........┃┃.....................┗╋┓..............┗╋┓.............┃┃........................┃",
            "┃........┃┃......................┗╋┓..............┗╋┓............┃┃........................┃",
            "┃........┃┃.......................┗╋┓..............┗╋┓...........┃┃........................┃",
            "┃........┃┃........................┗╋┓..............┗┛...........┃┃........................┃",
            "┃........┃┃.........................┗╋┓..........................┃┃........................┃",
            "┃........┃┗━━━━━━━┓..................┗╋┓.........................┃┃........................┃",
            "┃........┗━━━━━━━━┛...................┗╋┓........................┃┃........................┃",
            "┃......................................┗╋┓.......................┃┃........................┃",
            "┃.......................................┗╋┓......................┃┃....┏━━━━━━━━━━━━━┓.....┃",
            "┃...▶....................................┗╋┓.....................┃┃....┗━━━━━━━━━━━━━┛.....┃",
            "┃........................................┏╋┛.....................┃┃........................┃",
            "┃.......................................┏╋┛......................┃┃........................┃",
            "┃.................┏┓...................┏╋┛.......................┃┃........................┃",
            "┃.................┃┃..................┏╋┛........................┃┃........................┃",
            "┃...........┏┓....┃┃.................┏╋┛.........................┃┃........................┃",
            "┃..........┏╋┛....┃┃................┏╋┛..........................┃┃........................┃",
            "┃.........┏╋┛.....┃┃...............┏╋┛...........................┃┃........................┃",
            "┃........┏╋┛......┃┃...............┗┛............................┃┃........................┃",
            "┃.......┏╋┛.......┃┃.............................................┃┃........................┃",
            "┃......┏╋┛........┃┃.............................................┃┃........................┃",
            "┃......┗┛.........┃┃...............................▲.............┃┃........................┃",
            "┃.................┃┃.............................................┃┃........................┃",
            "┃.................┃┃.............................................┃┃........................┃",
            "┃.................┃┃.............................................┃┗━━━━━━━━━━━━━┓..........┃",
            "┃.................┃┃............┏━━━━━━━━━━━━━━━━━━━━━━┓.........┗━━━━━━━━━━━━━━┛..........┃",
            "┃.................┃┃............┗━━━━━━━━━━━━━━━━━━━━━━┛...................................┃",
            "┃.................┃┃.......................................................................┃",
            "┃.................┃┃...................................................................◀...┃",
            "┃.................┃┃.......................................................................┃",
            "┃.................┃┃......................◀................................................┃",
            "┃...s.............┃┃...................................................................r...┃",
            "┃.................┗┛.......................................................................┃",
            "┃...T...................................................................................U..┃",
            "┃..........................................................................................┃",
            "┃..........................................................................................┃",
            "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓........┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛"
        };

    /*\
    * * test heads
    \*/

    private static void test_heads ()
    {
        Test.message ("test heads 1");
        _test_heads (test_heads_1, /* worm 0 */ 6, 4, /* worm 1 */ 11, 4, /* lives */ 6, 6);

        Test.message ("test heads 2");
        _test_heads (test_heads_2, /* worm 0 */ 6, 4, /* worm 1 */ 11, 4, /* lives */ 6, 5);

        Test.message ("test heads 3");
        _test_heads (test_heads_3, /* worm 0 */ 6, 4, /* worm 1 */ 10, 4, /* lives */ 6, 6);

        Test.message ("test heads 4");
        _test_heads (test_heads_4, /* worm 0 */ 6, 4, /* worm 1 */ 10, 4, /* lives */ 0, 0);

        Test.message ("test heads 5");
        _test_heads (test_heads_5, /* worm 0 */ 6, 1, /* worm 1 */  6, 4, /* lives */ 6, 6);

        Test.message ("test heads 6");
        _test_heads (test_heads_6, /* worm 0 */ 6, 1, /* worm 1 */  6, 4, /* lives */ 4, 6);

        Test.message ("test heads 9");
        _test_heads (test_heads_9, /* worm 0 */ 6, 1, /* worm 1 */  6, 4, /* lives */ 6, 5);

        Test.message ("test heads 7");
        _test_heads (test_heads_7, /* worm 0 */ 6, 2, /* worm 1 */  6, 4, /* lives */ 6, 6);

        Test.message ("test heads 8");
        _test_heads (test_heads_8, /* worm 0 */ 6, 2, /* worm 1 */  6, 4, /* lives */ 0, 0);

        Test.message ("test heads 0");
        _test_heads (test_heads_0, /* worm 0 */ 6, 2, /* worm 1 */  6, 4, /* lives */ 6, 6);
    }

    private static void _test_heads (string [] board,
                                     int worm_0_x,
                                     int worm_0_y,
                                     int worm_1_x,
                                     int worm_1_y,
                                     int first_worm_lives,
                                     int second_worm_lives)
    {
        NibblesGame game = new NibblesGame (/* start level */ 0, /* speed */ 0, /* fakes */ false, test_heads_width, test_heads_height, /* no random */ true);

        game.numhumans = 0;
        game.numai = 2;
        game.create_worms ();

        game.load_board (board, /* regular bonus */ 1);

        ulong [] worms_handlers = new ulong [game.worms.size];
        foreach (Worm worm in game.worms)
            // FIXME we should not have to connect to anything 2/2
            worms_handlers [worm.id] = worm.finish_added.connect (() => { worm.dematerialize (game.board, 3); worm.is_stopped = false; });

        assert_true (game.numworms == 2);
        assert_true (game.worms.size == 2);

        ulong game_handler_1 = game.bonus_applied.connect ((bonus, worm) => { Test.message (@"worm $(worm.id) took bonus at [$(bonus.x), $(bonus.y)]"); });

        game.add_worms ();
        game.start (/* add initial bonus */ true);

        assert_true (game.worms.@get (0).lives == 6);
        assert_true (game.worms.@get (1).lives == 6);

        assert_true (game.worms.@get (0).score == 0);
        assert_true (game.worms.@get (1).score == 0);

        assert_true (game.worms.@get (0).head.x == worm_0_x && game.worms.@get (0).head.y == worm_0_y);
        assert_true (game.worms.@get (1).head.x == worm_1_x && game.worms.@get (1).head.y == worm_1_y);

        // run until game is finished
        bool completed = false;
        ulong game_handler_2 = game.level_completed.connect (() => { completed = true; });
        MainContext context = MainContext.@default ();
        do context.iteration (/* may block */ false);
        while (!completed && (game.get_game_status () != GameStatus.GAMEOVER));

        assert_true (game.worms.@get (0).lives == first_worm_lives);
        assert_true (game.worms.@get (1).lives == second_worm_lives);

        // FIXME looks like last bonus is not counted...
        assert_true (game.worms.@get (0).score == 0);
        assert_true (game.worms.@get (1).score == 0);

        foreach (Worm worm in game.worms)
            worm.disconnect (worms_handlers [worm.id]);
        game.disconnect (game_handler_1);
        game.disconnect (game_handler_2);
    }

    private const int test_heads_width = 18;
    private const int test_heads_height = 6;
    private const string [] test_heads_1 = {
            "┏━━━━━━━━━━━━━━━━┓",
            "┃................┃",
            "┃................┃",
            "┣━━━━━━━..━━━━━━━┫",
            "┃▶..............◀┃",
            "┗━━━━━━━━━━━━━━━━┛"
        };  /* expected: 6, 6 */
    private const string [] test_heads_2 = {
            "┏━━━━━━━━━━━━━━━━┓",
            "┃................┃",
            "┃................┃",
            "┣━━━━━━━.━━━━━━━━┫",
            "┃▶..............◀┃",
            "┗━━━━━━━━━━━━━━━━┛"
        };  /* expected: 6, 5 */
    private const string [] test_heads_3 = {
            "┏━━━━━━━━━━━━━━━━┓",
            "┃................┃",
            "┃................┃",
            "┣━━━━━━━..━━━━━━┳┫",
            "┃▶.............◀┣┫",
            "┗━━━━━━━━━━━━━━━┻┛"
        };  /* expected: 6, 6 */
    private const string [] test_heads_4 = {
            "┏━━━━━━━━━━━━━━━━┓",
            "┃................┃",
            "┃................┃",
            "┣━━━━━━━.━━━━━━━┳┫",
            "┃▶.............◀┣┫",
            "┗━━━━━━━━━━━━━━━┻┛"
        };  /* expected: 0, 0 */
    private const string [] test_heads_5 = {
            "┏━━━━━━━┳━━━━━━━━┓",
            "┃▶......┃........┃",
            "┗━━━━━┓..........┃",
            "┏━━━━━┛..........┃",
            "┃▶......┃........┃",
            "┗━━━━━━━┻━━━━━━━━┛"
        };  /* expected: 6, 6 */
    private const string [] test_heads_6 = {
            "┏━━━━━━━┳━━━━━━━━┓",
            "┃▶......┃........┃",
            "┗━━━━━┓.┃........┃",
            "┏━━━━━┛..........┃",
            "┃▶......┃........┃",
            "┗━━━━━━━┻━━━━━━━━┛"
        };  /* expected: 4, 6 */
    private const string [] test_heads_9 = {
            "┏━━━━━━━┳━━━━━━━━┓",
            "┃▶......┃........┃",
            "┗━━━━━┓..........┃",
            "┏━━━━━┛.┃........┃",
            "┃▶......┃........┃",
            "┗━━━━━━━┻━━━━━━━━┛"
        };  /* expected: 6, 5 */
    private const string [] test_heads_7 = {
            "........┏━━━━━━━━┓",
            "┏━━━━━━━┛........┃",
            "┃▶...............┃",
            "┣━━━━━━..........┃",
            "┃▶......┃........┃",
            "┗━━━━━━━┻━━━━━━━━┛"
        };  /* expected: 6, 6 */
    private const string [] test_heads_8 = {
            "........┏━━━━━━━━┓",
            "┏━━━━━━━┫........┃",
            "┃▶......┃........┃",
            "┣━━━━━━..........┃",
            "┃▶......┃........┃",
            "┗━━━━━━━┻━━━━━━━━┛"
        };  /* expected: 0, 0 */
    private const string [] test_heads_0 = {
            "........┏━━━━━━━━┓",
            "┏━━━━━━━┫........┃",
            "┃▶......┃........┃",
            "┣━━━━━━..........┃",
            "┃▶...............┃",
            "┗━━━━━━━━━━━━━━━━┛"
        };  /* expected: 6, 6 */
}
