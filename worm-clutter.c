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

  gnibbles_cworm_add_straight_actor (worm, SLENGTH);

  return worm;
}

void
gnibbles_cworm_add_straight_actor (GnibblesCWorm *worm, gint size)
{
  ClutterScript *script = NULL;
  ClutterActor *actor = NULL;

  gchar worm_script[300];  

  if (worm->direction == WORMRIGHT || worm->direction == WORMLEFT) {
    g_sprintf (worm_script,  "["
                             "  {"
                             "    \"id\" : \"worm\","
                             "    \"type\" : \"ClutterTexture\","
                             "    \"x\" : %d,"
                             "    \"y\" : %d,"
                             "    \"width\" : %d,"
                             "    \"height\" : %d,"
                             "    \"keep-aspect-ratio\" : true,"
                             "    \"visible\" : true,"
                             "    \"repeat-x\" : true,"
                             "    \"repeat-y\" : false,"
                             "  }"
                             "]",
                             worm->xstart,
                             worm->ystart,
                             size * (2 * properties->tilesize),
                             2 * properties->tilesize);

  } else if (worm->direction == WORMDOWN || worm->direction == WORMUP) {
    g_sprintf (worm_script,  "["
                             "  {"
                             "    \"id\" : \"worm\","
                             "    \"type\" : \"ClutterTexture\","
                             "    \"x\" : %d,"
                             "    \"y\" : %d,"
                             "    \"width\" : %d,"
                             "    \"height\" : %d,"
                             "    \"keep-aspect-ratio\" : true,"
                             "    \"visible\" : true,"
                             "    \"repeat-x\" : false,"
                             "    \"repeat-y\" : true,"
                             "  }"
                             "]",
                             worm->xstart,
                             worm->ystart,
                             2 * properties->tilesize,
                             size * (2 * properties->tilesize));

  }

  script = clutter_script_new ();

  clutter_script_load_from_data (script, worm_script, -1, NULL);
  clutter_script_get_objects (script, "worm", &actor, NULL);

  gtk_clutter_texture_set_from_pixbuf (CLUTTER_TEXTURE (actor), worm_pixmaps[0]);

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


