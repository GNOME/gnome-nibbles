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
#include <gdk/gdkkeysyms.h>
#include <time.h>
#include <gconf/gconf-client.h>

#include <games-gridframe.h>
#include <games-stock.h>

#include "main.h"
#include "properties.h"
#include "gnibbles.h"
#include "worm.h"
#include "bonus.h"
#include "boni.h"
#include "preferences.h"
#include "scoreboard.h"
#include "network.h"
#include "warp.h"

GtkWidget *window;
GtkWidget *drawing_area;
GtkWidget *appbar;

extern GdkPixmap *buffer_pixmap;
extern GdkPixbuf *logo_pixmap;

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

static struct _pointers {
	GdkCursor *current;
	GdkCursor *invisible;
} pointers = { NULL };

static gint add_bonus_cb (gpointer data);
static void render_logo (void);
static void new_network_game_cb (GtkAction *action, gpointer data);
static gint end_game_cb (GtkAction *action, gpointer data);

static GtkAction *new_network_game_action;
static GtkAction *pause_action;
static GtkAction *end_game_action;
static GtkAction *preferences_action;
static GtkAction *scores_action;

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

/* Avoid a race condition where a redraw is attempted
 * between the window being destroyed and the destroy
 * event being sent. */
static gint
delete_cb (GtkWidget * widget, gpointer data)
{
	if (main_id) g_source_remove (main_id);
	if (erase_id) g_source_remove (erase_id);
	if (dummy_id) g_source_remove (dummy_id);
	if (restart_id) g_source_remove (restart_id);

	return FALSE;
}

static void
quit_cb (GObject *object, gpointer data)
{
	games_kill_server ();
	gtk_main_quit ();
}

static void
about_cb (GtkAction *action, gpointer data)
{
	const gchar *authors[] = {"Sean MacIsaac", "Ian Peters", NULL};

	gtk_show_about_dialog (GTK_WINDOW (window),
			       "name", _("Nibbles"), 
			       "version", VERSION,
			       "copyright", "Copyright \xc2\xa9 1999-2004 Sean MacIsaac, Ian Peters",
			       "comments", _("A worm game for GNOME."),
			       "authors", authors,
			       "translator-credits", _("translator-credits"),
			       "logo-icon-name", "gnome-nibbles",
			       NULL);
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

	for (i = 0; i < BOARDWIDTH; i++) {
		for (j = 0; j < BOARDHEIGHT; j++) {
			gnibbles_draw_pixmap_buffer (0, i, j);
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
				/* Warp point. */
			}
		}
	}

	for (i=0; i<boni->numbonuses; i++)
		gnibbles_bonus_draw (boni->bonuses[i]);

	for (i=0; i<warpmanager->numwarps; i++)
		gnibbles_warp_draw_buffer (warpmanager->warps[i]);

	gdk_draw_drawable (GDK_DRAWABLE (drawing_area->window),
			   drawing_area->style->fg_gc[GTK_WIDGET_STATE (drawing_area)],
			   buffer_pixmap, 0, 0, 0, 0,
			   BOARDWIDTH*properties->tilesize,
			   BOARDHEIGHT*properties->tilesize);
	
}

static gint
window_configure_event_cb (GtkWidget *widget, GdkEventConfigure *event)
{
	gnibbles_properties_set_height(event->height);
	gnibbles_properties_set_width(event->width);
	return FALSE;
}

