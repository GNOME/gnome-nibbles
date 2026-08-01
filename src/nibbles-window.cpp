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
#include <fstream>
#include <cassert>
#include <mutex>
//#include <inplace_vector>
#include <queue>

#include <gtkmm.h>
#include <gsk/gsk.h>
#include <gtkmm/eventcontrollerlegacy.h>

#include <algorithm>/* required for std::ranges::contains */
#include <unordered_set>
#include <bitset>
#include <forward_list>

/* language */
#include <locale>
#include <glib/gi18n.h>

#include "definitions.h"
#include "pseudo_random.h"
#include "map.h"
#include "bonus.h"
#include "position.h"
#include "worm.h"
#include "warp.h"
#include "nibbles-window.h"
#include "game.h"
#include "view.h"
#include "confirm.h"

/*******************************************************************
 *                                                                 *
 *	NibblesWindow                                                  *
 *                                                                 *
 *******************************************************************/
/* text box functions */
void NibblesWindow::draw_text(const Glib::RefPtr<Gtk::Snapshot>&snapshot, const Glib::ustring text, const int width, const bool text_at_top, Gtk::Widget &widget)
{
    //std::cout << text << std::endl;
    const float PIby2 = 1.570796326794896619231321691639751442f; /* Pi / 2 */
    const int border = 20;
    double text_width;
    double text_height;
    int font_size = calculate_font_size (text, width - border, text_width, text_height, widget);
    float background_width = (float)text_width + border;
    float background_height = (float)text_height + border;
    float x = (widget.get_width () - background_width) / 2;
    float y = text_at_top ? 0 : (widget.get_height () - background_height) / 2;
    float arc_radius = background_width < background_height ? background_width / 3 : background_height / 3;
    /* draw background */
	auto background=Gsk::PathBuilder::create();
    /* top right corner */
    background->move_to (x + background_width - arc_radius, y + 0);
    background->svg_arc_to (arc_radius, arc_radius, PIby2, false, true, x + background_width, y + arc_radius);
    /* bottom right corner */
    background->line_to (x + background_width, y + background_height - arc_radius);
    background->svg_arc_to (arc_radius, arc_radius, PIby2, false, true, x + background_width - arc_radius, y + background_height);
    /* bottom left corner */
    background->line_to (x + arc_radius, y + background_height);
    background->svg_arc_to (arc_radius, arc_radius, PIby2, false, true, x + 0, y + background_height - arc_radius);
    /* top left corner */
    background->line_to (x + 0, y + arc_radius);
    background->svg_arc_to (arc_radius, arc_radius, PIby2, false, true, x + arc_radius, y + 0);
    /* fill with colour */
	snapshot->append_fill (background->to_path (), Gsk::FillRule::EVEN_ODD, {0.0f, 0.0f, 0.0f, 0.9f});
    /* draw the text */
    draw_text_font_size (snapshot, (int)(x + (background_width - text_width) / 2),
        (int)(y + (background_height - text_height) / 2), text, font_size, widget);
}
/* calculate the font size that fits in the space */
int NibblesWindow::calculate_font_size (const Glib::ustring &text, int target_width, double &width, double &height, Gtk::Widget &widget)
{
    bool rush_size_steps = true;
    int fail_count = 0;
    int last_font_size = 1;
    int target_font_size = 1;
    width = 0;
    height = 0;
    uint target_width_diff = std::numeric_limits<unsigned int>::max();

    for (int font_size = 1;font_size < 128;)
    {
        Glib::RefPtr<Pango::Layout> layout = widget.create_pango_layout (text);
        Pango::FontDescription font = layout->get_font_description ();
        if(!font.gobj())
        	font=Glib::wrap(pango_font_description_from_string("Sans Bold 1pt"));
        font.set_size (Pango::SCALE * font_size);
        layout->set_font_description (font);
        //layout->set_text (text, -1);
        Pango::Rectangle a,b;
        layout->get_extents (a, b);
        int width_diff = target_width - (int)(a.get_width() / Pango::SCALE);
        if (width_diff > 0 && width_diff < target_width_diff)
        {
            target_width_diff = width_diff;
            target_font_size = font_size;
            width = a.get_width() / Pango::SCALE;
            height = a.get_height() / Pango::SCALE;
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
void NibblesWindow::draw_text_font_size (const Glib::RefPtr<Gtk::Snapshot>&snapshot, int x, int y, const Glib::ustring &text, int font_size, Gtk::Widget &widget)
{
    int x_offset, y_offset;
    get_text_offsets (text, font_size, x_offset, y_offset, widget);
    snapshot->translate ({x - x_offset, y - y_offset});
    auto layout = widget.create_pango_layout (text);
    layout->set_alignment (Pango::Alignment::CENTER);
    auto font=layout->get_font_description ();
    if(!font.gobj())
    	font=Glib::wrap(pango_font_description_from_string("Sans Bold 1pt"));
    font.set_size (Pango::SCALE * font_size);
    layout->set_font_description (font);
    //layout->set_text (text, -1);
    snapshot->append_layout (layout, {1, 1, 1, 1});
}
void NibblesWindow::get_text_offsets (const Glib::ustring &text, int font_size, int &x_offset, int &y_offset, Gtk::Widget &widget)
{
    auto layout = widget.create_pango_layout (text);
    auto font=layout->get_font_description ();
    if(!font.gobj())
    	font=Glib::wrap(pango_font_description_from_string("Sans Bold 1pt"));
    font.set_size (Pango::SCALE * font_size);
    layout->set_font_description (font);
    //layout.set_text (text, -1);
    Pango::Rectangle a,b;
    layout->get_extents(a,b);
    x_offset = a.get_x() / Pango::SCALE;
    y_offset = a.get_y() / Pango::SCALE;
}
/* get the worm's colour from the settings, unknown_colour_worm if no colour set */
eWormColour get_worm_settings_colour(unsigned long worm_id)
{
	if(worm_id>=0 && worm_id<6)
	{
		Glib::ustring my_settings = WORM_BASE_KEY;
		my_settings += (char)('0'+worm_id);
		auto pWormSettings = Gio::Settings::create(my_settings);
		const Glib::ustring string_colour=pWormSettings->get_string("color");
		if(string_colour=="red")
			return red_worm;
		else if(string_colour=="green")
			return green_worm;
		else if(string_colour=="blue")
			return blue_worm;
		else if(string_colour=="yellow")
			return yellow_worm;
		else if(string_colour=="cyan")
			return cyan_worm;
		else if(string_colour=="purple")
			return purple_worm;
		else
			return unknown_colour_worm;
	}
	else
		return unknown_colour_worm;
}
/* write the worm's colour to settings */
void set_worm_settings_colour(unsigned long worm_id,eWormColour colour)
{
	if(worm_id>=0 && worm_id<6 && colour>=0 && colour<6)
	{
		Glib::ustring my_settings = WORM_BASE_KEY;
		my_settings += (char)('0'+worm_id);
		auto pWormSettings = Gio::Settings::create(my_settings);
		pWormSettings->set_string("color",worm_colour_string[colour]);
	}
}

void NibblesWindow::setup_controls()
{
	/* set up each player */
	auto players_grid = GetBox("grids_box");
	auto child = players_grid->get_first_child();
	for(uint p=0;p<player_count_selection;p++)
	{
		if(child)
		{
			child = child->get_next_sibling ();
		}
		else
		{
			/* create new players as needed */
			auto Builder0 = Gtk::Builder::create_from_resource("/org/gnome/Nibbles/ui/player-controls.ui", "player_controls");
			auto pc0 = Builder0->get_widget<Gtk::Box>("player_controls");
			players_grid->append(*pc0);
			Glib::ustring button_name = "name_label";
			auto button = Gtk::Builder::get_widget_derived<PlayerButton>(Builder0, "name_label", p, this);
		}
	}
	/* clean up unused players */
	for(;child;)
	{
		auto r = child;
		child = child->get_next_sibling ();
		players_grid->remove(*r);
	}
	/* only allow the game to start if there are no key clashes */
	check_and_enable_start_button();
}

void NibblesWindow::setup_game()
{
	auto game_box=GetBox("game_box");
	auto child = game_box->get_first_child();
	auto *view = dynamic_cast<View*>(child);
	if(!view || child->get_buildable_id()=="statusbar_stack")
	{
		/* create the view */
		view=Gtk::make_managed<View>(static_cast<Game::Progress>(progress_selection),
		std::clamp(pSettings->get_int(LEVEL_SETTINGS), 1 , 26), speed_selection,
		*GetButton("pause_button"),
			[this](const std::vector<WormScore> scores) {/*game_over*/
				/* disable new-game & pause/resume buttons */
				GetButton("new_game_button")->set_visible(0);
				GetButton("pause_button")->set_visible(0);
				update_high_scores(scores);
			}		
		);
	}
	/* pass keys to view */
	std::vector<PlayerButton*> players;
	auto players_count=get_players(players);
	for(uint u=0;u<players_count;u++)
		view->set_keys(get_worm_settings_colour(u),players[u]->get_raw_keys());
	/* enable new-game & pause/resume buttons */
	GetButton("new_game_button")->set_visible(1);
	GetButton("pause_button")->set_visible(1);
	/* Initialise and start the game */
	game_box->prepend(*view);
}

bool NibblesWindow::pass_key_to_view(guint keycode)
{
	auto game_box=GetBox("game_box");
	auto child = game_box->get_first_child();
	if(auto view = dynamic_cast<View*>(child))
		return view->key_press(keycode);
	else
		return false;
}

void NibblesWindow::pause_view(bool pause)
{
	auto game_box=GetBox("game_box");
	auto child = game_box->get_first_child();
	if(auto view = dynamic_cast<View*>(child))
		view->set_pause(pause);
}

bool NibblesWindow::on_key_pressed_callback(guint keyval, guint keycode, Gdk::ModifierType state)
{
	/* check for single key handler */
    if(key_handler)
    {
    	bool keep_using_key_handler = key_handler->key_pressed(keyval, keycode);/* pass key to handler */
    	if(!keep_using_key_handler)
        	key_handler=nullptr;/* stop using handler */
    	return true;
    }
    else
    	return pass_key_to_view(keycode);
}

void NibblesWindow::initilise_css()
{
	/* overview is used by the view to display messages over the view */
	auto provider0 = Gtk::CssProvider::create();
	provider0->load_from_data(R"(
	.overview {
		background-color: #404040;
		color: #c0c0c0;
		font-size: 18pt;
		font-weight: bold;
		padding: 8px 12px;
		border: 2px solid #808080;
		border-radius: 4px;}
	)");
	Gtk::StyleContext::add_provider_for_display(
		Gdk::Display::get_default(),
		provider0,
		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	/* rounded is used by the buttons */
	auto provider1 = Gtk::CssProvider::create();
	provider1->load_from_data(R"(
	.rounded {
		background: #3584e4;
		transition: background-color 300ms ease;
		color: white;
		font-weight: bold;
		border: 4px solid #0060e0;
		border-radius: 9999px;
		padding: 12px 24px;}
	.rounded:hover {
	    background: #4990e7;}
	.rounded:active {
		background: #2a6ab7;}
	)");
	Gtk::StyleContext::add_provider_for_display(
		Gdk::Display::get_default(),
		provider1,
		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	/* message for a text box */
	auto provider2 = Gtk::CssProvider::create();
	provider2->load_from_data(R"(
	.message {
		color: white;
		font-size: 12pt;
		font-weight: bold;
		padding: 8px 12px;}
	)");
	Gtk::StyleContext::add_provider_for_display(
		Gdk::Display::get_default(),
		provider2,
		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}


void NibblesWindow::initilise_about()
{
    std::vector<Glib::ustring> authors={
	    /* Translators: text crediting an author, in the about dialog */
	        _("Sean MacIsaac"),
	    /* Translators: text crediting an author, in the about dialog */
	        _("Ian Peters"),
	    /* Translators: text crediting an author, in the about dialog */
	        _("Andreas Røsdal"),
	    /* Translators: text crediting an author, in the about dialog */
	        _("Guillaume Beland"),
	    /* Translators: text crediting an author, in the about dialog */
	        _("Iulian-Gabriel Radu"),
	    /* Translators: text crediting an author, in the about dialog */
	        _("Ben Corby")
	    };

    /* Translators: text crediting a documenter, in the about dialog */
    std::vector<Glib::ustring> documenters={ _("Kevin Breit") };

    /* Translators: text crediting a designer, in the about dialog */
	std::vector<Glib::ustring> artists={ _("Allan Day") };

	Glib::ustring copyright;
    /* Translators: text crediting some maintainers, seen in the About dialog */
    copyright+=_("Copyright © 1999-2008 – Sean MacIsaac, Ian Peters, Andreas Røsdal\n");
     /* Translators: text crediting a maintainer, seen in the About dialog */
    copyright+=_("Copyright © 2009 – Guillaume Beland\n");
     /* Translators: text crediting a maintainer, seen in the About dialog */
    copyright+=_("Copyright © 2015-2020 – Iulian-Gabriel Radu\n");
     /* Translators: text crediting a maintainer, seen in the About dialog */
    copyright+=_("Copyright © 2022-2026 – Ben Corby");

	about.set_logo_icon_name("org.gnome.Nibbles");
	about.set_program_name(PROGRAM_NAME);
	//about.set_developer_name (_("The GNOME Project"));
	about.set_version(VERSION);
	about.set_authors(authors);
	about.set_documenters(documenters);
    about.set_artists(artists);
    about.set_copyright(copyright);
	about.set_license_type(Gtk::License::GPL_3_0);
	/* Translators: about dialog text; this string should be replaced by a text crediting yourselves and your translation team, or should be left empty. Do not translate literally! */
	about.set_translator_credits (_("translator-credits"));
    about.set_website (WEBSITE);
	 /* Translators: small description of the game, seen in the About dialog */
	about.set_comments(_("A worm game for GNOME"));

	about.set_hide_on_close(true);
	about.set_transient_for(*this);   // parent window
	about.set_modal(true);
}
void NibblesWindow::initilise_players()
{
	/* call backs from the "Number of players" configuration screen */
	player_count_selection = std::clamp(pSettings->get_int(PLAYER_SETTINGS), 1 , 4); /* initial selection */
	pPlayerButtons = add_action_radio_integer("change-players-number", sigc::mem_fun(*this, &NibblesWindow::change_players_number_callback ),
		player_count_selection);
	change_players_number_callback (player_count_selection); /* make sure the correct AI radio buttons are displayed */
	unsigned int ai_count_selection = std::clamp(pSettings->get_int(AI_SETTINGS), 0, 5); /* initial selection */
	pAiButtons = add_action_radio_integer("change-nibbles-number", sigc::mem_fun(*this, &NibblesWindow::change_nibbles_number_callback ),
		find_ai_button(player_count_selection, ai_count_selection));

	/* set the players button font */
	Glib::ustring label;
	for (unsigned int player = 1; player <= 4; player++)
	{
		label = "<b><span size=\"30.0pt\" font-family=\"Sans\">";
		label += (char)('0' + player);
		label += "</span></b>";
		static_cast<Gtk::Label*>(GetToggleButton("players", player)->get_child ())->set_markup(label);
	}
}
void NibblesWindow::initilise_progress()
{
	/* call backs from the "board_progress" configuration screen */
	progress_selection = std::clamp(pSettings->get_int(PROGRESS_SETTINGS), 0 , 2); /* initial selection */
	pProgressButtons = add_action_radio_integer("change-progress", sigc::mem_fun(*this, &NibblesWindow::change_progress_callback ),
		progress_selection);
	enable_disable_spin(progress_selection);
	
	/* set the progress' buttons font */
	for (unsigned int progress = 0; progress <= 2; progress++)
	{
		set_14_point(static_cast<Gtk::Label*>(GetToggleButton("progress", progress)->get_child()));
	} 
}
void NibblesWindow::initilise_speed_and_fakes()
{
	/* call backs from the speed & fakes configuration screen */
	speed_selection = std::clamp(pSettings->get_int(SPEED_SETTINGS), 1 , 4); /* initial selection */
	pSpeedButtons = add_action_radio_integer("change-speed", sigc::mem_fun(*this, &NibblesWindow::change_speed_callback ),
		speed_selection);
	fake_selection = pSettings->get_boolean(FAKE_SETTINGS); /* initial selection */
	pFakeButton = add_action_bool("toggle-fakes", sigc::mem_fun(*this, &NibblesWindow::change_fake_callback), fake_selection);

	/* set the speed buttons' font */
	for (unsigned int speed = 1; speed <= 4; speed++)
	{
		set_14_point(static_cast<Gtk::Label*>(GetToggleButton("speed", speed)->get_child()));
	} 
	set_14_point(static_cast<Gtk::Label*>(GetToggleButton("enable_fake_bonuses")->get_child()));
}
void NibblesWindow::initilise_keys()
{
    auto key_controller = Gtk::EventControllerKey::create();
    key_controller->signal_key_pressed().connect(sigc::mem_fun(*this, &NibblesWindow::on_key_pressed_callback), false);
    static_cast<Gtk::Widget*>(this)->add_controller(key_controller);		
}
void NibblesWindow::back_callback() /* escape key */
{
	if(full_screen)
		fullscreen_cb();
	else
	{
		auto s=pScreenStack->get_visible_child_name ();
		eSetupScreen e=name_to_screen(s);
		if(e==GAME)
		{
			auto game_box=GetBox("game_box");
			if(game_box)
			{
				auto child = game_box->get_first_child();
				auto *view = dynamic_cast<View*>(child);
				if(view)
				{
					pause(true); /* pause the game */

					// Translators: text displayed in a message box confirming game exit
					auto* confirm = new ConfirmWindow(*this, "Are you sure you want end this game?");
					
					// Handle the response callback asynchronously (non-blocking)
					confirm->signal_response().connect([this, confirm, game_box, view](bool confirmed) {
						if (confirmed)
						{
							game_box->remove(*view);
							ScreenStack_set_visible_child(PLAYERS);
						}
						else
							pause(false); /* continue the game */
						delete confirm;
					});
					confirm->present();
				}
			}
		}
		else
		{
			ScreenSave(e);
			switch(e)
			{
				default:
					break;
				case PROGRESS:
					ScreenStack_set_visible_child(PLAYERS, Gtk::StackTransitionType::SLIDE_DOWN);			
					break;
				case SPEED:
					ScreenStack_set_visible_child(PROGRESS, Gtk::StackTransitionType::SLIDE_DOWN);			
					break;
				case CONTROLS:
					ScreenStack_set_visible_child(SPEED, Gtk::StackTransitionType::SLIDE_DOWN);
					break;
			}
		}
	}
}

void NibblesWindow::update_high_scores(std::vector<WormScore> scores)
{
	
	
}

/*******************************************************************
 *                                                                 *
 *	KeyHandler                                                     *
 *                                                                 *
 *******************************************************************/
void KeyHandler::get_key()
{
	pWindow->set_key_handler(this);
}

/*******************************************************************
 *                                                                 *
 *	NibblesWindow::Arrow                                           *
 *                                                                 *
 *******************************************************************/
void NibblesWindow::Arrow::snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>& s)
{
	Gtk::Widget::snapshot_vfunc(s);

    //auto path = Gsk::PathBuilder::create ();
    auto b = gsk_path_builder_new();
    double width = get_width ();
    double height = get_height ();
    struct {double x; double y;} a[7];
    if(get_direction()=="up")
    {
    	a[0] ={0, height / 2};
    	a[1] ={width / 2, 0};
    	a[2] ={width, height /2};
    	a[3] ={width * 2 / 3, height / 2};
    	a[4] ={width * 2 / 3, height};
    	a[5] ={width / 3, height};
    	a[6] ={width / 3, height / 2};
    }
    else if(get_direction()=="down")
    {
		a[0] ={0, height / 2};
		a[1] ={width / 2, height};
		a[2] ={width, height /2};
		a[3] ={width * 2 / 3, height / 2};
		a[4] ={width * 2 / 3, 0};
		a[5] ={width / 3, 0};
		a[6] ={width / 3, height / 2};
	}
    else if(get_direction()=="left")
    {
		a[0] ={width / 2, 0};
		a[1] ={0, height / 2};
		a[2] ={width/2, height};
		a[3] ={width / 2, height * 2 / 3};
		a[4] ={width, height * 2 / 3};
		a[5] ={width, height / 3};
		a[6] ={width / 2, height / 3};
	}
    else if(get_direction()=="right")
    {
		a[0] ={width / 2, 0};
		a[1] ={width, height / 2};
		a[2] ={width / 2, height};
		a[3] ={width / 2, height * 2 / 3};
		a[4] ={0, height  * 2 / 3};
		a[5] ={0, height / 3};
		a[6] ={width / 2, height / 3};
	}
	
    for (int i = 0; i < 7; i++)
    {
        if (i == 0)
        {
            //path->move_to ((float)a[0].x, (float)a[0].y);
            gsk_path_builder_move_to(b, (float)a[0].x, (float)a[0].y);
        }
        else
        {
            //path->line_to ((float)a[i].x, (float)a[i].y);
            gsk_path_builder_line_to(b, (float)a[i].x, (float)a[i].y);
        }
    }
    GdkRGBA c = {0.2890625f, 0.5625f, 0.84765625f, 1.0f};
    //if (check_duplicate != null && check_duplicate (direction))
    //    c = {0.75f, 0f, 0f, 1f};
    //else
    //    c = {0.2890625f, 0.5625f, 0.84765625f, 1f};
    if(player->check_for_key_clash(get_direction()))
    	c = {0.75f, 0.0f, 0.0f, 1.0f};
    gsk_path_builder_close(b);
    auto path = gsk_path_builder_free_to_path(b);
    gtk_snapshot_append_fill (s->gobj(), path, GSK_FILL_RULE_EVEN_ODD, &c);
	gsk_path_unref(path);
    
        // Get widget allocation (size)
        //const Gtk::Allocation& allocation = get_allocation();
/*        const int width = get_width();
        const int height = get_height();

        // Draw a simple rectangle
        Gdk::RGBA color;
        color.set_rgba(0.2890625f, 0.5625f, 0.84765625f, 1.0f); // light blue
        s->append_color(color, Gdk::Rectangle{0, 0, width, height});

        // Draw a smaller red rectangle inside
        Gdk::RGBA red;
        red.set_rgba(0.75, 0.0, 0.0, 1.0);
        s->append_color(red, Gdk::Rectangle{width/4, height/4, width/2, height/2});    
*/
}

/*******************************************************************
 *                                                                 *
 *	NibblesWindow::PlayerButton                                    *
 *                                                                 *
 *******************************************************************/
void NibblesWindow::PlayerButton::on_clicked_up(Gtk::Widget *pWidget)
{
	if(!key_pressed_data.pKeyPressMessage)
	{
		// Translators: text displayed in a message box directing the player to press the key they want to use for up
		key_pressed_data.pKeyPressMessage = Gtk::make_managed<OverlayMessage>(_("Press a key for up."), pOverlay->get_width ());
		key_pressed_data.KeyToSet=0;
		key_pressed_data.pLabel=pWidget;
		pOverlay->add_overlay(*key_pressed_data.pKeyPressMessage);
		get_key(); /* get a single key press */
	}
}
void NibblesWindow::PlayerButton::on_clicked_left(Gtk::Widget *pWidget)
{
	if(!key_pressed_data.pKeyPressMessage)
	{
		// Translators: text displayed in a message box directing the player to press the key they want to use for left
		key_pressed_data.pKeyPressMessage = Gtk::make_managed<OverlayMessage>(_("Press a key for left."), pOverlay->get_width ());
		key_pressed_data.KeyToSet=1;
		key_pressed_data.pLabel=pWidget;
		pOverlay->add_overlay(*key_pressed_data.pKeyPressMessage);
		get_key(); /* get a single key press */
	}
}
void NibblesWindow::PlayerButton::on_clicked_right(Gtk::Widget *pWidget)
{
	if(!key_pressed_data.pKeyPressMessage)
	{
		// Translators: text displayed in a message box directing the player to press the key they want to use for right
		key_pressed_data.pKeyPressMessage = Gtk::make_managed<OverlayMessage>(_("Press a key for right."), pOverlay->get_width ());
		key_pressed_data.KeyToSet=2;
		key_pressed_data.pLabel=pWidget;
		pOverlay->add_overlay(*key_pressed_data.pKeyPressMessage);
		get_key(); /* get a single key press */
	}
}
void NibblesWindow::PlayerButton::on_clicked_down(Gtk::Widget *pWidget)
{
	if(!key_pressed_data.pKeyPressMessage)
	{
		// Translators: text displayed in a message box directing the player to press the key they want to use for down
		key_pressed_data.pKeyPressMessage = Gtk::make_managed<OverlayMessage>(_("Press a key for down."), pOverlay->get_width ());
		key_pressed_data.KeyToSet=3;
		key_pressed_data.pLabel=pWidget;
		pOverlay->add_overlay(*key_pressed_data.pKeyPressMessage);
		get_key(); /* get a single key press */
	}
}
bool NibblesWindow::PlayerButton::key_pressed(guint keyval, guint keycode)
{
    if(key_pressed_data.pKeyPressMessage)
    {
    	const unsigned long key_index=key_pressed_data.KeyToSet & 0x3; /* 0=up, 1=left, 2=right & 3=down */
    	/* store key values */
	    keys[key_index]=keyval;
	    raw_keys[key_index]=keycode;
	    /* remove overlay */
		pOverlay->remove_overlay(*key_pressed_data.pKeyPressMessage);
		key_pressed_data.pKeyPressMessage=nullptr;
		/* write settings */
		Glib::ustring settings = WORM_BASE_KEY;
		settings += (char)('0'+id);
		auto pWormSettings = Gio::Settings::create(settings);
		Glib::ustring key,raw_key;
		key="key-";
		const char *dir[]={"up","left","right","down"};
		key+=dir[key_index];
		raw_key=key+"-raw";
		pWormSettings->set_int(key, keys[key_index]);
		pWormSettings->set_int(raw_key, raw_keys[key_index]);
		/* update display text */
		Glib::ustring text = gtk_accelerator_get_label (keys[key_index], GDK_NO_MODIFIER_MASK);
		static_cast<Gtk::Label*>(key_pressed_data.pLabel)->set_text(text);
		/* redraw each player's arrows in case they have change colour */
		std::vector<PlayerButton*> players;
		auto player_count=get_players(players);
		for(uint u=0;u<player_count;u++)
			players[u]->redraw_arrows();
		/* allow the game to start if there are no key clashes */
		pWindow->check_and_enable_start_button();
	}
	return false; /* remove key handler */
}
bool NibblesWindow::PlayerButton::check_for_key_clash(const Glib::ustring &self)
{
	unsigned int raw_key;
	if(self=="up")
	{
		raw_key=raw_keys[0];
		if(raw_key==raw_keys[1] || raw_key==raw_keys[2] || raw_key==raw_keys[3])
			return true;
	}
	else if(self=="left")
	{
		raw_key=raw_keys[1];
		if(raw_key==raw_keys[0] || raw_key==raw_keys[2] || raw_key==raw_keys[3])
			return true;
	}
	else if(self=="right")
	{
		raw_key=raw_keys[2];
		if(raw_key==raw_keys[0] || raw_key==raw_keys[1] || raw_key==raw_keys[3])
			return true;
	}
	else if(self=="down")
	{
		raw_key=raw_keys[3];
		if(raw_key==raw_keys[0] || raw_key==raw_keys[1] || raw_key==raw_keys[2])
			return true;
	}
	else
	{
		Glib::ustring error="player-controls.ui: Invalid arrow direction \"";
		error+=self;
		error+="\".";
		g_critical(error.c_str());
	}

	std::vector<PlayerButton*> players;
	auto player_count=get_players(players);
	for(uint u=0;u<player_count;u++)
	{
		if(players[u]->id != id)
		{
			if(std::ranges::contains(players[u]->raw_keys,raw_key))
				return true;
		}
	}
	return false;
}

bool NibblesWindow::PlayerButton::check_for_key_clash()
{
	std::vector<PlayerButton*> players;
	auto player_count=get_players(players);
	for(uint u=0;u<player_count;u++)
	{
		if(players[u]->id != id)
		{
			for(uint i=0;i<raw_keys.size();i++)
				if(std::ranges::contains(players[u]->raw_keys,raw_keys[i]))
					return true;
		}
		else
		{
			if(raw_keys[0]==raw_keys[1] || raw_keys[0]==raw_keys[2] || raw_keys[0]==raw_keys[3] ||
				raw_keys[1]==raw_keys[2] || raw_keys[1]==raw_keys[3] || raw_keys[2]==raw_keys[3])
				return true;
		}
	}
	return false;
}


void NibblesWindow::PlayerButton::get_key_settings(Glib::RefPtr<Gio::Settings> pWormSettings,
	unsigned int &key, unsigned int &raw_key, const char *key_string)
{
	Glib::ustring k="key-";
	k+=key_string;
	Glib::ustring r=k;
	r+="-raw";
	key=pWormSettings->get_int(k);
	int i=pWormSettings->get_int(r);
	if(i<0)
	{
		GdkKeymapKey *_keys = NULL;
		int n_keys=0;
		/* todo add map_keyval to gtkmm */
		gdk_display_map_keyval(get_display()->gobj(),key,&_keys,&n_keys);
		if(n_keys>0)
		{
			raw_key=_keys[0].keycode;
			pWormSettings->set_int(r,raw_key);
		}
	}
	else
		raw_key=(unsigned int)i;
}

void NibblesWindow::PlayerButton::set_key_buttons(const Glib::RefPtr<Gtk::Builder>& refBuilder, unsigned int id)
{
	Glib::ustring my_settings = WORM_BASE_KEY;
	my_settings += (char)('0'+id);
	auto pWormSettings = Gio::Settings::create(my_settings);

	auto up = refBuilder->get_widget<Gtk::Button>("move_up_button");
	get_key_settings(pWormSettings,keys[0],raw_keys[0],"up");
	static_cast<Gtk::Label*>(up->get_child())->set_text(gtk_accelerator_get_label (keys[0], GDK_NO_MODIFIER_MASK));
	up->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &PlayerButton::on_clicked_up), up->get_child()));
	pArrow_up->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &PlayerButton::on_clicked_up), up->get_child()));

	auto left = refBuilder->get_widget<Gtk::Button>("move_left_button");
	get_key_settings(pWormSettings,keys[1],raw_keys[1],"left");
	static_cast<Gtk::Label*>(left->get_child())->set_text(gtk_accelerator_get_label (keys[1], GDK_NO_MODIFIER_MASK));
	left->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &PlayerButton::on_clicked_left), left->get_child()));
	pArrow_left->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &PlayerButton::on_clicked_left), left->get_child()));

	auto right = refBuilder->get_widget<Gtk::Button>("move_right_button");
	get_key_settings(pWormSettings,keys[2],raw_keys[2],"right");
	static_cast<Gtk::Label*>(right->get_child())->set_text(gtk_accelerator_get_label (keys[2], GDK_NO_MODIFIER_MASK));
	right->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &PlayerButton::on_clicked_right), right->get_child()));
	pArrow_right->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &PlayerButton::on_clicked_right), right->get_child()));

	auto down = refBuilder->get_widget<Gtk::Button>("move_down_button");
	get_key_settings(pWormSettings,keys[3],raw_keys[3],"down");
	static_cast<Gtk::Label*>(down->get_child())->set_text(gtk_accelerator_get_label (keys[3], GDK_NO_MODIFIER_MASK));
	down->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &PlayerButton::on_clicked_down), down->get_child()));
	pArrow_down->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &PlayerButton::on_clicked_down), down->get_child()));
}

