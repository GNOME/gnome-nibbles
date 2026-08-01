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

#include <iostream>
#include <gtkmm.h>
#include <gsk/gsk.h>
#include <cassert>
#include <mutex>
#include <source_location>/* for std::source_location::current().file_name() */
#include <unordered_set>
#include <bitset>
#include <forward_list>
#include <fstream>
#include <vector>
//#include <inplace_vector>
#include <functional>
#include <chrono>
#include <queue>
#include <gsound.h>

/* language */
#include <locale>
#include <glib/gi18n.h>

#include "definitions.h"
#include "map.h"
#include "pseudo_random.h"
#include "bonus.h"
#include "position.h"
#include "worm.h"
#include "warp.h"
#include "game.h"
#include "view.h"

inline void utoa(unsigned int u, Glib::ustring &result, unsigned int minimum_length=1)
{
	char buffer[10+1];
	char *p;
	p=&buffer[10];
	*(p--)='\0';
	for(;p>=buffer && u>0;)
	{
		*(p--)=(u%10)+'0';
		u/=10;
	}
	/* pad with zero until the minimum length is reached */
	for(;p>=buffer && buffer+sizeof(buffer)-p-2<minimum_length;*(p--)='0');
	result+=p+1;
}

inline Glib::ustring utoa(unsigned long u, unsigned long minimum_length=1)
{
	char buffer[20+1];
	char *p;
	p=&buffer[20];
	*(p--)='\0';
	for(;p>=buffer && u>0;)
	{
		*(p--)=(u%10)+'0';
		u/=10;
	}
	/* pad with zero until the minimum length is reached */
	for(;p>=buffer && buffer+sizeof(buffer)-p-2<minimum_length;*(p--)='0');
	return p+1;
}

View::View(Game::Progress progress, unsigned long start_level, unsigned long speed,
	Gtk::Button &pause_button,
	std::function<void(const std::vector<WormScore>)> game_over) :
	progress(progress), speed(speed), pause_button(pause_button),
	game_over(game_over), Gtk::Overlay(), static_view(*this), active_view(*this),
	game(play_sound,get_worm_settings_colour,
	[this](eWormColour worm_colour, unsigned long lives) {/*life_change*/
        if(score_box.contains(worm_colour))
        {
        	Gtk::Grid *pGrid=get_life_grid(score_box[worm_colour]);
        	if(pGrid)
        	{
				for (auto* child : pGrid->get_children())
					pGrid->remove(*child);
				for(unsigned int i=pGrid->get_children().size();i<lives;
					pGrid->attach(*Gtk::make_managed<Life>(),i % 6,i / 6,1,1),i++);
        	}
        }
    },
	[this](eWormColour worm_colour, unsigned long score) {/*score_change*/
        if(score_box.contains(worm_colour))
        {
        	Gtk::Label *pLabel=get_score_label(score_box[worm_colour]);
        	if(pLabel)
        	{
	        	pLabel->set_text(utoa(score));
        	}
        }
    },
    progress)
{
	unsigned long level;
	switch(progress)
	{
		case Game::Progress::SEQUENTIAL:
			level=start_level;
			break;
		case Game::Progress::RANDOM:
			level=pseudo_random(26)+1;
			break;
		case Game::Progress::FIXED:
			level=start_level;
			break;
		case Game::Progress::TEST:
			level=1;
			break;
	}
	load_board_level(level);

    set_child(static_view);
    add_overlay(active_view);
    
    /* do the next initilisation after being prepended to "game_box" */
    property_parent().signal_changed().connect(sigc::track_obj(
    	[this]() ->
    		void
    		{
    			if(get_parent())
	    			initialise_and_start();
	    		else
	    			timer.unset();
    		},
    		*this
    	));
}

void View::play_sound(const Glib::ustring &sound)
{
    GError* error = nullptr;
    
    // 1. Create and initialize the GSound context
    GSoundContext* ctx = gsound_context_new(nullptr, &error);
    if (!ctx) {
        std::cerr << "Failed to create GSound context: " << error->message << std::endl;
        g_error_free(error);
        return;
    }

	Glib::ustring path=Glib::build_filename(SOUND_DIRECTORY, sound+".ogg");
    // 2. Play a simple system sound event or a specific audio file path
    // Pass attributes as key-value string pairs, ending with a final nullptr
    gboolean success = gsound_context_play_simple(ctx, nullptr, &error,
        //GSOUND_ATTR_EVENT_ID, "audio-volume-change", // System theme sound event
        // To use an absolute path file instead, swap the line above with:
        GSOUND_ATTR_MEDIA_FILENAME, path.c_str(), 
        nullptr
    );

    if (!success) {
        std::cerr << "Error playing sound: " << path << " " << error->message << std::endl;
        g_error_free(error);
    }

    // 3. Clean up the context allocated on the heap
    g_object_unref(ctx);
}

