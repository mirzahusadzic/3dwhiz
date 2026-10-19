#ifndef GTK_RTG_MARSHALERS
#define GTK_RTG_MARSHALERS

/*! \file
 */

G_BEGIN_DECLS

#include <glib-object.h>


/* VOID:OBJECT,OBJECT (marshalers.list:1) */
extern void gtkrtg_marshal__VOID__OBJECT_OBJECT(GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);

/* BOOLEAN:VOID (marshalers.list:2) */
void
gtkrtg_marshal__BOOLEAN__VOID(GClosure     *closure,
                              GValue       *return_value G_GNUC_UNUSED,
                              guint         n_param_values,
                              const GValue *param_values,
                              gpointer      invocation_hint G_GNUC_UNUSED,
                              gpointer      marshal_data);


/* VOID:UINT,UINT (marshalers.list:3) */
void
gtkrtg_marshal__VOID__UINT_UINT (GClosure     *closure,
				 GValue       *return_value,
				 guint         n_param_values,
				 const GValue *param_values,
				 gpointer      invocation_hint,
				 gpointer      marshal_data);


/* VOID:INT,INT (marshalers.list:4) */
void
gtkrtg_marshal__VOID__INT_INT (GClosure     *closure,
			       GValue       *return_value,
			       guint         n_param_values,
			       const GValue *param_values,
			       gpointer      invocation_hint,
			       gpointer      marshal_data);


G_END_DECLS

#endif /* GTK_RTG_MARSHALERS */
