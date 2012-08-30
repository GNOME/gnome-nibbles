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

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

#include "preferences.h"
#include "main.h"
#include "games-pause-action.h"
#include "games-controls.h"

#define KB_TEXT_WIDTH 60
#define KB_TEXT_HEIGHT 32
#define KB_TEXT_NCHARS 8

extern GSettings *settings;
extern GSettings *worm_settings[NUMWORMS];
static GtkWidget *pref_dialog = NULL;
static gint unpause = 0;
extern GtkWidget *window;
extern GnibblesProperties *properties;

GtkWidget *start_level_label, *start_level_spin_button;
GtkWidget *num_human, *num_ai;

static void
destroy_cb (GtkWidget * widget, gpointer data)
{
  if (unpause) {
    gtk_action_activate (pause_action);
    unpause = 0;
  }
  pref_dialog = NULL;
}

static void
apply_cb (GtkWidget * widget, gint action, gpointer data)
{
  gtk_widget_destroy (widget);
}

static void
game_speed_cb (GtkWidget * widget, gpointer data)
{
  if (!pref_dialog)
    return;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
    gnibbles_properties_set_speed (GPOINTER_TO_INT (data));
  }
}

static void
start_level_cb (GtkWidget * widget, gpointer data)
{
  gint level;

  if (!pref_dialog)
    return;

  end_game (FALSE);

  level = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data));
  gnibbles_properties_set_start_level (level);
}

static void
random_order_cb (GtkWidget * widget, gpointer data)
{
  gboolean value;

  if (!pref_dialog)
    return;

  value = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  gtk_widget_set_sensitive (start_level_label, !value);
  gtk_widget_set_sensitive (start_level_spin_button, !value);

  gnibbles_properties_set_random (value);
}

static void
fake_bonus_cb (GtkWidget * widget, gpointer data)
{
  gboolean value;

  if (!pref_dialog)
    return;

  value = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  gnibbles_properties_set_fakes (value);
}

static void
sound_cb (GtkWidget * widget, gpointer data)
{
  gboolean value;

  if (!pref_dialog)
    return;

  value = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  gnibbles_properties_set_sound (value);
}

static void
num_worms_cb (GtkWidget * widget, gpointer data)
{
  gint human, ai;

  if (!pref_dialog)
    return;

  human = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (num_human));
  ai = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (num_ai));

  if (!ai && !human) {
    human = 1;
  } else if (data == num_human && ai + human >= NUMWORMS) {
    ai = NUMWORMS - human;
  } else if (data == num_ai && ai + human >= NUMWORMS) {
    human = NUMWORMS - ai;
  }
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (num_human), human);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (num_ai), ai);
  gnibbles_properties_set_worms_number (human);
  gnibbles_properties_set_ai_number (ai);
}

static void
set_worm_color_cb (GtkWidget * widget, gpointer data)
{
  gint color = gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) + WORMRED;
  gint worm = GPOINTER_TO_INT (data);

  gnibbles_properties_set_worm_color (worm, color);
}

static void
set_worm_controls_sensitivity (gint i, gboolean value)
{
  /* FIXME */

  /* This is meant to make the up and down controls
   * unavailable if we are in relative mode. However
   * The new key selection API doesn't support this
   * yet. */
}

static void
worm_relative_movement_cb (GtkWidget * widget, gpointer data)
{
  gint i;

  if (pref_dialog == NULL)
    return;

  i = GPOINTER_TO_INT (data);

  set_worm_controls_sensitivity
    (i, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));

  gnibbles_properties_set_worm_relative_movement
    (i, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
}

static void
set_label_bold (GtkLabel * label)
{
  PangoAttrList *attrlist;
  PangoAttribute *attr;

  g_assert (label != NULL);

  attrlist = pango_attr_list_new ();

  attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
  attr->start_index = 0;
  attr->end_index = -1;
  pango_attr_list_change (attrlist, attr);

  gtk_label_set_attributes (label, attrlist);
}

