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

#include "worm.h"
#include "gnibbles.h"
#include "boni.h"
#include "bonus.h"
#include "warpmanager.h"
#include "properties.h"

extern gchar board[BOARDWIDTH][BOARDHEIGHT];
extern GnibblesWorm *worms[NUMWORMS];
extern GnibblesBoni *boni;
extern GnibblesWarpManager *warpmanager;

extern GnibblesProperties *properties;

extern gint current_level;

GnibblesWorm *gnibbles_worm_new (guint t_number)
{
        GnibblesWorm *tmp = (GnibblesWorm *) malloc (sizeof (GnibblesWorm));

	tmp->xoff = (gint8 *) malloc (CAPACITY * sizeof (gint8));
	tmp->yoff = (gint8 *) malloc (CAPACITY * sizeof (gint8));
	tmp->lives = SLIVES;
	tmp->score = 0;
	tmp->number = t_number;

        return tmp;
}

void gnibbles_worm_destroy (GnibblesWorm *worm)
{
	free (worm->xoff);
	free (worm->yoff);
	free (worm);
}

void gnibbles_worm_set_start (GnibblesWorm *worm, guint t_xhead, guint t_yhead,
		gint t_direction)
{
	worm->xhead = t_xhead;
	worm->yhead = t_yhead;
	worm->xtail = t_xhead;
	worm->ytail = t_yhead;
	worm->direction = t_direction;
	worm->xoff[0] = 0;
	worm->yoff[0] = 0;
	worm->start = 0;
	worm->stop = 0;
	worm->length = 1;
	worm->change = SLENGTH - 1;
	worm->keypress = 0;
}

void gnibbles_worm_handle_keypress (GnibblesWorm *worm, guint keyval)
{
	if (worm->keypress)
		return;
	
	if (properties->wormprops[worm->number]->relmove) {
		if (keyval == properties->wormprops[worm->number]->left)
			worm->direction = worm->direction - 1;
		if (keyval == properties->wormprops[worm->number]->right)
			worm->direction = worm->direction + 1;
		if (worm->direction == 0)
			worm->direction = 4;
		if (worm->direction == 5)
			worm->direction = 1;
	} else {
		if ((keyval == properties->wormprops[worm->number]->up) &&
				(worm->direction != WORMDOWN)) {
			worm->direction = WORMUP;
			worm->keypress = 1;
		}
		if ((keyval == properties->wormprops[worm->number]->right) &&
				(worm->direction !=WORMLEFT)) {
			worm->direction = WORMRIGHT;
			worm->keypress = 1;
		}
		if ((keyval == properties->wormprops[worm->number]->down) &&
				(worm->direction != WORMUP)) {
			worm->direction = WORMDOWN;
			worm->keypress = 1;
		}
		if ((keyval == properties->wormprops[worm->number]->left) &&
				(worm->direction != WORMRIGHT)) {
			worm->direction = WORMLEFT;
			worm->keypress = 1;
		}
	}
}

static gint gnibbles_worm_reverse (gpointer data)
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

static void gnibbles_worm_grok_bonus (GnibblesWorm *worm)
{
	int i;

	if (gnibbles_boni_fake (boni, worm->xhead, worm->yhead)) {
		gtk_timeout_add(1, (GtkFunction) gnibbles_worm_reverse, worm);
		gnibbles_play_sound ("reverse");
		return;
	}

	switch (board[worm->xhead][worm->yhead] - 'A') {
		case BONUSREGULAR:
			boni->numleft--;
			worm->change += (NUMBONI - boni->numleft) * GROWFACTOR;
			worm->score += (NUMBONI - boni->numleft) *
				current_level;
			gnibbles_play_sound ("gobble");
			break;
		case BONUSDOUBLE:
			worm->score += (worm->length + worm->change) *
				current_level;
			worm->change += worm->length + worm->change;
			gnibbles_play_sound ("bonus");
			break;
		case BONUSHALF:
			if (worm->length + worm->change > 2) {
				worm->score += ((worm->length + worm->change) /
					2) * current_level;
				worm->change -= (worm->length + worm->change) /
					2;
				gnibbles_play_sound ("bonus");
			}
			break;
		case BONUSLIFE:
			worm->lives += 1;
			gnibbles_play_sound ("life");
			break;
		case BONUSREVERSE:
			for (i = 0; i < properties->numworms; i++)
				if (worm != worms[i])
					gtk_timeout_add(1, (GtkFunction)
							gnibbles_worm_reverse,
						 	worms[i]);
			gnibbles_play_sound ("reverse");
			break;
	}
}

