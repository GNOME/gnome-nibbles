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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <ctype.h>

#include <libgames-support/games-sound.h>

#include "worm.h"
#include "gnibbles.h"
#include "boni.h"
#include "bonus.h"
#include "warpmanager.h"
#include "properties.h"
#include "main.h"
#ifdef GGZ_CLIENT
#include "ggz-network.h"
#endif

extern gchar board[BOARDWIDTH][BOARDHEIGHT];
extern GnibblesWorm *worms[NUMWORMS];
extern GnibblesBoni *boni;
extern GnibblesWarpManager *warpmanager;

extern GnibblesProperties *properties;

extern gint current_level;

typedef struct _key_queue_entry {
  GnibblesWorm *worm;
  guint dir;
} key_queue_entry;

static GQueue *key_queue[NUMWORMS] = { NULL, NULL, NULL, NULL };

static void
gnibbles_worm_queue_keypress (GnibblesWorm * worm, guint dir)
{
  key_queue_entry *entry;
  int n = worm->number;

  if (key_queue[n] == NULL)
    key_queue[n] = g_queue_new ();

  /* Ignore duplicates in normal movement mode. This resolves the
   * key repeat issue. We ignore this in relative mode because then
   * you do want two keys that are the same in quick succession. */
  if ((!properties->wormprops[worm->number]->relmove) &&
      (!g_queue_is_empty (key_queue[n])) &&
      (dir == ((key_queue_entry *) g_queue_peek_tail (key_queue[n]))->dir))
    return;

  entry = g_malloc (sizeof (key_queue_entry));
  entry->worm = worm;
  entry->dir = dir;
  g_queue_push_tail (key_queue[n], (gpointer) entry);
}

static void
gnibbles_worm_queue_empty (GnibblesWorm * worm)
{
  key_queue_entry *entry;
  int n = worm->number;

  if (!key_queue[n])
    return;

  while (!g_queue_is_empty (key_queue[n])) {
    entry = g_queue_pop_head (key_queue[n]);
    g_free (entry);
  }
}

GnibblesWorm *
gnibbles_worm_new (guint t_number)
{
  GnibblesWorm *tmp = (GnibblesWorm *) g_malloc (sizeof (GnibblesWorm));

  tmp->xoff = (gint8 *) malloc (CAPACITY * sizeof (gint8));
  tmp->yoff = (gint8 *) malloc (CAPACITY * sizeof (gint8));
  tmp->lives = SLIVES;
  tmp->score = 0;
  tmp->number = t_number;

  return tmp;
}

void
gnibbles_worm_destroy (GnibblesWorm * worm)
{
  free (worm->xoff);
  free (worm->yoff);
  free (worm);
}

void
gnibbles_worm_set_start (GnibblesWorm * worm, guint t_xhead, guint t_yhead,
			 gint t_direction)
{
  worm->xhead = t_xhead;
  worm->xstart = t_xhead;
  worm->yhead = t_yhead;
  worm->ystart = t_yhead;
  worm->xtail = t_xhead;
  worm->ytail = t_yhead;
  worm->direction = t_direction;
  worm->direction_start = t_direction;
  worm->xoff[0] = 0;
  worm->yoff[0] = 0;
  worm->start = 0;
  worm->stop = 0;
  worm->length = 1;
  worm->change = SLENGTH - 1;
  worm->keypress = 0;
  gnibbles_worm_queue_empty (worm);
}

