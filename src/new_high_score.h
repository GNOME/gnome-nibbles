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
#include <gtkmm.h>
#include <iostream>

class NewHighScoreWindow : public Gtk::Window {
public:
	NewHighScoreWindow(Gtk::Window& parent) 
	{
		/* Translators: title for a window that records the player's new high score */
		set_title(_("New High Score"));
		set_default_size(300, 120);
		set_resizable(false);
		
		// Make it a modal transient popup for the parent window
		set_transient_for(parent);
		set_modal(true);

		// Styling and margins
		m_box.set_orientation(Gtk::Orientation::VERTICAL);
		m_box.set_spacing(15);
		m_box.set_margin(15);
		
			
		//m_label.set_halign(Gtk::Align::CENTER);
		//m_label.add_css_class("message");
		
		
		auto button_box=Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 10);
		button_box->set_halign(Gtk::Align::END);

		// Build the layout
		// Translators: text displayed in the button of the dialogue
		auto close_button = create_button(_("_Close"));
		button_box->append(*close_button);
		m_box.append(*button_box);
		set_child(m_box);

		close_button->signal_clicked().connect(sigc::track_obj(
			[this]()->
				void
				{
					m_responded=true;
					m_close_response.emit(get_names());
				},
				*this));
	}

	~NewHighScoreWindow()
	{
		for(auto &field : edit_fields)
		{
			set_last_used_name(field.first, field.second->get_text());
		}
	}

	// Define a signal so the main window can wait for the user
	sigc::signal<void(std::vector<Glib::ustring>)> close_response() { return m_close_response; }

	void add(const WormScore &ws)
	{
		auto row_box=Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 10);
		Gtk::Label *name=Gtk::make_managed<Gtk::Label>();
		constexpr std::array<const char *, 6> pango_colour = {"#ff0000","#00c000","#0080ff","#ffff00","#00ffff","#c000c0"};
		Glib::ustring markup="<span color=\"";
		markup+=pango_colour[ws.colour];
		markup+="\">";
		markup+=ws.worm_name;
		markup+="</span>";
		name->set_markup(markup);
		name->set_size_request(59,-1);
		name->set_xalign(0);

		Gtk::Label *score=Gtk::make_managed<Gtk::Label>(std::to_string(ws.score));
		
		auto edit=Gtk::make_managed<Gtk::Entry>();
		edit->set_text(get_last_used_name(ws.colour)); // Sets initial content
		edit_fields[ws.colour]=edit;
		
		row_box->append(*name);
		row_box->append(*score);
		row_box->append(*edit);
		m_box.prepend(*row_box);
	}
protected:
	bool on_close_request() override
	{
		if(!m_responded)
		{
			m_responded=true;
			m_close_response.emit(get_names());
		}
		return false; // Return false to let the window finish closing and destroying itself
	}
	Gtk::Box m_box;
	sigc::signal<void(std::vector<Glib::ustring>)> m_close_response;
	bool m_responded=false;
private:
	std::map<eWormColour,Gtk::Entry*> edit_fields;
	Gtk::Button *create_button(Glib::ustring text)
	{
		auto *button=Gtk::make_managed<Gtk::Button>(text);
		button->set_use_underline(true);
		button->add_css_class("rounded");
		//button->set_halign(Gtk::Align::CENTER);
		//button->set_margin_top(100);
		return button;
	}
	std::vector<Glib::ustring> get_names()
	{
		std::vector<Glib::ustring> result;
		auto *r=m_box.get_first_child();
		while(nullptr!=r)
		{
			if(auto edit = dynamic_cast<Gtk::Entry*>(r->get_last_child()))
				result.insert(result.begin(),edit->get_text());
			else
				break;
			r=r->get_next_sibling();
		}
		return result;
	}
	static constexpr std::array<const char *, 6> worm_colours = {"red","green","blue","yellow","cyan","purple"};
	Glib::ustring get_last_used_name(eWormColour colour)
	{
		for(uintsys worm_id=0;worm_id<6;worm_id++)
		{
			Glib::ustring my_settings = WORM_BASE_KEY;
			my_settings += (char)('0'+worm_id);
			auto pWormSettings = Gio::Settings::create(my_settings);
			auto string_colour=pWormSettings->get_string("color");
			if(string_colour==worm_colours[colour])
				return pWormSettings->get_string("high-score-name");
		}
		return "";
	}
	void set_last_used_name(eWormColour colour, const Glib::ustring &name)
	{
		for(uintsys worm_id=0;worm_id<6;worm_id++)
		{
			Glib::ustring my_settings = WORM_BASE_KEY;
			my_settings += (char)('0'+worm_id);
			auto pWormSettings = Gio::Settings::create(my_settings);
			auto string_colour=pWormSettings->get_string("color");
			if(string_colour==worm_colours[colour])
			{
				pWormSettings->set_string("high-score-name",name);
				break;
			}
		}
	}
};

