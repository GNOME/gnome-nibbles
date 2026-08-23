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
#include <inplace_vector>
#include <functional>
#include <chrono>
#include <queue>
#include <gsound.h>

/* language */
#include <locale>
#include <glib/gi18n.h>

#include "system_integer.h"
#include "definitions.h"
#include "critical.h"
#include "map.h"
#include "pseudo_random.h"
#include "bonus.h"
#include "position.h"
#include "worm.h"
#include "warp.h"
#include "game.h"
#include "view.h"

inline void utoa(uint32_t u, Glib::ustring &result, intsys minimum_length=1)
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

inline Glib::ustring utoa(uint64_t u, intsys minimum_length=1)
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

/*******************************************************************
 *                                                                 *
 *	View                                                  *
 *                                                                 *
 *******************************************************************/
View::View(Game::Progress progress, uintsys start_level, uintsys speed, bool fakes,
	Gtk::Button &pause_button,
	std::function<void(const Glib::ustring &level)> set_level_description,
	std::function<void(const std::vector<WormScore>)> game_over) : Gtk::Overlay(),
	progress(progress), speed(speed), pause_button(pause_button),
	set_level_description(set_level_description), game_over(game_over),
	static_view(*this), active_view(*this),
	game(
	[this](const Glib::ustring &sound) {/*play_sound*/
		play_sound(sound);
	},get_worm_settings_colour,
	[this](eWormColour worm_colour, uintsys lives) {/*life_change*/
		if(score_box.contains(worm_colour))
		{
			Gtk::Grid *pGrid=get_life_grid(score_box[worm_colour]);
			if(pGrid)
			{
				for (auto* child : pGrid->get_children())
					pGrid->remove(*child);
				for(uintsys i=pGrid->get_children().size();i<lives && i<6;
					pGrid->attach(*Gtk::make_managed<Life>(i==5 && lives>6?lives:0),i % 6,0,1,1),i++);
			}
		}
	},
	[this](eWormColour worm_colour, uintsys score) {/*score_change*/
		if(score_box.contains(worm_colour))
		{
			Gtk::Label *pLabel=get_score_label(score_box[worm_colour]);
			if(pLabel)
			{
				pLabel->set_text(utoa(score));
			}
		}
	},
	progress,fakes)
{
	// setup sound
	GError* error = nullptr;
	ctx = gsound_context_new(nullptr, &error);
	if(!ctx)
	{
		std::cerr << "Failed to create GSound context: " << error->message << std::endl;
		g_error_free(error);
	}

	uintsys level;
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
		default: /*Game::Progress::TEST*/
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
	if(!mute && nullptr!=ctx)
	{	
		GError* error = nullptr;
		Glib::ustring path=Glib::build_filename(SOUND_DIRECTORY, sound+".ogg");
		gboolean success = gsound_context_play_simple(ctx, nullptr, &error,
			GSOUND_ATTR_MEDIA_FILENAME, path.c_str(), 
			nullptr
		);
		if (!success) {
			std::cerr << "Error playing sound: " << path << " " << error->message << std::endl;
			g_error_free(error);
		}
	}
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
		auto name=get_worm_name(worm);
		if(!pBox)
		{
			pBox=create_score_box(name,c);
			score_box[c]=pBox;
			names[c]=name;
			get_scoreboard()->append(*pBox);
			pBox=nullptr;
		}
		else
		{
			score_box[c]=pBox;
			names[c]=name;
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
	countdown=3;
	play();
}

void View::load_board_level(uintsys level)
{
	Glib::ustring filename="level";
	utoa(level, filename, 3);
	filename+=".gnl";
	auto path=Glib::build_filename(PKGDATADIR, "levels", filename);
	game.load_board_from_file(path.c_str(), level);
	current_level=level;
	auto level_description=get_level_description(level);
	set_level_description(level_description);
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
			if(countdown>0)
			{
				active_view.redraw();
				play_sound("gobble");
				timer.set(sigc::mem_fun(*this, &View::play), 1000/*milli-seconds*/);
			}
			else
			{
				auto start = std::chrono::steady_clock::now();
				game.move_worms();
				active_view.redraw();
				auto finish = std::chrono::steady_clock::now();
				const intsys level_delay[]={52,70,105,140};/* milli-seconds */
				auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
				uintsys delay=1;
				if(elapsed_ms < level_delay[speed-1])
					delay = level_delay[speed-1] - elapsed_ms;
				timer.set(sigc::mem_fun(*this, &View::play), delay);
			}
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
					auto s=game.get_worm_scores();
					for(auto &w : s)
						w.worm_name=names[w.colour];
					game_over(s);
				}));
				add_overlay(*box);
			}
			else
			{
				uintsys next_level;
				if(Game::Progress::SEQUENTIAL==progress)
				{
					next_level=current_level+1;
					if(next_level==27)
						next_level=1;
				}
				else
				{
					std::unordered_set<uintsys> next_levels;
					next_levels.reserve(25);
					for(uintsys i=0;i<26;i++)
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
					countdown=3;
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
				auto s=game.get_worm_scores();
				for(auto &w : s)
					w.worm_name=names[w.colour];
				game_over(s);
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
				critical(buffer);
			}
			break;
	}
	return name;
}

