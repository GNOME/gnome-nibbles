#include <gdk/gdkkeysyms.h>

#include "preferences.h"
#include "../gnobots2/keylabels.h"

static GtkWidget *pref_dialog = NULL;

static GnibblesProperties *t_properties;

static gint pref_dialog_valid = 0;

extern GtkWidget *window;

extern GnibblesProperties *properties;

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
	pref_dialog_valid = 0;

	gnibbles_properties_destroy (t_properties);
}

static void apply_cb (GtkWidget *widget, gint pagenum, gpointer data)
{
	if (pagenum == -1) {
		gnibbles_properties_destroy (properties);
		properties = gnibbles_properties_copy (t_properties);

		gnibbles_properties_save (properties);
	}
}

static void game_speed_cb (GtkWidget *widget, gpointer data)
{
	if (pref_dialog_valid && GTK_TOGGLE_BUTTON (widget)->active) {
		t_properties->gamespeed = (gint) data;
		gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
	}
}

static void start_level_cb (GtkWidget *widget, gpointer data)
{
	if (!pref_dialog_valid)
		return;

	t_properties->startlevel = gtk_spin_button_get_value_as_int
		(GTK_SPIN_BUTTON (data));

	gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
}

static void random_order_cb (GtkWidget *widget, gpointer data)
{
	if (!pref_dialog_valid)
		return;

	if (GTK_TOGGLE_BUTTON (widget)->active) {
		t_properties->random = 1;
		gtk_widget_set_sensitive ((GtkWidget *) data, FALSE);
	} else {
		t_properties->random = 0;
		gtk_widget_set_sensitive ((GtkWidget *) data, TRUE);
	}

	gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
}

static void fake_bonus_cb (GtkWidget *widget, gpointer data)
{
	if (!pref_dialog_valid)
		return;

	if (GTK_TOGGLE_BUTTON (widget)->active)
		t_properties->fakes = 1;
	else
		t_properties->fakes = 0;

	gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
}

static void num_worms_cb (GtkWidget *widget, gpointer data)
{
	if (!pref_dialog_valid)
		return;

	t_properties->numworms = gtk_spin_button_get_value_as_int
		(GTK_SPIN_BUTTON (data));

	gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
}

static void worm_up_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	if (!pref_dialog_valid)
		return;
	
	gtk_entry_set_text (GTK_ENTRY (widget), keyboard_string
			(event->keyval));

	t_properties->wormprops[(gint) data]->up = event->keyval;

	gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
}

static void worm_down_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	if (!pref_dialog_valid)
		return;
	
	gtk_entry_set_text (GTK_ENTRY (widget), keyboard_string
			(event->keyval));

	t_properties->wormprops[(gint) data]->down = event->keyval;

	gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
}

static void worm_left_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	if (!pref_dialog_valid)
		return;
	
	gtk_entry_set_text (GTK_ENTRY (widget), keyboard_string
			(event->keyval));

	t_properties->wormprops[(gint) data]->left = event->keyval;

	gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
}

static void worm_right_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	if (!pref_dialog_valid)
		return;
	
	gtk_entry_set_text (GTK_ENTRY (widget), keyboard_string
			(event->keyval));

	t_properties->wormprops[(gint) data]->right = event->keyval;

	gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
}

