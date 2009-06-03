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
gnibbles_cworm_new (guint number, guint t_xhead,
			                    guint t_yhead, gint t_direction)
{
  GnibblesCWorm *worm = g_new (GnibblesCWorm, 1);
  
  worm->actors = clutter_group_new ();
  worm->list = NULL;
  worm->number = number;
  worm->lives = SLIVES;
  worm->direction = 1;
  worm->inverse = FALSE;

  worm->xhead = t_xhead;
  worm->xstart = t_xhead;
  worm->yhead = t_yhead;
  worm->ystart = t_yhead;
  worm->direction = t_direction;
  worm->direction_start = t_direction;

  gnibbles_cworm_add_straight_actor (worm);

  return worm;
}

void
gnibbles_cworm_add_straight_actor (GnibblesCWorm *worm)
{
  ClutterActor *actor = NULL;
  GValue val = {0,};
  gint size;

  actor = gtk_clutter_texture_new_from_pixbuf (worm_pixmaps[worm->number]);

  g_value_init (&val, G_TYPE_BOOLEAN);
  g_value_set_boolean (&val, TRUE);

  clutter_actor_set_position (CLUTTER_ACTOR (actor),
                              worm->xhead * properties->tilesize,
                              worm->yhead * properties->tilesize);

  g_object_set_property (G_OBJECT (actor), "keep-aspect-ratio", &val);

  ClutterActor *tmp = NULL;

  if (worm->list) {
    if (worm->inverse)
      tmp = (g_list_first (worm->list))->data;
    else 
      tmp = (g_list_last (worm->list))->data;
  } else {
    size = SLENGTH; 
  }

  if (tmp) {
    guint w,h;
    clutter_actor_get_size (CLUTTER_ACTOR (tmp), &w, &h);
    size = w < h ? h : w;
    size = size / properties->tilesize;
  }
  
  if (worm->direction == WORMRIGHT || worm->direction == WORMLEFT) {

    if (worm->direction == WORMRIGHT) {
      worm->yhead += properties->tilesize;
      worm->xhead += (properties->tilesize * size) - properties->tilesize;
    } else {
      worm->yhead -= properties->tilesize;
      worm->xhead -= (properties->tilesize * size) - properties->tilesize;
    }

    if (!tmp)
      clutter_actor_set_size (CLUTTER_ACTOR (actor),
                              properties->tilesize * size,
                              properties->tilesize);
    else
      clutter_actor_set_size (CLUTTER_ACTOR (actor), 0, properties->tilesize);

    g_object_set_property (G_OBJECT (actor), "repeat-x", &val);
  } else if (worm->direction == WORMDOWN || worm->direction == WORMUP) {

    if (worm->direction == WORMDOWN) {
      worm->xhead += properties->tilesize;
      worm->yhead += (properties->tilesize * size) - properties->tilesize;
    } else {
      worm->xhead -= properties->tilesize;
      worm->yhead -= (properties->tilesize * size) - properties->tilesize;
    }

    if (!tmp)
      clutter_actor_set_size (CLUTTER_ACTOR (actor),
                          properties->tilesize,
                          properties->tilesize * size);
    else
      clutter_actor_set_size (CLUTTER_ACTOR (actor), properties->tilesize, 0);

    g_object_set_property (G_OBJECT (actor), "repeat-y", &val);
  }

  clutter_container_add_actor (CLUTTER_CONTAINER (worm->actors), actor);  
  
  if (!worm->inverse)
    worm->list = g_list_prepend (worm->list, actor);
  else
    worm->list = g_list_append (worm->list, actor);
 
  //TODO: connect/timeline: start increasing the size of the actor
}

void
gnibbles_cworm_destroy (GnibblesCWorm *worm)
{
  while (worm->list)
    gnibbles_cworm_remove_actor (worm);

  g_list_free (worm->list);
  g_free (worm->actors);
}

void
gnibbles_cworm_remove_actor (GnibblesCWorm *worm)
{
  g_return_if_fail (g_list_first (worm->list)->data);

  ClutterActor *tmp = NULL;

  if (!worm->inverse) {
    tmp = CLUTTER_ACTOR ((g_list_first (worm->list))->data);
    worm->list = g_list_delete_link (worm->list, g_list_first (worm->list));
  } else {
    tmp = CLUTTER_ACTOR ((g_list_last (worm->list))->data);
    worm->list = g_list_delete_link (worm->list, g_list_last (worm->list));
  }

  clutter_container_remove_actor (CLUTTER_CONTAINER (worm->actors), tmp);
}

gint
gnibbles_cworm_lose_life (GnibblesCWorm * worm)
{
  worm->lives--;
  if (worm->lives < 0)
    return 1;

  return 0;
}
void 
gnibbles_cworm_resize (GnibblesCWorm *worm, gint newtile)
{
  if (!worm)
    return;
  if (!worm->actors)
    return;

  int i;
  int x_pos;
  int y_pos;
  int count;    
  guint w,h;
  guint size;
  gboolean direction;
  GValue val = {0,};
  ClutterActor *tmp;

  count = clutter_group_get_n_children (CLUTTER_GROUP (worm->actors));
  load_pixmap_with_tilesize (newtile);

  for (i = 0; i < count; i++) {
    tmp = clutter_group_get_nth_child (CLUTTER_GROUP (worm->actors), i);
    clutter_actor_get_position (tmp, &x_pos, &y_pos);

    clutter_actor_set_position (tmp,
                                (x_pos / properties->tilesize) * newtile,
                                (y_pos / properties->tilesize) * newtile);

    g_value_init (&val, G_TYPE_BOOLEAN);
    g_object_get_property (G_OBJECT (tmp), "repeat-x", &val);
    direction = g_value_get_boolean (&val);

    clutter_actor_get_size (CLUTTER_ACTOR (tmp), &w, &h);
    size = w < h ? h : w;
    size = size / newtile;

    if (direction)
      clutter_actor_set_size (tmp, newtile * size, newtile);
    else
      clutter_actor_set_size (tmp, newtile, newtile * size);


    //TODO: Resize/Reload pixbuf
    gtk_clutter_texture_set_from_pixbuf (CLUTTER_TEXTURE (tmp), worm_pixmaps[worm->number]);
  }

}
