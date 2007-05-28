/* ggz-network.c
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

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <gnome.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <pwd.h>

#include "main.h"

#include <ggzmod.h>
#include <ggz-embed.h>
#include <ggz-gtk.h>
#include "games-dlg-chat.h"
#include "games-dlg-players.h"

#include "gnibbles.h"
#include "properties.h"
#include "ggz-network.h"
#include "worm.h"


static void
get_sync (void)
{
  main_loop (NULL);
}

static int
get_seat (void)
{
  if (ggz_read_int (fd, &num_players) < 0 || ggz_read_int (fd, &seat) < 0)
    return -1;
  gnibbles_properties_set_worms_number (num_players);
  gnibbles_properties_set_ai_number (0);

  return 0;
}

static int
get_settings (void)
{
  int speed, fakes, startlevel;

  if (ggz_read_int (fd, &speed) < 0
      || ggz_read_int (fd, &fakes) < 0 || ggz_read_int (fd, &startlevel) < 0)
    return -1;
  gnibbles_properties_set_speed (speed);
  gnibbles_properties_set_fakes (fakes);
  gnibbles_properties_set_start_level (startlevel);

  return 0;
}
static int
get_boni (void)
{
  int x, y, type, fake, countdown;

  if (ggz_read_int (fd, &x) < 0
      || ggz_read_int (fd, &y) < 0
      || ggz_read_int (fd, &type) < 0
      || ggz_read_int (fd, &fake) < 0 || ggz_read_int (fd, &countdown) < 0)
    return -1;
  gnibbles_add_spec_bonus (x, y, type, fake, countdown);

  return 0;
}

static int
get_noboni (void)
{
  int x, y;

  if (ggz_read_int (fd, &x) < 0 || ggz_read_int (fd, &y) < 0)
    return -1;
  gnibbles_remove_spec_bonus (x, y);

  return 0;
}

static int
get_move (void)
{
  int player, move;

  if (ggz_read_int (fd, &player) < 0 || ggz_read_int (fd, &move) < 0)
    return -1;
  /* show some kind of pregame thing....  */
  worm_set_direction (player, move);
  return 0;
}

static int
get_players (void)
{
  int i, old;
  static int firsttime = 1;
  char *tmp;

  if (ggz_read_int (fd, &num_players) < 0)
    return -1;
  for (i = 0; i < num_players; i++) {
    old = seats[i];
    if (ggz_read_int (fd, &seats[i]) < 0)
      return -1;
    if (seats[i] != GGZ_SEAT_OPEN) {
      if (ggz_read_string (fd, (char *) &names[i], 17) < 0)
	return -1;
      /*display_set_name(i, game.names[i]); */
      if (old == GGZ_SEAT_OPEN && !firsttime) {
	tmp = g_strdup_printf (_("%s joined the game.\n"), names[i]);
	add_chat_text (tmp);
	g_free (tmp);
      }
    } 

    if (seats[i] == GGZ_SEAT_ABANDONED) {
      if (i == 0) {
	tmp =
	  g_strdup_printf (_
			   ("The game ended because the host %s left the game.\n"),
			   names[i]);
	add_chat_text (tmp);
	g_free (tmp);
      } else {
	tmp =
	  g_strdup_printf (_("%s left the game.\n"),
			   names[i]);
	add_chat_text (tmp);
	g_free (tmp);
      }
    }
  }

  firsttime = 0;
  /* game.got_players++; */

  return 0;
}

static gboolean
game_handle_io (GGZMod * mod)
{
  int op = -1;

  fd = ggzmod_get_server_fd (mod);

  // Read the fd
  if (ggz_read_int (fd, &op) < 0) {
    ggz_error_msg ("Couldn't read the game fd");
    return FALSE;
  }

  switch (op) {
  case GN_MSG_SEAT:
    get_seat ();
    break;
  case GN_MSG_PLAYERS:
    get_players ();
    break;

  case GN_MSG_START:
    gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), MAIN_PAGE);
    new_game ();
    break;

  case GN_MSG_SYNC:
    get_sync ();
    break;

  case GN_MSG_MOVE:
    get_move ();
    break;

  case GN_MSG_SETTINGS:
    get_settings ();
    break;

  case GN_MSG_BONI:
    get_boni ();
    break;

  case GN_MSG_NOBONI:
    get_noboni ();
    break;

  default:
    ggz_error_msg ("Incorrect opcode   %d \n", op);
    break;
  }

  return TRUE;
}


static gboolean
handle_ggzmod (GIOChannel * channel, GIOCondition cond, gpointer data)
{
  GGZMod *mod = data;

  return (ggzmod_dispatch (mod) >= 0);
}