/*******************************************************************
 *                                                                 *
 *	NibblesWindow::ColourWheel                                     *
 *                                                                 *
 *******************************************************************/

void NibblesWindow::ColourWheel::snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>&snapshot)
{
	Gtk::Box::snapshot_vfunc(snapshot);

	draw_text(snapshot,
	// Translators: text displayed in a message box directing the player to select the color they want for the worm
		_("Select Your Color"),
		get_width (), true, *this);
}

/* return the segment the mouse pointer is over, if any */
NibblesWindow::ColourWheelSegment *NibblesWindow::ColourWheel::get_mouse_point_segment()
{
    if(mouse_point.is_valid)
    {
    	auto segment_count=get_segment_count();
		unsigned int id=0;
		for(Gtk::Widget *p = get_first_child ();p!=nullptr;p = p->get_next_sibling (), id++)
		{
			auto segment=dynamic_cast<ColourWheelSegment *>(p);
			if(!segment)
			{
            	Glib::ustring buffer="player-controls.ui: all children of id=\"";
            	buffer+=get_buildable_id();
            	buffer+="\" must be of gtkmm__CustomObject_ColourWheelSegment class.";
            	g_critical(buffer.c_str());
            }
			else if(segment->calculate_segment_path(get_width (), get_height(), id, segment_count)->
				in_fill({mouse_point.x,mouse_point.y},Gsk::FillRule::EVEN_ODD))
                return segment;
		}
    }
    return nullptr;
}

