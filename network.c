/*
 * Network.c - network code for gnibbles.
 *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * For more details see the file COPYING.
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
#include "main.h"
#include "gnibbles.h"
#include "properties.h"
#include "games-network.h"
#include "games-network-dialog.h"
#include "network.h"
#include "worm.h"

char *game_server = "gnothello.gnome.org"; 
char *game_port = "26479";

static void game_handle_input (NetworkGame *ng, char *buf);
static int current_worm_id;

void
network_start (void)
{
  games_network_start();
}

void
network_stop (void)
{
  games_network_stop ();
}

void
network_move_worms (void)
{
  if (network_is_host ()) {
    games_send_gamedata("move_worms\n");
  }
}

gboolean
network_is_host (void)
{
  if (!is_network_running ()) {
    return TRUE;
  }
  return (current_worm_id == WORM1);
}

void
network_game_move (guint x)
{
  static char msgbuf[256];

  snprintf (msgbuf, sizeof (msgbuf), "move %u %u \n", x, current_worm_id);
  games_send_gamedata(msgbuf);

  if (network_is_host ()) {
    worm_set_direction (current_worm_id, x);
  }
}

int
network_allow (void)
{
  return games_network_allow ();
}

static 
void clear_board(void)
{
  end_game (1);
}

static 
void gui_message(gchar *msg)
{
  /* Gnibbles has no GUI for messages yet. */
}

void
network_new (GtkWidget *parent_window)
{
  set_game_input_cb (game_handle_input);
  set_game_clear_cb (clear_board);
  set_game_msg_cb (gui_message);
  games_network_new (game_server, game_port, parent_window);
}

void
network_add_bonus (gint t_x, gint t_y,
                   gint t_type, gint t_fake, gint t_countdown)
{
  static char msgbuf[256];

  snprintf (msgbuf, sizeof (msgbuf), "add_bonus %u %u %u %u %u\n", 
				t_x, t_y, t_type, t_fake, t_countdown);
  games_send_gamedata(msgbuf);
}

void 
network_remove_bonus (gint x, gint y)
{
  static char msgbuf[256];

  snprintf (msgbuf, sizeof (msgbuf), "remove_bonus %u %u\n", x, y);
  games_send_gamedata(msgbuf);
}

gboolean 
is_network_running (void)
{
  return (get_network_status () == CONNECTED);
}

static void 
game_handle_input (NetworkGame *ng, char *buf)
{
  char *args;

  args = strchr (buf, ' ');

  if (args) {
    *args = '\0';
    args++;
  }

  /*  Handle the set_peer message containing this the id of this player,
      which is sent from the server. */
  if (!strcmp (buf, "set_peer")) {
    int me;
      
    if (ng->mycolor) {
      network_set_status (ng, DISCONNECTED, _("Invalid move attempted"));
      return;
    }
      
    if (!args || sscanf (args, "%d", &me) != 1) {
      network_set_status (ng, DISCONNECTED, _("Invalid game data (set_peer)"));
      return;
    }

    set_numworms(2);  
    ng->mycolor = me;
    network_gui_message (_("Peer introduction complete"));

  /*  Handle the move message, telling the client which
      direction to move the worm. */
  } else if (! strcmp (buf, "move")) {
    int x, me;

    if (!args || sscanf(args, "%d %d", &x, &me) != 2) {
      network_set_status (ng, DISCONNECTED, _("Invalid game data (move)"));
      return;
    }
    if (!network_is_host ()) {
      worm_set_direction (me, x);
    } else {
      static char msgbuf[256];

      snprintf (msgbuf, sizeof (msgbuf), "ack_move %u %u \n", x, me);
      worm_set_direction (me, x);
      games_send_gamedata(msgbuf);
    }

  /* Move messages from the client to the host must be acknowledged
     by the host before the client can act upon it.  */
  } else if (! strcmp (buf, "ack_move")) {
    int x, me;

    if (!args || sscanf(args, "%d %d", &x, &me) != 2) {
      network_set_status (ng, DISCONNECTED, _("Invalid game data (move)"));
      return;
    }
    worm_set_direction (me, x);


  /* The move_worms message is used to synchronize the game.
     The host sends this message to the client, which then can
     move it's worms.   */
  } else if (! strcmp (buf, "move_worms")) {
    main_loop (NULL);

  /*  Adds a bonus.  */
  } else if (! strcmp (buf, "add_bonus")) {
    int x, y, type, fake, count;

    if (!args || sscanf(args, "%u %u %u %u %u", 
			&x, &y, &type, &fake, &count) != 5) {
      network_set_status (ng, DISCONNECTED, _("Invalid game data (add_bonus)"));
      return;
    }
    gnibbles_add_spec_bonus (x, y, type, fake, count);

  /*  Removes a bonus.  */
  } else if (! strcmp (buf, "remove_bonus")) {
    int x, y;

    if (!args || sscanf(args, "%u %u ", &x, &y) != 2) {
      network_set_status (ng, DISCONNECTED, _("Invalid game data (remove_bonus)"));
      return;
    }
    gnibbles_remove_spec_bonus (x, y);



  /*  Establish a new network game. */
  } else if (!strcmp(buf, "new_game")) {

    if (!ng->sent_newgame) {
      g_string_append_printf (ng->outbuf, "new_game %s \n", player_name);
    } else {
      network_gui_connected();
      network_gui_message (_("New game ready to be started"));
      network_gui_add_player(args);
    }
    ng->sent_newgame = 0;

  /*  Start the new network game.  */
  } else if (!strcmp(buf, "start_game")) {
    network_gui_message (_("New game started"));
                                                                                
    if (!ng->sent_startgame) {
      g_string_append_printf (ng->outbuf, "start_game\n");
    }

    ng->sent_startgame = 0;
                                                                                
    new_game ();
    network_gui_close();

    if (get_mycolor () == PLAYER_1) {
      current_worm_id = WORM1;
    } else {
      current_worm_id = WORM2;
    }


  }


}