gint
gnibbles_worm_handle_keypress (GnibblesWorm * worm, guint keyval)
{
  GnibblesWormProps *props;
  guint propsUp, propsLeft, propsDown, propsRight, keyvalUpper;
/*	if (worm->keypress) {
                gnibbles_worm_queue_keypress (worm, keyval);
		return FALSE;
	} */

  props = properties->wormprops[ggz_network_mode ? 0 : worm->number];
  propsUp = toupper(props->up);
  propsLeft = toupper(props->left);
  propsDown = toupper(props->down);
  propsRight = toupper(props->right);
  keyvalUpper = toupper(keyval);

  if (properties->wormprops[worm->number]->relmove) {
    if (keyvalUpper == propsLeft)
      worm_handle_direction (worm->number, worm->direction - 1);
    else if (keyvalUpper == propsRight)
      worm_handle_direction (worm->number, worm->direction + 1);
    else
      return FALSE;
    return TRUE;
  } else {
    if ((keyvalUpper == propsUp) && (worm->direction != WORMDOWN)) {
      worm_handle_direction (worm->number, WORMUP);
      /*worm->keypress = 1; */
      return TRUE;
    }
    if ((keyvalUpper == propsRight) && (worm->direction != WORMLEFT)) {
      worm_handle_direction (worm->number, WORMRIGHT);
      /*worm->keypress = 1; */
      return TRUE;
    }
    if ((keyvalUpper == propsDown) && (worm->direction != WORMUP)) {
      worm_handle_direction (worm->number, WORMDOWN);
      /*worm->keypress = 1; */
      return TRUE;
    }
    if ((keyvalUpper == propsLeft) && (worm->direction != WORMRIGHT)) {
      worm_handle_direction (worm->number, WORMLEFT);
      /*worm->keypress = 1; */
      return TRUE;
    }
  }
  return FALSE;
}

static void
gnibbles_worm_dequeue_keypress (GnibblesWorm * worm)
{
  key_queue_entry *entry;
  int n = worm->number;

  entry = (key_queue_entry *) g_queue_pop_head (key_queue[n]);

  worm_set_direction (entry->worm->number, entry->dir);

  g_free (entry);
}

static gint
gnibbles_worm_reverse (gpointer data)
{
  gint i, j, temp;
  GnibblesWorm *worm;

  worm = (GnibblesWorm *) data;
  temp = worm->xhead;
  worm->xhead = worm->xtail;
  worm->xtail = temp;
  temp = worm->yhead;
  worm->yhead = worm->ytail;
  worm->ytail = temp;
  temp = worm->yhead;
  i = worm->start - 1;
  j = worm->stop;
  while (i != j && i != j - 1) {
    temp = worm->xoff[j];
    worm->xoff[j] = -worm->xoff[i];
    worm->xoff[i] = -temp;
    temp = worm->yoff[j];
    worm->yoff[j] = -worm->yoff[i];
    worm->yoff[i] = -temp;
    i--;
    j++;
  }
  if (j == i) {
    worm->xoff[j] *= -1;
    worm->yoff[j] *= -1;
  }
  if (worm->xoff[worm->start - 1] == 1)
    worm->direction = WORMLEFT;
  if (worm->xoff[worm->start - 1] == -1)
    worm->direction = WORMRIGHT;
  if (worm->yoff[worm->start - 1] == 1)
    worm->direction = WORMUP;
  if (worm->yoff[worm->start - 1] == -1)
    worm->direction = WORMDOWN;

  return FALSE;
}

static void
gnibbles_worm_grok_bonus (GnibblesWorm * worm)
{
  int i;

  if (gnibbles_boni_fake (boni, worm->xhead, worm->yhead)) {
    g_timeout_add (1, (GtkFunction) gnibbles_worm_reverse, worm);
    games_sound_play ("reverse");
    return;
  }

  switch (board[worm->xhead][worm->yhead] - 'A') {
  case BONUSREGULAR:
    boni->numleft--;
    worm->change += (NUMBONI - boni->numleft) * GROWFACTOR;
    worm->score += (NUMBONI - boni->numleft) * current_level;
    games_sound_play ("gobble");
    break;
  case BONUSDOUBLE:
    worm->score += (worm->length + worm->change) * current_level;
    worm->change += worm->length + worm->change;
    games_sound_play ("bonus");
    break;
  case BONUSHALF:
    if (worm->length + worm->change > 2) {
      worm->score += ((worm->length + worm->change) / 2) * current_level;
      worm->change -= (worm->length + worm->change) / 2;
      games_sound_play ("bonus");
    }
    break;
  case BONUSLIFE:
    worm->lives += 1;
    games_sound_play ("life");
    break;
  case BONUSREVERSE:
    for (i = 0; i < properties->numworms; i++)
      if (worm != worms[i])
	g_timeout_add (1, (GSourceFunc)
		       gnibbles_worm_reverse, worms[i]);
    games_sound_play ("reverse");
    break;
  }
}