/* override the focus virtual function to enable forward/backward tabs
   and cursor keys */
bool NibblesWindow::ColourWheel::focus_vfunc(Gtk::DirectionType direction, Gtk::Widget *&set_focus_child)
{
	set_focus_child=nullptr;
	const unsigned int segment_count=get_segment_count();
	const unsigned int segment_degrees=360/segment_count;
	auto focus_child=get_focus_child();
	int focus_id=focus_child?get_segment_id(focus_child):-1;
	switch (direction)
    {
        case Gtk::DirectionType::TAB_FORWARD:
            if (focus_id < 0)
            {
                /* no focus, focus on first segment */
                auto first_segment=get_first_child();
                set_focus_child=first_segment;
                first_segment->queue_draw();/* focus */
                return true;
            }
            else if (focus_id < segment_count - 1)
            {
                /* move focus to next segment */
            	auto next_segment=focus_child->get_next_sibling ();
                set_focus_child=next_segment;
                focus_child->queue_draw();/* remove focus */
                next_segment->queue_draw();/* focus */
                return true;
            }
            else
            {
                /* last segment reached */
                Gtk::Widget *p,*last_segment;
				for(p = get_first_child ();p!=nullptr;last_segment = p, p = p->get_next_sibling ());
               	last_segment->queue_draw();/* remove focus */
                return false;
            }
        case Gtk::DirectionType::TAB_BACKWARD:
            if (focus_id < 0)
            {
				/* no focus, focus on last segment */
				Gtk::Widget *p,*last_segment;
				for(p = get_first_child ();p!=nullptr;last_segment = p, p = p->get_next_sibling ());
				set_focus_child=last_segment;
				last_segment->queue_draw();/* focus */
				return true;
            }
            else if (focus_id > 0)
            {
                Gtk::Widget *p,*previous_segment;
				for(p = get_first_child ();p!=nullptr && p!=focus_child;previous_segment = p, p = p->get_next_sibling ());
                set_focus_child=previous_segment;
                focus_child->queue_draw();/* remove focus */
                previous_segment->queue_draw();/* focus */
                return true;
            }
            else
            {
                /* first segment reached */
                auto first_segment=get_first_child();
                first_segment->queue_draw();/* remove focus */
                return false;
            }
        case Gtk::DirectionType::UP:
            if (focus_id < 0)
            {
                /* no focus */
                set_focus_child=get_child(180/segment_degrees);
                set_focus_child->queue_draw();/* focus */
                return true;
            }
            else if (focus_id == 0 || focus_id == segment_count - 1)
            {
                /* top reached */
                focus_child->queue_draw();/* remove focus */
                return false;
            }
            else
            {
                if (focus_id < segment_count/2)
                    set_focus_child=get_child(focus_id-1);
                else
                    set_focus_child=get_child(focus_id+1);
                set_focus_child->queue_draw();/* focus */
                focus_child->queue_draw();/* remove focus */
                return true;
            }
        case Gtk::DirectionType::DOWN:
            if (focus_id < 0)
            {
                /* no focus */
                set_focus_child=get_first_child();
                set_focus_child->queue_draw();/* focus */
                return true;
            }
            else if ((segment_count & 0x1) == 0 && (focus_id == segment_count / 2 || focus_id == segment_count / 2 - 1)
                    || (segment_count & 0x1) == 1 && focus_id == segment_count / 2)
            {
                /* bottom reached */
                focus_child->queue_draw();/* remove focus */
                return false;
            }
            else
            {
                if (focus_id < segment_count / 2)
                    set_focus_child=get_child(focus_id+1);
                else
                    set_focus_child=get_child(focus_id-1);
                set_focus_child->queue_draw();/* focus */
                focus_child->queue_draw();/* remove focus */
                return true;
            }
        case Gtk::DirectionType::LEFT:
            if (focus_id < 0)
            {
                /* no focus */
                set_focus_child=get_child(90/segment_degrees);
                set_focus_child->queue_draw();/* focus */
                return true;
            }
            else if (focus_id == 270 / segment_degrees || 270 % segment_degrees == 0 && focus_id == 270 / segment_degrees - 1)
            {
                /* left most reached */
                focus_child->queue_draw();/* remove focus */
                return false;
            }
            else
            {
                if (focus_id < 270 / segment_degrees && focus_id >= 90 / segment_degrees)
                    set_focus_child=get_child(focus_id + 1);
                else
                    set_focus_child=get_child(focus_id > 0 ? focus_id - 1 : segment_count - 1);
                set_focus_child->queue_draw();/* focus */
                focus_child->queue_draw();/* remove focus */
                return true;
            }
        case Gtk::DirectionType::RIGHT:
            if (focus_id < 0)
            {
                /* no focus */
                set_focus_child=get_child(270 / segment_degrees);
                set_focus_child->queue_draw();/* focus */
                return true;
            }
            else if (focus_id == 90 / segment_degrees || 90 % segment_degrees == 0 && focus_id == 90 / segment_degrees - 1)
            {
                /* right most reached */
                focus_child->queue_draw();/* remove focus */
                return false;
            }
            else
            {
                if (focus_id < 270 / segment_degrees && focus_id >= 90 / segment_degrees)
                    set_focus_child=get_child(focus_id - 1);
                else
                    set_focus_child=get_child((focus_id + 1) % segment_count);
                set_focus_child->queue_draw();/* focus */
                focus_child->queue_draw();/* remove focus */
                return true;
            }
        default:
            if (!focus_child)
            {
                set_focus_child=get_first_child();
                set_focus_child->queue_draw();/* focus */
	            return true;
           	}
           	else
                return false;
    }
}


