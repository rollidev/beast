/* GXK - Gtk+ Extension Kit
 * Copyright (C) 1998-2002 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "gxkutils.h"


/* --- generated marshallers --- */
#include "gxkmarshal.c"


/* --- generated type IDs and enums --- */
#include "gxkgentypes.c"


/* --- functions --- */
void
_gxk_init_utils (void)
{
  gxk_type_register_generated (G_N_ELEMENTS (generated_type_entries), generated_type_entries);
}

/**
 * gxk_type_register_generated
 * @n_entries: number of generated types to register
 * @entries:   GxkTypeGenerated type descriptions
 *
 * Register each of the generated type entries with the
 * type system. Currently supported parent types are
 * %G_TYPE_ENUM and %G_TYPE_FLAGS in which cases the
 * @type_data member must point to a %NULL terminated
 * array of GEnumValue or GFlagValue structures. No
 * extra copying is performed, so the passed in
 * structures have to persist throughout runtime.
 */
void
gxk_type_register_generated (guint                   n_entries,
			     const GxkTypeGenerated *entries)
{
  guint i;

  for (i = 0; i < n_entries; i++)
    {
      GType type_id;
      switch (entries[i].parent)
	{
	case G_TYPE_ENUM:
	  type_id = g_enum_register_static (entries[i].type_name, entries[i].type_data);
	  break;
	case G_TYPE_FLAGS:
	  type_id = g_flags_register_static (entries[i].type_name, entries[i].type_data);
	  break;
	default:
	  g_warning ("%s: unsupported parent type `%s'", G_STRLOC, g_type_name (entries[i].parent));
	  type_id = 0;
	  break;
	}
      *entries[i].type_id = type_id;
    }
}

/**
 * g_object_set_long
 * @object: a valid GObject
 * @name:   name of the long value to set
 * @v_long: the actual value
 *
 * Convenience variant of g_object_set_data() to set
 * a long instead of a pointer.
 */
void
g_object_set_long (gpointer     object,
		   const gchar *name,
		   glong        v_long)
{
  g_return_if_fail (G_IS_OBJECT (object));

  g_object_set_data (object, name, (gpointer) v_long);
}

/**
 * g_object_get_long
 * @object:  a valid GObject
 * @name:    name of the long value to retrive
 * @RETURNS: the actual value
 *
 * Convenience variant of g_object_get_data() to retrive
 * a long instead of a pointer.
 */
glong
g_object_get_long (gpointer     object,
		   const gchar *name)
{
  g_return_val_if_fail (G_IS_OBJECT (object), 0);

  return (glong) g_object_get_data (object, name);
}

/**
 * gxk_widget_make_insensitive
 * @widget: a valid GtkWidget
 *
 * This function is euqivalent to gtk_widget_set_sensitive (@widget, #FALSE);
 * It exists as a convenient signal connection callback.
 */
void
gxk_widget_make_insensitive (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gtk_widget_set_sensitive (widget, FALSE);
}

/**
 * gxk_widget_make_sensitive
 * @widget: a valid GtkWidget
 *
 * This function is euqivalent to gtk_widget_set_sensitive (@widget, #TRUE);
 * It exists as a convenient signal connection callback.
 */
void
gxk_widget_make_sensitive (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gtk_widget_set_sensitive (widget, TRUE);
}

static void
style_modify_fg_as_sensitive (GtkWidget *widget)
{
  GtkRcStyle *rc_style = gtk_rc_style_new ();

  rc_style->color_flags[GTK_STATE_INSENSITIVE] = GTK_RC_FG;
  rc_style->fg[GTK_STATE_INSENSITIVE].red = widget->style->fg[GTK_STATE_NORMAL].red;
  rc_style->fg[GTK_STATE_INSENSITIVE].green = widget->style->fg[GTK_STATE_NORMAL].green;
  rc_style->fg[GTK_STATE_INSENSITIVE].blue = widget->style->fg[GTK_STATE_NORMAL].blue;
  gtk_widget_modify_style (widget, rc_style);
}

/**
 * gxk_widget_modify_as_title
 * @widget: a valid GtkWidget
 *
 * Modify the widget and it's style, so that it is insensitive,
 * but doesn't quite look that way. Usefull for inactive title
 * menu items in menus.
 */
void
gxk_widget_modify_as_title (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gtk_widget_set_sensitive (widget, FALSE);
  if (GTK_WIDGET_REALIZED (widget))
    style_modify_fg_as_sensitive (widget);
  if (!gxk_signal_handler_pending (widget, "realize", G_CALLBACK (style_modify_fg_as_sensitive), NULL))
    g_signal_connect_after (widget, "realize", G_CALLBACK (style_modify_fg_as_sensitive), NULL);
  if (!gxk_signal_handler_pending (widget, "realize", G_CALLBACK (gxk_widget_make_insensitive), NULL))
    g_signal_connect_after (widget, "realize", G_CALLBACK (gxk_widget_make_insensitive), NULL);
}

