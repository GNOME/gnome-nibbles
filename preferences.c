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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gdk/gdkkeysyms.h>
#include <games-frame.h>

#include "preferences.h"
#include "main.h"

#define KB_TEXT_WIDTH 60
#define KB_TEXT_HEIGHT 32
#define KB_TEXT_NCHARS 8

extern GtkWidget *drawing_area;
static GtkWidget *pref_dialog = NULL;
static GnibblesProperties *t_properties;
static gint unpause = 0;
extern GtkWidget *window;
extern GnibblesProperties *properties;
extern gint paused;

GtkWidget *control_table[NUMWORMS];
GtkWidget *control_button[NUMWORMS][4];
GtkWidget *start_level_label, 
	*start_level_spin_button;

static gchar *
keyboard_string (gint ksym)
{
	gchar *name;
	name = gdk_keyval_name (ksym);
	return name;
}

static void
destroy_cb (GtkWidget *widget, gpointer data)
{
	if (unpause) {
		pause_game_cb (NULL, 0);
		unpause = 0;
	}
	pref_dialog = NULL;
}

static void
apply_cb (GtkWidget *widget, gint action, gpointer data)
{
	if (t_properties->tilesize != properties->tilesize) {
		gtk_widget_set_size_request (GTK_WIDGET (drawing_area),
					     t_properties->tilesize * BOARDWIDTH,
					     t_properties->tilesize * BOARDHEIGHT);
	}
	gnibbles_properties_destroy (properties);
	properties = gnibbles_properties_copy (t_properties);
	gnibbles_properties_save (properties);
	update_score_state ();

	gtk_widget_destroy (widget);
}

static void
game_speed_cb (GtkWidget *widget, gpointer data)
{
	if (!pref_dialog)
		return;

	if (GTK_TOGGLE_BUTTON (widget)->active) {
		t_properties->gamespeed = (gint) data;
		gnibbles_properties_set_tile_size ((gint) data);
	}
}

static void
start_level_cb (GtkWidget *widget, gpointer data)
{
	if (!pref_dialog)
		return;

	end_game (1);

	t_properties->startlevel = gtk_spin_button_get_value_as_int
		(GTK_SPIN_BUTTON (data));
	gnibbles_properties_set_start_level (t_properties->startlevel);
}

static void
random_order_cb (GtkWidget *widget, gpointer data)
{
	int i;
	gboolean value;
	gchar *name;

	if (!pref_dialog)
		return;

	value = GTK_TOGGLE_BUTTON (widget)->active;
	t_properties->random = value;

	gtk_widget_set_sensitive (start_level_label, !value);
	gtk_widget_set_sensitive (start_level_spin_button, !value);

	gnibbles_properties_set_random (GTK_TOGGLE_BUTTON (widget)->active);
}

static void
fake_bonus_cb (GtkWidget *widget, gpointer data)
{
	if (!pref_dialog)
		return;

	if (GTK_TOGGLE_BUTTON (widget)->active)
		t_properties->fakes = 1;
	else
		t_properties->fakes = 0;

	gnibbles_properties_set_fakes ((gint)data);
}

static void
sound_cb (GtkWidget *widget, gpointer data)
{
	if (!pref_dialog)
		return;

	if (GTK_TOGGLE_BUTTON (widget)->active)
		t_properties->sound = 1;
	else
		t_properties->sound = 0;
	gnibbles_properties_set_sound ((gint)data);
}

static void
tile_size_cb (GtkWidget *widget, gpointer data)
{
  if (!pref_dialog)
    return;

  if (GTK_TOGGLE_BUTTON (widget)->active) {
    t_properties->tilesize = (gint) data;
    if (t_properties->tilesize != properties->tilesize) {
	    properties->tilesize = t_properties->tilesize;
	    gtk_widget_set_size_request (GTK_WIDGET (drawing_area),
					 properties->tilesize * BOARDWIDTH,
					 properties->tilesize * BOARDHEIGHT);
	    gnibbles_properties_set_tile_size ((gint)data);
    }
    end_game (0);
  }
}

