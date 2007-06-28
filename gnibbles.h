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

#ifndef _GNIBBLES_H_
#define _GNIBBLES_H_

#include <config.h>
#include <gnome.h>

#include "warpmanager.h"

#define BOARDWIDTH 92
#define BOARDHEIGHT 66
#define BLANKPIXMAP 0

#define NUMWORMS 6

#define NUM_COLORS 7
#define WORMRED 12
#define WORMGREEN 13
#define WORMBLUE 14
#define WORMYELLOW 15
#define WORMCYAN 16
#define WORMPURPLE 17
#define WORMGRAY 18

#define WORMCHAR 'w'
#define EMPTYCHAR 'a'

#define CONTINUE 0
#define NEWROUND 1
#define GAMEOVER 2
#define VICTORY 3

#define GAMEDELAY 35
#define NETDELAY 2
#define BONUSDELAY 100

#define MAXLEVEL 26

extern GnibblesWarpManager *warpmanager;

void gnibbles_draw_pixmap (gint which, gint x, gint y);
void gnibbles_draw_big_pixmap (gint which, gint x, gint y);
void gnibbles_draw_pixmap_buffer (gint which, gint x, gint y);
void gnibbles_draw_big_pixmap_buffer (gint which, gint x, gint y);
void gnibbles_load_pixmap (GtkWidget * window);
void gnibbles_load_logo (GtkWidget * window);
void gnibbles_load_level (GtkWidget * window, gint level);
void gnibbles_init (void);
void gnibbles_add_bonus (gint regular);
gint gnibbles_move_worms (void);
gint gnibbles_get_winner (void);
gint gnibbles_keypress_worms (guint keyval);
void gnibbles_undraw_worms (gint data);
void gnibbles_show_scores (GtkWidget * window, gint pos);
void gnibbles_log_score (GtkWidget * window);
void gnibbles_add_spec_bonus (gint t_x, gint t_y,
			      gint t_type, gint t_fake, gint t_countdown);
void gnibbles_remove_spec_bonus (gint x, gint y);

#endif