static gboolean
handle_game_server (GIOChannel * channel, GIOCondition cond, gpointer data)
{
  GGZMod *mod = data;

  return game_handle_io (mod);
}

static void
handle_ggzmod_server (GGZMod * mod, GGZModEvent e, const void *data)
{
  const int *fd = data;
  GIOChannel *channel;

  ggzmod_set_state (mod, GGZMOD_STATE_PLAYING);
  channel = g_io_channel_unix_new (*fd);
  g_io_add_watch (channel, G_IO_IN, handle_game_server, mod);
}

void
network_init (void)
{
  GGZMod *mod;
  GIOChannel *channel;
  int ret, ggzmodfd;

  if (!ggzmod_is_ggz_mode ())
    return;
  ggz_network_mode = TRUE;

  mod = ggzmod_new (GGZMOD_GAME);
  ggzmod_set_handler (mod, GGZMOD_EVENT_SERVER, handle_ggzmod_server);

  ret = ggzmod_connect (mod);
  if (ret != 0) {
    /* Error: GGZ core client error (e.g. faked GGZMODE env variable) */
    return;
  }

  ggzmodfd = ggzmod_get_fd (mod);
  channel = g_io_channel_unix_new (ggzmodfd);
  g_io_add_watch (channel, G_IO_IN, handle_ggzmod, mod);

  init_player_list (mod);
  init_chat (mod);


}

void
network_move_worms (void)
{
  if (ggz_write_int (fd, GN_REQ_SYNC) < 0) {
    return;
  }

}

void
network_req_settings (int speed, int fake, int startlevel)
{
  if (ggz_write_int (fd, GN_REQ_SETTINGS) < 0
      || ggz_write_int (fd, speed) < 0
      || ggz_write_int (fd, fake) < 0 || ggz_write_int (fd, startlevel) < 0) {
    return;
  }

}

gboolean
network_is_host (void)
{
  if (!ggz_network_mode) {
    return TRUE;
  }

  return (seat == GN_HOST);
}

void
network_game_move (guint x)
{

  if (ggz_write_int (fd, GN_REQ_MOVE) < 0 || ggz_write_int (fd, x) < 0) {
    return;
  }

}


void
network_add_bonus (gint t_x, gint t_y,
		   gint t_type, gint t_fake, gint t_countdown)
{
  if (ggz_write_int (fd, GN_REQ_BONI) < 0
      || ggz_write_int (fd, t_x) < 0
      || ggz_write_int (fd, t_y) < 0
      || ggz_write_int (fd, t_type) < 0
      || ggz_write_int (fd, t_fake) < 0
      || ggz_write_int (fd, t_countdown) < 0) {
    return;
  }


}

void
network_remove_bonus (gint x, gint y)
{
  if (ggz_write_int (fd, GN_REQ_NOBONI) < 0
      || ggz_write_int (fd, x) < 0 || ggz_write_int (fd, y) < 0) {
    return;
  }

}

/****************************************************************************
  Callback function that's called by the library when a connection is
  established (or lost) to the GGZ server.  The server parameter gives
  the server (or NULL).
****************************************************************************/
static void
ggz_connected (GGZServer * server)
{
  /* Nothing useful to do... */
}

/****************************************************************************
  Callback function that's called by the library when we launch a game.  This
  means we now have a connection to a gnect server so handling can be given
  back to the regular gnect code.
****************************************************************************/
static void
ggz_game_launched (void)
{
  gchar *str = NULL;

  network_init ();
  end_game (TRUE);

  str = g_strdup_printf (_("Welcome to a network game of %s."),
			 NETWORK_ENGINE);
  add_chat_text (str);
  add_chat_text ("\n");
  g_free (str);

}

/****************************************************************************
  Callback function that's invoked when GGZ is exited.
****************************************************************************/
static void
ggz_closed (void)
{
  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), MAIN_PAGE);
  ggz_network_mode = FALSE;
  end_game (TRUE);
}

void
on_network_game (void)
{
  GtkWidget *ggzbox;
  struct passwd *pwent;  

  if (ggz_network_mode) {
    gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), NETWORK_PAGE);
    return;
  }

  ggz_network_mode = TRUE;

  ggz_gtk_initialize (FALSE,
		      ggz_connected, ggz_game_launched, ggz_closed,
		      NETWORK_ENGINE, NETWORK_VERSION, "gnibbles.xml",
		      "GGZ Gaming Zone");

  pwent = getpwuid(getuid());
  ggz_embed_ensure_server ("GGZ Gaming Zone", "gnome.ggzgamingzone.org",
			   5688, pwent->pw_name);

  ggzbox = ggz_gtk_create_main_area (window);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), ggzbox, NULL);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), NETWORK_PAGE);
}
