#include "preferences.h"

static GtkWidget *pref_dialog = NULL;

static GnibblesProperties *t_properties;

static gint pref_dialog_valid = 0;

extern GtkWidget *window;

extern GnibblesProperties *properties;

static void destroy_cb (GtkWidget *widget, gpointer data)
{
	pref_dialog_valid = 0;
}

static void apply_cb (GtkWidget *widget, gpointer data)
{
	gnibbles_properties_destroy (properties);
	properties = gnibbles_properties_copy (t_properties);

	gnibbles_properties_save (properties);
}

static void game_speed_cb (GtkWidget *widget, gpointer data)
{
	if (pref_dialog_valid && GTK_TOGGLE_BUTTON (widget)->active) {
		t_properties->gamespeed = (gint) data;
		gnome_property_box_changed (GNOME_PROPERTY_BOX (pref_dialog));
	}
}

static void random_order_cb (GtkWidget *widget, gpointer data)
{
	if (!pref_dialog_valid)
		return;

	if (GTK_TOGGLE_BUTTON (widget)->active)
		t_properties->random = 1;
	else
		t_properties->random = 0;

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

void gnibbles_preferences_cb (GtkWidget *widget, gpointer data)
{
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *frame;
	GtkWidget *button;
	GtkWidget *vbox;
	
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

	vbox = gtk_vbox_new (TRUE, 0);
	gtk_container_border_width (GTK_CONTAINER (vbox), GNOME_PAD_SMALL);
	gtk_widget_show (vbox);

	button = gtk_check_button_new_with_label (_("Levels in random order"));
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(random_order_cb), NULL);
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->random)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);

	button = gtk_check_button_new_with_label (_("Fake bonuses (fun!)"));
	gtk_signal_connect (GTK_OBJECT (button), "toggled", GTK_SIGNAL_FUNC
			(fake_bonus_cb), NULL);
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	if (properties->fakes)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				TRUE);

	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

	gnome_property_box_append_page (GNOME_PROPERTY_BOX (pref_dialog),
			hbox, label);

	gtk_signal_connect (GTK_OBJECT (pref_dialog), "apply", GTK_SIGNAL_FUNC
			(apply_cb), NULL);

	pref_dialog_valid = 1;

	gtk_widget_show (pref_dialog);
}