static gint
configure_event_cb (GtkWidget *widget, GdkEventConfigure *event)
{
	int tilesize, ts_x, ts_y;

	/* Compute the new tile size based on the size of the
	 * drawing area, rounded down. */
	ts_x = event->width / BOARDWIDTH;
	ts_y = event->height / BOARDHEIGHT;
	if (ts_x * BOARDWIDTH > event->width)
		ts_x--;
	if (ts_y * BOARDHEIGHT > event->height)
		ts_y--;
	tilesize = MIN(ts_x, ts_y);

	/* But, has the tile size changed? */
	if (properties->tilesize == tilesize) {

		/* We must always re-load the logo. */
		gnibbles_load_logo (window);
		return FALSE;
	}

	properties->tilesize = tilesize;
	gnibbles_properties_set_tile_size(tilesize);

	/* Reload the images pixmap. */
	gnibbles_load_logo (window);
	gnibbles_load_pixmap (window);

	/* Recreate the buffer pixmap. */
	if (buffer_pixmap)
		g_object_unref (G_OBJECT (buffer_pixmap));
	buffer_pixmap = gdk_pixmap_new (drawing_area->window,
					BOARDWIDTH * properties->tilesize,
					BOARDHEIGHT * properties->tilesize, -1);

	/* Erase the buffer pixmap. */
	gdk_draw_rectangle(buffer_pixmap,
				drawing_area->style->black_gc,
				TRUE, 0, 0,
				BOARDWIDTH * properties->tilesize,
				BOARDHEIGHT * properties->tilesize);

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
		if (!main_id && network_is_host ())
			main_id = g_timeout_add (GAMEDELAY * properties->gamespeed,
						   (GSourceFunc) main_loop,
						   NULL);
		if (!add_bonus_id && network_is_host ()) 
			add_bonus_id = g_timeout_add (BONUSDELAY *
							properties->gamespeed,
							(GSourceFunc) add_bonus_cb,
							NULL);
	}

	dummy_id = 0;

	return (FALSE);
}

gint
new_game (void)
{
        gtk_action_set_sensitive (new_network_game_action, FALSE);
	if (is_network_running ()) {
		gtk_action_set_sensitive (pause_action, FALSE);
	} else {
               gtk_action_set_sensitive (pause_action, TRUE);
	}
	gtk_action_set_sensitive (end_game_action, TRUE);

	if (game_running ()) {
			end_game (FALSE);
			main_id = 0;
	}

	gnibbles_init ();

	if (is_network_running ()) {
		current_level = 1;
	} else if (!properties->random) {
		current_level = properties->startlevel;
	} else {
		current_level = rand () % MAXLEVEL + 1;
	}

	zero_board();
	gnibbles_load_level (GTK_WIDGET (window), current_level);

	gnibbles_add_bonus (1);
	
	paused = 0;
	
	if (erase_id) {
		g_source_remove (erase_id);
		erase_id = 0;
	}

	if (restart_id) {
		g_source_remove (restart_id);
		restart_id = 0;
	}

	if (add_bonus_id) {
		g_source_remove (add_bonus_id);
		add_bonus_id = 0;
	}

	if (dummy_id)
		g_source_remove (dummy_id);

	dummy_id = g_timeout_add (1500, (GSourceFunc) new_game_2_cb, NULL);

	return TRUE;
}

static void
new_game_cb (GtkAction *action, gpointer data)
{
   new_game ();
}


gint pause_game_cb (GtkAction *action, gpointer data)
{
	if (paused) {
		paused = 0;
		dummy_id = g_timeout_add (500, (GSourceFunc) new_game_2_cb,
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
				g_source_remove (main_id);
				main_id = 0;
			}
			if (keyboard_id) {
				g_signal_handler_disconnect (G_OBJECT (window),
						keyboard_id);
				keyboard_id = 0;
			}
			if (add_bonus_id) {
				g_source_remove (add_bonus_id);
				add_bonus_id = 0;
			}
		}
	}
	return TRUE;
}

static void show_scores_cb (GtkAction *action, gpointer data)
{
	gnibbles_show_scores (window, 0);
}

void
end_game (gboolean show_splash)
{
	if (main_id) {
		g_source_remove (main_id);
		main_id = 0;
	}

	if (keyboard_id) {
		g_signal_handler_disconnect (G_OBJECT (window), keyboard_id);
		keyboard_id = 0;
	}

	if (add_bonus_id) {
		g_source_remove (add_bonus_id);
		add_bonus_id = 0;
	}

	if (erase_id) {
		g_source_remove (erase_id);
		erase_id = 0;
	}

	if (dummy_id) {
		g_source_remove (dummy_id);
		dummy_id = 0;
	}

	if (restart_id) {
		g_source_remove (restart_id);
		restart_id = 0;
	}

	if (show_splash) {
		render_logo ();
		gtk_action_set_sensitive (new_network_game_action, TRUE);
		gtk_action_set_sensitive (pause_action, FALSE);
		gtk_action_set_sensitive (end_game_action, FALSE);
		gtk_action_set_sensitive (preferences_action, TRUE);
	}

	paused = 0;

	if (is_network_running ()) {
		network_stop ();
	}
}

