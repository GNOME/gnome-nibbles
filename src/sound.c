/*
 *   Gnome Nibbles: Gnome Worm Game
 *   Written by Sean MacIsaac <sjm@acm.org>, Ian Peters <itp@gnu.org>,
 *              Guillaume Beland <guillaume.beland@gmail.com>
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

#include <gdk/gdk.h>
#include <canberra-gtk.h>

#include "sound.h"

extern GtkWidget *clutter_widget;

static gboolean enabled = TRUE;

void
sound_enable (gboolean enable)
{
  enabled = enable;
}

void
play_sound (const gchar *name)
{
  gchar *filename, *path;

  if (!enabled)
    return;

  filename = g_strdup_printf ("%s.ogg", name);
  path = g_build_filename (SOUND_DIRECTORY, filename, NULL);
  g_free (filename);

  ca_gtk_play_for_widget (clutter_widget,
                          0,
                          CA_PROP_MEDIA_NAME, name,
                          CA_PROP_MEDIA_FILENAME, path, NULL);
  g_free (path);
}
