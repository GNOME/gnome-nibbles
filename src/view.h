/*
 * This file is part of GNOME Nibbles.
 *
 * Copyright (C) 2026 Ben Corby
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

#pragma once


class View : public Gtk::Overlay
{
/* sub classes */
private:
	class TimeCallBack
	{
		sigc::connection timer;
		bool timer_set=false;
	public:
		TimeCallBack() = default;
		~TimeCallBack()
		{
			unset();
		}
		/*TimeCallBack &operator=(sigc::connection t)
		{
			timer=t;
			timer_set=true;
		}*/
		void set(sigc::bound_mem_functor<bool (View::*)()> function, unsigned long delay)
		{
			timer=Glib::signal_timeout().connect(function, delay);
			timer_set=true;
		}
		void unset()
		{
			if(timer_set && timer.connected())
				timer.disconnect();
			timer_set=false;
		}
	};
	class StaticView : public Gtk::Widget
	{
	public:
		StaticView(View &view) : view(view)
		{
			set_hexpand(true);
			set_vexpand(true);
		}
		virtual ~StaticView() override = default;
		void redraw () {queue_draw ();}
	protected:
		/*
		void measure_vfunc(Gtk::Orientation orientation, int for_size, int& minimum, int& natural,
			int& minimum_baseline, int& natural_baseline) const override
		{
			if (orientation == Gtk::Orientation::HORIZONTAL)
			{
				minimum = 35;
				natural = 35;
			}
			else
			{
				minimum = 35;
				natural = 35;
			}

			// Don't use baseline alignment.
			minimum_baseline = -1;
			natural_baseline = -1;
		}*/
	 	void snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>& snapshot) override
	 	{
            const double max_delta_deviation = 1.15;
            int x_delta = get_width() / view.game.get_width();
            int y_delta = get_height() / view.game.get_height();
            if (x_delta > max_delta_deviation * y_delta)
                x_delta = (int)(y_delta * max_delta_deviation);
            else if (y_delta > max_delta_deviation * x_delta)
                y_delta = (int)(x_delta * max_delta_deviation);
            int x_offset = (get_width () - x_delta * view.game.get_width()) / 2;
            int y_offset = (get_height () - y_delta * view.game.get_height()) / 2;

            // black background 
			auto background = Gsk::PathBuilder::create();
			if(view.is_fullscreen_active ())
				background->add_rect({0,0,get_width(),get_height()});
			else
				background->add_rect({x_offset,y_offset,x_delta*view.game.get_width(),y_delta*view.game.get_height()});
			snapshot->append_fill(background->to_path(), Gsk::FillRule::EVEN_ODD, {0,0,0,1});

            // draw walls 
            for (int x = 0; x < view.game.get_width(); x++)
            {
                for (int y = 0; y < view.game.get_height(); y++)
                {
                    // walls 
                    if (view.game[x,y] >= 'b' && view.game[x,y] <= 'l')
                        draw_wall_segment (view.game[x,y],
                            snapshot, x_delta * x + x_offset, y_delta * y + y_offset, x_delta, y_delta);
                }
            }
	 	}
	private:
		View &view;
		void draw_wall_segment (char i, const Glib::RefPtr<Gtk::Snapshot>& s,
			int x, int y, int x_size, int y_size)
		{
		    int x_s13 = x_size / 3;
		    int x_remainder = x_size - x_s13 * 3;
		    int y_s13 = y_size / 3;
		    int y_remainder = y_size - y_s13 * 3;
		    if (i >= 'b' && i <= 'l')
		    {
		        /* center square */
		        auto center_square = Gsk::PathBuilder::create();
		        center_square->add_rect ({x_remainder == 2 ? x_s13 + x + x_remainder : x_s13 + x,
		            y_remainder == 2 ? y_s13 + y + y_remainder : y_s13 + y,
		            x_remainder == 2 ? x_s13 : x_s13 + x_remainder,
		            y_remainder == 2 ? y_s13 : y_s13 + y_remainder});
		        s->append_fill (center_square->to_path (), Gsk::FillRule::EVEN_ODD, {0.5f, 0.5f, 0.5f, 1.0f});
		    }
		    if (i == 'b' || i == 'd' || i == 'e' || i == 'h' || i == 'i' || i == 'j' || i == 'l')
		    {
		        /* top square */
		        auto top_square = Gsk::PathBuilder::create();
		        top_square->add_rect ({x_remainder == 2 ? x_s13 + x + x_remainder : x_s13 + x,
		            y,
		            x_remainder == 2 ? x_s13 : x_s13 + x_remainder,
		            y_remainder == 2 ? y_s13 + y_remainder : y_s13});
		        s->append_fill (top_square->to_path (), Gsk::FillRule::EVEN_ODD, {0.5f, 0.5f, 0.5f, 1.0f});
		    }
		    if (i == 'c' || i == 'd' || i == 'f' || i == 'h' || i == 'i' || i == 'k' || i == 'l')
		    {
		        /* right square */
		        auto right_square = Gsk::PathBuilder::create();
		        right_square->add_rect ({x_s13 + x_s13 + x_remainder + x,
		            y_remainder == 2 ? y_s13 + y_remainder + y : y_s13 + y,
		            x_remainder == 2 ? x_s13 + x_remainder : x_s13,
		            y_remainder == 2 ? y_s13 : y_s13 + y_remainder});
		        s->append_fill (right_square->to_path (), Gsk::FillRule::EVEN_ODD, {0.5f, 0.5f, 0.5f, 1.0f});
		    }
		    if (i == 'b' || i == 'f' || i == 'g' || i == 'i' || i == 'j' || i == 'k' || i == 'l')
		    {
		        /* bottom square */
		        auto bottom_square = Gsk::PathBuilder::create();
		        bottom_square->add_rect ({x_remainder == 2 ? x_s13 + x + x_remainder : x_s13 + x,
		            y_s13 + y_s13 + y_remainder + y,
		            x_remainder == 2 ? x_s13 : x_s13 + x_remainder,
		            y_remainder == 2 ? y_s13 + y_remainder : y_s13});
		        s->append_fill (bottom_square->to_path (), Gsk::FillRule::EVEN_ODD, {0.5f, 0.5f, 0.5f, 1.0f});
		    }
		    if (i == 'c' || i == 'e' || i == 'g' || i == 'h' || i == 'j' || i == 'k' || i == 'l')
		    {
		        /* left square */
		        auto left_square = Gsk::PathBuilder::create();
		        left_square->add_rect ({x,
		            y_remainder == 2 ? y_s13 + y + y_remainder : y_s13 + y,
		            x_remainder == 2 ? x_s13 + x_remainder : x_s13,
		            y_remainder == 2 ? y_s13 : y_s13 + y_remainder});
		        s->append_fill (left_square->to_path (), Gsk::FillRule::EVEN_ODD, {0.5f, 0.5f, 0.5f, 1.0f});
		    }
		}
	};

	class ActiveView : public Gtk::Widget
	{
	public:
		ActiveView(View &view) : view(view)
		{
			set_hexpand(true);
			set_vexpand(true);
		}
		virtual ~ActiveView() override = default;
		void redraw () {queue_draw ();}
	protected:
	 	void snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>& snapshot) override
	 	{
            const double max_delta_deviation = 1.15;
            int x_delta = get_width () / view.game.get_width();
            int y_delta = get_height () / view.game.get_height();
            if (x_delta > max_delta_deviation * y_delta)
                x_delta = (int)(y_delta * max_delta_deviation);
            else if (y_delta > max_delta_deviation * x_delta)
                y_delta = (int)(x_delta * max_delta_deviation);
            int x_offset = (get_width () - x_delta * view.game.get_width()) / 2;
            int y_offset = (get_height () - y_delta * view.game.get_height()) / 2;

            /* draw warps */
            for(const auto &warp : view.game.get_warps())
            {
            	auto position = warp.second.get_source_top_left();
				draw_bonus(snapshot, x_delta * position.x + x_offset, y_delta * position.y + y_offset, x_delta + x_delta, y_delta + y_delta, Bonus::WARP, animate);
            }

            /* draw dematerialized worms */
            for(const auto &worm : view.game.get_worms())
            {
                if(!worm.is_materialized())
                {
                    for(const auto &position : worm.get_positions())
                    {
                        draw_worm_segment(snapshot, x_delta * (position >> 8) + x_offset, y_delta * (position & 0xff) + y_offset, x_delta, y_delta,
                        	worm.get_colour(), false, false);
                    }
                }
            }
            /* draw materialized worms */
            for(const auto &worm : view.game.get_worms())
            {
                if(worm.is_materialized())
                {
                    for(const auto &position : worm.get_positions())
                    {
                        draw_worm_segment(snapshot, x_delta * (position >> 8) + x_offset, y_delta * (position & 0xff) + y_offset, x_delta, y_delta,
                        	worm.get_colour(), true, worm.was_bonus_eaten_at_this_position(position));
                    }
                }
            }

            /* draw bonuses */
            for(auto bonus : view.game.get_bonuses())
            {
                draw_bonus(snapshot, x_delta * bonus.x + x_offset, y_delta * bonus.y + y_offset, x_delta + x_delta, y_delta + y_delta, bonus.type, animate);
            }
/*
            if (view.countdown_active () > 0)
            {
                // count down
                string text = view.seconds_string (view.countdown_active ());
                double w, h;
                int font_size = 252;
                view.calculate_text_size (text, font_size, out w, out h);
                view.draw_text_font_size (s, (int)(x_offset + x_delta * (WIDTH / 2) - w / 2), (int)(y_offset + y_delta * (HEIGHT / 2) - h / 2), text, font_size);

                //draw name labels
                for(Worm &worm : view.game.worms)
                {
                    if (!worm.list.is_empty)
                    {
                        if (worm.direction == WormDirection.UP || worm.direction == WormDirection.DOWN)
                        {
                            // vertical worm
                            int middle = worm.length / 2;
                            view.draw_text_target_width (s, x_offset + x_delta * ((worm.list[middle] >> 8) + 1) + x_delta / 2,
                                          y_offset + y_delta * ((uint8)worm.list[middle]),
                                          view.worm_name (worm.id + 1), x_delta * worm.length, worm.get_colour());
                        }
                        else if (worm.direction == WormDirection.LEFT || worm.direction == WormDirection.RIGHT)
                        {
                            // horizontal worm 
                            int x = worm.list[0] >> 8;
                            if (x > worm.list[worm.length-1] >> 8)
                                x = worm.list[worm.length-1] >> 8;
                            view.draw_text_target_width (s, x_offset + x_delta * x,
                                          y_offset + y_delta * ((uint8)worm.list[0]) - y_delta,
                                          view.worm_name (worm.id + 1), x_delta * worm.length, worm.get_colour());
                        }
                    }
                }
            }
*/
	 	}
	private:
		View &view;
		uint64_t animate;
		
		void draw_bonus(const Glib::RefPtr<Gtk::Snapshot> &s, int x, int y, int x_size, int y_size, Bonus::eType type, uint64_t animate)
		{
		    float x_m = x_size;
		    float y_m = y_size;
		    switch (type)
		    {
		        case Bonus::REGULAR:
		        {
		            x_m /= 18;
		            y_m /= 18;
					auto p0 = Gsk::PathBuilder::create();
		            p0->move_to (x + x_m * 15, y + y_m * 8);
		            p0->cubic_to (x + x_m * 15.023438f, y + y_m * 10.035156f, x + x_m * 13.953125f, y + y_m * 17.1875f, x + x_m * 8, y + y_m * 14.429688f);
		            p0->cubic_to (x + x_m * 1.90625f, y + y_m * 17.109375f, x + x_m * 1.03125f, y + y_m * 9.921875f, x + x_m * 1, y + y_m * 8);
		            p0->cubic_to (x + x_m * 1.007813f, y + y_m * 5.109375f, x + x_m * 3.300781f, y + y_m * 1.355469f, x + x_m * 8, y + y_m * 4.3125f);
		            p0->cubic_to (x + x_m * 12.933594f, y + y_m * 1.394531f, x + x_m * 15.0625f, y + y_m * 5, x + x_m * 15, y + y_m * 8);
		            s->append_fill (p0->to_path (), Gsk::FillRule::EVEN_ODD, {0.0f, 1.0f, 0.0f, 1.0f});
		            auto p1 = Gsk::PathBuilder::create();
		            p1->move_to (x + x_m * 9.65625f, y + y_m * 1.34375f);
		            p1->cubic_to (x + x_m * 8, y + y_m * 2, x + x_m * 8, y + y_m * 3.667969f, x + x_m * 8, y + y_m * 5);
		            s->append_fill (p1->to_path (), Gsk::FillRule::EVEN_ODD, {0.0f, 1.0f, 0.0f, 1.0f});
				}
		            break;
		        case Bonus::HALF:
		        {
		            x_m /= 16;
		            y_m /= 16;
					auto p0 = Gsk::PathBuilder::create();
		            p0->move_to (x + x_m * 10.253906f, y + y_m * 1.3125f);
		            p0->cubic_to (x + x_m * 9.472656f, y + y_m * 4.730469f, x + x_m * 9.445313f, y + y_m * 8.015625f, x + x_m * 11.625f, y + y_m * 10.683594f);
		            s->append_fill (p0->to_path (), Gsk::FillRule::EVEN_ODD, {0.305882f, 0.603922f, 0.0235294f, 1.0f});
		            auto p1 = Gsk::PathBuilder::create();
		            p1->move_to (x + x_m * 10.296875f, y + y_m * 1.152344f);
		            p1->cubic_to (x + x_m * 9.046875f, y + y_m * 7.132813f, x + x_m * 6.023438f, y + y_m * 7.765625f, x + x_m * 3.84375f, y + y_m * 10.429688f);
		            s->append_fill (p1->to_path (), Gsk::FillRule::EVEN_ODD, {0.305882f, 0.603922f, 0.0235294f, 1.0f});
		            auto p2 = Gsk::PathBuilder::create();
		            p2->move_to (x + x_m * 7, y + y_m * 10);
		            p2->cubic_to (x + x_m * 7, y + y_m * 11.65625f, x + x_m * 5.65625f, y + y_m * 13, x + x_m * 4, y + y_m * 13);
		            p2->cubic_to (x + x_m * 2.34375f, y + y_m * 13, x + x_m * 1, y + y_m * 11.65625f, x + x_m * 1, y + y_m * 10);
		            p2->cubic_to (x + x_m * 1, y + y_m * 8.34375f, x + x_m * 2.34375f, y + y_m * 7, x + x_m * 4, y + y_m * 7);
		            p2->cubic_to (x + x_m * 5.65625f, y + y_m * 7, x + x_m * 7, y + y_m * 8.34375f, x + x_m * 7, y + y_m * 10);
		            s->append_fill (p2->to_path (), Gsk::FillRule::EVEN_ODD, {0.8f, 0.0f, 0.0f, 1.0f});
		            auto p3 = Gsk::PathBuilder::create();
		            p3->move_to (x + x_m * 15, y + y_m * 12);
		            p3->cubic_to (x + x_m * 15, y + y_m * 13.65625f, x + x_m * 13.65625f, y + y_m * 15, x + x_m * 12, y + y_m * 15);
		            p3->cubic_to (x + x_m * 10.34375f, y + y_m * 15, x + x_m * 9, y + y_m * 13.65625f, x + x_m * 9, y + y_m * 12);
		            p3->cubic_to (x + x_m * 9, y + y_m * 10.34375f, x + x_m * 10.34375f, y + y_m * 9, x + x_m * 12, y + y_m * 9);
		            p3->cubic_to (x + x_m * 13.65625f, y + y_m * 9, x + x_m * 15, y + y_m * 10.34375f, x + x_m * 15, y + y_m * 12);
		            s->append_fill (p3->to_path (), Gsk::FillRule::EVEN_ODD, {0.8f, 0.0f, 0.0f, 1.0f});
		        }
		            break;
		        case Bonus::DOUBLE:
		        {
		            x_m /= 18;
		            y_m /= 18;
					auto p0 = Gsk::PathBuilder::create();
		            p0->move_to (x + x_m * 0.695313f, y + y_m * 8.425781f);
		            p0->cubic_to (x + x_m * 8.914063f, y + y_m * 11.246094f, x + x_m * 13.257813f, y + y_m * 5.894531f, x + x_m * 13.847656f, y + y_m * 4.394531f);
		            p0->cubic_to (x + x_m * 14.285156f, y + y_m * 3.351563f, x + x_m * 14.308594f, y + y_m * 3.082031f, x + x_m * 14.402344f, y + y_m * 2.535156f);
		            p0->cubic_to (x + x_m * 14.941406f, y + y_m * 2.433594f, x + x_m * 15.613281f, y + y_m * 2.71875f, x + x_m * 16, y + y_m * 3.0625f);
		            p0->cubic_to (x + x_m * 15.566406f, y + y_m * 3.535156f, x + x_m * 15.261719f, y + y_m * 4.246094f, x + x_m * 15.167969f, y + y_m * 4.984375f);
		            p0->cubic_to (x + x_m * 15.675781f, y + y_m * 11.316406f, x + x_m * 7.71875f, y + y_m * 17.683594f, x + x_m * 0, y + y_m * 9.972656f);
		            p0->cubic_to (x + x_m * 0.03125f, y + y_m * 9.433594f, x + x_m * 0.210938f, y + y_m * 8.84375f, x + x_m * 0.695313f, y + y_m * 8.425781f);
		            s->append_fill (p0->to_path (), Gsk::FillRule::EVEN_ODD, {0.988235f, 0.913725f, 0.309804f, 1.0f});
		        }
		            break;
		        case Bonus::LIFE:
		        {
		            x_m /= 16;
		            y_m /= 16;
					auto p0 = Gsk::PathBuilder::create();
		            p0->move_to (x + x_m * 4.753906f, y + y_m * 1.828125f);
		            p0->cubic_to (x + x_m * 2.652344f, y + y_m * 1.851563f, x + x_m * 1.019531f, y + y_m * 3.648438f, x + x_m * 1, y + y_m * 5.8125f);
		            p0->cubic_to (x + x_m * 0.972656f, y + y_m * 8.890625f, x + x_m * 2.808594f, y + y_m * 9.882813f, x + x_m * 8.015625f, y + y_m * 14.171875f);
		            p0->cubic_to (x + x_m * 12.992188f, y + y_m * 9.558594f, x + x_m * 14.976563f, y + y_m * 8.316406f, x + x_m * 15, y + y_m * 5.722656f);
		            p0->cubic_to (x + x_m * 15.027344f, y + y_m * 2.886719f, x + x_m * 10.90625f, y + y_m * 0.128906f, x + x_m * 7.910156f, y + y_m * 3.121094f);
		            p0->cubic_to (x + x_m * 6.835938f, y + y_m * 2.199219f, x + x_m * 5.742188f, y + y_m * 1.816406f, x + x_m * 4.753906f, y + y_m * 1.828125f);
		            s->append_fill (p0->to_path (), Gsk::FillRule::EVEN_ODD, {1.0f, 0.0f, 0.0f, 1.0f});
		        }
		            break;
		        case Bonus::REVERSE:
		        {
		            x_m /= 16;
		            y_m /= 16;
					auto p0 = Gsk::PathBuilder::create();
		            p0->move_to (x + x_m * 4, y + y_m * 2);
		            p0->line_to (x + x_m * 12, y + y_m * 2);
		            p0->line_to (x + x_m * 15, y + y_m * 6);
		            p0->line_to (x + x_m * 8, y + y_m * 15);
		            p0->line_to (x + x_m * 1, y + y_m * 6);
		            s->append_fill (p0->to_path (), Gsk::FillRule::EVEN_ODD, {0.717647f, 0.807843f, 0.901961f, 1.0f});
		            auto p1 = Gsk::PathBuilder::create();
		            p1->move_to (x + x_m * 11, y + y_m * 6);
		            p1->line_to (x + x_m * 8, y + y_m * 15);
		            p1->line_to (x + x_m * 5, y + y_m * 6);
		            s->append_fill (p1->to_path (), Gsk::FillRule::EVEN_ODD, {0.447059f, 0.623529f, 0.811765f, 1.0f});
		            auto p2 = Gsk::PathBuilder::create();
		            p2->move_to (x + x_m * 4, y + y_m * 2);
		            p2->line_to (x + x_m * 8, y + y_m * 2);
		            p2->line_to (x + x_m * 5, y + y_m * 6);
		            p2->line_to (x + x_m * 1, y + y_m * 6);
		            s->append_fill (p2->to_path (), Gsk::FillRule::EVEN_ODD, {0.447059f ,0.623529f ,0.811765f, 1.0f});
		            auto p3 = Gsk::PathBuilder::create();
		            p3->move_to (x + x_m * 12, y + y_m * 2);
		            p3->line_to (x + x_m * 8, y + y_m * 2);
		            p3->line_to (x + x_m * 11, y + y_m * 6);
		            p3->line_to (x + x_m * 15, y + y_m * 6);
		            s->append_fill (p3->to_path (), Gsk::FillRule::EVEN_ODD, {0.447059f, 0.623529f, 0.811765f, 1.0f});
		            break;
		        }
		        case Bonus::WARP:
		        {
		            x_m /= 16;
		            y_m /= 16;
					auto p0 = Gsk::PathBuilder::create();
		            p0->move_to (x + x_m * 8.664063f, y + y_m * 0.621094f);
		            p0->cubic_to (x + x_m * 6.179688f, y + y_m * 0.761719f, x + x_m * 4.265625f, y + y_m * 2.679688f, x + x_m * 4.40625f, y + y_m * 5.164063f);
		            p0->line_to (x + x_m * 7.433594f, y + y_m * 5.164063f);
		            p0->cubic_to (x + x_m * 7.386719f, y + y_m * 4.3125f, x + x_m * 8.003906f, y + y_m * 3.699219f, x + x_m * 8.855469f, y + y_m * 3.652344f);
		            p0->cubic_to (x + x_m * 9.707031f, y + y_m * 3.601563f, x + x_m * 10.417969f, y + y_m * 4.21875f, x + x_m * 10.464844f, y + y_m * 5.070313f);
		            p0->line_to (x + x_m * 10.464844f, y + y_m * 5.117188f);
		            p0->cubic_to (x + x_m * 10.46875f, y + y_m * 5.316406f, x + x_m * 10.417969f, y + y_m * 5.609375f, x + x_m * 10.273438f, y + y_m * 5.78125f);
		            p0->cubic_to (x + x_m * 9.929688f, y + y_m * 6.191406f, x + x_m * 9.542969f, y + y_m * 6.53125f, x + x_m * 9.234375f, y + y_m * 6.773438f);
		            p0->cubic_to (x + x_m * 8.890625f, y + y_m * 7.035156f, x + x_m * 8.515625f, y + y_m * 7.351563f, x + x_m * 8.144531f, y + y_m * 7.816406f);
		            p0->cubic_to (x + x_m * 7.773438f, y + y_m * 8.28125f, x + x_m * 7.433594f, y + y_m * 8.949219f, x + x_m * 7.433594f, y + y_m * 9.710938f);
		            p0->cubic_to (x + x_m * 7.425781f, y + y_m * 10.507813f, x + x_m * 8.148438f, y + y_m * 11.222656f, x + x_m * 8.949219f, y + y_m * 11.222656f);
		            p0->cubic_to (x + x_m * 9.75f, y + y_m * 11.222656f, x + x_m * 10.476563f, y + y_m * 10.507813f, x + x_m * 10.464844f, y + y_m * 9.710938f);
		            p0->cubic_to (x + x_m * 10.464844f, y + y_m * 9.710938f, x + x_m * 10.4375f, y + y_m * 9.753906f, x + x_m * 10.511719f, y + y_m * 9.664063f);
		            p0->cubic_to (x + x_m * 10.585938f, y + y_m * 9.566406f, x + x_m * 10.789063f, y + y_m * 9.40625f, x + x_m * 11.078125f, y + y_m * 9.1875f);
		            p0->cubic_to (x + x_m * 12.921875f, y + y_m * 7.792969f, x + x_m * 13.492188f, y + y_m * 7.003906f, x + x_m * 13.492188f, y + y_m * 4.882813f);
		            p0->cubic_to (x + x_m * 13.355469f, y + y_m * 2.394531f, x + x_m * 11.152344f, y + y_m * 0.484375f, x + x_m * 8.664063f, y + y_m * 0.621094f);
		            float r,g,b;
		            r = animate%30 < 10 ? (animate%30 / 10.0f) : (animate%30 >= 20 ? 0 : ((20 - animate%30) / 10.0f));
		            g = (animate+10)%30 < 10 ? ((animate+10)%30 / 10.0f) : ((animate+10)%30 >= 20 ? 0 : ((20 - (animate+10)%30) / 10.0f));
		            b = (animate+20)%30 < 10 ? ((animate+20)%30 / 10.0f) : ((animate+20)%30 >= 20 ? 0 : ((20 - (animate+20)%30) / 10.0f));
		            s->append_fill (p0->to_path (), Gsk::FillRule::EVEN_ODD, {r, g, b, 1.0f});
		            auto p1 = Gsk::PathBuilder::create();
		            p1->move_to (x + x_m * 8.949219f, y + y_m * 12.738281f);
		            p1->cubic_to (x + x_m * 8.113281f, y + y_m * 12.738281f, x + x_m * 7.433594f, y + y_m * 13.417969f, x + x_m * 7.433594f, y + y_m * 14.253906f);
		            p1->cubic_to (x + x_m * 7.433594f, y + y_m * 15.089844f, x + x_m * 8.113281f, y + y_m * 15.769531f, x + x_m * 8.949219f, y + y_m * 15.769531f);
		            p1->cubic_to (x + x_m * 9.785156f, y + y_m * 15.769531f, x + x_m * 10.464844f, y + y_m * 15.089844f, x + x_m * 10.464844f, y + y_m * 14.253906f);
		            p1->cubic_to (x + x_m * 10.464844f, y + y_m * 13.417969f, x + x_m * 9.785156f, y + y_m * 12.738281f, x + x_m * 8.949219f, y + y_m * 12.738281f);
		            s->append_fill (p1->to_path (), Gsk::FillRule::EVEN_ODD, {r, g, b, 1.0f});
		        }
		            break;
		        /*
		        case 6:
		            x_m /= 16;
		            y_m /= 16;
		            var p0 = new PathBuilder ();
		            p0.move_to (x + x_m * 8.902344f, y + y_m * 0.160156f);
		            p0.cubic_to (x + x_m * 6.953125f, y + y_m * 1.15625f, x + x_m * 7.480469f, y + y_m * 3.089844f, x + x_m * 7.453125f, y + y_m * 5.019531f);
		            p0.line_to (x + x_m * 8.257813f, y + y_m * 4.8125f);
		            p0.cubic_to (x + x_m * 8.144531f, y + y_m * 3.507813f, x + x_m * 9.359375f, y + y_m * 1.511719f, x + x_m * 10.742188f, y + y_m * 1.675781f);
		            s.append_fill (p0.to_path (), EVEN_ODD, {0.305882f, 0.603922f, 0.0235294f, 1.0f});
		            var p1 = new PathBuilder ();
		            p1.move_to (x + x_m * 14, y + y_m * 9);
		            p1.cubic_to (x + x_m * 14, y + y_m * 5.6875f, x + x_m * 11.3125f, y + y_m * 3, x + x_m * 8, y + y_m * 3);
		            p1.cubic_to (x + x_m * 4.6875f, y + y_m * 3, x + x_m * 2, y + y_m * 5.6875f, x + x_m * 2, y + y_m * 9);
		            p1.cubic_to (x + x_m * 2, y + y_m * 12.3125f, x + x_m * 4.6875f, y + y_m * 15, x + x_m * 8, y + y_m * 15);
		            p1.cubic_to (x + x_m * 11.3125f, y + y_m * 15, x + x_m * 14, y + y_m * 12.3125f, x + x_m * 14, y + y_m * 9);
		            s.append_fill (p1.to_path (), EVEN_ODD, {0.960784f, 0.47451f, 0.0f, 1.0f});
		            break;
		        case 7:
		            x_m /= 16;
		            y_m /= 16;
		            var p0 = new PathBuilder ();
		            p0.move_to (x + x_m * 4.585938f, y + y_m * 0.96875f);
		            p0.cubic_to (x + x_m * 3.914063f, y + y_m * 3.050781f, x + x_m * 5.65625f, y + y_m * 4.042969f, x + x_m * 7, y + y_m * 5.429688f);
		            p0.line_to (x + x_m * 7.421875f, y + y_m * 4.710938f);
		            p0.cubic_to (x + x_m * 6.417969f, y + y_m * 3.871094f, x + x_m * 5.867188f, y + y_m * 1.597656f, x + x_m * 6.960938f, y + y_m * 0.738281f);
		            s.append_fill (p0.to_path (), EVEN_ODD, {0.305882f, 0.603922f, 0.0235294f, 1.0f});
		            var p1 = new PathBuilder ();
		            p1.move_to (x + x_m * 12.933594f, y + y_m * 5.347656f);
		            p1.cubic_to (x + x_m * 13.652344f, y + y_m * 7.882813f, x + x_m * 12.867188f, y + y_m * 8.753906f, x + x_m * 12.871094f, y + y_m * 10.476563f);
		            p1.cubic_to (x + x_m * 12.875f, y + y_m * 12.890625f, x + x_m * 13.015625f, y + y_m * 14.386719f, x + x_m * 11.148438f, y + y_m * 15.089844f);
		            p1.cubic_to (x + x_m * 9.941406f, y + y_m * 15.492188f, x + x_m * 8.785156f, y + y_m * 15.382813f, x + x_m * 6.539063f, y + y_m * 12.617188f);
		            p1.cubic_to (x + x_m * 5.886719f, y + y_m * 11.765625f, x + x_m * 4.117188f, y + y_m * 11.683594f, x + x_m * 3.226563f, y + y_m * 10.214844f);
		            p1.cubic_to (x + x_m * 2.117188f, y + y_m * 8.375f, x + x_m * 2.902344f, y + y_m * 5.152344f, x + x_m * 6.707031f, y + y_m * 4.464844f);
		            p1.cubic_to (x + x_m * 8.609375f, y + y_m * 2.308594f, x + x_m * 11.933594f, y + y_m * 3.136719f, x + x_m * 12.933594f, y + y_m * 5.347656f);
		            s.append_fill (p1.to_path (), EVEN_ODD, {0.937255f, 0.160784f, 0.160784f, 1.0f});
		            break; */
		        default:
		            break;
		    }
		}
		void draw_worm_segment (const Glib::RefPtr<Gtk::Snapshot> &s, int x, int y, int x_size, int y_size, eWormColour colour, bool is_materialized, bool eaten_bonus)
		{
		    if (eaten_bonus)
		    {
		        int a = x_size + x_size / 5;
		        if (a < x_size + 1)
		            x_size += 1;
		        else
		        {
		            x -= (a - x_size) / 2;
		            x_size = a;
		        }
		        a = y_size + y_size / 5;
		        if (a < y_size + 1)
		            y_size += 1;
		        else
		        {
		            y -= (a - y_size) / 2;
		            y_size = a;
		        }
		    }
		    else
		    {
		        /* leave a one pixel border */
		        ++x;
		        ++y;
		        x_size -= 1;
		        y_size -= 1;
		    }

		    const float PI2 = 1.570796326794896619231321691639751442f;
		    float x_s13 = x_size / 3.0f;
		    float x_s23 = x_s13 + x_s13;
		    float y_s13 = y_size / 3.0f;
		    float y_s23 = y_s13 + y_s13;
			auto path = Gsk::PathBuilder::create();
		    /* top right corner */
		    path->move_to (x + x_s23, y + 0);
		    path->svg_arc_to (x_s13, y_s13, PI2, false, true, x + x_size, y + y_s13);
		    /* bottom right corner */
		    path->line_to (x + x_size, y + y_s23);
		    path->svg_arc_to (x_s13, y_s13, PI2, false, true, x + x_s23, y + y_size);
		    /* bottom left corner */
		    path->line_to (x + x_s13, y + y_size);
		    path->svg_arc_to (x_s13, y_s13, PI2, false, true, x + 0, y + y_s23);
		    /* top left corner */
		    path->line_to (x + 0, y + y_s13);
		    path->svg_arc_to (x_s13, y_s13, PI2, false, true, x + x_s13, y + 0);
		    /* fill */
		    auto [r,g,b] = view.get_worm_rgb(colour, is_materialized);
		    s->append_fill (path->to_path (), Gsk::FillRule::EVEN_ODD, {r, g, b, 1.0f});
		}
		
		
	};

	class Life : public Gtk::Widget
	{
	public:
		Life() : Gtk::Widget()
		{
		}
		virtual ~Life() override = default;
		explicit Life(GtkWidget* gobj) :
			Glib::ObjectBase(nullptr), // Passing nullptr avoids allocating a duplicate GObject
			Gtk::Widget(gobj)
		{
		}
	protected:
	 	void snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>& s) override
	 	{
			//Gtk::Widget::snapshot_vfunc(s);

			auto path = Gsk::PathBuilder::create();
			double x_m = get_width () / 16;
			double y_m = get_height () / 16;
	        const double x = 0;
	        const double y = 0;

			path->move_to(x + x_m * 4.753906f, y + y_m * 1.828125f);
		    path->cubic_to(x + x_m * 2.652344f, y + y_m * 1.851563f, x + x_m * 1.019531f, y + y_m * 3.648438f, x + x_m * 1.0f, y + y_m * 5.8125f);
		    path->cubic_to(x + x_m * 0.972656f, y + y_m * 8.890625f, x + x_m * 2.808594f, y + y_m * 9.882813f, x + x_m * 8.015625f, y + y_m * 14.171875f);
		    path->cubic_to(x + x_m * 12.992188f, y + y_m * 9.558594f, x + x_m * 14.976563f, y + y_m * 8.316406f, x + x_m * 15.0f, y + y_m * 5.722656f);
		    path->cubic_to(x + x_m * 15.027344f, y + y_m * 2.886719f, x + x_m * 10.90625f, y + y_m * 0.128906f, x + x_m * 7.910156f, y + y_m * 3.121094f);
		    path->cubic_to(x + x_m * 6.835938f, y + y_m * 2.199219f, x + x_m * 5.742188f, y + y_m * 1.816406f, x + x_m * 4.753906f, y + y_m * 1.828125f);

	        s->append_fill (path->to_path (), Gsk::FillRule::EVEN_ODD, {1.0f, 0.0f, 0.0f, 1.0f});
	 	}
		void measure_vfunc(Gtk::Orientation orientation, int for_size, int& minimum, int& natural,
			int& minimum_baseline, int& natural_baseline) const override
		{
			if (orientation == Gtk::Orientation::HORIZONTAL)
			{
				minimum = 16;
				natural = 16;
			}
			else
			{
				minimum = 16;
				natural = 16;
			}

			// Don't use baseline alignment.
			minimum_baseline = -1;
			natural_baseline = -1;
		}
	private:
		static Glib::ObjectBase* wrap_new(GObject* o)
		{
			// Tie lifetime cleanup directly to the parent widget lifecycle
			return Gtk::manage(new Life(GTK_WIDGET(o)));
		}
	};