void NibblesWindow::ColourWheel::select_segment(Gtk::Widget *segment)
{
	const eWormColour new_colour=(eWormColour)get_segment_id(segment);
	auto box=get_parent();
	assert(box->get_buildable_id()=="player_controls");
	auto player=static_cast<PlayerButton *>(box->get_first_child());
	assert(player->get_buildable_id()=="name_label");
	eWormColour old_colour=get_worm_settings_colour(player->id);
	if(old_colour!=new_colour)
	{
    	/* check if any other player is using this colour */
		std::vector<PlayerButton*> players;
    	unsigned int player_count=get_players(players);
		for(unsigned int worm_id=0;worm_id<player_count;worm_id++)
		{
			if(worm_id!=player->id) /* ignore self */
			{
				eWormColour worm_colour=get_worm_settings_colour(worm_id);
				if(new_colour==worm_colour)
				{
					if(unknown_colour_worm==old_colour)
					{
						/* find an unused colour */
						std::unordered_set<eWormColour> available_worm_colours = {red_worm,green_worm,blue_worm,yellow_worm,cyan_worm,purple_worm};
						available_worm_colours.erase(new_colour);
						for(unsigned int id=0;id<player_count;id++)
						{
							if(id!=player->id)
								available_worm_colours.erase(get_worm_settings_colour(id));
						}
						if(!available_worm_colours.empty())
						{
							auto it=available_worm_colours.begin();
							eWormColour c=*it;
							set_worm_settings_colour(worm_id,c);
							players[worm_id]->set_text(c);
						}
					}
					else
					{
						/* set the other player to my old colour */
						set_worm_settings_colour(worm_id,old_colour);
						players[worm_id]->set_text(old_colour);
						old_colour=unknown_colour_worm; /* don't set any other worm with my new colour to my old colour */
					}
				}
			}
		}
		/* set my new colour */
		set_worm_settings_colour(player->id,new_colour);
		player->set_text(new_colour);
	}
   	player->on_clicked();
}

