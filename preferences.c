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

#include <gdk/gdkkeysyms.h>

#include "preferences.h"
#include "../gnobots2/keylabels.h"
#include "main.h"

extern GtkWidget *drawing_area;

static GtkWidget *pref_dialog = NULL;

static GnibblesProperties *t_properties;

static gint unpause = 0;

extern GtkWidget *window;

extern GnibblesProperties *properties;

extern gint paused;

static gchar *keyboard_string (gint ksym)
{
	gint i;

	for (i = 0; i < KB_MAP_SIZE; i++)
		if (ksym == kb_map[i].ksym)
			return kb_map[i].str;

	return "UNK";
}

static void destroy_cb (GtkWidget *widget, gpointer data)
{
	gnibbles_properties_destroy (t_properties);

	if (unpause) {
		pause_game_cb (NULL, 0);
		unpause = 0;
	}
}

static void apply_cb (GtkWidget *widget, gint pagenum, gpointer data)
{
	if (pagenum == -1) {
    if (t_properties->tilesize != properties->tilesize) {
      gtk_drawing_area_size (GTK_DRAWING_AREA (drawing_area),
                             t_properties->tilesize * BOARDWIDTH,
                             t_properties->tilesize * BOARDHEIGHT);
    }
    
		gnibbles_properties_destroy (properties);
		properties = gnibbles_properties_copy (t_properties);

		gnibbles_properties_save (properties);
	}
}

static void game_speed_cb (GtkWidget *widget, gpointer data)
{
	if (!pref_dialog)
		return;

	if (GTK_TOGGLE_BUTTON (widget)->active) {
		t_properties->gamespeed = (gint) data;
		gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
	}
}

static void start_level_cb (GtkWidget *widget, gpointer data)
{
	if (!pref_dialog)
		return;

	t_properties->startlevel = gtk_spin_button_get_value_as_int
		(GTK_SPIN_BUTTON (data));

	gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
}

static void random_order_cb (GtkWidget *widget, gpointer data)
{
	GList *list;
	int i;
	
	if (!pref_dialog)
		return;

	list = gtk_container_children (GTK_CONTAINER (widget->parent));
	list = g_list_reverse (list);

	if (GTK_TOGGLE_BUTTON (widget)->active) {
		t_properties->random = 1;
		gtk_widget_set_sensitive ((GtkWidget *) list->data, FALSE);
		list = list->next;
		gtk_widget_set_sensitive ((GtkWidget *) list->data, FALSE);
	} else {
		t_properties->random = 0;
		gtk_widget_set_sensitive ((GtkWidget *) list->data, TRUE);
		list = list->next;
		gtk_widget_set_sensitive ((GtkWidget *) list->data, TRUE);
	}

	g_list_free (list);

	gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
}

static void fake_bonus_cb (GtkWidget *widget, gpointer data)
{
	if (!pref_dialog)
		return;

	if (GTK_TOGGLE_BUTTON (widget)->active)
		t_properties->fakes = 1;
	else
		t_properties->fakes = 0;

	gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
}

static void sound_cb (GtkWidget *widget, gpointer data)
{
	if (!pref_dialog)
		return;

	if (GTK_TOGGLE_BUTTON (widget)->active)
		t_properties->sound = 1;
	else
		t_properties->sound = 0;

	gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
}

static void tile_size_cb (GtkWidget *widget, gpointer data)
{
  if (!pref_dialog)
    return;

  if (GTK_TOGGLE_BUTTON (widget)->active) {
    t_properties->tilesize = (gint) data;
	  gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
  }
}

static void num_worms_cb (GtkWidget *widget, gpointer data)
{
	if (!pref_dialog)
		return;

	t_properties->numworms = gtk_spin_button_get_value_as_int
		(GTK_SPIN_BUTTON (data));

	gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
}

static void worm_up_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	if (!pref_dialog)
		return;
	
	gtk_entry_set_text (GTK_ENTRY (widget), keyboard_string
			(event->keyval));

	t_properties->wormprops[(gint) data]->up = event->keyval;

	gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
}

static void worm_down_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	if (!pref_dialog)
		return;
	
	gtk_entry_set_text (GTK_ENTRY (widget), keyboard_string
			(event->keyval));

	t_properties->wormprops[(gint) data]->down = event->keyval;

	gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
}

