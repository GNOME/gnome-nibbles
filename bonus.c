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
