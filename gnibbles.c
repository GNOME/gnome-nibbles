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

#include <stdlib.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <clutter/clutter.h>

#include <libgames-support/games-gtk-compat.h>
#include <libgames-support/games-runtime.h>
#include <libgames-support/games-scores-dialog.h>
#include <libgames-support/games-scores.h>
#include <libgames-support/games-sound.h>

#include "main.h"
#include "gnibbles.h"
#include "worm.h"
#include "boni.h"
#include "bonus.h"
#include "warpmanager.h"
#include "properties.h"
#include "scoreboard.h"
#include "board.h"
#include "level.h"

#include "worm-clutter.h"

#ifdef GGZ_CLIENT
#include "ggz-network.h"
#endif

GnibblesCWorm *cworms[NUMWORMS];

GnibblesWorm *worms[NUMWORMS];
GnibblesBoni *boni = NULL;
GnibblesWarpManager *warpmanager;

GdkPixmap *buffer_pixmap = NULL;
GdkPixbuf *logo_pixmap = NULL;
//old pixbuf
GdkPixbuf *bonus_pixmaps[9] = { NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL
};
GdkPixbuf *small_pixmaps[19] = { NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL
};

// clutter-related pixbuf
GdkPixbuf *wall_pixmaps[11] = { NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL,
  NULL
};

GdkPixbuf *worm_pixmaps[7] = { NULL, NULL, NULL, NULL, NULL,
  NULL, NULL
};

GdkPixbuf *boni_pixmaps[9] = { NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL
};

extern GtkWidget *drawing_area;

extern gchar board[BOARDWIDTH][BOARDHEIGHT];
extern GnibblesLevel *level;
extern GnibblesBoard *clutter_board;

extern GnibblesProperties *properties;

extern GnibblesScoreboard *scoreboard;

/*
extern guint properties->tilesize, properties->tilesize;
*/

static GdkPixbuf *
gnibbles_clutter_load_pixmap_file (const gchar * pixmap, gint xsize, gint ysize)
{
  GdkPixbuf *image;
  gchar *filename;
  const char *dirname;

  dirname = games_runtime_get_directory (GAMES_RUNTIME_GAME_PIXMAP_DIRECTORY);
  filename = g_build_filename (dirname, pixmap, NULL);

  if (!filename) {
    char *message =
      g_strdup_printf (_("Nibbles couldn't find pixmap file:\n%s\n\n"
			 "Please check your Nibbles installation"), pixmap);
    //gnibbles_error (window, message;
    g_free(message);
  }

  image = gdk_pixbuf_new_from_file_at_scale (filename, xsize, ysize, TRUE, NULL);
  g_free (filename);

  return image;
}

void 
gnibbles_clutter_load_pixmap (gint tilesize)
{
  gchar *bonus_files[] = {
    "blank.svg",
    "diamond.svg",
    "bonus1.svg",
    "bonus2.svg",
    "life.svg",
    "bonus3.svg",
    "bonus4.svg",
    "bonus5.svg",
    "questionmark.svg"
  };

  gchar *small_files[] = {
    "wall-straight-up.svg",
    "wall-straight-side.svg",
    "wall-corner-bottom-left.svg",
    "wall-corner-bottom-right.svg",
    "wall-corner-top-left.svg",
    "wall-corner-top-right.svg",
    "wall-tee-up.svg",
    "wall-tee-right.svg",
    "wall-tee-left.svg",
    "wall-tee-down.svg",
    "wall-cross.svg"
  };
  
  gchar *worm_files[] = {
    "snake-red.svg",
    "snake-green.svg",
    "snake-blue.svg",
    "snake-yellow.svg",
    "snake-cyan.svg",
    "snake-magenta.svg",
    "snake-grey.svg"
  };

  int i;

  for (i = 0; i < 9; i++) {
    if (boni_pixmaps[i])
      g_object_unref (boni_pixmaps[i]);

    boni_pixmaps[i] = gnibbles_clutter_load_pixmap_file (bonus_files[i],
  					                                             4 * tilesize,
                          						                   4 * tilesize);
  }

  for (i = 0; i < 11; i++) {
    if (wall_pixmaps[i])
      g_object_unref (wall_pixmaps[i]);
      
    wall_pixmaps[i] = gnibbles_clutter_load_pixmap_file (small_files[i],
  	 	  		                                             2 * tilesize,
                           						                   2 * tilesize);
  }

  for (i = 0; i < 7; i++) {
    if (worm_pixmaps[i])
      g_object_unref (worm_pixmaps[i]);

    worm_pixmaps[i] = gnibbles_clutter_load_pixmap_file (worm_files[i], 
                                                        tilesize, tilesize);
  }
}