void View::initialise_and_start()
{
	/* Initialise and start the game */

	/* in here we know that get_parent() is valid */

	/* initilise player count and ai count */        			
	auto pSettings = Gio::Settings::create(APP_NAME);
	player_count=std::clamp(pSettings->get_int(PLAYER_SETTINGS), 1 , 4);
	ai_count=std::clamp(pSettings->get_int(AI_SETTINGS), 0, 5);
	if(player_count+ai_count>6)
	{
		ai_count=6-player_count;
		pSettings->set_int(AI_SETTINGS,ai_count);
	}
	
	/* build the score board */
	std::unordered_set<eWormColour> colours_used;
	Gtk::Box* pBox=dynamic_cast<Gtk::Box*>(get_scoreboard()->get_first_child());
	for(unsigned int worm=0;worm<player_count+ai_count;worm++)
	{
		eWormColour c;
		if(worm_colour.size()<worm+1)
		{
			c=get_worm_settings_colour(worm);
			if(colours_used.contains(c))
			{
				for(c=red_worm;c<unknown_colour_worm && colours_used.contains(c);++c);
			}
			set_worm_settings_colour(worm,c);
			worm_colour.push_back(c);
		}
		else
			c=worm_colour[worm];
		colours_used.insert(c);
		if(!pBox)
		{
			auto name=get_worm_name(worm);
			pBox=create_score_box(name,c);
			score_box[c]=pBox;
			get_scoreboard()->append(*pBox);

			pBox=nullptr;
		}
		else
		{
			/* make sure there are six lives */
			Gtk::Grid *pGrid=get_life_grid(pBox);
			for (auto* child : pGrid->get_children())
				pGrid->remove(*child);
			for(unsigned int i=pGrid->get_children().size();i<6;
				pGrid->attach(*Gtk::make_managed<Life>(),i++,0,1,1));
			/* set the score to 0 */
			Gtk::Label *pLabel=get_score_label(pBox);
			pLabel->set_text("0");
			
			pBox=dynamic_cast<Gtk::Box*>(pBox->get_next_sibling());
		}
	}
	
	/* switch to the score board (not paused)*/
	get_statusbar_stack()->set_visible_child("scoreboard");
	
	/* worms */
    game.create_worms(player_count, ai_count);
	game.spawn_worms(true);
    game.add_bonus(true);

    /* play game */
    paused=false;
    play();
}

void View::load_board_level(unsigned long level)
{
	Glib::ustring filename="level";
	utoa(level, filename, 3);
	filename+=".gnl";
	auto path=Glib::build_filename(PKGDATADIR, "levels", filename);
	game.load_board_from_file(path.c_str(), level);
	current_level=level;
	levels.set(current_level-1);
}

