/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* 
 *   Gnome Nibbles: Gnome Worm Game
 *   Written by Sean MacIsaac <sjm@acm.org>, Ian Peters <itp@gnu.org>
 *              Guillaume Béland <guillaume.beland@gmail.com>
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
#include <glib/gprintf.h>

#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <stdlib.h>
#include <libgames-support/games-runtime.h>
#include <clutter-gtk/clutter-gtk.h>
#include "main.h"
#include "gnibbles.h"
#include "properties.h"
#include "worm-clutter.h"

extern GnibblesProperties *properties;
extern GdkPixbuf *worm_pixmaps[];

GnibblesCWorm*
gnibbles_cworm_new (guint number, gint x_s, gint y_s)
{
  GnibblesCWorm *worm = g_new (GnibblesCWorm, 1);
  
  worm->actors = clutter_group_new ();
  worm->list = NULL;
  worm->number = number;
  worm->lives = SLIVES;
  worm->direction = 1;
  worm->inverse = FALSE;
  worm->xstart = x_s;
  worm->ystart = y_s;

  gnibbles_cworm_add_straight_actor (worm, 30);

  return worm;
}

void
gnibbles_cworm_add_straight_actor (GnibblesCWorm *worm, gint size)
{
  ClutterActor *actor = NULL;
  GValue val = {0,};

  actor = gtk_clutter_texture_new_from_pixbuf (worm_pixmaps[0]);

  g_value_init (&val, G_TYPE_BOOLEAN);
  g_value_set_boolean ( &val, TRUE);

  clutter_actor_set_position (CLUTTER_ACTOR (actor),
                              worm->xstart,
                              worm->ystart);
  g_object_set_property (G_OBJECT (actor), "keep-aspect-ratio", &val);

  if (worm->direction == WORMRIGHT || worm->direction == WORMLEFT) {
    clutter_actor_set_size (CLUTTER_ACTOR (actor),
                          properties->tilesize * size,
                          properties->tilesize);
    g_object_set_property (G_OBJECT (actor), "repeat-x", &val);
  } else if (worm->direction == WORMDOWN || worm->direction == WORMUP) {
    clutter_actor_set_size (CLUTTER_ACTOR (actor),
                          properties->tilesize,
                          properties->tilesize * size);
    g_object_set_property (G_OBJECT (actor), "repeat-y", &val);
  }

  clutter_container_add_actor (CLUTTER_CONTAINER (worm->actors), actor);  
  
  if (!worm->inverse)
    worm->list = g_list_append (worm->list, actor);
  else
    worm->list = g_list_prepend (worm->list, actor);
  
  //TODO: connect/timeline: start increasing the size of the actor
}

void
gnibbles_cworm_add_corner_actor (GnibblesCWorm *worm)
{
  //TODO: rounded corner
  ClutterActor *corner = clutter_rectangle_new ();

  //TODO: switch to determine how the corner is rounded
  switch (worm->direction)
  {
    case WORMRIGHT:
      break;
    case WORMLEFT:
      break;
    case WORMDOWN:
      break;
    case WORMUP:
      break;
    default:
      clutter_actor_set_size (corner, properties->tilesize, properties->tilesize);
      break;
  }

  if (!worm->inverse)
    worm->list = g_list_append (worm->list, corner);
  else
    worm->list = g_list_prepend (worm->list, corner);

  clutter_container_add_actor (CLUTTER_CONTAINER (worm->actors), corner);
}

void
gnibbles_cworm_remove_actor (GnibblesCWorm *worm)
{
  if (!worm->inverse)
    worm->list = g_list_remove_link (worm->list, g_list_first (worm->list));
  else 
    worm->list = g_list_remove_link (worm->list, g_list_last (worm->list));
}


