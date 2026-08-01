#include <gtkmm.h>
#include <iostream>

// 1. The Confirmation Window Class
class ConfirmWindow : public Gtk::Window {
public:
    ConfirmWindow(Gtk::Window& parent, const std::string& message) 
        : m_label(message)
    {
        set_title("Confirmation");
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
		// Translators: text displayed in the negative button of a confirming message box
        auto no_button = create_button(_("_No"));
        button_box->append(*no_button);
		// Translators: text displayed in the positive button of a confirming message box
        auto yes_button = create_button(_("_Yes"));
        button_box->append(*yes_button);
        main_box->append(m_label);
        main_box->append(*button_box);
        set_child(*main_box);

        yes_button->signal_clicked().connect(sigc::track_obj(
        	[this]()->
        		void
        		{
					m_responded=true;
					m_signal_response.emit(true);
					//close(); // Closes and destroys the window
        		},
        		*this));
        no_button->signal_clicked().connect(sigc::track_obj(
        	[this]()->
        		void
        		{
					m_responded=true;
					m_signal_response.emit(false);
					//close(); // Closes and destroys the window
        		},
        		*this));        
    }

    // Define a signal so the main window can listen for the user's choice
    sigc::signal<void(bool)> signal_response() { return m_signal_response; }

protected:
    bool on_close_request() override {
        // If they closed via 'X' without clicking Yes or No explicitly
        if(!m_responded)
        {
			m_responded=true;
	        m_signal_response.emit(false); // Emit "false" (cancelled/No)
	    }
        return false; // Return false to let the window finish closing and destroying itself
    }
    Gtk::Label m_label;
    sigc::signal<void(bool)> m_signal_response;
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


