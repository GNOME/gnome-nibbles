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

#ifndef _WORM_H_
#define _WORM_H_

#include <config.h>
#include <gnome.h>

#define WORMNONE  0
#define WORMRIGHT 1
#define WORMDOWN  2
#define WORMLEFT  3
#define WORMUP    4
#define SLENGTH   5
#define SLIVES    3
#define CAPACITY  BOARDWIDTH * BOARDHEIGHT
#define ERASESIZE 6
#define ERASETIME 500

#define GROWFACTOR 4

typedef struct
{
	gint xhead, yhead;
	gint xtail, ytail;
	gint direction;
	gint8 *xoff, *yoff;
	gint start, stop;
	gint length;
	gint change;
	gint keypress;
	gint lives;
	guint score;
	guint number;
} GnibblesWorm;

GnibblesWorm *gnibbles_worm_new (guint t_number);

void gnibbles_worm_destroy (GnibblesWorm *worm);

void gnibbles_worm_set_start (GnibblesWorm *worm, guint t_xhead, guint t_yhead,
		gint t_direction);

void gnibbles_worm_handle_keypress (GnibblesWorm *worm, guint keyval);

gint gnibbles_worm_move_test_head (GnibblesWorm *worm);

void gnibbles_worm_move_tail (GnibblesWorm *worm);

void gnibbles_worm_undraw_nth (GnibblesWorm *worm, gint offset);

#endif
