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

#include <config.h>
#include <gnome.h>
#include <gconf/gconf-client.h>

#include "properties.h"

#define KEY_DIR "/apps/gnibbles"
#define KEY_NUM_WORMS "/apps/gnibbles/preferences/worms_number"
#define KEY_SPEED "/apps/gnibbles/preferences/speed"
#define KEY_FAKES "/apps/gnibbles/preferences/fakes"
#define KEY_RANDOM "/apps/gnibbles/preferences/random"
#define KEY_START_LEVEL "/apps/gnibbles/preferences/start_level"
#define KEY_SOUND "/apps/gnibbles/preferences/sound"
#define KEY_TILE_SIZE "/apps/gnibbles/preferences/tile_size"

#define KEY_WORM_COLOR "/apps/gnibbles/worm/%d/color"
#define KEY_WORM_REL_MOVE "/apps/gnibbles/worm/%d/move_relative"
#define KEY_WORM_UP "/apps/gnibbles/worm/%d/key_up"
#define KEY_WORM_DOWN "/apps/gnibbles/worm/%d/key_down"
#define KEY_WORM_LEFT "/apps/gnibbles/worm/%d/key_left"
#define KEY_WORM_RIGHT "/apps/gnibbles/worm/%d/key_right"

static GConfClient *conf_client;

GnibblesProperties *
gnibbles_properties_new ()
{
	GnibblesProperties *tmp;
	gint i;
	gchar buffer[256];

	conf_client = gconf_client_get_default ();

	tmp = (GnibblesProperties *) g_malloc (sizeof (GnibblesProperties));

	tmp->numworms = gconf_client_get_int (conf_client,
					      KEY_NUM_WORMS,
					      NULL);
	if (tmp->numworms < 1)
		tmp->numworms = 1;

	tmp->gamespeed = gconf_client_get_int (conf_client,
					       KEY_SPEED,
					       NULL);
	if (tmp->gamespeed < 1)
		tmp->gamespeed = 2;

	tmp->fakes = gconf_client_get_bool (conf_client,
					    KEY_FAKES,
					    NULL);

	tmp->random = gconf_client_get_bool (conf_client,
					     KEY_RANDOM,
					     NULL);
	tmp->startlevel = gconf_client_get_int (conf_client,
						KEY_START_LEVEL,
						NULL);
	if (tmp->startlevel < 1)
		tmp->startlevel = 1;

	tmp->sound = gconf_client_get_bool (conf_client,
					    KEY_SOUND,
					    NULL);
	tmp->tilesize = gconf_client_get_int (conf_client,
					      KEY_TILE_SIZE,
					      NULL);
	if (tmp->tilesize < 1)
		tmp->tilesize = 5;

	for (i = 0; i < NUMWORMS; i++) {
		tmp->wormprops[i] = (GnibblesWormProps *) g_malloc (sizeof
								    (GnibblesWormProps));

		sprintf (buffer, KEY_WORM_COLOR, i);
		tmp->wormprops[i]->color = gconf_client_get_int (conf_client,
								 buffer,
								 NULL);
		if (tmp->wormprops[i]->color < 1)
			tmp->wormprops[i]->color = (i % 7) + 12;

		sprintf (buffer, KEY_WORM_REL_MOVE, i);
		tmp->wormprops[i]->relmove = gconf_client_get_bool (conf_client,
								    buffer,
								    NULL);
		sprintf (buffer, KEY_WORM_UP, i);
		tmp->wormprops[i]->up = gconf_client_get_int (conf_client,
							      buffer,
							      NULL);
		if (!tmp->wormprops[i]->up)
			tmp->wormprops[i]->up = GDK_Up;

		sprintf (buffer, KEY_WORM_DOWN, i);
		tmp->wormprops[i]->down = gconf_client_get_int (conf_client,
								buffer,
								NULL);
		if (!tmp->wormprops[i]->down)
			tmp->wormprops[i]->down = GDK_Down;

		sprintf (buffer, KEY_WORM_LEFT, i);
		tmp->wormprops[i]->left = gconf_client_get_int (conf_client,
								buffer,
								NULL);
		if (!tmp->wormprops[i]->left)
			tmp->wormprops[i]->left = GDK_Left;

		sprintf (buffer, KEY_WORM_RIGHT, i);
		tmp->wormprops[i]->right = gconf_client_get_int (conf_client,
								 buffer,
								 NULL);
		if (!tmp->wormprops[i]->right)
			tmp->wormprops[i]->right = GDK_Right;
	}

	return (tmp);
}

void
gnibbles_properties_destroy (GnibblesProperties *props)
{
	int i;

	for (i = 0; i < NUMWORMS; i++)
		free (props->wormprops[i]);
	
	free (props);
}