void
gnibbles_worm_draw_head (GnibblesWorm * worm)
{
  worm->keypress = 0;

  switch (worm->direction) {
  case WORMUP:
    worm->xoff[worm->start] = 0;
    worm->yoff[worm->start] = 1;
    worm->yhead--;
    break;
  case WORMDOWN:
    worm->xoff[worm->start] = 0;
    worm->yoff[worm->start] = -1;
    worm->yhead++;
    break;
  case WORMLEFT:
    worm->xoff[worm->start] = 1;
    worm->yoff[worm->start] = 0;
    worm->xhead--;
    break;
  case WORMRIGHT:
    worm->xoff[worm->start] = -1;
    worm->yoff[worm->start] = 0;
    worm->xhead++;
    break;
  }

  if (worm->xhead == BOARDWIDTH) {
    worm->xhead = 0;
    worm->xoff[worm->start] += BOARDWIDTH;
  }
  if (worm->xhead < 0) {
    worm->xhead = BOARDWIDTH - 1;
    worm->xoff[worm->start] -= BOARDWIDTH;
  }
  if (worm->yhead == BOARDHEIGHT) {
    worm->yhead = 0;
    worm->yoff[worm->start] += BOARDHEIGHT;
  }
  if (worm->yhead < 0) {
    worm->yhead = BOARDHEIGHT - 1;
    worm->yoff[worm->start] -= BOARDHEIGHT;
  }

  if ((board[worm->xhead][worm->yhead] != EMPTYCHAR) &&
      (board[worm->xhead][worm->yhead] != WARPLETTER)) {
    gnibbles_worm_grok_bonus (worm);
    if ((board[worm->xhead][worm->yhead] == BONUSREGULAR + 'A') &&
	!gnibbles_boni_fake (boni, worm->xhead, worm->yhead)) {
      gnibbles_boni_remove_bonus_final (boni, worm->xhead, worm->yhead);
      if (boni->numleft != 0)
	gnibbles_add_bonus (1);
    } else
      gnibbles_boni_remove_bonus_final (boni, worm->xhead, worm->yhead);
  }

  if (board[worm->xhead][worm->yhead] == WARPLETTER) {
    gnibbles_warpmanager_worm_change_pos (warpmanager, worm);
    games_sound_play ("teleport");
  }

  worm->start++;

  if (worm->start == CAPACITY)
    worm->start = 0;

  board[worm->xhead][worm->yhead] = WORMCHAR + worm->number;

  gnibbles_draw_pixmap (properties->wormprops[worm->number]->color,
			worm->xhead, worm->yhead);

  if (key_queue[worm->number] && !g_queue_is_empty (key_queue[worm->number])) {
    gnibbles_worm_dequeue_keypress (worm);
  }
}

gint
gnibbles_worm_can_move_to (GnibblesWorm * worm, gint x, gint y)
{
  if (worm->xhead == x)
    return worm->yhead - 1 == y || worm->yhead + 1 == y;
  if (worm->yhead == y)
    return worm->xhead - 1 == x || worm->xhead + 1 == x;
  return (FALSE);
}

