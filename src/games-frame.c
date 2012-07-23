/*
 * Copyright © 2009 Christian Persch
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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

#include <gtk/gtk.h>

#include "games-frame.h"

enum {
  PROP_0,
  PROP_LABEL
};

G_DEFINE_TYPE (GamesFrame, games_frame, GTK_TYPE_BOX);

struct GamesFramePrivate {
  GtkWidget *label;
  GtkWidget *alignment;
};

static void
games_frame_init (GamesFrame * frame)
{
  GtkBox *box = GTK_BOX (frame);
  PangoAttrList *attr_list;
  PangoAttribute *attr;

  frame->priv = G_TYPE_INSTANCE_GET_PRIVATE (frame, GAMES_TYPE_FRAME, GamesFramePrivate);

  gtk_box_set_spacing (box, 6);
  gtk_box_set_homogeneous (box, FALSE);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (frame), GTK_ORIENTATION_VERTICAL);

  frame->priv->label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (frame->priv->label), 0.0, 0.5);

  attr_list = pango_attr_list_new ();
  attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
  attr->start_index = 0;
  attr->end_index = -1;
  pango_attr_list_insert (attr_list, attr);
  gtk_label_set_attributes (GTK_LABEL (frame->priv->label), attr_list);
  pango_attr_list_unref (attr_list);

  frame->priv->alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (frame->priv->alignment), 0, 0, 12, 0);

  gtk_box_pack_start (box, frame->priv->label, FALSE, FALSE, 0);
  gtk_box_pack_start (box, frame->priv->alignment, TRUE, TRUE, 0);

  gtk_widget_set_no_show_all (frame->priv->label, TRUE);
  gtk_widget_show (frame->priv->alignment);
}

static void
add_atk_relation (GtkWidget *widget, GtkWidget *other, AtkRelationType type)
{
  AtkRelationSet *set;
  AtkRelation *relation;
  AtkObject *object;

  object = gtk_widget_get_accessible (other);
  set = atk_object_ref_relation_set (gtk_widget_get_accessible (widget));
  relation = atk_relation_new (&object, 1, type);
  atk_relation_set_add (set, relation);
  g_object_unref (relation);
  g_object_unref (set);
}

static void
games_frame_add (GtkContainer *container,
                 GtkWidget *child)
{
  GamesFrame *frame = GAMES_FRAME (container);

  gtk_container_add (GTK_CONTAINER (frame->priv->alignment), child);

  add_atk_relation (frame->priv->label, child, ATK_RELATION_LABEL_FOR);
  add_atk_relation (child, frame->priv->label, ATK_RELATION_LABELLED_BY);
}

static void
games_frame_set_property (GObject *object,
                          guint prop_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
  GamesFrame *frame = GAMES_FRAME (object);

  switch (prop_id) {
    case PROP_LABEL:
      games_frame_set_label (frame, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
games_frame_class_init (GamesFrameClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->set_property = games_frame_set_property;
  container_class->add = games_frame_add;

  g_type_class_add_private (object_class, sizeof (GamesFramePrivate));

  g_object_class_install_property
    (object_class,
     PROP_LABEL,
     g_param_spec_string ("label", NULL, NULL,
                          NULL,
                          G_PARAM_WRITABLE |
                          G_PARAM_STATIC_NAME |
                          G_PARAM_STATIC_NICK |
                          G_PARAM_STATIC_BLURB));
}

/**
 * games_frame_new:
 * @label: the frame's title, or %NULL
 *
 * Returns: a new #GamesFrame
 **/
GtkWidget *
games_frame_new (const char * label)
{
  return g_object_new (GAMES_TYPE_FRAME,
                       "label", label,
                       NULL);
}

/**
 * games_frame_set_label:
 * @frame:
 * @label:
 *
 * Sets @frame's title label.
 */
void
games_frame_set_label (GamesFrame *frame,
                       const char *label)
{
  g_return_if_fail (GAMES_IS_FRAME (frame));

  if (label) {
    gtk_label_set_text (GTK_LABEL (frame->priv->label), label);
  } else {
    gtk_label_set_text (GTK_LABEL (frame->priv->label), "");
  }

  g_object_set (frame->priv->label, "visible", label && label[0], NULL);

  g_object_notify (G_OBJECT (frame), "label");
}
