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

#include <cstdlib>
#include <iostream>
#include <cassert>

#include <glibmm.h>
#include <gtkmm.h>
//#include <giomm.h>
#include <gtkmm/eventcontrollerlegacy.h>

#include <unordered_set>/* required for std::unordered_set */

/* language */
#include <locale>
#include <glib/gi18n.h>

#include "definitions.h"
#include "nibbles.h"
#include "boolean.h"
#include "nibbles-window.h"

class Nibbles : public Gtk::Application
{
protected:
	Nibbles() : Gtk::Application("org.gnome.Nibbles", Gio::Application::Flags::HANDLES_COMMAND_LINE)
	{
		Glib::set_application_name (PROGRAM_NAME);
		
		/* Translators: command-line option description, see 'gnome-nibbles --help' */
		add_main_option_entry (Gio::Application::OptionType::BOOL, DISABLE_FAKES_ARGUMENT, 'd', N_("Disable fake bonuses"));

		/* Translators: command-line option description, see 'gnome-nibbles --help' */
		add_main_option_entry (Gio::Application::OptionType::BOOL, ENABLE_FAKES_ARGUMENT, 'e', N_("Enable fake bonuses"));

		/* Translators: command-line option description, see 'gnome-nibbles --help' */
		add_main_option_entry (Gio::Application::OptionType::INT, LEVEL_ARGUMENT,	'l', N_("Start at given level (1-26)"),
		/* Translators: in the command-line options description, text to indicate the user should specify the start level, see 'gnome-nibbles --help' */
				N_("NUMBER"));

		/* Translators: command-line option description, see 'gnome-nibbles --help' */
		add_main_option_entry (Gio::Application::OptionType::INT, PROGRESS_ARGUMENT,	0, N_("Method of progress through the boards (0 - sequentially, 1 - randomly, 2 - fixed board)"),
		/* Translators: in the command-line options description, text to indicate the user should specify the progres method, see 'gnome-nibbles --help' */
		        N_("NUMBER"));

		/* Translators: command-line option description, see 'gnome-nibbles --help' */
		add_main_option_entry (Gio::Application::OptionType::INT, NIBBLES_ARGUMENT, 'n', N_("Set number of nibbles (4-6)"),
		/* Translators: in the command-line options description, text to indicate the user should specify number of nibbles, see 'gnome-nibbles --help' */
		        N_("NUMBER"));

		/* Translators: command-line option description, see 'gnome-nibbles --help' */
		add_main_option_entry (Gio::Application::OptionType::INT, PLAYERS_ARGUMENT, 'p', N_("Set number of players (1-4)"),
		/* Translators: in the command-line options description, text to indicate the user should specify number of players, see 'gnome-nibbles --help' */
		        N_("NUMBER"));

		/* Translators: command-line option description, see 'gnome-nibbles --help' */
		add_main_option_entry (Gio::Application::OptionType::INT, SPEED_ARGUMENT, 's', N_("Set worms speed (4-1, 4 for slow)"),
		/* Translators: in the command-line options description, text to indicate the user should specify the worms speed, see 'gnome-nibbles --help' */
		        N_("NUMBER"));

		/* Translators: command-line option description, see 'gnome-nibbles --help' */
		add_main_option_entry (Gio::Application::OptionType::BOOL, THREE_DIMENSIONAL_ARGUMENT, '3', N_("Set three dimensional view"));

		/* Translators: command-line option description, see 'gnome-nibbles --help' */
		add_main_option_entry (Gio::Application::OptionType::BOOL, TWO_DIMENSIONAL_ARGUMENT, '2', N_("Set two dimensional view"));

		/* Translators: command-line option description, see 'gnome-nibbles --help' */
		add_main_option_entry (Gio::Application::OptionType::BOOL, START_ARGUMENT, 0, N_("Start playing"));

		/* Translators: command-line option description, see 'gnome-nibbles --help' */
		add_main_option_entry (Gio::Application::OptionType::BOOL, MUTE_ARGUMENT, 0, N_("Turn off the sound"));

		/* Translators: command-line option description, see 'gnome-nibbles --help' */
		add_main_option_entry (Gio::Application::OptionType::BOOL, UNMUTE_ARGUMENT, 0, N_("Turn on the sound"));

		/* Translators: command-line option description, see 'gnome-nibbles --help' */
		add_main_option_entry (Gio::Application::OptionType::BOOL, VERSION_ARGUMENT, 'v', N_("Show release version and exit"));
	}

	
public:
	static Glib::RefPtr<Nibbles> create()
	{
		return Glib::make_refptr_for_instance<Nibbles>(new Nibbles());
	}

private:
    NibblesWindow *pWindow		= nullptr;
    bool start                  = false;
    int level                   = std::numeric_limits<int>::min();
    int speed                   = std::numeric_limits<int>::min();
	bool nibbles_changed		= false;
	bool players_changed		= false;
    Boolean fakes;

protected:
    void on_startup () override 
    {
        Gtk::Application::on_startup ();
        
        Glib::set_prgname ("org.gnome.Nibbles");

		Glib::RefPtr<Gtk::Settings> settings = Gtk::Settings::get_default();
		settings->set_property("gtk-application-prefer-dark-theme", true);

		add_action("quit",  sigc::mem_fun(*this, &Nibbles::quit));

        // F1 and friends are managed manually
        set_accels_for_action ("win.new-game",  {"<Primary>n"});
        set_accels_for_action ("app.fullscreen",{"F11"});
        set_accels_for_action ("win.scores",	{});
        set_accels_for_action ("app.pause",     {"<Primary>p", "Pause"});
        set_accels_for_action ("app.quit",      {"<Primary>q"});
        set_accels_for_action ("win.back",      {"Escape"});
        set_accels_for_action ("win.hamburger", {"F10", "Menu"});
    }