static gint end_game_cb (GtkAction *action, gpointer data)
{
	end_game (TRUE);
	return (FALSE);
}

static gint add_bonus_cb (gpointer data)
{
	gnibbles_add_bonus (0);

	return (TRUE);
}

static gint restart_game (gpointer data)
{
	zero_board();
	gnibbles_load_level (GTK_WIDGET (window), current_level);

	gnibbles_add_bonus (1);

	dummy_id = g_timeout_add (1500, (GSourceFunc) new_game_2_cb,
				  NULL);

	restart_id = 0;

	return (FALSE);
}

static gint erase_worms_cb (gpointer datap)
{
	gint data = GPOINTER_TO_INT(datap);

	if (data == 0) {
		erase_id = 0;
		if (!restart_id)
			end_game (TRUE);
	} else {
		gnibbles_undraw_worms (ERASESIZE - data);
		erase_id = g_timeout_add (ERASETIME / ERASESIZE,
				(GSourceFunc) erase_worms_cb,
				GINT_TO_POINTER(data - 1));
	}

	return (FALSE);
}

gint 
main_loop (gpointer data)
{
	gint status;
	gint tmp;

	status = gnibbles_move_worms ();
	gnibbles_scoreboard_update (scoreboard);
	network_move_worms ();

	if (status == GAMEOVER) {
		g_signal_handler_disconnect (G_OBJECT (window), keyboard_id);
		keyboard_id = 0;
		main_id = 0;
		if (add_bonus_id) {
			g_source_remove (add_bonus_id);
		}
		add_bonus_id = 0;
		erase_id = g_timeout_add (3000,
					  (GSourceFunc) erase_worms_cb,
					  (gpointer) ERASESIZE);
		gnibbles_log_score (window);
		return (FALSE);
	}

	if (status == NEWROUND) {
		g_signal_handler_disconnect (G_OBJECT (window), keyboard_id);
		keyboard_id = 0;
		if (add_bonus_id) {
			g_source_remove (add_bonus_id);
		}
		add_bonus_id = 0;
		main_id = 0;
		erase_id = g_timeout_add (ERASETIME / ERASESIZE,
					  (GSourceFunc) erase_worms_cb,
					  (gpointer) ERASESIZE);
		restart_id = g_timeout_add (1000, (GSourceFunc) restart_game,
					    NULL);
		return (FALSE);
	}

	if (boni->numleft == 0) {
		g_signal_handler_disconnect (G_OBJECT (window), keyboard_id);
		keyboard_id = 0;
		if (add_bonus_id) {
			g_source_remove (add_bonus_id);
		}
		add_bonus_id = 0;
		main_id = 0;
		if ((current_level < MAXLEVEL) && (!properties->random 
		     || is_network_running ()))
			current_level++;
		else if (properties->random && !is_network_running ()) {
			tmp = rand () % MAXLEVEL + 1;
			while (tmp == current_level)
				tmp = rand () % MAXLEVEL + 1;
			current_level = tmp;
		}
		restart_id = g_timeout_add (1000, (GSourceFunc) restart_game,
					    NULL);
		return (FALSE);
	}

	return (TRUE);
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
		gtk_action_set_sensitive (scores_action, TRUE);
		g_strfreev (names);
		g_free (scores);
		g_free (scoretimes);
	} else {
		gtk_action_set_sensitive (scores_action, FALSE);
	}
}

static gboolean
show_cursor_cb (GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
        show_cursor ();
        return FALSE;
}

static void
help_cb (GtkAction *action, gpointer data)
{
	gnome_help_display ("gnibbles.xml", NULL, NULL);
}
 