const Glib::ustring View::get_level_completed_message(uintsys level)
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

const Glib::ustring View::get_next_level_message(uintsys level)
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

const Glib::ustring View::get_level_description(uintsys level)
{
	switch(level)
	{
		case 1:
			// Translators: information message describing the level
			return _("level one");
		case 2:
			// Translators: information message describing the level
			return _("level two");
		case 3:
			// Translators: information message describing the level
			return _("level three");
		case 4:
			// Translators: information message describing the level
			return _("level four");
		case 5:
			// Translators: information message describing the level
			return _("level five");
		case 6:
			// Translators: information message describing the level
			return _("level six");
		case 7:
			// Translators: information message describing the level
			return _("level seven");
		case 8:
			// Translators: information message describing the level
			return _("level eight");
		case 9:
			// Translators: information message describing the level
			return _("level nine");
		case 10:
			// Translators: information message describing the level
			return _("level ten");
		case 11:
			// Translators: information message describing the level
			return _("level eleven");
		case 12:
			// Translators: information message describing the level
			return _("level twelve");
		case 13:
			// Translators: information message describing the level
			return _("level thirteen");
		case 14:
			// Translators: information message describing the level
			return _("level fourteen");
		case 15:
			// Translators: information message describing the level
			return _("level fifteen");
		case 16:
			// Translators: information message describing the level
			return _("level sixteen");
		case 17:
			// Translators: information message describing the level
			return _("level seventeen");
		case 18:
			// Translators: information message describing the level
			return _("level eighteen");
		case 19:
			// Translators: information message describing the level
			return _("level nineteen");
		case 20:
			// Translators: information message describing the level
			return _("level twenty");
		case 21:
			// Translators: information message describing the level
			return _("level twenty one");
		case 22:
			// Translators: information message describing the level
			return _("level twenty two");
		case 23:
			// Translators: information message describing the level
			return _("level twenty three");
		case 24:
			// Translators: information message describing the level
			return _("level twenty four");
		case 25:
			// Translators: information message describing the level
			return _("level twenty five");
		case 26:
			// Translators: information message describing the level
			return _("level twenty six");
		case 27:
			// Translators: information message describing the level
			return _("level twenty seven");
		case 28:
			// Translators: information message describing the level
			return _("level twenty eight");
		case 29:
			// Translators: information message describing the level
			return _("level twenty nine");
		case 30:
			// Translators: information message describing the level
			return _("level thirty");
		default:
			// Translators: information message describing the level
			return _("unknown level");
	}
}

const Glib::ustring View::get_countdown_message(uintsys count)
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

/*******************************************************************
 *                                                                 *
 *	View::StaticView                                               *
 *                                                                 *
 *******************************************************************/
