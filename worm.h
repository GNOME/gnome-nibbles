#ifndef _WORM_H_
#define _WORM_H_

#include <config.h>
#include <gnome.h>

#define WORMNONE  0
#define WORMRIGHT 1
#define WORMDOWN  2
#define WORMLEFT  3
#define WORMUP    4
#define SLENGTH   5
#define SLIVES    3
#define CAPACITY  BOARDWIDTH * BOARDHEIGHT
#define ERASESIZE 6
#define ERASETIME 500

typedef struct
{
	gint xhead, yhead;
	gint xtail, ytail;
	gint pixmap;
	gint direction;
	guint up, down, left, right;
	gint8 *xoff, *yoff;
	gint start, stop;
	gint length;
	gint change;
	gint keypress;
	gint lives;
	guint score;
} GnibblesWorm;

GnibblesWorm *gnibbles_worm_new (gint8 t_pixmap, guint t_up, guint t_down,
		guint t_left, guint t_right);

void gnibbles_worm_destroy (GnibblesWorm *worm);

void gnibbles_worm_set_start (GnibblesWorm *worm, guint t_xhead, guint t_yhead,
		gint t_direction);

void gnibbles_worm_handle_keypress (GnibblesWorm *worm, guint keyval);

gint gnibbles_worm_move_test_head (GnibblesWorm *worm);
void gnibbles_worm_move_tail (GnibblesWorm *worm);
void gnibbles_worm_undraw_nth (GnibblesWorm *worm, gint offset);

#endif
