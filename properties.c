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
#include <string.h>
#include <gconf/gconf-client.h>

#include "properties.h"

#define MAX_SPEED 4

static GConfClient *conf_client;

typedef struct _ColorLookup ColorLookup;

struct _ColorLookup {
  gint   colorval;
  gchar *name;
};

ColorLookup color_lookup[NUM_COLORS] = {
	{WORMRED, "red"},
	{WORMGREEN, "green"},
	{WORMBLUE, "blue"},
	{WORMYELLOW, "yellow"},
	{WORMCYAN, "cyan"},
	{WORMPURPLE, "purple"},
	{WORMGRAY, "gray"}
};

static gint
colorval_from_name (gchar *name)
{
	gint i;
	
	for (i = 0; i < NUM_COLORS; i++)
		if (!strcmp (name, color_lookup[i].name))
			return color_lookup[i].colorval;
	
	return 0;
}

static gchar *
colorval_name (gint colorval)
{
	gint i;
	
	for (i = 0; i < NUM_COLORS; i++)
		if (colorval == color_lookup[i].colorval)
			return color_lookup[i].name;
	
	return "unknown";
}

void
gnibbles_properties_update (GnibblesProperties * tmp)
{
	gint i;
	gchar buffer[256];
	gchar *color_name;

	conf_client = gconf_client_get_default ();

	tmp->numworms = gconf_client_get_int (conf_client,
					      KEY_NUM_WORMS,
					      NULL);
	if (tmp->numworms < 1)
		tmp->numworms = 1;
	else if (tmp->numworms > NUMWORMS)
		tmp->numworms = NUMWORMS;

	tmp->gamespeed = gconf_client_get_int (conf_client,
					       KEY_SPEED,
					       NULL);
	if (tmp->gamespeed < 1)
		tmp->gamespeed = 2;
	else if (tmp->gamespeed > MAX_SPEED)
		tmp->gamespeed = MAX_SPEED;

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
	if (tmp->startlevel > MAXLEVEL)
		tmp->startlevel = MAXLEVEL;

	tmp->sound = gconf_client_get_bool (conf_client,
					    KEY_SOUND,
					    NULL);
	tmp->tilesize = gconf_client_get_int (conf_client,
					      KEY_TILE_SIZE,
					      NULL);
	if (tmp->tilesize < 1)
		tmp->tilesize = 5;
	if (tmp->tilesize > 30)
		tmp->tilesize = 30;

	for (i = 0; i < NUMWORMS; i++) {
		tmp->wormprops[i] = (GnibblesWormProps *) g_malloc (sizeof
								    (GnibblesWormProps));

		sprintf (buffer, KEY_WORM_COLOR, i);
		color_name = gconf_client_get_string (conf_client,
						      buffer,
						      NULL);
		if (color_name == NULL)
			color_name = g_strdup ("red");
		tmp->wormprops[i]->color = colorval_from_name (color_name);
		g_free (color_name);

		if (tmp->wormprops[i]->color < 1)
			tmp->wormprops[i]->color = (i % NUM_COLORS) + WORMRED;
		if (tmp->wormprops[i]->color > WORMRED + NUM_COLORS)
			tmp->wormprops[i]->color = (i % NUM_COLORS) + WORMRED;

		sprintf (buffer, KEY_WORM_REL_MOVE, i);
		tmp->wormprops[i]->relmove = gconf_client_get_bool (conf_client,
								    buffer,
								    NULL);
		sprintf (buffer, KEY_WORM_UP, i);
		tmp->wormprops[i]->up = gconf_client_get_string (conf_client,
								 buffer,
								 NULL);
		if (!tmp->wormprops[i]->up)
			tmp->wormprops[i]->up = gdk_keyval_name (GDK_Up);

		sprintf (buffer, KEY_WORM_DOWN, i);
		tmp->wormprops[i]->down = gconf_client_get_string (conf_client,
								   buffer,
								   NULL);
		if (!tmp->wormprops[i]->down)
			tmp->wormprops[i]->down = gdk_keyval_name (GDK_Down);

		sprintf (buffer, KEY_WORM_LEFT, i);
		tmp->wormprops[i]->left = gconf_client_get_string (conf_client,
								   buffer,
								   NULL);
		if (!tmp->wormprops[i]->left)
			tmp->wormprops[i]->left = gdk_keyval_name (GDK_Left);

		sprintf (buffer, KEY_WORM_RIGHT, i);
		tmp->wormprops[i]->right = gconf_client_get_string (conf_client,
								    buffer,
								    NULL);
		if (!tmp->wormprops[i]->right)
			tmp->wormprops[i]->right = gdk_keyval_name (GDK_Right);
	}

}

GnibblesProperties *
gnibbles_properties_new (void)
{
	GnibblesProperties *tmp;

	tmp = (GnibblesProperties *) g_malloc (sizeof (GnibblesProperties));

	gnibbles_properties_update (tmp);

	return tmp;
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
	buffer = g_strdup_printf (KEY_WORM_REL_MOVE, i);
	gconf_client_set_bool (conf_client, buffer, value, NULL);
	g_free (buffer);
}
void
gnibbles_properties_set_worm_color (gint i, gint value)
{
	gchar *buffer;
	gchar *color_name;
	
	color_name = colorval_name (value);
	buffer = g_strdup_printf (KEY_WORM_COLOR, i);
	gconf_client_set_string (conf_client, buffer, color_name, NULL);
	g_free (buffer);
}
void
gnibbles_properties_set_worm_up (gint i, gchar *value)
{
	gchar *buffer;
	buffer = g_strdup_printf (KEY_WORM_UP, i);
	gconf_client_set_string (conf_client, buffer, value, NULL);
	g_free (buffer);
}
void
gnibbles_properties_set_worm_down (gint i, gchar *value)
{
	gchar *buffer;
	buffer = g_strdup_printf (KEY_WORM_DOWN, i);
	gconf_client_set_string (conf_client, buffer, value, NULL);
	g_free (buffer);
}
void
gnibbles_properties_set_worm_left (gint i, gchar *value)
{
	gchar *buffer;
	buffer = g_strdup_printf (KEY_WORM_LEFT, i);
	gconf_client_set_string (conf_client, buffer, value, NULL);
	g_free (buffer);
}
void
gnibbles_properties_set_worm_right (gint i, gchar *value)
{
	gchar *buffer;
	buffer = g_strdup_printf (KEY_WORM_RIGHT, i);
	gconf_client_set_string (conf_client, buffer, value, NULL);
	g_free (buffer);
}

void
gnibbles_properties_save (GnibblesProperties *props)
{
}
