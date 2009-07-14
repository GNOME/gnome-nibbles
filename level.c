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
#include "boni.h"

#ifdef GGZ_CLIENT
#include "ggz-network.h"
#endif
extern GnibblesWorm *worms[];
extern GnibblesProperties *properties;
extern GnibblesWarpManager *warpmanager;
extern GnibblesBoni *boni;

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

  if (boni)
    gnibbles_boni_destroy (boni);

  boni = gnibbles_boni_new ();

  for (i = 0; i < properties->numworms; i++)
    if (worms[i])
      gnibbles_worm_destroy (worms[i]);

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
          if (count < properties->numworms) {
            worms[count] = gnibbles_worm_new (count, j, i, WORMUP);
            count++;
          }
          break;
        case 'n':
          lvl->walls[j][i] = EMPTYCHAR;
          if (count < properties->numworms) {
            worms[count] = gnibbles_worm_new (count, j, i, WORMLEFT);
            count++;
          }
          break;
        case 'o':
          lvl->walls[j][i] = EMPTYCHAR;
          if (count < properties->numworms) {
            worms[count] = gnibbles_worm_new (count, j, i, WORMDOWN);
            count++;
          }
          break;
        case 'p':
          lvl->walls[j][i] = EMPTYCHAR;
          if (count < properties->numworms) {
            worms[count] = gnibbles_worm_new (count, j, i, WORMRIGHT);
            count++;
          }
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
            (warpmanager, j - 1, i - 1, -(lvl->walls[j][i]), 0);
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
            (warpmanager, -(lvl->walls[j][i] - 'a' + 'A'), 0, j, i);
          lvl->walls[j][i] = EMPTYCHAR;
          break;
       }
    }
  }

  g_free (filename);
  fclose (in);

  return lvl;
}

void
gnibbles_level_add_bonus (GnibblesLevel *level, gint regular)
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