bool View::play()
{
	timer.unset();
	if(!paused)
	{
		auto state=game.get_game_status();
		if(state==Game::ACTIVE)
		{
			auto start = std::chrono::steady_clock::now();
			game.move_worms();
			active_view.redraw();
			auto finish = std::chrono::steady_clock::now();
			const long level_delay[]={52,70,105,140};/* milli-seconds */
			auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
			unsigned long delay=0;
			if(elapsed_ms < level_delay[speed-1])
				delay = level_delay[speed-1] - elapsed_ms;
			//timer=Glib::signal_timeout().connect(sigc::mem_fun(*this, &View::play), delay);
			timer.set(sigc::mem_fun(*this, &View::play), delay);
		}
		else if(state==Game::NEWROUND)
		{
			pause_button.set_visible(0);
			if(levels.all()) /* all levels have been compleated */
			{
				/* VICTORY */
				auto *box=Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
				box->set_valign(Gtk::Align::CENTER);
				// Translators: first line of congratulations message for compleating the game
				auto *message=create_label(_("Congratulations"));
				box->append(*message);
				// Translators: second line of congratulations message for compleating the game
				message=create_label(_("You have completed every level!"),10);
				box->append(*message);

				// Translators: button to press to finish the game
				auto *button=create_button(_("_Done"));
				box->append(*button);

				button->signal_clicked().connect(sigc::track_obj([this,box]() {
					remove_overlay(*box);
					game_over(game.get_worm_scores());
				}));
			    add_overlay(*box);
			}
			else
			{
				unsigned long next_level;
				if(Game::Progress::SEQUENTIAL==progress)
				{
					next_level=current_level+1;
					if(next_level==27)
						next_level=1;
				}
				else
				{
					std::unordered_set<unsigned long> next_levels;
					next_levels.reserve(25);
					for(unsigned long i=0;i<26;i++)
					{
						if(!levels[i])
							next_levels.emplace(i+1);
					}
					auto pick = next_levels.cbegin();
					std::advance(pick, pseudo_random(next_levels.size()));
					next_level=*pick;
				}

				auto *box=Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
				box->set_valign(Gtk::Align::CENTER);
				auto *message=create_label(get_level_completed_message(current_level));
				box->append(*message);
				if(Game::Progress::RANDOM==progress)
				{
					message=create_label(get_next_level_message(next_level));
					message->set_margin_top(10);
					box->append(*message);
				}
				// Translators: button to press to move on to the next level of the game
				auto *button=create_button(_("_Next Level"));
				box->append(*button);
				
				button->signal_clicked().connect(sigc::track_obj([this,next_level,box]() {
					remove_overlay(*box);
					
					load_board_level(next_level);
					game.spawn_worms(true);
					static_view.redraw();
				    game.add_bonus(true);

				    /* play game */
				    paused=false;
					play();
				}));
			    add_overlay(*box);
			}
		}
		else /* VICTORY or GAMEOVER */
		{
			pause_button.set_visible(0);
			auto *box=Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
			box->set_valign(Gtk::Align::CENTER);
			if(state==Game::VICTORY)
			{
				// Translators: first line of congratulations message for compleating the game
				auto *message=create_label(_("Congratulations"));
				box->append(*message);
				// Translators: second line of congratulations message for compleating the game
				message=create_label(_("You have defeated every enemy worm!"),10);
				box->append(*message);

			}
			else /* GAMEOVER */
			{
				// Translators: game over message
				auto *message=create_label(_("Game Over"));
				box->append(*message);
			}
			// Translators: button to press to finish the game
			auto *button=create_button(_("_Done"));
			box->append(*button);

			button->signal_clicked().connect(sigc::track_obj([this,box]() {
				remove_overlay(*box);
				game_over(game.get_worm_scores());
			}));
		    add_overlay(*box);
		}
	}
	return false;
}

Gtk::Label* View::create_label(Glib::ustring text)
{
	auto *message=Gtk::make_managed<Gtk::Label>(text);
	message->add_css_class("overview");
	message->set_halign(Gtk::Align::CENTER);
	return message;
}

Gtk::Button* View::create_button(Glib::ustring text)
{
	auto *button=Gtk::make_managed<Gtk::Button>(text);
	button->set_use_underline(true);
	button->add_css_class("rounded");
	button->set_halign(Gtk::Align::CENTER);
	button->set_margin_top(100);
	return button;
}

const Glib::ustring View::get_worm_name(unsigned int worm_id)
{
	Glib::ustring name;
	switch(worm_id)
	{
		case 0:
            // Translators: the first worm's name.
            name=_("Worm 1");
            break;
		case 1:
            // Translators: the seconds worm's name.
            name=_("Worm 2");
            break;
		case 2:
            // Translators: the third worm's name.
            name=_("Worm 3");
            break;
		case 3:
            // Translators: the fourth worm's name.
            name=_("Worm 4");
            break;
		case 4:
            // Translators: the fifth worm's name.
            name=_("Worm 5");
            break;
		case 5:
            // Translators: the sixth worm's name.
            name=_("Worm 6");
            break;
        default:
        	{
        		Glib::ustring buffer=std::source_location::current().file_name();
        		buffer+=": More than six worms defined!";
	        	g_critical(buffer.c_str());
	        }
            break;
	}
	return name;
}