static void worm_left_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	if (!pref_dialog)
		return;
	
	gtk_entry_set_text (GTK_ENTRY (widget), keyboard_string
			(event->keyval));

	t_properties->wormprops[(gint) data]->left = event->keyval;

	gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
}

static void worm_right_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	if (!pref_dialog)
		return;
	
	gtk_entry_set_text (GTK_ENTRY (widget), keyboard_string
			(event->keyval));

	t_properties->wormprops[(gint) data]->right = event->keyval;

	gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
}

static void set_worm_color_cb (GtkWidget *widget, gpointer data)
{
	gint color = ((gint) data) >> 2;
	gint worm = ((gint) data) & 3;

	t_properties->wormprops[worm]->color = color;

	gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
}

static void worm_relative_movement_cb (GtkWidget *widget, gpointer data)
{
	GList *list;
	int i;
	
	if (!pref_dialog)
		return;

	list = gtk_container_children (GTK_CONTAINER (widget->parent));
	list = g_list_reverse (list);
	
	if (GTK_TOGGLE_BUTTON (widget)->active) {
		t_properties->wormprops[(gint) data]->relmove = 1;
		for (i = 0; i < 4; i++) {
			gtk_widget_set_sensitive (list->data, FALSE);
			list = list->next;
		}
	} else {
		t_properties->wormprops[(gint) data]->relmove = 0;
		for (i = 0; i < 4; i++) {
			gtk_widget_set_sensitive (list->data, TRUE);
			list = list->next;
		}
	}

	g_list_free (list);

	gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
}

