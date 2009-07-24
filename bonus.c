/* 
 *   Gnome Nibbles: Gnome Worm Game
 *   Written by Sean MacIsaac <sjm@acm.org>, Ian Peters <itp@gnu.org>,
 *              Guillaume Beland <guillaume.beland@gmail.com>
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

#include <stdlib.h>

#include <gtk/gtk.h>
#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>

#include <libgames-support/games-runtime.h>

#include "gnibbles.h"
#include "bonus.h"
#include "properties.h"
#include "board.h"

extern GdkPixbuf *boni_pixmaps[];
extern GnibblesProperties *properties;
extern GnibblesBoard *board;

GnibblesBonus *
gnibbles_bonus_new (gint t_x, gint t_y, gint t_type,
		    gint t_fake, gint t_countdown)
{
  GnibblesBonus *tmp;

  tmp = g_new (GnibblesBonus, 1);

  tmp->x = t_x;
  tmp->y = t_y;
  tmp->type = t_type;
  tmp->fake = t_fake;
  tmp->countdown = t_countdown;
  tmp->actor = clutter_texture_new ();
  return (tmp);
}

static void
animate_bonus (ClutterAnimation *animation, ClutterActor *actor)
{
  ClutterVertex center;
  ClutterAnimation *anim;

  center = (ClutterVertex){(gfloat)properties->tilesize, 0, 0};
  anim = clutter_actor_animate (actor, CLUTTER_LINEAR, 2000,
                         "rotation-angle-y", 360.f,
                         "fixed::rotation-center-y", &center,
                         NULL);
  clutter_animation_set_loop (anim, TRUE);
  
}

void
gnibbles_bonus_draw (GnibblesBonus *bonus)
{
  clutter_actor_set_position (CLUTTER_ACTOR (bonus->actor),
                              bonus->x * properties->tilesize,
                              bonus->y * properties->tilesize);

  gtk_clutter_texture_set_from_pixbuf (CLUTTER_TEXTURE (bonus->actor),
                                       boni_pixmaps[bonus->type]);

  clutter_container_add_actor (CLUTTER_CONTAINER (board->stage), bonus->actor);

  clutter_actor_set_opacity (bonus->actor, 0);
  g_signal_connect_after (
    clutter_actor_animate (bonus->actor, CLUTTER_EASE_IN_QUAD, 500,
                         "opacity", 0xff,
                          NULL),
    "completed", G_CALLBACK (animate_bonus), bonus->actor);
}

void
gnibbles_bonus_erase (GnibblesBonus *bonus)
{
  clutter_actor_hide (bonus->actor);
}
