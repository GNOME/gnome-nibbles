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
#include <libgnomeui/gnome-window-icon.h>
#include <gdk/gdkkeysyms.h>
#include <time.h>

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
static void render_logo ();
static gint new_game_cb (GtkWidget *widget, gpointer data);
gint pause_game_cb (GtkWidget *widget, gpointer data);
static gint end_game_cb (GtkWidget *widget, gpointer data);
static void quit_cb (GtkWidget *widget, gpointer data);
static void about_cb (GtkWidget *widget, gpointer data);
static gint show_scores_cb (GtkWidget *widget, gpointer data);

static GnomeUIInfo game_menu[] = {
	GNOMEUIINFO_MENU_NEW_GAME_ITEM (new_game_cb, NULL),
	GNOMEUIINFO_MENU_PAUSE_GAME_ITEM (pause_game_cb, NULL),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_SCORES_ITEM (show_scores_cb, NULL),
	GNOMEUIINFO_MENU_END_GAME_ITEM (end_game_cb, (gpointer) 2),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_EXIT_ITEM (quit_cb, NULL),
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

void hide_cursor ()
{
	if (pointers.current != pointers.invisible) {
		gdk_window_set_cursor (drawing_area->window, pointers.invisible);
		pointers.current = pointers.invisible;
	}
}

void show_cursor ()
{
	if (pointers.current != NULL) {
		gdk_window_set_cursor (drawing_area->window, NULL);
		pointers.current = NULL;
	}
}

gint game_running ()
{
	return (main_id || erase_id || dummy_id || restart_id || paused);
}

static void zero_board ()
{
	int i, j;

	for (i = 0; i < BOARDWIDTH; i++)
		for (j = 0; j < BOARDHEIGHT; j++) {
			board[i][j] = EMPTYCHAR;
			gnibbles_draw_pixmap_buffer (0, i, j);
		}
}

static gint end_game_box ()
{
	gint pause_state;
	static GtkWidget *box;
	gint status;

	if (box)
		return 0;

	pause_state = paused;
	if (!paused)
		pause_game_cb (NULL, (gpointer) 0);
	box = gnome_message_box_new (
			_("Do you really want to end this game?"),
			GNOME_MESSAGE_BOX_QUESTION,
			GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO,
			NULL);
	gnome_dialog_set_parent (GNOME_DIALOG (box), GTK_WINDOW
			(window));
	gnome_dialog_set_default (GNOME_DIALOG (box), 0);
	status = gnome_dialog_run (GNOME_DIALOG (box));
	box = NULL;
	if (!pause_state && status)
		pause_game_cb (NULL, (gpointer) 0);
	return (status);
}

static void quit_cb (GtkWidget *widget, gpointer data)
{
	if (game_running () && end_game_box ())
		return;

	gnibbles_destroy ();
	gtk_main_quit ();
}

static void about_cb (GtkWidget *widget, gpointer data)
{
	static GtkWidget *about;

	const gchar *authors[] = {"Sean MacIsaac", "Ian Peters", NULL};

	if (about != NULL) {
		gdk_window_raise (about->window);
		gdk_window_show (about->window);
		return;
	}

	about = gnome_about_new (_("Gnibbles"), VERSION,
			"(C) 1999 Sean MacIsaac and Ian Peters",
			(const char **)authors,
			_("Send comments and bug reports to: "
			  "sjm@acm.org, itp@gnu.org"),
			NULL);
	gtk_signal_connect (GTK_OBJECT (about), "destroy", GTK_SIGNAL_FUNC
			(gtk_widget_destroyed), &about);
	gnome_dialog_set_parent (GNOME_DIALOG (about), GTK_WINDOW (window));

	gtk_widget_show (about);
}

static gint expose_event_cb (GtkWidget *widget, GdkEventExpose *event)
{
	gdk_draw_pixmap (widget->window, widget->style->fg_gc[GTK_WIDGET_STATE
			(widget)], buffer_pixmap, event->area.x, event->area.y,
			event->area.x, event->area.y, event->area.width,
			event->area.height);

	return (FALSE);
}

static gint key_press_cb (GtkWidget *widget, GdkEventKey *event)
{
	gnibbles_keypress_worms (event->keyval);
	hide_cursor ();
}

static void draw_board ()
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

	gdk_draw_pixmap (drawing_area->window,
			 drawing_area->style->fg_gc[GTK_WIDGET_STATE
			 (drawing_area)], buffer_pixmap, 0, 0, 0, 0,
			 BOARDWIDTH*properties->tilesize, BOARDHEIGHT*properties->tilesize);
	
}

