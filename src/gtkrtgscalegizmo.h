#ifndef GTK_RTG_SCALE_GIZMO_H
#define GTK_RTG_SCALE_GIZMO_H

#include "gtkrtggizmo.h"

#define GTK_RTG_TYPE_SCALE_GIZMO            (gtk_rtg_scale_gizmo_get_type())
#define GTK_RTG_SCALE_GIZMO(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_RTG_TYPE_SCALE_GIZMO, GtkRtgScaleGizmo))
#define GTK_RTG_SCALE_GIZMO_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass),  GTK_RTG_TYPE_SCALE_GIZMO, GtkRtgScaleGizmoClass))
#define GTK_RTG_IS_SCALE_GIZMO(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_RTG_TYPE_SCALE_GIZMO))
#define GTK_RTG_IS_SCALE_GIZMO_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass),  GTK_RTG_TYPE_SCALE_GIZMO))
#define GTK_RTG_SCALE_GIZMO_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),  GTK_RTG_TYPE_SCALE_GIZMO, GtkRtgScaleGizmoClass))

typedef struct _GtkRtgScaleGizmo	  GtkRtgScaleGizmo;
typedef struct _GtkRtgScaleGizmoClass GtkRtgScaleGizmoClass;

#define SCALE_POLY_VERTS 15

typedef struct { gtkrtg::point3f p[SCALE_POLY_VERTS]; }ScalePoly;

struct _GtkRtgScaleGizmo
{
	GtkRtgGizmo parent;
	GdkPoint vec_poly2d[GIZMO_PLANES][SCALE_POLY_VERTS];
    ScalePoly spoly_persp[GIZMO_PLANES];
    gtkrtg::point3f point_ax3d_offscreen_persp[AXES][AXIS_POINTS];
	gfloat ax_mirror[AXES];
};

struct _GtkRtgScaleGizmoClass
{
	GtkRtgGizmoClass parent_class;
};

GType gtk_rtg_scale_gizmo_get_type(void) G_GNUC_CONST;
GtkRtgScaleGizmo *gtk_rtg_scale_gizmo_new(GtkRtgCanvas *canvas, gchar *font_desc);

#endif /* GTK_RTG_SCALE_GIZMO_H */
