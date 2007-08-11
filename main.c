/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

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

#include <config.h>
#include <gnome.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <time.h>

#include <games-gridframe.h>
#include <games-stock.h>
#include <games-scores.h>
#include <games-sound.h>
#include <games-conf.h>

#include "main.h"
#include "properties.h"
#include "gnibbles.h"
#include "worm.h"
#include "bonus.h"
#include "boni.h"
#include "preferences.h"
#include "scoreboard.h"
#include "warp.h"

#ifdef GGZ_CLIENT
#include <games-dlg-chat.h>
#include <games-dlg-players.h>
#include "ggz-network.h"
#include <ggz-embed.h>
#endif

#define DEFAULT_WIDTH 650
#define DEFAULT_HEIGHT 520

GtkWidget *window;
GtkWidget *drawing_area;
GtkWidget *appbar;
GtkWidget *notebook;
GtkWidget *chat = NULL;

static const GamesScoresCategory scorecats[] = { {"4.0", N_("Beginner")},
{"3.0", N_("Slow")},
{"2.0", N_("gnibbles|Medium")},
{"1.0", N_("Fast")},
{"4.1", N_("Beginner with Fakes")},
{"3.1", N_("Slow with Fakes")},
{"2.1", N_("Medium with Fakes")},
{"1.1", N_("Fast with Fakes")},
GAMES_SCORES_LAST_CATEGORY
};
static const GamesScoresDescription scoredesc = { scorecats,
  "4.0",
  "gnibbles",
  GAMES_SCORES_STYLE_PLAIN_DESCENDING
};

GamesScores *highscores;

extern GdkPixmap *buffer_pixmap;
extern GdkPixbuf *logo_pixmap;

GnibblesProperties *properties;

GnibblesScoreboard *scoreboard;

extern GnibblesBoni *boni;

gchar board[BOARDWIDTH][BOARDHEIGHT];

gint main_id = 0;
gint dummy_id = 0;
gint keyboard_id = 0;
gint erase_id = 0;
gint add_bonus_id = 0;
gint restart_id = 0;

gint paused = 0;

gint current_level;

static struct _pointers {
  GdkCursor *current;
  GdkCursor *invisible;
} pointers = {
NULL};

static gint add_bonus_cb (gpointer data);
static void render_logo (void);
static gint end_game_cb (GtkAction * action, gpointer data);

static GtkAction *new_game_action;
static GtkAction *new_network_action;
static GtkAction *player_list_action;
static GtkAction *pause_action;
static GtkAction *resume_action;
static GtkAction *end_game_action;
static GtkAction *preferences_action;
static GtkAction *scores_action;
static GtkAction *fullscreen_action;
static GtkAction *leave_fullscreen_action;

static void
hide_cursor (void)
{
  if (pointers.current != pointers.invisible) {
    gdk_window_set_cursor (drawing_area->window, pointers.invisible);
    pointers.current = pointers.invisible;
  }
}

static void
set_fullscreen_actions (gboolean is_fullscreen)
{
  gtk_action_set_sensitive (leave_fullscreen_action, is_fullscreen);
  gtk_action_set_visible (leave_fullscreen_action, is_fullscreen);

  gtk_action_set_sensitive (fullscreen_action, !is_fullscreen);
  gtk_action_set_visible (fullscreen_action, !is_fullscreen);
}

static void
fullscreen_cb (GtkAction * action)
{
  if (action == fullscreen_action) {
    gtk_window_fullscreen (GTK_WINDOW (window));
  } else {
    gtk_window_unfullscreen (GTK_WINDOW (window));
  }
}
static void
network_gui_update (void)
{

#ifdef GGZ_CLIENT
  if (ggz_network_mode) {
    gtk_widget_show (chat);
  } else {
    gtk_widget_hide (chat);
  }
  gtk_action_set_visible (new_game_action, !ggz_network_mode);
  gtk_action_set_visible (player_list_action, ggz_network_mode);
  gtk_container_check_resize (GTK_CONTAINER (window));

#endif
}

static gboolean
window_state_cb (GtkWidget * widget, GdkEventWindowState * event)
{
  /* Handle fullscreen, in case something else takes us to/from fullscreen. */
  if (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN)
    set_fullscreen_actions (event->new_window_state
			    & GDK_WINDOW_STATE_FULLSCREEN);
    
  return FALSE;
}


