#ifndef _WARP_H_
#define _WARP_H_

#include <config.h>
#include <gnome.h>

#define WARP     8

typedef struct {
	gint x, y;
	gint wx, wy;
} GnibblesWarp;

GnibblesWarp *gnibbles_warp_new (gint t_x, gint t_y, gint t_wx, gint t_wy);

void gnibbles_warp_draw_buffer (GnibblesWarp *warp);

#endif