static void
num_worms_cb (GtkWidget *widget, gpointer data)
{
	if (!pref_dialog)
		return;

	t_properties->numworms = gtk_spin_button_get_value_as_int
		(GTK_SPIN_BUTTON (data));

	gnibbles_properties_set_worms_number (t_properties->numworms);
}

static gint
worm_up_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	gchar *key_name;

	if (!pref_dialog)
		return;
	
	key_name = keyboard_string (event->keyval);
	gtk_entry_set_text (GTK_ENTRY (widget), key_name);
	gtk_widget_set_sensitive (widget, FALSE);
	gtk_widget_grab_focus (control_button[(gint) data][0]);
	
	t_properties->wormprops[(gint) data]->up = key_name;

	gnibbles_properties_set_worm_up ((gint)data, key_name);

	return TRUE;
}

static gint
worm_down_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	gchar *key_name;

	if (!pref_dialog)
		return;
	
	key_name = keyboard_string (event->keyval);
	gtk_entry_set_text (GTK_ENTRY (widget), key_name);
	gtk_widget_set_sensitive (widget, FALSE);
	gtk_widget_grab_focus (control_button[(gint) data][1]);

	t_properties->wormprops[(gint) data]->down = key_name;

	gnibbles_properties_set_worm_down ((gint)data, key_name);

	return TRUE;
}

static gint
worm_left_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	gchar *key_name;
	if (!pref_dialog)
		return;
	
	key_name = keyboard_string (event->keyval);
	gtk_entry_set_text (GTK_ENTRY (widget), key_name);
	gtk_widget_set_sensitive (widget, FALSE);
	gtk_widget_grab_focus (control_button[(gint) data][2]);

	t_properties->wormprops[(gint) data]->left = key_name;

	gnibbles_properties_set_worm_left ((gint)data, key_name);

	return TRUE;
}

static gint
worm_right_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	gchar *key_name;
	if (!pref_dialog)
		return;
	
	key_name = keyboard_string (event->keyval);
	gtk_entry_set_text (GTK_ENTRY (widget), key_name);
	gtk_widget_set_sensitive (widget, FALSE);
	gtk_widget_grab_focus (control_button[(gint) data][3]);

	t_properties->wormprops[(gint) data]->right = key_name;

	gnibbles_properties_set_worm_right ((gint)data, key_name);

	return TRUE;
}

static void
set_worm_color_cb (GtkWidget *widget, gpointer data)
{
	gint color = ((gint) data) >> 2;
	gint worm = ((gint) data) & 3;

	t_properties->wormprops[worm]->color = color;

	gnibbles_properties_set_worm_color (worm, color);
}

static void
set_worm_controls_sensitivity (gint i, gboolean value)
{
	const gchar *name;
	GList *list = NULL;

	list = gtk_container_get_children (GTK_CONTAINER (control_table[i]));
	list = g_list_reverse (list);

	for (; list; list = list->next) {
		name = gtk_widget_get_name (GTK_WIDGET (list->data));
		if (g_strrstr (name, "UpLabel") || g_strrstr (name, "DownLabel"))
			gtk_widget_set_sensitive (list->data, !value);
	}
	g_list_free (list);
}

static void
worm_relative_movement_cb (GtkWidget *widget, gpointer data)
{
	gint i;
	
	if (pref_dialog == NULL)
		return;
	
	i = (gint) data;
	
	set_worm_controls_sensitivity (i, GTK_TOGGLE_BUTTON (widget)->active);

	gnibbles_properties_set_worm_relative_movement 
		(i, GTK_TOGGLE_BUTTON (widget)->active);
}

static void
key_change_cb (GtkWidget * widget, GtkWidget * target)
{
	gtk_widget_set_sensitive (target, TRUE);
	gtk_widget_grab_focus (target);
}

