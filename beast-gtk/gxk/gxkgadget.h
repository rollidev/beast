/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2003 Tim Janik
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
#ifndef __GXK_GADGET_H__
#define __GXK_GADGET_H__

#include "gxkutils.h"

G_BEGIN_DECLS

/* --- structures --- */
#define GXK_GADGET    G_OBJECT
#define GXK_IS_GADGET G_IS_OBJECT
typedef void          GxkGadget;
typedef struct _GxkGadgetOpt GxkGadgetOpt;


/* --- gadget functions --- */
void            gxk_gadget_parse        (const gchar    *domain_name,
                                         const gchar    *file_name,
                                         GError        **error);
void            gxk_gadget_parse_text   (const gchar    *domain_name,
                                         const gchar    *text,
                                         gint            text_len,
                                         GError        **error);
GxkGadgetOpt*   gxk_gadget_const_options(void);
GxkGadgetOpt*   gxk_gadget_options      (const gchar    *name1,
                                         ...);
GxkGadgetOpt*   gxk_gadget_options_set  (GxkGadgetOpt   *opt,
                                         const gchar    *name,
                                         const gchar    *value);
const gchar*    gxk_gadget_options_get  (GxkGadgetOpt   *opt,
                                         const gchar    *name);
GxkGadgetOpt*   gxk_gadget_options_copy (GxkGadgetOpt   *opt);
void            gxk_gadget_free_options (GxkGadgetOpt   *opt);
GxkGadget*      gxk_gadget_create       (const gchar    *domain_name,
                                         const gchar    *name,
                                         GxkGadgetOpt   *options);
GxkGadget*      gxk_gadget_complete     (GxkGadget      *gadget,
                                         const gchar    *domain_name,
                                         const gchar    *name,
                                         GxkGadgetOpt   *options);
GxkGadget*      gxk_gadget_create_add   (const gchar    *domain_name,
                                         const gchar    *name,
                                         GxkGadget      *parent,
                                         GxkGadgetOpt   *options);
void            gxk_gadget_add          (GxkGadget      *gadget,
                                         const gchar    *region,
                                         gpointer        widget);
const gchar*    gxk_gadget_get_domain   (GxkGadget      *gadget);
gpointer        gxk_gadget_find         (GxkGadget      *gadget,
                                         const gchar    *region);
void            gxk_gadget_sensitize    (GxkGadget      *gadget,
                                         const gchar    *region,
                                         gboolean        sensitive);


/* --- gadget types --- */
typedef struct {
  GxkGadget*  (*create)    (GType         type,
                            const gchar  *name);
  GParamSpec* (*find_prop) (GxkGadget    *gadget,
                            const gchar  *prop_name);
  void        (*set_prop)  (GxkGadget    *gadget,
                            const gchar  *prop_name,
                            const GValue *value);
  void        (*adopt)     (GxkGadget    *gadget,
                            GxkGadget    *parent);
  GParamSpec* (*find_pack) (GxkGadget    *gadget,
                            const gchar  *pack_name);
  void        (*set_pack)  (GxkGadget    *gadget,
                            const gchar  *pack_name,
                            const GValue *value);
} GxkGadgetType;
void      gxk_gadget_define_widget_type   (GType                type);
void      gxk_gadget_define_type          (GType                type,
                                           const GxkGadgetType *ggtype);
gboolean  gxk_gadget_type_lookup          (GType                type,
                                           GxkGadgetType       *ggtype);


G_END_DECLS

#endif /* __GXK_GADGET_H__ */
