#ifndef _GNIBBLES_H_
#define _GNIBBLES_H_

#include <config.h>
#include <gnome.h>

#define PIXMAPWIDTH 10
#define PIXMAPHEIGHT 10
#define BOARDWIDTH 92
#define BOARDHEIGHT 66
#define BLANKPIXMAP 0

#define NUMWORMS 4

#define WORMRED 12
#define WORMGREEN 13
#define WORMBLUE 14
#define WORMYELLOW 15
#define WORMCYAN 16
#define WORMPURPLE 17
#define WORMGRAY 18

#define WORMCHAR 'w'
#define EMPTYCHAR 'a'

#define CONTINUE 0
#define NEWROUND 1
#define GAMEOVER 2

#define GAMEDELAY 20
#define BONUSDELAY 100

#define MAXLEVEL 26

void gnibbles_draw_pixmap (gint which, gint x, gint y);
void gnibbles_draw_big_pixmap (gint which, gint x, gint y);
void gnibbles_draw_pixmap_buffer (gint which, gint x, gint y);
void gnibbles_draw_big_pixmap_buffer (gint which, gint x, gint y);
void gnibbles_load_pixmap ();
void gnibbles_load_level (int level);
void gnibbles_init ();
void gnibbles_add_bonus (int regular);
void gnibbles_destroy ();
gint gnibbles_move_worms ();
void gnibbles_keypress_worms (guint keyval);
void gnibbles_undraw_worms (gint data);

#endif