void
gnibbles_preferences_cb (GtkWidget *widget, gpointer data)
{
	GtkWidget *notebook;
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *frame;
	GtkWidget *button;
	GtkWidget *levelspinner;
	GtkWidget *vbox, *vbox2;
	GtkObject *adjustment;
	GtkWidget *hbox2;
	GtkWidget *label2;
	GtkWidget *table, *table2;
	GtkWidget *entry;
	GtkWidget *omenu;
	GtkWidget *menuitem;
	GtkWidget *menu;
	GtkWidget *entries[NUMWORMS][4];
	gchar *buffer;
	gint i, j;
	GList *list;
	gint running = 0;
	
	if (pref_dialog) {
		gtk_window_present (GTK_WINDOW(pref_dialog));
		return;
	}

	if (!paused) {
		unpause = 1;
		pause_game_cb (NULL, 0);
	}

	if (game_running ())
		running = 1;

	t_properties = gnibbles_properties_copy (properties);

	pref_dialog = gtk_dialog_new_with_buttons (_("Gnibbles Preferences"),
                                                   GTK_WINDOW(window), 0,
                                                   GTK_STOCK_CLOSE,
                                                   GTK_RESPONSE_CLOSE, NULL);

	notebook = gtk_notebook_new ();
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (pref_dialog)->vbox),
                           notebook);

	label = gtk_label_new (_("Game"));
	table = gtk_table_new (1, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_container_set_border_width (GTK_CONTAINER (table), 12);

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                                  table, label);

	frame = games_frame_new (_("Speed"));
	if (running)
		gtk_widget_set_sensitive (frame, FALSE);

	gtk_table_attach_defaults (GTK_TABLE (table), frame, 0, 1, 0, 1);
        
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
	gtk_container_add (GTK_CONTAINER (frame), vbox);

	button = gtk_radio_button_new_with_label (NULL, _("Nibbles newbie"));

	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->gamespeed == 4)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
					      TRUE);
	g_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			  (game_speed_cb), (gpointer) 4);

	button = gtk_radio_button_new_with_label (gtk_radio_button_get_group
						  (GTK_RADIO_BUTTON (button)), _("My second day"));

	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->gamespeed == 3)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
					      TRUE);
	g_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			  (game_speed_cb), (gpointer) 3);

	button = gtk_radio_button_new_with_label (gtk_radio_button_get_group
						  (GTK_RADIO_BUTTON (button)), _("Not too shabby"));

	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->gamespeed == 2)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
					      TRUE);
	g_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			  (game_speed_cb), (gpointer) 2);

	button = gtk_radio_button_new_with_label (gtk_radio_button_get_group
						  (GTK_RADIO_BUTTON (button)),
						  _("Finger-twitching good"));

	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->gamespeed == 1)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
					      TRUE);
	g_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			  (game_speed_cb), (gpointer) 1);


        /* Options */
        frame = games_frame_new (_("Options"));
	gtk_table_attach_defaults (GTK_TABLE (table), frame, 1, 2, 0, 1);

        vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
        gtk_container_add (GTK_CONTAINER (frame), vbox);

	button = gtk_check_button_new_with_label (_("Play levels in random order"));
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

	if (running)
		gtk_widget_set_sensitive (button, FALSE);
	if (properties->random)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
					      TRUE);
	g_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
                          (random_order_cb), NULL);

	button = gtk_check_button_new_with_label (_("Enable fake bonuses"));
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

	if (running)
		gtk_widget_set_sensitive (button, FALSE);
	if (properties->fakes)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
					      TRUE);
	g_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
                          (fake_bonus_cb), NULL);
        
	button = gtk_check_button_new_with_label (_("Enable sounds"));
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->sound)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
					      TRUE);
	g_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			  (sound_cb), NULL);

	table2 = gtk_table_new (2, 2, FALSE);
	gtk_box_pack_start (GTK_BOX (vbox), table2, FALSE, FALSE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table2), 6);
	gtk_container_set_border_width (GTK_CONTAINER (table2), 0);

	label2 = gtk_label_new (_("Starting level: "));
	start_level_label = label2;
	gtk_widget_set_name (label2, "StartLevelLabel");
	gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

	if (properties->random)
		gtk_widget_set_sensitive (GTK_WIDGET (label2), FALSE);
	if (running)
		gtk_widget_set_sensitive (GTK_WIDGET (label2), FALSE);
	gtk_table_attach_defaults (GTK_TABLE (table2), label2, 0, 1, 0, 1);

	adjustment = gtk_adjustment_new ((gfloat) properties->startlevel, 1.0,
					 MAXLEVEL, 1.0, 5.0, 0.0);

	levelspinner = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 0, 0);
	start_level_spin_button = levelspinner;
	gtk_widget_set_name (levelspinner, "StartLevelSpinButton");
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (levelspinner), FALSE);

	if (properties->random)
		gtk_widget_set_sensitive (GTK_WIDGET (levelspinner), FALSE);
	if (running)
		gtk_widget_set_sensitive (GTK_WIDGET (levelspinner), FALSE);
	gtk_table_attach_defaults (GTK_TABLE (table2), levelspinner, 1, 2, 0, 1);
	g_signal_connect (GTK_OBJECT (adjustment), "value_changed",
			  GTK_SIGNAL_FUNC (start_level_cb), levelspinner);

	label2 = gtk_label_new (_("Number of players: "));
	gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

	gtk_table_attach_defaults (GTK_TABLE (table2), label2, 0, 1, 1, 2);
	if (running)
		gtk_widget_set_sensitive (label2, FALSE);

	adjustment = gtk_adjustment_new ((gfloat) properties->numworms, 1.0,
                                         NUMWORMS, 1.0, 1.0, 0.0);

	button = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 0, 0);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (button), FALSE);

	gtk_table_attach_defaults (GTK_TABLE (table2), button, 1, 2, 1, 2);
	if (running)
		gtk_widget_set_sensitive (button, FALSE);
	g_signal_connect (GTK_OBJECT (adjustment), "value_changed",
                          GTK_SIGNAL_FUNC (num_worms_cb), button);
        

        label = gtk_label_new (_("Appearance"));

	hbox = gtk_hbox_new (FALSE, GNOME_PAD);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), GNOME_PAD);

	frame = games_frame_new (_("Board size"));

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), GNOME_PAD);

	button = gtk_radio_button_new_with_label (NULL, _("Tiny  (184 \xc3\x97 132)"));

	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->tilesize == 2)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
					      TRUE);
	g_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			  (tile_size_cb), (gpointer) 2);

	button = gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON(button)),
						  _("Small  (368 \xc3\x97 264)"));
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->tilesize == 4)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
					      TRUE);
	g_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			  (tile_size_cb), (gpointer) 4);

	button = gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON(button)),
						  _("Medium  (460 \xc3\x97 330)"));
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->tilesize == 5)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
					      TRUE);
	g_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			  (tile_size_cb), (gpointer) 5);

	button = gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON(button)),
						  _("Large  (736 \xc3\x97 528)"));
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->tilesize == 8)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
					      TRUE);
	g_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			  (tile_size_cb), (gpointer) 8);

	button = gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON(button)),
						  _("Extra large  (920 \xc3\x97 660)"));
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->tilesize == 10)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
					      TRUE);
	g_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			  (tile_size_cb), (gpointer) 10);

	button = gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON(button)),
						  _("Huge  (1840 \xc3\x97 1320)"));
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->tilesize == 20)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
					      TRUE);
	g_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			  (tile_size_cb), (gpointer) 20);

	gtk_container_add (GTK_CONTAINER (frame), vbox);

	gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                                  hbox, label);

	for (i = 0; i < NUMWORMS; i++) {
		buffer = g_strdup_printf ("%s %d", _("Worm"), i + 1);
		label = gtk_label_new (buffer);
                g_free (buffer);

                vbox = gtk_vbox_new (FALSE, 6);
                gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

		gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);

                frame = games_frame_new (_("Keyboard controls"));
                gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

		table = gtk_table_new (4, 5, FALSE);
		control_table[i] = table;
                gtk_container_add (GTK_CONTAINER (frame), table);
        
		gtk_container_set_border_width (GTK_CONTAINER (table), 0);
		gtk_table_set_col_spacings (GTK_TABLE (table), 0);
		gtk_table_set_row_spacings (GTK_TABLE (table), 0);
		
		control_button[i][0] = gtk_button_new_with_label (_("Up"));
		gtk_widget_set_name (control_button[i][0],
				     "WormControlUpLabel");
		gtk_table_attach (GTK_TABLE (table), control_button[i][0],
				  2, 3, 0, 1, 0, 0 , 3, 3);

		entry = gtk_entry_new ();
		gtk_widget_set_name (entry, "WormControlUpEntry");
		gtk_entry_set_text (GTK_ENTRY (entry), properties->wormprops[i]->up);
		gtk_entry_set_editable (GTK_ENTRY (entry), FALSE);
                gtk_entry_set_width_chars (GTK_ENTRY (entry), KB_TEXT_NCHARS);
		gtk_table_attach (GTK_TABLE (table), entry, 2, 3, 1, 2, 0, 0, 3, 3);
		g_signal_connect (GTK_OBJECT (entry), "key_press_event",
                                  GTK_SIGNAL_FUNC (worm_up_cb), (gpointer) i);
		g_signal_connect (GTK_OBJECT (control_button[i][0]),
				  "clicked",
				  GTK_SIGNAL_FUNC(key_change_cb),
				  entry);
		entries[i][0] = entry;
		
		control_button[i][1] = gtk_button_new_with_label (_("Down"));
		gtk_widget_set_name (control_button[i][1],
				     "WormControlDownLabel");
		gtk_table_attach (GTK_TABLE (table), control_button[i][1],
				  2, 3, 4, 5, 0, 0, 3, 3);

		entry = gtk_entry_new ();
		gtk_widget_set_name (entry, "WormControlDownEntry");
		gtk_entry_set_text (GTK_ENTRY (entry), properties->wormprops[i]->down);
		gtk_entry_set_editable (GTK_ENTRY (entry), FALSE);
                gtk_entry_set_width_chars (GTK_ENTRY (entry), KB_TEXT_NCHARS);
		gtk_table_attach (GTK_TABLE (table), entry, 2, 3, 3, 4, 0, 0, 3, 3);
		g_signal_connect (GTK_OBJECT (entry), "key_press_event",
                                  GTK_SIGNAL_FUNC (worm_down_cb), (gpointer) i);
		g_signal_connect (GTK_OBJECT (control_button[i][1]),
				  "clicked",
				  GTK_SIGNAL_FUNC(key_change_cb),
				  entry);
		entries[i][1] = entry;

		control_button[i][2] = gtk_button_new_with_label (_("Left"));
		gtk_table_attach (GTK_TABLE (table),
				  control_button[i][2],
				  0, 1, 2, 3, 0, 0, 3, 3);

		entry = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (entry), properties->wormprops[i]->left);
		gtk_entry_set_editable (GTK_ENTRY (entry), FALSE);
                gtk_entry_set_width_chars (GTK_ENTRY (entry), KB_TEXT_NCHARS);
		gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 2, 3, 0, 0, 3, 3);

		g_signal_connect (GTK_OBJECT (entry), "key_press_event",
                                  GTK_SIGNAL_FUNC (worm_left_cb), (gpointer) i);
		g_signal_connect (GTK_OBJECT (control_button[i][2]),
				  "clicked",
				  GTK_SIGNAL_FUNC(key_change_cb),
				  entry);
		entries[i][2] = entry;

		control_button[i][3] = gtk_button_new_with_label (_("Right"));
		gtk_table_attach (GTK_TABLE (table), control_button[i][3],
				  4, 5, 2, 3, 0, 0, 3, 3);

		entry = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (entry), properties->wormprops[i]->right);
		gtk_entry_set_editable (GTK_ENTRY (entry), FALSE);
                gtk_entry_set_width_chars (GTK_ENTRY (entry), KB_TEXT_NCHARS);
		gtk_table_attach (GTK_TABLE (table), entry, 3, 4, 2, 3, 0, 0, 3, 3);
		g_signal_connect (GTK_OBJECT (entry), "key_press_event",
                                  GTK_SIGNAL_FUNC (worm_right_cb), (gpointer) i);
		g_signal_connect (GTK_OBJECT (control_button[i][3]),
				  "clicked",
				  GTK_SIGNAL_FUNC(key_change_cb),
				  entry);
		entries[i][3] = entry;
                
                frame = games_frame_new (_("Options"));
                gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

                vbox2 = gtk_vbox_new (FALSE, 6);
                gtk_container_add (GTK_CONTAINER (frame), vbox2);

		button = gtk_check_button_new_with_label
			(_("Use relative movement"));
                gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);

                table2 = gtk_table_new (1, 2, FALSE);
                gtk_box_pack_start (GTK_BOX (vbox2), table2, FALSE, FALSE, 0);

		label2 = gtk_label_new (_("Worm color:"));
		gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
                gtk_table_attach_defaults (GTK_TABLE (table2), label2, 0, 1, 0, 1);

		omenu = gtk_option_menu_new ();
		menu = gtk_menu_new ();
		menuitem = gtk_menu_item_new_with_label (_("Red"));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
		g_signal_connect (GTK_OBJECT (menuitem), "activate",
				  GTK_SIGNAL_FUNC (set_worm_color_cb),
				  (gpointer) (WORMRED << 2 | i));
		menuitem = gtk_menu_item_new_with_label (_("Green"));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
		g_signal_connect (GTK_OBJECT (menuitem), "activate",
				  GTK_SIGNAL_FUNC (set_worm_color_cb),
				  (gpointer) (WORMGREEN << 2 | i));
		menuitem = gtk_menu_item_new_with_label (_("Blue"));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
		g_signal_connect (GTK_OBJECT (menuitem), "activate",
				  GTK_SIGNAL_FUNC (set_worm_color_cb),
				  (gpointer) (WORMBLUE << 2 | i));
		menuitem = gtk_menu_item_new_with_label (_("Yellow"));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
		g_signal_connect (GTK_OBJECT (menuitem), "activate",
				  GTK_SIGNAL_FUNC (set_worm_color_cb),
				  (gpointer) (WORMYELLOW << 2 | i));
		menuitem = gtk_menu_item_new_with_label (_("Cyan"));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
		g_signal_connect (GTK_OBJECT (menuitem), "activate",
				  GTK_SIGNAL_FUNC (set_worm_color_cb),
				  (gpointer) (WORMCYAN << 2 | i));
		menuitem = gtk_menu_item_new_with_label (_("Purple"));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
		g_signal_connect (GTK_OBJECT (menuitem), "activate",
				  GTK_SIGNAL_FUNC (set_worm_color_cb),
				  (gpointer) (WORMPURPLE << 2 | i));
		menuitem = gtk_menu_item_new_with_label (_("Gray"));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
		g_signal_connect (GTK_OBJECT (menuitem), "activate",
				  GTK_SIGNAL_FUNC (set_worm_color_cb),
				  (gpointer) (WORMGRAY << 2 | i));

		gtk_menu_set_active (GTK_MENU (menu),
				     properties->wormprops[i]->color - WORMRED);
		
		gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
                gtk_table_attach_defaults (GTK_TABLE (table2), omenu, 1, 2, 0, 1);

		set_worm_controls_sensitivity (i, properties->wormprops[i]->relmove);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
					      properties->wormprops[i]->relmove);
		g_signal_connect (G_OBJECT (button), "toggled",
				  G_CALLBACK (worm_relative_movement_cb),
				  (gpointer) i);
	}

	g_signal_connect (G_OBJECT (pref_dialog), "response",
			  G_CALLBACK (apply_cb), NULL);
	g_signal_connect (G_OBJECT (pref_dialog), "destroy",
			  G_CALLBACK (destroy_cb), NULL);
	g_signal_connect (G_OBJECT (pref_dialog), "close",
			  G_CALLBACK (destroy_cb), NULL);
	
	gtk_widget_show_all (pref_dialog);

	for (i = 0; i<NUMWORMS; i++)
		for (j = 0; j<4; j++)
			gtk_widget_set_sensitive (entries[i][j], FALSE);
			
}
