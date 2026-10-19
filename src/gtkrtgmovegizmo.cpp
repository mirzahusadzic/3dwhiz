#include "gtkrtgmovegizmo.h"
#include <gtkrtg/gtkrtgmath.h>
#include "gtkrtgglu.h"
#include "gtkrtgutils.h"

G_DEFINE_TYPE(GtkRtgMoveGizmo, gtk_rtg_move_gizmo, GTK_RTG_TYPE_GIZMO)

using namespace gtkrtg;

static void	gtk_rtg_move_gizmo_project(GtkRtgTripod *tripod, point3f *offset, gpointer data);
static void	gtk_rtg_move_gizmo_render(GtkRtgTripod *tripod);
static void	gtk_rtg_move_gizmo_render_axis(GtkRtgTripod *tripod, guint ax_id, gpointer data);
static void	gtk_rtg_move_gizmo_render_axis_offscreen(GtkRtgTripod *tripod, guint ax_id, gpointer data);
static void	gtk_rtg_move_gizmo_process_motion(GtkRtgTripod *tripod, gint x, gint y);
static void	gtk_rtg_move_gizmo_process_drag(GtkRtgTripod *tripod, gint x, gint y);

extern guint gizmo_sel_color[GIZMO_PLANES];

static point3f point_ax3d[AXES][AXIS_POINTS] =
{
	{ point3f( .25f, 0.f,   0.f),   point3f( .7525f, 0.f,     0.f) },
	{ point3f(0.f,    .25f, 0.f),   point3f(0.f,      .7525f, 0.f) },
	{ point3f(0.f,   0.f,    .25f), point3f(0.f,     0.f,      .7525f) }
};

enum
{
	VEC_ORIGIN,
	VEC_XY,
	VEC_X,
	VEC_XZ,
	VEC_Z,
	VEC_YZ,
	VEC_Y
};

static point3f vec_plane[GIZMO_PLANE_VERTS] =
{
    point3f(0.f, 0.f, 0.f),		/* VEC_ORIGIN */
	point3f(0.35f, 0.35f, 0.f),	/* VEC_XY     */
	point3f(0.35f, 0.f, 0.f),	/* VEC_X      */
    point3f(0.35f, 0.f, 0.35f),	/* VEC_XZ     */
	point3f(0.f, 0.f, 0.35f),	/* VEC_Z      */
	point3f(0.f, 0.35f, 0.35f),	/* VEC_YZ     */
	point3f(0.f, 0.35f, 0.f)	/* VEC_Y      */
};


static void
gtk_rtg_move_gizmo_class_init(GtkRtgMoveGizmoClass *klass)
{
	GtkRtgTripodClass *tripod_class = GTK_RTG_TRIPOD_CLASS(klass);

	tripod_class->process_motion = gtk_rtg_move_gizmo_process_motion;
	tripod_class->render = gtk_rtg_move_gizmo_render;
}


GtkRtgMoveGizmo *
gtk_rtg_move_gizmo_new(GtkRtgCanvas *canvas, gchar *font_desc)
{
	return GTK_RTG_MOVE_GIZMO(g_object_new(GTK_RTG_TYPE_MOVE_GIZMO, "font-desc", font_desc,
																	"canvas",    canvas, NULL));
}


static void
gtk_rtg_move_gizmo_init(GtkRtgMoveGizmo *mg)
{
	GtkRtgGizmo  *gizmo  = GTK_RTG_GIZMO(mg);

	g_signal_connect(mg, "project",					G_CALLBACK(gtk_rtg_move_gizmo_project),					NULL);
	g_signal_connect(mg, "render-axis",				G_CALLBACK(gtk_rtg_move_gizmo_render_axis),				NULL);
	g_signal_connect(mg, "render-axis-offscreen",	G_CALLBACK(gtk_rtg_move_gizmo_render_axis_offscreen),	NULL);
	g_signal_connect(mg, "process-drag",			G_CALLBACK(gtk_rtg_move_gizmo_process_drag),			NULL);

	/* set axis */
	memcpy(gizmo->point_ax3d_ortho, point_ax3d, sizeof(point_ax3d));
	memcpy(gizmo->point_ax3d_persp, point_ax3d, sizeof(point_ax3d));
}