void
gnibbles_worm_position_move_head (GnibblesWorm * worm, gint *x, gint *y)
{
  *x = worm->xhead;
  *y = worm->yhead;

  switch (worm->direction) {
  case WORMUP:
    *y = worm->yhead - 1;
    break;
  case WORMDOWN:
    *y = worm->yhead + 1;
    break;
  case WORMLEFT:
    *x = worm->xhead - 1;
    break;
  case WORMRIGHT:
    *x = worm->xhead + 1;
    break;
  }

  if (*x == BOARDWIDTH) {
    *x = 0;
  }
  if (*x < 0) {
    *x = BOARDWIDTH - 1;
  }
  if (*y == BOARDHEIGHT) {
    *y = 0;
  }
  if (*y < 0) {
    *y = BOARDHEIGHT - 1;
  }
}

gint
gnibbles_worm_test_move_head (GnibblesWorm * worm)
{
  int x, y;

  gnibbles_worm_position_move_head(worm, &x, &y);

  if (board[x][y] > EMPTYCHAR && board[x][y] < 'z' + properties->numworms)
    return (FALSE);

  return (TRUE);
}

gint
gnibbles_worm_is_move_safe (GnibblesWorm * worm)
{
  int x, y, i;

  gnibbles_worm_position_move_head(worm, &x, &y);

  for (i = 0; i < properties->numworms; i++) {
    if (i != worm->number) {
      if (gnibbles_worm_can_move_to (worms[i], x, y))
        return (FALSE);
    }
  }

  return (TRUE);
}

void
gnibbles_worm_erase_tail (GnibblesWorm * worm)
{
  if (worm->change <= 0) {
    board[worm->xtail][worm->ytail] = EMPTYCHAR;
    if (worm->change) {
      board[worm->xtail - worm->xoff[worm->stop]]
	[worm->ytail - worm->yoff[worm->stop]] = EMPTYCHAR;
    }
  }
}

void
gnibbles_worm_move_tail (GnibblesWorm * worm)
{
  if (worm->change <= 0) {
    gnibbles_draw_pixmap (BLANKPIXMAP, worm->xtail, worm->ytail);
    worm->xtail -= worm->xoff[worm->stop];
    worm->ytail -= worm->yoff[worm->stop];
    worm->stop++;
    if (worm->stop == CAPACITY)
      worm->stop = 0;
    if (worm->change) {
      gnibbles_draw_pixmap (BLANKPIXMAP, worm->xtail, worm->ytail);
      board[worm->xtail][worm->ytail] = EMPTYCHAR;
      worm->xtail -= worm->xoff[worm->stop];
      worm->ytail -= worm->yoff[worm->stop];
      worm->stop++;
      if (worm->stop == CAPACITY)
	worm->stop = 0;
      worm->change++;
      worm->length--;
    }
  } else {
    worm->change--;
    worm->length++;
  }
}

gint
gnibbles_worm_lose_life (GnibblesWorm * worm)
{
  worm->lives--;
  if (worm->lives < 0)
    return 1;

  return 0;
}

void
gnibbles_worm_undraw_nth (GnibblesWorm * worm, gint offset)
{
  int x, y, i, j;

  x = worm->xhead;
  y = worm->yhead;

  i = worm->start - 1;
  if (i <= 0)
    i = CAPACITY - 1;

  for (j = 0; j < offset; j++) {
    if ((worm->stop == 0 && i == CAPACITY - 1) ||
	(worm->stop != 0 && i == worm->stop - 1))
      return;
    x += worm->xoff[i];
    y += worm->yoff[i];
    i--;
    if (i == 0)
      i = CAPACITY - 1;
  }

  while (1) {
    gnibbles_draw_pixmap (BLANKPIXMAP, x, y);
    for (j = 0; j < ERASESIZE; j++) {
      x += worm->xoff[i];
      y += worm->yoff[i];
      if ((worm->stop == 0 && i == CAPACITY - 1) ||
	  (worm->stop != 0 && i == worm->stop - 1))
	return;
      i--;
      if (i == 0 && worm->stop == 0)
	i = CAPACITY - 1;
    }
  }


}