GnibblesProperties *
gnibbles_properties_copy (GnibblesProperties *props)
{
	GnibblesProperties *tmp;
	int i;

	tmp = (GnibblesProperties *) g_malloc (sizeof (GnibblesProperties));

	tmp->numworms = props->numworms;
	tmp->gamespeed = props->gamespeed;
	tmp->fakes = props->fakes;
	tmp->random = props->random;
	tmp->startlevel = props->startlevel;
	tmp->sound = props->sound;
	tmp->tilesize = props->tilesize;
	
	for (i = 0; i < NUMWORMS; i++) {
		tmp->wormprops[i] = (GnibblesWormProps *) g_malloc (sizeof
				(GnibblesWormProps));

		tmp->wormprops[i]->color = props->wormprops[i]->color;
		tmp->wormprops[i]->relmove = props->wormprops[i]->relmove;
		tmp->wormprops[i]->up = props->wormprops[i]->up;
		tmp->wormprops[i]->down = props->wormprops[i]->down;
		tmp->wormprops[i]->left = props->wormprops[i]->left;
		tmp->wormprops[i]->right = props->wormprops[i]->right;
	}

	return (tmp);
}

/* FIXME: I suppose these set functions should be combined somehow */

void
gnibbles_properties_set_worms_number (gint value)
{
	gconf_client_set_int (conf_client, KEY_NUM_WORMS, value, NULL);
}
void
gnibbles_properties_set_speed (gint value)
{
	gconf_client_set_int (conf_client, KEY_SPEED, value, NULL);
}
void
gnibbles_properties_set_fakes (gboolean value)
{
	gconf_client_set_bool (conf_client, KEY_FAKES, value, NULL);
}
void
gnibbles_properties_set_random (gboolean value)
{
	gconf_client_set_bool (conf_client, KEY_RANDOM, value, NULL);
}
void
gnibbles_properties_set_start_level (gint value)
{
	gconf_client_set_int (conf_client, KEY_START_LEVEL, value, NULL);
}
void
gnibbles_properties_set_sound (gboolean value)
{
	gconf_client_set_bool (conf_client, KEY_SOUND, value, NULL);
}
void
gnibbles_properties_set_tile_size (gint value)
{
	gconf_client_set_int (conf_client, KEY_TILE_SIZE, value, NULL);
}
void
gnibbles_properties_set_worm_relative_movement (gint i, gboolean value)
{
	gchar *buffer;
	buffer = g_strdup_printf (KEY_WORM_REL_MOVE, i, value);
	gconf_client_set_bool (conf_client, buffer, value, NULL);
	g_free (buffer);
}
void
gnibbles_properties_set_worm_color (gint i, gint value)
{
	gchar *buffer;
	buffer = g_strdup_printf (KEY_WORM_COLOR, i, value);
	gconf_client_set_int (conf_client, buffer, value, NULL);
	g_free (buffer);
}
void
gnibbles_properties_set_worm_up (gint i, gint value)
{
	gchar *buffer;
	buffer = g_strdup_printf (KEY_WORM_UP, i, value);
	gconf_client_set_int (conf_client, buffer, value, NULL);
	g_free (buffer);
}
void
gnibbles_properties_set_worm_down (gint i, gint value)
{
	gchar *buffer;
	buffer = g_strdup_printf (KEY_WORM_DOWN, i, value);
	gconf_client_set_int (conf_client, buffer, value, NULL);
	g_free (buffer);
}
void
gnibbles_properties_set_worm_left (gint i, gint value)
{
	gchar *buffer;
	buffer = g_strdup_printf (KEY_WORM_LEFT, i, value);
	gconf_client_set_int (conf_client, buffer, value, NULL);
	g_free (buffer);
}
void
gnibbles_properties_set_worm_right (gint i, gint value)
{
	gchar *buffer;
	buffer = g_strdup_printf (KEY_WORM_RIGHT, i, value);
	gconf_client_set_int (conf_client, buffer, value, NULL);
	g_free (buffer);
}

void
gnibbles_properties_save (GnibblesProperties *props)
{
#if 0
	gchar buffer[255];
	int i;
	gnome_config_set_int ("/gnibbles/Preferences/numworms",
			props->numworms);
	gnome_config_set_int ("/gnibbles/Preferences/gamespeed",
			props->gamespeed);
	gnome_config_set_int ("/gnibbles/Preferences/fakes", props->fakes);
	gnome_config_set_int ("/gnibbles/Preferences/random", props->random);
	gnome_config_set_int ("/gnibbles/Preferences/startlevel",
			props->startlevel);
	gnome_config_set_int ("/gnibbles/Preferences/sound", props->sound);
	gnome_config_set_int ("/gnibbles/Preferences/tilesize", props->tilesize);

	for (i = 0; i < NUMWORMS; i++) {
		sprintf (buffer, "/gnibbles/Worm%d/color", i);
		gnome_config_set_int (buffer, props->wormprops[i]->color);
		sprintf (buffer, "/gnibbles/Worm%d/relmove", i);
		gnome_config_set_int (buffer, props->wormprops[i]->relmove);
		sprintf (buffer, "/gnibbles/Worm%d/up", i);
		gnome_config_set_int (buffer, props->wormprops[i]->up);
		sprintf (buffer, "/gnibbles/Worm%d/down", i);
		gnome_config_set_int (buffer, props->wormprops[i]->down);
		sprintf (buffer, "/gnibbles/Worm%d/left", i);
		gnome_config_set_int (buffer, props->wormprops[i]->left);
		sprintf (buffer, "/gnibbles/Worm%d/right", i);
		gnome_config_set_int (buffer, props->wormprops[i]->right);
	}

	gnome_config_sync ();
#endif
}
