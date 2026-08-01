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

#if !defined(GETTEXT_PACKAGE)
	#define GETTEXT_PACKAGE "gnome-nibbles"
#endif

/* Translators: name of the program, as seen in the headerbar, in GNOME Shell, or in the about dialog */
#define PROGRAM_NAME _("Nibbles")

#define APP_NAME			"org.gnome.Nibbles"

/* Command line arguments */
#define VERSION_ARGUMENT	"version"
#define LEVEL_ARGUMENT		"level"
#define PROGRESS_ARGUMENT	"progress"
#define NIBBLES_ARGUMENT	"nibbles"
#define PLAYERS_ARGUMENT	"players"
#define SPEED_ARGUMENT		"speed"
#define DISABLE_FAKES_ARGUMENT "disable-fakes"
#define ENABLE_FAKES_ARGUMENT "enable-fakes"
#define MUTE_ARGUMENT		"mute"
#define UNMUTE_ARGUMENT		"unmute"
#define THREE_DIMENSIONAL_ARGUMENT "3D"
#define TWO_DIMENSIONAL_ARGUMENT "2D"
#define START_ARGUMENT		"start"

/* Settings */
#define PLAYER_SETTINGS		PLAYERS_ARGUMENT
#define SPEED_SETTINGS		SPEED_ARGUMENT
#define AI_SETTINGS			"ai"
#define FAKE_SETTINGS		"fakes"
#define SOUND_SETTINGS		"sound"
#define THREE_DIMENSIONAL_SETTINGS "three-dimensional-view"
#define PROGRESS_SETTINGS	PROGRESS_ARGUMENT
#define LEVEL_SETTINGS		"start-level"
#define WORM_BASE_KEY		"org.gnome.Nibbles.worm"

static const char *worm_colour_string[]={"red","green","blue","yellow","cyan","purple"};

enum eWormColour {
	red_worm,green_worm,blue_worm,yellow_worm,
	cyan_worm,purple_worm,unknown_colour_worm};
	
inline eWormColour& operator++(eWormColour& c)
{
    if(eWormColour::purple_worm==c || eWormColour::unknown_colour_worm==c)
    	c=red_worm;
    else
	    c=static_cast<eWormColour>(static_cast<int>(c) + 1);
    return c;
}

/* utility functions */
eWormColour get_worm_settings_colour(unsigned long worm_id);
void set_worm_settings_colour(unsigned long worm_id,eWormColour colour);

struct WormScore
{
	eWormColour colour;
	unsigned long score;
};



