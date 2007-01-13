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

#ifndef _MAIN_H_
#define _MAIN_H_

#include <games-scores.h>

#include "gnibbles.h"

#define MAIN_PAGE           	0
#define NETWORK_PAGE           	1


gboolean ggz_network_mode;
int player_id;
int num_players;
int seat;
int seats[NUMWORMS];
char names[NUMWORMS][17];

gint game_running (void);
gint pause_game_cb (GtkAction * action, gpointer data);
void end_game (gboolean);
gint new_game (void);
gint main_loop (gpointer data);

extern GamesScores *highscores;
extern GtkWidget *notebook;
extern GtkWidget *window;

#endif
