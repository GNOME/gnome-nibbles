/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* 
 *   Gnome Nibbles: Gnome Worm Game
 *   Written by Sean MacIsaac <sjm@acm.org>, Ian Peters <itp@gnu.org>
                Guillaume Beland <guillaume.beland@gmail.com>
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

#include <stdlib.h>
#include <glib/gi18n.h>
#include "level.h"
#include "gnibbles.h"
#include "worm-clutter.h"
#include "main.h"
#include "properties.h"

extern GnibblesCWorm *cworms[];
extern GnibblesProperties *properties;

GnibblesLevel *
gnibbles_level_new (gint level)
{
  GnibblesLevel *lvl = g_new (GnibblesLevel, 1);
  lvl->level = level;
  gchar *tmp = NULL;
  const char *dirname;
  gchar *filename;
  FILE *in;
  gchar tmpboard [BOARDWIDTH +2];
  gint i,j;
  gint count = 0;

  tmp = g_strdup_printf("level%03d.gnl", level);
  
  dirname = games_runtime_get_directory (GAMES_RUNTIME_GAME_GAMES_DIRECTORY);
  filename = g_build_filename (dirname, tmp, NULL);

  g_free (tmp);

  if ((in = fopen (filename, "r")) == NULL) {
    char *message =
      g_strdup_printf (_
                       ("Nibbles couldn't load level file:\n%s\n\n"
                        "Please check your Nibbles installation"), filename);
    //gnibbles_error (window, message);
    g_free (message);
  }

  for (i = 0; i < properties->numworms; i++)
    if (cworms[i])
      gnibbles_cworm_destroy (cworms[i]);

  for (i = 0; i < BOARDHEIGHT; i++) {
    if (!fgets (tmpboard, sizeof (tmpboard), in)) {
      char *message =
        g_strdup_printf (_
                         ("Level file appears to be damaged:\n%s\n\n"
                         "Please check your Nibbles installation"), filename);
      //gnibbles_error (window, message);
      g_free (message);
      break;
    }

    for (j = 0; j < BOARDWIDTH; j++) {
      lvl->walls[j][i] = tmpboard[j];
      switch (lvl->walls[j][i]) {
        case 'm':
          lvl->walls[j][i] = EMPTYCHAR;
          if (count < properties->numworms)
            cworms[count] = gnibbles_cworm_new (count++, j, i, WORMUP);
          break;
        case 'n':
          lvl->walls[j][i] = EMPTYCHAR;
          if (count < properties->numworms)
            cworms[count] = gnibbles_cworm_new (count++, j, i, WORMDOWN);
          break;
        case 'o':
          lvl->walls[j][i] = EMPTYCHAR;
          if (count < properties->numworms)
            cworms[count] = gnibbles_cworm_new (count++, j, i, WORMLEFT);
          break;
        case 'p':
          lvl->walls[j][i] = EMPTYCHAR;
          if (count < properties->numworms)
            cworms[count] = gnibbles_cworm_new (count++, j, i, WORMRIGHT);
          break;
        case 'Q':
          //gnibbles_warpmanager_add_warp (warpmanager, j - 1, i - 1, -1, -1);
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
          //gnibbles_warpmanager_add_warp
          //  (warpmanager, j - 1, i - 1, -board[j][i], 0);
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
          //gnibbles_warpmanager_add_warp
          //  (warpmanager, -(board[j][i] - 'a' + 'A'), 0, j, i);
          lvl->walls[j][i] = EMPTYCHAR;
          break;
       }
    }
  }

  g_free (filename);
  fclose (in);

  return lvl;
}

