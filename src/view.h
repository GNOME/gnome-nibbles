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
		void set(sigc::bound_mem_functor<bool (View::*)()> function, uintsys delay)
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
		void snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>& snapshot) override;
	private:
		View &view;
		void draw_wall_segment (char i, const Glib::RefPtr<Gtk::Snapshot>& s,
			int x, int y, int x_size, int y_size);
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
		void redraw() {animate++; queue_draw ();}
	protected:
		void snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>& snapshot) override;
	private:
		View &view;
		uint64_t animate;
		
		void draw_bonus(const Glib::RefPtr<Gtk::Snapshot> &s, int x, int y, int x_size, int y_size, Bonus::eType type, uint64_t animate);
		void draw_worm_segment(const Glib::RefPtr<Gtk::Snapshot> &s, int x, int y, int x_size, int y_size, eWormColour colour, bool is_materialized, bool eaten_bonus);
		/* calculate the width & height of the text */
		std::pair<double,double> calculate_text_size(const Glib::ustring &text, int font_size)
		{
			auto layout = get_layout(text, font_size);
			Pango::Rectangle a,b;
			layout->get_extents(a, b);
			return {a.get_width() / Pango::SCALE, a.get_height() / Pango::SCALE};
		}
		std::pair<intsys,intsys> get_text_offsets(const Glib::ustring &text, int font_size)
		{
			auto layout = get_layout(text, font_size);
			Pango::Rectangle a,b;
			layout->get_extents(a, b);
		    return {a.get_x() / Pango::SCALE, a.get_y() / Pango::SCALE};
		}
		/* draw the text */
		void draw_text_font_size(const Glib::RefPtr<Gtk::Snapshot> &snapshot, int x, int y, const Glib::ustring &text, int font_size)
		{
			auto [x_offset, y_offset]=get_text_offsets(text, font_size);
			snapshot->save();
			snapshot->translate(/*Gdk::Graphene::Point*/{x - x_offset, y - y_offset});
			auto layout = get_layout(text, font_size);
			snapshot->append_layout(layout, {1, 1, 1, 1});
			snapshot->restore();
		}
		void draw_text_target_width(const Glib::RefPtr<Gtk::Snapshot> &snapshot, int x, int y, const Glib::ustring &text, int target_width);
		Glib::RefPtr<Pango::Layout> get_layout(const Glib::ustring &text, uintsys font_size);
	};

	class Life : public Gtk::Widget
	{
	public:
		Life(uintsys number=0) : Gtk::Widget(), number(number)
		{
		}
		virtual ~Life() override = default;
		explicit Life(GtkWidget* gobj) :
			Glib::ObjectBase(nullptr), // Passing nullptr avoids allocating a duplicate GObject
			Gtk::Widget(gobj), number(0)
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

			if(number>0)
				draw_text_target_height(s, 0, 0, number>9 ? "*" : std::to_string(number), 16, 16);
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
		const uintsys number;
		static Glib::ObjectBase* wrap_new(GObject* o)
		{
			// Tie lifetime cleanup directly to the parent widget lifecycle
			return Gtk::manage(new Life(GTK_WIDGET(o)));
		}
		/* calculate the width & height of the text */
		std::pair<double,double> calculate_text_size(const Glib::ustring &text, int font_size)
		{
			auto layout = get_layout(text, font_size);
			Pango::Rectangle a,b;
			layout->get_extents(a, b);
			return {a.get_width() / Pango::SCALE, a.get_height() / Pango::SCALE};
		}
		std::pair<intsys,intsys> get_text_offsets(const Glib::ustring &text, int font_size)
		{
			auto layout = get_layout(text, font_size);
			Pango::Rectangle a,b;
			layout->get_extents(a, b);
		    return {a.get_x() / Pango::SCALE, a.get_y() / Pango::SCALE};
		}
		/* draw the text */
		void draw_text_font_size(const Glib::RefPtr<Gtk::Snapshot> &snapshot, int x, int y, const Glib::ustring &text, int font_size)
		{
			auto [x_offset, y_offset]=get_text_offsets(text, font_size);
			snapshot->save();
			snapshot->translate(/*Gdk::Graphene::Point*/{x - x_offset, y - y_offset});
			auto layout = get_layout(text, font_size);
			snapshot->append_layout(layout, {1, 1, 1, 1});
			snapshot->restore();
		}
		void draw_text_target_height(const Glib::RefPtr<Gtk::Snapshot> &snapshot,
			intsys x, intsys y, const Glib::ustring &text, intsys target_width, intsys center_width);
		Glib::RefPtr<Pango::Layout> get_layout(const Glib::ustring &text, uintsys font_size);
	};

/* class View */
public:
	View(Game::Progress progress, uintsys start_level, uintsys speed, bool fakes,
		Gtk::Button &new_game_button, Gtk::Button &pause_button,
		std::function<void(const Glib::ustring &level)> set_level_description,
		std::function<void(const std::vector<WormScore>)> game_over,
		std::function<void(Gtk::Widget *)> next_level_function
	);
	virtual ~View() override
	{
		if(nullptr!=ctx)	
			g_object_unref(ctx); /* free sound context */
	}
	uintsys countdown_left()
	{
		return countdown;
	}
	void countdown_decrement()
	{
		countdown--;
	}
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
	void set_fullscreen(bool b)
	{
		fullscreen=b;
	}
	void set_pause(bool state/*false for resume*/)
	{
		if(state)
		{
			paused=true;
		}
		else
		{
			paused=false;
			play();
		}
	}
	void set_mute(bool state)
	{
		mute=state;
	}
private:
	Game::Progress progress;
	std::bitset<26> levels;
	uintsys current_level;
	const uintsys speed;
	Gtk::Button &new_game_button, &pause_button;
	std::function<void(const Glib::ustring &level)> set_level_description;
	std::function<void(const std::vector<WormScore>)> game_over;
	std::function<void(Gtk::Widget *)> next_level_function;
	GSoundContext* ctx; /* sound */
	uintsys countdown;
	std::map<unsigned int, HumanAction> keys;
	StaticView static_view;
	ActiveView active_view;
	unsigned int player_count,ai_count;
	std::vector<eWormColour> worm_colour;
	bool fullscreen;
	void play_sound(const Glib::ustring &sound);
	Game game;
	std::unordered_map<eWormColour, Gtk::Box *> score_box;
	std::unordered_map<eWormColour, Glib::ustring> names;
	bool paused;
	bool mute;
	TimeCallBack timer;
	
private:
	void initialise_and_start();
	void load_board_level(uintsys level);
	bool play();
	Gtk::Label* create_label(Glib::ustring text);
	Gtk::Button* create_button(Glib::ustring text);
	const Glib::ustring get_worm_name(unsigned int worm_id);
	const Glib::ustring get_level_completed_message(uintsys level);
	const Glib::ustring get_next_level_message(uintsys level);
	const Glib::ustring get_level_description(uintsys level);
	const Glib::ustring get_countdown_message(uintsys count);

	Gtk::Label* create_label(Glib::ustring text, uintsys top_margin)
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
			critical(buffer);
		}
		auto r=dynamic_cast<Gtk::Box*>(scoreboard);
		if(!r)
		{
			Glib::ustring buffer="nibbles-window.ui: id=\"";
			buffer+=scoreboard->get_buildable_id();
			buffer+="\" is not a Gtk::Box!";
			critical(buffer);
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
			critical(buffer);
		}
		auto r=dynamic_cast<Gtk::Stack*>(p);
		if(!r)
		{
			critical("nibbles-window.ui: id=\"statusbar_stack\" is not a Gtk::Stack!");
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