/*
 * Publics (lib only)
 *
 */
static void
gtk_rtg_move_gizmo_project(GtkRtgTripod *tripod, point3f *offset, gpointer data)
{
	GtkRtgMoveGizmo *mg = GTK_RTG_MOVE_GIZMO(tripod);
	GtkRtgGizmo *gizmo = GTK_RTG_GIZMO(tripod);
	GtkRtgCamera *camera = gtk_rtg_canvas_camera_get_default(tripod->canvas);

	if(PROJECTION(camera) == GTK_RTG_CAM_PROJ_PERSP)
	{
		for(guint i=0; i<GIZMO_PLANE_VERTS; ++i)
		{
			/* project point */
			point3f point_orient = vec_plane[i] * gizmo->persp_scale + *offset;

			gluProjectf(point_orient, tripod->matrixproject.m,
									  gizmo->viewport, mg->vec_plane2d[i]);
		}

		/*
		 * Get fall-back selection
		 *
		 */
		if(tripod->fallback)
		{
			gboolean xy = TRUE;
			for(m_axesType::const_iterator it=tripod->m_ax->begin(); it!=tripod->m_ax->end(); ++it)
			{
				TripodAx *ax = it->second;

				if(fabsf(ax->angle) > 0.99999f)
				{
					xy = FALSE;
					break;
				}

				tripod->sel_fallback |= ax->sel_flag;
			}

			if(xy)	tripod->sel_fallback = SELECTED_AX_X | SELECTED_AX_Y;
		}
	}else
	/* orthographic */
	{
		for(guint i=0; i<GIZMO_PLANE_VERTS; ++i)
		{
			point3f point_orient = gizmo->matrixproject * vec_plane[i];
			utils_unproject_point2screen(point_orient, gizmo->alphabeta, gizmo->alphabeta, mg->vec_plane2d[i]);
			mg->vec_plane2d[i].y = gizmo->size - mg->vec_plane2d[i].y;

			/* translate to offset */
			mg->vec_plane2d[i].x += gizmo->xt;
			mg->vec_plane2d[i].y += gizmo->yt;
		}

		/*
		 * Get fall-back selection
		 *
		 */
		if(tripod->fallback)
		{
			switch(gtk_rtg_canvas_view_get_predefined(tripod->canvas))
			{
				case GTK_RTG_CANVAS_VIEW_FRONT:
				case GTK_RTG_CANVAS_VIEW_BACK:   tripod->sel_fallback = gtk_rtg_canvas_view_get_up(tripod->canvas) == GTK_RTG_CANVAS_VIEW_UP_Y
                                                 ? SELECTED_PLANE_YZ : SELECTED_PLANE_XZ; break;

				case GTK_RTG_CANVAS_VIEW_TOP:
				case GTK_RTG_CANVAS_VIEW_BOTTOM: tripod->sel_fallback = gtk_rtg_canvas_view_get_up(tripod->canvas) == GTK_RTG_CANVAS_VIEW_UP_Y
                                                 ? SELECTED_PLANE_XZ : SELECTED_PLANE_XY; break;

				case GTK_RTG_CANVAS_VIEW_LEFT:
				case GTK_RTG_CANVAS_VIEW_RIGHT:  tripod->sel_fallback = gtk_rtg_canvas_view_get_up(tripod->canvas) == GTK_RTG_CANVAS_VIEW_UP_Y
                                                 ? SELECTED_PLANE_XY : SELECTED_PLANE_YZ; break;

				default: break;
			};
		}
	}
}


