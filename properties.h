/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* 
 *   Gnome Nibbles: Gnome Worm Game
 *   Written by Sean MacIsaac <sjm@acm.org>, Ian Peters <itp@gnu.org>
 * 
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _PROPERTIES_H_
#define _PROPERTIES_H_

#include <config.h>
#include <gnome.h>

#include "gnibbles.h"

#define KEY_DIR "/apps/gnibbles"
#define KEY_PREFERENCES_DIR "/apps/gnibbles/preferences"
#define KEY_NUM_WORMS "/apps/gnibbles/preferences/players"
#define KEY_NUM_AI "/apps/gnibbles/preferences/ai"
#define KEY_SPEED "/apps/gnibbles/preferences/speed"
#define KEY_FAKES "/apps/gnibbles/preferences/fakes"
#define KEY_RANDOM "/apps/gnibbles/preferences/random"
#define KEY_START_LEVEL "/apps/gnibbles/preferences/start_level"
#define KEY_SOUND "/apps/gnibbles/preferences/sound"
#define KEY_TILE_SIZE "/apps/gnibbles/preferences/tile_size"
#define KEY_HEIGHT "/apps/gnibbles/preferences/height"
#define KEY_WIDTH "/apps/gnibbles/preferences/width"

#define KEY_WORM_DIR "/apps/gnibbles/preferences/worm/%d"
#define KEY_WORM_COLOR "/apps/gnibbles/preferences/worm/%d/color"
#define KEY_WORM_REL_MOVE "/apps/gnibbles/preferences/worm/%d/move_relative"
#define KEY_WORM_UP "/apps/gnibbles/preferences/worm/%d/key_up"
#define KEY_WORM_DOWN "/apps/gnibbles/preferences/worm/%d/key_down"
#define KEY_WORM_LEFT "/apps/gnibbles/preferences/worm/%d/key_left"
#define KEY_WORM_RIGHT "/apps/gnibbles/preferences/worm/%d/key_right"

typedef struct {
  gint color;
  gboolean relmove;
  gchar *up, *down, *left, *right;
} GnibblesWormProps;

typedef struct {
  gint numworms;
  gint human;
  gint ai;
  gint gamespeed;
  gint fakes;
  gint random;
  gint startlevel;
  gint sound;
  gint tilesize;
  gint height;
  gint width;
  GnibblesWormProps *wormprops[NUMWORMS];
} GnibblesProperties;

GnibblesProperties *gnibbles_properties_new (void);

void gnibbles_properties_update (GnibblesProperties * tmp);

void gnibbles_properties_destroy (GnibblesProperties * props);

GnibblesProperties *gnibbles_properties_copy (GnibblesProperties * props);

void gnibbles_properties_set_worms_number (gint value);
void gnibbles_properties_set_ai_number (gint value);
void gnibbles_properties_set_speed (gint value);
void gnibbles_properties_set_fakes (gboolean value);
void gnibbles_properties_set_random (gboolean value);
void gnibbles_properties_set_start_level (gint value);
void gnibbles_properties_set_sound (gboolean value);
void gnibbles_properties_set_tile_size (gint value);
void gnibbles_properties_set_worm_relative_movement (gint i, gboolean value);
void gnibbles_properties_set_worm_color (gint i, gint value);
void gnibbles_properties_set_worm_up (gint i, gchar * value);
void gnibbles_properties_set_worm_down (gint i, gchar * value);
void gnibbles_properties_set_worm_left (gint i, gchar * value);
void gnibbles_properties_set_worm_right (gint i, gchar * value);
void gnibbles_properties_set_height (gint value);
void gnibbles_properties_set_width (gint value);

void gnibbles_properties_save (GnibblesProperties * props);
gchar *colorval_name (gint colorval);

#endif