    void on_activate() override
    {
		Start();
    }

	int on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line) override
	{
		Start();
		/* The local instance will eventually exit with this status code: */
		return EXIT_SUCCESS;
	}

	void Start()
	{
		eSetupScreen setup;
		if (start)
		    setup = GAME;
		else if (nibbles_changed && players_changed)
		{
		    if (speed != std::numeric_limits<int>::min() && fakes.is_set())
		        setup = CONTROLS;
		    else
		        setup = SPEED;
		}
		else
		    setup = USUAL;  // first-run or nibbles-number
		
		try
		{
			// create app window
			pWindow = NibblesWindow::create (PROGRAM_NAME, level == std::numeric_limits<int>::min() ? 0 : level, setup);
			add_window (*pWindow);
			pWindow->set_default_icon_name ("org.gnome.Nibbles");
			pWindow->present ();
			add_action("new-game",sigc::mem_fun(*pWindow, &NibblesWindow::new_game_cb));
			add_action("fullscreen",sigc::mem_fun(*pWindow, &NibblesWindow::fullscreen_cb));
			add_action("help",  sigc::mem_fun(*pWindow, &NibblesWindow::help_cb));
			add_action("about", sigc::mem_fun(*pWindow, &NibblesWindow::about_cb));
			add_action("scores",sigc::mem_fun(*pWindow, &NibblesWindow::scores_cb));
			add_action("pause", sigc::mem_fun(*pWindow, &NibblesWindow::pause_cb));
		}
		catch (const Glib::Error& ex)
		{
			std::cerr << "Nibbles::Start() Glib::Error :" << ex.what() << std::endl;
		}
		catch (const std::exception& ex)
		{
			std::cerr << "Nibbles::Start() std::exception :" << ex.what() << std::endl;
		}
	}

	int on_handle_local_options(const Glib::RefPtr<Glib::VariantDict>& options) override
	{
		auto pSettings = Gio::Settings::create("org.gnome.Nibbles");

        if(is_arg(options, VERSION_ARGUMENT))
        {
        	options->remove(VERSION_ARGUMENT);
            /* Not translated so can be easily parsed */
            std::cerr << "gnome-nibbles " << VERSION << std::endl;
            return EXIT_SUCCESS;
        }

		if(get_arg_value(options, LEVEL_ARGUMENT, level))
		{
        	options->remove(LEVEL_ARGUMENT);
			if(level < 1 || level > 26)
			{
		        /* Translators: command-line error message, displayed for an invalid start level request; see 'gnome-nibbles -l 0' */
		        std::cerr << _("Start level must be between 1 and 26 inclusive.") << std::endl;
		        return EXIT_FAILURE;
			}
		}
		
		auto progress_variant = g_variant_dict_lookup_value(options->gobj(), PROGRESS_ARGUMENT, nullptr);	
		if(progress_variant)
		{
        	options->remove(PROGRESS_ARGUMENT);
			auto type = g_variant_get_type(progress_variant);
			if(G_VARIANT_TYPE_STRING == type)
			{
				gsize length;
				auto s = g_variant_get_string(progress_variant, &length);
				if('s' == s[0])
				{
	                pSettings->set_int (PROGRESS_SETTINGS, 0);
				}
				else if('r' == s[0])
				{
	                pSettings->set_int (PROGRESS_SETTINGS, 1);
				}
				else if('f' == s[0])
				{
	                pSettings->set_int (PROGRESS_SETTINGS, 2);
				}
				else
				{
				    /* Translators: command-line error message, displayed for an invalid progress method request; see 'gnome-nibbles --progress' */
				    std::cerr << _("Progress method should be; sequentially, randomly, or fixed board.") << std::endl;
				    return EXIT_FAILURE;
				}
			}
			else if(G_VARIANT_TYPE_BYTE == type)
			{
				auto c = g_variant_get_byte(progress_variant);
				if('s' == c)
				{
	                pSettings->set_int (PROGRESS_SETTINGS, 0);
				}
				else if('r' == c)
				{
	                pSettings->set_int (PROGRESS_SETTINGS, 1);
				}
				else if('f' == c)
				{
	                pSettings->set_int (PROGRESS_SETTINGS, 2);
				}
				else
				{
				    /* Translators: command-line error message, displayed for an invalid progress method request; see 'gnome-nibbles --progress' */
				    std::cerr << _("Progress method should be; s for sequentially, r for randomly or f for fixed board.") << std::endl;
				    return EXIT_FAILURE;
				}
			}
			else if(G_VARIANT_TYPE_INT16 == type || G_VARIANT_TYPE_UINT16 == type ||
					G_VARIANT_TYPE_INT32 == type || G_VARIANT_TYPE_UINT32 == type ||
					G_VARIANT_TYPE_INT64 == type || G_VARIANT_TYPE_UINT64 == type)
			{
				auto i = g_variant_get_int64(progress_variant);
				if(0 == i)
				{
	                pSettings->set_int (PROGRESS_SETTINGS, 0);
				}
				else if(1 == i)
				{
	                pSettings->set_int (PROGRESS_SETTINGS, 1);
				}
				else if(2 == i)
				{
	                pSettings->set_int (PROGRESS_SETTINGS, 2);
				}
				else
				{
				    /* Translators: command-line error message, displayed for an invalid progress method request; see 'gnome-nibbles --progress' */
				    std::cerr << _("Progress method should be; 0 - sequentially, 1 - randomly or 2 - fixed board.") << std::endl;
				    return EXIT_FAILURE;
				}				
			}
			else
			{
			    /* Translators: command-line error message, displayed for an invalid progress method request; see 'gnome-nibbles --progress' */
			    std::cerr << _("Progress method should be; 0 - sequentially, 1 - randomly or 2 - fixed board.") << std::endl;
			    return EXIT_FAILURE;
			}				
		}

		int players,nibbles;
		if(get_arg_value(options, PLAYERS_ARGUMENT, players))
		{
        	options->remove(PLAYERS_ARGUMENT);
        	players_changed = true;
			if(players < 1 || players > 4)
			{
		        /* Translators: command-line error message, displayed for an invalid number of players; see 'gnome-nibbles -p 5' */
		        std::cerr << _("Players must be between 1 and 4 inclusive.") << std::endl;
		        return EXIT_FAILURE;
			}
			else
			{
                pSettings->set_int (PLAYER_SETTINGS, players);
				if(get_arg_value(options, NIBBLES_ARGUMENT, nibbles))
				{
					options->remove(NIBBLES_ARGUMENT);
					nibbles_changed	= true;
					if(nibbles < players || nibbles > 6)
					{
						/* Translators: command-line error message, displayed for an invalid number of nibbles; see 'gnome-nibbles -n 0' */
						std::cerr << _("Nibbles must be between ") << players << _(" and 6 inclusive.") << std::endl;
						return EXIT_FAILURE;
					}
					else
				        pSettings->set_int (AI_SETTINGS, nibbles - players);
				}
            }
		}
		else if(get_arg_value(options, NIBBLES_ARGUMENT, nibbles))
		{
            players = pSettings->get_int (PLAYER_SETTINGS);
        	options->remove(NIBBLES_ARGUMENT);
			nibbles_changed	= true;
			if(nibbles < players || nibbles > 6)
			{
				/* Translators: command-line error message, displayed for an invalid number of nibbles; see 'gnome-nibbles -n 0' */
				std::cerr << _("Nibbles must be between ") << players << _(" and 6 inclusive.") << std::endl;
				return EXIT_FAILURE;
			}
			else
		        pSettings->set_int (AI_SETTINGS, nibbles - players);
		}

		if(get_arg_value(options, SPEED_ARGUMENT, speed))
		{
        	options->remove(SPEED_ARGUMENT);
			if(speed < 1 || speed > 4)
			{
		        /* Translators: command-line error message, displayed for an invalid number of players; see 'gnome-nibbles -s 5' */
		        std::cerr << _("Speed must be between 1(fast) and 4(beginner) inclusive.") << std::endl;
		        return EXIT_FAILURE;
			}
			else
                pSettings->set_int (SPEED_SETTINGS, speed);
		}
			
	    bool disable_fakes	= is_arg(options, DISABLE_FAKES_ARGUMENT);
	    bool enable_fakes	= is_arg(options, ENABLE_FAKES_ARGUMENT);
        if (disable_fakes && enable_fakes)
        {
        	options->remove(DISABLE_FAKES_ARGUMENT);
        	options->remove(ENABLE_FAKES_ARGUMENT);
            /* Translators: command-line error message, displayed for an invalid combination of options; see 'gnome-nibbles -d -e' */
            std::cerr << _("Options --disable-fakes (-d) and --enable-fakes (-e) are mutually exclusive.") << std::endl;
            return EXIT_FAILURE;
        }
        else if (disable_fakes)
        {
        	options->remove(DISABLE_FAKES_ARGUMENT);
	        fakes = false;
            pSettings->set_boolean (FAKE_SETTINGS, fakes);
        }
        else if (enable_fakes)
        {
        	options->remove(ENABLE_FAKES_ARGUMENT);
	        fakes = true;
            pSettings->set_boolean (FAKE_SETTINGS, fakes);
	    }

		bool mute   = is_arg(options, MUTE_ARGUMENT);
		bool unmute = is_arg(options, UNMUTE_ARGUMENT);
        if (mute && unmute)
        {
        	options->remove(MUTE_ARGUMENT);
        	options->remove(UNMUTE_ARGUMENT);
            /* Translators: command-line error message, displayed for an invalid combination of options; see 'gnome-nibbles --mute --unmute' */
            std::cerr << _("Options --mute and --unmute are mutually exclusive.") << std::endl;
            return EXIT_FAILURE;
        }
        else if (mute)
        {
        	options->remove(MUTE_ARGUMENT);
            pSettings->set_boolean (SOUND_SETTINGS, false);
	    }
        else if (unmute)
        {
        	options->remove(UNMUTE_ARGUMENT);
            pSettings->set_boolean (SOUND_SETTINGS, true);
	    }

		bool three_dimensional = is_arg(options, THREE_DIMENSIONAL_ARGUMENT);
		bool two_dimensional   = is_arg(options, TWO_DIMENSIONAL_ARGUMENT);
        if (three_dimensional && two_dimensional)
        {
        	options->remove(THREE_DIMENSIONAL_ARGUMENT);
        	options->remove(TWO_DIMENSIONAL_ARGUMENT);
            /* Translators: command-line error message, displayed for an invalid combination of options; see 'gnome-nibbles -3 -2' */
            std::cerr << _("Options --3D (-3) and --2D (-2) are mutually exclusive.") << std::endl;
            return EXIT_FAILURE;
        }
        else if (three_dimensional)
        {
        	options->remove(THREE_DIMENSIONAL_ARGUMENT);
            pSettings->set_boolean (THREE_DIMENSIONAL_SETTINGS, true);
        }
        else if (two_dimensional)
       	{
        	options->remove(TWO_DIMENSIONAL_ARGUMENT);
            pSettings->set_boolean (THREE_DIMENSIONAL_SETTINGS, false);
        }

        if (is_arg(options, START_ARGUMENT))
        {
        	options->remove(START_ARGUMENT);
            start = true;
        }

        /* Activate */
        return -1;
	}

	static bool is_arg(const Glib::RefPtr<Glib::VariantDict>& options, const Glib::ustring& arg_name)
	{
		auto gvariant = g_variant_dict_lookup_value(options->gobj(), arg_name.c_str(), nullptr);
		return gvariant;
	}

	template <typename T_ArgType>
	static bool get_arg_value(const Glib::RefPtr<Glib::VariantDict>& options, const Glib::ustring& arg_name, T_ArgType& arg_value)
	{
		arg_value = T_ArgType();
		if(options->lookup_value(arg_name, arg_value))
		{
			//std::cout << "The \"" << arg_name << "\" value was in the options VariantDict." << std::endl;
			return true;
		}
		else
		{
			auto gvariant = g_variant_dict_lookup_value(options->gobj(), arg_name.c_str(), nullptr);
			if(!gvariant)
			{
				//std::cerr << "The \"" << arg_name << "\" value was not in the options VariantDict." << std::endl;
			}
			else
			{
				std::cerr <<
					"The \"" << arg_name <<"\" value was of type " << g_variant_get_type_string(gvariant) <<
					" instead of " << Glib::Variant<T_ArgType>::variant_type().get_string() << std::endl;
			}
			return false;
		}
	}

    void on_shutdown () override
    {
        if (nullptr != pWindow)
            pWindow->close ();

        Gtk::Application::on_shutdown ();
        //base.shutdown ();
    }
};

int main(int argc, char* argv[])
{
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    //gtk_init();

	auto application = Nibbles::create();
    return application->run(argc, argv);
}

