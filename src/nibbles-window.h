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
#include <glibmm/extraclassinit.h>

enum eSetupScreen
{
	FIRST_RUN,
	PLAYERS,
	PROGRESS,
	SPEED,
	CONTROLS,
	GAME,
	USUAL
};

#define name_to_screen(screen)										\
	"first_run"==(screen) ? eSetupScreen::FIRST_RUN :				\
	(																\
		"number_of_players"==(screen) ? eSetupScreen::PLAYERS :		\
		(															\
			"board_progress"==(screen) ? eSetupScreen::PROGRESS :	\
			(														\
				"speed"==(screen) ? eSetupScreen::SPEED :			\
				(													\
					"controls"==(screen) ? eSetupScreen::CONTROLS :	\
					(												\
						"game_box"==(screen) ? eSetupScreen::GAME :	\
							USUAL									\
					)												\
				)													\
			)														\
		)															\
	)

eWormColour& operator++(eWormColour& c);

class NibblesWindow; /* forward reference */

class KeyHandler
{
public:
	virtual bool key_pressed(guint keyval, guint keycode) = 0;
	KeyHandler(NibblesWindow *pWindow) : pWindow(pWindow)
	{
	}
	virtual ~KeyHandler() = default;
protected:
	void get_key();
private:
	NibblesWindow *pWindow;
	//std::shared_ptr<NibblesWindow> Window;
};

class NibblesWindow : public Gtk::ApplicationWindow
{
private:
	/* static member function */
	static void draw_text(const Glib::RefPtr<Gtk::Snapshot>&snapshot, const Glib::ustring text, const int width, const bool text_at_top, Gtk::Widget &widget);
	static int calculate_font_size (const Glib::ustring &text, int target_width, double &width, double &height, Gtk::Widget &widget);
	static void draw_text_font_size (const Glib::RefPtr<Gtk::Snapshot>&snapshot, int x, int y, const Glib::ustring &text, int font_size, Gtk::Widget &widget);
	static void get_text_offsets (const Glib::ustring &text, int font_size, int &x_offset, int &y_offset, Gtk::Widget &widget);
private:
	/* member function */
	void setup_controls();
	void setup_game();

public:
	class PlayerControls : public Gtk::Widget
	{
	public:
		PlayerControls(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder)
			: Gtk::Widget(cobject), m_refBuilder(refBuilder)
		{
		}
		explicit PlayerControls(GtkWidget* gobj) :
			Glib::ObjectBase(nullptr), // Passing nullptr avoids allocating a duplicate GObject
			Gtk::Widget(gobj)
		{
		}
	protected:
		Glib::RefPtr<Gtk::Builder> m_refBuilder;
	};
	
	class PlayerButton; /* forward reference */
	/* gtkmm__CustomObject_Arrow is a gtk template */
	class Arrow : public Gtk::Widget
	{
	public:
		/* constructor used by Gtk::Builder::get_widget_derived<Arrow> */
		Arrow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder) :
			Gtk::Widget(cobject), property_direction_(*this, "direction", "")
		{
			auto legacy_controller = Gtk::EventControllerLegacy::create();// Create an event controller
			// Connect the raw event signal
			legacy_controller->signal_event().connect(sigc::mem_fun(*this, &Arrow::on_legacy_event),
				false /*false allows us to return true from the callback to block further propagation.*/);
			add_controller(legacy_controller);// Add the controller to the window
		}
		/* constructor used for registering class via dummy object in register_type() */
		Arrow() : Glib::ObjectBase("Arrow"), Gtk::Widget(), property_direction_(*this, "direction", "")
		{
		}
		virtual ~Arrow() = default;
		void set_direction(const Glib::ustring& direction)
		{
			property_direction_ = direction;
		}
		Glib::ustring get_direction() const
		{
			return property_direction_.get_value();
		}
		explicit Arrow(GtkWidget* gobj) :
			Glib::ObjectBase(nullptr), // Passing nullptr avoids allocating a duplicate GObject
			Gtk::Widget(gobj),
			property_direction_(*this, "direction", "")
		{
		}
		static void register_type()
		{
			static GType custom_gtype = 0; // gtype for the Arrow class

			if (custom_gtype != 0)
				return; // Prevent duplicate registration

			// Instantiate a temporary instance to resolve the unique internal GType runtime tracking ID
			Arrow dummy; 
			GtkWidget* raw_widget = dummy.gobj();
			custom_gtype = G_OBJECT_TYPE(raw_widget);

			// Register our C++ factory method to handle this unique GType globally
			Glib::wrap_register(custom_gtype, &Arrow::wrap_new);
			
			// Verify registration worked
			GType found_type = g_type_from_name("gtkmm__CustomObject_Arrow");
			if(!found_type)
			{
				Glib::ustring buffer="class gtkmm__CustomObject_Arrow not registered";
				critical(buffer);
			}
		}
		/* call back signal */
		using type_signal_status = sigc::signal<void()>;
		type_signal_status& signal_clicked() {return m_signal_status;}
	protected:
		type_signal_status m_signal_status;
		