/*******************************************************************
 *                                                                 *
 *	NibblesWindow::ColourWheelSegment                              *
 *                                                                 *
 *******************************************************************/
void NibblesWindow::ColourWheelSegment::snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>&snapshot)
{
	Gtk::Widget::snapshot_vfunc(snapshot);

    const unsigned int ID = ((ColourWheel *)get_parent ())->get_segment_id (this);
    auto parent_width = ((ColourWheel *)get_parent ())->get_width ();
    auto parent_height = ((ColourWheel *)get_parent ())->get_height ();
    double radius = parent_width > parent_height ? parent_height / 2.0 : parent_width / 2.0;
    double segment = PIx2 / ((ColourWheel *)get_parent ())->get_segment_count ();
    /* path for segment */
    auto p = Gsk::PathBuilder::create();
    /* move to centre of the wheel */
    double cx = parent_width / 2.0/* - offset_from_parent.get_x()*/;
    double cy = parent_height / 2.0/* - offset_from_parent.get_y()*/;
    if (is_focus ())
    {
        cx += sin (segment * ID + segment/2) * (radius / 10);
        cy -= cos (segment * ID + segment/2) * (radius / 10);
    }
    p->move_to ((float)cx, (float)cy);
    /* line to start of arc */
    double x1 = sin (segment * ID) * radius + cx;
    double y1 = -cos (segment * ID) * radius + cy;
    p->line_to ((float)x1, (float)y1);
    /* arc */
    double x2 = sin (segment * (ID + 1)) * radius + cx;
    double y2 = -cos (segment * (ID + 1)) * radius + cy;
    p->svg_arc_to ((float)radius, (float)radius, (float)segment, false, true, (float)x2, (float)y2);
    /* fill the segment with our colour */
    snapshot->append_fill(p->to_path (), Gsk::FillRule::EVEN_ODD, { (get_colour() >> 16 & 0xff) / 255.0f, (get_colour() >> 8 & 0xff) / 255.0f, (get_colour() & 0xff) / 255.0f, 1});
}

