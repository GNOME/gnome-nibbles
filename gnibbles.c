#include <config.h>
#include <gnome.h>
#include <gdk/gdkkeysyms.h>

#include "gnibbles.h"
#include "worm.h"
#include "boni.h"
#include "bonus.h"
#include "warpmanager.h"
#include "properties.h"
#include "scoreboard.h"

GnibblesWorm *worms[NUMWORMS] = { NULL, NULL, NULL, NULL };
GnibblesBoni *boni = NULL;
GnibblesWarpManager *warpmanager;

GdkPixmap *buffer_pixmap = NULL;
GdkPixmap *gnibbles_pixmap = NULL;
GdkPixmap *logo_pixmap = NULL;

//gint numworms = 1;

extern GtkWidget *drawing_area;

extern gchar board[BOARDWIDTH][BOARDHEIGHT];

extern GnibblesProperties *properties;

extern GnibblesScoreboard *scoreboard;

void gnibbles_draw_pixmap (gint which, gint x, gint y)
{
	gdk_draw_pixmap (drawing_area->window,
			drawing_area->style->fg_gc[GTK_WIDGET_STATE
			(drawing_area)], gnibbles_pixmap, (which % 10) * 10,
			(which / 10) * 10, x * 10, y * 10, 10, 10);
	gdk_draw_pixmap (buffer_pixmap,
			drawing_area->style->fg_gc[GTK_WIDGET_STATE
			(drawing_area)], gnibbles_pixmap, (which % 10) * 10,
			(which / 10) * 10, x * 10, y * 10, 10, 10);
}

void gnibbles_draw_big_pixmap (gint which, gint x, gint y)
{
	gdk_draw_pixmap (drawing_area->window,
			drawing_area->style->fg_gc[GTK_WIDGET_STATE
			(drawing_area)], gnibbles_pixmap, (which % 5) * 20,
			60 + (which / 5) * 20, x * 10, y * 10, 20, 20);
	gdk_draw_pixmap (buffer_pixmap,
			drawing_area->style->fg_gc[GTK_WIDGET_STATE
			(drawing_area)], gnibbles_pixmap, (which % 5) * 20,
			60 + (which / 5) * 20, x * 10, y * 10, 20, 20);
}

void gnibbles_draw_pixmap_buffer (gint which, gint x, gint y)
{
	gdk_draw_pixmap (buffer_pixmap,
			drawing_area->style->fg_gc[GTK_WIDGET_STATE
			(drawing_area)], gnibbles_pixmap, (which % 10) * 10,
			(which / 10) * 10, x * 10, y * 10, 10, 10);
}

void gnibbles_draw_big_pixmap_buffer (gint which, gint x, gint y)
{
	gdk_draw_pixmap (buffer_pixmap,
			drawing_area->style->fg_gc[GTK_WIDGET_STATE
			(drawing_area)], gnibbles_pixmap, (which % 5) * 20,
			60 + (which / 5) * 20, x * 10, y * 10, 20, 20);
}

void gnibbles_load_pixmap ()
{
	GdkImlibImage *image;
	GdkVisual *visual;
	gchar *filename;

	filename = gnome_unconditional_pixmap_file ("gnibbles/gnibbles.png");

	if (!g_file_exists (filename)) {
		g_print (_("Couldn't find pixmap file!\n"));
		exit (1);
	}

	image = gdk_imlib_load_image (filename);
	visual = gdk_imlib_get_visual ();
	/*
	if (visual->type != GDK_VISUAL_TRUE_COLOR) {
		gdk_imlib_set_render_type (RT_PLAIN_PALETTE);
	}
	*/
	gdk_imlib_render (image, image->rgb_width, image->rgb_height);
	gnibbles_pixmap = gdk_imlib_move_image (image);

	gdk_imlib_destroy_image (image);
	g_free (filename);

	filename = gnome_unconditional_pixmap_file
		("gnibbles/gnibbles_logo.png");

	if (!g_file_exists (filename)) {
		g_print (_("Couldn't find pixmap file!\n"));
		exit (1);
	}

	image = gdk_imlib_load_image (filename);
	visual = gdk_imlib_get_visual ();
	/*
	if (visual->type != GDK_VISUAL_TRUE_COLOR) {
		gdk_imlib_set_render_type (RT_PLAIN_PALETTE);
	}
	*/
	gdk_imlib_render (image, image->rgb_width, image->rgb_height);
	logo_pixmap = gdk_imlib_move_image (image);

	gdk_imlib_destroy_image (image);
	g_free (filename);
}