	protected:
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
		}
	 	void snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>& s) override;
		bool on_legacy_event(const Glib::RefPtr<const Gdk::Event>&event)
		{
			switch (event->get_event_type ())
			{
				case Gdk::Event::Type::BUTTON_RELEASE:
					m_signal_status.emit();
					return true;
				default:
					return false;
			}
		}
	private:
		static Glib::ObjectBase* wrap_new(GObject* o)
		{
			// Tie lifetime cleanup directly to the parent widget lifecycle
			return Gtk::manage(new Arrow(GTK_WIDGET(o)));
		}
		Glib::Property<Glib::ustring> property_direction_;
	public:
		PlayerButton *player=nullptr;
	};
	class ColourWheelSegment; /* forward reference */
	/* gtkmm__CustomObject_ColourWheel is a gtk template */
	class ColourWheel : public Glib::ExtraClassInit, public Gtk::Box
	{
	public:
		/* constructor used for registering class via dummy object in register_type() */
		ColourWheel() : Glib::ObjectBase("ColourWheel"),
			Glib::ExtraClassInit(&ColourWheel::custom_class_init),
			Gtk::Box()
		{
		}
		/* constructor used by Gtk::Builder::get_widget_derived<ColourWheel> */
		ColourWheel(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder) :
			Glib::ExtraClassInit(&ColourWheel::custom_class_init), Gtk::Box(cobject)
		{
			auto legacy_controller = Gtk::EventControllerLegacy::create();// Create an event controller
			// Connect the raw event signal
			legacy_controller->signal_event().connect(sigc::mem_fun(*this, &ColourWheel::on_legacy_event),
				false /*false allows us to return true from the callback to block further propagation.*/);
			add_controller(legacy_controller);// Add the controller to the window
			
			auto keypress_controller = Gtk::EventControllerKey::create();// Create an key controller
			// Connect the raw event signal
			keypress_controller->signal_key_pressed().connect(sigc::mem_fun(*this, &ColourWheel::on_keypress_event),
				false /*false allows us to return true from the callback to block further propagation.*/);
			add_controller(keypress_controller);// Add the controller to the window

			auto mouse_position = Gtk::EventControllerMotion::create ();// Create a mouse controller
			mouse_position->signal_motion().connect(sigc::track_obj(
				[this](double x, double y) ->
					void
					{
						mouse_point={true,x,y};
						focus_mouse_segment();
					},
					*this
				));
			mouse_position->signal_enter().connect(sigc::track_obj(
				[this](double x, double y) ->
					void
					{
						mouse_point={true,x,y};
						focus_mouse_segment();
					},
					*this
				));
			mouse_position->signal_leave().connect(sigc::track_obj(
				[this]() ->
					void
					{
						mouse_point.is_valid=false;
						focus_mouse_segment();
					},
					*this
				));
			add_controller (mouse_position);
		}
		virtual ~ColourWheel() override = default;
		explicit ColourWheel(GtkWidget* gobj) :
			Glib::ObjectBase(nullptr), Glib::ExtraClassInit(&ColourWheel::custom_class_init), Gtk::Box()
		{
		}
		void on_show() override
		{
			mouse_point.is_valid=false;
			mouse_pressed=false;
			Gtk::Box::on_show();
		}
		/* override the size_allocate virtual function to calculate path & bounds */
		void size_allocate_vfunc(int width, int height, int baseline) override
		{
			Gtk::Box::size_allocate_vfunc(width, height, baseline);
			for(Gtk::Widget *pSegment=get_first_child();pSegment!=nullptr;pSegment=pSegment->get_next_sibling ())
			{
				Gtk::Allocation child_allocation;
  				auto r = static_cast<ColourWheelSegment*>(pSegment)->get_bounds(width, height);

				//Place the first visible child at the top-left:
				child_allocation.set_x(r->get_x());
				child_allocation.set_y(r->get_y());

				//Make it take up the full width available:
				child_allocation.set_width(r->get_width());
				child_allocation.set_height(r->get_height());
  
				static_cast<ColourWheelSegment*>(pSegment)->offset_from_parent = r->get_origin();
				static_cast<ColourWheelSegment*>(pSegment)->size_allocate(child_allocation, baseline);
			}
		}
		void measure_vfunc(Gtk::Orientation orientation, int for_size, int& minimum, int& natural,
			int& minimum_baseline, int& natural_baseline) const override
		{
			Gtk::Widget::measure_vfunc(orientation, for_size, minimum, natural, minimum_baseline, natural_baseline);
		}
		void snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>&snapshot) override;
		static void register_type()
		{
			static GType custom_gtype = 0; // gtype for the ColourWheelSegment class

			if (custom_gtype != 0)
				return; // Prevent duplicate registration

			// Instantiate a temporary instance to resolve the unique internal GType runtime tracking ID
			ColourWheel dummy; 
			GtkBox* raw_widget = dummy.gobj();
			custom_gtype = G_OBJECT_TYPE(raw_widget);

			// Register our C++ factory method to handle this unique GType globally
			Glib::wrap_register(custom_gtype, &ColourWheel::wrap_new);
			
			// Verify registration worked
			GType found_type = g_type_from_name("gtkmm__CustomObject_ColourWheel");
			if(!found_type)
			{
				Glib::ustring buffer="class gtkmm__CustomObject_ColourWheel not registered";
				critical(buffer);
			}
		}
		/* utility methods */
		unsigned int get_segment_id(Gtk::Widget *pSegment)
		{
			unsigned int id=0;
			for(Gtk::Widget *p = get_first_child ();p!=nullptr && p!=pSegment;p = p->get_next_sibling (), id++);
			return id;
		}
		unsigned int get_segment_count()
		{
			unsigned int count=0;
			for(Gtk::Widget *p = get_first_child ();p!=nullptr;p = p->get_next_sibling (), count++);
			return count;
		}
	protected:
		static Glib::ObjectBase* wrap_new(GObject* o)
		{
			// Tie lifetime cleanup directly to the parent widget lifecycle
			return Gtk::manage(new ColourWheel(GTK_WIDGET(o)));
		}
		bool on_legacy_event(const Glib::RefPtr<const Gdk::Event>&event)
		{
			switch (event->get_event_type ())
			{
				case Gdk::Event::Type::BUTTON_PRESS:
					if (!mouse_pressed)
					{
						mouse_pressed = true;
						focus_mouse_segment();
					}
					return true;
				case Gdk::Event::Type::BUTTON_RELEASE:
					if (mouse_point.is_valid && get_mouse_point_segment())
					{
						select_segment(get_mouse_point_segment());
					}
					else if (mouse_pressed)
					{
						mouse_pressed = false;
						focus_mouse_segment();
					}
					return true;
				default:
					return false;
			}
		}
		bool on_keypress_event(const unsigned int &keyval, const unsigned int &keycode, const Gdk::ModifierType &state)
		{
			switch (keyval)
			{
				case 65293: /* enter key */
				case 65421: /* keypad enter key */
				case 32: /* space key */
					if (auto focus_segment=get_focus_child())
					{
						select_segment(focus_segment);
						return true;
					}
					else
						return false;
				default:
					return false;
			}
		}
		void select_segment(Gtk::Widget *segment);
		/* todo convert to gtkmm see https://gitlab.gnome.org/GNOME/gtkmm/-/commit/a21fc8a0fc13fa9431fe15e9a3e83b507c1970b9 */
		static void custom_class_init(void* g_class, void* class_data)
		{
			GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(g_class);
			widget_class->focus = &ColourWheel::on_focus_vfunc;
		}
		/* todo convert to gtkmm see https://gitlab.gnome.org/GNOME/gtkmm/-/commit/a21fc8a0fc13fa9431fe15e9a3e83b507c1970b9 */
	 	bool focus_vfunc(Gtk::DirectionType direction,Gtk::Widget *&set_focus_child);
		/* todo convert to gtkmm see https://gitlab.gnome.org/GNOME/gtkmm/-/commit/a21fc8a0fc13fa9431fe15e9a3e83b507c1970b9 */
	 	static gboolean on_focus_vfunc(GtkWidget *self, GtkDirectionType direction)
	 	{
	 		Gtk::DirectionType d;
	 		switch(direction)
	 		{
	 			case GTK_DIR_TAB_FORWARD:
		 			d=Gtk::DirectionType::TAB_FORWARD;
		 			break;
		 		case GTK_DIR_TAB_BACKWARD:
		 			d=Gtk::DirectionType::TAB_BACKWARD;
		 			break;
		 		case GTK_DIR_UP:
		 			d=Gtk::DirectionType::UP;
		 			break;
		 		case GTK_DIR_DOWN:
		 			d=Gtk::DirectionType::DOWN;
		 			break;
		 		case GTK_DIR_LEFT:
		 			d=Gtk::DirectionType::LEFT;
		 			break;
		 		case GTK_DIR_RIGHT:
		 			d=Gtk::DirectionType::RIGHT;
		 			break;
		 		default:
		 			return false;
	 		}
	 		Gtk::Widget* wrapped_widget = Glib::wrap(self);
	 		if(auto cpp_self = dynamic_cast<ColourWheel*>(wrapped_widget))
	 		{
				Gtk::Widget *set_focus_child; /* returned value */
	 			if(cpp_self->focus_vfunc(d, set_focus_child) && set_focus_child)
	 			{
					gtk_widget_child_focus(set_focus_child->gobj(), direction);
					return true;
				}
			}
	 		return false;
	 	}
	 	Gtk::Widget *get_child(unsigned int ID)
	 	{
	 		unsigned int i;
			Gtk::Widget *p,*last=nullptr;
			for(i=0,p=get_first_child();p!=nullptr && i<ID;last=p, p=p->get_next_sibling(), i++);
			return p?p:last;
	 	}
	private:
		/* if the mouse button is pressed focus the segment its over */
		void focus_mouse_segment()
		{
			if (mouse_point.is_valid)
			{
				auto segment = get_mouse_point_segment();
				if (nullptr != segment)
				{
					if (mouse_pressed && !segment->is_focus())
					{
						auto lose_focus = get_focus_child();
						if (nullptr != lose_focus)
							lose_focus->queue_draw();
						if(!segment->grab_focus())
						{
							Glib::ustring buffer = "gtkmm__CustomObject_ColourWheelSegment::grab_focus(): id=\"";
							buffer += segment->get_buildable_id();
							buffer += "\" failed!";
							critical(buffer);
						}
						segment->queue_draw();
					}
					return;
				}
			}
			/*auto lose_focus = get_focus_child ();
			if (nullptr != lose_focus)
				lose_focus->queue_draw();*/
		}
		/* return the segment the mouse pointer is over, if any */
		ColourWheelSegment *get_mouse_point_segment ();
	private:
		bool mouse_pressed;
		struct {bool is_valid; float x; float y;} mouse_point;
	private:
		unsigned int get_players(std::vector<PlayerButton*> &players)
		{
			auto box=get_parent();
			if(box->get_buildable_id()!="player_controls")
			{
				Glib::ustring buffer="player-controls.ui: id=\"player_controls\" is not the parent of id=\"";
				buffer+=get_buildable_id();
				buffer+="\" id=\"";
				buffer+=box->get_buildable_id();
				buffer+="\" is!";
				critical(buffer);
			}
			auto grid=box->get_parent();
			if(grid->get_buildable_id()!="grids_box")
			{
				Glib::ustring buffer="nibble-window.ui & player-controls.ui: id=\"grids_box\" is not the parent of id=\"";
				buffer+=box->get_buildable_id();
				buffer+="\", id=\"";
				buffer+=grid->get_buildable_id();
				buffer+="\" is!";
				critical(buffer);
			}
			players.clear();
			for(auto box=grid->get_first_child();box;box=box->get_next_sibling())
			{
				auto button=box->get_first_child();
				assert(button->get_buildable_id()=="name_label");
				players.push_back(static_cast<PlayerButton*>(button));
			}
			return players.size();
		}
   	};

	/* gtkmm__CustomObject_ColourWheelSegment is a gtk template */
	class ColourWheelSegment : public Gtk::Widget
	{                  
	public:
		const double PIx2 = 6.28318530717958647692528676655900577; /* 2Pi */

		/* constructor used for registering class via dummy object in register_type() */
		ColourWheelSegment() : Glib::ObjectBase("ColourWheelSegment"), Gtk::Widget(),
			property_colour_(*this, "colour", 0xffffff)
		{
		}
		/* constructor used by Gtk::Builder::get_widget_derived<ColourWheelSegment> */
		ColourWheelSegment(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder) : Gtk::Widget(cobject),
			property_colour_(*this, "colour", 0xffffff)
		{
			set_can_focus();
			set_focus_on_click();
			//focusable = true;
			set_sensitive();
		}
		virtual ~ColourWheelSegment() = default;
		/* override the size_allocate virtual function to calculate path & bounds */
		void size_allocate_vfunc(int width, int height, int baseline) override
		{
			Gtk::Widget::size_allocate_vfunc(width, height, baseline);
		}
		void measure_vfunc(Gtk::Orientation orientation, int for_size, int& minimum, int& natural,
			int& minimum_baseline, int& natural_baseline) const override
		{
			Gtk::Widget::measure_vfunc(orientation, for_size, minimum, natural, minimum_baseline, natural_baseline);
		}
		void snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>&snapshot);
		void set_colour(const gulong& colour)
		{
			property_colour_ = colour;
		}
		gulong get_colour() const
		{
			return property_colour_.get_value();
		}
		explicit ColourWheelSegment(GtkWidget* gobj) :
			Glib::ObjectBase(nullptr), // Passing nullptr avoids allocating a duplicate GObject
			Gtk::Widget(gobj),
			property_colour_(*this, "colour", 0xffffff)
		{
		}
		static void register_type()
		{
			static GType custom_gtype = 0; // gtype for the ColourWheelSegment class

			if (custom_gtype != 0)
				return; // Prevent duplicate registration

			// Instantiate a temporary instance to resolve the unique internal GType runtime tracking ID
			ColourWheelSegment dummy; 
			GtkWidget* raw_widget = dummy.gobj();
			custom_gtype = G_OBJECT_TYPE(raw_widget);

			// Register our C++ factory method to handle this unique GType globally
			Glib::wrap_register(custom_gtype, &ColourWheelSegment::wrap_new);
			
			// Verify registration worked
			GType found_type = g_type_from_name("gtkmm__CustomObject_ColourWheelSegment");
			if(!found_type)
			{
				Glib::ustring buffer="class gtkmm__CustomObject_ColourWheelSegment not registered";
				critical(buffer);
			}
		}
		const std::optional<Gdk::Graphene::Rect> get_bounds(unsigned int width, unsigned int height);
	private:
		static Glib::ObjectBase* wrap_new(GObject* o)
		{
			// Tie lifetime cleanup directly to the parent widget lifecycle
			return Gtk::manage(new ColourWheelSegment(GTK_WIDGET(o)));
		}
		Glib::Property<gulong> property_colour_;
	public:
		Glib::RefPtr<Gsk::Path> calculate_segment_path (uint width, uint height, uint ID, uint segment_count);
		Gdk::Graphene::Point offset_from_parent; /* x & y position relative to parent */
   	};

	class OverlayMessage : public Gtk::Widget
	{
	public:
		OverlayMessage (const Glib::ustring &text, const int width) :
			Gtk::Widget(), text(text), width(width)
		{
		}
		void snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>&snapshot) override
		{
			Gtk::Widget::snapshot_vfunc(snapshot);
			draw_text(snapshot,	text, width, false, *this);
		}
	private:
		const Glib::ustring text;
		const int width;
	};

	class PlayerButton : public Gtk::Button, public KeyHandler
	{
	public:
		PlayerButton(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder, const guint &id, NibblesWindow *pWindow) :
			Glib::ObjectBase(nullptr), Gtk::Button(cobject), KeyHandler(pWindow), id(id), pWindow(pWindow)
		{
			// remember grid_overlay
			pOverlay = refBuilder->get_widget<Gtk::Overlay>("grid_overlay");
			
			// remember wheel
			pWheel = Gtk::Builder::get_widget_derived<ColourWheel>(refBuilder, "wheel");
			auto pWheelSegment0 = Gtk::Builder::get_widget_derived<ColourWheelSegment>(refBuilder, "segment0");
			if(!pWheelSegment0)
				critical("player-controls.ui: id=\"segment0\" not found!");
			auto pWheelSegment1 = Gtk::Builder::get_widget_derived<ColourWheelSegment>(refBuilder, "segment1");
			if(!pWheelSegment1)
				critical("player-controls.ui: id=\"segment1\" not found!");
			auto pWheelSegment2 = Gtk::Builder::get_widget_derived<ColourWheelSegment>(refBuilder, "segment2");
			if(!pWheelSegment2)
				critical("player-controls.ui: id=\"segment2\" not found!");
			auto pWheelSegment3 = Gtk::Builder::get_widget_derived<ColourWheelSegment>(refBuilder, "segment3");
			if(!pWheelSegment3)
				critical("player-controls.ui: id=\"segment3\" not found!");
			auto pWheelSegment4 = Gtk::Builder::get_widget_derived<ColourWheelSegment>(refBuilder, "segment4");
			if(!pWheelSegment4)
				critical("player-controls.ui: id=\"segment4\" not found!");
			auto pWheelSegment5 = Gtk::Builder::get_widget_derived<ColourWheelSegment>(refBuilder, "segment5");
			if(!pWheelSegment5)
				critical("player-controls.ui: id=\"segment5\" not found!");

			// create widget wraps
			pArrow_up = Gtk::Builder::get_widget_derived<Arrow>(refBuilder, "arrow_up");
			pArrow_up->player = this;
			pArrow_left = Gtk::Builder::get_widget_derived<Arrow>(refBuilder, "arrow_left");
			pArrow_left->player = this;
			pArrow_right = Gtk::Builder::get_widget_derived<Arrow>(refBuilder, "arrow_right");
			pArrow_right->player = this;
			pArrow_down = Gtk::Builder::get_widget_derived<Arrow>(refBuilder, "arrow_down");
			pArrow_down->player = this;

			// attach on_clicked()
			signal_clicked().connect(sigc::mem_fun(*this, &PlayerButton::on_clicked));

			// ensure our colour is unique
			std::unordered_set<eWormColour> colours_used;
			for(unsigned int i=0;i<=id;i++)
			{
				eWormColour colour=get_worm_settings_colour(i);
				if(unknown_colour_worm==colour || colours_used.contains(colour))
				{
					// invalid colour or colour clash so choose an unused colour
					for(colour=red_worm;colours_used.contains(colour);++colour);
					set_worm_settings_colour(i,colour);
				}
				colours_used.insert(colour);
			}

			// set button text
			set_text(get_worm_settings_colour(id));
			
			// set key direction buttons' text & action
			set_key_buttons(refBuilder,id);
		}
		virtual ~PlayerButton() = default;
		void on_clicked()
		{
			if(pOverlay->is_visible())
			{
				pOverlay->set_visible(false);
				pWheel->set_visible(true);
			}
			else
			{
				pOverlay->set_visible(true);
				pWheel->set_visible(false);
			}
		}
		void set_text(unsigned int colour_number)
		{
			Glib::ustring name = "Player ";
			name += (char)('0'+(id+1));
			Glib::ustring markup = "<b><span font-family=\"Sans\" color=\"";
			const char *pango_colour[]={"#ff0000","#00c000","#0080ff","#ffff00","#00ffff","#c000c0"};
			markup += pango_colour[colour_number];
			markup += "\" size=\"x-large\">";
			markup += name;
			markup += "</span></b>";
			static_cast<Gtk::Label*>(get_child())->set_markup(markup);
		}
	protected:
		void on_clicked_up(Gtk::Widget *pWidget);
		void on_clicked_left(Gtk::Widget *pWidget);
		void on_clicked_right(Gtk::Widget *pWidget);
		void on_clicked_down(Gtk::Widget *pWidget);
	 	bool key_pressed(guint keyval, guint keycode) override;
	 	void redraw_arrows()
	 	{
			pArrow_up->queue_draw();
			pArrow_left->queue_draw();
			pArrow_right->queue_draw();
			pArrow_down->queue_draw();
	 	}
	public:
		bool check_for_key_clash();
		bool check_for_key_clash(const Glib::ustring &self);
		const std::array<unsigned int, 4> &get_raw_keys() const
		{
			return raw_keys;
		}
	public:
		const guint id;
	private:
		NibblesWindow *pWindow;
		Gtk::Overlay *pOverlay=nullptr;
		ColourWheel *pWheel=nullptr;
		Arrow *pArrow_up, *pArrow_left, *pArrow_right, *pArrow_down;
		std::array<unsigned int, 4> keys,raw_keys;/* up, left, right & down */
		struct
		{
			OverlayMessage *pKeyPressMessage=nullptr;
			unsigned long KeyToSet;/* 0=up, 1=left, 2=right & 3=down */
			Gtk::Widget *pLabel;/* Gtk::Label* */
		} key_pressed_data;
	private:
		unsigned int get_players(std::vector<PlayerButton*> &players)
		{
			auto box=get_parent();
			if(box->get_buildable_id()!="player_controls")
			{
				Glib::ustring buffer="player-controls.ui: id=\"player_controls\" is not the parent of id=\"";
				buffer+=get_buildable_id();
				buffer+="\" id=\"";
				buffer+=box->get_buildable_id();
				buffer+="\" is!";
				critical(buffer);
			}
			auto grid=box->get_parent();
			if(grid->get_buildable_id()!="grids_box")
			{
				Glib::ustring buffer="nibble-window.ui & player-controls.ui: id=\"grids_box\" is not the parent of id=\"";
				buffer+=box->get_buildable_id();
				buffer+="\", id=\"";
				buffer+=grid->get_buildable_id();
				buffer+="\" is!";
				critical(buffer);
			}
			players.clear();
			for(auto box=grid->get_first_child();box;box=box->get_next_sibling())
			{
				auto button=box->get_first_child();
				assert(button->get_buildable_id()=="name_label");
				players.push_back(static_cast<PlayerButton*>(button));
			}
			return players.size();
		}
		void get_key_settings(Glib::RefPtr<Gio::Settings> pWormSettings, unsigned int &key, unsigned int &raw_key, const char *key_string);
		void set_key_buttons(const Glib::RefPtr<Gtk::Builder>& refBuilder, unsigned int id);
	};
	class Scores : public Gtk::Window
	{
	private:
		Gtk::HeaderBar m_headerbar;
		struct Score
		{
			uint64_t score;
			uint64_t date;
			std::string name;
		};
		/* to do, combine the next two maps */
		std::unordered_map<uint8_t, std::vector<Score>> m_scores;
		std::unordered_map<uint8_t, Glib::ustring> m_score_file;
		uint8_t m_display_category;
	public:
		Scores()
		{
			set_titlebar(m_headerbar);
			add_trash_icon();
		}
		~Scores() override
		{
			clear();
		}
		uint8_t to_category(unsigned long speed, bool fakes, unsigned long progress, unsigned long level) const;
		const Glib::ustring &get_file(uint8_t category_index) {return m_score_file[category_index];}
		const std::vector<Score> &get_scores(uint8_t category_index) {return m_scores[category_index];}
		void add_category(const Glib::ustring &path, const Glib::ustring &file_name);
		std::vector<unsigned long> add(uint8_t category_index, const std::vector<WormScore> &scores);
		void clear()
		{
			m_scores.clear();
			m_score_file.clear();
		}
		bool is_empty() const
		{
			return m_scores.empty();
		}
		void set_title()
		{
			if(m_scores.size()==1)
			{
				auto it=m_scores.cbegin();
				uint8_t category_index=it->first;
				auto* title_label = Gtk::make_managed<Gtk::Label>(to_title(category_index));
				title_label->add_css_class("title");
				m_headerbar.set_title_widget(*title_label);
				display_scores(category_index);
			}
			else if(m_scores.size()>1)
			{
				auto [category_index, strings]=get_ordered_categories();
				auto* title = Gtk::make_managed<Gtk::DropDown>(strings);
				title->property_selected().signal_changed().connect(sigc::track_obj(
					[category_index, title, this]() ->
						void
						{
							auto selected = title->get_selected();
							if(selected!=GTK_INVALID_LIST_POSITION)
								display_scores(category_index[selected]);
						},
						category_index, title, *this
					));
				m_headerbar.set_title_widget(*title);
				display_scores(category_index[0]);
			}
		}
		std::pair<bool,Glib::ustring> create_scores_directory()
		{
			auto path=Glib::build_filename(Glib::get_user_data_dir());
			if(create_directory(path))
			{
				path=Glib::build_filename(path, "gnome-nibbles");
				if(create_directory(path))
				{
					path=Glib::build_filename(path, "scores");
					if(create_directory(path))
						return {true,path};
				}
			}
			return {false,{}};
		}
		void set_name(uint8_t category_index, unsigned long row, const Glib::ustring &name)
		{
			m_scores[category_index][row].name=name;
		}
	private:
		class RowData : public Glib::Object {
			unsigned long rank;
			unsigned long score;
			Glib::ustring name;
			bool modify;
		protected:
			RowData(unsigned long rank, unsigned long score, const Glib::ustring& name, bool modify=false)
				: Glib::ObjectBase(typeid(RowData)), rank(rank), score(score), name(name), modify(modify)
			{
			}
		public:
			static Glib::RefPtr<RowData> create(unsigned long rank, unsigned long score, const Glib::ustring& name, bool modify=false)
			{
				return Glib::make_refptr_for_instance<RowData>(new RowData(rank, score, name, modify));
			}
			unsigned long get_rank() const { return rank; }
			unsigned long get_score() const { return score; }
			Glib::ustring get_name() const { return name; }
			bool get_modify() const { return modify; }
		};
		bool create_directory(const Glib::ustring &directory)
		{
			Glib::RefPtr<Gio::File> dir = Gio::File::create_for_path(directory);
			try
			{
				auto info = dir->query_info(G_FILE_ATTRIBUTE_STANDARD_TYPE);
				if(info->get_file_type() == Gio::FileType::DIRECTORY)
				{
					/* nothing to do */
					return true;
				}
				else
				{
					/* we have a problem */
					return false;
				}
			}
			catch(const Glib::Error& ex)
			{
				/* try to create the directory */
				try
				{
					dir->make_directory();
					return true;
				}
				catch(const Glib::Error& make_ex)
				{
					/* we have a problem */
					return false;
				}
			}
		}
		void add_trash_icon()
		{
			auto* trash_button = Gtk::make_managed<Gtk::Button>();
			auto* trash_icon = Gtk::make_managed<Gtk::Image>();
			trash_icon->set_from_icon_name("user-trash-symbolic");
			trash_button->set_child(*trash_icon);
			trash_button->add_css_class("flat");
			trash_button->set_tooltip_text("Clear Scores");
			trash_button->signal_clicked().connect(sigc::track_obj(
				[this]() ->
					void
					{
						auto [success, path]=create_scores_directory();
						if(success)
						{
							if(m_score_file.contains(m_display_category))
							{
								auto file_name=Glib::build_filename(path, m_score_file[m_display_category]);
								Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(file_name);
								try
								{
									file->remove();
								}
								catch(const Glib::Error& ex)
								{
								}
							}
						}
						close();
					},
					*this
				));
			m_headerbar.pack_start(*trash_button);
		}
		std::pair<uint64_t,bool> read_integer(std::ifstream &stream);
		std::pair<std::string,bool> read_string(std::ifstream &stream);
		uint8_t to_catagory_index(const Glib::ustring &file_name)
		{
			uint8_t speed=4; /* 1 to 4 inclusive */
			uint8_t fakes=0; /* 0 to 1 inclusive */
			uint8_t fixed=0; /* 0 - not fixed, 1-26 fixed at level, 31 random */
			/* read each token from the file name */
			for(const char *p=file_name.c_str();p;)
			{
				const char *e;
				for(e=p;*e && *e!='-';e++);
				if(match(p,e,"beginner"))
					speed = 4;
				else if(match(p,e,"slow"))
					speed = 3;
				else if(match(p,e,"medium"))
					speed = 2;
				else if(match(p,e,"fast"))
					speed = 1;
				else if(match(p,e,"fakes"))
					fakes = 1;
				else if(match(p,e,"random"))
					fixed = 31;
				else if(e-p>5 && strncmp("fixed", p, 5)==0)
				{
					fixed=0;
					p+=5;
					for(;p<e && *p>='0' && *p<='9';)
					{
						fixed*=10;
						fixed+=(*p-'0');
						p++;
					}
					if(fixed<1 || fixed>26)
						fixed=1;
				}
				p=e;
				if(*p)
					p++;
				if(!*p)
					break;
			}
			return (speed-1)/*2 bits wide*/ | (fakes<<2)/*1 bit wide*/ | (fixed<<3)/*5 bits wide*/;
		}
		std::string to_title(uint8_t category_index);
		bool match(const char *s, const char *e, const std::string &str)
		{
			return e-s == str.length() && strncmp(str.c_str(),s,e-s)==0;
		}
		std::pair<std::vector<uint8_t>,std::vector<Glib::ustring>> get_ordered_categories()
		{
			std::set<uint8_t> categories;
			for(const auto &c : m_scores)
			{
				uint8_t lo_order=c.first>>2;
				uint8_t hi_order=( (c.first & 0b11) ^ 0b11 )<<6;
				categories.insert(hi_order | lo_order);
			}
			std::vector<uint8_t> r0;
			std::vector<Glib::ustring> r1;
			for(uint8_t i : categories)
			{
				uint8_t index=i<<2 | ((i>>6)^0b11);
				r0.emplace_back( index );
				r1.emplace_back(to_title( index ));
			}
			return {r0,r1};
		}
		void scores_to_store(uint8_t category_index, Glib::RefPtr<Gio::ListStore<RowData>> &store)
		{
			unsigned long rank=0;
			for(const auto &score : m_scores[category_index])
			{
				store->append(RowData::create(++rank, score.score, score.name));
				if(rank>=10)
					break;
			}
		}
		void display_scores(uint8_t category_index);
	};
	
	static NibblesWindow* create(const char *program_name, int cli_start_level, eSetupScreen start_screen)
	{
		Arrow::register_type();
		ColourWheel::register_type();
		ColourWheelSegment::register_type();
		auto refBuilder = Gtk::Builder::create_from_resource("/org/gnome/Nibbles/ui/nibbles-window.ui");
		auto window = Gtk::Builder::get_widget_derived<NibblesWindow>(
			refBuilder, 
			"nibbles-window", program_name, cli_start_level, start_screen);
		if (!window)
		{
			Glib::ustring buffer="nibbles-window.ui: No \"nibbles_window\" object.";
		   	critical(buffer);
		}

		return window;
	}

	NibblesWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refBuilder,
			const char *program_name, int _cli_start_level, eSetupScreen start_screen)
			: Gtk::ApplicationWindow(cobject), m_refBuilder(refBuilder)
	{
		set_title(program_name);
		set_default_size(300,200);
		pSettings = Gio::Settings::create("org.gnome.Nibbles");

		initilise_css();

		cli_start_level = _cli_start_level;
		//start_screen = _start_screen;

		initilise_about();

		// Bind UI variables to the template elements defined in the .ui file
		pScreenStack = m_refBuilder->get_widget<Gtk::Stack>("main_stack");
		if (!pScreenStack)
		{
			Glib::ustring buffer="nibbles-window.ui: No \"main_stack\" object.";
		   	critical(buffer);
		}

		/* common call backs from the screens */
		add_action("next-screen", sigc::mem_fun(*this, &NibblesWindow::next_callback));
		add_action("back", sigc::mem_fun(*this, &NibblesWindow::back_callback));
		add_action("start-game", sigc::mem_fun(*this, &NibblesWindow::next_callback));

		initilise_players();

		initilise_progress();

		initilise_speed_and_fakes();
	
		initilise_keys();
		
		/* Check whether to display the first run screen */
		switch (start_screen)
		{
			case GAME:
				/*
				game.numhumans = pSettings->get_int (PLAYER_SETTINGS);
				game.numai     = pSettings->get_int (AI_SETTINGS);
				game.speed     = pSettings->get_int (SPEED_SETTINGS);
				game.fakes     = pSettings->get_boolean (FAKE_SETTINGS);
				game.create_worms (worm_settings);
				start_game ();
				*/
				break;
			case CONTROLS:
				/*
				game.numhumans = pSettings->get_int (PLAYER_SETTINGS);
				game.numai     = pSettings->get_int (AI_SETTINGS);
				game.speed     = pSettings->get_int (SPEED_SETTINGS);
				game.fakes     = pSettings->get_boolean (FAKE_SETTINGS);
				show_controls_screen ();
				*/
				break;
			case SPEED:
				/*
				game.numhumans = pSettings->get_int (PLAYER_SETTINGS);
				game.numai     = pSettings->get_int (AI_SETTINGS);
				main_stack.set_visible_child_name (SPEED_SETTINGS);
				*/
				break;
			default:
				if (pSettings->get_boolean ("first-run"))
				{
					pSettings->set_boolean ("first-run",false);
					ScreenStack_set_visible_child(FIRST_RUN);
				}
				else
					ScreenStack_set_visible_child(PLAYERS);
				break;
		}
	}

