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

#include "gnibbles.h"
#include "scoreboard.h"

GnibblesScoreboard *gnibbles_scoreboard_new (GtkWidget *t_appbar)
{
	int i;
	char buffer[255];
	GtkWidget *hbox;
	GtkWidget *vsep;

	GnibblesScoreboard *tmp = (GnibblesScoreboard *) malloc (sizeof
			(GnibblesScoreboard));

	tmp->count = 0;

	for (i = 0; i < NUMWORMS; i++) {
		hbox = gtk_hbox_new (FALSE, GNOME_PAD);
		gtk_widget_show (hbox);

		if (i) {
			vsep = gtk_vseparator_new ();
			gtk_widget_show (vsep);
			gtk_box_pack_start (GTK_BOX (t_appbar), vsep, FALSE,
					FALSE, 0);
		}
		
		sprintf (buffer, _("Worm %d:"), i + 1);
		tmp->names[i] = gtk_label_new (buffer);
		gtk_widget_set_sensitive (tmp->names[i], FALSE);
		gtk_widget_show (tmp->names[i]);
		gtk_box_pack_start (GTK_BOX (hbox), tmp->names[i], FALSE,
				FALSE, 0);
		tmp->data[i] = gtk_label_new ("00, 00000");
		gtk_widget_set_sensitive (tmp->data[i], FALSE);
		gtk_widget_show (tmp->data[i]);
		gtk_box_pack_start (GTK_BOX (hbox), tmp->data[i], FALSE, FALSE,
				0);

		gtk_box_pack_start (GTK_BOX (t_appbar), hbox, FALSE, FALSE,
				GNOME_PAD);
	}

	return (tmp);
}

void gnibbles_scoreboard_destroy (GnibblesScoreboard *scoreboard)
{
	free (scoreboard);
}

void gnibbles_scoreboard_register (GnibblesScoreboard *scoreboard,
		GnibblesWorm *t_worm)
{
	scoreboard->worms[scoreboard->count] = t_worm;
	gtk_widget_set_sensitive (scoreboard->names[scoreboard->count], TRUE);
	gtk_widget_set_sensitive (scoreboard->data[scoreboard->count], TRUE);
	scoreboard->count++;
}

void gnibbles_scoreboard_update (GnibblesScoreboard *scoreboard)
{
	int i;
	gchar buffer[15];
	gchar *buffer2;

	for (i = 0; i < scoreboard->count; i++) {
		sprintf (buffer, "%02d, %05d",
				(scoreboard->worms[i]->lives > -1) ?
				scoreboard->worms[i]->lives : 0,
				scoreboard->worms[i]->score);
		gtk_label_get (GTK_LABEL (scoreboard->data[i]), &buffer2);
		if (strcmp (buffer, buffer2))
			gtk_label_set_text (GTK_LABEL (scoreboard->data[i]),
					buffer);
	}
}

void gnibbles_scoreboard_clear (GnibblesScoreboard *scoreboard)
{
	int i;

	scoreboard->count = 0;
	
	for (i = 0; i < NUMWORMS; i++) {
		gtk_widget_set_sensitive (scoreboard->names[i], FALSE);
		gtk_widget_set_sensitive (scoreboard->data[i], FALSE);
	}
}