static void
gtk_rtg_move_gizmo_render_axis(GtkRtgTripod *tripod, guint ax_id, gpointer data)
{
	GtkRtgMoveGizmo *mg = GTK_RTG_MOVE_GIZMO(tripod);

	glBegin(GL_LINES);

		switch(ax_id)
		{
			case AX_X:	tripod->selected == SELECTED_PLANE_XY ? glColor3usv(&tripod->colors[AXES].red) : glColor3usv(&tripod->colors[ax_id].red);
						glVertex2i(mg->vec_plane2d[VEC_X ].x, mg->vec_plane2d[VEC_X ].y);
						glVertex2i(mg->vec_plane2d[VEC_XY].x, mg->vec_plane2d[VEC_XY].y);

						tripod->selected == SELECTED_PLANE_XZ ? glColor3usv(&tripod->colors[AXES].red) : glColor3usv(&tripod->colors[ax_id].red);
						glVertex2i(mg->vec_plane2d[VEC_X ].x, mg->vec_plane2d[VEC_X ].y);
						glVertex2i(mg->vec_plane2d[VEC_XZ].x, mg->vec_plane2d[VEC_XZ].y);

						break;

			case AX_Y:  tripod->selected == SELECTED_PLANE_XY ? glColor3usv(&tripod->colors[AXES].red) : glColor3usv(&tripod->colors[ax_id].red);
						glVertex2i(mg->vec_plane2d[VEC_Y ].x, mg->vec_plane2d[VEC_Y ].y);
						glVertex2i(mg->vec_plane2d[VEC_XY].x, mg->vec_plane2d[VEC_XY].y);

						tripod->selected == SELECTED_PLANE_YZ ? glColor3usv(&tripod->colors[AXES].red) : glColor3usv(&tripod->colors[ax_id].red);
						glVertex2i(mg->vec_plane2d[VEC_Y ].x, mg->vec_plane2d[VEC_Y ].y);
						glVertex2i(mg->vec_plane2d[VEC_YZ].x, mg->vec_plane2d[VEC_YZ].y);

						break;

			case AX_Z:	tripod->selected == SELECTED_PLANE_XZ ? glColor3usv(&tripod->colors[AXES].red) : glColor3usv(&tripod->colors[ax_id].red);
						glVertex2i(mg->vec_plane2d[VEC_Z ].x, mg->vec_plane2d[VEC_Z ].y);
						glVertex2i(mg->vec_plane2d[VEC_XZ].x, mg->vec_plane2d[VEC_XZ].y);

						tripod->selected == SELECTED_PLANE_YZ ? glColor3usv(&tripod->colors[AXES].red) : glColor3usv(&tripod->colors[ax_id].red);
						glVertex2i(mg->vec_plane2d[VEC_Z ].x, mg->vec_plane2d[VEC_Z ].y);
						glVertex2i(mg->vec_plane2d[VEC_YZ].x, mg->vec_plane2d[VEC_YZ].y);

						break;
		};

	glEnd();
}


static void
gtk_rtg_move_gizmo_render(GtkRtgTripod *tripod)
{
	/*
	 * Render selected plane.
	 *
	 */

	GtkRtgMoveGizmo *mg	= GTK_RTG_MOVE_GIZMO(tripod);
	GtkRtgGizmo *gizmo = GTK_RTG_GIZMO(tripod);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glColor4f(gizmo->color_plane[0], gizmo->color_plane[1], gizmo->color_plane[2], gizmo->color_plane[3]);

	glBegin(GL_QUADS);

		if(tripod->selected == SELECTED_PLANE_XY)
		{
			glVertex2i(mg->vec_plane2d[VEC_ORIGIN].x, mg->vec_plane2d[VEC_ORIGIN].y);
			glVertex2i(mg->vec_plane2d[VEC_Y].x,	  mg->vec_plane2d[VEC_Y].y);
			glVertex2i(mg->vec_plane2d[VEC_XY].x,	  mg->vec_plane2d[VEC_XY].y);
			glVertex2i(mg->vec_plane2d[VEC_X].x,	  mg->vec_plane2d[VEC_X].y);
		}else
		if(tripod->selected == SELECTED_PLANE_XZ)
		{
			glVertex2i(mg->vec_plane2d[VEC_ORIGIN].x, mg->vec_plane2d[VEC_ORIGIN].y);
			glVertex2i(mg->vec_plane2d[VEC_X].x,	  mg->vec_plane2d[VEC_X].y);
			glVertex2i(mg->vec_plane2d[VEC_XZ].x,	  mg->vec_plane2d[VEC_XZ].y);
			glVertex2i(mg->vec_plane2d[VEC_Z].x,	  mg->vec_plane2d[VEC_Z].y);

		}else
		if(tripod->selected == SELECTED_PLANE_YZ)
		{
			glVertex2i(mg->vec_plane2d[VEC_ORIGIN].x, mg->vec_plane2d[VEC_ORIGIN].y);
			glVertex2i(mg->vec_plane2d[VEC_Z].x,	  mg->vec_plane2d[VEC_Z].y);
			glVertex2i(mg->vec_plane2d[VEC_YZ].x,	  mg->vec_plane2d[VEC_YZ].y);
			glVertex2i(mg->vec_plane2d[VEC_Y].x,	  mg->vec_plane2d[VEC_Y].y);
		}

	glEnd();

	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
}


