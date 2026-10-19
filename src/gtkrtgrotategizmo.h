#ifndef GTK_RTG_ROTATE_GIZMO_H
#define GTK_RTG_ROTATE_GIZMO_H

#include "gtkrtggizmo.h"



#define GTK_RTG_TYPE_ROTATE_GIZMO            (gtk_rtg_rotate_gizmo_get_type())
#define GTK_RTG_ROTATE_GIZMO(obj)			 (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_RTG_TYPE_ROTATE_GIZMO, GtkRtgRotateGizmo))
#define GTK_RTG_ROTATE_GIZMO_CLASS(klass)	 (G_TYPE_CHECK_CLASS_CAST ((klass),  GTK_RTG_TYPE_ROTATE_GIZMO, GtkRtgRotateGizmoClass))
#define GTK_RTG_IS_ROTATE_GIZMO(obj)		 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_RTG_TYPE_ROTATE_GIZMO))
#define GTK_RTG_IS_ROTATE_GIZMO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GTK_RTG_TYPE_ROTATE_GIZMO))
#define GTK_RTG_ROTATE_GIZMO_GET_CLASS(obj)	 (G_TYPE_INSTANCE_GET_CLASS ((obj),  GTK_RTG_TYPE_ROTATE_GIZMO, GtkRtgRotateGizmoClass))

typedef struct _GtkRtgRotateGizmo	   GtkRtgRotateGizmo;
typedef struct _GtkRtgRotateGizmoClass GtkRtgRotateGizmoClass;

#define CIRCLE_VERTS 32
typedef struct
{
	GdkPoint point_2d;
	guint front:1;

}GizmoPPoint;

struct _GtkRtgRotateGizmo
{
	GtkRtgGizmo	parent;
	GizmoPPoint	circle_plane[GIZMO_PLANES][CIRCLE_VERTS]; /* clipped plane circles */
	GdkPoint circle_outline[CIRCLE_VERTS]; /* perpendicular to the camera / viewer */
	GdkPoint circle_outer[CIRCLE_VERTS]; /* perpendicular to the camera / viewer */
	GdkColor ax_dimmed_color;
	GdkColor ax_outer_color;
	GdkColor *colors[AXES];

	gtkrtg::quat_f quat;
	gtkrtg::matrix4x4f mcam2world;
	GdkPoint center;

	/* rotation point on circle */
	GdkPoint pc;

	/* flags */
	guint outline:1;
	gboolean backline[AXES];
};

struct _GtkRtgRotateGizmoClass
{
	GtkRtgGizmoClass parent_class;
};

GType gtk_rtg_rotate_gizmo_get_type(void) G_GNUC_CONST;
GtkRtgRotateGizmo *gtk_rtg_rotate_gizmo_new(GtkRtgCanvas *canvas, gchar *font_desc);

#endif /* GTK_RTG_ROTATE_GIZMO_H */