void gnibbles_load_level (int level)
{
	gchar tmp[30];
	gchar *filename;
	FILE *in;
	gchar tmpboard[BOARDWIDTH + 1];
	int i, j;
	int count = 0;

	sprintf (tmp, "gnibbles/level%03d.gnl", level);
	filename = gnome_unconditional_datadir_file (tmp);

	if ((in = fopen (filename, "r")) == NULL) {
		printf ("This is really bad I'll figure out later\n");
		exit (1);
	}

	g_free (filename);

	if (warpmanager)
		gnibbles_warpmanager_destroy (warpmanager);

	warpmanager = gnibbles_warpmanager_new ();

	if (boni)
		gnibbles_boni_destroy (boni);

	boni = gnibbles_boni_new ();

	for (i = 0; i < BOARDHEIGHT; i++) {
		fgets (tmpboard, 255, in);
		for (j = 0; j < BOARDWIDTH; j++) {
			board[j][i] = tmpboard[j];
			switch (board[j][i]) {
				case 'm':
					board[j][i] = 'a';
					if (count < properties->numworms)
						gnibbles_worm_set_start
							(worms[count++], j, i,
						 	WORMUP);
					break;
				case 'n':
					board[j][i] = 'a';
					if (count < properties->numworms)
						gnibbles_worm_set_start
							(worms[count++], j, i,
							 WORMLEFT);
					break;
				case 'o':
					board[j][i] = 'a';
					if (count < properties->numworms)
						gnibbles_worm_set_start
							(worms[count++], j, i,
							 WORMDOWN);
					break;
				case 'p':
					board[j][i] = 'a';
					if (count < properties->numworms)
						gnibbles_worm_set_start
							(worms[count++], j, i,
						 	WORMRIGHT);
					break;
				case 'Q':
					gnibbles_warpmanager_add_warp
						(warpmanager, j - 1, i - 1,
						 -1, -1);
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
						(warpmanager, j - 1, i - 1,
						 -board[j][i], 0);
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
						(warpmanager,
						 -(board[j][i]-'a'+'A'), 0,
						 j, i);
					board[j][i] = EMPTYCHAR;
					break;
			}
			gnibbles_draw_pixmap_buffer (board[j][i]-'a', j, i);
		}
	}

	gdk_draw_pixmap (drawing_area->window, drawing_area->style->fg_gc
			[GTK_WIDGET_STATE (drawing_area)], buffer_pixmap, 0, 0,
			0, 0, BOARDWIDTH * 10, BOARDHEIGHT * 10);

	fclose (in);
}

void gnibbles_init ()
{
	int i;

	for (i = 0; i < properties->numworms; i++)
		if (worms[i])
			gnibbles_worm_destroy (worms[i]);
	
	gnibbles_scoreboard_clear (scoreboard);

	for (i = 0; i < properties->numworms; i++) {
		worms[i] = gnibbles_worm_new (properties->wormprops[i]->color,
				properties->wormprops[i]->up,
				properties->wormprops[i]->down,
				properties->wormprops[i]->left,
				properties->wormprops[i]->right);
		gnibbles_scoreboard_register (scoreboard, worms[i]);
	}

	gnibbles_scoreboard_update (scoreboard);
	/*
	worms[0] = gnibbles_worm_new (WORMCYAN, GDK_Up, GDK_Down, GDK_Left,
			GDK_Right);
	if (numworms > 1) 
		worms[1] = gnibbles_worm_new (WORMGRAY, GDK_w, GDK_s, GDK_a,
				GDK_d);
				*/
}

void gnibbles_destroy ()
{
	int i;

	if (warpmanager)
		gnibbles_warpmanager_destroy (warpmanager);

	if (boni)
		gnibbles_boni_destroy (boni);

	for (i = 0; i < properties->numworms; i++)
		if (worms[i])
			gnibbles_worm_destroy (worms[i]);

	if (properties)
		gnibbles_properties_destroy (properties);
}

