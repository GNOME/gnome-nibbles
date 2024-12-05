/* -*- Mode: vala; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Gnome Nibbles: Gnome Worm Game
 * Copyright (C) 2015 Iulian-Gabriel Radu <iulian.radu67@gmail.com>
 * Copyright (C) 2022-24 Ben Corby <bcorby@new-ms.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Coding style.
 *
 * To help you comply with the coding style in this project use the
 * following greps. Any lines returned should be adjusted so they
 * don't match. The convoluted regular expressions are so they don't
 * match them self.
 *
 * grep -ne '[^][)(_!$ "](' *.vala
 * grep -ne '[(] ' *.vala
 * grep -ne '[ ])' *.vala
 * grep -ne ' $' *.vala
 *
 */
private class Bonus : Object
{
    internal enum eType
    {
        REGULAR,
        HALF,
        DOUBLE,
        LIFE,
        REVERSE,
        WARP;
    }
    public eType etype          { internal get; protected construct; }
    public bool fake            { internal get; protected construct; }
    public uint8 x              { internal get; protected construct; }
    public uint8 y              { internal get; protected construct; }
    public uint16 countdown     { internal get; internal construct set; }
    /* constructor */
    internal Bonus (uint8 x, uint8 y, Type type, bool fake, uint16 countdown)
    {
        Object (x: x, y: y, etype: type, fake: fake, countdown: countdown);
    }
}