static void
gtk_rtg_move_gizmo_render_axis_offscreen(GtkRtgTripod *tripod, guint ax_id, gpointer data)
{
	GtkRtgMoveGizmo  *mg = GTK_RTG_MOVE_GIZMO(tripod);

	glBegin(GL_LINES);

		switch(ax_id)
		{
				case AX_X:      glColor3ubv((guchar*)&gizmo_sel_color[PLANE_XY]);
								glVertex2i(mg->vec_plane2d[VEC_X ].x, mg->vec_plane2d[VEC_X ].y);
								glVertex2i(mg->vec_plane2d[VEC_XY].x, mg->vec_plane2d[VEC_XY].y);

								glColor3ubv((guchar*)&gizmo_sel_color[PLANE_XZ]);
								glVertex2i(mg->vec_plane2d[VEC_X ].x, mg->vec_plane2d[VEC_X ].y);
								glVertex2i(mg->vec_plane2d[VEC_XZ].x, mg->vec_plane2d[VEC_XZ].y);

								break;

				case AX_Y:      glColor3ubv((guchar*)&gizmo_sel_color[PLANE_XY]);
								glVertex2i(mg->vec_plane2d[VEC_Y ].x, mg->vec_plane2d[VEC_Y ].y);
								glVertex2i(mg->vec_plane2d[VEC_XY].x, mg->vec_plane2d[VEC_XY].y);

								glColor3ubv((guchar*)&gizmo_sel_color[PLANE_YZ]);
								glVertex2i(mg->vec_plane2d[VEC_Y ].x, mg->vec_plane2d[VEC_Y ].y);
								glVertex2i(mg->vec_plane2d[VEC_YZ].x, mg->vec_plane2d[VEC_YZ].y);

								break;

				case AX_Z:      glColor3ubv((guchar*)&gizmo_sel_color[PLANE_XZ]);
								glVertex2i(mg->vec_plane2d[VEC_Z ].x, mg->vec_plane2d[VEC_Z ].y);
								glVertex2i(mg->vec_plane2d[VEC_XZ].x, mg->vec_plane2d[VEC_XZ].y);

								glColor3ubv((guchar*)&gizmo_sel_color[PLANE_YZ]);
								glVertex2i(mg->vec_plane2d[VEC_Z ].x, mg->vec_plane2d[VEC_Z ].y);
								glVertex2i(mg->vec_plane2d[VEC_YZ].x, mg->vec_plane2d[VEC_YZ].y);

								break;
		};

	glEnd();
}


static void
gtk_rtg_move_gizmo_process_motion(GtkRtgTripod *tripod, gint x, gint y)
{
	switch(tripod->color_id)
	{
		case COLOR_ID_GIZMO_PLANE_XY: tripod->selected |= SELECTED_PLANE_XY; break;
		case COLOR_ID_GIZMO_PLANE_XZ: tripod->selected |= SELECTED_PLANE_XZ; break;
		case COLOR_ID_GIZMO_PLANE_YZ: tripod->selected |= SELECTED_PLANE_YZ; break;
	};
}


static void
gtk_rtg_move_gizmo_process_drag(GtkRtgTripod *tripod, gint x, gint y)
{
	GtkRtgGizmo *gizmo = GTK_RTG_GIZMO(tripod);

	if(gizmo->intersect)
	{
		point3f off = tripod->offset + tripod->delta_offset;
		gtk_rtg_tripod_project(tripod, &off);
	}
}