Glib::RefPtr<Gsk::Path> NibblesWindow::ColourWheelSegment::calculate_segment_path(uint width, uint height, uint ID, uint segment_count)
{
    /* path for segment */
    auto p = Gsk::PathBuilder::create();
    /* move to centre of the wheel */
    p->move_to (width / 2.0f, height / 2.0f);
    /* line to start of arc (focus position) */
    auto radius = width > height ? height / 2.0 : width / 2.0;
    auto segment = PIx2 / segment_count;
    auto cx = width / 2.0 + sin (segment * ID + segment/2) * (radius / 10);
    auto cy = height / 2.0 - cos (segment * ID + segment/2) * (radius / 10);
    /* line to start of arc */
    p->line_to ((float)(sin (segment * ID) * radius + cx),
        (float)(-cos (segment * ID) * radius + cy));
    /* arc (focus position) */
    p->svg_arc_to ((float)radius, (float)radius, (float)segment, false, true,
        (float)(sin (segment * (ID + 1)) * radius + cx),
        (float)(-cos (segment * (ID + 1)) * radius + cy));
    /* line back to the center of the wheel */
    p->close ();
    return p->to_path();
}

const std::optional<Gdk::Graphene::Rect> NibblesWindow::ColourWheelSegment::get_bounds(unsigned int width, unsigned int height)
{
	auto path=calculate_segment_path(width, height,
		((ColourWheel *)get_parent ())->get_segment_id(this),
		((ColourWheel *)get_parent ())->get_segment_count());
    return path->get_bounds();
}

/*******************************************************************
 *                                                                 *
 *	NibblesWindow::Scores                                          *
 *                                                                 *
 *******************************************************************/
void NibblesWindow::Scores::add_category(const std::string &path, const std::string &file_name)
{
	auto file_path=Glib::build_filename(path, file_name);
	try {
		std::ifstream score_file(file_path);
		if (score_file.is_open())
		{
			while(true)
			{		
				auto [score, score_success] = read_integer(score_file);
				if(score_success)
				{
					auto [date, date_success] = read_integer(score_file);
					if(date_success)
					{
						auto [name, name_success] = read_string(score_file);
						if(name_success)
						{
							auto category_index=to_catagory_index(file_name);
							Score s(score,date,name);
							auto &vector=m_scores[category_index];
							auto it=vector.begin();
							while(it != vector.end() &&
								((*it).score>s.score || (*it).score==s.score && (*it).date<s.date))
								it++;
							vector.insert(it,s);
							if(!m_score_file.contains(category_index))
								m_score_file[category_index]=file_path;
						}
						else
							break;
					}
					else
						break;
				}
				else
					break;
			}
		}
	} catch (const std::exception& ex) {
		/* problem reading file */
	}
}

std::pair<uint64_t,bool> NibblesWindow::Scores::read_integer(std::ifstream &stream)
{

	bool successful_read=true;
	char c;
	uint64_t i=0;
	/* ignore rubbish characters */
	while(successful_read)
	{
		if(stream.get(c))
		{
			if(c>='0' && c<='9')
				break;
		}
		else
			successful_read=false;
	}
	/* read integer */
	while(successful_read)
	{
		if(c>='0' && c<='9')
		{
			i*=10;
			i+=(c-'0');
		}
		else
			break;
		if(!stream.get(c))
			successful_read=false;
	}
	return {i,successful_read};
}

std::pair<std::string,bool> NibblesWindow::Scores::read_string(std::ifstream &stream)
{
	bool successful_read=true;
	char c;
	std::string s;
	/* ignore rubbish characters */
	while(successful_read)
	{
		if(stream.get(c))
		{
			if(c>=' ' && c<0x7f)
				break;
		}
		else
			successful_read=false;
	}
	/* read string */
	while(successful_read)
	{
		if(c>=' ' && c<0x7f)
			s+=c;
		else
			break;
		if(!stream.get(c))
			successful_read=false;
	}
	return {s,successful_read};
}

