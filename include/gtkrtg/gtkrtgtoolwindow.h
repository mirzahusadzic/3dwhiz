#ifndef GTK_RTG_TOOL_WINDOW_H
#define GTK_RTG_TOOL_WINDOW_H

#include <gtkrtg/gtkrtgbase.h>
#include <gtkrtg/gtkrtgexception.h>

/*! \file */

G_BEGIN_DECLS

#define GTK_RTG_TYPE_TOOL_WINDOW            (gtk_rtg_tool_window_get_type ())
#define GTK_RTG_TOOL_WINDOW(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), GTK_RTG_TYPE_TOOL_WINDOW, GtkRtgToolWindow))
#define GTK_RTG_TOOL_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),	   GTK_RTG_TYPE_TOOL_WINDOW, GtkRtgToolWindowClass))
#define GTK_RTG_IS_TOOL_WINDOW(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), GTK_RTG_TYPE_TOOL_WINDOW))
#define GTK_RTG_IS_TOOL_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),	   GTK_RTG_TYPE_TOOL_WINDOW))
#define GTK_RTG_TOOL_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),	   GTK_RTG_TYPE_TOOL_WINDOW, GtkRtgToolWindowClass))

typedef struct _GtkRtgToolWindow        GtkRtgToolWindow;
typedef struct _GtkRtgToolWindowClass   GtkRtgToolWindowClass;
typedef struct _GtkRtgToolWindowPrivate GtkRtgToolWindowPrivate;

struct _GtkRtgToolWindow
{
	GtkWindow	         	 parent;
    GtkRtgToolWindowPrivate *priv;
};

struct _GtkRtgToolWindowClass
{
	GtkWindowClass parent_class;
};

GType gtk_rtg_tool_window_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_rtg_tool_window_new (GtkWindow *transient=NULL);

G_END_DECLS

#endif /* GTK_RTG_TOOL_WINDOW_H */
