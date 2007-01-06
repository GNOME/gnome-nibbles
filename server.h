/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#define BLACK -1
#define WHITE +1
#define EMPTY  0

#include "ggzdmod.h"

#define GN_MSG_SEAT 0
#define GN_MSG_PLAYERS 1
#define GN_MSG_SYNC 2
#define GN_MSG_START 5
#define GN_REQ_MOVE 6
#define GN_MSG_MOVE 7
#define GN_ACK_START 8
#define GN_REQ_SETTINGS 9
#define GN_MSG_SETTINGS 10
#define GN_REQ_SYNC 11
#define GN_REQ_BONI 12
#define GN_MSG_BONI 13
#define GN_REQ_NOBONI 14
#define GN_MSG_NOBONI 15

// States
#define GN_STATE_INIT 0
#define GN_STATE_WAIT 1
#define GN_STATE_PLAYING 2
#define GN_STATE_DONE 3

// Responses from server
#define GN_SERVER_ERROR -1
#define GN_SERVER_OK 0
#define GN_SERVER_JOIN 1
#define GN_SERVER_LEFT 2
#define GN_SERVER_QUIT 3

// Errors
#define GN_ERROR_INVALIDMOVE -1
#define GN_ERROR_WRONGTURN -2
#define GN_ERROR_CANTMOVE -3


struct game_t {
  /* GGZ data */
  GGZdMod *ggz;
  // State
  char state;
};

// Intializes game variables
void game_init (GGZdMod * ggzdmod);
// Handle server messages
void game_handle_ggz_state (GGZdMod * ggz,
			    GGZdModEvent event, const void *data);
void game_handle_ggz_seat (GGZdMod * ggz,
			   GGZdModEvent event, const void *data);
// Handle player messages
void game_handle_player (GGZdMod * ggz, GGZdModEvent event, const void *data);
// Handle player move
int game_handle_move (int);

int game_handle_settings (int seat);

int game_handle_boni (int seat);
int game_handle_noboni (int seat);

// Send to the player what is his seat
int game_send_seat (int);

int game_send_sync (void);
// Send to everyone who is playing
int game_send_players (void);
// Sends the start message and start the game
int game_start (void);
