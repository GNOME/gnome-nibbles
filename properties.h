#ifndef _PROPERTIES_H_
#define _PROPERTIES_H_

#include <config.h>
#include <gnome.h>

#include "gnibbles.h"

typedef struct {
	gint color;
	guint up, down, left, right;
} GnibblesWormProps;

typedef struct {
	gint numworms;
	gint gamespeed;
	gint fakes;
	gint random;
	gint startlevel;
	GnibblesWormProps *wormprops[NUMWORMS];
} GnibblesProperties;

GnibblesProperties *gnibbles_properties_new ();

void gnibbles_properties_destroy (GnibblesProperties *props);

GnibblesProperties *gnibbles_properties_copy (GnibblesProperties *props);

void gnibbles_properties_save (GnibblesProperties *props);

#endif