void View::StaticView::snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>& snapshot)
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
	for (unsigned int x = 0; x < view.game.get_width(); x++)
	{
		for (unsigned int y = 0; y < view.game.get_height(); y++)
		{
			// walls
			if (view.game[x,y] >= 'b' && view.game[x,y] <= 'l')
				draw_wall_segment (view.game[x,y],
					snapshot, x_delta * x + x_offset, y_delta * y + y_offset, x_delta, y_delta);
		}
	}
}
void View::StaticView::draw_wall_segment (char i, const Glib::RefPtr<Gtk::Snapshot>& s,
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

/*******************************************************************
 *                                                                 *
 *	View::ActiveView                                               *
 *                                                                 *
 *******************************************************************/
void View::ActiveView::snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>& snapshot)
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

	/* draw bonuses */
	for(auto bonus : view.game.get_bonuses())
	{
		draw_bonus(snapshot, x_delta * bonus.x + x_offset, y_delta * bonus.y + y_offset, x_delta + x_delta, y_delta + y_delta, bonus.type, animate);
	}

	if (view.countdown_left() > 0)
	{
		// count down
		int font_size = 252;
		auto text=std::to_string(view.countdown_left());
		auto [w,h]=calculate_text_size(text, font_size);
		draw_text_font_size(snapshot, (int)(x_offset + x_delta * (view.game.get_width() / 2) - w / 2), (int)(y_offset + y_delta * (view.game.get_height() / 2) - h / 2), text, font_size);

		//draw name labels
		uintsys id=0;
		for(const auto &worm : view.game.get_worms())
		{
			if (!worm.get_positions().is_empty())
			{
				if (worm.get_direction() == eDirection::UP || worm.get_direction() == eDirection::DOWN)
				{
					// vertical worm
					int middle = worm.get_length() / 2;
					auto p=worm.get_positions()[middle];
					draw_text_target_width(snapshot, x_offset + x_delta * (p.x + 1) + x_delta / 2,
								  y_offset + y_delta * (p.y),
								  view.get_worm_name(id++), x_delta * worm.get_length());
				}
				else if (worm.get_direction() == eDirection::LEFT || worm.get_direction() == eDirection::RIGHT)
				{
					// horizontal worm
					auto head=worm.get_positions()[0];
					int x = head.x;
					if (x > worm.get_positions()[worm.get_length()-1].x)
						x = worm.get_positions()[worm.get_length()-1].x;
					draw_text_target_width(snapshot, x_offset + x_delta * x,
								  y_offset + y_delta * (head.y) - y_delta,
								  view.get_worm_name(id++), x_delta * worm.get_length());
				}
			}
		}
		view.countdown_decrement();
	}
}
void View::ActiveView::draw_bonus(const Glib::RefPtr<Gtk::Snapshot> &s, int x, int y, int x_size, int y_size, Bonus::eType type, uint64_t animate)
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
void View::ActiveView::draw_worm_segment (const Glib::RefPtr<Gtk::Snapshot> &s,
	int x, int y, int x_size, int y_size, eWormColour colour,
	bool is_materialized, bool eaten_bonus)
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
	const float x_s13 = x_size / 3.0f;
	const float x_s23 = x_s13 + x_s13;
	const float x_s16 = x_size / 6.0f;
	const float x_s56 = x_s16 * 5.0f;
	const float y_s13 = y_size / 3.0f;
	const float y_s23 = y_s13 + y_s13;
	const float y_s16 = y_size / 6.0f;
	const float y_s56 = y_s16 * 5.0f;
	auto path = Gsk::PathBuilder::create();
	/* top right corner */
	path->move_to(x + x_s23, y + 0);
	path->svg_arc_to(x_s13, y_s13, PI2, false, true, x + x_size, y + y_s13);
	/* bottom right corner */
	path->line_to(x + x_size, y + y_s23);
	path->svg_arc_to(x_s13, y_s13, PI2, false, true, x + x_s23, y + y_size);
	/* bottom left corner */
	path->line_to(x + x_s13, y + y_size);
	path->svg_arc_to(x_s13, y_s13, PI2, false, true, x + 0, y + y_s23);
	/* top left corner */
	path->line_to(x + 0, y + y_s13);
	path->svg_arc_to(x_s13, y_s13, PI2, false, true, x + x_s13, y + 0);
	if(!is_materialized) /* leave centre empty */
	{
		/* line back to top right corner */
		path->line_to(x + x_s23, y + 0);
		/* centre of top right corner */
		path->line_to(x + x_s56, y + y_s16);
		/* centre of top left corner */
		path->line_to(x + x_s16, y + y_s16);
		/* centre of bottom left corner */
		path->line_to(x + x_s16, y + y_s56);
		/* centre of bottom right corner */
		path->line_to(x + x_s56, y + y_s56);
		/* centre of top right corner */
		path->line_to(x + x_s56, y + y_s16);
	}
	/* fill */
	auto [r,g,b] = view.get_worm_rgb(colour, is_materialized);
	s->append_fill(path->to_path (), Gsk::FillRule::EVEN_ODD, {r, g, b, 1.0f});
}
void View::ActiveView::draw_text_target_width(const Glib::RefPtr<Gtk::Snapshot> &snapshot, int x, int y, const Glib::ustring &text, int target_width)
{
	/* draw using x,y as the top left corner of the text */
	intsys target_font_size = 1;
	uintsys target_width_diff = std::numeric_limits<uintsys>::max();
	Pango::Rectangle a = {0,0,0,0};

	for (int font_size = 1;font_size < 200;font_size++)
	{
		auto layout = get_layout(text, font_size);
	    Pango::Rectangle b;
	    layout->get_extents(a, b);
	    uintsys width_diff = abs(target_width - (intsys)a.get_width() / Pango::SCALE);
	    if (width_diff > target_width_diff && width_diff - target_width_diff > 2)
	        break;
	    else if (width_diff < target_width_diff)
	    {
	        target_width_diff = width_diff;
	        target_font_size = font_size;
	    }
	}
	snapshot->save();
	snapshot->translate({x - a.get_x() / Pango::SCALE, y - a.get_y() / Pango::SCALE});
	auto layout = get_layout(text, target_font_size);
	snapshot->append_layout(layout, {1, 1, 1, 1});
	snapshot->restore();
}
Glib::RefPtr<Pango::Layout> View::ActiveView::get_layout(const Glib::ustring &text, uintsys font_size)
{
	auto layout = create_pango_layout(text);
	auto font = layout->get_font_description();
	if(nullptr==font.gobj() || font.get_family().empty())
		font = Pango::FontDescription("Sans Bold 1pt");
	font.set_size(Pango::SCALE * font_size);
	layout->set_font_description(font);
	layout->set_text(text);
	return layout;
}

