/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */

/*
 *   Gnome Nibbles: Gnome Worm Game
 *   Written by Sean MacIsaac <sjm@acm.org>, Ian Peters <itp@gnu.org>,
 *              Guillaume Beland <guillaume.beland@gmail.com>
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

#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <locale.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "main.h"
#include "properties.h"
#include "gnibbles.h"
#include "bonus.h"
#include "boni.h"
#include "preferences.h"
#include "scoreboard.h"
#include "warp.h"
#include "games-gridframe.h"
#include "games-scores.h"

#include <clutter-gtk/clutter-gtk.h>
#include <clutter/clutter.h>

#include "board.h"
#include "worm.h"

#define DEFAULT_WIDTH 650
#define DEFAULT_HEIGHT 520

GSettings *settings;
GSettings *worm_settings[NUMWORMS];
GtkWidget *window;
GtkWidget *statusbar;
GtkWidget *notebook;
GtkWidget *chat = NULL;

static const GamesScoresCategory scorecats[] = {
{ "4.0", NC_("game speed", "Beginner")            },
{ "3.0", NC_("game speed", "Slow")                },
{ "2.0", NC_("game speed", "Medium")              },
{ "1.0", NC_("game speed", "Fast")                },
{ "4.1", NC_("game speed", "Beginner with Fakes") },
{ "3.1", NC_("game speed", "Slow with Fakes")     },
{ "2.1", NC_("game speed", "Medium with Fakes")   },
{ "1.1", NC_("game speed", "Fast with Fakes")     }
};

GamesScores *highscores;

GnibblesProperties *properties;

GnibblesBoard *board;

GnibblesScoreboard *scoreboard;

GtkWidget *clutter_widget;
ClutterActor *stage;

extern GnibblesWorm *worms[];
extern GnibblesBoni *boni;

gint main_id = 0;
gint dummy_id = 0;
gint keyboard_id = 0;
gint add_bonus_id = 0;
gint restart_id = 0;

gint current_level;

gboolean is_paused;
static gboolean new_game_2_cb (GtkWidget * widget, gpointer data);

static gint add_bonus_cb (gpointer data);

static GSimpleAction *pause_action;

gint
game_running (void)
{
  return (main_id || dummy_id || restart_id || is_paused);
}

/* Avoid a race condition where a redraw is attempted
 * between the window being destroyed and the destroy
 * event being sent. */
static gint
delete_cb (GtkWidget * widget, gpointer data)
{
  if (main_id)
    g_source_remove (main_id);
  if (dummy_id)
    g_source_remove (dummy_id);
  if (restart_id)
    g_source_remove (restart_id);

  return FALSE;
}

static void
activate_toggle (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  GVariant *state;

  state = g_action_get_state (G_ACTION (action));
  g_action_change_state (G_ACTION (action), g_variant_new_boolean (!g_variant_get_boolean (state)));
  g_variant_unref (state);
}

static void
change_pause_state (GSimpleAction *action,
                         GVariant      *state,
                         gpointer       user_data)
{
  if (!is_paused) {  //If it's not currently paused, pause the game
    is_paused = TRUE;
    if (main_id || restart_id || dummy_id) {
      if (main_id) {
        g_source_remove (main_id);
        main_id = 0;
      }
      if (keyboard_id) {
        g_signal_handler_disconnect (G_OBJECT (stage), keyboard_id);
        keyboard_id = 0;
      }
      if (add_bonus_id) {
        g_source_remove (add_bonus_id);
        add_bonus_id = 0;
      }
    }
  }
  else {  //Resume the game
    is_paused = FALSE;
    dummy_id = g_timeout_add (500, (GSourceFunc) new_game_2_cb, NULL);
  }

  g_simple_action_set_state (action, g_variant_new_boolean (is_paused));
}


static void
newgame_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  new_game ();
}

static void
scores_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  gnibbles_show_scores (window, 0);
}

static void
preferences_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  gnibbles_preferences_cb (window, user_data);
}

static void
help_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  GError *error = NULL;

  gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (window)), "help:gnome-nibbles", gtk_get_current_event_time (), &error);
  if (error)
    g_warning ("Failed to show help: %s", error->message);
  g_clear_error (&error);
}



static void
about_activated (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  const gchar *authors[] = { "Sean MacIsaac", "Ian Peters", "Andreas Røsdal",
                             "Guillaume Beland", NULL };

  const gchar *documenters[] = { "Kevin Breit", NULL };

  gtk_show_about_dialog (GTK_WINDOW (window),
       "program-name", _("Nibbles"),
       "version", VERSION,
       "copyright",
       "Copyright © 1999–2008 Sean MacIsaac, Ian Peters, Andreas Røsdal\n"
       "Copyright © 2009 Guillaume Beland",
       "license-type", GTK_LICENSE_GPL_2_0, 
       "comments", _("A worm game for GNOME"),
       "authors", authors,
       "documenters", documenters, 
       "translator-credits", _("translator-credits"), 
       "logo-icon-name", "gnome-nibbles", 
       "website", "https://wiki.gnome.org/Apps/Nibbles",
       NULL);
}

