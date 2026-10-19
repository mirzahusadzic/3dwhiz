#ifndef GTK_RTG_GIZMO_H
#define GTK_RTG_GIZMO_H

#include <gtkrtg/gtkrtgbase.h>
#include <gtkrtg/gtkrtgmath.h>
#include <gtkrtg/gtkrtgcanvas.h>
#include <gtkrtg/gtkrtgcamera.h>
#include "gtkrtgresources.h"
#include "gtkrtgextras.h"
#include "gtkrtgtripod.h"
#include "gtkrtgcolorid.h"



#define GTK_RTG_TYPE_GIZMO            (gtk_rtg_gizmo_get_type())
#define GTK_RTG_GIZMO(obj)		      (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_RTG_TYPE_GIZMO, GtkRtgGizmo))
#define GTK_RTG_GIZMO_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass),  GTK_RTG_TYPE_GIZMO, GtkRtgGizmoClass))
#define GTK_RTG_IS_GIZMO(obj)		  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_RTG_TYPE_GIZMO))
#define GTK_RTG_IS_GIZMO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GTK_RTG_TYPE_GIZMO))
#define GTK_RTG_GIZMO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GTK_RTG_TYPE_GIZMO, GtkRtgGizmoClass))

typedef struct _GtkRtgGizmo		 GtkRtgGizmo;
typedef struct _GtkRtgGizmoClass GtkRtgGizmoClass;

enum
{
	PLANE_XY,
	PLANE_XZ,
	PLANE_YZ,
	GIZMO_PLANES
};

enum
{
	SELECTED_PLANE_XY  = SELECTED_AX_X | SELECTED_AX_Y,
	SELECTED_PLANE_XZ  = SELECTED_AX_X | SELECTED_AX_Z,
	SELECTED_PLANE_YZ  = SELECTED_AX_Y | SELECTED_AX_Z,
	SELECTED_XYZ	   = SELECTED_AX_X | SELECTED_AX_Y | SELECTED_AX_Z
};

#define GIZMO_ARROW_VERTS	5
enum{ COLOR_NORMAL, COLOR_DARKER, COLOR_DARKEST, COLORS };
typedef std::multimap<gfloat, guint, floatCmp>   m_arrowPolyType;

typedef struct
{
	GdkPoint arrow2d[GIZMO_ARROW_VERTS];
	GdkColor c_arrow[COLORS];
	m_arrowPolyType *m_apoly;
	GdkPoint text_pos;
}GizmoAx;

struct _GtkRtgGizmo
{
	GtkRtgTripod parent;
	GizmoAx ax[3];
	guint arrow:1;
	gint viewport[4];
	gtkrtg::point3f text_pos_ortho[AXES];
	gtkrtg::point3f text_pos_persp[AXES];

	/* axis setup */
	gtkrtg::point3f point_ax3d_persp[AXES][AXIS_POINTS];
    gtkrtg::point3f point_ax3d_ortho[AXES][AXIS_POINTS];
	gfloat color_plane[4];

	/* camera to offset vector */
    gtkrtg::vector3f cam_offset;
    gfloat cam_offset_angle;

	/* persp */
    gfloat persp_scale;

	/* ortho */
	gtkrtg::matrix4x4f matrixproject;
	gint size;
	gfloat alphabeta;
	gint xt;
	gint yt;

	/* inverse */
	gtkrtg::matrix4x4f minverse;
	guint intersect:1;
	GdkPoint point;
};

struct _GtkRtgGizmoClass
{
	GtkRtgTripodClass parent_class;
};

GType gtk_rtg_gizmo_get_type(void) G_GNUC_CONST;
GtkRtgGizmo *gtk_rtg_gizmo_new(GtkRtgCanvas *canvas, gchar *font_desc);

#endif /* GTK_RTG_GIZMO_H */
