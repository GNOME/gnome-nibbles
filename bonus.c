#include <config.h>
#include <gnome.h>

#include "gnibbles.h"
#include "bonus.h"

GnibblesBonus *gnibbles_bonus_new (gint t_x, gint t_y, gint t_type,
		gint t_fake, gint t_countdown)
{
	GnibblesBonus *tmp;

	tmp = (GnibblesBonus *) malloc (sizeof (GnibblesBonus));

	tmp->x = t_x;
	tmp->y = t_y;
	tmp->type = t_type;
	tmp->fake = t_fake;
	tmp->countdown = t_countdown;

	return (tmp);
}

void gnibbles_bonus_draw (GnibblesBonus *bonus)
{
	gnibbles_draw_big_pixmap (bonus->type, bonus->x, bonus->y);
}

void gnibbles_bonus_erase (GnibblesBonus *bonus)
{
	gnibbles_draw_big_pixmap (BONUSNONE, bonus->x, bonus->y);

	free (bonus);
}
