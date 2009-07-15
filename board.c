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

extern GnibblesProperties *properties;

extern GdkPixbuf *wall_pixmaps[];

GnibblesBoard *
gnibbles_board_new (gint t_w, gint t_h) 
{
  ClutterColor stage_color = {0x00,0x00,0x00,0xff};
  gchar *filename;
  const char *dirname;
  GValue val = {0,};

  GnibblesBoard *board = g_new (GnibblesBoard, 1);
  board->width = t_w;
  board->height = t_h;
  board->level = NULL;
  board->surface = NULL;
  board->clutter_widget = gtk_clutter_embed_new ();

  ClutterActor *stage;

  stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (board->clutter_widget));
  clutter_stage_set_color (CLUTTER_STAGE(stage), &stage_color);

  clutter_stage_set_user_resizable (CLUTTER_STAGE(stage), FALSE); 
  clutter_actor_set_size (CLUTTER_ACTOR (stage), 
                          properties->tilesize * BOARDWIDTH,
                          properties->tilesize * BOARDHEIGHT);
  clutter_stage_set_user_resizable (CLUTTER_STAGE (stage), FALSE);
  clutter_actor_show (stage);

  dirname = games_runtime_get_directory (GAMES_RUNTIME_GAME_PIXMAP_DIRECTORY);
  filename = g_build_filename (dirname, "wall-small-empty.svg", NULL);

  board->surface = clutter_texture_new_from_file (filename, NULL);
  
  g_value_init (&val, G_TYPE_BOOLEAN);
  g_value_set_boolean ( &val, TRUE);

  g_object_set_property (G_OBJECT (board->surface), "repeat-y", &val);
  g_object_set_property (G_OBJECT (board->surface), "repeat-x", &val);

  clutter_actor_set_position (CLUTTER_ACTOR (board->surface), 0, 0);
  clutter_actor_set_size (CLUTTER_ACTOR (board->surface),
                          properties->tilesize * BOARDWIDTH,
                          properties->tilesize * BOARDHEIGHT);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), board->surface);
  clutter_actor_show (board->surface);

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
  gboolean is_wall = TRUE;

  if (board->level)
    g_object_unref (board->level);

  board->level = clutter_group_new ();

  /* Load wall_pixmaps onto the surface*/
  for (i = 0; i < BOARDHEIGHT; i++) {
    y_pos = i * properties->tilesize;
    for (j = 0; j < BOARDWIDTH; j++) {
      is_wall = TRUE;
      switch (level->walls[j][i]) {
        case 'a': // empty space
          is_wall = FALSE;
          break; // break right away
        case 'b': // straight up
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[0]);
          break;
        case 'c': // straight side
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[1]);
          break;
        case 'd': // corner bottom left
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[2]);
          break;
        case 'e': // corner bottom right
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[3]);
          break;
          case 'f': // corner up left
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[4]);
          break;
        case 'g': // corner up right
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[5]);
          break;
        case 'h': // tee up
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[6]);
          break;
        case 'i': // tee right
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[7]);
          break;
        case 'j': // tee left
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[8]);
          break;
        case 'k': // tee down
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[9]);
          break;
        case 'l': // cross
          tmp = gtk_clutter_texture_new_from_pixbuf (wall_pixmaps[10]);
          break;
        default:
          is_wall = FALSE;
          break;
      }

      if (is_wall) {
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
  clutter_actor_raise (board->level,board->surface);

  clutter_actor_set_opacity (board->level, 0);
  clutter_actor_animate (board->level, CLUTTER_EASE_IN_QUAD, 410,
                         "opacity", 255,
                         NULL);

}

void
gnibbles_board_resize (GnibblesBoard *board, gint newtile)
{
  if (!board->level)
    return;
  if (!board->surface)
    return;

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