static gint configure_event_cb (GtkWidget *widget, GdkEventConfigure *event)
{
	int i, j;

	gnibbles_load_pixmap();
	gdk_imlib_free_pixmap (buffer_pixmap);
	buffer_pixmap = gdk_pixmap_new (drawing_area->window,
					BOARDWIDTH * properties->tilesize,
					BOARDHEIGHT * properties->tilesize, -1);

	if (game_running ())
		draw_board ();
	else
		render_logo ();

	return (FALSE);
}

static gint new_game_2_cb (GtkWidget *widget, gpointer data)
{
	if (!paused) {
		if (!keyboard_id)
			keyboard_id = gtk_signal_connect (GTK_OBJECT (window),
					"key_press_event",
					GTK_SIGNAL_FUNC (key_press_cb), NULL);
		if (!main_id)
			main_id = gtk_timeout_add (GAMEDELAY * properties->gamespeed,
					(GtkFunction) main_loop, NULL);
		if (!add_bonus_id)
			add_bonus_id = gtk_timeout_add (BONUSDELAY *
					properties->gamespeed,
					(GtkFunction) add_bonus_cb,
					NULL);
	}

	dummy_id = 0;

	return (FALSE);
}

static gint new_game_cb (GtkWidget *widget, gpointer data)
{
	gtk_widget_set_sensitive (game_menu[1].widget, TRUE);
	gtk_widget_set_sensitive (game_menu[4].widget, TRUE);
	/*
	gtk_widget_set_sensitive (settings_menu[0].widget, FALSE);
	*/

	if (game_running ()) {
		if (!end_game_box ()) {
			end_game_cb (widget, (gpointer) 0);
			main_id = 0;
		} else
			return 0;
	}

	gnibbles_init ();

	if (!properties->random)
		current_level = properties->startlevel;
	else
		current_level = rand () % MAXLEVEL + 1;
	
	gnibbles_load_level (current_level);

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
				gtk_signal_disconnect (GTK_OBJECT (window),
						keyboard_id);
				keyboard_id = 0;
			}
			if (add_bonus_id) {
				gtk_timeout_remove (add_bonus_id);
				add_bonus_id = 0;
			}
		}
	}
}

static gint show_scores_cb (GtkWidget *widget, gpointer data)
{
	gnibbles_show_scores (0);
}

