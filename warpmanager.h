#ifndef _WARPMANAGER_H_
#define _WARPMANAGER_H_

#include <config.h>
#include <gnome.h>

#include "warp.h"
#include "worm.h"

#define MAXWARPS 200
#define WARPLETTER 'W'

typedef struct {
	GnibblesWarp *warps[MAXWARPS];
	gint numwarps;
} GnibblesWarpManager;

GnibblesWarpManager *gnibbles_warpmanager_new ();

void gnibbles_warpmanager_destroy (GnibblesWarpManager *warpmanager);

void gnibbles_warpmanager_add_warp (GnibblesWarpManager *warpmanager, gint t_x,
		gint t_y, gint t_wx, gint t_wy);

void gnibbles_warpmanager_worm_change_pos (GnibblesWarpManager *warpmanager,
		GnibblesWorm *worm);

#endif
