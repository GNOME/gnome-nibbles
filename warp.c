#include <config.h>
#include <gnome.h>

#include "gnibbles.h"
#include "warp.h"

GnibblesWarp *gnibbles_warp_new (gint t_x, gint t_y, gint t_wx, gint t_wy)
{
	GnibblesWarp *tmp;

	tmp = (GnibblesWarp *) malloc (sizeof (GnibblesWarp));

	tmp->x = t_x;
	tmp->y = t_y;
	tmp->wx = t_wx;
	tmp->wy = t_wy;

	return (tmp);
}

void gnibbles_warp_draw_buffer (GnibblesWarp *warp)
{
	gnibbles_draw_big_pixmap_buffer (WARP, warp->x, warp->y);
}