static void
style_modify_bg_as_base (GtkWidget *widget)
{
  GtkRcStyle *rc_style = gtk_widget_get_modifier_style (widget);

  rc_style->color_flags[GTK_STATE_NORMAL] = GTK_RC_BG;
  rc_style->bg[GTK_STATE_NORMAL].red = widget->style->base[GTK_STATE_NORMAL].red;
  rc_style->bg[GTK_STATE_NORMAL].green = widget->style->base[GTK_STATE_NORMAL].green;
  rc_style->bg[GTK_STATE_NORMAL].blue = widget->style->base[GTK_STATE_NORMAL].blue;
  gtk_widget_modify_style (widget, rc_style);
}

/**
 * gxk_widget_modify_bg_as_base
 * @widget: a valid GtkWidget
 *
 * Modify the widget's background to look like the background
 * of a text or list widget (usually white). This is usefull
 * if a hbox or similar widget is used to "simulate" a list,
 * text, or similar widget.
 */
void
gxk_widget_modify_bg_as_base (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (GTK_WIDGET_REALIZED (widget))
    style_modify_bg_as_base (widget);
  if (!gxk_signal_handler_pending (widget, "realize", G_CALLBACK (style_modify_bg_as_base), NULL))
    g_signal_connect_after (widget, "realize", G_CALLBACK (style_modify_bg_as_base), NULL);
}

static void
style_modify_base_as_bg (GtkWidget *widget)
{
  GtkRcStyle *rc_style = gtk_rc_style_new ();

  rc_style->color_flags[GTK_STATE_NORMAL] = GTK_RC_BASE;
  rc_style->base[GTK_STATE_NORMAL].red = widget->style->bg[GTK_STATE_NORMAL].red;
  rc_style->base[GTK_STATE_NORMAL].green = widget->style->bg[GTK_STATE_NORMAL].green;
  rc_style->base[GTK_STATE_NORMAL].blue = widget->style->bg[GTK_STATE_NORMAL].blue;
  gtk_widget_modify_style (widget, rc_style);
}

/**
 * gxk_widget_modify_base_as_bg
 * @widget: a valid GtkWidget
 *
 * Modify the widget's base background (used by list and
 * text widgets) to look like an ordinary widget background.
 * This is usefull if a list, text or similar widget shouldn't
 * stand out as such, e.g. when the GtkTextView widget displaying
 * a long non-editable text should look similar to a GtkLabel.
 */
void
gxk_widget_modify_base_as_bg (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (GTK_WIDGET_REALIZED (widget))
    style_modify_base_as_bg (widget);
  if (!gxk_signal_handler_pending (widget, "realize", G_CALLBACK (style_modify_base_as_bg), NULL))
    g_signal_connect_after (widget, "realize", G_CALLBACK (style_modify_base_as_bg), NULL);
}

static gboolean
expose_bg_clear (GtkWidget      *widget,
		 GdkEventExpose *event)
{
  gtk_paint_flat_box (widget->style, widget->window, GTK_STATE_NORMAL,
		      GTK_SHADOW_NONE, &event->area, widget, "base", 0, 0, -1, -1);

  return FALSE;
}

/**
 * gxk_widget_force_bg_clear
 * @widget: a valid GtkWidget
 *
 * Enforce drawing of a widget's background.
 * Some widgets do not explicitely draw their background,
 * but simply draw themsleves on top of their parent's
 * background. This function forces the widget into
 * drawing its background according to its style settings.
 */
void
gxk_widget_force_bg_clear (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gtk_widget_set_redraw_on_allocate (widget, TRUE);
  if (!gxk_signal_handler_pending (widget, "expose_event", G_CALLBACK (expose_bg_clear), NULL))
    g_signal_connect (widget, "expose_event", G_CALLBACK (expose_bg_clear), NULL);
}

/**
 * gxk_signal_handler_pending
 * @instance:        object instance with signals
 * @detailed_signal: signal name
 * @callback:        custom callback function
 * @data:            callback data
 * @RETURNS:         whether callback is connected
 *
 * Find out whether a specific @callback is pending for a
 * specific signal on an instance. %TRUE is returned if
 * the @callback is found, %FALSE otherwise.
 */
gboolean
gxk_signal_handler_pending (gpointer     instance,
			    const gchar *detailed_signal,
			    GCallback    callback,
			    gpointer     data)
{
  guint signal_id;
  GQuark detail = 0;

  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE (instance), FALSE);
  g_return_val_if_fail (detailed_signal != NULL, FALSE);
  g_return_val_if_fail (callback != NULL, FALSE);

  if (g_signal_parse_name (detailed_signal, G_TYPE_FROM_INSTANCE (instance),
			   &signal_id, &detail, FALSE))
    {
      if (g_signal_handler_find (instance, (G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA |
					    (detail ? G_SIGNAL_MATCH_DETAIL : 0)),
				 signal_id, detail, NULL, callback, data) != 0)
	return TRUE;
    }
  else
    g_warning ("%s: signal name \"%s\" is invalid for instance `%p'", G_STRLOC, detailed_signal, instance);
  return FALSE;
}
