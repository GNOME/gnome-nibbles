#ifndef NETWORK_H
#define NETWORK_H

#define PLAYER_1 1
#define PLAYER_2 31

#define WORM1 0
#define WORM2 1

extern char *game_server;
extern void network_game_move (guint);
extern int network_allow (void);
extern void network_new (GtkWidget *parent_window);
extern void network_start (void);
extern void network_stop (void);
extern gboolean is_network_running (void);
extern void network_add_bonus (gint t_x, gint t_y,
                   gint t_type, gint t_fake, gint t_countdown);
extern void network_remove_bonus (gint x, gint y);
extern gboolean network_is_host (void);
extern void network_move_worms (void);

#endif