static void
show_cursor (void)
{
  if (pointers.current != NULL) {
    gdk_window_set_cursor (drawing_area->window, NULL);
    pointers.current = NULL;
  }
}

gint
game_running (void)
{
  return (main_id || erase_id || dummy_id || restart_id || paused);
}

static void
zero_board (void)
{
  gint i, j;

  for (i = 0; i < BOARDWIDTH; i++)
    for (j = 0; j < BOARDHEIGHT; j++) {
      board[i][j] = EMPTYCHAR;
      gnibbles_draw_pixmap_buffer (0, i, j);
    }
}

static void
on_player_list (void)
{
#ifdef GGZ_CLIENT
  create_or_raise_dlg_players (GTK_WINDOW (window));
#endif
}

/* Avoid a race condition where a redraw is attempted
 * between the window being destroyed and the destroy
 * event being sent. */
static gint
delete_cb (GtkWidget * widget, gpointer data)
{
  if (main_id)
    g_source_remove (main_id);
  if (erase_id)
    g_source_remove (erase_id);
  if (dummy_id)
    g_source_remove (dummy_id);
  if (restart_id)
    g_source_remove (restart_id);

  return FALSE;
}

static void
quit_cb (GObject * object, gpointer data)
{
  gtk_widget_destroy (window);
}

static void
about_cb (GtkAction * action, gpointer data)
{
  const gchar *authors[] = { "Sean MacIsaac", "Ian Peters", "Andreas Røsdal", NULL };

  const gchar *documenters[] = { "Kevin Breit", NULL };
  gchar *license = games_get_license (_("Nibbles"));

  gtk_show_about_dialog (GTK_WINDOW (window),
#if GTK_CHECK_VERSION (2, 11, 0)
                         "program-name", _("Nibbles"),
#else
			 "name", _("Nibbles"),
#endif
			 "version", VERSION,
			 "copyright",
			 "Copyright \xc2\xa9 1999-2007 Sean MacIsaac, Ian Peters, Andreas Røsdal",
			 "license", license, "comments",
			 _("A worm game for GNOME.\n\nNibbles is a part of GNOME Games."), "authors", authors,
			 "documenters", documenters, "translator-credits",
			 _("translator-credits"), "logo-icon-name",
			 "gnome-gnibbles", "website",
			 "http://www.gnome.org/projects/gnome-games/",
			 "website-label", _("GNOME Games web site"),
			 "wrap-license", TRUE, NULL);
  g_free (license);
}

static gint
expose_event_cb (GtkWidget * widget, GdkEventExpose * event)
{
  gdk_draw_drawable (GDK_DRAWABLE (widget->window),
		     widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		     buffer_pixmap, event->area.x, event->area.y,
		     event->area.x, event->area.y, event->area.width,
		     event->area.height);

  return (FALSE);
}

static gint
key_press_cb (GtkWidget * widget, GdkEventKey * event)
{
  hide_cursor ();

  return gnibbles_keypress_worms (event->keyval);
}

static void
draw_board ()
{
  int i, j;

  for (i = 0; i < BOARDWIDTH; i++) {
    for (j = 0; j < BOARDHEIGHT; j++) {
      gnibbles_draw_pixmap_buffer (0, i, j);
      if (board[i][j] >= EMPTYCHAR && board[i][j] < EMPTYCHAR + 19) {
	gnibbles_draw_pixmap_buffer (board[i][j] - EMPTYCHAR, i, j);
      } else if (board[i][j] >= WORMCHAR && board[i][j] < WORMCHAR + NUMWORMS) {
	gnibbles_draw_pixmap_buffer (properties->wormprops
				     [board[i][j] - WORMCHAR]->color, i, j);
      } else if (board[i][j] >= 'A' && board[i][j] < 'J') {
	/* bonus */
      } else {
	/* Warp point. */
      }
    }
  }

  for (i = 0; i < boni->numbonuses; i++)
    gnibbles_bonus_draw (boni->bonuses[i]);

  for (i = 0; i < warpmanager->numwarps; i++)
    gnibbles_warp_draw_buffer (warpmanager->warps[i]);

  gdk_draw_drawable (GDK_DRAWABLE (drawing_area->window),
		     drawing_area->style->
		     fg_gc[GTK_WIDGET_STATE (drawing_area)], buffer_pixmap, 0,
		     0, 0, 0, BOARDWIDTH * properties->tilesize,
		     BOARDHEIGHT * properties->tilesize);

}