static void set_worm_color_cb (GtkWidget *widget, gpointer data)
{
	gint color = ((gint) data) >> 2;
	gint worm = ((gint) data) & 2;

	t_properties->wormprops[worm]->color = color;

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
	gint i;
	
	if (pref_dialog)
		return;

	t_properties = gnibbles_properties_copy (properties);

	pref_dialog = gnome_property_box_new ();
	gnome_dialog_set_parent (GNOME_DIALOG (pref_dialog), GTK_WINDOW
			(window));
	gtk_signal_connect (GTK_OBJECT (pref_dialog), "destroy",
			GTK_SIGNAL_FUNC (gtk_widget_destroyed), &pref_dialog);
	gtk_signal_connect (GTK_OBJECT (pref_dialog), "destroy",
			GTK_SIGNAL_FUNC (destroy_cb), NULL);

	label = gtk_label_new (_("Game"));
	gtk_widget_show (label);

	hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_border_width (GTK_CONTAINER (hbox), GNOME_PAD_SMALL);
	gtk_widget_show (hbox);

	frame = gtk_frame_new (_("Speed"));
	gtk_container_border_width (GTK_CONTAINER (frame), GNOME_PAD_SMALL);
	gtk_widget_show (frame);

	vbox = gtk_vbox_new (TRUE, 0);
	gtk_container_border_width (GTK_CONTAINER (vbox), GNOME_PAD_SMALL);
	gtk_widget_show (vbox);

	button = gtk_radio_button_new_with_label (NULL, _("Nibbles newbie"));
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(game_speed_cb), (gpointer) 4);
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->gamespeed == 4)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);

	button = gtk_radio_button_new_with_label (gtk_radio_button_group
			(GTK_RADIO_BUTTON (button)), _("My second day"));
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(game_speed_cb), (gpointer) 3);
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->gamespeed == 3)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);

	button = gtk_radio_button_new_with_label (gtk_radio_button_group
			(GTK_RADIO_BUTTON (button)), _("Not too shabby"));
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(game_speed_cb), (gpointer) 2);
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->gamespeed == 2)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);

	button = gtk_radio_button_new_with_label (gtk_radio_button_group
			(GTK_RADIO_BUTTON (button)),
			_("Finger-twitching good"));
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(game_speed_cb), (gpointer) 1);
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->gamespeed == 1)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);

	gtk_container_add (GTK_CONTAINER (frame), vbox);

	gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);

	table = gtk_table_new (4, 2, FALSE);
	gtk_widget_show (table);
	gtk_container_border_width (GTK_CONTAINER (table), GNOME_PAD_SMALL);
	gtk_table_set_row_spacings (GTK_TABLE (table), GNOME_PAD);

	label2 = gtk_label_new ("Starting level: ");
	gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
	gtk_widget_show (label2);
	gtk_table_attach (GTK_TABLE (table), label2, 0, 1, 2, 3, GTK_EXPAND |
			GTK_FILL, 0, 0, 0);

	adjustment = gtk_adjustment_new ((gfloat) properties->startlevel, 1.0,
			MAXLEVEL, 1.0, 5.0, 0.0);

	levelspinner = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 0, 0);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (levelspinner), FALSE);
	gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
			GTK_SIGNAL_FUNC (start_level_cb), levelspinner);
	gtk_widget_show (levelspinner);
	if (properties->random)
		gtk_widget_set_sensitive (GTK_WIDGET (levelspinner), FALSE);
	gtk_table_attach (GTK_TABLE (table), levelspinner, 1, 2, 2, 3,
			GTK_EXPAND | GTK_FILL, 0, 0, 0);

	button = gtk_check_button_new_with_label (_("Levels in random order"));
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(random_order_cb), levelspinner);
	gtk_widget_show (button);
	gtk_table_attach (GTK_TABLE (table), button, 0, 2, 0, 1,
			GTK_EXPAND | GTK_FILL, 0, 0, 0);
	if (properties->random)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);

	button = gtk_check_button_new_with_label (_("Fake bonuses (fun!)"));
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(fake_bonus_cb), NULL);
	gtk_widget_show (button);
	gtk_table_attach (GTK_TABLE (table), button, 0, 2, 1, 2,
			GTK_EXPAND | GTK_FILL, 0, 0, 0);
	if (properties->fakes)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);

	label2 = gtk_label_new ("Number of players: ");
	gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
	gtk_widget_show (label2);
	gtk_table_attach (GTK_TABLE (table), label2, 0, 1, 3, 4, GTK_EXPAND |
			GTK_FILL, 0, 0, 0);

	adjustment = gtk_adjustment_new ((gfloat) properties->numworms, 1.0,
			NUMWORMS, 1.0, 1.0, 0.0);

	button = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 0, 0);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (button), FALSE);
	gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
			GTK_SIGNAL_FUNC (num_worms_cb), button);
	gtk_widget_show (button);
	gtk_table_attach (GTK_TABLE (table), button, 1, 2, 3, 4, GTK_EXPAND |
			GTK_FILL, 0, 0, 0);

	gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);

	gnome_property_box_append_page (GNOME_PROPERTY_BOX (pref_dialog),
			hbox, label);

	for (i = 0; i < NUMWORMS; i++) {
		sprintf (buffer, "Worm %d", i + 1);
		label = gtk_label_new (buffer);
		gtk_widget_show (label);

		table = gtk_table_new (3, 4, FALSE);
		gtk_container_border_width (GTK_CONTAINER (table),
				GNOME_PAD_SMALL);
		gtk_table_set_col_spacings (GTK_TABLE (table), GNOME_PAD);
		gtk_widget_show (table);

		label2 = gtk_label_new ("Up:");
		gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
		gtk_widget_show (label2);
		gtk_table_attach (GTK_TABLE (table), label2, 0, 1, 0, 1,
				GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
				0, 0);

		entry = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (entry), keyboard_string
				(properties->wormprops[i]->up));
		gtk_entry_set_editable (GTK_ENTRY (entry), FALSE);
		gtk_signal_connect (GTK_OBJECT (entry), "key_press_event",
				GTK_SIGNAL_FUNC (worm_up_cb), (gpointer) i);
		gtk_widget_show (entry);
		gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1,
				0, 0,
				0, 0);

		label2 = gtk_label_new ("Down:");
		gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
		gtk_widget_show (label2);
		gtk_table_attach (GTK_TABLE (table), label2, 2, 3, 0, 1,
				GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
				0, 0);

		entry = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (entry), keyboard_string
				(properties->wormprops[i]->down));
		gtk_entry_set_editable (GTK_ENTRY (entry), FALSE);
		gtk_signal_connect (GTK_OBJECT (entry), "key_press_event",
				GTK_SIGNAL_FUNC (worm_down_cb), (gpointer) i);
		gtk_widget_show (entry);
		gtk_table_attach (GTK_TABLE (table), entry, 3, 4, 0, 1,
				0, 0,
				0, 0);

		label2 = gtk_label_new ("Left:");
		gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
		gtk_widget_show (label2);
		gtk_table_attach (GTK_TABLE (table), label2, 0, 1, 1, 2,
				GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
				0, 0);

		entry = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (entry), keyboard_string
				(properties->wormprops[i]->left));
		gtk_entry_set_editable (GTK_ENTRY (entry), FALSE);
		gtk_signal_connect (GTK_OBJECT (entry), "key_press_event",
				GTK_SIGNAL_FUNC (worm_left_cb), (gpointer) i);
		gtk_widget_show (entry);
		gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2,
				0, 0,
				0, 0);

		label2 = gtk_label_new ("Right:");
		gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
		gtk_widget_show (label2);
		gtk_table_attach (GTK_TABLE (table), label2, 2, 3, 1, 2,
				GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
				0, 0);

		entry = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (entry), keyboard_string
				(properties->wormprops[i]->right));
		gtk_entry_set_editable (GTK_ENTRY (entry), FALSE);
		gtk_signal_connect (GTK_OBJECT (entry), "key_press_event",
				GTK_SIGNAL_FUNC (worm_right_cb), (gpointer) i);
		gtk_widget_show (entry);
		gtk_table_attach (GTK_TABLE (table), entry, 3, 4, 1, 2,
				0, 0,
				0, 0);

		label2 = gtk_label_new ("Color:");
		gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
		gtk_widget_show (label2);
		gtk_table_attach (GTK_TABLE (table), label2, 0, 1, 2, 3,
				GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
				0, 0);

		omenu = gtk_option_menu_new ();
		menu = gtk_menu_new ();
		menuitem = gtk_menu_item_new_with_label ("Red");
		gtk_widget_show (menuitem);
		gtk_menu_append (GTK_MENU (menu), menuitem);
		gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				GTK_SIGNAL_FUNC (set_worm_color_cb),
				(gpointer) (WORMRED << 2 | i));
		menuitem = gtk_menu_item_new_with_label ("Green");
		gtk_widget_show (menuitem);
		gtk_menu_append (GTK_MENU (menu), menuitem);
		gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				GTK_SIGNAL_FUNC (set_worm_color_cb),
				(gpointer) (WORMGREEN << 2 | i));
		menuitem = gtk_menu_item_new_with_label ("Blue");
		gtk_widget_show (menuitem);
		gtk_menu_append (GTK_MENU (menu), menuitem);
		gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				GTK_SIGNAL_FUNC (set_worm_color_cb),
				(gpointer) (WORMBLUE << 2 | i));
		menuitem = gtk_menu_item_new_with_label ("Yellow");
		gtk_widget_show (menuitem);
		gtk_menu_append (GTK_MENU (menu), menuitem);
		gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				GTK_SIGNAL_FUNC (set_worm_color_cb),
				(gpointer) (WORMYELLOW << 2 | i));
		menuitem = gtk_menu_item_new_with_label ("Cyan");
		gtk_widget_show (menuitem);
		gtk_menu_append (GTK_MENU (menu), menuitem);
		gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				GTK_SIGNAL_FUNC (set_worm_color_cb),
				(gpointer) (WORMCYAN << 2 | i));
		menuitem = gtk_menu_item_new_with_label ("Purple");
		gtk_widget_show (menuitem);
		gtk_menu_append (GTK_MENU (menu), menuitem);
		gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				GTK_SIGNAL_FUNC (set_worm_color_cb),
				(gpointer) (WORMPURPLE << 2 | i));
		menuitem = gtk_menu_item_new_with_label ("Gray");
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

		gnome_property_box_append_page (GNOME_PROPERTY_BOX
				(pref_dialog), table, label);
	}

	gtk_signal_connect (GTK_OBJECT (pref_dialog), "apply", GTK_SIGNAL_FUNC
			(apply_cb), NULL);

	pref_dialog_valid = 1;

	gtk_widget_show (pref_dialog);
}
