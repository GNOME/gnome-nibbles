/* ggz-network.h
 *
 * Copyright (C) 2006 -  Andreas Røsdal <andrearo@pvv.ntnu.no>
 *
 * This game is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 * USA
 */

#ifndef NETWORK_H
#define NETWORK_H

#define NETWORK_ENGINE "Gnibbles"
#define NETWORK_VERSION "1"

#define GN_MSG_SEAT 0
#define GN_MSG_PLAYERS 1
#define GN_MSG_SYNC 2
#define GN_MSG_START 5
#define GN_REQ_MOVE 6
#define GN_MSG_MOVE 7
#define GN_ACK_START 8
#define GN_REQ_SYNC 11
#define GN_REQ_SETTINGS 9
#define GN_MSG_SETTINGS 10
#define GN_REQ_BONI 12
#define GN_MSG_BONI 13
#define GN_REQ_NOBONI 14
#define GN_MSG_NOBONI 15

#define GN_HOST 0

int fd;

extern char *game_server;

void network_init (void);
void network_game_move (guint);
void network_add_bonus (gint t_x, gint t_y,
			gint t_type, gint t_fake, gint t_countdown);
void network_remove_bonus (gint x, gint y);
gboolean network_is_host (void);
void network_move_worms (void);
void network_req_settings (int speed, int fake, int startlevel);
void on_network_game (void);

#endif
