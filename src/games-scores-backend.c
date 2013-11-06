/* games-scores-backend.c 
 *
 * Copyright (C) 2005 Callum McKenzie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <glib.h>
#include <glib-object.h>

#include <string.h>
#include <stdlib.h>

#include "games-score.h"
#include "games-scores.h"
#include "games-scores-backend.h"

struct GamesScoresBackendPrivate {
  GList *scores_list;
  GamesScoreStyle style;
  time_t timestamp;
  gchar *filename;
  gint fd;
};

G_DEFINE_TYPE (GamesScoresBackend, games_scores_backend, G_TYPE_OBJECT);

void
games_scores_backend_startup (void)
{

}

static void
games_scores_backend_finalize (GObject *object)
{
  GamesScoresBackend *backend = GAMES_SCORES_BACKEND (object);
  GamesScoresBackendPrivate *priv = backend->priv;

  g_free (priv->filename);
  /* FIXME: more to do? */

  G_OBJECT_CLASS (games_scores_backend_parent_class)->finalize (object);
}

static void
games_scores_backend_class_init (GamesScoresBackendClass * klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GamesScoresBackendPrivate));
  oclass->finalize = games_scores_backend_finalize;
}

static void
games_scores_backend_init (GamesScoresBackend * backend)
{
  backend->priv = G_TYPE_INSTANCE_GET_PRIVATE (backend,
                                               GAMES_TYPE_SCORES_BACKEND,
                                               GamesScoresBackendPrivate);
}

GamesScoresBackend *
games_scores_backend_new (GamesScoreStyle style,
                          char *base_name,
                          char *name)
{
  GamesScoresBackend *backend;
  gchar *fullname;

  backend = GAMES_SCORES_BACKEND (g_object_new (GAMES_TYPE_SCORES_BACKEND,
                                                NULL));

  if (name[0] == '\0')                /* Name is "" */
    fullname = g_strjoin (".", base_name, "scores", NULL);
  else
    fullname = g_strjoin (".", base_name, name, "scores", NULL);

  backend->priv->timestamp = 0;
  backend->priv->style = style;
  backend->priv->scores_list = NULL;
  backend->priv->filename = g_build_filename (SCORESDIR, fullname, NULL);
  g_free (fullname);

  backend->priv->fd = -1;

  return backend;
}



/**
 * games_scores_backend_get_scores:
 * @self: the backend to get the scores from
 * 
 * You can alter the list returned by this function, but you must
 * make sure you set it again with the _set_scores method or discard it
 * with with the _discard_scores method. Otherwise deadlocks will ensue.
 *
 * Return value: (transfer none) (allow-none) (element-type GnomeGamesSupport.Score): The list of scores
 */
GList *
games_scores_backend_get_scores (GamesScoresBackend * self)
{
  return NULL;
}

gboolean
games_scores_backend_set_scores (GamesScoresBackend * self, GList * list)
{
  return FALSE;
}

void
games_scores_backend_discard_scores (GamesScoresBackend * self)
{

}
