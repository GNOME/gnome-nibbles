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
 * grep -ne '[^][~)(}{_!$ "-](' *.vala
 * grep -ne '[(] ' *.vala
 * grep -ne '[ ])' *.vala
 * grep -ne ' $' *.vala
 *
 */
using Gtk;
using Gsk;
using Graphene;

[GtkTemplate (ui = "/org/gnome/Nibbles/ui/colourwheelsegment.ui")]
internal class ColourWheelSegment : Widget
{
    const double PIx2 = 6.28318530717958647692528676655900577; /* 2Pi */
    /* colour property */
    private ulong _colour;
    internal ulong colour {get {return _colour;} set {transparent = false; _colour = value;}}
    /* transparent */
    internal bool transparent = true; /* a transparent segment is never drawn */
    /* parent offset */
    public Point offset_from_parent; /* x & y position relative to parent */
    /* path */
    Gsk.Path segment_path; /* union of both the selected & unselected segment */
    /* instance initiliser */
    construct
    {
        can_focus = true;
        focus_on_click = true;
        focusable = true;
        sensitive = true;
    }
    public Gsk.Path get_path ()
    {
        return segment_path;
    }
    public override void snapshot (Snapshot snapshot)
    {
        if (!transparent)
        {
            int ID = ((ColourWheel)get_parent ()).GetVisibleID (this);
            var parent_width = ((ColourWheel)get_parent ()).get_width ();
            var parent_height = ((ColourWheel)get_parent ()).get_height ();
            double radius = parent_width > parent_height ? parent_height / 2.0 : parent_width / 2.0;
            double segment = PIx2 / ((ColourWheel)get_parent ()).GetVisibleSegmentCount ();
            /* path for segment */
            var p = new PathBuilder ();
            /* move to centre of the wheel */
            double cx = ((ColourWheel)get_parent ()).get_width () / 2.0 - offset_from_parent.x;
            double cy = ((ColourWheel)get_parent ()).get_height () / 2.0 - offset_from_parent.y;
            if (is_focus ())
            {
                cx += Math.sin (segment * ID + segment/2) * (radius / 10);
                cy -= Math.cos (segment * ID + segment/2) * (radius / 10);
            }
            p.move_to ((float)cx, (float)cy);
            /* line to start of arc */
            double x1 = Math.sin (segment * ID) * radius + cx;
            double y1 = -Math.cos (segment * ID) * radius + cy;
            p.line_to ((float)x1, (float)y1);
            /* arc */
            double x2 = Math.sin (segment * (ID + 1)) * radius + cx;
            double y2 = -Math.cos (segment * (ID + 1)) * radius + cy;
            p.svg_arc_to ((float)radius, (float)radius, (float)segment, false, true, (float)x2, (float)y2);
            /* line back to the center of the wheel */
            p.close ();
            /* fill the segment with our colour */
            snapshot.append_fill (p.to_path (), EVEN_ODD, { (_colour >> 16 & 0xff) / 255.0f, (_colour >> 8 & 0xff) / 255.0f, (_colour & 0xff) / 255.0f, 1});
        }
    }
    public Gsk.Path calculate_segment_path (uint width, uint height, uint ID, uint segment_count)
    {
        /* path for segment */
        var p = new PathBuilder ();
        /* move to centre of the wheel */
        p.move_to (width / 2.0f, height / 2.0f);
        /* line to start of arc (focus position) */
        var radius = width > height ? height / 2.0 : width / 2.0;
        var segment = PIx2 / segment_count;
        var cx = width / 2.0 + Math.sin (segment * ID + segment/2) * (radius / 10);
        var cy = height / 2.0 - Math.cos (segment * ID + segment/2) * (radius / 10);
        p.line_to ((float)(Math.sin (segment * ID) * radius + cx),
            (float)(-Math.cos (segment * ID) * radius + cy));
        /* arc (focus position) */
        p.svg_arc_to ((float)radius, (float)radius, (float)segment, false, true,
            (float)(Math.sin (segment * (ID + 1)) * radius + cx),
            (float)(-Math.cos (segment * (ID + 1)) * radius + cy));
        /* line back to the center of the wheel */
        p.close ();
        segment_path = p.to_path ();
        return segment_path;
    }
    protected override bool focus (DirectionType direction)
    {
        grab_focus ();
        queue_draw ();
        return true;
    }
}

