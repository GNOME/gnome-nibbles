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
#include <gdk/gdkkeysyms.h>

#include "gnibbles.h"
#include "worm.h"
#include "boni.h"
#include "bonus.h"
#include "warpmanager.h"
#include "properties.h"
#include "scoreboard.h"

GnibblesWorm *worms[NUMWORMS];
GnibblesBoni *boni = NULL;
GnibblesWarpManager *warpmanager;

GdkPixmap *buffer_pixmap = NULL;
GdkPixmap *gnibbles_pixmap = NULL;
GdkPixmap *logo_pixmap = NULL;

extern GtkWidget *drawing_area;

extern gchar board[BOARDWIDTH][BOARDHEIGHT];

extern GnibblesProperties *properties;

extern GnibblesScoreboard *scoreboard;

/*
extern guint properties->tilesize, properties->tilesize;
*/

void gnibbles_copy_pixmap (GdkDrawable *drawable, gint which, gint x, gint y,
			   gboolean big)
{
	gint w = properties->tilesize * (big ? 2 : 1),
		h = properties->tilesize * (big ? 2 : 1);
	guint nh = 10 / (big ? 2 : 1), nv = 10 / (big ? 2 : 1);

	gdk_draw_pixmap (drawable, drawing_area->style->fg_gc[GTK_WIDGET_STATE
			(drawing_area)], gnibbles_pixmap, (which % nh) * w,
			((big ? 3 : 0) + which / nv) * h, x * properties->tilesize,
			y * properties->tilesize, w, h);
}

void gnibbles_draw_pixmap (gint which, gint x, gint y)
{
	gnibbles_copy_pixmap(drawing_area->window, which, x, y, FALSE);
	gnibbles_copy_pixmap(buffer_pixmap, which, x, y, FALSE);
}

void gnibbles_draw_big_pixmap (gint which, gint x, gint y)
{
	gnibbles_copy_pixmap(drawing_area->window, which, x, y, TRUE);
	gnibbles_copy_pixmap(buffer_pixmap, which, x, y, TRUE);
}

void gnibbles_draw_pixmap_buffer (gint which, gint x, gint y)
{
	gnibbles_copy_pixmap(buffer_pixmap, which, x, y, FALSE);
}

void gnibbles_draw_big_pixmap_buffer (gint which, gint x, gint y)
{
	gnibbles_copy_pixmap(buffer_pixmap, which, x, y, TRUE);
}

void gnibbles_load_pixmap ()
{
	GdkImlibImage *image;
	GdkVisual *visual;
	gchar *filename;

	filename = gnome_unconditional_pixmap_file ("gnibbles/gnibbles.png");

	if (!g_file_exists (filename)) {
		char *message =
		    g_strdup_printf (_("Gnibbles couldn't find pixmap file:\n%s\n\n"
			"Please check your Gnibbles installation"), filename);
		GtkWidget *w = gnome_error_dialog (message);
		gnome_dialog_run_and_close (GNOME_DIALOG(w));
		g_free (message);
		exit (1);
	}

	image = gdk_imlib_load_image (filename);
	visual = gdk_imlib_get_visual ();
	gdk_imlib_render (image, 10 * properties->tilesize, 10 * properties->tilesize);
	gdk_imlib_free_pixmap (gnibbles_pixmap);
	gnibbles_pixmap = gdk_imlib_move_image (image);

	gdk_imlib_destroy_image (image);
	g_free (filename);

	filename = gnome_unconditional_pixmap_file
		("gnibbles/gnibbles-logo.png");

	if (!g_file_exists (filename)) {
		char *message = g_strdup_printf (_("Gnibbles Couldn't find pixmap file:\n%s\n\n"
			"Please check your Gnibbles instalation"), filename);
		GtkWidget *w = gnome_error_dialog (message);
		gnome_dialog_run_and_close (GNOME_DIALOG(w));
		g_free (message);
		exit (1);
	}

	image = gdk_imlib_load_image (filename);
	visual = gdk_imlib_get_visual ();
	gdk_imlib_render (image, BOARDWIDTH * properties->tilesize,
			  BOARDHEIGHT * properties->tilesize);
	gdk_imlib_free_pixmap (logo_pixmap);
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
		char *message = g_strdup_printf (
                        _("Gnibbles couldn't load level file:\n%s\n\n"
                         "Please check your Gnibbles installation"), filename);
                GtkWidget *w = gnome_error_dialog (message);
                gnome_dialog_run_and_close (GNOME_DIALOG(w));
                g_free (message);
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
		worms[i] = gnibbles_worm_new (i);
		gnibbles_scoreboard_register (scoreboard, worms[i]);
	}

	gnibbles_scoreboard_update (scoreboard);
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

	if (scoreboard)
		gnibbles_scoreboard_destroy (scoreboard);
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
	int *dead = g_new (int, properties->numworms);

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

	/* If one worm has died, me must make sure that an earlier worm was not
	   supposed to die as well. */

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
					gnibbles_draw_pixmap
						(properties->wormprops[i]
						 ->color,
						 worms[i]->xhead,
						 worms[i]->yhead);
				}

	for (i = 0; i < properties->numworms; i++)
		if (dead[i]) {
			if (properties->numworms > 1)
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
		gnibbles_play_sound ("crash");
		gnibbles_play_sound ("gameover");
		return (GAMEOVER);
	}

	if (status)
		return (CONTINUE);

	gnibbles_play_sound ("crash");
	g_free (dead);
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

void gnibbles_play_sound (const char *which)
{
	if (properties->sound)
		gnome_triggers_do (NULL, NULL, "gnibbles", which, NULL);
}

void gnibbles_show_scores (gint pos)
{
	char buf[10];

	sprintf (buf, "%d.%d", properties->gamespeed, properties->fakes);

	gnome_scores_display ("Gnibbles", "gnibbles", buf, pos);
}

void gnibbles_log_score ()
{
	char buf[10];
	int pos;
	
	if (properties->numworms > 1)
		return;

	if (properties->startlevel != 1)
		return;

	if (!worms[0]->score)
		return;

	sprintf (buf, "%d.%d", properties->gamespeed, properties->fakes);

	pos = gnome_score_log (worms[0]->score, buf, TRUE);

	gnibbles_show_scores (pos);
}
