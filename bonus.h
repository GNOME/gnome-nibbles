#ifndef _BONUS_H_
#define _BONUS_H_

#include <config.h>
#include <gnome.h>

#define BONUSNONE	0
#define BONUSREGULAR	1
#define BONUSHALF	2
#define BONUSDOUBLE	3
#define BONUSLIFE	4
#define BONUSREVERSE	5
#define BONUSCUT	6
#define BONUSSWITCH	7

typedef struct {
	gint x, y;
	guint type;
	gint fake;
	gint countdown;
} GnibblesBonus;

GnibblesBonus *gnibbles_bonus_new (gint t_x, gint t_y, gint t_type,
		gint t_fake, gint t_countdown);

void gnibbles_bonus_draw (GnibblesBonus *bonus);

void gnibbles_bonus_erase (GnibblesBonus *bonus);

#endif