[GtkTemplate (ui = "/org/gnome/Nibbles/ui/colourwheel.ui")]
internal class ColourWheel : Widget
{
    [GtkChild] private unowned ColourWheelSegment __initilisation_segment;
    /* call back function to return the selected segment */
    internal delegate void ResultFunction (int r);
    unowned ResultFunction result_function;
    /* mouse details */
    Point mouse_point;
    bool mouse_point_valid = false;
    bool mouse_depressed = false;
    /* IDs go from 0 to n for all visible segments */
    public int GetVisibleID (ColourWheelSegment s)
    {
        var p = get_first_child ();
        int id = 0;
        if (null != p)
        {
            if (s == p)
                return s.visible?id:-1;
            if (p.visible)
                id++;
            for (; (p = p.get_next_sibling ()) != null;)
            {
                if (p.visible)
                {
                    if (s == p)
                        return id;
                    id++;
                }
                else if (s == p)
                    return -1;
            }
        }
        return -1;
    }
    /* total count of visible segments */
    public uint GetVisibleSegmentCount ()
    {
        var p = get_first_child ();
        uint count = 0;
        if (null != p)
        {
            if (p.visible)
                count++;
            for (; (p = p.get_next_sibling ()) != null;)
            {
                if (p.visible)
                    count++;
            }
        }
        return count;
    }
    /* returns an array of visible segment objects */
    ColourWheelSegment[] get_visible_segments ()
    {
        ColourWheelSegment[] segments = {};
        var p = get_first_child ();
        if (null != p)
        {
            if (p.visible)
                segments += (ColourWheelSegment)p;
            for (; (p = p.get_next_sibling ()) != null;)
            {
                if (p.visible) segments += (ColourWheelSegment)p;
            }
        }
        return segments;
    }
    /* override the size_allocate virtual function to calculate path & bounds */
    protected override void size_allocate (int width, int height, int baseline)
    {
        var segments = get_visible_segments ();
        uint count = 0;
        foreach (var segment in segments)
        {
            var p = segment.calculate_segment_path (width, height, count, segments.length);
            Rect r;
            p.get_bounds (out r);
            segment.offset_from_parent = r.origin;
            segment.allocate_size ({ (int)r.origin.x, (int)r.origin.y, (int)r.size.width, (int)r.size.height}, baseline);
            count++;
        }
        base.size_allocate (width, height, baseline);
    }
    /* override the focus virtual function to enable forward/backward tabs
       and cursor keys */
    protected override bool focus (DirectionType direction)
    {
        var segment_count = GetVisibleSegmentCount ();
        int focus_id = null != get_focus_child () ?
            GetVisibleID ((ColourWheelSegment)get_focus_child ()) : -1;
        switch (direction)
        {
            case TAB_FORWARD:
                if (focus_id < 0)
                {
                    /* no focus */
                    set_focus_child (get_visible_segments ()[0]);
                    base.focus (direction);
                    get_visible_segments ()[0].queue_draw ();
                    return true;
                }
                else if (focus_id < segment_count - 1)
                {
                    set_focus_child (get_visible_segments ()[focus_id + 1]);
                    base.focus (direction);
                    get_visible_segments ()[focus_id].queue_draw ();
                    return true;
                }
                else
                {
                    /* last segment reached */
                    get_visible_segments ()[focus_id].queue_draw ();
                    return false;
                }
            case TAB_BACKWARD:
                if (focus_id < 0)
                {
                    /* no focus */
                    set_focus_child (get_visible_segments ()[segment_count - 1]);
                    base.focus (direction);
                    return true;
                }
                else if (focus_id > 0)
                {
                    set_focus_child (get_visible_segments ()[focus_id - 1]);
                    base.focus (direction);
                    get_visible_segments ()[focus_id].queue_draw ();
                    return true;
                }
                else
                {
                    /* first segment reached */
                    get_visible_segments ()[focus_id].queue_draw ();
                    return false;
                }
            case UP:
                if (focus_id < 0)
                {
                    /* no focus */
                    var segment_degrees = 360 / segment_count;
                    set_focus_child (get_visible_segments ()[180 / segment_degrees]);
                    base.focus (direction);
                    return true;
                }
                else if (focus_id == 0 || focus_id == segment_count - 1)
                {
                    /* top reached */
                    get_visible_segments ()[focus_id].queue_draw ();
                    return false;
                }
                else
                {
                    if (focus_id < segment_count / 2)
                        set_focus_child (get_visible_segments ()[focus_id - 1]);
                    else
                        set_focus_child (get_visible_segments ()[focus_id + 1]);
                    base.focus (direction);
                    get_visible_segments ()[focus_id].queue_draw ();
                    return true;
                }
            case DOWN:
                if (focus_id < 0)
                {
                    /* no focus */
                    set_focus_child (get_visible_segments ()[0]);
                    base.focus (direction);
                    return true;
                }
                else if ((segment_count & 0x1) == 0 && (focus_id == segment_count / 2 || focus_id == segment_count / 2 - 1)
                        || (segment_count & 0x1) == 1 && focus_id == segment_count / 2)
                {
                    /* bottom reached */
                    get_visible_segments ()[focus_id].queue_draw ();
                    return false;
                }
                else
                {
                    if (focus_id < segment_count / 2)
                        set_focus_child (get_visible_segments ()[focus_id + 1]);
                    else
                        set_focus_child (get_visible_segments ()[focus_id - 1]);
                    base.focus (direction);
                    get_visible_segments ()[focus_id].queue_draw ();
                    return true;
                }
            case LEFT:
                var segment_degrees = 360 / segment_count;
                if (focus_id < 0)
                {
                    /* no focus */
                    set_focus_child (get_visible_segments ()[90 / segment_degrees]);
                    base.focus (direction);
                    return true;
                }
                else if (focus_id == 270 / segment_degrees || 270 % segment_degrees == 0 && focus_id == 270 / segment_degrees - 1)
                {
                    /* left most reached */
                    get_visible_segments ()[focus_id].queue_draw ();
                    return false;
                }
                else
                {
                    if (focus_id < 270 / segment_degrees && focus_id >= 90 / segment_degrees)
                        set_focus_child (get_visible_segments ()[focus_id + 1]);
                    else
                        set_focus_child (get_visible_segments ()[focus_id > 0 ? focus_id - 1 : segment_count - 1]);
                    base.focus (direction);
                    get_visible_segments ()[focus_id].queue_draw ();
                    return true;
                }
            case RIGHT:
                var segment_degrees = 360 / segment_count;
                if (focus_id < 0)
                {
                    /* no focus */
                    set_focus_child (get_visible_segments ()[270 / segment_degrees]);
                    base.focus (direction);
                    return true;
                }
                else if (focus_id == 90 / segment_degrees || 90 % segment_degrees == 0 && focus_id == 90 / segment_degrees - 1)
                {
                    /* right most reached */
                    get_visible_segments ()[focus_id].queue_draw ();
                    return false;
                }
                else
                {
                    if (focus_id < 270 / segment_degrees && focus_id >= 90 / segment_degrees)
                        set_focus_child (get_visible_segments ()[focus_id - 1]);
                    else
                        set_focus_child (get_visible_segments ()[(focus_id + 1) % segment_count]);
                    base.focus (direction);
                    get_visible_segments ()[focus_id].queue_draw ();
                    return true;
                }
            default:
                if (null == get_is_focus_child ())
                    set_focus_child (get_visible_segments ()[0]);
                base.focus (direction);
                return true;
        }
    }
    /* return the one child segment with focus */
    public ColourWheelSegment ?get_is_focus_child ()
    {
        foreach (var s in get_visible_segments ())
            if (s.is_focus ())
                return s;
        return null;
    }
    /* return the segment the mouse pointer is over, if any */
    public ColourWheelSegment ?get_mouse_point_segment ()
    {
        if (mouse_point_valid)
            foreach (var s in get_visible_segments ())
                if (s.get_path ().in_fill (mouse_point, EVEN_ODD))
                    return s;
        return null;
    }
    /* initilise the widget and event controllers */
    construct
    {
        focusable = false;
        can_focus = true;
        sensitive = true;
        focus_on_click = true;
        can_target = true;
        assert_false (__initilisation_segment.visible);

        /* mouse position */
        var mouse_position = new EventControllerMotion ();
        mouse_position.motion.connect ((x,y)=>
        {
            mouse_point.x = (float)x;
            mouse_point.y = (float)y;
            mouse_point_valid = true;
            move_mouse ();
        });
        mouse_position.enter.connect ((x,y)=>
        {
            mouse_point.x = (float)x;
            mouse_point.y = (float)y;
            mouse_point_valid = true;
            move_mouse ();
        });
        mouse_position.leave.connect (()=>
        {
            mouse_point_valid = false;
            move_mouse ();
        });
        add_controller (mouse_position);

        /* mouse button click */
        var mouse_click = new EventControllerLegacy ();
        mouse_click.event.connect ((event)=>
        {
            switch (event.get_event_type ())
            {
                case Gdk.EventType.BUTTON_PRESS:
                    if (!mouse_depressed)
                    {
                        mouse_depressed = true;
                        move_mouse ();
                    }
                    return true;
                case Gdk.EventType.BUTTON_RELEASE:
                    var segment = get_mouse_point_segment ();
                    if (mouse_point_valid && null != segment)
                        select (segment);
                    else if (mouse_depressed)
                    {
                        mouse_depressed = false;
                        move_mouse ();
                    }
                    return true;
                default:
                    return false;
            }
        });
        add_controller (mouse_click);

        /* key presses */
        var key = new EventControllerKey ();
        key.key_released.connect ((/*uint*/keyval, /*uint*/ keycode, /*ModifierType*/ state)=>
        {
            switch (keyval)
            {
                case 65293: /* enter key */
                case 65421: /* keypad enter key */
                case 32: /* space key */
                    var segment = get_is_focus_child ();
                    if (null != segment)
                        select (segment);
                    break;
                default:
                    break;
            }
        });
        add_controller (key);
    }
    /* if the mouse button has been depressed select the segment its over */
    void move_mouse ()
    {
        if (mouse_point_valid)
        {
            var segment = get_mouse_point_segment ();
            if (null != segment)
            {
                if (mouse_depressed && !segment.is_focus ())
                {
                    var lose_focus = get_is_focus_child ();
                    if (null != lose_focus)
                        lose_focus.queue_draw ();
                    segment.grab_focus ();
                    segment.queue_draw ();
                }
                return;
            }
        }
        var lose_focus = get_is_focus_child ();
        if (null != lose_focus)
            lose_focus.queue_draw ();
    }
    /* return the selected segment via a call back function */
    void select (ColourWheelSegment segment)
    {
        result_function (GetVisibleID (segment));
    }
    /* start the colour wheel */
    public void do_select_segment (int current_selection, ResultFunction result_function)
    {
        this.result_function = result_function;
        mouse_point_valid = false;
        mouse_depressed = false;
        if (GetVisibleSegmentCount () > 0 && current_selection >= 0)
            get_visible_segments ()[current_selection].grab_focus ();
    }
    /* draw the colour wheel's segments and text message */
    public override void snapshot (Snapshot snapshot)
    {
        base.snapshot (snapshot);
        draw_label (snapshot, get_width (), get_height (),
        // Translators: text displayed in a message box directing the player to select the color they want for the worm
            _("Select Your Color"));
    }
    /* calculate the font size that fits in the space */
    int calculate_font_size (string text, int target_width, out double width, out double height)
    {
        bool rush_size_steps = true;
        int fail_count = 0;
        int last_font_size = 1;
        int target_font_size = 1;
        width = 0;
        height = 0;
        uint target_width_diff = uint.MAX;

        for (int font_size = 1;font_size < 128;)
        {
            var layout = create_pango_layout (text);
            var font = null == layout.get_font_description () ?
                Pango.FontDescription.from_string ("Sans Bold 1pt") :
                layout.get_font_description ().copy ();
            font.set_size (Pango.SCALE * font_size);
            layout.set_font_description (font);
            layout.set_text (text, -1);
            Pango.Rectangle a,b;
            layout.get_extents (out a, out b);
            uint width_diff = target_width - (int)(a.width / Pango.SCALE);
            if (width_diff > 0 && width_diff < target_width_diff)
            {
                target_width_diff = width_diff;
                target_font_size = font_size;
                width = a.width / Pango.SCALE;
                height = a.height / Pango.SCALE;
                if (!rush_size_steps)
                    fail_count = 0;
            }
            else
            {
                if (rush_size_steps)
                {
                    rush_size_steps = false;
                    font_size = last_font_size + 1;
                    fail_count = 0;
                }
                else if (fail_count > 2)
                    break;
                else
                    fail_count++;
            }
            if (rush_size_steps)
            {
                last_font_size = font_size;
                font_size *= 2;
            }
            else
                font_size++;
        }
        return target_font_size;
    }
    /* draw the text */
    void draw_text_font_size (Snapshot snapshot, int x, int y, string text, int font_size)
    {
        int x_offset, y_offset;
        get_text_offsets (text, font_size, out x_offset, out y_offset);
        snapshot.translate ({x - x_offset, y - y_offset});
        var layout = create_pango_layout (text);
        layout.set_alignment (1);
        var font = null == layout.get_font_description () ?
            Pango.FontDescription.from_string ("Sans Bold 1pt") :
            layout.get_font_description ().copy ();
        font.set_size (Pango.SCALE * font_size);
        layout.set_font_description (font);
        layout.set_text (text, -1);
        snapshot.append_layout (layout, {1, 1, 1, 1});
    }
    void get_text_offsets (string text, int font_size, out int x_offset, out int y_offset)
    {
        var layout = create_pango_layout (text);
        var font = null == layout.get_font_description () ?
            Pango.FontDescription.from_string ("Sans Bold 1pt") :
            layout.get_font_description ().copy ();
        font.set_size (Pango.SCALE * font_size);
        layout.set_font_description (font);
        layout.set_text (text, -1);
        Pango.Rectangle a,b;
        layout.get_extents (out a, out b);
        x_offset = a.x / Pango.SCALE;
        y_offset = a.y / Pango.SCALE;
    }
    void draw_label (Snapshot snapshot, double width, double height, string text)
    {
        const float PIby2 = 1.570796326794896619231321691639751442f; /* Pi / 2 */
        const int border = 20;
        double text_width;
        double text_height;
        int font_size = calculate_font_size (text, (int)width - border, out text_width, out text_height);
        float background_width = (float)text_width + border;
        float background_height = (float)text_height + border;
        float x = (get_width () - background_width) / 2;
        float y = 0;
        #if USE_PILL_BUTTON
        float arc_radius = background_width < background_height ? background_width / 2 : background_height / 2;
        #else
        float arc_radius = background_width < background_height ? background_width / 3 : background_height / 3;
        #endif
        /* draw background */
        var background = new PathBuilder ();
        /* top right corner */
        background.move_to (x + background_width - arc_radius, y + 0);
        background.svg_arc_to (arc_radius, arc_radius, PIby2, false, true, x + background_width, y + arc_radius);
        /* bottom right corner */
        background.line_to (x + background_width, y + background_height - arc_radius);
        background.svg_arc_to (arc_radius, arc_radius, PIby2, false, true, x + background_width - arc_radius, y + background_height);
        /* bottom left corner */
        background.line_to (x + arc_radius, y + background_height);
        background.svg_arc_to (arc_radius, arc_radius, PIby2, false, true, x + 0, y + background_height - arc_radius);
        /* top left corner */
        background.line_to (x + 0, y + arc_radius);
        background.svg_arc_to (arc_radius, arc_radius, PIby2, false, true, x + arc_radius, y + 0);
        /* fill with colour */
        snapshot.append_fill (background.to_path (), EVEN_ODD, {0.0f, 0.0f, 0.0f, 0.9f});
        /* draw the text */
        draw_text_font_size (snapshot, (int)(x + (background_width - text_width) / 2),
            (int)(y + (background_height - text_height) / 2), text, font_size);
    }
}
