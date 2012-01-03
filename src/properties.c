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

#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <libgames-support/games-scores.h>

#include "properties.h"
#include "main.h"
#include "sound.h"

#define MAX_SPEED 4

typedef struct _ColorLookup ColorLookup;

struct _ColorLookup {
  gint colorval;
  gchar *name;
};

ColorLookup color_lookup[NUM_COLORS] = {
  {WORMRED, "red"},
  {WORMGREEN, "green"},
  {WORMBLUE, "blue"},
  {WORMYELLOW, "orange"},
  {WORMCYAN, "cyan"},
  {WORMPURPLE, "purple"},
  {WORMGRAY, "gray"}
};

static gint
colorval_from_name (gchar * name)
{
  gint i;

  for (i = 0; i < NUM_COLORS; i++)
    if (!strcmp (name, color_lookup[i].name))
      return color_lookup[i].colorval;

  return 0;
}

gchar *
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
  gchar *category;
  gchar *color_name;

  tmp->human = g_settings_get_integer (settings, KEY_NUM_WORMS, NULL);
  if (tmp->human < 0)
    tmp->human = 0;
  else if (tmp->human > NUMWORMS)
    tmp->human = NUMWORMS;

  tmp->ai = g_settings_get_integer (settings, KEY_NUM_AI, NULL);
  if (tmp->ai < 0)
    tmp->ai = 0;
  else if (tmp->ai > NUMWORMS)
    tmp->ai = NUMWORMS;

  tmp->numworms = tmp->human + tmp->ai;

  tmp->gamespeed = g_settings_get_integer (settings,
                                           KEY_SPEED, NULL);
  if (tmp->gamespeed < 1)
    tmp->gamespeed = 2;
  else if (tmp->gamespeed > MAX_SPEED)
    tmp->gamespeed = MAX_SPEED;

  tmp->fakes = g_settings_get_boolean (settings,
                                       KEY_FAKES, NULL);

  tmp->random = g_settings_get_boolean (settings,
                                        KEY_RANDOM, NULL);

  tmp->startlevel = g_settings_get_integer (settings,
                                            KEY_START_LEVEL, NULL);
  if (tmp->startlevel < 1)
    tmp->startlevel = 1;
  if (tmp->startlevel > MAXLEVEL)
    tmp->startlevel = MAXLEVEL;

  tmp->sound = g_settings_get_boolean (settings, KEY_SOUND, NULL);
  sound_enable (tmp->sound);

  tmp->tilesize = g_settings_get_integer (settings,
                                          KEY_TILE_SIZE, NULL);
  if (tmp->tilesize < 1)
    tmp->tilesize = 5;
  if (tmp->tilesize > 30)
    tmp->tilesize = 30;

  for (i = 0; i < NUMWORMS; i++) {
    tmp->wormprops[i] = g_slice_new0 (GnibblesWormProps);

    g_snprintf (buffer, sizeof (buffer), KEY_WORM_COLOR, i);
    color_name = g_settings_get_string_with_default (settings,
                                                     buffer, "red");
    tmp->wormprops[i]->color = colorval_from_name (color_name);
    g_free (color_name);

    if (tmp->wormprops[i]->color < 1)
      tmp->wormprops[i]->color = (i % NUM_COLORS) + WORMRED;
    if (tmp->wormprops[i]->color > WORMRED + NUM_COLORS)
      tmp->wormprops[i]->color = (i % NUM_COLORS) + WORMRED;

    g_snprintf (buffer, sizeof (buffer), KEY_WORM_REL_MOVE, i);
    tmp->wormprops[i]->relmove = g_settings_get_boolean (settings, buffer);

    g_snprintf (buffer, sizeof (buffer), KEY_WORM_UP, i);
    tmp->wormprops[i]->up = g_settings_get_int (settings, buffer);

    g_snprintf (buffer, sizeof (buffer), KEY_WORM_DOWN, i);
    tmp->wormprops[i]->down = g_settings_get_int (settings, buffer);

    g_snprintf (buffer, sizeof (buffer), KEY_WORM_LEFT, i);
    tmp->wormprops[i]->left = g_settings_get_int (settings, buffer);

    g_snprintf (buffer, sizeof (buffer), KEY_WORM_RIGHT, i);
    tmp->wormprops[i]->right = g_settings_get_int (settings, buffer);
  }

  category = g_strdup_printf ("%d.%d", tmp->gamespeed, tmp->fakes);
  games_scores_set_category (highscores, category);
  g_free (category);
}

static void
conf_value_changed_cb (GamesConf *conf,
                       const char *group,
                       const char *key,
                       gpointer data)
{
  GnibblesProperties *props = (GnibblesProperties *) data;

  if (!group || strcmp (group, settings) != 0)
    return;

  gnibbles_properties_update (props);
}

GnibblesProperties *
gnibbles_properties_new (void)
{
  GnibblesProperties *props;

  props = g_slice_new0 (GnibblesProperties);

  props->conf_notify_id = g_signal_connect (g_settings_get_default (),
                                            "value-changed",
                                            G_CALLBACK (conf_value_changed_cb),
                                            props);

  gnibbles_properties_update (props);

  return props;
}

void
gnibbles_properties_destroy (GnibblesProperties * props)
{
  int i;

  for (i = 0; i < NUMWORMS; i++)
    g_slice_free (GnibblesWormProps, props->wormprops[i]);

  g_signal_handler_disconnect (g_settings_get_default (),
                               props->conf_notify_id);

  g_slice_free (GnibblesProperties, props);
}

/* FIXME: I suppose these set functions should be combined somehow */

void
gnibbles_properties_set_worms_number (gint value)
{
  g_settings_set_integer (settings, KEY_NUM_WORMS, value);
}

void
gnibbles_properties_set_ai_number (gint value)
{
  g_settings_set_integer (settings, KEY_NUM_AI, value);
}

void
gnibbles_properties_set_speed (gint value)
{
  g_settings_set_integer (settings, KEY_SPEED, value);
}

void
gnibbles_properties_set_fakes (gboolean value)
{
  g_settings_set_boolean (settings, KEY_FAKES, value);
}

void
gnibbles_properties_set_random (gboolean value)
{
  g_settings_set_boolean (settings, KEY_RANDOM, value);
}

void
gnibbles_properties_set_start_level (gint value)
{
  g_settings_set_integer (settings, KEY_START_LEVEL, value);
}

void
gnibbles_properties_set_sound (gboolean value)
{
  g_settings_set_boolean (settings, KEY_SOUND, value);
}

void
gnibbles_properties_set_tile_size (gint value)
{
  g_settings_set_integer (settings, KEY_TILE_SIZE, value);
}

void
gnibbles_properties_set_worm_relative_movement (gint i, gboolean value)
{
  char key[64];
  g_snprintf (key, sizeof (key), KEY_WORM_REL_MOVE, i);
  g_settings_set_boolean (settings, key, value);
}

void
gnibbles_properties_set_worm_color (gint i, gint value)
{
  char key[64];
  char *color_name;

  g_snprintf (key, sizeof (key), KEY_WORM_COLOR, i);

  color_name = colorval_name (value);
  g_settings_set_string (settings, key, color_name);
}

void
gnibbles_properties_save (GnibblesProperties * props)
{
}