protected:
	Glib::RefPtr<Gtk::Builder> m_refBuilder;
	Gtk::Stack* pScreenStack {nullptr};
	Glib::RefPtr<Gio::Settings> pSettings;
	Glib::RefPtr<Gio::SimpleAction> pPlayerButtons;
	Glib::RefPtr<Gio::SimpleAction> pAiButtons;
	Glib::RefPtr<Gio::SimpleAction> pProgressButtons;
	Glib::RefPtr<Gio::SimpleAction> pSpeedButtons;
	Glib::RefPtr<Gio::SimpleAction> pFakeButton;

private:
	Gtk::ToggleButton* GetToggleButton (const Glib::ustring& name, unsigned int number)
	{
		Glib::ustring button_name;
		button_name += name;
		button_name += (char)('0' + number);
		return m_refBuilder->get_widget<Gtk::ToggleButton>(button_name);
	}
	
	Gtk::ToggleButton* GetToggleButton (const Glib::ustring& name)
	{
		Glib::ustring button_name = name;
		return m_refBuilder->get_widget<Gtk::ToggleButton>(button_name);
	}

	Gtk::Button* GetButton (const Glib::ustring& name)
	{
		Glib::ustring button_name = name;
		return m_refBuilder->get_widget<Gtk::Button>(button_name);
	}

	Gtk::Widget* GetWidget (const Glib::ustring& name)
	{
		Glib::ustring widget_name = name;
		return m_refBuilder->get_widget<Gtk::Widget>(widget_name);
	}

	Gtk::Box* GetBox (const Glib::ustring& name)
	{
		Glib::ustring box_name = name;
		auto box=m_refBuilder->get_widget<Gtk::Box>(box_name);
		if(!box)
		{
				Glib::ustring buffer="nibbles-window.ui: id=\"";
				buffer+=name;
				buffer+="\" not found!";
				critical(buffer);
		}
		return box;
	}

	void set_14_point(Gtk::Label *pLabel)
	{
		set_14_point(pLabel, pLabel->get_text ());
	}
	
	void set_14_point(Gtk::Label *pLabel, const Glib::ustring& text)
	{
		Glib::ustring label;
		label = "<b><span size=\"14.0pt\" font-family=\"Sans\">";
		label += text;
		label += "</span></b>";
		pLabel->set_markup(label);
		pLabel->set_margin_top (14);
		pLabel->set_margin_bottom (14);
		pLabel->set_margin_start (14);
		pLabel->set_halign (Gtk::Align::START);
	}

	void ScreenStack_set_visible_child (eSetupScreen screen,
		Gtk::StackTransitionType tt = Gtk::StackTransitionType::NONE)
	{
		ScreenSetup(screen);
		switch(screen)
		{
			case FIRST_RUN:
				pScreenStack->set_visible_child ("first_run", tt);
				break;
			case USUAL:
			case PLAYERS:
				pScreenStack->set_visible_child ("number_of_players", tt);
				break;
			case PROGRESS:
				pScreenStack->set_visible_child ("board_progress", tt);
				break;
			case SPEED:
				pScreenStack->set_visible_child ("speed", tt);
				break;
			case CONTROLS:
				pScreenStack->set_visible_child ("controls", tt);
				break;
			case GAME:
				pScreenStack->set_visible_child ("game_box", tt);
				break;
		}
	}

	void ScreenSetup(eSetupScreen screen)
	{
		switch(screen)
		{
			case FIRST_RUN:
				break;
			case PLAYERS:
				break;
			case PROGRESS:
				break;
			case SPEED:
				break;
			case CONTROLS:
				setup_controls();
				break;
			case GAME:
				setup_game();
				break;
			case USUAL:
				break;
		}
	}

	void ScreenSave(eSetupScreen screen)
	{
		switch(screen)
		{
			case FIRST_RUN:
				break;
			case PLAYERS:
				save_ai_count();
				break;
			case PROGRESS:
				break;
			case SPEED:
				break;
			case CONTROLS:
				break;
			case GAME:
				break;
			case USUAL:
				break;
		}
	}

	void next_callback()
	{
		auto s=pScreenStack->get_visible_child_name ();
		eSetupScreen e=name_to_screen(s);
		ScreenSave(e);
		switch(e)
		{
			case FIRST_RUN:
				ScreenStack_set_visible_child(PLAYERS, Gtk::StackTransitionType::SLIDE_UP);			
				break;
			default:
			case PLAYERS:
				ScreenStack_set_visible_child(PROGRESS, Gtk::StackTransitionType::SLIDE_UP);			
				break;
			case PROGRESS:
				ScreenStack_set_visible_child(SPEED, Gtk::StackTransitionType::SLIDE_UP);			
				break;
			case SPEED:
				ScreenStack_set_visible_child(CONTROLS, Gtk::StackTransitionType::SLIDE_UP);			
				break;
			case CONTROLS:
				ScreenStack_set_visible_child(GAME, Gtk::StackTransitionType::SLIDE_UP);
				break;
		}
	}

	void back_callback();
	void update_high_scores(unsigned long speed, bool fakes,
		unsigned long progress,	unsigned long level, const std::vector<WormScore> &scores);
	void launch_help();

	KeyHandler *key_handler=nullptr;