static void
gnibbles_error (GtkWidget * window, gchar * message)
{
  GtkWidget *w =
    gtk_message_dialog_new (GTK_WINDOW (window), GTK_DIALOG_MODAL,
			    GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
			    "%s", message);
  gtk_dialog_run (GTK_DIALOG (w));
  gtk_widget_destroy (GTK_WIDGET (w));
  exit (1);
}

static GdkPixbuf *
gnibbles_load_pixmap_file (GtkWidget * window, const gchar * pixmap,
			   gint xsize, gint ysize)
{
  GdkPixbuf *image;
  gchar *filename;
  const char *dirname;

  dirname = games_runtime_get_directory (GAMES_RUNTIME_GAME_PIXMAP_DIRECTORY);
  filename = g_build_filename (dirname, pixmap, NULL);

  if (!filename) {
    char *message =
      g_strdup_printf (_("Nibbles couldn't find pixmap file:\n%s\n\n"
			 "Please check your Nibbles installation"), pixmap);
    gnibbles_error (window, message);
    /* We should never get here since the app exits in gnibbles_error. But let's
     * free it anyway in case someone comes along and changes gnibbles_error */
    g_free(message);
  }

  image = gdk_pixbuf_new_from_file_at_size (filename, xsize, ysize, NULL);
  g_free (filename);

  return image;
}

static void
gnibbles_copy_pixmap (GdkDrawable * drawable, gint which, gint x, gint y,
		      gboolean big)
{
  gint size = properties->tilesize * (big == TRUE ? 2 : 1);
  GdkPixbuf *copy_buf;

  if (big == TRUE) {
    if (which < 0 || which > 8) {
      g_warning ("Invalid bonus image %d\n", which);
      return;
    }
    copy_buf = bonus_pixmaps[which];
  } else {
    if (which < 0 || which > 19) {
      g_warning ("Invalid tile image %d\n", which);
      return;
    }
    copy_buf = small_pixmaps[which];
  }

  gdk_draw_pixbuf (GDK_DRAWABLE (drawable),
		   gtk_widget_get_style (drawing_area)->
		   fg_gc[gtk_widget_get_state (drawing_area)], copy_buf, 0, 0,
		   x * properties->tilesize, y * properties->tilesize, size,
		   size, GDK_RGB_DITHER_NORMAL, 0, 0);
}

void
gnibbles_draw_pixmap (gint which, gint x, gint y)
{
  gnibbles_copy_pixmap (gtk_widget_get_window (drawing_area), which, x, y, FALSE);
  gnibbles_copy_pixmap (buffer_pixmap, which, x, y, FALSE);
}

void
gnibbles_draw_big_pixmap (gint which, gint x, gint y)
{
  gnibbles_copy_pixmap (gtk_widget_get_window (drawing_area), which, x, y, TRUE);
  gnibbles_copy_pixmap (buffer_pixmap, which, x, y, TRUE);
}

void
gnibbles_draw_pixmap_buffer (gint which, gint x, gint y)
{
  gnibbles_copy_pixmap (buffer_pixmap, which, x, y, FALSE);
}

void
gnibbles_draw_big_pixmap_buffer (gint which, gint x, gint y)
{
  gnibbles_copy_pixmap (buffer_pixmap, which, x, y, TRUE);
}

void
gnibbles_load_logo (void)
{
  if (logo_pixmap)
    g_object_unref (logo_pixmap);

  logo_pixmap = gnibbles_clutter_load_pixmap_file ("gnibbles-logo.svg",
                               			            clutter_board->width, 
                                                clutter_board->height);
}

