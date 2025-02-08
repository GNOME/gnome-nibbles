/*
 * Gnome Nibbles: Gnome Worm Game
 * Copyright (C) 2025 Ben Corby <bcorby@new-ms.com>
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
/*
 * A simple transparent widget that can contain a single child widget.
 */
using Gtk; /* designed for Gtk 4, link with libgtk-4-dev or gtk4-devel */
public class TransparentContainer : Widget
{
    private Widget _child = null;
    public Widget child
    {
        get { return _child; }
        set
        {
            if (_child != value)
            {
                if (null != _child)
                    _child.unparent ();
                _child = value;
                if (null != _child)
                    _child.set_parent (this);
                queue_resize ();
            }
        }
    }
    protected override void dispose ()
    {
        if (null != child)
        {
            child.unparent ();
            child = null;
        }
        base.dispose ();
    }
    protected override void size_allocate (int width, int height, int baseline)
    {
        if (null != child && child.get_visible ())
            child.allocate_size ({ 0, 0, width, height}, baseline);
        base.size_allocate (width, height, baseline);
    }
}