void gnibbles_worm_draw_head (GnibblesWorm *worm)
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
				!gnibbles_boni_fake (boni, worm->xhead,
				worm->yhead)) {
			gnibbles_boni_remove_bonus (boni, worm->xhead,
					worm->yhead);
			if (boni->numleft != 0)
				gnibbles_add_bonus (1);
		} else
			gnibbles_boni_remove_bonus (boni, worm->xhead,
					worm->yhead);
	}

	if (board[worm->xhead][worm->yhead] == WARPLETTER) {
		gnibbles_warpmanager_worm_change_pos (warpmanager, worm);
		gnibbles_play_sound ("teleport");
	}

	worm->start++;

	if (worm->start == CAPACITY)
		worm->start = 0;
		
	board[worm->xhead][worm->yhead] = WORMCHAR + worm->number;

	gnibbles_draw_pixmap (properties->wormprops[worm->number]->color,
			worm->xhead, worm->yhead);
}

gint gnibbles_worm_test_move_head(GnibblesWorm *worm)
{
	int x, y;

	x = worm->xhead;
	y = worm->yhead;

	switch (worm->direction) {
		case WORMUP:
			y = worm->yhead - 1;
			break;
		case WORMDOWN:
			y = worm->yhead + 1;
			break;
		case WORMLEFT:
			x = worm->xhead - 1;
			break;
		case WORMRIGHT:
			x = worm->xhead + 1;
			break;
	}

	if (x == BOARDWIDTH) {
		x = 0;
	}
	if (x < 0) {
		x = BOARDWIDTH - 1;
	}
	if (y == BOARDHEIGHT) {
		y = 0;
	}
	if (y < 0) {
		y = BOARDHEIGHT - 1;
	}

	if (board[x][y] > EMPTYCHAR && board[x][y] < 'z')
		return (FALSE);

	return (TRUE);
}

void gnibbles_worm_erase_tail (GnibblesWorm *worm)
{
	if (worm->change <= 0) {
		board[worm->xtail][worm->ytail] = EMPTYCHAR;
		if (worm->change) {
			board[worm->xtail - worm->xoff[worm->stop]]
				[worm->ytail - worm->yoff[worm->stop]] =
				EMPTYCHAR;
		}
	}
}

void gnibbles_worm_move_tail (GnibblesWorm *worm)
{
	if (worm->change <= 0) {
		gnibbles_draw_pixmap (BLANKPIXMAP, worm->xtail, worm->ytail);
		worm->xtail -= worm->xoff[worm->stop];
		worm->ytail -= worm->yoff[worm->stop];
		worm->stop++;
		if (worm->stop == CAPACITY)
			worm->stop = 0;
		if (worm->change) {
			gnibbles_draw_pixmap (BLANKPIXMAP, worm->xtail,
					worm->ytail);
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

gint gnibbles_worm_lose_life (GnibblesWorm *worm)
{
	worm->lives--;
	if (worm->lives < 0)
		return 1;

	return 0;
}

void gnibbles_worm_undraw_nth (GnibblesWorm *worm, gint offset)
{
	int x, y, i, j;

	x = worm->xhead;
	y = worm->yhead;

	i = worm->start - 1;
	if (i <= 0)
		i = CAPACITY - 1;

	for (j = 0; j < offset; j++) {
		if ((worm->stop == 0 && i == CAPACITY - 1) ||
				(worm->stop != 0 &&
				 i == worm->stop - 1))
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
					(worm->stop != 0 &&
					 i == worm->stop - 1))
				return;
			i--;
			if (i == 0 && worm->stop == 0)
				i = CAPACITY - 1;
		}
	}
}