const Glib::ustring View::get_level_completed_message(unsigned long level)
{
	switch(level)
	{
		case 1:
			// Translators: information message indicating the completion of a level
			return _("Level one completed.");
		case 2:
			// Translators: information message indicating the completion of a level
			return _("Level two completed.");
		case 3:
			// Translators: information message indicating the completion of a level
			return _("Level three completed.");
		case 4:
			// Translators: information message indicating the completion of a level
			return _("Level four completed.");
		case 5:
			// Translators: information message indicating the completion of a level
			return _("Level five completed.");
		case 6:
			// Translators: information message indicating the completion of a level
			return _("Level six completed.");
		case 7:
			// Translators: information message indicating the completion of a level
			return _("Level seven completed.");
		case 8:
			// Translators: information message indicating the completion of a level
			return _("Level eight completed.");
		case 9:
			// Translators: information message indicating the completion of a level
			return _("Level nine completed.");
		case 10:
			// Translators: information message indicating the completion of a level
			return _("Level ten completed.");
		case 11:
			// Translators: information message indicating the completion of a level
			return _("Level eleven completed.");
		case 12:
			// Translators: information message indicating the completion of a level
			return _("Level twelve completed.");
		case 13:
			// Translators: information message indicating the completion of a level
			return _("Level thirteen completed.");
		case 14:
			// Translators: information message indicating the completion of a level
			return _("Level fourteen completed.");
		case 15:
			// Translators: information message indicating the completion of a level
			return _("Level fifteen completed.");
		case 16:
			// Translators: information message indicating the completion of a level
			return _("Level sixteen completed.");
		case 17:
			// Translators: information message indicating the completion of a level
			return _("Level seventeen completed.");
		case 18:
			// Translators: information message indicating the completion of a level
			return _("Level eighteen completed.");
		case 19:
			// Translators: information message indicating the completion of a level
			return _("Level nineteen completed.");
		case 20:
			// Translators: information message indicating the completion of a level
			return _("Level twenty completed.");
		case 21:
			// Translators: information message indicating the completion of a level
			return _("Level twenty one completed.");
		case 22:
			// Translators: information message indicating the completion of a level
			return _("Level twenty two completed.");
		case 23:
			// Translators: information message indicating the completion of a level
			return _("Level twenty three completed.");
		case 24:
			// Translators: information message indicating the completion of a level
			return _("Level twenty four completed.");
		case 25:
			// Translators: information message indicating the completion of a level
			return _("Level twenty five completed.");
		case 26:
			// Translators: information message indicating the completion of a level
			return _("Level twenty six completed.");
		case 27:
			// Translators: information message indicating the completion of a level
			return _("Level twenty seven completed.");
		case 28:
			// Translators: information message indicating the completion of a level
			return _("Level twenty eight completed.");
		case 29:
			// Translators: information message indicating the completion of a level
			return _("Level twenty nine completed.");
		case 30:
			// Translators: information message indicating the completion of a level
			return _("Level thirty completed.");
		default:
			// Translators: information message indicating the completion of a level
			return _("Unknown level completed.");
	}
}

const Glib::ustring View::get_next_level_message(unsigned long level)
{
	switch(level)
	{
		case 1:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level one.");
		case 2:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level two.");
		case 3:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level three.");
		case 4:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level four.");
		case 5:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level five.");
		case 6:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level six.");
		case 7:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level seven.");
		case 8:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level eight.");
		case 9:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level nine.");
		case 10:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level ten.");
		case 11:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level eleven.");
		case 12:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level twelve.");
		case 13:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level thirteen.");
		case 14:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level fourteen.");
		case 15:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level fifteen.");
		case 16:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level sixteen.");
		case 17:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level seventeen.");
		case 18:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level eighteen.");
		case 19:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level nineteen.");
		case 20:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level twenty.");
		case 21:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level twenty one.");
		case 22:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level twenty two.");
		case 23:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level twenty three.");
		case 24:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level twenty four.");
		case 25:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level twenty five.");
		case 26:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level twenty six.");
		case 27:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level twenty seven.");
		case 28:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level twenty eight.");
		case 29:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level twenty nine.");
		case 30:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is level thirty.");
		default:
			// Translators: information message indicating the next level in a random level game
			return _("The next level is unknown.");
	}
}

const Glib::ustring View::get_countdown_message(unsigned long count)
{
	switch(count)
	{
		case 3:
			// Translators: information message indicating 3 seconds until the start of play
			return _("3");
		case 2:
			// Translators: information message indicating 2 seconds until the start of play
			return _("2");
		case 1:
			// Translators: information message indicating 1 seconds until the start of play
			return _("1");
		default:
			return "";
	}
}