void
worm_handle_direction (int worm, int dir)
{
  if (ggz_network_mode) {
#ifdef GGZ_CLIENT
    network_game_move (dir);

    worms[0]->direction = dir;
    worms[0]->keypress = 1;
#endif
  } else {
    worm_set_direction (worm, dir);
  }
}


void
worm_set_direction (int worm, int dir)
{

  if (worm >= properties->human) {
    return;
  }

  if (worms[worm]) {

    if (dir > 4)
      dir = 1;
    if (dir < 1)
      dir = 4;

    if (worms[worm]->keypress) {
      gnibbles_worm_queue_keypress (worms[worm], dir);
      return;
    }

    worms[worm]->direction = dir;
    worms[worm]->keypress = 1;
  }

}

/* Remove Worm from Field */
void
gnibbles_worm_reset (GnibblesWorm * worm)
{
  while (worm->stop != worm->start) {
    board[worm->xtail][worm->ytail] = EMPTYCHAR;
    gnibbles_draw_pixmap (BLANKPIXMAP, worm->xtail, worm->ytail);

    worm->xtail -= worm->xoff[worm->stop];
    worm->ytail -= worm->yoff[worm->stop];

    worm->stop++;
    if (worm->stop == CAPACITY)
      worm->stop = 0;
  }

  board[worm->xtail][worm->ytail] = EMPTYCHAR;
  gnibbles_draw_pixmap (BLANKPIXMAP, worm->xtail, worm->ytail);
}

/* Check whether the worm will be trapped in a dead end. A location
   within the dead end and the length of the worm is given. This
   prevents worms getting trapped in a spiral, or in a corner sharper
   than 90 degrees.  runnumber is a unique number used to update the
   deadend board. The principle of the deadend board is that it marks
   all squares previously checked, so the exact size of the deadend
   can be calculated in O(n) time; to prevent the need to clear it
   afterwards, a different number is stored in the board each time
   (the number will not have been previously used, so the board will
   appear empty). Although in theory deadend_runnumber may wrap round,
   after 4 billion steps the entire board is likely to have been
   overwritten anyway. */
static guint deadendboard[BOARDWIDTH][BOARDHEIGHT] = {{0}};
static guint deadend_runnumber = 0;
static gint
gnibbles_worm_ai_deadend (gint x, gint y, gint lengthleft)
{
  gint cdir, cx, cy;

  if (x >= BOARDWIDTH)
    x = 0;
  if (x < 0)
    x = BOARDWIDTH - 1;
  if (y >= BOARDHEIGHT)
    y = 0;
  if (y < 0)
    y = BOARDHEIGHT - 1;

  if (! lengthleft)
    return 0;

  cdir = 5;
  while (--cdir) {
    cx = x;
    cy = y;
    switch (cdir) {
    case WORMUP:
      cy -= 1;
      break;
    case WORMDOWN:
      cy += 1;
      break;
    case WORMLEFT:
      cx -= 1;
      break;
    case WORMRIGHT:
      cx += 1;
      break;
    }

    if (cx >= BOARDWIDTH)
      cx = 0;
    if (cx < 0)
      cx = BOARDWIDTH - 1;
    if (cy >= BOARDHEIGHT)
      cy = 0;
    if (cy < 0)
      cy = BOARDHEIGHT - 1;

    if ((board[cx][cy] <= EMPTYCHAR
	 || board[x][y] >= 'z' + properties->numworms)
	&& deadendboard[cx][cy] != deadend_runnumber) {
      deadendboard[cx][cy] = deadend_runnumber;
      lengthleft = gnibbles_worm_ai_deadend(cx, cy, lengthleft - 1);
      if (!lengthleft)
	return 0;
    }
  }
  return lengthleft;
}

/* Check a deadend starting from the next square in this direction,
   rather than from this square. Also block off the squares near worm
   heads, so that humans can't kill AI players by trapping them
   against a wall.  The given length is quartered and squared; this
   allows for the situation where the worm has gone round in a square
   and is about to get trapped in a spiral. However, it's set to at
   least BOARDWIDTH, so that on the levels with long thin paths a worm
   won't start down the path if it'll crash at the other end. */
