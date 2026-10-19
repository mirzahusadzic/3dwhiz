#ifndef GTK_RTG_TOOLBAR_H
#define GTK_RTG_TOOLBAR_H

#include <gtkrtg/gtkrtgbase.h>
#include <gtkrtg/gtkrtgexception.h>
#include <gtkrtg/gtkrtgcanvas.h>

/*! \file */

G_BEGIN_DECLS

#define GTK_RTG_TYPE_TOOLBAR			(gtk_rtg_toolbar_get_type())
#define GTK_RTG_TOOLBAR(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_RTG_TYPE_TOOLBAR, GtkRtgToolbar))
#define GTK_RTG_TOOLBAR_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass),  GTK_RTG_TYPE_TOOLBAR, GtkRtgToolbarClass))
#define GTK_RTG_IS_TOOLBAR(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_RTG_TYPE_TOOLBAR))
#define GTK_RTG_IS_TOOLBAR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass),  GTK_RTG_TYPE_TOOLBAR))
#define GTK_RTG_TOOLBAR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),  GTK_RTG_TYPE_TOOLBAR, GtkRtgToolbarClass))

typedef struct _GtkRtgToolbar		 GtkRtgToolbar;
typedef struct _GtkRtgToolbarClass	 GtkRtgToolbarClass;
typedef struct _GtkRtgToolbarPrivate GtkRtgToolbarPrivate;

struct _GtkRtgToolbar
{
	GtkToolbar		 	  parent;
	GtkRtgToolbarPrivate *priv;
};

struct _GtkRtgToolbarClass
{
	GtkToolbarClass parent_class;

	void (* maximize_viewport) (GtkRtgToolbar *tb, GtkRtgCanvas *canvas);
    void (* refocus)           (GtkRtgToolbar *tb);
};

GType gtk_rtg_toolbar_get_type(void) G_GNUC_CONST;
GtkWidget *gtk_rtg_toolbar_new();
void gtk_rtg_toolbar_canvas_push(GtkRtgToolbar *tb, GtkRtgCanvas *canvas, ...) throw (GtkRtgException);
gboolean gtk_rtg_toolbar_canvas_is_pushed(GtkRtgToolbar *tb, GtkRtgCanvas *canvas) throw (GtkRtgException);
void gtk_rtg_toolbar_canvas_pop(GtkRtgToolbar *tb, GtkRtgCanvas *canvas) throw (GtkRtgException);

G_END_DECLS

#endif /* GTK_RTG_TOOLBAR_H */
