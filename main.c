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

#include <config.h>
#include <gnome.h>
#include <string.h>
#include <libgnomeui/gnome-window-icon.h>
#include <gdk/gdkkeysyms.h>
#include <time.h>
#include <gconf/gconf-client.h>

#include "main.h"
#include "properties.h"
#include "gnibbles.h"
#include "worm.h"
#include "bonus.h"
#include "boni.h"
#include "preferences.h"
#include "scoreboard.h"

GtkWidget *window;
GtkWidget *drawing_area;
GtkWidget *appbar;

extern GdkPixmap *buffer_pixmap;
extern GdkPixmap *gnibbles_pixmap;
extern GdkPixmap *logo_pixmap;

GnibblesProperties *properties;

GnibblesScoreboard *scoreboard;

extern GnibblesBoni *boni;

gchar board[BOARDWIDTH][BOARDHEIGHT];

gint main_id = 0;
gint dummy_id = 0;
gint keyboard_id = 0;
gint erase_id = 0;
gint add_bonus_id = 0;
gint restart_id = 0;

gint paused = 0;

gint current_level;

/*
guint pixmap_width = 10, pixmap_height = 10;
*/

static struct _pointers {
	GdkCursor *current;
	GdkCursor *invisible;
} pointers = { NULL };

static gint main_loop (gpointer data);
static gint add_bonus_cb (gpointer data);
static void render_logo (void);
static gint new_game_cb (GtkWidget *widget, gpointer data);
gint pause_game_cb (GtkWidget *widget, gpointer data);
static gint end_game_cb (GtkWidget *widget, gpointer data);
static void quit_cb (GtkWidget *widget, gpointer data);
static void about_cb (GtkWidget *widget, gpointer data);
static void show_scores_cb (GtkWidget *widget, gpointer data);