void
gnibbles_load_pixmap (GtkWidget * window)
{
  gchar *bonus_files[] = {
    "blank.svg",
    "diamond.svg",
    "bonus1.svg",
    "bonus2.svg",
    "life.svg",
    "bonus3.svg",
    "bonus4.svg",
    "bonus5.svg",
    "questionmark.svg"
  };

  gchar *small_files[] = {
    "wall-empty.svg",
    "wall-straight-up.svg",
    "wall-straight-side.svg",
    "wall-corner-bottom-left.svg",
    "wall-corner-bottom-right.svg",
    "wall-corner-top-left.svg",
    "wall-corner-top-right.svg",
    "wall-tee-up.svg",
    "wall-tee-right.svg",
    "wall-tee-left.svg",
    "wall-tee-down.svg",
    "wall-cross.svg",
    "snake-red.svg",
    "snake-green.svg",
    "snake-blue.svg",
    "snake-yellow.svg",
    "snake-cyan.svg",
    "snake-magenta.svg",
    "snake-grey.svg"
  };
  int i;

  for (i = 0; i < 9; i++) {
    if (bonus_pixmaps[i])
      g_object_unref (bonus_pixmaps[i]);
    bonus_pixmaps[i] = gnibbles_load_pixmap_file (window, bonus_files[i],
						  2 * properties->tilesize,
						  2 * properties->tilesize);
  }

  for (i = 0; i < 19; i++) {
    if (small_pixmaps[i])
      g_object_unref (small_pixmaps[i]);
    small_pixmaps[i] = gnibbles_load_pixmap_file (window, small_files[i],
						  properties->tilesize,
						  properties->tilesize);
  }
}

void
gnibbles_load_level (GtkWidget * window, gint level)
{
  gchar *tmp = NULL;
  const char *dirname;
  gchar *filename;
  FILE *in;
  gchar tmpboard[BOARDWIDTH + 2];
  gint i, j;
  gint count = 0;

  tmp = g_strdup_printf ("level%03d.gnl", level);

  dirname = games_runtime_get_directory (GAMES_RUNTIME_GAME_GAMES_DIRECTORY);
  filename = g_build_filename (dirname, tmp, NULL);

  g_free (tmp);
  if ((in = fopen (filename, "r")) == NULL) {
    char *message =
      g_strdup_printf (_
                       ("Nibbles couldn't load level file:\n%s\n\n"
                        "Please check your Nibbles installation"), filename);
    gnibbles_error (window, message);
    g_free (message);
  }

  g_free (filename);

  if (warpmanager)
    gnibbles_warpmanager_destroy (warpmanager);

  warpmanager = gnibbles_warpmanager_new ();

  if (boni)
    gnibbles_boni_destroy (boni);

  boni = gnibbles_boni_new ();

  for (i = 0; i < BOARDHEIGHT; i++) {
    if (!fgets (tmpboard, sizeof (tmpboard), in)) {
      char *message =
        g_strdup_printf (_
                         ("Level file appears to be damaged:\n%s\n\n"
                         "Please check your Nibbles installation"), filename);
      gnibbles_error (window, message);
      g_free (message);
      break;
    }
    for (j = 0; j < BOARDWIDTH; j++) {
      board[j][i] = tmpboard[j];
      switch (board[j][i]) {
      case 'm':
        board[j][i] = 'a';
        if (count < properties->numworms)
          gnibbles_worm_set_start (worms[count++], j, i, WORMUP);
        break;
      case 'n':
        board[j][i] = 'a';
        if (count < properties->numworms)
          gnibbles_worm_set_start (worms[count++], j, i, WORMLEFT);
        break;
      case 'o':
        board[j][i] = 'a';
        if (count < properties->numworms)
          gnibbles_worm_set_start (worms[count++], j, i, WORMDOWN);
        break;
      case 'p':
        board[j][i] = 'a';
        if (count < properties->numworms)
          gnibbles_worm_set_start (worms[count++], j, i, WORMRIGHT);
        break;
      case 'Q':
        gnibbles_warpmanager_add_warp (warpmanager, j - 1, i - 1, -1, -1);
        break;
      case 'R':
      case 'S':
      case 'T':
      case 'U':
      case 'V':
      case 'W':
      case 'X':
      case 'Y':
      case 'Z':
        gnibbles_warpmanager_add_warp
          (warpmanager, j - 1, i - 1, -board[j][i], 0);
        break;
      case 'r':
      case 's':
      case 't':
      case 'u':
      case 'v':
      case 'w':
      case 'x':
      case 'y':
      case 'z':
        gnibbles_warpmanager_add_warp
          (warpmanager, -(board[j][i] - 'a' + 'A'), 0, j, i);
        board[j][i] = EMPTYCHAR;
        break;
      }
      /* Warpmanager draws the warp points. Everything else gets drawn here. */
      if (board[j][i] >= 'a')
        gnibbles_draw_pixmap_buffer (board[j][i] - 'a', j, i);
    }
  }

  gdk_draw_drawable (GDK_DRAWABLE (gtk_widget_get_window (drawing_area)),
                     gtk_widget_get_style (drawing_area)->
                     fg_gc[gtk_widget_get_state (drawing_area)], buffer_pixmap,
                     0, 0, 0, 0, BOARDWIDTH * properties->tilesize,
                     BOARDHEIGHT * properties->tilesize);

  fclose (in);
}