void gnibbles_add_bonus (int regular)
{
	gint x, y, good;

	if (regular) {
		good = 0;
	} else {
		good = rand() % 50;
		if (good)
			return;
	}

	while (!good) {
		good = 1;
		x = rand() % (BOARDWIDTH - 1);
		y = rand() % (BOARDHEIGHT - 1);
		if (board[x][y] != EMPTYCHAR)
			good = 0;
		if (board[x + 1][y] != EMPTYCHAR)
			good = 0;
		if (board[x][y + 1] != EMPTYCHAR)
			good = 0;
		if (board[x + 1][y + 1] != EMPTYCHAR)
			good = 0;
	}

	if (regular) {
		if ((rand() % 7 == 0) && properties->fakes)
			gnibbles_boni_add_bonus (boni, x, y, BONUSREGULAR, 1,
					300);
		good = 0;
		while (!good) {
			good = 1;
			x = rand() % (BOARDWIDTH - 1);
			y = rand() % (BOARDHEIGHT - 1);
			if (board[x][y] != EMPTYCHAR)
				good = 0;
			if (board[x + 1][y] != EMPTYCHAR)
				good = 0;
			if (board[x][y + 1] != EMPTYCHAR)
				good = 0;
			if (board[x + 1][y + 1] != EMPTYCHAR)
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
				gnibbles_boni_add_bonus (boni, x, y, BONUSHALF,
						good, 200);
				break;
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
				gnibbles_boni_add_bonus (boni, x, y,
						BONUSDOUBLE, good, 150);
				break;
			case 15:
				gnibbles_boni_add_bonus (boni, x, y, BONUSLIFE,
						good, 100);
				break;
			case 16:
			case 17:
			case 18:
			case 19:
			case 20:
				if (properties->numworms > 1)
					gnibbles_boni_add_bonus (boni, x, y,
							BONUSREVERSE, good,
							150);
				break;
		}
	}
}

gint gnibbles_move_worms ()
{
	int i, j, status = 1;
	int dead[properties->numworms];

	if (boni->missed > MAXMISSED)
		for (i = 0; i < properties->numworms; i++)
			if (worms[i]->score)
				worms[i]->score--;
	
	for (i = 0; i < boni->numbonuses; i++) {
		if (!(boni->bonuses[i]->countdown--))
			if (boni->bonuses[i]->type == BONUSREGULAR &&
					!boni->bonuses[i]->fake) {
				gnibbles_boni_remove_bonus (boni,
						boni->bonuses[i]->x,
						boni->bonuses[i]->y);
				boni->missed++;
				gnibbles_add_bonus (1);
			} else
				gnibbles_boni_remove_bonus (boni,
						boni->bonuses[i]->x,
						boni->bonuses[i]->y);
	}
		
	for (i = 0; i < properties->numworms; i++) {
		gnibbles_worm_erase_tail (worms[i]);
	}

	for (i = 0; i < properties->numworms; i++) {
		dead[i] = !gnibbles_worm_test_move_head (worms[i]);
		status &= !dead[i];
	}

	// If one worm has died, me must make sure that an earlier worm was not
	// supposed to die as well.

	if (!status)
		for (i = 0; i < properties->numworms; i++)
			if (!dead[i])
				for (j = 0; j < properties->numworms; j++) {
					if (i != j && worms[i]->xhead ==
							worms[j]->xhead &&
							worms[i]->yhead ==
							worms[j]->yhead)
						dead[i] = TRUE;
					gnibbles_draw_pixmap (BLANKPIXMAP,
							worms[i]->xtail,
							worms[i]->ytail);
					gnibbles_draw_pixmap (worms[i]->pixmap,
							worms[i]->xhead,
							worms[i]->yhead);
				}

	for (i = 0; i < properties->numworms; i++)
		if (dead[i]) {
			worms[i]->score *= .7;
			status |= gnibbles_worm_lose_life (worms[i]) << 1;
		}

	for (i = 0; i < properties->numworms; i++)
		if (!dead[i])
			gnibbles_worm_move_tail (worms[i]);

	for (i = 0; i < properties->numworms; i++)
		if (!dead[i])
			gnibbles_worm_draw_head (worms[i]);

	if (status & GAMEOVER) {
		return (GAMEOVER);
	}

	if (status)
		return (CONTINUE);

	return (NEWROUND);
}

void gnibbles_keypress_worms (guint keyval)
{
	int i;

	for (i = 0; i < properties->numworms; i++)
		gnibbles_worm_handle_keypress (worms[i], keyval);
}

void gnibbles_undraw_worms (gint data)
{
	int i;

	for (i = 0; i < properties->numworms; i++)
		gnibbles_worm_undraw_nth (worms[i], data);
}
