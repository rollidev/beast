/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-1999, 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_PROCEDURE_H__
#define __BSE_PROCEDURE_H__

#include	<bse/bseparam.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- BSE type macros --- */
#define	BSE_PROCEDURE_TYPE(proc)	(G_TYPE_FROM_CLASS (proc))
#define	BSE_IS_PROCEDURE_CLASS(proc)	(G_TYPE_CHECK_CLASS_TYPE ((proc), BSE_TYPE_PROCEDURE))


/* --- limits --- */
#define	BSE_PROCEDURE_MAX_IN_PARAMS	(16)
#define	BSE_PROCEDURE_MAX_OUT_PARAMS	(16)


/* --- BseProcedureClass --- */
typedef BseErrorType  (*BseProcedureExec)    (BseProcedureClass *procedure,
					      GValue		*in_values,
					      GValue		*out_values);
struct _BseProcedureClass
{
  GTypeClass      bse_class;
  gchar          *name;
  gchar          *blurb;
  guint           private_id;

  /* setup upon init */
  gchar          *help;
  gchar          *author;
  gchar          *copyright;
  gchar          *date; /* copyright date */
  
  /* implementation */
  guint           n_in_pspecs;
  GParamSpec	**in_pspecs;
  guint           n_out_pspecs;
  GParamSpec	**out_pspecs;

  BseProcedureExec execute;
};


/* --- notifiers --- */
typedef gboolean (*BseProcedureNotify) (gpointer     func_data,
					const gchar *proc_name,
					BseErrorType exit_status);
typedef BseErrorType (*BseProcedureMarshal) (BseProcedureClass *proc,
					     const GValue      *ivalues,
					     GValue	       *ovalues,
					     gpointer	        marshal_data);


/* --- prototypes --- */
/* execute procedure, passing n_in_pspecs param values for in
 * values and n_out_pspecs param value locations for out values
 */
BseErrorType bse_procedure_exec	  	  (const gchar		*proc_name,
					   ...);
GType	     bse_procedure_lookup	  (const gchar		*proc_name);
BseErrorType bse_procedure_marshal_valist (GType		 proc_type,
					   const GValue		*first_value,
					   BseProcedureMarshal	 marshal,
					   gpointer		 marshal_data,
					   gboolean		 skip_ovalues,
					   va_list		 var_args);
BseErrorType bse_procedure_marshal        (GType		 proc_type,
					   const GValue		*ivalues,
					   GValue		*ovalues,
					   BseProcedureMarshal	 marshal,
					   gpointer		 marshal_data);
BseErrorType bse_procedure_execvl	  (BseProcedureClass	*proc,
					   GSList		*in_value_list,
					   GSList		*out_value_list);
/* functions to call from very time consuming procedures to keep the
 * main program (and playback) alive.
 * "progress"    - value in the range from 0...1 to indicate how far
 *                 the procedure has proceeded yet (*100 = %).
 *		   values <0 indidicate progress of unknown amount.
 * return value  - if the return value is TRUE, the procedure is requested
 *                 to abort, and should return BSE_ERROR_PROC_ABORT
 * FIXME: bse_procedure_status() has to become bse_server_script_status()
 */
gboolean bse_procedure_status		(BseProcedureClass	*proc,
					 gfloat			 progress);


/* --- internal --- */
const gchar* bse_procedure_type_register (const gchar		*name,
					  const gchar		*blurb,
					  BsePlugin		*plugin,
					  GType  		*ret_type);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PROCEDURE_H__ */
