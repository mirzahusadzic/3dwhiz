#ifndef GTK_RTG_FOUR_PANED_H
#define GTK_RTG_FOUR_PANED_H

#include <gtkrtg/gtkrtgbase.h>
#include <gtkrtg/gtkrtgexception.h>

/*! \file */

G_BEGIN_DECLS

#define GTK_RTG_TYPE_FOUR_PANED			   (gtk_rtg_four_paned_get_type())
#define GTK_RTG_FOUR_PANED(obj)		       (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_RTG_TYPE_FOUR_PANED, GtkRtgFourPaned))
#define GTK_RTG_FOUR_PANED_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass),  GTK_RTG_TYPE_FOUR_PANED, GtkRtgFourPanedClass))
#define GTK_RTG_IS_FOUR_PANED(obj)		   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_RTG_TYPE_FOUR_PANED))
#define GTK_RTG_IS_FOUR_PANED_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GTK_RTG_TYPE_FOUR_PANED))
#define GTK_RTG_FOUR_PANED_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GTK_RTG_TYPE_FOUR_PANED, GtkRtgFourPanedClass))

typedef struct _GtkRtgFourPaned		GtkRtgFourPaned;
typedef struct _GtkRtgFourPanedClass	GtkRtgFourPanedClass;
typedef struct _GtkRtgFourPanedPrivate	GtkRtgFourPanedPrivate;

struct _GtkRtgFourPaned
{
	GtkContainer             parent;
	GtkRtgFourPanedPrivate	*priv;
};


struct _GtkRtgFourPanedClass
{
	GtkContainerClass       parent_class;

	gboolean (* toggle_handle_focus)(GtkRtgFourPaned *);
};

GType	    gtk_rtg_four_paned_get_type(void) G_GNUC_CONST;
GtkWidget  *gtk_rtg_four_paned_new();
void	    gtk_rtg_four_paned_pack1(GtkRtgFourPaned *paned, GtkWidget *child, gboolean resize=TRUE, gboolean shrink=FALSE) throw (GtkRtgException);
void	    gtk_rtg_four_paned_pack2(GtkRtgFourPaned *paned, GtkWidget *child, gboolean resize=TRUE, gboolean shrink=FALSE) throw (GtkRtgException);
void	    gtk_rtg_four_paned_pack3(GtkRtgFourPaned *paned, GtkWidget *child, gboolean resize=TRUE, gboolean shrink=FALSE) throw (GtkRtgException);
void	    gtk_rtg_four_paned_pack4(GtkRtgFourPaned *paned, GtkWidget *child, gboolean resize=TRUE, gboolean shrink=FALSE) throw (GtkRtgException);
void	    gtk_rtg_four_paned_macro_pane(GtkRtgFourPaned *paned, GtkWidget *child) throw (GtkRtgException);
gboolean    gtk_rtg_four_paned_is_macro_pane(GtkRtgFourPaned *paned) throw (GtkRtgException);
void	    gtk_rtg_four_paned_restore_panes(GtkRtgFourPaned *paned) throw (GtkRtgException);

G_END_DECLS

#endif /* GTK_RTG_FOUR_PANED_H */