void
gnibbles_clutter_init ()
{
  if (clutter_board == NULL)
    return;

  gint i;
/*
  for (i = 0; i < properties->numworms; i++)
    if (cworms[i])
      gnibbles_cworm_destroy (cworms[i]);
*/
  gnibbles_scoreboard_clear (scoreboard);

  for (i = 0; i < properties->numworms; i++) {
    //gnibbles_scoreboard_register (scoreboard, cworms[i], 
	  //               colorval_name (properties->wormprops[i]->color));
  }

  ClutterActor *stage = gnibbles_board_get_stage (clutter_board);

  for (i = 0; i < properties->numworms; i++) {
    if (cworms[i]) {
      clutter_container_add_actor (CLUTTER_CONTAINER (stage), cworms[i]->actors);
      clutter_actor_raise_top (cworms[i]->actors);
    }
  }

  gnibbles_scoreboard_update (scoreboard);
}

void
gnibbles_init (void)
{
  gint i;

  for (i = 0; i < properties->numworms; i++)
    if (worms[i])
      gnibbles_worm_destroy (worms[i]);

  gnibbles_scoreboard_clear (scoreboard);

  for (i = 0; i < properties->numworms; i++) {
    worms[i] = gnibbles_worm_new (i);
    gnibbles_scoreboard_register (scoreboard, worms[i], 
	                 colorval_name (properties->wormprops[i]->color));
  }

  gnibbles_scoreboard_update (scoreboard);
}

void
gnibbles_clutter_add_bonus (gint regular)
{
  gint x, y, good;

#ifdef GGZ_CLIENT
  if (!network_is_host ()) {
    return;
  }
#endif

  if (regular) {
    good = 0;
  } else {
    good = rand () % 50;
    if (good)
      return;
  }

  do {
    good = 1;
    x = rand () % (BOARDWIDTH - 1);
    y = rand () % (BOARDHEIGHT - 1);
    if (level->walls[x][y] != EMPTYCHAR)
      good = 0;
    if (level->walls[x + 1][y] != EMPTYCHAR)
      good = 0;
    if (level->walls[x][y + 1] != EMPTYCHAR)
      good = 0;
    if (level->walls[x + 1][y + 1] != EMPTYCHAR)
      good = 0;
  } while (!good);

  if (regular) {
    if ((rand () % 7 == 0) && properties->fakes)
      gnibbles_boni_add_bonus (boni, x, y, BONUSREGULAR, 1, 300);
    good = 0;
    while (!good) {
      good = 1;
      x = rand () % (BOARDWIDTH - 1);
      y = rand () % (BOARDHEIGHT - 1);
      if (level->walls[x][y] != EMPTYCHAR)
	      good = 0;
      if (level->walls[x + 1][y] != EMPTYCHAR)
	      good = 0;
      if (level->walls[x][y + 1] != EMPTYCHAR)
	      good = 0;
      if (level->walls[x + 1][y + 1] != EMPTYCHAR)
	      good = 0;
    }
    gnibbles_boni_add_bonus (boni, x, y, BONUSREGULAR, 0, 300);
  } else if (boni->missed <= MAXMISSED) {
    good = rand () % 7;

    if (good)
      good = 0;
    else
      good = 1;

    if (good && !properties->fakes)
      return;

    switch (rand () % 21) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
      gnibbles_boni_add_bonus (boni, x, y, BONUSHALF, good, 200);
      break;
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
      gnibbles_boni_add_bonus (boni, x, y, BONUSDOUBLE, good, 150);
      break;
    case 15:
      gnibbles_boni_add_bonus (boni, x, y, BONUSLIFE, good, 100);
      break;
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
      if (properties->numworms > 1)
	      gnibbles_boni_add_bonus (boni, x, y, BONUSREVERSE, good, 150);
      break;
    }
  }
}