/* class View */
public:
	View(Game::Progress progress, unsigned long start_level, unsigned long speed,
		Gtk::Button &pause_button,
		std::function<void(const std::vector<WormScore>)> game_over
	);
	virtual ~View() override = default;
	bool is_fullscreen_active()
	{
		return fullscreen;
	}
	void set_keys(eWormColour colour, const std::array<unsigned int, 4> &raw_keys/* up, left, right & down */)
	{
		keys.insert({raw_keys[0],{colour, eDirection::UP}});
		keys.insert({raw_keys[1],{colour, eDirection::LEFT}});
		keys.insert({raw_keys[2],{colour, eDirection::RIGHT}});
		keys.insert({raw_keys[3],{colour, eDirection::DOWN}});
	}
	bool key_press(guint keycode)
	{
		auto it = keys.find(keycode);
		if(it != keys.end())
		{
			game.human_action(it->second);
			return true;
		}
		else
			return false;
	}
	void set_pause(bool state/*false for resume*/)
	{
		if(state)
		{
			paused=true;
			get_statusbar_stack()->set_visible_child("paused"); /* switch to paused message */
		}
		else
		{
			paused=false;
			get_statusbar_stack()->set_visible_child("scoreboard"); /* switch to the score board */
			play();
		}
	}
private:
	Game::Progress progress;
	std::bitset<26> levels;
	unsigned long current_level;
	const unsigned long speed;
	Gtk::Button &pause_button;
	std::function<void(const std::vector<WormScore>)> game_over;
	std::map<unsigned int, HumanAction> keys;
	StaticView static_view;
	ActiveView active_view;
	unsigned int player_count,ai_count;
	std::vector<eWormColour> worm_colour;
	bool fullscreen=false;
	static void play_sound(const Glib::ustring &sound);
	Game game;
	std::unordered_map<eWormColour, Gtk::Box *> score_box;
	bool paused;
	TimeCallBack timer;
	
