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

#ifndef WORM_CLUTTER_H_
#define WORM_CLUTTER_H_

#include <gtk/gtk.h>
#include <clutter/clutter.h>

#define WORMNONE  0
#define WORMRIGHT 1
#define WORMDOWN  2
#define WORMLEFT  3
#define WORMUP    4
#define SLENGTH   5
#define SLIVES    10
#define ERASESIZE 6
#define ERASETIME 500

#define GROWFACTOR 4

typedef struct {
  ClutterActor *actors;
  GList *list;
  gint xstart, ystart;
  guint xhead, yhead;
  guint xtail, ytail;
  gint direction;
  gint direction_start;
  gint length;
  gint lives;
  guint score;
  guint number;
  gint start;
  gint stop;
  gint change;
} GnibblesCWorm;

typedef struct {
  ClutterActor *actor;
  gint direction;
} WormStraight;

typedef struct {
  ClutterActor *actor;
  gint direction;
} WormCorner;

GnibblesCWorm * gnibbles_cworm_new (guint number, guint t_xhead,
			                    guint t_yhead, gint t_direction);
                          
void gnibbles_cworm_add_straight_actor (GnibblesCWorm *worm);
void gnibbles_cworm_remove_actor (GnibblesCWorm *worm);
void gnibbles_cworm_destroy (GnibblesCWorm * worm);
void gnibbles_cworm_inverse (GnibblesCWorm *worm);
gint gnibbles_cworm_lose_life (GnibblesCWorm * worm);
void gnibbles_cworm_resize (GnibblesCWorm *worm, gint newtile);
void gnibbles_cworm_move (ClutterTimeline *timeline, gint frame_num, gpointer data);

gint gnibbles_cworm_handle_keypress (GnibblesCWorm * worm, guint keyval);
void gnibbles_cworm_draw_head (GnibblesCWorm * worm);
gint gnibbles_cworm_can_move_to (GnibblesCWorm * worm, gint x, gint y);
void gnibbles_cworm_position_move_head (GnibblesCWorm * worm, gint *x, gint *y);
gint gnibbles_cworm_test_move_head (GnibblesCWorm * worm);
gint gnibbles_cworm_is_move_safe (GnibblesCWorm * worm);
void gnibbles_cworm_move_tail (GnibblesCWorm * worm);
void gnibbles_cworm_ai_move (GnibblesCWorm * worm);

#endif