static gboolean
configure_event_cb (GtkWidget * widget, GdkEventConfigure * event)
{
  int tilesize, ts_x, ts_y;

  /* Compute the new tile size based on the size of the
   * drawing area, rounded down. */
  ts_x = event->width / BOARDWIDTH;
  ts_y = event->height / BOARDHEIGHT;
  if (ts_x * BOARDWIDTH > event->width)
    ts_x--;
  if (ts_y * BOARDHEIGHT > event->height)
    ts_y--;
  tilesize = MIN (ts_x, ts_y);

  /* But, has the tile size changed? */
  if (properties->tilesize == tilesize) {

    /* We must always re-load the logo. */
    gnibbles_load_logo (window);
    return FALSE;
  }

  properties->tilesize = tilesize;
  gnibbles_properties_set_tile_size (tilesize);

  /* Reload the images pixmap. */
  gnibbles_load_logo (window);
  gnibbles_load_pixmap (window);

  /* Recreate the buffer pixmap. */
  if (buffer_pixmap)
    g_object_unref (G_OBJECT (buffer_pixmap));
  buffer_pixmap = gdk_pixmap_new (drawing_area->window,
				  BOARDWIDTH * properties->tilesize,
				  BOARDHEIGHT * properties->tilesize, -1);

  /* Erase the buffer pixmap. */
  gdk_draw_rectangle (buffer_pixmap,
		      drawing_area->style->black_gc,
		      TRUE, 0, 0,
		      BOARDWIDTH * properties->tilesize,
		      BOARDHEIGHT * properties->tilesize);

  if (game_running ())
    draw_board ();
  else
    render_logo ();

  return FALSE;
}

#ifdef GGZ_CLIENT
static gint
network_loop (gpointer data)
{
  if (ggz_network_mode) { 
    network_move_worms ();
  }
  return TRUE;
}
#endif