private:
	void initialise_and_start();
	void load_board_level(unsigned long level);
	bool play();
	Gtk::Label* create_label(Glib::ustring text);
	Gtk::Button* create_button(Glib::ustring text);
	const Glib::ustring get_worm_name(unsigned int worm_id);
	const Glib::ustring get_level_completed_message(unsigned long level);
	const Glib::ustring get_next_level_message(unsigned long level);
	const Glib::ustring get_countdown_message(unsigned long count);

	Gtk::Label* create_label(Glib::ustring text, unsigned long top_margin)
	{
		auto *l=create_label(text);
		l->set_margin_top(top_margin);
		return l;
	}
	Gtk::Box* get_scoreboard()
	{
		auto scoreboard=get_statusbar_stack()->get_child_by_name("scoreboard");
		if(!scoreboard)
		{
        	Glib::ustring buffer="nibbles-window.ui: GtkStackPage property name = \"scoreboard\" not found!";
        	g_critical(buffer.c_str());
		}
		auto r=dynamic_cast<Gtk::Box*>(scoreboard);
		if(!r)
		{
        	Glib::ustring buffer="nibbles-window.ui: id=\"";
        	buffer+=scoreboard->get_buildable_id();
        	buffer+="\" is not a Gtk::Box!";
        	g_critical(buffer.c_str());
		}
		return r;
	}
	Gtk::Stack* get_statusbar_stack()
	{
		Gtk::Widget *p=get_parent();
		for(p=p->get_first_child();p && p->get_buildable_id()!="statusbar_stack";p=p->get_next_sibling());
		if(!p)
		{
        	Glib::ustring buffer="nibbles-window.ui: id=\"statusbar_stack\" not found!";
        	g_critical(buffer.c_str());
		}
		auto r=dynamic_cast<Gtk::Stack*>(p);
		if(!r)
		{
        	Glib::ustring buffer="nibbles-window.ui: id=\"statusbar_stack\" is not a Gtk::Stack!";
        	g_critical(buffer.c_str());
		}
		return r;
	}
	Gtk::Box* create_score_box(const Glib::ustring &name_text, eWormColour colour)
	{
		/* create score box */
		Gtk::Label *name=Gtk::make_managed<Gtk::Label>();
		const char *pango_colour[]={"#ff0000","#00c000","#0080ff","#ffff00","#00ffff","#c000c0"};
		Glib::ustring markup="<span color=\"";
		markup+=pango_colour[colour];
		markup+="\">";
		markup+=name_text;
		markup+="</span>";
        name->set_markup(markup);
		name->set_size_request(59,-1);
		name->set_xalign(0);
		Gtk::Label *score=Gtk::make_managed<Gtk::Label>();
		score->set_text("0");
		score->set_size_request(43,-1);
		score->set_xalign(1);
		Gtk::Box *label_box=Gtk::make_managed<Gtk::Box>();
		label_box->set_spacing(4);
		label_box->append(*name);
		label_box->append(*score);
		Gtk::Grid *grid=Gtk::make_managed<Gtk::Grid>();
		grid->set_column_spacing(2);
		for(unsigned int i=0;i<6;i++)
			grid->attach(*Gtk::make_managed<Life>(),i,0,1,1);
		Gtk::Box *pBox=Gtk::make_managed<Gtk::Box>();
		pBox->set_orientation(Gtk::Orientation::VERTICAL);
		pBox->set_spacing(5);
		pBox->append(*label_box);
		pBox->append(*grid);
		return pBox;
	}
	Gtk::Grid *get_life_grid(Gtk::Box *score_box)
	{
		/* the life grid is the only child that is a Gtk::Grid */
		Gtk::Grid *pGrid=nullptr;
		for(auto p=score_box->get_first_child();p;p=p->get_next_sibling())
		{
			pGrid=dynamic_cast<Gtk::Grid*>(p);
			if(pGrid)
				break;
		}
		return pGrid;
	}
	Gtk::Label *get_score_label(Gtk::Box *score_box)
	{
		Gtk::Box *pBox=nullptr;
		for(auto p=score_box->get_first_child();p;p=p->get_next_sibling())
		{
			pBox=dynamic_cast<Gtk::Box*>(p);
			if(pBox)
				break;
		}
		/* the score label is the last child in the box */
		Gtk::Widget *pLabel=nullptr;
		if(pBox)
		{
			for(auto p=pBox->get_first_child();p;pLabel=p,p=p->get_next_sibling());
		}
		return dynamic_cast<Gtk::Label*>(pLabel);
	}
	std::tuple<float, float, float> get_worm_rgb(eWormColour colour, bool bright)
	{
	    switch (colour)
		{
		    case 0: /* red */
		    	return {bright ? 1.0 : 0.75, 0.0, 0.0};
		    case 1: /* green */
		    	return {0.0, bright ? 0.75 : 0.5, 0.0};
		    case 2: /* blue */
		    	return {0.0, bright ? 0.5 : 0.25, bright ? 1 : 0.75};
		    case 3: /* yellow */
		    	return {bright ? 0.9 : 0.75, bright ? 0.9 : 0.75, 0.0};
		    case 4: /* cyan */
		    	return {0, bright ? 1 : 0.75, bright ? 1 : 0.75};
		    case 5: /* magenta */
		    	return {bright ? 0.75 : 0.5, 0.0, bright ? 0.75 : 0.5};
		    default:
		    	return {bright ? 1 : 0.75, bright ? 1 : 0.75, bright ? 1 : 0.75};
		}
	}
};





