#ifndef _BONI_H_
#define _BONI_H_

#include <config.h>
#include <gnome.h>

#include "bonus.h"

#define MAXBONUSES 100
#define MAXMISSED 3

typedef struct {
	GnibblesBonus *bonuses[MAXBONUSES];
	gint numbonuses;
	gint numleft;
	gint missed;
} GnibblesBoni;

GnibblesBoni *gnibbles_boni_new ();

void gnibbles_boni_destroy (GnibblesBoni *boni);

void gnibbles_boni_add_bonus (GnibblesBoni *boni, gint t_x, gint t_y,
		gint t_type, gint t_fake, gint t_countdown);

void gnibbles_boni_remove_bonus (GnibblesBoni *boni, gint x, gint y);

int gnibbles_boni_fake (GnibblesBoni *boni, gint x, gint y);

#endif
