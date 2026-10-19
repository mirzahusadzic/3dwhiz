#ifndef GTK_RTG_TRIPOD_H
#define GTK_RTG_TRIPOD_H

#include <gtkrtg/gtkrtgbase.h>
#include <gtkrtg/gtkrtgmath.h>
#include <gtkrtg/gtkrtgcanvas.h>
#include "gtkrtgresources.h"
#include "gtkrtgextras.h"
#include <map>

#define GTK_RTG_TYPE_TRIPOD            (gtk_rtg_tripod_get_type())
#define GTK_RTG_TRIPOD(obj)		       (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_RTG_TYPE_TRIPOD, GtkRtgTripod))
#define GTK_RTG_TRIPOD_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass),  GTK_RTG_TYPE_TRIPOD, GtkRtgTripodClass))
#define GTK_RTG_IS_TRIPOD(obj)		   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_RTG_TYPE_TRIPOD))
#define GTK_RTG_IS_TRIPOD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GTK_RTG_TYPE_TRIPOD))
#define GTK_RTG_TRIPOD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GTK_RTG_TYPE_TRIPOD, GtkRtgTripodClass))

typedef struct _GtkRtgTripod	  GtkRtgTripod;
typedef struct _GtkRtgTripodClass GtkRtgTripodClass;

/* defines */
enum { AX_X, AX_Y, AX_Z, AXES        };
enum { AX_START, AX_END, AXIS_POINTS };
enum
{
	SELECTED_AX_X	      = 0x02,
	SELECTED_AX_Y	      = 0x04,
	SELECTED_AX_Z	      = 0x08,
	SELECTED_PLANE_CAMERA = 0x10,
    SELECTED_CIRCLE       = 0x20
};

typedef struct
{
	guint id;
	GdkPoint ax2d[AXIS_POINTS];
    GdkPoint ax2d_offscreen[AXIS_POINTS];
	gfloat scale;
	GtkRtgText *text;
	GdkColor *c_ax;
    gfloat angle;

    /* flags */
    guint sel_flag;
    gboolean mirror;
}TripodAx;

typedef std::multimap<gfloat, TripodAx*, floatCmp>   m_axesType;

struct _GtkRtgTripod
{
	GObject parent;
	GtkRtgCanvas *canvas;
	gtkrtg::GtkRtgPickingBuffer	*pb;
	TripodAx ax[AXES];
	GdkPoint center;
	m_axesType *m_ax;
	gtkrtg::matrix4x4f matrixproject;
	gchar *font_desc;
	GdkColor colors[AXES+1];
    gtkrtg::point3f offset;

	/* drag */
	gtkrtg::point3f delta_offset;
	gtkrtg::point3f current_offset;

	/* picking */
	guint color_id;

	/* flags */
	guint text:1;
	guint selected;	/* complete selection */
	guint sel_fallback;	/* fall-back selection */
	gboolean sel_hit; /* last selection hit */
	guint drag:1;
    guint fallback:1;
	guint skip_parallel_ax:1;

	/* offscreen */
	guint offscreen:1;
};

struct _GtkRtgTripodClass
{
	GObjectClass		parent_class;

	/* signal(s) */
	void (* project)				(GtkRtgTripod *tripod, gtkrtg::point3f *offset);
	void (* render)					(GtkRtgTripod *tripod);
	void (* render_axis)			(GtkRtgTripod *tripod, guint axis);
	void (* render_last)			(GtkRtgTripod *tripod);
	void (* render_offscreen)		(GtkRtgTripod *tripod);
	void (* render_axis_offscreen)	(GtkRtgTripod *tripod, guint axis);
	void (* process_motion)			(GtkRtgTripod *tripod, gint x, gint y);
	void (* process_drag)			(GtkRtgTripod *tripod, gint x, gint y);
	void (* update_selection)		(GtkRtgTripod *tripod);
};

enum
{
	GIZMO_NO_PROCESSING  = 0x00,
	GIZMO_PROCESS_CURSOR = 0x02,
	GIZMO_PROCESS_REDRAW = 0x04
};

GType gtk_rtg_tripod_get_type(void) G_GNUC_CONST;
GtkRtgTripod *gtk_rtg_tripod_new(GtkRtgCanvas *canvas, gchar *font_desc);
void gtk_rtg_tripod_project(GtkRtgTripod *tripod, gtkrtg::point3f *offset=NULL);
void gtk_rtg_tripod_render(GtkRtgTripod *tripod, GtkRtgCanvasViewPredefined predefined);
void gtk_rtg_tripod_render_offscreen(GtkRtgTripod *tripod, GtkRtgCanvasViewPredefined predefined);
guint gtk_rtg_tripod_process_motion(GtkRtgTripod *tripod, gint x, gint y);
const gtkrtg::vector3f &gtk_rtg_tripod_process_drag(GtkRtgTripod *tripod, gint x, gint y);
void gtk_rtg_tripod_end_process_drag(GtkRtgTripod *tripod);

#endif /* GTK_RTG_TRIPOD_H */