static void
quit_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  GApplication *app = user_data;

  g_application_quit (app);
}

static GActionEntry app_entries[] = {
  { "newgame", newgame_activated, NULL, NULL, NULL },
  { "pause", activate_toggle, NULL, "false", change_pause_state},
  { "preferences", preferences_activated, NULL, NULL, NULL },
  { "scores", scores_activated, NULL, NULL, NULL },
  { "help", help_activated, NULL, NULL, NULL },
  { "about", about_activated, NULL, NULL, NULL },
  { "quit", quit_activated, NULL, NULL, NULL },
};

static gboolean
key_press_cb (ClutterActor *actor, ClutterEvent *event, gpointer data)
{

  if (!(event->type == CLUTTER_KEY_PRESS))
    return FALSE;

  return gnibbles_keypress_worms (event->key.keyval);
}

static gboolean
configure_event_cb (GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
  int tilesize, ts_x, ts_y;
  int i;

  /* Compute the new tile size based on the size of the
   * drawing area, rounded down. */
  ts_x = event->width / BOARDWIDTH;
  ts_y = event->height / BOARDHEIGHT;
  if (ts_x * BOARDWIDTH > event->width)
    ts_x--;
  if (ts_y * BOARDHEIGHT > event->height)
    ts_y--;
  tilesize = MIN (ts_x, ts_y);

  if (tilesize != properties->tilesize)
  {	
    gnibbles_load_pixmap (tilesize);

    clutter_actor_set_size (CLUTTER_ACTOR (stage),
                          BOARDWIDTH * tilesize,
                          BOARDHEIGHT * tilesize);

    gnibbles_board_rescale (board, tilesize);

    if (game_running ()) {
      gnibbles_boni_rescale (boni, tilesize);

      for (i=0; i<properties->numworms; i++)
        gnibbles_worm_rescale (worms[i], tilesize);

      if (warpmanager)
        gnibbles_warpmanager_rescale (warpmanager, tilesize);
    }

    properties->tilesize = tilesize;
    gnibbles_properties_set_tile_size (tilesize);
  }

  return FALSE;
}

static gboolean
new_game_2_cb (GtkWidget * widget, gpointer data)
{
    if (!keyboard_id)
      keyboard_id = g_signal_connect (G_OBJECT (stage),
                                      "key-press-event",
                                      G_CALLBACK (key_press_cb), NULL);
    if (!main_id) {
      main_id = g_timeout_add (GAMEDELAY * properties->gamespeed,
                               (GSourceFunc) main_loop, NULL);
    }
    if (!add_bonus_id) {
      add_bonus_id = g_timeout_add (BONUSDELAY *
                                    properties->gamespeed,
                                    (GSourceFunc) add_bonus_cb, NULL);
    }

  dummy_id = 0;

  return FALSE;
}

gboolean
new_game (void)
{
  int i;
  g_simple_action_set_enabled (pause_action, TRUE);

  if (game_running ()) {
    end_game ();
  }

  if (!properties->random) {
    current_level = properties->startlevel;
  } else {
    current_level = rand () % MAXLEVEL + 1;
  }

  gnibbles_init ();
  gnibbles_board_level_new (board, current_level);
  gnibbles_board_level_add_bonus (board, 1);

  for (i = 0; i < properties->numworms; i++) {
    if (!clutter_actor_get_stage (worms[i]->actors))
      clutter_actor_add_child (stage, worms[i]->actors);
    gnibbles_worm_show (worms[i]);
  }

  is_paused = FALSE;

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

  dummy_id = g_timeout_add_seconds (1, (GSourceFunc) new_game_2_cb, NULL);

  return TRUE;
}

void
end_game (void)
{
  if (main_id) {
    g_source_remove (main_id);
    main_id = 0;
  }

  if (keyboard_id) {
    g_signal_handler_disconnect (G_OBJECT (stage), keyboard_id);
    keyboard_id = 0;
  }

  if (add_bonus_id) {
    g_source_remove (add_bonus_id);
    add_bonus_id = 0;
  }

  if (dummy_id) {
    g_source_remove (dummy_id);
    dummy_id = 0;
  }

  if (restart_id) {
    g_source_remove (restart_id);
    restart_id = 0;
  }

  animate_end_game ();

  g_simple_action_set_enabled (pause_action, FALSE);

  is_paused = FALSE;
}