public:
	void set_key_handler(KeyHandler *pHandler)
	{
		key_handler = pHandler;
	}

	void check_and_enable_start_button()
	{
		/* only allow the game to start if there are no key clashes */
		GetButton("button")->set_sensitive(!check_for_key_clash());
	}

private:
	bool pass_key_to_view(guint keycode);
	bool on_key_pressed_callback(guint keyval, guint keycode, Gdk::ModifierType state);
	void pause_view(bool pause);
	void initilise_css();
	void initilise_about();
	void initilise_players();
	void initilise_progress();
	void initilise_speed_and_fakes();
	void initilise_keys();

	unsigned int get_players(std::vector<PlayerButton*> &players)
	{
		auto grid = GetBox("grids_box");
		players.clear();
		for(auto box=grid->get_first_child();box;box=box->get_next_sibling())
		{
			auto button=box->get_first_child();
			if(button->get_buildable_id()!="name_label")
			{
				Glib::ustring buffer="player-controls.ui: id=\"name_label\" is not the first child of id=\"player_controls\" id=\"";
				buffer+=box->get_buildable_id();
				buffer+="\" is!";
				critical(buffer);
			}
			players.push_back(static_cast<PlayerButton*>(button));
		}
		return players.size();
	}

	bool check_for_key_clash()
	{
		std::vector<PlayerButton*> players;
		auto players_count=get_players(players);
		for(uint u=0;u<players_count;u++)
		{
			if(players[u]->check_for_key_clash())
				return true;
		}
		return false;
	}
	
	unsigned int find_ai_button(int player_count, int ai)
	{
		unsigned int button_number = player_count + ai;
		if(button_number < 2)
			return 2;
		else if(button_number > 6)
			return 6;
		else
			return button_number;
	}

	unsigned int player_count_selection;
	void change_players_number_callback(const int &i)
	{
		player_count_selection = i;
		pSettings->set_int(PLAYER_SETTINGS,player_count_selection);
		auto state = Glib::Variant<int>::create(i);
		GetToggleButton("worms",2)->set_visible (i <= 2);
		GetToggleButton("worms",3)->set_visible (i <= 3);
		pPlayerButtons->set_state(state);
		Glib::ustring label;
		for(int ai = i == 1 ? 1 : 0; ai <= 6 - i; ai++)
		{
			label = "<b><span size=\"30.0pt\" font-family=\"Sans\">";
			label += (char)('0' + ai);
			label += "</span></b>";
			static_cast<Gtk::Label*>(GetToggleButton("worms", find_ai_button(i, ai))->get_child ())->set_markup(label);
		}
	}

	//unsigned int ai_count_selection;
	void change_nibbles_number_callback(const int &i)
	{
		/*auto ai_count_selection = i - player_count_selection;
		pSettings->set_int(AI_SETTINGS, ai_count_selection);*/
		auto state = Glib::Variant<int>::create(i);
		pAiButtons->set_state(state);
	}
	void save_ai_count()
	{
		auto p=pAiButtons->property_state();
		Glib::VariantBase data = p.get_value();
		if(data)
		{
			try {
				auto int_variant = Glib::VariantBase::cast_dynamic<Glib::Variant<int>>(data);
				auto ai_count_selection = int_variant.get()-player_count_selection;
				pSettings->set_int(AI_SETTINGS,ai_count_selection);
			} catch (const std::bad_cast& e) {
				Glib::ustring buffer = "The type of pAiButtons->property_state() is ";
				buffer+=data.get_type_string();
				buffer+=" not i(integer)!";
				critical(buffer);
			}
		}
	}

	Gtk::SpinButton* spin = nullptr;
	unsigned int progress_selection;
	void change_progress_callback(const int &progress)
	{
		progress_selection = progress;
		pSettings->set_int(PROGRESS_SETTINGS,progress_selection);
		for(int i=0; i<=2; i++)
		{
			GetToggleButton("progress", i)->set_has_frame (i == progress);
		}
		enable_disable_spin(progress_selection);
		auto state = Glib::Variant<int>::create(progress);
		pProgressButtons->set_state(state);
	}
	void enable_disable_spin(unsigned int progress)
	{
		if(progress == 2)
		{
			if (nullptr == spin)
			{
				spin = Gtk::make_managed<Gtk::SpinButton>();
				spin->set_range(1, 26);
				spin->set_increments(1, 1);
				spin->set_halign(Gtk::Align::END);
				spin->get_adjustment()->signal_value_changed().connect(
					sigc::mem_fun(*this, &NibblesWindow::change_spin_callback) );
				spin->set_value(std::clamp(pSettings->get_int(LEVEL_SETTINGS), 1 , 26)); /* initial selection */
				m_refBuilder->get_widget<Gtk::Overlay>("spin")->add_overlay (*spin);
			}
		}
		else
		{
			if(spin)
			{
				m_refBuilder->get_widget<Gtk::Overlay>("spin")->remove_overlay (*spin);
				spin=nullptr;
			}
		}
	}
	unsigned int progress_fixed_level_selection;
	void change_spin_callback()
	{
		progress_fixed_level_selection = spin->get_value_as_int ();
		pSettings->set_int(LEVEL_SETTINGS,progress_fixed_level_selection);
	}

	unsigned int speed_selection;
	void change_speed_callback(const int &speed)
	{
		speed_selection = speed;
		pSettings->set_int(SPEED_SETTINGS,speed_selection);
		for(unsigned int i=1; i<=4; i++)
		{
			GetToggleButton("speed", i)->set_has_frame (i == speed);
		}
		auto state = Glib::Variant<int>::create(speed);
		pSpeedButtons->set_state(state);
	}

	bool fake_selection;
	void change_fake_callback()
	{
		fake_selection = !GetToggleButton("enable_fake_bonuses")->get_active();
		pSettings->set_boolean(FAKE_SETTINGS,fake_selection);
		auto state = Glib::Variant<bool>::create(fake_selection);
		pFakeButton->set_state(state);
	}
	
	void name_label_clicked_callback()
	{
		if(GetWidget("grid_overlay")->is_visible())
		{
			GetWidget("grid_overlay")->set_visible(false);
			GetWidget("wheel")->set_visible(true);
		}
		else
		{
			GetWidget("grid_overlay")->set_visible(true);
			GetWidget("wheel")->set_visible(false);
		}
	}
	void delete_view();