static gint end_game_cb (GtkWidget *widget, gpointer data)
{
	if ((gint) data == 2 && end_game_box ())
			return 0;

	if (main_id) {
		gtk_timeout_remove (main_id);
		main_id = 0;
	}

	if (keyboard_id) {
		gtk_signal_disconnect (GTK_OBJECT (window), keyboard_id);
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

	return (FALSE);
}

static gint add_bonus_cb (gpointer data)
{
	gnibbles_add_bonus (0);

	return (TRUE);
}

static gint restart_game (gpointer data)
{
	gnibbles_load_level (current_level);

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
		gtk_signal_disconnect (GTK_OBJECT (window), keyboard_id);
		keyboard_id = 0;
		main_id = 0;
		gtk_timeout_remove (add_bonus_id);
		add_bonus_id = 0;
		erase_id = gtk_timeout_add (3000,
				(GtkFunction) erase_worms_cb,
				(gpointer) ERASESIZE);
		gnibbles_log_score ();
		return (FALSE);
	}

	if (status == NEWROUND) {
		gtk_signal_disconnect (GTK_OBJECT (window), keyboard_id);
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
		gtk_signal_disconnect (GTK_OBJECT (window), keyboard_id);
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

static void set_bg_color ()
{
	GdkImage *tmp_image;
	GdkColor bgcolor;

	tmp_image = gdk_image_get (gnibbles_pixmap, 0, 0, 1, 1);
	bgcolor.pixel = gdk_image_get_pixel (tmp_image, 0, 0);
	gdk_window_set_background (drawing_area->window, &bgcolor);
	gdk_image_destroy (tmp_image);
}

gboolean
show_cursor_cb (GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
        show_cursor ();
        return FALSE;
}

static void setup_window ()
{
	GtkWidget *label, *hbox;
	GdkPixmap *cursor_dot_pm;

	window = gnome_app_new ("gnibbles", "GNOME Nibbles");
	gtk_window_set_policy (GTK_WINDOW (window), FALSE, FALSE, TRUE);
	gtk_widget_realize (window);
	gtk_signal_connect (GTK_OBJECT (window), "delete_event",
			GTK_SIGNAL_FUNC (quit_cb), NULL);

	gtk_widget_push_visual (gdk_imlib_get_visual ());
	gtk_widget_push_colormap (gdk_imlib_get_colormap ());

	drawing_area = gtk_drawing_area_new ();

	cursor_dot_pm = gdk_pixmap_create_from_data(
		window->window, "\0", 1, 1, 1,
		&drawing_area->style->fg[GTK_STATE_ACTIVE],
		&drawing_area->style->bg[GTK_STATE_ACTIVE]);

	pointers.invisible = gdk_cursor_new_from_pixmap (
		cursor_dot_pm, cursor_dot_pm,
		&drawing_area->style->fg[GTK_STATE_ACTIVE],
		&drawing_area->style->bg[GTK_STATE_ACTIVE], 0, 0);

	gtk_widget_pop_colormap ();
	gtk_widget_pop_visual ();

	gnome_app_set_contents (GNOME_APP (window), drawing_area);

	gtk_signal_connect (GTK_OBJECT (drawing_area), "configure_event",
			GTK_SIGNAL_FUNC (configure_event_cb), NULL);

	gtk_signal_connect (GTK_OBJECT(drawing_area), "motion_notify_event",
			GTK_SIGNAL_FUNC (show_cursor_cb), NULL);

	gtk_signal_connect (GTK_OBJECT(window), "focus_out_event",
			GTK_SIGNAL_FUNC (show_cursor_cb), NULL);

	gtk_drawing_area_size (GTK_DRAWING_AREA (drawing_area),
			BOARDWIDTH*properties->tilesize, BOARDHEIGHT*properties->tilesize);
	gtk_signal_connect (GTK_OBJECT (drawing_area), "expose_event",
			GTK_SIGNAL_FUNC (expose_event_cb), NULL);
	gtk_widget_set_events (drawing_area, GDK_BUTTON_PRESS_MASK |
			GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK);
	gtk_widget_show (drawing_area);

	gnome_app_create_menus (GNOME_APP (window), main_menu);

	appbar = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_USER);
	gnome_app_set_statusbar (GNOME_APP (window), appbar);
	gnome_app_install_menu_hints (GNOME_APP (window), main_menu);

	scoreboard = gnibbles_scoreboard_new (appbar);
}

static void load_properties ()
{
	properties = gnibbles_properties_new ();
}

static void render_logo ()
{
	gint i, j;

	zero_board ();

	gdk_draw_pixmap (buffer_pixmap,
			drawing_area->style->fg_gc[GTK_WIDGET_STATE (drawing_area)], logo_pixmap,
      0, 0, 0, 0, BOARDWIDTH*properties->tilesize,
      BOARDHEIGHT*properties->tilesize);

	gdk_draw_pixmap (drawing_area->window,
			drawing_area->style->fg_gc[GTK_WIDGET_STATE (drawing_area)],
      buffer_pixmap, 0, 0, 0, 0, BOARDWIDTH*properties->tilesize,
      BOARDHEIGHT*properties->tilesize);
}

int main (int argc, char **argv)
{
	gint foo;

	gnome_score_init ("gnibbles");

	bindtextdomain(PACKAGE, GNOMELOCALEDIR);
	textdomain(PACKAGE);
	
	gnome_init ("Gnibbles", VERSION, argc, argv);
	gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/gnome-nibbles.png");
	srand (time (NULL));

	load_properties ();

	setup_window ();

	gnibbles_load_pixmap ();

	gtk_widget_show (window);

	buffer_pixmap = gdk_pixmap_new (drawing_area->window,
			BOARDWIDTH * properties->tilesize, BOARDHEIGHT * properties->tilesize,
			-1);

	render_logo ();

	set_bg_color ();

	gtk_widget_set_sensitive (game_menu[1].widget, FALSE);
	gtk_widget_set_sensitive (game_menu[4].widget, FALSE);

	gtk_main ();

	return 0;
}