static const GtkActionEntry action_entry[] = {
	{ "GameMenu", NULL, N_("_Game") },
	{ "SettingsMenu", NULL, N_("_Settings") },
	{ "HelpMenu", NULL, N_("_Help") },
	{ "NewGame", GAMES_STOCK_NEW_GAME, NULL, NULL, NULL, G_CALLBACK (new_game_cb) },
	{ "NewNetworkGame", NULL, "New Net_work Game", NULL, NULL, G_CALLBACK (new_network_game_cb) },
	{ "Pause", GAMES_STOCK_PAUSE_GAME, NULL, NULL, NULL, G_CALLBACK (pause_game_cb) },
	{ "EndGame", GAMES_STOCK_END_GAME, NULL, NULL, NULL, G_CALLBACK (end_game_cb) },
	{ "Scores", GAMES_STOCK_SCORES, NULL, NULL, NULL, G_CALLBACK (show_scores_cb) },
	{ "Quit", GTK_STOCK_QUIT, NULL, NULL, NULL, G_CALLBACK (quit_cb) },
	{ "Preferences", GTK_STOCK_PREFERENCES, NULL, NULL, NULL, G_CALLBACK (gnibbles_preferences_cb) },
	{ "Contents", GAMES_STOCK_CONTENTS, NULL, NULL, NULL, G_CALLBACK (help_cb) },
	{ "About", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK (about_cb) }
};

static const char ui_description[] =
"<ui>"
"  <menubar name='MainMenu'>"
"    <menu action='GameMenu'>"
"      <menuitem action='NewGame'/>"
"      <menuitem action='NewNetworkGame'/>"
"      <menuitem action='EndGame'/>"
"      <separator/>"
"      <menuitem action='Pause'/>"
"      <separator/>"
"      <menuitem action='Scores'/>"
"      <separator/>"
"      <menuitem action='Quit'/>"
"    </menu>"
"    <menu action='SettingsMenu'>"
"      <menuitem action='Preferences'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='Contents'/>"
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"
"</ui>";

static void
create_menus (GtkUIManager *ui_manager)
{
        GtkActionGroup *action_group;

        action_group = gtk_action_group_new ("group");

        gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE);
        gtk_action_group_add_actions (action_group, action_entry, G_N_ELEMENTS (action_entry), window);

        gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
        gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL);

        new_network_game_action = gtk_action_group_get_action (action_group, "NewNetworkGame");
	scores_action = gtk_action_group_get_action (action_group, "Scores");
        end_game_action = gtk_action_group_get_action (action_group, "EndGame");
        pause_action = gtk_action_group_get_action (action_group, "Pause");
	preferences_action = gtk_action_group_get_action (action_group, "Preferences");
}

