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
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <stdlib.h>
#include <libgames-support/games-runtime.h>

#include "main.h"
#include "gnibbles.h"
#include "properties.h"
#include "worm-clutter.h"

extern GnibblesProperties *properties;

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

  ClutterActor *actor = clutter_rectangle_new ();

  if (worm->direction == WORMRIGHT || worm->direction == WORMLEFT) {
    clutter_actor_set_size (CLUTTER_ACTOR (actor), 
                            SLENGTH * properties->tilesize, 
                            properties->tilesize);
   
  } else if (worm->direction == WORMDOWN || worm->direction == WORMUP) {
    clutter_actor_set_size (CLUTTER_ACTOR (actor),  
                            properties->tilesize,
                            SLENGTH * properties->tilesize);
  }

  clutter_actor_set_position (CLUTTER_ACTOR (actor), worm->xstart, worm->ystart);
  clutter_container_add_actor (CLUTTER_CONTAINER (worm->actors), actor);  
  
  worm->list = g_list_append (worm->list, actor);
  return worm;
}

void
gnibbles_cworm_add_straight_actor (GnibblesCWorm *worm)
{
  ClutterActor *straight = clutter_rectangle_new ();
  
  if (worm->direction == WORMRIGHT || worm->direction == WORMLEFT)
    clutter_actor_set_size (straight, properties->tilesize ,0);
  else if (worm->direction == WORMDOWN || worm->direction == WORMUP)
    clutter_actor_set_size (straight, 0, properties->tilesize);

  if (!worm->inverse)
    worm->list = g_list_append (worm->list, straight);
  else
    worm->list = g_list_prepend (worm->list, straight);
  
  clutter_container_add_actor (CLUTTER_CONTAINER (worm->actors), straight);

  //TODO: start increasing the size of the actor
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