static gint
new_game_2_cb (GtkWidget * widget, gpointer data)
{
  if (!paused) {
    if (!keyboard_id)
      keyboard_id = g_signal_connect (G_OBJECT (window),
				      "key_press_event",
				      G_CALLBACK (key_press_cb), NULL);
#ifdef GGZ_CLIENT
    if (!main_id && ggz_network_mode && network_is_host ()) {
      main_id = g_timeout_add (GAMEDELAY * (properties->gamespeed + NETDELAY),
			       (GSourceFunc) network_loop, NULL);
    } else
#endif
    if (!main_id && !ggz_network_mode) {
      main_id = g_timeout_add (GAMEDELAY * properties->gamespeed,
			       (GSourceFunc) main_loop, NULL);
    }
#ifdef GGZ_CLIENT
    if (!add_bonus_id && network_is_host ()) {
#else
    if (!add_bonus_id) {
#endif
      add_bonus_id = g_timeout_add (BONUSDELAY *
				    properties->gamespeed,
				    (GSourceFunc) add_bonus_cb, NULL);
    }
  }

  dummy_id = 0;

  return (FALSE);
}

gint
new_game (void)
{
  gtk_action_set_sensitive (new_network_action, FALSE);

  if (ggz_network_mode) {
    gtk_action_set_sensitive (pause_action, FALSE);
  } else {
    gtk_action_set_sensitive (pause_action, TRUE);
  }
  gtk_action_set_sensitive (end_game_action, TRUE);
  gtk_action_set_sensitive (preferences_action, !ggz_network_mode);

  if (game_running ()) {
    end_game (FALSE);
    main_id = 0;
  }

  gnibbles_init ();

  if (ggz_network_mode || !properties->random) {
    current_level = properties->startlevel;
  } else {
    current_level = rand () % MAXLEVEL + 1;
  }

  zero_board ();
  gnibbles_load_level (GTK_WIDGET (window), current_level);

  gnibbles_add_bonus (1);

  paused = 0;
  gtk_action_set_visible (pause_action, !paused);
  gtk_action_set_visible (resume_action, paused);
  gtk_action_set_visible (player_list_action, ggz_network_mode);

  if (erase_id) {
    g_source_remove (erase_id);
    erase_id = 0;
  }

  if (restart_id) {
    g_source_remove (restart_id);
    restart_id = 0;
  }

  if (add_bonus_id) {
    g_source_remove (add_bonus_id);
    add_bonus_id = 0;
  }

  if (dummy_id)
    g_source_remove (dummy_id);

  dummy_id = g_timeout_add (1500, (GSourceFunc) new_game_2_cb, NULL);

  network_gui_update ();

  return TRUE;
}

static void
new_game_cb (GtkAction * action, gpointer data)
{
  new_game ();
}


gint
pause_game_cb (GtkAction * action, gpointer data)
{
  if (paused) {
    paused = 0;
    dummy_id = g_timeout_add (500, (GSourceFunc) new_game_2_cb, NULL);
    /*
     * main_id = gtk_timeout_add (GAMEDELAY * properties->gamespeed,
     * (GtkFunction) main_loop, NULL);
     * keyboard_id = gtk_signal_connect (GTK_OBJECT (window),
     * "key_press_event", GTK_SIGNAL_FUNC (key_press_cb),
     * NULL);
     * add_bonus_id = gtk_timeout_add (BONUSDELAY *
     * properties->gamespeed,
     * (GtkFunction) add_bonus_cb,
     * NULL);
     */
  } else {
    if (main_id || erase_id || restart_id || dummy_id) {
      paused = 1;
      if (main_id) {
	g_source_remove (main_id);
	main_id = 0;
      }
      if (keyboard_id) {
	g_signal_handler_disconnect (G_OBJECT (window), keyboard_id);
	keyboard_id = 0;
      }
      if (add_bonus_id) {
	g_source_remove (add_bonus_id);
	add_bonus_id = 0;
      }
    }
  }
  gtk_action_set_sensitive (pause_action, !paused);
  gtk_action_set_sensitive (resume_action, paused);
  gtk_action_set_visible (pause_action, !paused);
  gtk_action_set_visible (resume_action, paused);

  return TRUE;
}

static void
show_scores_cb (GtkAction * action, gpointer data)
{
  gnibbles_show_scores (window, 0);
}

void
end_game (gboolean show_splash)
{
  if (main_id) {
    g_source_remove (main_id);
    main_id = 0;
  }

  if (keyboard_id) {
    g_signal_handler_disconnect (G_OBJECT (window), keyboard_id);
    keyboard_id = 0;
  }

  if (add_bonus_id) {
    g_source_remove (add_bonus_id);
    add_bonus_id = 0;
  }

  if (erase_id) {
    g_source_remove (erase_id);
    erase_id = 0;
  }

  if (dummy_id) {
    g_source_remove (dummy_id);
    dummy_id = 0;
  }

  if (restart_id) {
    g_source_remove (restart_id);
    restart_id = 0;
  }

  if (show_splash) {
    render_logo ();
    gtk_action_set_sensitive (new_network_action, TRUE);
    gtk_action_set_sensitive (pause_action, FALSE);
    gtk_action_set_sensitive (resume_action, FALSE);
    gtk_action_set_sensitive (end_game_action, FALSE);
    gtk_action_set_sensitive (preferences_action, TRUE);
  }

  network_gui_update ();
  paused = 0;

}

static gint
end_game_cb (GtkAction * action, gpointer data)
{

#ifdef GGZ_CLIENT
  if (ggz_network_mode) {
    ggz_embed_leave_table ();
    gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), NETWORK_PAGE);
  }
#endif

  end_game (TRUE);
  return (FALSE);
}

static gint
add_bonus_cb (gpointer data)
{
  gnibbles_add_bonus (0);

  return (TRUE);
}

static gint
restart_game (gpointer data)
{
  zero_board ();
  gnibbles_load_level (GTK_WIDGET (window), current_level);

  gnibbles_add_bonus (1);

  dummy_id = g_timeout_add (1500, (GSourceFunc) new_game_2_cb, NULL);

  restart_id = 0;

  return (FALSE);
}