static gboolean
add_bonus_cb (gpointer data)
{
  gnibbles_board_level_add_bonus (board, 0);
  return TRUE;
}

static gboolean
restart_game (gpointer data)
{
  int i;

  gnibbles_board_level_new (board, current_level);
  gnibbles_board_level_add_bonus (board, 1);

  for (i = 0; i < properties->numworms; i++) {
    if (!clutter_actor_get_stage (worms[i]->actors))
      clutter_actor_add_child (stage, worms[i]->actors);
    gnibbles_worm_show (worms[i]);
  }

  for (i = 0; i < properties->human; i++)
    worms[i]->human = TRUE;

  dummy_id = g_timeout_add_seconds (1, (GSourceFunc) new_game_2_cb, NULL);
  restart_id = 0;

  return FALSE;
}

void
animate_end_game (void)
{
  int i;
  for (i = 0; i < properties->numworms; i++) {
    clutter_actor_save_easing_state (worms[i]->actors);
    clutter_actor_set_easing_mode (worms[i]->actors, CLUTTER_EASE_IN_QUAD);
    clutter_actor_set_easing_duration (worms[i]->actors, GAMEDELAY * 15);
    clutter_actor_set_scale (worms[i]->actors, 0.4, 0.4);
    clutter_actor_set_opacity (worms[i]->actors, 0);
    clutter_actor_restore_easing_state (worms[i]->actors);
  }

  for (i = 0; i < boni->numbonuses; i++) {
    clutter_actor_save_easing_state(boni->bonuses[i]->actor);
    clutter_actor_set_easing_mode (boni->bonuses[i]->actor, CLUTTER_EASE_IN_QUAD);
    clutter_actor_set_easing_duration (boni->bonuses[i]->actor, GAMEDELAY * 15);
    clutter_actor_set_scale (boni->bonuses[i]->actor, 0.4, 0.4);
    clutter_actor_set_pivot_point (boni->bonuses[i]->actor,.5,.5);
    clutter_actor_set_opacity (boni->bonuses[i]->actor, 0);
    clutter_actor_restore_easing_state(boni->bonuses[i]->actor);
  }

  for (i = 0; i < warpmanager->numwarps; i++) {
    clutter_actor_save_easing_state (warpmanager->warps[i]->actor);
    clutter_actor_set_easing_mode (warpmanager->warps[i]->actor, CLUTTER_EASE_IN_QUAD);
    clutter_actor_set_easing_duration (warpmanager->warps[i]->actor, GAMEDELAY * 15);
    clutter_actor_set_scale (warpmanager->warps[i]->actor, 0.4, 0.4);
    clutter_actor_set_pivot_point (warpmanager->warps[i]->actor,.5,.5);
    clutter_actor_set_opacity (warpmanager->warps[i]->actor, 0);
    clutter_actor_restore_easing_state (warpmanager->warps[i]->actor);
  }

  clutter_actor_save_easing_state (board->level);
  clutter_actor_set_easing_mode (board->level, CLUTTER_EASE_IN_QUAD);
  clutter_actor_set_easing_duration (board->level, GAMEDELAY * 20);
  clutter_actor_set_scale (board->level, 0.4, 0.4);
  clutter_actor_set_pivot_point (board->level,.5,.5);
  clutter_actor_set_opacity (board->level, 0);
  clutter_actor_restore_easing_state (board->level);
}

gboolean
main_loop (gpointer data)
{
  gint status;
  gint tmp, winner;
  gchar *str = NULL;

  status = gnibbles_move_worms ();
  gnibbles_scoreboard_update (scoreboard);

  if (status == VICTORY) {
    end_game ();
    winner = gnibbles_get_winner ();

    if (winner == -1)
      return FALSE;

    gnibbles_log_score (window);

    return FALSE;
  }

  if (status == GAMEOVER) {
    end_game ();

    gnibbles_log_score (window);

    return FALSE;
  }

  if (status == NEWROUND) {
    if (keyboard_id) {
      g_signal_handler_disconnect (G_OBJECT (stage), keyboard_id);
      keyboard_id = 0;
    }
    if (add_bonus_id)
      g_source_remove (add_bonus_id);

    if (main_id) {
      g_source_remove (main_id);
      main_id = 0;
    }
    add_bonus_id = 0;

    animate_end_game ();
    restart_id = g_timeout_add_seconds (1, (GSourceFunc) restart_game, NULL);

    return FALSE;
  }

  if (boni->numleft == 0) {
    if (restart_id)
      return TRUE;

    if (keyboard_id)
      g_signal_handler_disconnect (G_OBJECT (stage), keyboard_id);

    keyboard_id = 0;

    if (add_bonus_id)
      g_source_remove (add_bonus_id);

    add_bonus_id = 0;
    if (main_id) {
      g_source_remove (main_id);
      main_id = 0;
    }
    if ((current_level < MAXLEVEL) && (!properties->random)) {
      current_level++;
    } else if (properties->random) {
      tmp = rand () % MAXLEVEL + 1;
      while (tmp == current_level)
        tmp = rand () % MAXLEVEL + 1;
      current_level = tmp;
    }
    animate_end_game ();
    restart_id = g_timeout_add_seconds (1, (GSourceFunc) restart_game, NULL);
    return FALSE;
  }

  return TRUE;
}

