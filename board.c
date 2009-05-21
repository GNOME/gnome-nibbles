/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

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

#include <stdlib.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <libgames-support/games-runtime.h>

#include <clutter-gtk/clutter-gtk.h>

#include "main.h"
#include "gnibbles.h"
#include "properties.h"
#include "board.h"

static GdkPixbuf*  load_pixmap_file (const gchar * pixmap,
			                                      gint xsize, gint ysize);
static void load_pixmap ();

GdkPixbuf *wall_pixmaps[19] = { NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL
};

extern GnibblesProperties *properties;


GnibblesBoard *
gnibbles_board_new (gint t_w, gint t_h) 
{
  ClutterColor stage_color = {0x00,0x00,0x00,0xff};

  GnibblesBoard *board = g_new (GnibblesBoard, 1);
  board->width = t_w;
  board->height = t_h;
  board->level = NULL;
  board->surface =NULL;
  board->clutter_widget = gtk_clutter_embed_new ();

  ClutterActor *stage;

  load_pixmap ();

  stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (board->clutter_widget));
  clutter_stage_set_color (CLUTTER_STAGE(stage), &stage_color);

  clutter_stage_set_user_resizable (CLUTTER_STAGE(stage), FALSE); 
  clutter_actor_set_size (CLUTTER_ACTOR (stage), 
                        properties->tilesize * BOARDWIDTH,
                        properties->tilesize * BOARDHEIGHT);
  clutter_stage_set_user_resizable (CLUTTER_STAGE (stage), FALSE);
  clutter_actor_show (stage);

  gchar *filename;
  const char *dirname;

  dirname = games_runtime_get_directory (GAMES_RUNTIME_GAME_PIXMAP_DIRECTORY);
  filename = g_build_filename (dirname, "wall-small-empty.svg", NULL);

  /* Using ClutterScript to set special texture property such as "repeat-x",
   * "repeat-y" and "keep-aspect-ratio" */
  gchar texture_script[200];

  g_sprintf (texture_script, "["
                             "  {"
                             "    \"id\" : \"surface\","
                             "    \"type\" : \"ClutterTexture\","
                             "    \"filename\" : \"%s\","
                             "    \"x\" : 0,"
                             "    \"y\" : 0,"
                             "    \"width\" : %d,"
                             "    \"height\" : %d,"
                             "    \"keep-aspect-ratio\" : true"
                             "    \"visible\" : true,"
                             "    \"repeat-x\" : true,"
                             "    \"repeat-y\" : true"
                             "  }"
                             "]",
                             filename,
                             properties->tilesize,
                             properties->tilesize);

  ClutterScript *script = clutter_script_new ();

  clutter_script_load_from_data (script, texture_script, -1, NULL);
  clutter_script_get_objects (script, "surface", &(board->surface), NULL);

  clutter_actor_set_size (CLUTTER_ACTOR (board->surface),
                          properties->tilesize * BOARDWIDTH,
                          properties->tilesize * BOARDHEIGHT);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), board->surface);
  clutter_actor_show (board->surface);

  g_object_unref (script);
  return board;
}

ClutterActor *
gnibbles_board_get_stage (GnibblesBoard *board) 
{
  return gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(board->clutter_widget));
}

void 
gnibbles_board_load_level (GnibblesBoard *board, GnibblesLevel *level) 
{
  gint i,j;
  gint x_pos, y_pos;
  ClutterActor *tmp;  
  gboolean wall = TRUE;

  if (board->level)
    g_object_unref (board->level);

  board->level = clutter_group_new ();

  /* Load walls onto the surface*/
  for (i = 0; i < BOARDHEIGHT; i++) {
    y_pos = i * properties->tilesize;
    for (j = 0; j < BOARDWIDTH; j++) {
      wall = TRUE;
      switch (level->walls[j][i]) {
        case 'a': // empty space
          wall = FALSE;
          break; // break right away
        case 'b': // straight up
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[1]);
          break;
        case 'c': // straight side
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[2]);
          break;
        case 'd': // corner bottom left
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[3]);
          break;
        case 'e': // corner bottom right
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[4]);
          break;
        case 'f': // corner up left
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[5]);
          break;
        case 'g': // corner up right
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[6]);
          break;
        case 'h': // tee up
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[7]);
          break;
        case 'i': // tee right
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[8]);
          break;
        case 'j': // tee left
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[9]);
          break;
        case 'k': // tee down
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[10]);
          break;
        case 'l': // cross
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[11]);
          break;
        default:
          wall = FALSE;
          break;
      }

      if (wall == TRUE) {
        x_pos = j * properties->tilesize;

        clutter_actor_set_size (CLUTTER_ACTOR(tmp),
                                 properties->tilesize,
                                 properties->tilesize);

        clutter_actor_set_position (CLUTTER_ACTOR (tmp), x_pos, y_pos);
        clutter_actor_show (CLUTTER_ACTOR (tmp));
        clutter_container_add_actor (CLUTTER_CONTAINER (board->level), tmp);
      }
    }
  }

  ClutterActor *stage = gnibbles_board_get_stage (board);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), board->level);
  //raise the level above the surface
  clutter_actor_raise (board->level,board->surface);
}

void
gnibbles_board_resize (GnibblesBoard *board, gint newtile)
{
  int i;
  int x_pos;
  int y_pos;
  int count;

  ClutterActor *tmp;
  ClutterActor *stage = gnibbles_board_get_stage (board);

  clutter_actor_set_size (stage, 
                          BOARDWIDTH * newtile,
                          BOARDHEIGHT * newtile);
  clutter_actor_set_size (board->surface,
                          BOARDWIDTH * newtile,
                          BOARDHEIGHT * newtile);

  if (!board->level)
    return;

  count = clutter_group_get_n_children (CLUTTER_GROUP (board->level));

  for (i = 0; i < count; i++) {
    tmp = clutter_group_get_nth_child (CLUTTER_GROUP (board->level), i);
    clutter_actor_get_position (tmp, &x_pos, &y_pos);
    clutter_actor_set_position (tmp,
                                (x_pos / properties->tilesize) * newtile,
                                (y_pos / properties->tilesize) * newtile);
    clutter_actor_set_size (tmp ,newtile, newtile);
  }
}

static void 
load_pixmap ()
{

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

  for (i = 0; i < 19; i++) {
    if (wall_pixmaps[i])
      g_object_unref (wall_pixmaps[i]);
      
    wall_pixmaps[i] = load_pixmap_file (small_files[i],
		  		                              4 * properties->tilesize,
                           						  4 * properties->tilesize);
  }
}

static GdkPixbuf *
load_pixmap_file (const gchar * pixmap, gint xsize, gint ysize)
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

  image = gdk_pixbuf_new_from_file_at_size (filename, xsize, ysize, NULL);
  g_free (filename);

  return image;
}