/*******************************************************************
 *                                                                 *
 *	View::Life                                                     *
 *                                                                 *
 *******************************************************************/
void View::Life::draw_text_target_height(const Glib::RefPtr<Gtk::Snapshot> &snapshot, intsys x, intsys y, const Glib::ustring &text, intsys target_height, intsys center_width)
{
	/* draw using x,y as the top left corner of the text */
	intsys target_font_size = 1;
	uintsys target_height_diff = std::numeric_limits<uintsys>::max();
	Pango::Rectangle a = {0,0,0,0};

	for (intsys font_size = 1;font_size < 200;font_size++)
	{
		auto layout = get_layout(text, font_size);
	    Pango::Rectangle b;
	    layout->get_extents(a, b);
	    uintsys height_diff = abs(target_height - (intsys)a.get_height() / Pango::SCALE);
	    if (height_diff > target_height_diff && height_diff - target_height_diff > 2)
	        break;
	    else if (height_diff < target_height_diff)
	    {
	        target_height_diff = height_diff;
	        target_font_size = font_size;
	    }
	}
	auto width=(intsys)a.get_width() / Pango::SCALE;
	auto x_center_offset = width<center_width ? (16 - width)/2 : 0;
	snapshot->save();
	snapshot->translate({x - a.get_x() / Pango::SCALE + x_center_offset, y - a.get_y() / Pango::SCALE});
	auto layout = get_layout(text, target_font_size);
	snapshot->append_layout(layout, {1, 1, 1, 1});
	snapshot->restore();
}
Glib::RefPtr<Pango::Layout> View::Life::get_layout(const Glib::ustring &text, uintsys font_size)
{
	auto layout = create_pango_layout(text);
	auto font = layout->get_font_description();
	if(nullptr==font.gobj() || font.get_family().empty())
		font = Pango::FontDescription("Sans Bold 1pt");
	font.set_size(Pango::SCALE * font_size);
	layout->set_font_description(font);
	layout->set_text(text);
	return layout;
}