void
gnibbles_preferences_cb (GtkWidget * widget, gpointer data)
{
  GtkWidget *notebook;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *levelspinner;
  GtkWidget *vbox_game, *vbox_speed, *vbox_options, *vbox_wormx;
  GtkAdjustment *adjustment;
  GtkWidget *label2;
  GtkWidget *grid, *grid2;
  GtkWidget *omenu;
  GtkWidget *controls;
  gchar *buffer;
  gint i;
  gint running = 0;

  if (pref_dialog) {
    gtk_window_present (GTK_WINDOW (pref_dialog));
    return;
  }

  if (!games_pause_action_get_is_paused (GAMES_PAUSE_ACTION (pause_action))) {
    unpause = 1;
    gtk_action_activate (pause_action);
  }

  if (game_running ())
    running = 1;

  pref_dialog = gtk_dialog_new_with_buttons (_("Nibbles Preferences"),
                                             GTK_WINDOW (window), 0,
                                             GTK_STOCK_CLOSE,
                                             GTK_RESPONSE_CLOSE, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (pref_dialog), 5);
  gtk_box_set_spacing
    (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (pref_dialog))), 2);

  notebook = gtk_notebook_new ();
  gtk_container_set_border_width (GTK_CONTAINER (notebook), 5);
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (pref_dialog))),
                     notebook);

  label = gtk_label_new (_("Game"));
 

  vbox_game = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox_game), 12);
  gtk_box_set_spacing (GTK_BOX(vbox_game), 18);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox_game, label);

  vbox_speed   = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  vbox_options = gtk_box_new (GTK_ORIENTATION_VERTICAL, 7);

  gtk_box_pack_start (GTK_BOX (vbox_game), vbox_speed, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox_game), vbox_options, FALSE, FALSE, 0);

  /* Speed */
  label = gtk_label_new (_("Speed"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  set_label_bold (GTK_LABEL(label));
  gtk_box_pack_start (GTK_BOX (vbox_speed), label, FALSE, FALSE, 0);

  button = gtk_radio_button_new_with_label (NULL, _("Nibbles newbie"));

  gtk_box_pack_start (GTK_BOX (vbox_speed), button, FALSE, FALSE, 0);
  if (properties->gamespeed == 4)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (GTK_WIDGET (button), "toggled", G_CALLBACK
                    (game_speed_cb), (gpointer) 4);

  button = gtk_radio_button_new_with_label (gtk_radio_button_get_group
                                            (GTK_RADIO_BUTTON (button)),
                                            _("My second day"));

  gtk_box_pack_start (GTK_BOX (vbox_speed), button, FALSE, FALSE, 0);
  if (properties->gamespeed == 3)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (GTK_WIDGET (button), "toggled", G_CALLBACK
                    (game_speed_cb), (gpointer) 3);

  button = gtk_radio_button_new_with_label (gtk_radio_button_get_group
                                            (GTK_RADIO_BUTTON (button)),
                                            _("Not too shabby"));

  gtk_box_pack_start (GTK_BOX (vbox_speed), button, FALSE, FALSE, 0);
  if (properties->gamespeed == 2)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (GTK_WIDGET (button), "toggled", G_CALLBACK
                    (game_speed_cb), (gpointer) 2);

  button = gtk_radio_button_new_with_label (gtk_radio_button_get_group
                                            (GTK_RADIO_BUTTON (button)),
                                            _("Finger-twitching good"));

  gtk_box_pack_start (GTK_BOX (vbox_speed), button, FALSE, FALSE, 0);
  if (properties->gamespeed == 1)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (GTK_WIDGET (button), "toggled", G_CALLBACK
                    (game_speed_cb), (gpointer) 1);


  /* Options */
  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);

  label = gtk_label_new (_("Options"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  set_label_bold (GTK_LABEL(label));
  gtk_box_pack_start (GTK_BOX (vbox_options), label, FALSE, FALSE, 0);


  button =
    gtk_check_button_new_with_mnemonic (_("_Play levels in random order"));
  gtk_box_pack_start (GTK_BOX (vbox_options), button, FALSE, FALSE, 0);

  if (running)
    gtk_widget_set_sensitive (button, FALSE);
  if (properties->random)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (GTK_WIDGET (button), "toggled", G_CALLBACK
                    (random_order_cb), NULL);

  button = gtk_check_button_new_with_mnemonic (_("_Enable fake bonuses"));
  gtk_box_pack_start (GTK_BOX (vbox_options), button, FALSE, FALSE, 0);

  if (running)
    gtk_widget_set_sensitive (button, FALSE);
  if (properties->fakes)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (GTK_WIDGET (button), "toggled", G_CALLBACK
                    (fake_bonus_cb), NULL);

  button = gtk_check_button_new_with_mnemonic (_("E_nable sounds"));
  gtk_box_pack_start (GTK_BOX (vbox_options), button, FALSE, FALSE, 0);
  if (properties->sound)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (GTK_WIDGET (button), "toggled", G_CALLBACK
                    (sound_cb), NULL);

  grid2 = gtk_grid_new ();
  gtk_box_pack_start (GTK_BOX (vbox_options), grid2, FALSE, FALSE, 0);
  gtk_grid_set_row_spacing (GTK_GRID (grid2), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid2), 12);
  gtk_container_set_border_width (GTK_CONTAINER (grid2), 0);

  label2 = gtk_label_new_with_mnemonic (_("_Starting level:"));
  start_level_label = label2;
  gtk_widget_set_name (label2, "StartLevelLabel");
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

  if (properties->random)
    gtk_widget_set_sensitive (GTK_WIDGET (label2), FALSE);
  if (running)
    gtk_widget_set_sensitive (GTK_WIDGET (label2), FALSE);
  gtk_widget_set_hexpand (label2, TRUE);
  gtk_grid_attach (GTK_GRID (grid2), label2, 0, 0, 1, 1);

  adjustment = gtk_adjustment_new ((gfloat) properties->startlevel, 1.0,
                                   MAXLEVEL, 1.0, 5.0, 0.0);

  levelspinner = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 0, 0);
  start_level_spin_button = levelspinner;
  gtk_widget_set_name (levelspinner, "StartLevelSpinButton");
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (levelspinner), FALSE);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label2), levelspinner);

  if (properties->random)
    gtk_widget_set_sensitive (GTK_WIDGET (levelspinner), FALSE);
  if (running)
    gtk_widget_set_sensitive (GTK_WIDGET (levelspinner), FALSE);
  gtk_grid_attach (GTK_GRID (grid2), levelspinner, 1, 0, 1, 1);
  g_signal_connect (GTK_ADJUSTMENT (adjustment), "value_changed",
                    G_CALLBACK (start_level_cb), levelspinner);

  label2 = gtk_label_new_with_mnemonic (_("Number of _human players:"));
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

  gtk_widget_set_hexpand (label2, TRUE);
  gtk_grid_attach (GTK_GRID (grid2), label2, 0, 1, 1, 1);
  if (running)
    gtk_widget_set_sensitive (label2, FALSE);

  adjustment = gtk_adjustment_new ((gfloat) properties->human, 0.0,
                                   NUMWORMS, 1.0, 1.0, 0.0);

  num_human = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (num_human), FALSE);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label2), num_human);

  gtk_grid_attach (GTK_GRID (grid2), num_human, 1, 1, 1, 1);
  if (running)
    gtk_widget_set_sensitive (num_human, FALSE);
  g_signal_connect (GTK_ADJUSTMENT (adjustment), "value_changed",
                    G_CALLBACK (num_worms_cb), num_human);

  label2 = gtk_label_new_with_mnemonic (_("Number of _AI players:"));
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

  gtk_widget_set_hexpand (label2, TRUE);  
  gtk_grid_attach (GTK_GRID (grid2), label2, 0, 2, 1, 1);
  if (running)
    gtk_widget_set_sensitive (label2, FALSE);

  adjustment = gtk_adjustment_new ((gfloat) properties->ai, 0.0,
                                   NUMWORMS, 1.0, 1.0, 0.0);

  num_ai = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (num_ai), FALSE);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label2), num_ai);

  gtk_grid_attach (GTK_GRID (grid2), num_ai, 1, 2, 1, 1);
  if (running)
    gtk_widget_set_sensitive (num_ai, FALSE);
  g_signal_connect (GTK_ADJUSTMENT (adjustment), "value_changed",
                    G_CALLBACK (num_worms_cb), num_ai);

  

  /* Per worm options */
  for (i = 0; i < NUMWORMS; i++) {
    buffer = g_strdup_printf ("%s %d", _("Worm"), i + 1);
    label = gtk_label_new (buffer);
    g_free (buffer);

    vbox_wormx = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width (GTK_CONTAINER (vbox_wormx), 12);

    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox_wormx, label);

    label2 = gtk_label_new (_("Keyboard Options"));
    gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
    set_label_bold (GTK_LABEL(label2));
    gtk_box_pack_start (GTK_BOX (vbox_wormx), label2, FALSE, FALSE, 0);

    controls = games_controls_list_new (worm_settings[i]);

    games_controls_list_add_controls (GAMES_CONTROLS_LIST (controls),
                                      "key-left", _("Move left"), GDK_KEY_Left,
                                      "key-right", _("Move right"), GDK_KEY_Right,
                                      "key-up", _("Move up"), GDK_KEY_Up,
                                      "key-down", _("Move down"), GDK_KEY_Down,
                                      NULL);
    gtk_box_pack_start (GTK_BOX (vbox_wormx), controls, TRUE, TRUE, 0);

    label2 = gtk_label_new (_("Options"));
    gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
    set_label_bold (GTK_LABEL(label2));
    gtk_box_pack_start (GTK_BOX (vbox_wormx), label2, FALSE, FALSE, 0);

    button = gtk_check_button_new_with_mnemonic (_("_Use relative movement"));
    gtk_box_pack_start (GTK_BOX (vbox_wormx), button, FALSE, FALSE, 0);

    grid2 = gtk_grid_new ();
    gtk_grid_set_column_spacing (GTK_GRID (grid2), 12);
    gtk_box_pack_start (GTK_BOX (vbox_wormx), grid2, FALSE, FALSE, 0);

    label2 = gtk_label_new_with_mnemonic (_("_Worm color:"));
    gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
    gtk_grid_attach (GTK_GRID (grid2), label2, 0, 0, 1, 1);

    omenu = gtk_combo_box_text_new ();
    gtk_label_set_mnemonic_widget (GTK_LABEL (label2), omenu);
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (omenu), _("Red"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (omenu), _("Green"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (omenu), _("Blue"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (omenu), _("Yellow"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (omenu), _("Cyan"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (omenu), _("Purple"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (omenu), _("Gray"));
    g_signal_connect (GTK_WIDGET (omenu), "changed",
                      G_CALLBACK (set_worm_color_cb),
                      GINT_TO_POINTER (i));
    gtk_combo_box_set_active (GTK_COMBO_BOX (omenu),
                              properties->wormprops[i]->color - WORMRED);
    gtk_grid_attach (GTK_GRID (grid2), omenu, 1, 0, 1, 1);

    set_worm_controls_sensitivity (i, properties->wormprops[i]->relmove);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                  properties->wormprops[i]->relmove);
    g_signal_connect (G_OBJECT (button), "toggled",
                      G_CALLBACK (worm_relative_movement_cb),
                      GINT_TO_POINTER (i));
  }

  g_signal_connect (G_OBJECT (pref_dialog), "response",
                    G_CALLBACK (apply_cb), NULL);
  g_signal_connect (G_OBJECT (pref_dialog), "destroy",
                    G_CALLBACK (destroy_cb), NULL);
  g_signal_connect (G_OBJECT (pref_dialog), "close",
                    G_CALLBACK (destroy_cb), NULL);

  gtk_widget_show_all (pref_dialog);

}