std::string NibblesWindow::Scores::to_title(uint8_t category_index)
{
	//std::inplace_vector<std::string,256> description =
	std::vector<std::string> description =
	{
		// Translators: text displayed at the top of the high scores dialogue
		_("Fast"),
		_("Medium"),
		_("Slow"),
		_("Beginner"),
		_("Fast with fakes"),
		_("Medium with fakes"),
		_("Slow with fakes"),
		_("Beginner with fakes"),
		_("Fast fixed on level 1"),
		_("Medium fixed on level 1"),
		_("Slow fixed on level 1"),
		_("Beginner fixed on level 1"),
		_("Fast with fakes, fixed on level 1"),
		_("Medium with fakes, fixed on level 1"),
		_("Slow with fakes, fixed on level 1"),
		_("Beginner with fakes, fixed on level 1"),
		_("Fast fixed on level 2"),
		_("Medium fixed on level 2"),
		_("Slow fixed on level 2"),
		_("Beginner fixed on level 2"),
		_("Fast with fakes, fixed on level 2"),
		_("Medium with fakes, fixed on level 2"),
		_("Slow with fakes, fixed on level 2"),
		_("Beginner with fakes, fixed on level 2"),
		_("Fast fixed on level 3"),
		_("Medium fixed on level 3"),
		_("Slow fixed on level 3"),
		_("Beginner fixed on level 3"),
		_("Fast with fakes, fixed on level 3"),
		_("Medium with fakes, fixed on level 3"),
		_("Slow with fakes, fixed on level 3"),
		_("Beginner with fakes, fixed on level 3"),
		_("Fast fixed on level 4"),
		_("Medium fixed on level 4"),
		_("Slow fixed on level 4"),
		_("Beginner fixed on level 4"),
		_("Fast with fakes, fixed on level 4"),
		_("Medium with fakes, fixed on level 4"),
		_("Slow with fakes, fixed on level 4"),
		_("Beginner with fakes, fixed on level 4"),
		_("Fast fixed on level 5"),
		_("Medium fixed on level 5"),
		_("Slow fixed on level 5"),
		_("Beginner fixed on level 5"),
		_("Fast with fakes, fixed on level 5"),
		_("Medium with fakes, fixed on level 5"),
		_("Slow with fakes, fixed on level 5"),
		_("Beginner with fakes, fixed on level 5"),
		_("Fast fixed on level 6"),
		_("Medium fixed on level 6"),
		_("Slow fixed on level 6"),
		_("Beginner fixed on level 6"),
		_("Fast with fakes, fixed on level 6"),
		_("Medium with fakes, fixed on level 6"),
		_("Slow with fakes, fixed on level 6"),
		_("Beginner with fakes, fixed on level 6"),
		_("Fast fixed on level 7"),
		_("Medium fixed on level 7"),
		_("Slow fixed on level 7"),
		_("Beginner fixed on level 7"),
		_("Fast with fakes, fixed on level 7"),
		_("Medium with fakes, fixed on level 7"),
		_("Slow with fakes, fixed on level 7"),
		_("Beginner with fakes, fixed on level 7"),
		_("Fast fixed on level 8"),
		_("Medium fixed on level 8"),
		_("Slow fixed on level 8"),
		_("Beginner fixed on level 8"),
		_("Fast with fakes, fixed on level 8"),
		_("Medium with fakes, fixed on level 8"),
		_("Slow with fakes, fixed on level 8"),
		_("Beginner with fakes, fixed on level 8"),
		_("Fast fixed on level 9"),
		_("Medium fixed on level 9"),
		_("Slow fixed on level 9"),
		_("Beginner fixed on level 9"),
		_("Fast with fakes, fixed on level 9"),
		_("Medium with fakes, fixed on level 9"),
		_("Slow with fakes, fixed on level 9"),
		_("Beginner with fakes, fixed on level 9"),
		_("Fast fixed on level 10"),
		_("Medium fixed on level 10"),
		_("Slow fixed on level 10"),
		_("Beginner fixed on level 10"),
		_("Fast with fakes, fixed on level 10"),
		_("Medium with fakes, fixed on level 10"),
		_("Slow with fakes, fixed on level 10"),
		_("Beginner with fakes, fixed on level 10"),
		_("Fast fixed on level 11"),
		_("Medium fixed on level 11"),
		_("Slow fixed on level 11"),
		_("Beginner fixed on level 11"),
		_("Fast with fakes, fixed on level 11"),
		_("Medium with fakes, fixed on level 11"),
		_("Slow with fakes, fixed on level 11"),
		_("Beginner with fakes, fixed on level 11"),
		_("Fast fixed on level 12"),
		_("Medium fixed on level 12"),
		_("Slow fixed on level 12"),
		_("Beginner fixed on level 12"),
		_("Fast with fakes, fixed on level 12"),
		_("Medium with fakes, fixed on level 12"),
		_("Slow with fakes, fixed on level 12"),
		_("Beginner with fakes, fixed on level 12"),
		_("Fast fixed on level 13"),
		_("Medium fixed on level 13"),
		_("Slow fixed on level 13"),
		_("Beginner fixed on level 13"),
		_("Fast with fakes, fixed on level 13"),
		_("Medium with fakes, fixed on level 13"),
		_("Slow with fakes, fixed on level 13"),
		_("Beginner with fakes, fixed on level 13"),
		_("Fast fixed on level 14"),
		_("Medium fixed on level 14"),
		_("Slow fixed on level 14"),
		_("Beginner fixed on level 14"),
		_("Fast with fakes, fixed on level 14"),
		_("Medium with fakes, fixed on level 14"),
		_("Slow with fakes, fixed on level 14"),
		_("Beginner with fakes, fixed on level 14"),
		_("Fast fixed on level 15"),
		_("Medium fixed on level 15"),
		_("Slow fixed on level 15"),
		_("Beginner fixed on level 15"),
		_("Fast with fakes, fixed on level 15"),
		_("Medium with fakes, fixed on level 15"),
		_("Slow with fakes, fixed on level 15"),
		_("Beginner with fakes, fixed on level 15"),
		_("Fast fixed on level 16"),
		_("Medium fixed on level 16"),
		_("Slow fixed on level 16"),
		_("Beginner fixed on level 16"),
		_("Fast with fakes, fixed on level 16"),
		_("Medium with fakes, fixed on level 16"),
		_("Slow with fakes, fixed on level 16"),
		_("Beginner with fakes, fixed on level 16"),
		_("Fast fixed on level 17"),
		_("Medium fixed on level 17"),
		_("Slow fixed on level 17"),
		_("Beginner fixed on level 17"),
		_("Fast with fakes, fixed on level 17"),
		_("Medium with fakes, fixed on level 17"),
		_("Slow with fakes, fixed on level 17"),
		_("Beginner with fakes, fixed on level 17"),
		_("Fast fixed on level 18"),
		_("Medium fixed on level 18"),
		_("Slow fixed on level 18"),
		_("Beginner fixed on level 18"),
		_("Fast with fakes, fixed on level 18"),
		_("Medium with fakes, fixed on level 18"),
		_("Slow with fakes, fixed on level 18"),
		_("Beginner with fakes, fixed on level 18"),
		_("Fast fixed on level 19"),
		_("Medium fixed on level 19"),
		_("Slow fixed on level 19"),
		_("Beginner fixed on level 19"),
		_("Fast with fakes, fixed on level 19"),
		_("Medium with fakes, fixed on level 19"),
		_("Slow with fakes, fixed on level 19"),
		_("Beginner with fakes, fixed on level 19"),
		_("Fast fixed on level 20"),
		_("Medium fixed on level 20"),
		_("Slow fixed on level 20"),
		_("Beginner fixed on level 20"),
		_("Fast with fakes, fixed on level 20"),
		_("Medium with fakes, fixed on level 20"),
		_("Slow with fakes, fixed on level 20"),
		_("Beginner with fakes, fixed on level 20"),
		_("Fast fixed on level 21"),
		_("Medium fixed on level 21"),
		_("Slow fixed on level 21"),
		_("Beginner fixed on level 21"),
		_("Fast with fakes, fixed on level 21"),
		_("Medium with fakes, fixed on level 21"),
		_("Slow with fakes, fixed on level 21"),
		_("Beginner with fakes, fixed on level 21"),
		_("Fast fixed on level 22"),
		_("Medium fixed on level 22"),
		_("Slow fixed on level 22"),
		_("Beginner fixed on level 22"),
		_("Fast with fakes, fixed on level 22"),
		_("Medium with fakes, fixed on level 22"),
		_("Slow with fakes, fixed on level 22"),
		_("Beginner with fakes, fixed on level 22"),
		_("Fast fixed on level 23"),
		_("Medium fixed on level 23"),
		_("Slow fixed on level 23"),
		_("Beginner fixed on level 23"),
		_("Fast with fakes, fixed on level 23"),
		_("Medium with fakes, fixed on level 23"),
		_("Slow with fakes, fixed on level 23"),
		_("Beginner with fakes, fixed on level 23"),
		_("Fast fixed on level 24"),
		_("Medium fixed on level 24"),
		_("Slow fixed on level 24"),
		_("Beginner fixed on level 24"),
		_("Fast with fakes, fixed on level 24"),
		_("Medium with fakes, fixed on level 24"),
		_("Slow with fakes, fixed on level 24"),
		_("Beginner with fakes, fixed on level 24"),
		_("Fast fixed on level 25"),
		_("Medium fixed on level 25"),
		_("Slow fixed on level 25"),
		_("Beginner fixed on level 25"),
		_("Fast with fakes, fixed on level 25"),
		_("Medium with fakes, fixed on level 25"),
		_("Slow with fakes, fixed on level 25"),
		_("Beginner with fakes, fixed on level 25"),
		_("Fast fixed on level 26"),
		_("Medium fixed on level 26"),
		_("Slow fixed on level 26"),
		_("Beginner fixed on level 26"),
		_("Fast with fakes, fixed on level 26"),
		_("Medium with fakes, fixed on level 26"),
		_("Slow with fakes, fixed on level 26"),
		_("Beginner with fakes, fixed on level 26"),
		_("Fast fixed on level 27"),
		_("Medium fixed on level 27"),
		_("Slow fixed on level 27"),
		_("Beginner fixed on level 27"),
		_("Fast with fakes, fixed on level 27"),
		_("Medium with fakes, fixed on level 27"),
		_("Slow with fakes, fixed on level 27"),
		_("Beginner with fakes, fixed on level 27"),
		_("Fast fixed on level 28"),
		_("Medium fixed on level 28"),
		_("Slow fixed on level 28"),
		_("Beginner fixed on level 28"),
		_("Fast with fakes, fixed on level 28"),
		_("Medium with fakes, fixed on level 28"),
		_("Slow with fakes, fixed on level 28"),
		_("Beginner with fakes, fixed on level 28"),
		_("Fast fixed on level 29"),
		_("Medium fixed on level 29"),
		_("Slow fixed on level 29"),
		_("Beginner fixed on level 29"),
		_("Fast with fakes, fixed on level 29"),
		_("Medium with fakes, fixed on level 29"),
		_("Slow with fakes, fixed on level 29"),
		_("Beginner with fakes, fixed on level 29"),
		_("Fast fixed on level 30"),
		_("Medium fixed on level 30"),
		_("Slow fixed on level 30"),
		_("Beginner fixed on level 30"),
		_("Fast with fakes, fixed on level 30"),
		_("Medium with fakes, fixed on level 30"),
		_("Slow with fakes, fixed on level 30"),
		_("Beginner with fakes, fixed on level 30"),
		_("Fast with random levels"),
		_("Medium with random levels"),
		_("Slow with random levels"),
		_("Beginner with random levels"),
		_("Fast with fakes and random levels"),
		_("Medium with fakes and random levels"),
		_("Slow with fakes and random levels"),
		_("Beginner with fakes and random levels")
	};

	return description[category_index];
}

