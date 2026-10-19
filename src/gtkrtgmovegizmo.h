#ifndef GTK_RTG_MOVE_GIZMO_H
#define GTK_RTG_MOVE_GIZMO_H

#include "gtkrtggizmo.h"

#define GTK_RTG_TYPE_MOVE_GIZMO            (gtk_rtg_move_gizmo_get_type())
#define GTK_RTG_MOVE_GIZMO(obj)		       (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_RTG_TYPE_MOVE_GIZMO, GtkRtgMoveGizmo))
#define GTK_RTG_MOVE_GIZMO_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass),  GTK_RTG_TYPE_MOVE_GIZMO, GtkRtgMoveGizmoClass))
#define GTK_RTG_IS_MOVE_GIZMO(obj)		   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_RTG_TYPE_MOVE_GIZMO))
#define GTK_RTG_IS_MOVE_GIZMO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GTK_RTG_TYPE_MOVE_GIZMO))
#define GTK_RTG_MOVE_GIZMO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GTK_RTG_TYPE_MOVE_GIZMO, GtkRtgMoveGizmoClass))

typedef struct _GtkRtgMoveGizmo		 GtkRtgMoveGizmo;
typedef struct _GtkRtgMoveGizmoClass GtkRtgMoveGizmoClass;

#define GIZMO_PLANE_VERTS 7

struct _GtkRtgMoveGizmo
{
	GtkRtgGizmo	parent;
	GdkPoint	vec_plane2d[GIZMO_PLANE_VERTS];
};

struct _GtkRtgMoveGizmoClass
{
	GtkRtgGizmoClass parent_class;
};

GType gtk_rtg_move_gizmo_get_type(void) G_GNUC_CONST;
GtkRtgMoveGizmo *gtk_rtg_move_gizmo_new(GtkRtgCanvas *canvas, gchar *font_desc);

#endif /* GTK_RTG_MOVE_GIZMO_H */