static void
activate (GtkApplication* app,
          gpointer        user_data)
{
  GtkWidget *headerbar;
  GtkWidget *label;

  GtkWidget *vbox;
  ClutterColor stage_color = {0x00,0x00,0x00,0xff};

  GtkWidget *packing;

  headerbar = gtk_header_bar_new ();
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (headerbar), TRUE);
  gtk_header_bar_set_title (GTK_HEADER_BAR (headerbar), _("Nibbles"));
  gtk_widget_show (headerbar);

  window = gtk_application_window_new (app);
  gtk_window_set_titlebar (GTK_WINDOW (window), headerbar);

  g_action_map_add_action_entries (G_ACTION_MAP (app), app_entries, G_N_ELEMENTS (app_entries), app);

  pause_action          = G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (app) , "pause"));

  g_simple_action_set_enabled (pause_action, FALSE);

  clutter_widget = gtk_clutter_embed_new ();
  stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (clutter_widget));

  clutter_actor_set_background_color (stage, &stage_color);

  clutter_actor_set_size (CLUTTER_ACTOR (stage),
                          properties->tilesize * BOARDWIDTH,
                          properties->tilesize * BOARDHEIGHT);
  gtk_widget_set_size_request (clutter_widget, DEFAULT_WIDTH, DEFAULT_HEIGHT);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  //Games Grid Frame Packing
  packing = games_grid_frame_new (BOARDWIDTH, BOARDHEIGHT);
  gtk_container_add (GTK_CONTAINER (packing), clutter_widget);
  gtk_box_pack_start (GTK_BOX (vbox), packing, FALSE, TRUE, 0);

  //Statusbar
  statusbar = gtk_statusbar_new ();
  gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, FALSE, 0);

  //Actually inits the board/scoreboard
  board = gnibbles_board_new ();
  scoreboard = gnibbles_scoreboard_new (statusbar);

  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show_all (window);

  g_signal_connect (G_OBJECT (clutter_widget), "configure_event",
                    G_CALLBACK (configure_event_cb), NULL);
}

static GOptionEntry entries[] =
{
  { "DEBUG_GAMEDELAY", 0, 0, G_OPTION_ARG_INT, &GAMEDELAY, "Built-in speed multiplier, lower is faster, default is 35", "D" },
  { NULL }
};


int
main (int argc, char **argv)
{
  int i;
  GtkApplication *app;
  int status;

  //Handle Command Line options
  GAMEDELAY = DEFAULTGAMEDELAY;
  GError *error = NULL;
  GOptionContext *context;

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  context = g_option_context_new ("");
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (FALSE));

  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_print ("option parsing failed: %s\n", error->message);
      exit (1);
    }
  //Done handling Command Line options

  if (gtk_clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS) {
    GtkWidget *dialog = gtk_message_dialog_new (NULL,
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_NONE,
                                                "%s", "Unable to initialize Clutter.");
    gtk_window_set_title (GTK_WINDOW (dialog), g_get_application_name ());
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    exit (1);
  }

  games_scores_startup ();

  g_set_application_name (_("Nibbles"));

  settings = g_settings_new ("org.gnome.nibbles");
  for (i = 0; i < NUMWORMS; i++)
  {
    gchar *name = g_strdup_printf ("org.gnome.nibbles.worm%d", i);
    worm_settings[i] = g_settings_new (name);
    g_free (name);
  }

  gtk_window_set_default_icon_name ("gnome-nibbles");
  srand (time (NULL));

  highscores = games_scores_new ("gnome-nibbles",
                                 scorecats, G_N_ELEMENTS (scorecats),
                                 "game speed", NULL,
                                 0 /* default category */,
                                 GAMES_SCORES_STYLE_PLAIN_DESCENDING);

  properties = gnibbles_properties_new ();
  gnibbles_load_pixmap (properties->tilesize);

  app = gtk_application_new ("org.gnome.nibbles", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
