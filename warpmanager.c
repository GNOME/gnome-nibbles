#include <config.h>
#include <gnome.h>

#include "gnibbles.h"
#include "warp.h"
#include "warpmanager.h"
#include "boni.h"
#include "worm.h"

extern gchar board[BOARDWIDTH][BOARDHEIGHT];
extern GnibblesBoni *boni;

GnibblesWarpManager *gnibbles_warpmanager_new ()
{
	int i;
	GnibblesWarpManager *tmp;

	tmp = (GnibblesWarpManager *) malloc (sizeof (GnibblesWarpManager));
	for (i = 0; i < MAXWARPS; i++)
		tmp->warps[i] = NULL;
	tmp->numwarps = 0;

	return tmp;
}

void gnibbles_warpmanager_destroy (GnibblesWarpManager *warpmanager)
{
	gint i;

	for (i = 0; i < warpmanager->numwarps; i++)
		free (warpmanager->warps[i]);
	warpmanager->numwarps = 0;
	free (warpmanager);
}

void gnibbles_warpmanager_add_warp (GnibblesWarpManager *warpmanager, gint t_x,
		gint t_y, gint t_wx, gint t_wy)
{
	gint i, add = 1, draw;

	if (t_x < 0) {
		for (i = 0; i < warpmanager->numwarps; i++) {
			if (warpmanager->warps[i]->wx == t_x) {
				warpmanager->warps[i]->wx = t_wx;
				warpmanager->warps[i]->wy = t_wy;
				return;
			}
		}

		if (warpmanager->numwarps == MAXWARPS)
			return;
		warpmanager->warps[warpmanager->numwarps] = gnibbles_warp_new
				(t_x, t_y, t_wx, t_wy);
		warpmanager->numwarps++;
	} else {
		for (i = 0; i < warpmanager->numwarps; i++) {
			if (warpmanager->warps[i]->x == t_wx) {
				warpmanager->warps[i]->x = t_x;
				warpmanager->warps[i]->y = t_y;
				draw = i;
				add = 0;
			}
		}
		if (add) {
			if (warpmanager->numwarps == MAXWARPS)
				return;
			warpmanager->warps[warpmanager->numwarps] =
				gnibbles_warp_new (t_x, t_y, t_wx, t_wy);
			draw = warpmanager->numwarps;
			warpmanager->numwarps++;
		}
		board[t_x][t_y] = WARPLETTER; 
		board[t_x + 1][t_y] = WARPLETTER; 
		board[t_x][t_y + 1] = WARPLETTER;
		board[t_x + 1][t_y + 1] = WARPLETTER;
		gnibbles_warp_draw_buffer
				(warpmanager->warps[draw]);
	}
}

void gnibbles_warpmanager_worm_change_pos (GnibblesWarpManager *warpmanager,
		                GnibblesWorm *worm)
{
        int i, x, y, good;

        for (i = 0; i < warpmanager->numwarps; i++) {
		if ((worm->xhead == warpmanager->warps[i]->x &&
		 		worm->yhead == warpmanager->warps[i]->y) ||
				(worm->xhead == warpmanager->warps[i]->x + 1 &&
		 		worm->yhead == warpmanager->warps[i]->y) ||
				(worm->xhead == warpmanager->warps[i]->x &&
		 		worm->yhead == warpmanager->warps[i]->y + 1) ||
				(worm->xhead == warpmanager->warps[i]->x + 1 &&
		 		worm->yhead == warpmanager->warps[i]->y + 1)) {
			if (warpmanager->warps[i]->wx == -1) {
				good = 0;
				while (!good) {
					x = rand() % BOARDWIDTH;
					y = rand() % BOARDHEIGHT;
					if (board[x][y] == EMPTYCHAR)
						good = 1;
				}
			}	
			else {
				x = warpmanager->warps[i]->wx;
				y = warpmanager->warps[i]->wy;
				if (board[x][y] != EMPTYCHAR)
					gnibbles_boni_remove_bonus (boni, x,
							y);
			}
			worm->xoff[worm->start] += worm->xhead - x;
			worm->yoff[worm->start] += worm->yhead - y;

			worm->xhead = x;
			worm->yhead = y;
		}
	}
}