void gnibbles_preferences_cb (GtkWidget *widget, gpointer data)
{
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *frame;
	GtkWidget *button;
	GtkWidget *levelspinner;
	GtkWidget *vbox;
	GtkObject *adjustment;
	GtkWidget *hbox2;
	GtkWidget *label2;
	GtkWidget *table;
	GtkWidget *entry;
	GtkWidget *omenu;
	GtkWidget *menuitem;
	GtkWidget *menu;
	gchar buffer[256];
	gint i, j;
	GList *list;
	gint running = 0;
	
	if (pref_dialog)
		return;

	if (!paused) {
		unpause = 1;
		pause_game_cb (NULL, 0);
	}

	if (game_running ())
		running = 1;

	t_properties = gnibbles_properties_copy (properties);

	pref_dialog = gnome_property_box_new ();
	gnome_dialog_set_parent (GNOME_DIALOG (pref_dialog), GTK_WINDOW
			(window));
	gtk_window_set_title (GTK_WINDOW (pref_dialog),
			_("Gnibbles Preferences"));
	gtk_signal_connect (GTK_OBJECT (pref_dialog), "destroy",
			GTK_SIGNAL_FUNC (gtk_widget_destroyed), &pref_dialog);
	gtk_signal_connect (GTK_OBJECT (pref_dialog), "destroy",
			GTK_SIGNAL_FUNC (destroy_cb), NULL);

	label = gtk_label_new (_("Game"));
	gtk_widget_show (label);

	hbox = gtk_hbox_new (FALSE, GNOME_PAD);
	gtk_container_border_width (GTK_CONTAINER (hbox), GNOME_PAD);
	gtk_widget_show (hbox);

	frame = gtk_frame_new (_("Speed"));
	gtk_container_border_width (GTK_CONTAINER (frame), 0);
	if (running)
		gtk_widget_set_sensitive (frame, FALSE);
	gtk_widget_show (frame);

	vbox = gtk_vbox_new (TRUE, 0);
	gtk_container_border_width (GTK_CONTAINER (vbox), GNOME_PAD);
	gtk_widget_show (vbox);

	button = gtk_radio_button_new_with_label (NULL, _("Nibbles newbie"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->gamespeed == 4)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(game_speed_cb), (gpointer) 4);

	button = gtk_radio_button_new_with_label (gtk_radio_button_group
			(GTK_RADIO_BUTTON (button)), _("My second day"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->gamespeed == 3)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(game_speed_cb), (gpointer) 3);

	button = gtk_radio_button_new_with_label (gtk_radio_button_group
			(GTK_RADIO_BUTTON (button)), _("Not too shabby"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->gamespeed == 2)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(game_speed_cb), (gpointer) 2);

	button = gtk_radio_button_new_with_label (gtk_radio_button_group
			(GTK_RADIO_BUTTON (button)),
			_("Finger-twitching good"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->gamespeed == 1)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(game_speed_cb), (gpointer) 1);

	gtk_container_add (GTK_CONTAINER (frame), vbox);

	gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);

	table = gtk_table_new (5, 2, FALSE);
	gtk_widget_show (table);
	gtk_table_set_row_spacings (GTK_TABLE (table), GNOME_PAD);

	label2 = gtk_label_new (_("Starting level: "));
	gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
	gtk_widget_show (label2);
	if (properties->random)
		gtk_widget_set_sensitive (GTK_WIDGET (label2), FALSE);
	if (running)
		gtk_widget_set_sensitive (GTK_WIDGET (label2), FALSE);
	gtk_table_attach (GTK_TABLE (table), label2, 0, 1, 3, 4, GTK_EXPAND |
			GTK_FILL, 0, 0, 0);

	adjustment = gtk_adjustment_new ((gfloat) properties->startlevel, 1.0,
			MAXLEVEL, 1.0, 5.0, 0.0);

	levelspinner = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 0, 0);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (levelspinner), FALSE);
	gtk_widget_show (levelspinner);
	if (properties->random)
		gtk_widget_set_sensitive (GTK_WIDGET (levelspinner), FALSE);
	if (running)
		gtk_widget_set_sensitive (GTK_WIDGET (levelspinner), FALSE);
	gtk_table_attach (GTK_TABLE (table), levelspinner, 1, 2, 3, 4,
			GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
			GTK_SIGNAL_FUNC (start_level_cb), levelspinner);

	button = gtk_check_button_new_with_label (_("Levels in random order"));
	gtk_widget_show (button);
	gtk_table_attach (GTK_TABLE (table), button, 0, 2, 0, 1,
			GTK_EXPAND | GTK_FILL, 0, 0, 0);
	if (running)
		gtk_widget_set_sensitive (button, FALSE);
	if (properties->random)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(random_order_cb), NULL);

	button = gtk_check_button_new_with_label (_("Fake bonuses"));
	gtk_widget_show (button);
	gtk_table_attach (GTK_TABLE (table), button, 0, 2, 1, 2,
			GTK_EXPAND | GTK_FILL, 0, 0, 0);
	if (running)
		gtk_widget_set_sensitive (button, FALSE);
	if (properties->fakes)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(fake_bonus_cb), NULL);

	button = gtk_check_button_new_with_label (_("Sounds"));
	if (properties->sound)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);
	gtk_widget_show (button);
	gtk_table_attach (GTK_TABLE (table), button, 0, 2, 2, 3,
			GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(sound_cb), NULL);

	label2 = gtk_label_new (_("Number of players: "));
	gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
	gtk_widget_show (label2);
	gtk_table_attach (GTK_TABLE (table), label2, 0, 1, 4, 5, GTK_EXPAND |
			GTK_FILL, 0, 0, 0);
	if (running)
		gtk_widget_set_sensitive (label2, FALSE);

	adjustment = gtk_adjustment_new ((gfloat) properties->numworms, 1.0,
			NUMWORMS, 1.0, 1.0, 0.0);

	button = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 0, 0);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (button), FALSE);
	gtk_widget_show (button);
	gtk_table_attach (GTK_TABLE (table), button, 1, 2, 4, 5, GTK_EXPAND |
			GTK_FILL, 0, 0, 0);
	if (running)
		gtk_widget_set_sensitive (button, FALSE);
	gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
			GTK_SIGNAL_FUNC (num_worms_cb), button);

	gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);

	gnome_property_box_append_page (GNOME_PROPERTY_BOX (pref_dialog),
			hbox, label);

  label = gtk_label_new (_("Graphics"));
  gtk_widget_show (label);

	hbox = gtk_hbox_new (FALSE, GNOME_PAD);
	gtk_container_border_width (GTK_CONTAINER (hbox), GNOME_PAD);
	gtk_widget_show (hbox);

	frame = gtk_frame_new (_("Board size"));
	gtk_container_border_width (GTK_CONTAINER (frame), 0);
	gtk_widget_show (frame);

	vbox = gtk_vbox_new (TRUE, 0);
	gtk_container_border_width (GTK_CONTAINER (vbox), GNOME_PAD);
	gtk_widget_show (vbox);

	button = gtk_radio_button_new_with_label (NULL, _("Tiny (184x132)"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->tilesize == 2)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(tile_size_cb), (gpointer) 2);

	button = gtk_radio_button_new_with_label (gtk_radio_button_group (button),
                                            _("Small (368x264)"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->tilesize == 4)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(tile_size_cb), (gpointer) 4);

	button = gtk_radio_button_new_with_label (gtk_radio_button_group (button),
                                            _("Medium (460x330)"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->tilesize == 5)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(tile_size_cb), (gpointer) 5);

	button = gtk_radio_button_new_with_label (gtk_radio_button_group (button),
                                            _("Large (736x528)"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->tilesize == 8)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(tile_size_cb), (gpointer) 8);

	button = gtk_radio_button_new_with_label (gtk_radio_button_group (button),
                                            _("Extra large (920x660)"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->tilesize == 10)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(tile_size_cb), (gpointer) 10);

	button = gtk_radio_button_new_with_label (gtk_radio_button_group (button),
                                            _("Huge (1840x1320)"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->tilesize == 20)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(tile_size_cb), (gpointer) 20);

	gtk_container_add (GTK_CONTAINER (frame), vbox);

	gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);

	gnome_property_box_append_page (GNOME_PROPERTY_BOX (pref_dialog),
			hbox, label);

	for (i = 0; i < NUMWORMS; i++) {
		sprintf (buffer, _("Worm %d"), i + 1);
		label = gtk_label_new (buffer);
		gtk_widget_show (label);

		table = gtk_table_new (3, 4, FALSE);
		gtk_container_border_width (GTK_CONTAINER (table),
				GNOME_PAD);
		gtk_table_set_col_spacings (GTK_TABLE (table),
				GNOME_PAD);
		gtk_table_set_row_spacings (GTK_TABLE (table),
				GNOME_PAD);
		gtk_widget_show (table);

		label2 = gtk_label_new (_("Up:"));
		gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
		gtk_widget_show (label2);
		gtk_table_attach (GTK_TABLE (table), label2, 0, 1, 0, 1,
				GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
				0, 0);

		entry = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (entry), keyboard_string
				(properties->wormprops[i]->up));
		gtk_entry_set_editable (GTK_ENTRY (entry), FALSE);
		gtk_widget_show (entry);
		gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1,
				GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
				0, 0);
		gtk_signal_connect (GTK_OBJECT (entry), "key_press_event",
				GTK_SIGNAL_FUNC (worm_up_cb), (gpointer) i);

		label2 = gtk_label_new (_("Down:"));
		gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
		gtk_widget_show (label2);
		gtk_table_attach (GTK_TABLE (table), label2, 2, 3, 0, 1,
				GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
				0, 0);

		entry = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (entry), keyboard_string
				(properties->wormprops[i]->down));
		gtk_entry_set_editable (GTK_ENTRY (entry), FALSE);
		gtk_widget_show (entry);
		gtk_table_attach (GTK_TABLE (table), entry, 3, 4, 0, 1,
				GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
				0, 0);
		gtk_signal_connect (GTK_OBJECT (entry), "key_press_event",
				GTK_SIGNAL_FUNC (worm_down_cb), (gpointer) i);

		label2 = gtk_label_new (_("Left:"));
		gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
		gtk_widget_show (label2);
		gtk_table_attach (GTK_TABLE (table), label2, 0, 1, 1, 2,
				GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
				0, 0);

		entry = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (entry), keyboard_string
				(properties->wormprops[i]->left));
		gtk_entry_set_editable (GTK_ENTRY (entry), FALSE);
		gtk_widget_show (entry);
		gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2,
				GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
				0, 0);
		gtk_signal_connect (GTK_OBJECT (entry), "key_press_event",
				GTK_SIGNAL_FUNC (worm_left_cb), (gpointer) i);

		label2 = gtk_label_new (_("Right:"));
		gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
		gtk_widget_show (label2);
		gtk_table_attach (GTK_TABLE (table), label2, 2, 3, 1, 2,
				GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
				0, 0);

		entry = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (entry), keyboard_string
				(properties->wormprops[i]->right));
		gtk_entry_set_editable (GTK_ENTRY (entry), FALSE);
		gtk_widget_show (entry);
		gtk_table_attach (GTK_TABLE (table), entry, 3, 4, 1, 2,
				GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
				0, 0);
		gtk_signal_connect (GTK_OBJECT (entry), "key_press_event",
				GTK_SIGNAL_FUNC (worm_right_cb), (gpointer) i);

		label2 = gtk_label_new (_("Color:"));
		gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
		gtk_widget_show (label2);
		gtk_table_attach (GTK_TABLE (table), label2, 0, 1, 2, 3,
				GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
				0, 0);

		omenu = gtk_option_menu_new ();
		menu = gtk_menu_new ();
		menuitem = gtk_menu_item_new_with_label (_("Red"));
		gtk_widget_show (menuitem);
		gtk_menu_append (GTK_MENU (menu), menuitem);
		gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				GTK_SIGNAL_FUNC (set_worm_color_cb),
				(gpointer) (WORMRED << 2 | i));
		menuitem = gtk_menu_item_new_with_label (_("Green"));
		gtk_widget_show (menuitem);
		gtk_menu_append (GTK_MENU (menu), menuitem);
		gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				GTK_SIGNAL_FUNC (set_worm_color_cb),
				(gpointer) (WORMGREEN << 2 | i));
		menuitem = gtk_menu_item_new_with_label (_("Blue"));
		gtk_widget_show (menuitem);
		gtk_menu_append (GTK_MENU (menu), menuitem);
		gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				GTK_SIGNAL_FUNC (set_worm_color_cb),
				(gpointer) (WORMBLUE << 2 | i));
		menuitem = gtk_menu_item_new_with_label (_("Yellow"));
		gtk_widget_show (menuitem);
		gtk_menu_append (GTK_MENU (menu), menuitem);
		gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				GTK_SIGNAL_FUNC (set_worm_color_cb),
				(gpointer) (WORMYELLOW << 2 | i));
		menuitem = gtk_menu_item_new_with_label (_("Cyan"));
		gtk_widget_show (menuitem);
		gtk_menu_append (GTK_MENU (menu), menuitem);
		gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				GTK_SIGNAL_FUNC (set_worm_color_cb),
				(gpointer) (WORMCYAN << 2 | i));
		menuitem = gtk_menu_item_new_with_label (_("Purple"));
		gtk_widget_show (menuitem);
		gtk_menu_append (GTK_MENU (menu), menuitem);
		gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				GTK_SIGNAL_FUNC (set_worm_color_cb),
				(gpointer) (WORMPURPLE << 2 | i));
		menuitem = gtk_menu_item_new_with_label (_("Gray"));
		gtk_widget_show (menuitem);
		gtk_menu_append (GTK_MENU (menu), menuitem);
		gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				GTK_SIGNAL_FUNC (set_worm_color_cb),
				(gpointer) (WORMGRAY << 2 | i));

		gtk_menu_set_active (GTK_MENU (menu),
				properties->wormprops[i]->color - WORMRED);
		
		gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
		
		gtk_widget_show(omenu);
		gtk_table_attach (GTK_TABLE (table), omenu, 1, 2, 2, 3,
				GTK_EXPAND | GTK_FILL, 0, 0, 0);

		button = gtk_check_button_new_with_label
			(_("Relative movement"));
		if (properties->wormprops[i]->relmove) {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
					(button), TRUE);
			list = gtk_container_children (GTK_CONTAINER (table));
			list = g_list_reverse (list);
			for (j = 0; j < 4; j++) {
				gtk_widget_set_sensitive (GTK_WIDGET
						(list->data), FALSE);
				list = list->next;
			}
			g_list_free (list);
		}
		gtk_signal_connect (GTK_OBJECT (button), "toggled",
				GTK_SIGNAL_FUNC (worm_relative_movement_cb),
				(gpointer) i);

		gtk_widget_show (button);
		gtk_table_attach (GTK_TABLE (table), button, 2, 4, 2, 3,
				GTK_EXPAND | GTK_FILL, 0, 0, 0);

		gnome_property_box_append_page (GNOME_PROPERTY_BOX
				(pref_dialog), table, label);
	}

	gtk_signal_connect (GTK_OBJECT (pref_dialog), "apply", GTK_SIGNAL_FUNC
			(apply_cb), NULL);

	gtk_widget_show (pref_dialog);
}