void
gnibbles_add_bonus (gint regular)
{
  gint x, y, good;

#ifdef GGZ_CLIENT
  if (!network_is_host ()) {
    return;
  }
#endif

  if (regular) {
    good = 0;
  } else {
    good = rand () % 50;
    if (good)
      return;
  }

  do {
    good = 1;
    x = rand () % (BOARDWIDTH - 1);
    y = rand () % (BOARDHEIGHT - 1);
    if (board[x][y] != EMPTYCHAR)
      good = 0;
    if (board[x + 1][y] != EMPTYCHAR)
      good = 0;
    if (board[x][y + 1] != EMPTYCHAR)
      good = 0;
    if (board[x + 1][y + 1] != EMPTYCHAR)
      good = 0;
  } while (!good);

  if (regular) {
    if ((rand () % 7 == 0) && properties->fakes)
      gnibbles_boni_add_bonus (boni, x, y, BONUSREGULAR, 1, 300);
    good = 0;
    while (!good) {
      good = 1;
      x = rand () % (BOARDWIDTH - 1);
      y = rand () % (BOARDHEIGHT - 1);
      if (board[x][y] != EMPTYCHAR)
	good = 0;
      if (board[x + 1][y] != EMPTYCHAR)
	good = 0;
      if (board[x][y + 1] != EMPTYCHAR)
	good = 0;
      if (board[x + 1][y + 1] != EMPTYCHAR)
	good = 0;
    }
    gnibbles_boni_add_bonus (boni, x, y, BONUSREGULAR, 0, 300);
  } else if (boni->missed <= MAXMISSED) {
    good = rand () % 7;

    if (good)
      good = 0;
    else
      good = 1;

    if (good && !properties->fakes)
      return;

    switch (rand () % 21) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
      gnibbles_boni_add_bonus (boni, x, y, BONUSHALF, good, 200);
      break;
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
      gnibbles_boni_add_bonus (boni, x, y, BONUSDOUBLE, good, 150);
      break;
    case 15:
      gnibbles_boni_add_bonus (boni, x, y, BONUSLIFE, good, 100);
      break;
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
      if (properties->numworms > 1)
	gnibbles_boni_add_bonus (boni, x, y, BONUSREVERSE, good, 150);
      break;
    }
  }
}

gint
gnibbles_move_worms (void)
{
  gint i, j, status = 1, nlives = 0;
  gint *dead;

  dead = g_new (gint, properties->numworms);

  for (i = 0; i < properties->ai; i++) {
    gnibbles_worm_ai_move (worms[properties->human + i]);
  }


  if (boni->missed > MAXMISSED)
    for (i = 0; i < properties->numworms; i++)
      if (worms[i]->score)
	      worms[i]->score--;

  for (i = 0; i < boni->numbonuses; i++) {
    if (!(boni->bonuses[i]->countdown--)) {
      if (boni->bonuses[i]->type == BONUSREGULAR && !boni->bonuses[i]->fake) {
	      gnibbles_boni_remove_bonus (boni,
				          boni->bonuses[i]->x, boni->bonuses[i]->y);
	      boni->missed++;
	      gnibbles_add_bonus (1);
      } else {
	      gnibbles_boni_remove_bonus (boni,
				    boni->bonuses[i]->x, boni->bonuses[i]->y);
      }
    }
  }

  for (i = 0; i < properties->numworms; i++) {
    gnibbles_worm_erase_tail (worms[i]);
  }

  for (i = 0; i < properties->numworms; i++) {
    dead[i] = !gnibbles_worm_test_move_head (worms[i]);
    status &= !dead[i];
  }

  for (i = 0; i < properties->numworms; i++)
    if (!dead[i] && worms[i]->lives > 0)
      gnibbles_worm_move_tail (worms[i]);

  for (i = 0; i < properties->numworms; i++)
    if (!dead[i] && worms[i]->lives > 0)
      gnibbles_worm_draw_head (worms[i]);


  /* If one worm has died, me must make sure that an earlier worm was not
   * supposed to die as well. */

  for (i = 0; i < properties->numworms; i++)
    for (j = 0; j < properties->numworms; j++) {
      if (i != j
          && worms[i]->xhead == worms[j]->xhead
	  && worms[i]->yhead == worms[j]->yhead
	  && worms[i]->lives > 0
	  && worms[j]->lives > 0)
	dead[i] = TRUE;
    }

  for (i = 0; i < properties->numworms; i++)
    if (dead[i]) {
      if (properties->numworms > 1)
	      worms[i]->score *= .7;
      if (!gnibbles_worm_lose_life (worms[i])) {
        /* One of the worms lost one life, but the round continues. */
        gnibbles_worm_reset (worms[i]);
        gnibbles_worm_set_start (worms[i],
				  worms[i]->xstart,
				  worms[i]->ystart,
				  worms[i]->direction_start);
	      games_sound_play ("crash");
	/* Don't return here.  May need to reset more worms. */
	    }
    }

  if (status & GAMEOVER) {
    games_sound_play ("crash");
    games_sound_play ("gameover");
    return (GAMEOVER);
  }

  for (i = 0; i < properties->numworms; i++) {
    if (worms[i]->lives > 0)
      nlives += 1;
  }
  if (nlives == 1 && (properties->ai + properties->human > 1)) {
    /* There is one player left, the other AI players are dead, and that player has won! */
    return (VICTORY);
  } else if (nlives == 0) {
    /* There was only one worm, and it died. */
    return (GAMEOVER);
  }

   /* Noone died, so the round can continue. */

  g_free (dead);
  return (CONTINUE);
}