static gint
gnibbles_worm_ai_deadend_after (gint x, gint y, gint dir, gint length)
{
  gint cx, cy, cl, i;

  if (x < 0 || x >= BOARDWIDTH || y < 0 || y >= BOARDHEIGHT) {
    return 0;
  }

  ++deadend_runnumber;

  if (dir > 4)
    dir = 1;
  if (dir < 1)
    dir = 4;

  i = properties->numworms;
  while(i--) {
    cx = worms[i]->xhead;
    cy = worms[i]->yhead;
    if(cx != x || cy != y) {
      if(cx > 0) deadendboard[cx-1][cy] = deadend_runnumber;
      if(cy > 0) deadendboard[cx][cy-1] = deadend_runnumber;
      if(cx < BOARDWIDTH-1) deadendboard[cx+1][cy] = deadend_runnumber;
      if(cy < BOARDHEIGHT-1) deadendboard[cx][cy+1] = deadend_runnumber;
    }
  }

  cx = x;
  cy = y;
  switch (dir) {
  case WORMUP:
    cy -= 1;
    break;
  case WORMDOWN:
    cy += 1;
    break;
  case WORMLEFT:
    cx -= 1;
    break;
  case WORMRIGHT:
    cx += 1;
    break;
  }

  if (cx >= BOARDWIDTH)
    cx = 0;
  if (cx < 0)
    cx = BOARDWIDTH - 1;
  if (cy >= BOARDHEIGHT)
    cy = 0;
  if (cy < 0)
    cy = BOARDHEIGHT - 1;

  deadendboard[x][y] = deadend_runnumber;
  deadendboard[cx][cy] = deadend_runnumber;

  cl = (length * length) / 16;
  if (cl < BOARDWIDTH)
    cl = BOARDWIDTH;
  return gnibbles_worm_ai_deadend (cx, cy, cl);
}

/* Check to see if another worm's head is too close in front of us;
   that is, that it's within 3 in the direction we're going and within
   1 to the side. */
static gint
gnibbles_worm_ai_tooclose (GnibblesWorm * worm)
{
  gint i = properties->numworms;
  gint dx, dy;
  while (i--) {
    dx = worm->xhead - worms[i]->xhead;
    dy = worm->yhead - worms[i]->yhead;
    switch (worm->direction)
    {
    case WORMUP:
      if (dy > 0 && dy <= 3 && dx >= -1 && dx <= 1)
	return 1;
      break;
    case WORMDOWN:
      if (dy < 0 && dy >= -3 && dx >= -1 && dx <= 1)
	return 1;
      break;
    case WORMLEFT:
      if (dx > 0 && dx <= 3 && dy >= -1 && dy <= 1)
	return 1;
      break;
    case WORMRIGHT:
      if (dx < 0 && dx >= -3 && dy >= -1 && dy <= 1)
	return 1;
      break;
    }
  }
  return 0;
}

static gint
gnibbles_worm_ai_wander (gint x, gint y, gint dir, gint ox, gint oy)
{
  if (dir > 4)
    dir = 1;
  if (dir < 1)
    dir = 4;

  switch (dir) {
  case WORMUP:
    y -= 1;
    break;
  case WORMDOWN:
    y += 1;
    break;
  case WORMLEFT:
    x -= 1;
    break;
  case WORMRIGHT:
    x += 1;
    break;
  }

  if (x >= BOARDWIDTH)
    x = 0;
  if (x < 0)
    x = BOARDWIDTH - 1;
  if (y >= BOARDHEIGHT)
    y = 0;
  if (y < 0)
    y = BOARDHEIGHT - 1;

  switch (board[x][y] - 'A') {
  case BONUSREGULAR:
  case BONUSDOUBLE:
  case BONUSLIFE:
  case BONUSREVERSE:
    return 1;
    break;
  case BONUSHALF:
    return 0;
    break;
  default:
    if (board[x][y] > EMPTYCHAR && board[x][y] < 'z' + properties->numworms) {
      return 0;
    } else {
      if (ox == x && oy == y)
	return 0;
      return gnibbles_worm_ai_wander (x, y, dir, ox, oy);
    }
    break;
  }
}

