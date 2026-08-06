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

class InformWindow : public Gtk::Window {
public:
	InformWindow(Gtk::Window& parent, const std::string& title, const std::string& message) 
		: m_label(message)
	{
		set_title(title);
		set_default_size(300, 120);
		set_resizable(false);
		
		// Make it a modal transient popup for the parent window
		set_transient_for(parent);
		set_modal(true);

		// Styling and margins
		auto main_box=Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 15);
		main_box->set_margin(15);
		m_label.set_halign(Gtk::Align::CENTER);
		m_label.add_css_class("message");
		auto button_box=Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 10);
		button_box->set_halign(Gtk::Align::END);

		// Build the layout
		// Translators: text displayed in the button of the inform dialogue
		auto close_button = create_button(_("_Close"));
		button_box->append(*close_button);
		main_box->append(m_label);
		main_box->append(*button_box);
		set_child(*main_box);

		close_button->signal_clicked().connect(sigc::track_obj(
			[this]()->
				void
				{
					m_close_response.emit();
				},
				*this));
	}

	// Define a signal so the main window can wait for the user
	sigc::signal<void(void)> close_response() { return m_close_response; }

protected:
	bool on_close_request() override
	{
		m_close_response.emit();
		return false; // Return false to let the window finish closing and destroying itself
	}
	Gtk::Label m_label;
	sigc::signal<void(void)> m_close_response;
	bool m_responded=false;
private:
	Gtk::Button *create_button(Glib::ustring text)
	{
		auto *button=Gtk::make_managed<Gtk::Button>(text);
		button->set_use_underline(true);
		button->add_css_class("rounded");
		//button->set_halign(Gtk::Align::CENTER);
		//button->set_margin_top(100);
		return button;
	}
};