public:
	void new_game_cb()
	{
	}
	void fullscreen_cb()
	{
		if(full_screen)
		{
			full_screen=false;
			unfullscreen();
		}
		else
		{
			full_screen=true;
			fullscreen();
		}
	}
	void help_cb()
	{
		//if (false/*!is_game_paused()*/)
		//    activate_action ("pause");

		launch_help();
	}
	void about_cb()
	{
		//if (false/*!is_game_paused()*/)
		//    activate_action ("pause");
		about.present();
	}
	void scores_cb()
	{
		//scores.set_decorated(false);

		load_high_scores();

		if(scores.is_empty())
		{
			pause(true); /* pause the game */

			// Translators: text displayed in an inform box
			auto* inform = new InformWindow(*this, "No high scores","You must compleate a game to create a high score.");
			
			// Handle the response callback asynchronously (non-blocking)
			inform->close_response().connect([this, inform]() {
				pause(false); /* continue the game */
				delete inform;
			});
			inform->present();
		}
		else
		{
			scores.set_transient_for(*this);
			scores.set_modal();
			scores.set_hide_on_close();
			scores.set_resizable(true);
			scores.set_title();
			scores.present();
		}
	}
	void pause_cb() /*toggle pause*/
	{
		auto button=GetButton("pause_button");
		if(button)
		{
	 		if(auto icon = dynamic_cast<Gtk::Image*>(button->get_child()))
	 		{
	 			if(icon->get_icon_name()=="media-playback-pause-symbolic")
	 			{
	 				/* pause the game*/
	 				pause_view(true);
	 				icon->set_from_icon_name("media-playback-start-symbolic");/* change icon to resume */
	 			}
	 			else
	 			{
	 				/* resume the game */
	 				pause_view(false);
	 				icon->set_from_icon_name("media-playback-pause-symbolic");/* change icon to pause */
	 			}
	 		}
		}
	}
	void pause(bool p)
	{
		auto button=GetButton("pause_button");
		if(button)
		{
	 		if(auto icon = dynamic_cast<Gtk::Image*>(button->get_child()))
	 		{
	 			if(p)
	 			{
	 				/* pause the game*/
	 				pause_view(true);
	 				icon->set_from_icon_name("media-playback-start-symbolic");/* change icon to resume */
	 			}
	 			else
	 			{
	 				/* resume the game */
	 				pause_view(false);
	 				icon->set_from_icon_name("media-playback-pause-symbolic");/* change icon to pause */
	 			}
	 		}
		}
	}
	void load_high_scores()
	{
		/* load high scores from score directory */
		scores.clear();
		auto [success, path]=scores.create_scores_directory();
		if(success)
		{
			Glib::RefPtr<Gio::File> directory = Gio::File::create_for_path(path);
			Gio::FileType type = directory->query_file_type(Gio::FileQueryInfoFlags::NONE);
			if(type == Gio::FileType::DIRECTORY)
			{
				try {
					Glib::RefPtr<Gio::FileEnumerator> enumerator = directory->enumerate_children("standard::name", Gio::FileQueryInfoFlags::NONE);
					Glib::RefPtr<Gio::FileInfo> file_info;
					while((file_info = enumerator->next_file()))
					{
						if(file_info->get_file_type()==Gio::FileType::REGULAR)
							scores.add_category(path, file_info->get_name());
					}
					enumerator->close();
				} catch (const std::exception& ex) {
					// no scores to load
				}
			}
			
		}
	}
	void save_high_scores(uint8_t category_index);
private:
	int cli_start_level;
	//eSetupScreen start_screen;
	bool full_screen=false;
	Gtk::AboutDialog about;
	Scores scores;
};