/* Determines the direction of the AI worm. */
void
gnibbles_worm_ai_move (GnibblesWorm * worm)
{
  int opposite, dir, left, right, front;
  gint bestyet, bestdir, thislen, olddir;

  opposite = (worm->direction + 1) % 4 + 1;

  front = gnibbles_worm_ai_wander
    (worm->xhead, worm->yhead, worm->direction, worm->xhead, worm->yhead);
  left = gnibbles_worm_ai_wander
    (worm->xhead, worm->yhead, worm->direction - 1, worm->xhead, worm->yhead);
  right = gnibbles_worm_ai_wander
    (worm->xhead, worm->yhead, worm->direction + 1, worm->xhead, worm->yhead);

  if (!front) {
    if (left) {
      // Found a bonus to the left
      dir = worm->direction - 1;
      if (dir < 1)
        dir = 4;
      worm->direction = dir;
    } else if (right) {
      // Found a bonus to the right
      dir = worm->direction + 1;
      if (dir > 4)
        dir = 1;
      worm->direction = dir;
    } else {
      // Else move in random direction at random time intervals
      if (rand () % 30 == 1) {
        dir = worm->direction + (rand() % 2 ? 1 : -1);
        if (dir != opposite) {
          if (dir > 4)
            dir = 1;
          if (dir < 1)
            dir = 4;
          worm->direction = dir;
        }
      }
    }
  }

  /* Avoid walls, dead-ends and other worm's heads. This is done using
     an evalution function which is CAPACITY for a wall, 4 if another
     worm's head is in the tooclose area, 4 if another worm's head
     could move to the same location as ours, plus 0 if there's no
     dead-end, or the amount that doesn't fit for a deadend. olddir's
     score is reduced by 100, to favour it, but only if its score is 0
     otherwise; this is so that if we're currently trapped in a dead
     end, the worm will move in a space-filling manner in the hope
     that the dead end will disappear (e.g. if it's made from the tail
     of some worm, as often happens). */
  olddir = worm->direction;
  bestyet = CAPACITY*2;
  bestdir = -1;
  for (dir = 1; dir <= 4; dir++) {
    worm->direction = dir;
    if (dir == opposite) continue;
    thislen = 0;
    if(!gnibbles_worm_test_move_head (worm))
      thislen += CAPACITY;
    if(gnibbles_worm_ai_tooclose (worm))
      thislen += 4;
    if(!gnibbles_worm_is_move_safe (worm))
      thislen += 4;
    thislen += gnibbles_worm_ai_deadend_after
      (worm->xhead, worm->yhead, dir, worm->length + worm->change);
    if (dir == olddir && !thislen)
      thislen -= 100;
    /* If the favoured direction isn't appropriate, then choose
       another direction at random rather than favouring one in
       particular, to stop the worms bunching in the bottom-
       right corner of the board. */
    if (thislen <= 0)
      thislen -= random() % 100;
    if (thislen < bestyet)
    {
      bestyet = thislen;
      bestdir = dir;
    }
  }
  if (bestdir == -1) /* this should never happen, but just in case... */
    bestdir = olddir;
  worm->direction = bestdir;

  /* Make sure we are at least avoiding walls.
   * Mostly other snakes should avoid our head. */
  for (dir = 1; dir <= 4; dir++) {
    if (dir == opposite) continue;
    if (!gnibbles_worm_test_move_head (worm)) {
      worm->direction = dir;
    } else {
      continue;
    }
  }
}