static gint
erase_worms_cb (gpointer datap)
{
  gint data = GPOINTER_TO_INT (datap);

  if (data == 0) {
    erase_id = 0;
    if (!restart_id)
      end_game (TRUE);
  } else {
    gnibbles_undraw_worms (ERASESIZE - data);
    erase_id = g_timeout_add (ERASETIME / ERASESIZE,
			      (GSourceFunc) erase_worms_cb,
			      GINT_TO_POINTER (data - 1));
  }

  return (FALSE);
}

gint
main_loop (gpointer data)
{
  gint status;
  gint tmp, winner;
  gchar *str = NULL;

  status = gnibbles_move_worms ();
  gnibbles_scoreboard_update (scoreboard);

  if (status == VICTORY) {
    end_game (TRUE);
    winner = gnibbles_get_winner ();
    if (winner == -1)
      return FALSE;

    str = g_strdup_printf (_("Game over! The game has been won by %s!"),
			   names[winner]);
#ifdef GGZ_CLIENT
    add_chat_text (str);
#endif
    g_free (str);

    if (keyboard_id) {
      g_signal_handler_disconnect (G_OBJECT (window), keyboard_id);
      keyboard_id = 0;
    }
    if (main_id) {
      g_source_remove (main_id);
      main_id = 0;
    }
    if (add_bonus_id) {
      g_source_remove (add_bonus_id);
    }
    add_bonus_id = 0;
    erase_id = g_timeout_add (3000,
			      (GSourceFunc) erase_worms_cb,
			      (gpointer) ERASESIZE);
    gnibbles_log_score (window);

    return FALSE;


  }

  if (status == GAMEOVER) {

    if (keyboard_id) {
      g_signal_handler_disconnect (G_OBJECT (window), keyboard_id);
      keyboard_id = 0;
    }
    main_id = 0;
    if (add_bonus_id) {
      g_source_remove (add_bonus_id);
    }
    add_bonus_id = 0;
    erase_id = g_timeout_add (3000,
			      (GSourceFunc) erase_worms_cb,
			      (gpointer) ERASESIZE);
    gnibbles_log_score (window);
    return (FALSE);
  }

  if (status == NEWROUND) {
#ifdef GGZ_CLIENT
    if (ggz_network_mode) {
      end_game (TRUE);
      add_chat_text (_("The game is over."));
      return FALSE;
    }
#endif

    if (keyboard_id) {
      g_signal_handler_disconnect (G_OBJECT (window), keyboard_id);
      keyboard_id = 0;
    }
    if (add_bonus_id) {
      g_source_remove (add_bonus_id);
    }
    if (main_id) {
      g_source_remove (main_id);
      main_id = 0;
    }
    add_bonus_id = 0;
    erase_id = g_timeout_add (ERASETIME / ERASESIZE,
			      (GSourceFunc) erase_worms_cb,
			      (gpointer) ERASESIZE);
    restart_id = g_timeout_add (1000, (GSourceFunc) restart_game, NULL);
    return (FALSE);
  }

  if (boni->numleft == 0) {
    if (restart_id) {
      return TRUE;
    }
    if (keyboard_id) {
      g_signal_handler_disconnect (G_OBJECT (window), keyboard_id);
    }
    keyboard_id = 0;
    if (add_bonus_id) {
      g_source_remove (add_bonus_id);
    }
    add_bonus_id = 0;
    if (main_id) {
      g_source_remove (main_id);
      main_id = 0;
    }
    if ((current_level < MAXLEVEL) && (!properties->random
				       || ggz_network_mode))
      current_level++;
    else if (properties->random && !ggz_network_mode) {
      tmp = rand () % MAXLEVEL + 1;
      while (tmp == current_level)
	tmp = rand () % MAXLEVEL + 1;
      current_level = tmp;
    }
    restart_id = g_timeout_add (1000, (GSourceFunc) restart_game, NULL);
    return (FALSE);
  }

  return (TRUE);
}

static gboolean
show_cursor_cb (GtkWidget * widget, GdkEventMotion * event, gpointer data)
{
  show_cursor ();
  return FALSE;
}

static void
help_cb (GtkAction * action, gpointer data)
{
  gnome_help_display ("gnibbles.xml", NULL, NULL);
}