void NibblesWindow::Scores::display_scores(uint8_t category_index)
{
static Glib::RefPtr<Gio::ListStore<RowData>> store;

	auto child=get_child();
	if(auto view = dynamic_cast<Gtk::ColumnView*>(child))
	{
		store->remove_all();
		scores_to_store(category_index, store);
	}
	else
	{
		view = Gtk::make_managed<Gtk::ColumnView>();
		view->set_reorderable(false);
		view->set_tab_behavior(Gtk::ListTabBehavior::ITEM);

		/* rank column */
		auto rank_factory=Gtk::SignalListItemFactory::create();
		rank_factory->signal_setup().connect(sigc::track_obj([](const Glib::RefPtr<Gtk::ListItem>& item) {
            item->set_child(*Gtk::make_managed<Gtk::Label>("", Gtk::Align::START));
		}));
		rank_factory->signal_bind().connect(sigc::track_obj([](const Glib::RefPtr<Gtk::ListItem>& item) {
    		auto data = std::dynamic_pointer_cast<RowData>(item->get_item());
    		auto* label = dynamic_cast<Gtk::Label*>(item->get_child());
		    if (data && label) {
		        label->set_text(std::to_string(data->get_rank()));
		    }
        }));
		// Translators: text displayed at the top of the first column in the high scores dialogue
		auto rank_column=Gtk::ColumnViewColumn::create(_("Rank"),rank_factory);
        rank_column->set_expand(true);
		rank_column->set_fixed_width(0);
        view->append_column(rank_column);
        
        /* score column */
		auto score_factory=Gtk::SignalListItemFactory::create();
		score_factory->signal_setup().connect(sigc::track_obj([](const Glib::RefPtr<Gtk::ListItem>& item) {
            item->set_child(*Gtk::make_managed<Gtk::Label>("", Gtk::Align::START));
		}));
		score_factory->signal_bind().connect(sigc::track_obj([](const Glib::RefPtr<Gtk::ListItem>& item) {
    		auto data = std::dynamic_pointer_cast<RowData>(item->get_item());
    		auto* label = dynamic_cast<Gtk::Label*>(item->get_child());
		    if (data && label) {
		        label->set_text(std::to_string(data->get_score()));
		    }
        }));
		// Translators: text displayed at the top of the second column in the high scores dialogue
		auto score_column=Gtk::ColumnViewColumn::create(_("Score"),score_factory);
        score_column->set_expand(true);
		score_column->set_fixed_width(0);
        view->append_column(score_column);
        
        /* player name column */
		auto name_factory=Gtk::SignalListItemFactory::create();
		name_factory->signal_setup().connect(sigc::track_obj([](const Glib::RefPtr<Gtk::ListItem>& item) {
			auto entry = Gtk::make_managed<Gtk::Entry>();
			/*entry->signal_changed().connect(sigc::track_obj([entry]() {
					entry->set_has_frame(false);
					entry->set_editable(false);
			}));*/
			entry->signal_activate().connect(sigc::track_obj([entry]() {
				entry->set_has_frame(false);
				entry->set_editable(false);
			}));
            item->set_child(*entry);
		}));
		name_factory->signal_bind().connect(sigc::track_obj([](const Glib::RefPtr<Gtk::ListItem>& item) {
    		auto data = std::dynamic_pointer_cast<RowData>(item->get_item());
			auto* entry = dynamic_cast<Gtk::Entry*>(item->get_child());
    		if(data && entry)
    		{
				entry->set_text(data->get_name());
    			if(data->get_modify())
					entry->grab_focus();
				else
				{
					entry->set_has_frame(false);
					entry->set_editable(false);
				}
			}
        }));
		// Translators: text displayed at the top of the third column in the high scores dialogue
        auto player_column=Gtk::ColumnViewColumn::create(_("Player"),name_factory);
        player_column->set_expand(true);
		player_column->set_fixed_width(0);
		view->append_column(player_column);
		
		store = Gio::ListStore<RowData>::create();
		scores_to_store(category_index, store);
		auto selection_model = Gtk::NoSelection::create(store);
		view->set_model(selection_model);

		auto scrolled_window = Gtk::make_managed<Gtk::ScrolledWindow>();
		scrolled_window->set_min_content_height(440);
		scrolled_window->set_child(*view);

		set_child(*scrolled_window);
	}
	m_display_category = category_index;
}


