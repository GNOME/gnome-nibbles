#ifndef _SCOREBOARD_H_
#define _SCOREBOARD_H_

#include <config.h>
#include <gnome.h>

#include "gnibbles.h"
#include "worm.h"

typedef struct
{
	GnibblesWorm *worms[NUMWORMS];
	GtkWidget *appbar;
	GtkWidget *names[NUMWORMS];
	GtkWidget *data[NUMWORMS];
	gint count;
} GnibblesScoreboard;

GnibblesScoreboard *gnibbles_scoreboard_new (GtkWidget *t_appbar);

void gnibbles_scoreboard_register (GnibblesScoreboard *scoreboard,
		GnibblesWorm *t_worm);

void gnibbles_scoreboard_update (GnibblesScoreboard *scoreboard);

void gnibbles_scoreboard_clear (GnibblesScoreboard *scoreboard);

#endif
