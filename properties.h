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

#ifndef _PROPERTIES_H_
#define _PROPERTIES_H_

#include <config.h>
#include <gnome.h>

#include "gnibbles.h"

typedef struct {
	gint color;
	gint relmove;
	guint up, down, left, right;
} GnibblesWormProps;

typedef struct {
	gint numworms;
	gint gamespeed;
	gint fakes;
	gint random;
	gint startlevel;
	gint sound;
  gint tilesize;
	GnibblesWormProps *wormprops[NUMWORMS];
} GnibblesProperties;

GnibblesProperties *gnibbles_properties_new ();

void gnibbles_properties_destroy (GnibblesProperties *props);

GnibblesProperties *gnibbles_properties_copy (GnibblesProperties *props);

void gnibbles_properties_save (GnibblesProperties *props);

#endif
