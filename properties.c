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

#include "properties.h"

GnibblesProperties *gnibbles_properties_new ()
{
	GnibblesProperties *tmp;
	gint i;
	gchar buffer[256];

	tmp = (GnibblesProperties *) malloc (sizeof (GnibblesProperties));

	tmp->numworms = gnome_config_get_int
		("/gnibbles/Preferences/numworms=1");
	tmp->gamespeed = gnome_config_get_int
		("/gnibbles/Preferences/gamespeed=2");
	tmp->fakes = gnome_config_get_int ("/gnibbles/Preferences/fakes=0");
	tmp->random = gnome_config_get_int ("/gnibbles/Preferences/random=0");
	tmp->startlevel = gnome_config_get_int
		("/gnibbles/Preferences/startlevel=1");
	tmp->sound = gnome_config_get_int ("/gnibbles/Preferences/sound=1");
  tmp->tilesize = gnome_config_get_int ("/gnibbles/Preferences/tilesize=10");

	for (i = 0; i < NUMWORMS; i++) {
		tmp->wormprops[i] = (GnibblesWormProps *) malloc (sizeof
				(GnibblesWormProps));

		sprintf (buffer, "/gnibbles/Worm%d/color=%d", i, (i % 7) + 12);
		tmp->wormprops[i]->color = gnome_config_get_int (buffer);
		sprintf (buffer, "/gnibbles/Worm%d/relmove=0", i);
		tmp->wormprops[i]->relmove = gnome_config_get_int (buffer);
		sprintf (buffer, "/gnibbles/Worm%d/up=0", i);
		tmp->wormprops[i]->up = gnome_config_get_int (buffer);
		if (!tmp->wormprops[i]->up)
			tmp->wormprops[i]->up = GDK_Up;
		sprintf (buffer, "/gnibbles/Worm%d/down=0", i);
		tmp->wormprops[i]->down = gnome_config_get_int (buffer);
		if (!tmp->wormprops[i]->down)
			tmp->wormprops[i]->down = GDK_Down;
		sprintf (buffer, "/gnibbles/Worm%d/left=0", i);
		tmp->wormprops[i]->left = gnome_config_get_int (buffer);
		if (!tmp->wormprops[i]->left)
			tmp->wormprops[i]->left = GDK_Left;
		sprintf (buffer, "/gnibbles/Worm%d/right=0", i);
		tmp->wormprops[i]->right = gnome_config_get_int (buffer);
		if (!tmp->wormprops[i]->right)
			tmp->wormprops[i]->right = GDK_Right;
	}

	return (tmp);
}

void gnibbles_properties_destroy (GnibblesProperties *props)
{
	int i;

	for (i = 0; i < NUMWORMS; i++)
		free (props->wormprops[i]);
	
	free (props);
}

GnibblesProperties *gnibbles_properties_copy (GnibblesProperties *props)
{
	GnibblesProperties *tmp;
	int i;

	tmp = (GnibblesProperties *) malloc (sizeof (GnibblesProperties));

	tmp->numworms = props->numworms;
	tmp->gamespeed = props->gamespeed;
	tmp->fakes = props->fakes;
	tmp->random = props->random;
	tmp->startlevel = props->startlevel;
	tmp->sound = props->sound;
  tmp->tilesize = props->tilesize;
	
	for (i = 0; i < NUMWORMS; i++) {
		tmp->wormprops[i] = (GnibblesWormProps *) malloc (sizeof
				(GnibblesWormProps));

		tmp->wormprops[i]->color = props->wormprops[i]->color;
		tmp->wormprops[i]->relmove = props->wormprops[i]->relmove;
		tmp->wormprops[i]->up = props->wormprops[i]->up;
		tmp->wormprops[i]->down = props->wormprops[i]->down;
		tmp->wormprops[i]->left = props->wormprops[i]->left;
		tmp->wormprops[i]->right = props->wormprops[i]->right;
	}

	return (tmp);
}

void gnibbles_properties_save (GnibblesProperties *props)
{
	gchar buffer[255];
	int i;

	gnome_config_set_int ("/gnibbles/Preferences/numworms",
			props->numworms);
	gnome_config_set_int ("/gnibbles/Preferences/gamespeed",
			props->gamespeed);
	gnome_config_set_int ("/gnibbles/Preferences/fakes", props->fakes);
	gnome_config_set_int ("/gnibbles/Preferences/random", props->random);
	gnome_config_set_int ("/gnibbles/Preferences/startlevel",
			props->startlevel);
	gnome_config_set_int ("/gnibbles/Preferences/sound", props->sound);
  gnome_config_set_int ("/gnibbles/Preferences/tilesize", props->tilesize);

	for (i = 0; i < NUMWORMS; i++) {
		sprintf (buffer, "/gnibbles/Worm%d/color", i);
		gnome_config_set_int (buffer, props->wormprops[i]->color);
		sprintf (buffer, "/gnibbles/Worm%d/relmove", i);
		gnome_config_set_int (buffer, props->wormprops[i]->relmove);
		sprintf (buffer, "/gnibbles/Worm%d/up", i);
		gnome_config_set_int (buffer, props->wormprops[i]->up);
		sprintf (buffer, "/gnibbles/Worm%d/down", i);
		gnome_config_set_int (buffer, props->wormprops[i]->down);
		sprintf (buffer, "/gnibbles/Worm%d/left", i);
		gnome_config_set_int (buffer, props->wormprops[i]->left);
		sprintf (buffer, "/gnibbles/Worm%d/right", i);
		gnome_config_set_int (buffer, props->wormprops[i]->right);
	}

	gnome_config_sync ();
}