static const GtkActionEntry action_entry[] = {
  {"GameMenu", NULL, N_("_Game")},
  {"ViewMenu", NULL, N_("_View")},
  {"SettingsMenu", NULL, N_("_Settings")},
  {"HelpMenu", NULL, N_("_Help")},
  {"NewGame", GAMES_STOCK_NEW_GAME, NULL, NULL, NULL,
   G_CALLBACK (new_game_cb)},
#ifdef GGZ_CLIENT
  {"NewNetworkGame", GAMES_STOCK_NETWORK_GAME, NULL, NULL, NULL,
   G_CALLBACK (on_network_game)},
#else
  {"NewNetworkGame", GAMES_STOCK_NETWORK_GAME, NULL, NULL, NULL, NULL},
#endif
  {"PlayerList", GAMES_STOCK_PLAYER_LIST, NULL, NULL, NULL,
   G_CALLBACK (on_player_list)},
  {"Pause", GAMES_STOCK_PAUSE_GAME, NULL, NULL, NULL,
   G_CALLBACK (pause_game_cb)},
  {"Resume", GAMES_STOCK_RESUME_GAME, NULL, NULL, NULL,
   G_CALLBACK (pause_game_cb)},
  {"EndGame", GAMES_STOCK_END_GAME, NULL, NULL, NULL,
   G_CALLBACK (end_game_cb)},
  {"Scores", GAMES_STOCK_SCORES, NULL, NULL, NULL,
   G_CALLBACK (show_scores_cb)},
  {"Quit", GTK_STOCK_QUIT, NULL, NULL, NULL, G_CALLBACK (quit_cb)},
  {"Fullscreen", GAMES_STOCK_FULLSCREEN, NULL, NULL, NULL,
   G_CALLBACK (fullscreen_cb)},
  {"LeaveFullscreen", GAMES_STOCK_LEAVE_FULLSCREEN, NULL, NULL, NULL,
   G_CALLBACK (fullscreen_cb)},
  {"Preferences", GTK_STOCK_PREFERENCES, NULL, NULL, NULL,
   G_CALLBACK (gnibbles_preferences_cb)},
  {"Contents", GAMES_STOCK_CONTENTS, NULL, NULL, NULL, G_CALLBACK (help_cb)},
  {"About", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK (about_cb)}
};

static const char ui_description[] =
  "<ui>"
  "  <menubar name='MainMenu'>"
  "    <menu action='GameMenu'>"
  "      <menuitem action='NewGame'/>"
  "      <menuitem action='NewNetworkGame'/>"
  "      <menuitem action='PlayerList'/>"
  "      <menuitem action='EndGame'/>"
  "      <separator/>"
  "      <menuitem action='Pause'/>"
  "      <menuitem action='Resume'/>"
  "      <separator/>"
  "      <menuitem action='Scores'/>"
  "      <separator/>"
  "      <menuitem action='Quit'/>"
  "    </menu>"
  "    <menu action='ViewMenu'>"
  "      <menuitem action='Fullscreen'/>"
  "      <menuitem action='LeaveFullscreen'/>"
  "    </menu>"
  "    <menu action='SettingsMenu'>"
  "      <menuitem action='Preferences'/>"
  "    </menu>"
  "    <menu action='HelpMenu'>"
  "      <menuitem action='Contents'/>"
  "      <menuitem action='About'/>" "    </menu>" "  </menubar>" "</ui>";

static void
create_menus (GtkUIManager * ui_manager)
{
  GtkActionGroup *action_group;

  action_group = gtk_action_group_new ("group");

  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (action_group, action_entry,
				G_N_ELEMENTS (action_entry), window);

  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL);

  new_game_action = gtk_action_group_get_action (action_group, "NewGame");
  scores_action = gtk_action_group_get_action (action_group, "Scores");
  end_game_action = gtk_action_group_get_action (action_group, "EndGame");
  pause_action = gtk_action_group_get_action (action_group, "Pause");
  resume_action = gtk_action_group_get_action (action_group, "Resume");
  preferences_action =
    gtk_action_group_get_action (action_group, "Preferences");
  fullscreen_action =
    gtk_action_group_get_action (action_group, "Fullscreen");
  leave_fullscreen_action =
    gtk_action_group_get_action (action_group, "LeaveFullscreen");
  new_network_action =
    gtk_action_group_get_action (action_group, "NewNetworkGame");