static GnomeUIInfo game_menu[] = {
	GNOMEUIINFO_MENU_NEW_GAME_ITEM (new_game_cb, NULL),
	GNOMEUIINFO_MENU_PAUSE_GAME_ITEM (pause_game_cb, NULL),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_SCORES_ITEM (show_scores_cb, NULL),
	GNOMEUIINFO_MENU_END_GAME_ITEM (end_game_cb, (gpointer) 2),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_QUIT_ITEM (quit_cb, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo settings_menu[] = {
	GNOMEUIINFO_MENU_PREFERENCES_ITEM (gnibbles_preferences_cb, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo help_menu[] = {
	GNOMEUIINFO_HELP ("gnibbles"),
	GNOMEUIINFO_MENU_ABOUT_ITEM (about_cb, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo main_menu[] = {
	GNOMEUIINFO_MENU_GAME_TREE (game_menu),
	GNOMEUIINFO_MENU_SETTINGS_TREE (settings_menu),
	GNOMEUIINFO_MENU_HELP_TREE (help_menu),
	GNOMEUIINFO_END
};

static void
hide_cursor (void)
{
	if (pointers.current != pointers.invisible) {
		gdk_window_set_cursor (drawing_area->window, pointers.invisible);
		pointers.current = pointers.invisible;
	}
}

static void
show_cursor (void)
{
	if (pointers.current != NULL) {
		gdk_window_set_cursor (drawing_area->window, NULL);
		pointers.current = NULL;
	}
}

gint 
game_running (void)
{
	return (main_id || erase_id || dummy_id || restart_id || paused);
}

static void
zero_board (void)
{
	gint i, j;

	for (i = 0; i < BOARDWIDTH; i++)
		for (j = 0; j < BOARDHEIGHT; j++) {
			board[i][j] = EMPTYCHAR;
			gnibbles_draw_pixmap_buffer (0, i, j);
		}
}

static void
quit_cb (GtkWidget *widget, gpointer data)
{
	gtk_main_quit ();
}

static void
about_cb (GtkWidget *widget, gpointer data)
{
	static GtkWidget *about = NULL;
	GdkPixbuf *pixbuf = NULL;

	const gchar *authors[] = {"Sean MacIsaac", "Ian Peters", NULL};
	gchar *documenters[] = {
                NULL
        };
        /* Translator credits */
        gchar *translator_credits = _("translator_credits");

	if (about != NULL) {
		gtk_window_present (GTK_WINDOW(about));
		return;
	}

	{
		char *filename = NULL;

		filename = gnome_program_locate_file (NULL,
				GNOME_FILE_DOMAIN_APP_PIXMAP, "gnome-nibbles.png",
				TRUE, NULL);
		if (filename != NULL)
		{
			pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
			g_free (filename);
		}
	}

	about = gnome_about_new (_("Gnibbles"), VERSION,
				 "Copyright \xc2\xa9 1999-2003 Sean MacIsaac, "
				 "Ian Peters",
				 _("A worm game for GNOME."),
				 (const char **)authors,
				 (const char **)documenters,
				 strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
				 pixbuf);
   
	if (pixbuf != NULL)
		gdk_pixbuf_unref (pixbuf);
	
	gtk_window_set_transient_for (GTK_WINDOW (about), GTK_WINDOW (window));
	g_signal_connect (G_OBJECT (about), "destroy", G_CALLBACK
			(gtk_widget_destroyed), &about);
	gtk_widget_show (about);
}

static gint expose_event_cb (GtkWidget *widget, GdkEventExpose *event)
{
	gdk_draw_drawable (GDK_DRAWABLE (widget->window),
			   widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
			   buffer_pixmap, event->area.x, event->area.y,
			   event->area.x, event->area.y, event->area.width,
			   event->area.height);

	return (FALSE);
}

static gint key_press_cb (GtkWidget *widget, GdkEventKey *event)
{
	hide_cursor ();
	return gnibbles_keypress_worms (event->keyval);
}

static void
draw_board ()
{
	int i, j;

	for (i = 0; i < BOARDWIDTH; i++)
		for (j = 0; j < BOARDHEIGHT; j++)
			if (board[i][j] >= EMPTYCHAR &&
			    board[i][j] < EMPTYCHAR + 19) {
				gnibbles_draw_pixmap_buffer (
					board[i][j] - EMPTYCHAR, i, j);
			} else if (board[i][j] >= WORMCHAR &&
				   board[i][j] < WORMCHAR + NUMWORMS) {
				gnibbles_draw_pixmap_buffer (
					properties->wormprops
					[board[i][j] - WORMCHAR]->color, i, j);
			} else if (board[i][j] >= 'A' && board[i][j] < 'J') {
				/* bonus */
			} else {
				g_warning("Unknown board charact at "
					  "%d, %d: '%c' (%u)\n", i, j,
					  board[i][j], board[i][j]);
			}

	for (i=0; i<boni->numbonuses; i++)
		gnibbles_bonus_draw (boni->bonuses[i]);

	gdk_draw_drawable (GDK_DRAWABLE (drawing_area->window),
			   drawing_area->style->fg_gc[GTK_WIDGET_STATE (drawing_area)],
			   buffer_pixmap, 0, 0, 0, 0,
			   BOARDWIDTH*properties->tilesize,
			   BOARDHEIGHT*properties->tilesize);
	
}

static gint
configure_event_cb (GtkWidget *widget, GdkEventConfigure *event)
{
	gnibbles_load_pixmap (window);
	if (buffer_pixmap)
		g_object_unref (G_OBJECT (buffer_pixmap));
	buffer_pixmap = gdk_pixmap_new (drawing_area->window,
					BOARDWIDTH * properties->tilesize,
					BOARDHEIGHT * properties->tilesize, -1);

	if (game_running ())
		draw_board ();
	else
		render_logo ();

	return (FALSE);
}

static gint
new_game_2_cb (GtkWidget *widget, gpointer data)
{
	if (!paused) {
		if (!keyboard_id)
			keyboard_id = g_signal_connect (G_OBJECT (window),
							"key_press_event",
							G_CALLBACK (key_press_cb),
							NULL);
		if (!main_id)
			main_id = gtk_timeout_add (GAMEDELAY * properties->gamespeed,
						   (GtkFunction) main_loop,
						   NULL);
		if (!add_bonus_id)
			add_bonus_id = gtk_timeout_add (BONUSDELAY *
							properties->gamespeed,
							(GtkFunction) add_bonus_cb,
							NULL);
	}

	dummy_id = 0;

	return (FALSE);
}

static gint
new_game_cb (GtkWidget *widget, gpointer data)
{
	gtk_widget_set_sensitive (game_menu[1].widget, TRUE);
	gtk_widget_set_sensitive (game_menu[4].widget, TRUE);
	/*
	gtk_widget_set_sensitive (settings_menu[0].widget, FALSE);
	*/

	if (game_running ()) {
			end_game_cb (widget, (gpointer) 0);
			main_id = 0;
	}

	gnibbles_init ();

	if (!properties->random)
		current_level = properties->startlevel;
	else
		current_level = rand () % MAXLEVEL + 1;
	
	gnibbles_load_level (GTK_WIDGET (window), current_level);

	gnibbles_add_bonus (1);
	
	paused = 0;
	
	if (erase_id) {
		gtk_timeout_remove (erase_id);
		erase_id = 0;
	}

	if (restart_id) {
		gtk_timeout_remove (restart_id);
		restart_id = 0;
	}

	if (add_bonus_id) {
		gtk_timeout_remove (add_bonus_id);
		add_bonus_id = 0;
	}

	if (dummy_id)
		gtk_timeout_remove (dummy_id);

	dummy_id = gtk_timeout_add (1500, (GtkFunction) new_game_2_cb, NULL);

	return TRUE;
}

gint pause_game_cb (GtkWidget *widget, gpointer data)
{
	if (paused) {
		paused = 0;
		dummy_id = gtk_timeout_add (500, (GtkFunction) new_game_2_cb,
				NULL);
		/*
		main_id = gtk_timeout_add (GAMEDELAY * properties->gamespeed,
				(GtkFunction) main_loop, NULL);
		keyboard_id = gtk_signal_connect (GTK_OBJECT (window),
			"key_press_event", GTK_SIGNAL_FUNC (key_press_cb),
			NULL);
		add_bonus_id = gtk_timeout_add (BONUSDELAY *
				properties->gamespeed,
				(GtkFunction) add_bonus_cb,
				NULL);
				*/
	} else {
		if (main_id || erase_id || restart_id || dummy_id ) {
			paused = 1;
			if (main_id) {
				gtk_timeout_remove (main_id);
				main_id = 0;
			}
			if (keyboard_id) {
				g_signal_handler_disconnect (G_OBJECT (window),
						keyboard_id);
				keyboard_id = 0;
			}
			if (add_bonus_id) {
				gtk_timeout_remove (add_bonus_id);
				add_bonus_id = 0;
			}
		}
	}
	return TRUE;
}

static void show_scores_cb (GtkWidget *widget, gpointer data)
{
	gnibbles_show_scores (window, 0);
}

void
end_game (gint data)
{
	if (main_id) {
		gtk_timeout_remove (main_id);
		main_id = 0;
	}

	if (keyboard_id) {
		g_signal_handler_disconnect (G_OBJECT (window), keyboard_id);
		keyboard_id = 0;
	}

	if (add_bonus_id) {
		gtk_timeout_remove (add_bonus_id);
		add_bonus_id = 0;
	}

	if (erase_id) {
		gtk_timeout_remove (erase_id);
		erase_id = 0;
	}

	if (dummy_id) {
		gtk_timeout_remove (dummy_id);
		dummy_id = 0;
	}

	if (restart_id) {
		gtk_timeout_remove (restart_id);
		restart_id = 0;
	}

	if ((gint) data) {
		render_logo ();
		gtk_widget_set_sensitive (game_menu[1].widget, FALSE);
		gtk_widget_set_sensitive (game_menu[4].widget, FALSE);
		/*
		gtk_widget_set_sensitive (settings_menu[0].widget, TRUE);
		*/
	}

	paused = 0;
}

static gint end_game_cb (GtkWidget *widget, gpointer data)
{
	end_game ((gint)data);
	return (FALSE);
}

static gint add_bonus_cb (gpointer data)
{
	gnibbles_add_bonus (0);

	return (TRUE);
}

static gint restart_game (gpointer data)
{
	gnibbles_load_level (GTK_WIDGET (window), current_level);

	gnibbles_add_bonus (1);

	dummy_id = gtk_timeout_add (1500, (GtkFunction) new_game_2_cb,
			NULL);

	restart_id = 0;

	return (FALSE);
}

static gint erase_worms_cb (gpointer data)
{
	if ((gint) data == 0) {
		erase_id = 0;
		if (!restart_id)
			end_game_cb (NULL, (gpointer) 1);
	} else {
		gnibbles_undraw_worms (ERASESIZE - (gint) data);
		erase_id = gtk_timeout_add (ERASETIME / ERASESIZE,
				(GtkFunction) erase_worms_cb,
				(gpointer) ((gint) data - 1));
	}

	return (FALSE);
}

static gint main_loop (gpointer data)
{
	gint status;
	gint tmp;

	status = gnibbles_move_worms ();

	gnibbles_scoreboard_update (scoreboard);

	if (status == GAMEOVER) {
		g_signal_handler_disconnect (G_OBJECT (window), keyboard_id);
		keyboard_id = 0;
		main_id = 0;
		gtk_timeout_remove (add_bonus_id);
		add_bonus_id = 0;
		erase_id = gtk_timeout_add (3000,
					    (GtkFunction) erase_worms_cb,
					    (gpointer) ERASESIZE);
		gnibbles_log_score (window);
		return (FALSE);
	}

	if (status == NEWROUND) {
		g_signal_handler_disconnect (G_OBJECT (window), keyboard_id);
		keyboard_id = 0;
		gtk_timeout_remove (add_bonus_id);
		add_bonus_id = 0;
		main_id = 0;
		erase_id = gtk_timeout_add (ERASETIME / ERASESIZE,
				(GtkFunction) erase_worms_cb,
				(gpointer) ERASESIZE);
		restart_id = gtk_timeout_add (1000, (GtkFunction) restart_game,
				NULL);
		return (FALSE);
	}

	if (boni->numleft == 0) {
		g_signal_handler_disconnect (G_OBJECT (window), keyboard_id);
		keyboard_id = 0;
		gtk_timeout_remove (add_bonus_id);
		add_bonus_id = 0;
		main_id = 0;
		if ((current_level < MAXLEVEL) && !properties->random)
			current_level++;
		else if (properties->random) {
			tmp = rand () % MAXLEVEL + 1;
			while (tmp == current_level)
				tmp = rand () % MAXLEVEL + 1;
			current_level = tmp;
		}
		restart_id = gtk_timeout_add (1000, (GtkFunction) restart_game,
				NULL);
		return (FALSE);
	}

	return (TRUE);
}

static void
set_bg_color (void)
{
	GdkImage *tmp_image;
	GdkColor bgcolor;

	tmp_image = gdk_drawable_get_image (GDK_DRAWABLE (gnibbles_pixmap),
					    0, 0, 1, 1);
	bgcolor.pixel = gdk_image_get_pixel (tmp_image, 0, 0);
	gdk_window_set_background (drawing_area->window, &bgcolor);
	g_object_unref (G_OBJECT (tmp_image));
}

void
update_score_state (void)
{
        gchar **names = NULL;
        gfloat *scores = NULL;
        time_t *scoretimes = NULL;
	gint top;

	gchar *buf = NULL;
	buf = g_strdup_printf ("%d.%d", properties->gamespeed,
			       properties->fakes);

	top = gnome_score_get_notable ("gnibbles", buf,
				       &names, &scores, &scoretimes);
	g_free (buf);
	if (top > 0) {
		gtk_widget_set_sensitive (game_menu[3].widget, TRUE);
		g_strfreev (names);
		g_free (scores);
		g_free (scoretimes);
	} else {
		gtk_widget_set_sensitive (game_menu[3].widget, FALSE);
	}
}

static gboolean
show_cursor_cb (GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
        show_cursor ();
        return FALSE;
}

static void
setup_window (void)
{
	GdkPixmap *cursor_dot_pm;

	window = gnome_app_new ("gnibbles", "GNOME Nibbles");
	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	gtk_widget_realize (window);
	g_signal_connect (G_OBJECT (window), "delete_event",
			G_CALLBACK (quit_cb), NULL);

	drawing_area = gtk_drawing_area_new ();

	cursor_dot_pm = gdk_pixmap_create_from_data(
		window->window, "\0", 1, 1, 1,
		&drawing_area->style->fg[GTK_STATE_ACTIVE],
		&drawing_area->style->bg[GTK_STATE_ACTIVE]);

	pointers.invisible = gdk_cursor_new_from_pixmap (
		cursor_dot_pm, cursor_dot_pm,
		&drawing_area->style->fg[GTK_STATE_ACTIVE],
		&drawing_area->style->bg[GTK_STATE_ACTIVE], 0, 0);

	gnome_app_set_contents (GNOME_APP (window), drawing_area);

	g_signal_connect (G_OBJECT (drawing_area), "configure_event",
			G_CALLBACK (configure_event_cb), NULL);

	g_signal_connect (G_OBJECT(drawing_area), "motion_notify_event",
			G_CALLBACK (show_cursor_cb), NULL);

	g_signal_connect (G_OBJECT(window), "focus_out_event",
			G_CALLBACK (show_cursor_cb), NULL);

	gtk_widget_set_size_request (GTK_WIDGET (drawing_area),
				     BOARDWIDTH*properties->tilesize,
				     BOARDHEIGHT*properties->tilesize);
	g_signal_connect (G_OBJECT (drawing_area), "expose_event",
			G_CALLBACK (expose_event_cb), NULL);
	gtk_widget_set_events (drawing_area, GDK_BUTTON_PRESS_MASK |
			GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK);
	gtk_widget_show (drawing_area);

	gnome_app_create_menus (GNOME_APP (window), main_menu);

	appbar = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_USER);
	gnome_app_set_statusbar (GNOME_APP (window), appbar);
	gnome_app_install_menu_hints (GNOME_APP (window), main_menu);

	scoreboard = gnibbles_scoreboard_new (appbar);
}

static void
gconf_key_change_cb (GConfClient *client, guint cnxn_id,
		     GConfEntry *entry, gpointer user_data)
{
	end_game (1);
	gnibbles_properties_destroy (properties);
	properties = gnibbles_properties_new ();
}

static void
load_properties ()
{
	gint i;
	gchar *buffer;

	properties = gnibbles_properties_new ();

	/* maybe this should to into properties.c */
        gconf_client_add_dir (gconf_client_get_default (),
                              KEY_DIR,
                              GCONF_CLIENT_PRELOAD_RECURSIVE,
                              NULL);
	gconf_client_notify_add (gconf_client_get_default (),
				 KEY_PREFERENCES_DIR,
				 gconf_key_change_cb, NULL, NULL, NULL);
	for (i = 0; i < NUMWORMS; i++) {
		buffer = g_strdup_printf (KEY_WORM_DIR, i);
		gconf_client_notify_add (gconf_client_get_default (),
					 buffer,
					 gconf_key_change_cb, NULL, NULL, NULL);
		g_free (buffer);
	}
}

static void
render_logo (void)
{
	zero_board ();

	gdk_draw_drawable (GDK_DRAWABLE (buffer_pixmap),
			   drawing_area->style->fg_gc[GTK_WIDGET_STATE (drawing_area)],
			   logo_pixmap,
			   0, 0, 0, 0,
			   BOARDWIDTH * properties->tilesize,
			   BOARDHEIGHT * properties->tilesize);

	gdk_draw_drawable (GDK_DRAWABLE (drawing_area->window),
			   drawing_area->style->fg_gc[GTK_WIDGET_STATE (drawing_area)],
			   buffer_pixmap,
			   0, 0, 0, 0,
			   BOARDWIDTH * properties->tilesize,
			   BOARDHEIGHT * properties->tilesize);
}

int 
main (int argc, char **argv)
{
	gnome_score_init ("gnibbles");

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
	
	gnome_program_init ("gnibbles", VERSION, LIBGNOMEUI_MODULE,
			    argc, argv,
			    GNOME_PARAM_POPT_TABLE, NULL,
			    GNOME_PARAM_APP_DATADIR, REAL_DATADIR,
			    NULL);
	gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/gnome-nibbles.png");
	srand (time (NULL));

	load_properties ();

	setup_window ();

	update_score_state ();

	gnibbles_load_pixmap (window);

	gtk_widget_show (window);

	buffer_pixmap = gdk_pixmap_new (drawing_area->window,
					BOARDWIDTH * properties->tilesize,
					BOARDHEIGHT * properties->tilesize,
					-1);

	render_logo ();

	set_bg_color ();

	gtk_widget_set_sensitive (game_menu[1].widget, FALSE);
	gtk_widget_set_sensitive (game_menu[4].widget, FALSE);

	gtk_main ();

	return 0;
}
