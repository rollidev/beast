// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstdial.hh"
#include "bstknob.hh"

/* --- scale-alike parameter editor --- */
enum {
  PARAM_SCALE_DIAL,
  PARAM_SCALE_KNOB,
  PARAM_SCALE_LOGARITHMIC       = 0x10000
};

static GtkWidget*
param_scale_create (GxkParam    *param,
                    const gchar *tooltip,
                    guint        variant)
{
  GtkWidget *widget = NULL;
  guint svariant = variant & 0xffff;
  guint logarithmic = variant & PARAM_SCALE_LOGARITHMIC;
  GtkAdjustment *adjustment = logarithmic ? gxk_param_get_log_adjustment (param) : NULL;
  if (!logarithmic &&
      (g_param_spec_check_option (param->pspec, "db-volume") ||
       g_param_spec_check_option (param->pspec, "db-range")) &&
      !g_param_spec_check_option (param->pspec, "db-value"))
    adjustment = gxk_param_get_decibel_adjustment (param);
  if (!adjustment)
    adjustment = gxk_param_get_adjustment (param);

  switch (svariant)
    {
    case PARAM_SCALE_DIAL:
      widget = (GtkWidget*) g_object_new (BST_TYPE_DIAL, NULL);
      bst_dial_set_adjustment (BST_DIAL (widget), adjustment);
      break;
    case PARAM_SCALE_KNOB:
      widget = (GtkWidget*) g_object_new (BST_TYPE_KNOB, NULL);
      bst_knob_set_adjustment (BST_KNOB (widget), adjustment);
      break;
    }
  g_object_set (widget,
		"visible", TRUE,
		"can_focus", FALSE,
		NULL);
  gxk_param_add_grab_widget (param, widget);
  gxk_widget_set_tooltip (widget, tooltip);
  return widget;
}

static GxkParamEditor param_scale1 = {
  { "knob-lin",         N_("Knob"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { NULL,         +5,   TRUE, },        /* options, rating, editing */
  param_scale_create,   NULL,   PARAM_SCALE_KNOB,
};
static GxkParamEditor param_scale2 = {
  { "knob-log",         N_("Knob (Logarithmic)"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { "log-scale",  +5,   TRUE, },        /* options, rating, editing */
  param_scale_create,   NULL,   PARAM_SCALE_KNOB | PARAM_SCALE_LOGARITHMIC,
};
static const gchar *param_scale_aliases1[] = {
  "knob",
  "knob-lin", "knob-log",
  NULL,
};

static GxkParamEditor param_scale3 = {
  { "dial-lin",         N_("Dial"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { NULL,         +5,   TRUE, },        /* options, rating, editing */
  param_scale_create,   NULL,   PARAM_SCALE_DIAL,
};
static GxkParamEditor param_scale4 = {
  { "dial-log",         N_("Dial (Logarithmic)"), },
  { G_TYPE_NONE,  NULL, TRUE, TRUE, },  /* all int types and all float types */
  { "log-scale",  +5,   TRUE, },        /* options, rating, editing */
  param_scale_create,   NULL,   PARAM_SCALE_DIAL | PARAM_SCALE_LOGARITHMIC,
};
static const gchar *param_scale_aliases2[] = {
  "dial",
  "dial-lin", "dial-log",
  NULL,
};