#ifndef GGZ_CLIENT
  gtk_action_set_sensitive (new_network_action, FALSE);
#endif
  player_list_action =
    gtk_action_group_get_action (action_group, "PlayerList");


}

static void
setup_window (void)
{
  GdkPixmap *cursor_dot_pm;
  GtkWidget *vbox;
  GtkWidget *packing;
  GtkWidget *menubar;
  GtkUIManager *ui_manager;
  GtkAccelGroup *accel_group;

  window = gnome_app_new ("gnibbles", "Nibbles");

  gtk_window_set_default_size (GTK_WINDOW (window), DEFAULT_WIDTH, DEFAULT_HEIGHT);
  games_conf_add_window (GTK_WINDOW (window), KEY_PREFERENCES_GROUP);

  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect (G_OBJECT (window), "delete_event",
		    G_CALLBACK (delete_cb), NULL);
  g_signal_connect (G_OBJECT (window), "window_state_event",
		    G_CALLBACK (window_state_cb), NULL);

  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);

  games_stock_init ();
  ui_manager = gtk_ui_manager_new ();
  create_menus (ui_manager);
  set_fullscreen_actions (FALSE);
  notebook = gtk_notebook_new ();
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);

  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  menubar = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);

  packing = games_grid_frame_new (BOARDWIDTH, BOARDHEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox), packing, TRUE, TRUE, 0);
  gtk_widget_show (packing);

  drawing_area = gtk_drawing_area_new ();

  cursor_dot_pm = gdk_pixmap_create_from_data (window->window, "\0", 1, 1, 1,
					       &drawing_area->style->
					       fg[GTK_STATE_ACTIVE],
					       &drawing_area->style->
					       bg[GTK_STATE_ACTIVE]);

  pointers.invisible =
    gdk_cursor_new_from_pixmap (cursor_dot_pm, cursor_dot_pm,
				&drawing_area->style->fg[GTK_STATE_ACTIVE],
				&drawing_area->style->bg[GTK_STATE_ACTIVE], 0,
				0);

  gtk_container_add (GTK_CONTAINER (packing), drawing_area);
#ifdef GGZ_CLIENT
  chat = create_chat_widget ();
  gtk_box_pack_start (GTK_BOX (vbox), chat, FALSE, TRUE, 0);
#endif

  g_signal_connect (G_OBJECT (drawing_area), "configure_event",
		    G_CALLBACK (configure_event_cb), NULL);

  g_signal_connect (G_OBJECT (drawing_area), "motion_notify_event",
		    G_CALLBACK (show_cursor_cb), NULL);

  g_signal_connect (G_OBJECT (window), "focus_out_event",
		    G_CALLBACK (show_cursor_cb), NULL);

  gtk_widget_set_size_request (GTK_WIDGET (drawing_area),
			       BOARDWIDTH * 5, BOARDHEIGHT * 5);
  g_signal_connect (G_OBJECT (drawing_area), "expose_event",
		    G_CALLBACK (expose_event_cb), NULL);
  /* We do our own double-buffering. */
  gtk_widget_set_double_buffered (GTK_WIDGET (drawing_area), FALSE);
  gtk_widget_set_events (drawing_area, GDK_BUTTON_PRESS_MASK |
			 GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK);

  gnome_app_set_contents (GNOME_APP (window), notebook);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, NULL);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), MAIN_PAGE);

  gtk_widget_show_all (window);
#ifdef GGZ_CLIENT
  gtk_widget_hide (chat);
#endif
  gtk_widget_show (drawing_area);

  appbar = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_USER);
  gnome_app_set_statusbar (GNOME_APP (window), appbar);

  scoreboard = gnibbles_scoreboard_new (appbar);

}