static void
setup_window (void)
{
	GdkPixmap *cursor_dot_pm;
	GtkWidget *vbox;
	GtkWidget *packing;
	GtkWidget *menubar;
	GtkUIManager 	*ui_manager;
	GtkAccelGroup	*accel_group;

	window = gnome_app_new ("gnibbles", "Nibbles");
	gtk_widget_realize (window);
	gtk_window_resize(GTK_WINDOW(window), properties->width, properties->height);
	g_signal_connect (G_OBJECT (window), "destroy",
			G_CALLBACK (quit_cb), NULL);
	g_signal_connect (G_OBJECT (window), "delete_event",
			G_CALLBACK (delete_cb), NULL);

	vbox = gtk_vbox_new (FALSE, 0);
	gnome_app_set_contents (GNOME_APP (window), vbox);

	games_stock_init ();
	ui_manager = gtk_ui_manager_new ();
	create_menus (ui_manager);

	accel_group = gtk_ui_manager_get_accel_group (ui_manager);
	gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

	menubar = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
	gtk_box_pack_start (GTK_BOX(vbox), menubar, FALSE, FALSE, 0);	

	packing = games_grid_frame_new (BOARDWIDTH, BOARDHEIGHT);
	gtk_box_pack_start (GTK_BOX(vbox), packing, TRUE, TRUE, 0);
	gtk_widget_show (packing);

	drawing_area = gtk_drawing_area_new ();

	cursor_dot_pm = gdk_pixmap_create_from_data(
		window->window, "\0", 1, 1, 1,
		&drawing_area->style->fg[GTK_STATE_ACTIVE],
		&drawing_area->style->bg[GTK_STATE_ACTIVE]);

	pointers.invisible = gdk_cursor_new_from_pixmap (
		cursor_dot_pm, cursor_dot_pm,
		&drawing_area->style->fg[GTK_STATE_ACTIVE],
		&drawing_area->style->bg[GTK_STATE_ACTIVE], 0, 0);

	gtk_container_add (GTK_CONTAINER (packing), drawing_area);

	g_signal_connect (G_OBJECT (drawing_area), "configure_event",
			G_CALLBACK (configure_event_cb), NULL);

	g_signal_connect (G_OBJECT (window), "configure_event",
			G_CALLBACK (window_configure_event_cb), NULL);

	g_signal_connect (G_OBJECT(drawing_area), "motion_notify_event",
			G_CALLBACK (show_cursor_cb), NULL);

	g_signal_connect (G_OBJECT(window), "focus_out_event",
			G_CALLBACK (show_cursor_cb), NULL);

	gtk_widget_set_size_request (GTK_WIDGET (drawing_area),
				     BOARDWIDTH*5,
				     BOARDHEIGHT*5);
	g_signal_connect (G_OBJECT (drawing_area), "expose_event",
			G_CALLBACK (expose_event_cb), NULL);
	gtk_widget_set_events (drawing_area, GDK_BUTTON_PRESS_MASK |
			GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK);
	gtk_widget_show (drawing_area);

	appbar = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_USER);
	gnome_app_set_statusbar (GNOME_APP (window), appbar);

	scoreboard = gnibbles_scoreboard_new (appbar);
}

static void
gconf_key_change_cb (GConfClient *client, guint cnxn_id,
		     GConfEntry *entry, gpointer user_data)
{
	gnibbles_properties_update (properties);
}

static void
load_properties ()
{
	GConfClient * client;

	properties = gnibbles_properties_new ();
	client = gconf_client_get_default ();

	/* maybe this should to into properties.c */
        gconf_client_add_dir (client,
                              KEY_DIR,
                              GCONF_CLIENT_PRELOAD_RECURSIVE,
                              NULL);
	gconf_client_notify_add (client,
				 KEY_PREFERENCES_DIR,
				 gconf_key_change_cb, NULL, NULL, NULL);
}

static void
render_logo (void)
{
	zero_board ();

	gdk_draw_pixbuf (GDK_DRAWABLE (buffer_pixmap),
			   drawing_area->style->fg_gc[GTK_WIDGET_STATE (drawing_area)],
			   logo_pixmap,
			   0, 0, 0, 0,
			   BOARDWIDTH * properties->tilesize,
			   BOARDHEIGHT * properties->tilesize,
			   GDK_RGB_DITHER_NORMAL, 0, 0);

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
	gtk_window_set_default_icon_name ("gnome-nibbles");
	srand (time (NULL));

	load_properties ();

	setup_window ();

	update_score_state ();

	gnibbles_load_logo (window);
	gnibbles_load_pixmap (window);

	gtk_widget_show (window);

	buffer_pixmap = gdk_pixmap_new (drawing_area->window,
					BOARDWIDTH * properties->tilesize,
					BOARDHEIGHT * properties->tilesize,
					-1);

	render_logo ();

	gtk_action_set_sensitive (pause_action, FALSE);
	gtk_action_set_sensitive (end_game_action, FALSE);

	gtk_main ();

	gnome_accelerators_sync();

	return 0;
}

static void
new_network_game_cb (GtkAction *action, gpointer data)
{
  gtk_action_set_sensitive (preferences_action, FALSE);
  network_new (window);
  gtk_action_set_sensitive (preferences_action, TRUE);
}

void 
set_numworms (int num)
{
  properties->numworms = num;
}