gint
gnibbles_get_winner (void)
{
  int i;

  for (i = 0; i < properties->numworms; i++) {
    if (worms[i]->lives > 0) {
      return i;
    }
  }
  return -1;
}

gint
gnibbles_keypress_worms (guint keyval)
{
  gint i;
  gint numworms = ggz_network_mode ? 1 : properties->numworms;

  for (i = 0; i < numworms; i++)
    if (gnibbles_worm_handle_keypress (worms[i], keyval)) {
      return TRUE;
    }

  return FALSE;
}

void
gnibbles_undraw_worms (gint data)
{
  gint i;

  for (i = 0; i < properties->numworms; i++)
    gnibbles_worm_undraw_nth (worms[i], data);
}

void
gnibbles_show_scores (GtkWidget * window, gint pos)
{
  static GtkWidget *scoresdialog = NULL;
  gchar *message;

  if (!scoresdialog) {
    scoresdialog = games_scores_dialog_new (GTK_WINDOW (window), highscores, _("Nibbles Scores"));
    games_scores_dialog_set_category_description (GAMES_SCORES_DIALOG
						  (scoresdialog),
						  _("Speed:"));
  }
  if (pos > 0) {
    games_scores_dialog_set_hilight (GAMES_SCORES_DIALOG (scoresdialog), pos);
    message = g_strdup_printf ("<b>%s</b>\n\n%s",
			       _("Congratulations!"),
                               pos == 1 ? _("Your score is the best!") :
			       _("Your score has made the top ten."));
    games_scores_dialog_set_message (GAMES_SCORES_DIALOG (scoresdialog),
				     message);
    g_free (message);
  } else {
    games_scores_dialog_set_message (GAMES_SCORES_DIALOG (scoresdialog),
				     NULL);
  }

  gtk_dialog_run (GTK_DIALOG (scoresdialog));
  gtk_widget_hide (scoresdialog);
}

void
gnibbles_log_score (GtkWidget * window)
{
  GamesScoreValue score;
  gint pos;

  if (properties->numworms > 1)
    return;

  if (properties->human != 1)
    return;

  if (properties->startlevel != 1)
    return;

  if (!worms[0]->score)
    return;

  score.plain = worms[0]->score;
  pos = games_scores_add_score (highscores, score);

  gnibbles_show_scores (window, pos);
}


void
gnibbles_add_spec_bonus (gint t_x, gint t_y,
			 gint t_type, gint t_fake, gint t_countdown)
{
  gnibbles_boni_add_bonus_final (boni, t_x, t_y, t_type, t_fake, t_countdown);
}

void
gnibbles_remove_spec_bonus (gint x, gint y)
{
  gnibbles_boni_remove_bonus_final (boni, x, y);
}
