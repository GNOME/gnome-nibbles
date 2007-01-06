/* server.c
 * Network server for Gnibbles.
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
#include "config.h"
#include <gnome.h>
#include <ggzdmod.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "server.h"

// Global game variables
struct game_t game;


// Initializes everything
void
game_init (GGZdMod * ggzdmod)
{
  game.ggz = ggzdmod;
  game.state = GN_STATE_INIT;
}

// Handle server messages
void
game_handle_ggz_state (GGZdMod * ggz, GGZdModEvent event, const void *data)
{
  GGZdModState new_state = ggzdmod_get_state (ggz);
  const GGZdModState *old_state_ptr = data;
  const GGZdModState old_state = *old_state_ptr;

  // Check if it's the right time to launch the game and if ggz could do taht
  if (old_state == GGZDMOD_STATE_CREATED) {
    /*assert(game.state == GN_STATE_INIT); */
    return;
  }

  if (new_state == GGZDMOD_STATE_PLAYING) {

    // No! Let's start the game!
    game_start ();
  }

  if (new_state == GGZDMOD_STATE_WAITING) {
    // That's great! Update the state
    // Now waiting for people to join
    game.state = GN_STATE_WAIT;
    ggzdmod_log (game.ggz, "Waiting for players");
  }
}


static int
seats_full (void)
{
  /* This calculation is a bit inefficient, but that's OK */
  return ggzdmod_count_seats (game.ggz, GGZ_SEAT_OPEN) == 0
    && ggzdmod_count_seats (game.ggz, GGZ_SEAT_RESERVED) == 0
    && ggzdmod_count_seats (game.ggz, GGZ_SEAT_ABANDONED) == 0;
}


static int
seats_empty (void)
{
  /* This calculation is a bit inefficient, but that's OK */
  return ggzdmod_count_seats (game.ggz, GGZ_SEAT_PLAYER) == 0
    && ggzdmod_count_spectators (game.ggz) == 0;
}


void
game_handle_ggz_seat (GGZdMod * ggz, GGZdModEvent event, const void *data)
{
  const GGZSeat *old_seat = data;
  GGZSeat new_seat = ggzdmod_get_seat (ggz, old_seat->num);
  GGZdModState new_state;

  /* Check the state. */
  if (seats_full ())
    new_state = GGZDMOD_STATE_PLAYING;
  else if (seats_empty ())
    new_state = GGZDMOD_STATE_DONE;
  else
    new_state = GGZDMOD_STATE_WAITING;

  // That's great!! Do stuff

  if (new_seat.type == GGZ_SEAT_PLAYER)
    game_send_seat (new_seat.num);
  game_send_players ();

  ggzdmod_set_state (ggz, new_state);
}


/* Send out game sync signal. */
int
game_send_sync (void)
{
  int j, mfd;

  for (j = 0; j < ggzdmod_get_num_seats (game.ggz); j++) {
    if ((mfd = ggzdmod_get_seat (game.ggz, j).fd) == -1)
      continue;

    if (ggz_write_int (mfd, GN_MSG_SYNC) < 0)
      return -1;
  }

  return 0;
}


/* Send out seat assignment */
int
game_send_seat (int seat)
{
  int fd = ggzdmod_get_seat (game.ggz, seat).fd;

  ggzdmod_log (game.ggz, "Sending player %d's seat num", seat);

  if (ggz_write_int (fd, GN_MSG_SEAT) < 0
      || ggz_write_int (fd, ggzdmod_get_num_seats (game.ggz)) < 0
      || ggz_write_int (fd, seat) < 0)
    return -1;

  return 0;
}


/* Send out player list to everybody */
int
game_send_players (void)
{
  int i, j, mfd;

  for (j = 0; j < ggzdmod_get_num_seats (game.ggz); j++) {
    if ((mfd = ggzdmod_get_seat (game.ggz, j).fd) == -1)
      continue;

    ggzdmod_log (game.ggz, "Sending player list to player %d", j);

    if (ggz_write_int (mfd, GN_MSG_PLAYERS) < 0)
      return -1;

    if (ggz_write_int (mfd, ggzdmod_get_num_seats (game.ggz)) < 0)
      return -1;
    for (i = 0; i < ggzdmod_get_num_seats (game.ggz); i++) {
      GGZSeat seat = ggzdmod_get_seat (game.ggz, i);
      if (ggz_write_int (mfd, seat.type) < 0)
	return -1;
      if (seat.type != GGZ_SEAT_OPEN
	  /* FIXME: This is a problem since seat.name
	   * can in theory be NULL. --JDS */
	  && ggz_write_string (mfd, seat.name) < 0)
	return -1;
    }
  }
  return 0;
}

int
game_start (void)
{

  int i, fd;

  // Start game variables
  game.state = GN_STATE_PLAYING;

  // Sends out start message
  for (i = 0; i < ggzdmod_get_num_seats (game.ggz); i++) {
    fd = ggzdmod_get_seat (game.ggz, i).fd;
    // Don't send anything if the player is a computer!
    if (fd == -1)
      continue;
    if (ggz_write_int (fd, GN_MSG_START) < 0)
      return -1;
  }

  ggzdmod_log (game.ggz, "Game has started!\n");

  return 0;
}