static void
render_logo (void)
{
  PangoContext *context;
  PangoLayout *layout;
  PangoFontDescription * pfd;
  int size;
  static int width, height;

  zero_board ();

  gdk_draw_pixbuf (GDK_DRAWABLE (buffer_pixmap),
		   drawing_area->style->
		   fg_gc[GTK_WIDGET_STATE (drawing_area)], logo_pixmap, 0, 0,
		   0, 0, BOARDWIDTH * properties->tilesize,
		   BOARDHEIGHT * properties->tilesize, GDK_RGB_DITHER_NORMAL,
		   0, 0);

  context = gdk_pango_context_get ();
  layout = pango_layout_new (context);
  pfd = pango_context_get_font_description (context);
  size = pango_font_description_get_size (pfd);
  pango_font_description_set_size (pfd, 
		(size * drawing_area->allocation.width) / 100);
  pango_font_description_set_family (pfd, "Sans");
  pango_font_description_set_weight(pfd, PANGO_WEIGHT_BOLD); 
  pango_layout_set_font_description (layout, pfd);
  pango_layout_set_text (layout, _("Nibbles"), -1);
  pango_layout_get_pixel_size(layout, &width, &height);  

  gdk_draw_layout (GDK_DRAWABLE (buffer_pixmap), drawing_area->style->black_gc,  
		   (drawing_area->allocation.width - width) * 0.5 + 3, 
		   (drawing_area->allocation.height * 0.72) + 3, layout);
  gdk_draw_layout (GDK_DRAWABLE (buffer_pixmap), drawing_area->style->white_gc,  
		   (drawing_area->allocation.width - width) * 0.5, 
		   (drawing_area->allocation.height * 0.72), layout);

  pango_font_description_set_size (pfd, 
		(size * drawing_area->allocation.width) / 400);
  pango_layout_set_font_description (layout, pfd);
  /* Translators: This string will be included in the intro screen, so don't make sure it fits! */
  pango_layout_set_text (layout, _("A worm game for GNOME."), -1);
  pango_layout_get_pixel_size(layout, &width, &height);  

  gdk_draw_layout (GDK_DRAWABLE (buffer_pixmap), drawing_area->style->black_gc,
		   (drawing_area->allocation.width - width) * 0.5 + 2, 
                   (drawing_area->allocation.height * 0.94) + 2, layout);
  gdk_draw_layout (GDK_DRAWABLE (buffer_pixmap), drawing_area->style->white_gc,
		   (drawing_area->allocation.width - width) * 0.5,
                   (drawing_area->allocation.height * 0.94), layout);


  gdk_draw_drawable (GDK_DRAWABLE (drawing_area->window),
		     drawing_area->style->
		     fg_gc[GTK_WIDGET_STATE (drawing_area)], buffer_pixmap, 
		     0, 0, 0, 0, BOARDWIDTH * properties->tilesize,
		     BOARDHEIGHT * properties->tilesize);


}

int
main (int argc, char **argv)
{
  GnomeProgram *program;
  GOptionContext *context;

  setgid_io_init ();

  bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  g_thread_init (NULL);
  context = g_option_context_new (NULL);
  games_sound_add_option_group (context);

  program = gnome_program_init ("gnibbles", VERSION, LIBGNOMEUI_MODULE,
				argc, argv,
				GNOME_PARAM_GOPTION_CONTEXT, context,
				GNOME_PARAM_POPT_TABLE, NULL,
				GNOME_PARAM_APP_DATADIR, REAL_DATADIR, NULL);
  gtk_window_set_default_icon_name ("gnome-gnibbles");
  srand (time (NULL));

  highscores = games_scores_new (&scoredesc);

  games_conf_initialise ("Gnibbles");

  properties = gnibbles_properties_new ();

  setup_window ();

  gnibbles_load_logo (window);
  gnibbles_load_pixmap (window);

  gtk_widget_show (window);

  buffer_pixmap = gdk_pixmap_new (drawing_area->window,
				  BOARDWIDTH * properties->tilesize,
				  BOARDHEIGHT * properties->tilesize, -1);
#ifdef GGZ_CLIENT
  network_init ();
  network_gui_update ();
#endif

  render_logo ();

  gtk_action_set_sensitive (pause_action, FALSE);
  gtk_action_set_sensitive (resume_action, FALSE);
  gtk_action_set_sensitive (end_game_action, FALSE);
  gtk_action_set_visible (resume_action, paused);
  gtk_action_set_visible (new_game_action, !ggz_network_mode);
  gtk_action_set_visible (player_list_action, ggz_network_mode);

  gtk_main ();

  gnibbles_properties_destroy (properties);

  games_conf_shutdown ();

  g_object_unref (program);

  return 0;
}
