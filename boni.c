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

#include "gnibbles.h"
#include "bonus.h"
#include "boni.h"

extern gchar board[BOARDWIDTH][BOARDHEIGHT];

GnibblesBoni *gnibbles_boni_new ()
{
	int i;
	GnibblesBoni *tmp;

	tmp = (GnibblesBoni *) malloc (sizeof (GnibblesBoni));
	for (i = 0; i < MAXBONUSES; i++)
		tmp->bonuses[i] = NULL;
	tmp->numbonuses = 0;
	tmp->numleft = NUMBONI;
	tmp->missed = 0;

	return tmp;
}

void gnibbles_boni_destroy (GnibblesBoni *boni)
{
	int i;

	for (i = 0; i < boni->numbonuses; i++)
		free (boni->bonuses[i]);
	boni->numbonuses = 0;
	free (boni);
}

void gnibbles_boni_add_bonus (GnibblesBoni *boni, gint t_x, gint t_y,
		gint t_type, gint t_fake, gint t_countdown)
{
	if (boni->numbonuses == MAXBONUSES)
		return;
	boni->bonuses[boni->numbonuses] = gnibbles_bonus_new (t_x, t_y,
			t_type, t_fake, t_countdown);
	board[t_x][t_y] = t_type + 'A';
	board[t_x + 1][t_y] = t_type + 'A';
	board[t_x][t_y + 1] = t_type + 'A';
	board[t_x + 1][t_y + 1] = t_type + 'A';
	gnibbles_bonus_draw (boni->bonuses[boni->numbonuses]);
	boni->numbonuses++;
	if (t_type != BONUSREGULAR)
		gnibbles_play_sound ("appear");
}

int gnibbles_boni_fake (GnibblesBoni *boni, gint x, gint y)
{
	int i;

	for (i = 0; i < boni->numbonuses; i++) {
		if ((x == boni->bonuses[i]->x && 
				y == boni->bonuses[i]->y) ||
				(x == boni->bonuses[i]->x + 1 &&
				y == boni->bonuses[i]->y) ||
				(x == boni->bonuses[i]->x &&
				y == boni->bonuses[i]->y + 1) ||
				(x == boni->bonuses[i]->x + 1 &&
				y == boni->bonuses[i]->y + 1)) {
			return (boni->bonuses[i]->fake);
		}
	}
	return 0;
}

void gnibbles_boni_remove_bonus (GnibblesBoni *boni, gint x, gint y) { 
	int i;

	for (i = 0; i < boni->numbonuses; i++) {
		if ((x == boni->bonuses[i]->x && 
				y == boni->bonuses[i]->y) ||
				(x == boni->bonuses[i]->x + 1 &&
				y == boni->bonuses[i]->y) ||
				(x == boni->bonuses[i]->x &&
				y == boni->bonuses[i]->y + 1) ||
				(x == boni->bonuses[i]->x + 1 &&
				y == boni->bonuses[i]->y + 1)) {
			board[boni->bonuses[i]->x][boni->bonuses[i]->y] =
				EMPTYCHAR;
			board[boni->bonuses[i]->x + 1][boni->bonuses[i]->y] =
				EMPTYCHAR;
			board[boni->bonuses[i]->x][boni->bonuses[i]->y + 1] =
				EMPTYCHAR;
			board[boni->bonuses[i]->x + 1][boni->bonuses[i]->y + 1]
				= EMPTYCHAR;
			gnibbles_bonus_erase (boni->bonuses[i]);
			boni->bonuses[i] = boni->bonuses[--boni->numbonuses];
			return;
		}
	}
}