/* return -1 on error, 1 on gameover */
void
game_handle_player (GGZdMod * ggz, GGZdModEvent event, const void *data)
{
  const int *seat_ptr = data;
  const int seat = *seat_ptr;
  int op;
  int fd = ggzdmod_get_seat (ggz, seat).fd;

  if (ggz_read_int (fd, &op) < 0)
    return;

  switch (op) {


  case GN_REQ_SYNC:
    if (seat == 0) {
      game_send_sync ();
    }
    break;

  case GN_REQ_MOVE:
    game_handle_move (seat);
    break;

  case GN_REQ_SETTINGS:
    game_handle_settings (seat);
    break;

  case GN_REQ_BONI:
    if (seat == 0) {
      game_handle_boni (seat);
    }
    break;
  case GN_REQ_NOBONI:
    if (seat == 0) {
      game_handle_noboni (seat);
    }
    break;


  default:
    ggzdmod_log (game.ggz, "ERROR: unknown player opcode %d.", op);
    break;
  }
}


int
game_handle_noboni (int seat)
{
  int i, x, y;
  int fd = ggzdmod_get_seat (game.ggz, seat).fd;

  // Get the move from the message
  if (ggz_read_int (fd, &x) < 0 || ggz_read_int (fd, &y) < 0)
    return -1;

  // Sends out start message
  for (i = 0; i < ggzdmod_get_num_seats (game.ggz); i++) {
    fd = ggzdmod_get_seat (game.ggz, i).fd;
    // Don't send anything if the player is a computer!
    if (fd == -1)
      continue;
    if (ggz_write_int (fd, GN_MSG_NOBONI) < 0 || ggz_write_int (fd, x) < 0
	|| ggz_write_int (fd, y) < 0)
      return -1;
  }

  return 2;

}

int
game_handle_boni (int seat)
{
  int i, x, y, type, fake, countdown;
  int fd = ggzdmod_get_seat (game.ggz, seat).fd;

  // Get the move from the message
  if (ggz_read_int (fd, &x) < 0
      || ggz_read_int (fd, &y) < 0
      || ggz_read_int (fd, &type) < 0
      || ggz_read_int (fd, &fake) < 0 || ggz_read_int (fd, &countdown) < 0)
    return -1;

  // Sends out start message
  for (i = 0; i < ggzdmod_get_num_seats (game.ggz); i++) {
    fd = ggzdmod_get_seat (game.ggz, i).fd;
    // Don't send anything if the player is a computer!
    if (fd == -1)
      continue;
    if (ggz_write_int (fd, GN_MSG_BONI) < 0 || ggz_write_int (fd, x) < 0
	|| ggz_write_int (fd, y) < 0
	|| ggz_write_int (fd, type) < 0
	|| ggz_write_int (fd, fake) < 0 || ggz_write_int (fd, countdown) < 0)
      return -1;
  }

  return 2;

}

int
game_handle_settings (int seat)
{
  int speed, fakes, startlevel, i;
  int fd = ggzdmod_get_seat (game.ggz, seat).fd;

  // Get the move from the message
  if (ggz_read_int (fd, &speed) < 0
      || ggz_read_int (fd, &fakes) < 0 || ggz_read_int (fd, &startlevel) < 0)
    return -1;

  // Sends out start message
  for (i = 0; i < ggzdmod_get_num_seats (game.ggz); i++) {
    fd = ggzdmod_get_seat (game.ggz, i).fd;
    // Don't send anything if the player is a computer!
    if (fd == -1)
      continue;
    if (ggz_write_int (fd, GN_MSG_SETTINGS) < 0
	|| ggz_write_int (fd, speed) < 0 || ggz_write_int (fd, fakes) < 0
	|| ggz_write_int (fd, startlevel) < 0)
      return -1;
  }

  return 2;

}

int
game_handle_move (int seat)
{
  int fd = ggzdmod_get_seat (game.ggz, seat).fd;
  int move, i;

  // Get the move from the message
  if (ggz_read_int (fd, &move) < 0)
    return -1;

  // Sends out start message
  for (i = 0; i < ggzdmod_get_num_seats (game.ggz); i++) {
    fd = ggzdmod_get_seat (game.ggz, i).fd;
    // Don't send anything if the player is a computer!
    if (fd == -1)
      continue;
    if (ggz_write_int (fd, GN_MSG_MOVE) < 0 || ggz_write_int (fd, seat) < 0
	|| ggz_write_int (fd, move) < 0)
      return -1;
  }

  return 2;

}



int
main (void)
{
  GGZdMod *ggz = ggzdmod_new (GGZDMOD_GAME);

  /* game_init is called at the start of _each_ game, so we must do
   * ggz stuff here. */
  ggzdmod_set_handler (ggz, GGZDMOD_EVENT_STATE, &game_handle_ggz_state);
  ggzdmod_set_handler (ggz, GGZDMOD_EVENT_JOIN, &game_handle_ggz_seat);
  ggzdmod_set_handler (ggz, GGZDMOD_EVENT_LEAVE, &game_handle_ggz_seat);
  ggzdmod_set_handler (ggz, GGZDMOD_EVENT_SEAT, &game_handle_ggz_seat);
  ggzdmod_set_handler (ggz, GGZDMOD_EVENT_PLAYER_DATA, &game_handle_player);

  game_init (ggz);

  /* Connect to GGZ server; main loop */
  if (ggzdmod_connect (ggz) < 0)
    return -1;
  (void) ggzdmod_loop (ggz);
  (void) ggzdmod_disconnect (ggz);
  ggzdmod_free (ggz);

  return 0;
}
