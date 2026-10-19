#include "gtkrtgscalegizmo.h"
#include <gtkrtg/gtkrtgmath.h>
#include "gtkrtgglu.h"
#include "gtkrtgutils.h"

G_DEFINE_TYPE(GtkRtgScaleGizmo, gtk_rtg_scale_gizmo, GTK_RTG_TYPE_GIZMO)

using namespace gtkrtg;

static void	gtk_rtg_scale_gizmo_project(GtkRtgTripod *tripod, point3f *offset, gpointer data);
static void	gtk_rtg_scale_gizmo_render(GtkRtgTripod *tripod);
static void	gtk_rtg_scale_gizmo_render_axis(GtkRtgTripod *tripod, guint ax_id, gpointer data);
static void	gtk_rtg_scale_gizmo_render_axis_offscreen(GtkRtgTripod *tripod, guint ax_id, gpointer data);
static void	gtk_rtg_scale_gizmo_process_motion(GtkRtgTripod *tripod, gint x, gint y);
static void	gtk_rtg_scale_gizmo_process_drag(GtkRtgTripod *tripod, gint x, gint y);

enum
{
	NO_MIRROR       = 0x00,
	MIRRORED_AX_X   = 0x02,
	MIRRORED_AX_Y   = 0x04,
	MIRRORED_AX_Z   = 0x08,
	MIRRORED_AX_XY  = MIRRORED_AX_X | MIRRORED_AX_Y,
	MIRRORED_AX_XZ  = MIRRORED_AX_X | MIRRORED_AX_Z,
	MIRRORED_AX_YZ  = MIRRORED_AX_Y | MIRRORED_AX_Z,
	MIRRORED_AX_XYZ = MIRRORED_AX_X | MIRRORED_AX_Y | MIRRORED_AX_Z
};

extern guint tripod_sel_color[AXES];
extern guint gizmo_sel_color[GIZMO_PLANES];

static point3f point_ax3d[AXES][AXIS_POINTS] =
{
	{ point3f(0.f, 0.f, 0.f), point3f(1.f, 0.f, 0.f)},
	{ point3f(0.f, 0.f, 0.f), point3f(0.f, 1.f, 0.f)},
	{ point3f(0.f, 0.f, 0.f), point3f(0.f, 0.f, 1.f)}
};

static point3f point_ax3d_offscreen_ortho[AXES][AXIS_POINTS]=
{
	{ point3f( .5f, 0.f,  0.f),  point3f( .97f, 0.f,   0.f)},
	{ point3f(0.f,   .5f, 0.f),  point3f(0.f,    .97f, 0.f)},
	{ point3f(0.f,  0.f,   .5f), point3f(0.f,   0.f,    .97f)}
};

static ScalePoly spoly_ortho[GIZMO_PLANES] =
{
	/* x-y */
	{
		{
			point3f(0.50f, 0.00f, 0.00f), point3f(0.48f, 0.13f, 0.00f), point3f(0.43f, 0.25f, 0.00f),
			point3f(0.35f, 0.35f, 0.00f), point3f(0.25f, 0.43f, 0.00f), point3f(0.13f, 0.48f, 0.00f),
			point3f(0.00f, 0.50f, 0.00f), point3f(0.00f, 0.00f, 0.00f), point3f(0.70f, 0.00f, 0.00f),
			point3f(0.67f, 0.19f, 0.00f), point3f(0.60f, 0.35f, 0.00f), point3f(0.49f, 0.49f, 0.00f),
			point3f(0.35f, 0.60f, 0.00f), point3f(0.19f, 0.67f, 0.00f), point3f(0.00f, 0.70f, 0.00f)
		}
	},

	/* x-z */
	{
		{
			point3f(0.50f, 0.00f, 0.00f), point3f(0.48f, 0.00f, 0.13f), point3f(0.43f, 0.00f, 0.25f),
			point3f(0.35f, 0.00f, 0.35f), point3f(0.25f, 0.00f, 0.43f), point3f(0.13f, 0.00f, 0.48f),
			point3f(0.00f, 0.00f, 0.50f), point3f(0.00f, 0.00f, 0.00f), point3f(0.70f, 0.00f, 0.00f),
			point3f(0.67f, 0.00f, 0.19f), point3f(0.60f, 0.00f, 0.35f), point3f(0.49f, 0.00f, 0.49f),
			point3f(0.35f, 0.00f, 0.60f), point3f(0.19f, 0.00f, 0.67f), point3f(0.00f, 0.00f, 0.70f)
		}
	},

	/* y-z */
	{
		{
			point3f(0.00f, 0.50f, 0.00f), point3f(0.00f, 0.48f, 0.13f), point3f(0.00f, 0.43f, 0.25f),
			point3f(0.00f, 0.35f, 0.35f), point3f(0.00f, 0.25f, 0.43f), point3f(0.00f, 0.13f, 0.48f),
			point3f(0.00f, 0.00f, 0.50f), point3f(0.00f, 0.00f, 0.00f), point3f(0.00f, 0.70f, 0.00f),
			point3f(0.00f, 0.67f, 0.19f), point3f(0.00f, 0.60f, 0.35f), point3f(0.00f, 0.49f, 0.49f),
			point3f(0.00f, 0.35f, 0.60f), point3f(0.00f, 0.19f, 0.67f), point3f(0.00f, 0.00f, 0.70f)
		}
	}
};

#define SCALE_POLY_QUADS 6
#define SCALE_POLY_TRIS	6

/* fcon: a lot to optimize here (tris into fan and quads into quad strip) */
static guint quad_i[SCALE_POLY_QUADS*4]= {9, 1, 0, 8, 10, 2, 1, 9, 11, 3, 2, 10, 12, 4, 3, 11, 13, 5, 4, 12, 14, 6, 5, 13};	/* quad indices */
static guint tris_i[SCALE_POLY_TRIS*3] = {0, 1, 7, 1, 2, 7, 2, 3, 7, 3, 4, 7, 4, 5, 7, 5, 6, 7};				/* triangle indices */

#define GIZMO_POINT_SIZE 7.f

static void
gtk_rtg_scale_gizmo_class_init(GtkRtgScaleGizmoClass *klass)
{
	GtkRtgTripodClass *tripod_class = GTK_RTG_TRIPOD_CLASS(klass);

	tripod_class->process_motion = gtk_rtg_scale_gizmo_process_motion;
	tripod_class->render = gtk_rtg_scale_gizmo_render;
}

GtkRtgScaleGizmo *
gtk_rtg_scale_gizmo_new(GtkRtgCanvas *canvas, gchar *font_desc)
{
	return GTK_RTG_SCALE_GIZMO(g_object_new(GTK_RTG_TYPE_SCALE_GIZMO, "font-desc", font_desc,
									  	  	  	  	  	  	  	  	  "canvas",    canvas, NULL));
}

static void
gtk_rtg_scale_gizmo_init(GtkRtgScaleGizmo *sg)
{
	GtkRtgTripod *tripod = GTK_RTG_TRIPOD(sg);
	GtkRtgGizmo  *gizmo  = GTK_RTG_GIZMO(sg);

	g_signal_connect(sg, "project",					G_CALLBACK(gtk_rtg_scale_gizmo_project),				NULL);
	g_signal_connect(sg, "render-axis",				G_CALLBACK(gtk_rtg_scale_gizmo_render_axis),			NULL);
	g_signal_connect(sg, "render-axis-offscreen",	G_CALLBACK(gtk_rtg_scale_gizmo_render_axis_offscreen),	NULL);
	g_signal_connect(sg, "process-drag",			G_CALLBACK(gtk_rtg_scale_gizmo_process_drag),			NULL);

	/* set axis */
	memcpy(gizmo->point_ax3d_ortho, point_ax3d, sizeof(point_ax3d));
	memcpy(gizmo->point_ax3d_persp, point_ax3d, sizeof(point_ax3d));

	/* no arrows */
	gizmo->arrow = 0;

	/* selection */
	tripod->selected = SELECTED_XYZ;
    tripod->sel_fallback = SELECTED_XYZ;

	/* get changeable  */
	memcpy(sg->spoly_persp, spoly_ortho, sizeof(spoly_ortho));
	memcpy(sg->point_ax3d_offscreen_persp, point_ax3d_offscreen_ortho, sizeof(point_ax3d_offscreen_ortho));

	sg->ax_mirror[AX_X]=sg->ax_mirror[AX_Y]=sg->ax_mirror[AX_Z]=1.f;
}

/*
 * Publics (lib only)
 *
 */
static void
gtk_rtg_scale_gizmo_project(GtkRtgTripod *tripod, point3f *offset, gpointer data)
{
	GtkRtgScaleGizmo  *sg	 = GTK_RTG_SCALE_GIZMO(tripod);
	GtkRtgGizmo      *gizmo  = GTK_RTG_GIZMO(tripod);
	GtkRtgCamera	 *camera = gtk_rtg_canvas_camera_get_default(tripod->canvas);

	if(PROJECTION(camera) == GTK_RTG_CAM_PROJ_PERSP)
	{
		gint mirror_ax = NO_MIRROR;

		/* should mirror? */
		for(guint i=0; i<AXES; ++i)
		{
			if(tripod->ax[i].mirror)
			{
				sg->ax_mirror[i] *= -1.f;	/* sign cache */

				/* mirror vectors */
				gizmo->point_ax3d_persp[i][AX_START] *= -1.f;
				gizmo->point_ax3d_persp[i][AX_END] *= -1.f;

				sg->point_ax3d_offscreen_persp[i][AX_START] *= -1.f;
				sg->point_ax3d_offscreen_persp[i][AX_END] *= -1.f;

				gizmo->text_pos_persp[i] *= -1.f;

				/* mark axis */
				mirror_ax  |= i ? i<<2 : MIRRORED_AX_X;
			}
		}

		if(mirror_ax)
		{
			/*
			 * Do local mirror and re-project gizmo.
			 *
			 */

			for(guint i=0; i<GIZMO_PLANES; ++i)
				for(guint j=0; j<SCALE_POLY_VERTS; ++j)
				{
					switch(mirror_ax)
					{
						case MIRRORED_AX_X:     X(sg->spoly_persp[i].p[j]) *= -1.f; break;
						case MIRRORED_AX_Y:     Y(sg->spoly_persp[i].p[j]) *= -1.f; break;
						case MIRRORED_AX_Z:     Z(sg->spoly_persp[i].p[j]) *= -1.f; break;
						case MIRRORED_AX_XY:    X(sg->spoly_persp[i].p[j]) *= -1.f; Y(sg->spoly_persp[i].p[j]) *= -1.f; break;
						case MIRRORED_AX_XZ:    X(sg->spoly_persp[i].p[j]) *= -1.f; Z(sg->spoly_persp[i].p[j]) *= -1.f; break;
						case MIRRORED_AX_YZ:    Y(sg->spoly_persp[i].p[j]) *= -1.f; Z(sg->spoly_persp[i].p[j]) *= -1.f; break;
						case MIRRORED_AX_XYZ:     sg->spoly_persp[i].p[j]  *= -1.f; break;
					};
				}

			/* fcon: avoid re-projection by recursion */
			return gtk_rtg_tripod_project(tripod, offset);
		}

		/*
		 * Project
		 *
		 */

		for(guint i=0; i<GIZMO_PLANES; ++i)
			for(guint j=0; j<SCALE_POLY_VERTS; ++j)
			{
				point3f point_orient = sg->spoly_persp[i].p[j] * gizmo->persp_scale + *offset;
				gluProjectf(point_orient, tripod->matrixproject.m, gizmo->viewport, sg->vec_poly2d[i][j]);
			}

		for(guint i=0; i<AXES; ++i)
			for(guint j=0; j<AXIS_POINTS; ++j)
			{
				point3f ax_orient = sg->point_ax3d_offscreen_persp[i][j] * tripod->ax[i].scale * gizmo->persp_scale + *offset;
				gluProjectf(ax_orient, tripod->matrixproject.m, gizmo->viewport, tripod->ax[i].ax2d_offscreen[j]);
			}
	}else
	/* orthographic */
	{
		for(guint i=0; i<GIZMO_PLANES; ++i)
			for(guint j=0; j<SCALE_POLY_VERTS; ++j)
			{
				point3f point_orient = gizmo->matrixproject * spoly_ortho[i].p[j];
				utils_unproject_point2screen(point_orient, gizmo->alphabeta, gizmo->alphabeta, sg->vec_poly2d[i][j]);
				sg->vec_poly2d[i][j].y = gizmo->size - sg->vec_poly2d[i][j].y;

				/* translate to offset */
				sg->vec_poly2d[i][j].x += gizmo->xt;
				sg->vec_poly2d[i][j].y += gizmo->yt;
			}


		for(guint i=0; i<AXES; ++i)
			for(guint j=0; j<AXIS_POINTS; ++j)
			{
				point3f ax_orient = gizmo->matrixproject * (point_ax3d_offscreen_ortho[i][j] * tripod->ax[i].scale);

				utils_unproject_point2screen(ax_orient, gizmo->alphabeta, gizmo->alphabeta, tripod->ax[i].ax2d_offscreen[j]);
				tripod->ax[i].ax2d_offscreen[j].y = gizmo->size - tripod->ax[i].ax2d_offscreen[j].y;

				/* translate to offset */
				tripod->ax[i].ax2d_offscreen[j].x += gizmo->xt;
				tripod->ax[i].ax2d_offscreen[j].y += gizmo->yt;
			}
	}

	/*
	 * Get fall-back selection
	 *
	 */
	if(tripod->fallback)
		tripod->sel_fallback = tripod->selected = SELECTED_XYZ;
}

#define RENDER_ARCS_LINE_FIRST_HALF(sg, id) \
{ \
	/* inner half arc */ \
	glBegin(GL_LINE_STRIP); \
		for(guint i=0; i<4; ++i) \
			glVertex2i(sg->vec_poly2d[id][i].x, sg->vec_poly2d[id][i].y); \
	glEnd(); \
 \
	/* outer half arc */ \
	glBegin(GL_LINE_STRIP); \
	for(guint i=8; i<12; ++i) \
		glVertex2i(sg->vec_poly2d[id][i].x, sg->vec_poly2d[id][i].y); \
	glEnd(); \
}

#define RENDER_ARCS_LINE_SECOND_HALF(sg, id) \
{ \
	/* inner half arc */ \
	glBegin(GL_LINE_STRIP); \
		for(guint i=3; i<7; ++i) \
			glVertex2i(sg->vec_poly2d[id][i].x, sg->vec_poly2d[id][i].y); \
	glEnd(); \
 \
	/* outer half arc */ \
	glBegin(GL_LINE_STRIP); \
		for(guint i=11; i<15; ++i) \
			glVertex2i(sg->vec_poly2d[id][i].x, sg->vec_poly2d[id][i].y); \
	glEnd(); \
}

#define RENDER_POINT(ax, color) \
{ \
	gfloat point_size; \
	glGetFloatv(GL_POINT_SIZE, &point_size); \
	glPointSize(GIZMO_POINT_SIZE); \
	glColor3usv(&color); \
	glEnable(GL_POINT_SMOOTH); \
	glBegin(GL_POINTS); \
		glVertex2i(ax->ax2d_offscreen[AX_END].x, ax->ax2d_offscreen[AX_END].y); \
	glEnd(); \
 \
	if(point_size != GIZMO_POINT_SIZE) \
		glPointSize(point_size); \
 \
	glDisable(GL_POINT_SMOOTH); \
}

static void
gtk_rtg_scale_gizmo_render_axis(GtkRtgTripod *tripod, guint ax_id, gpointer data)
{
	GtkRtgScaleGizmo *sg = GTK_RTG_SCALE_GIZMO(tripod);
	GtkRtgCamera *cam = gtk_rtg_canvas_camera_get_default(tripod->canvas);
	GtkRtgCamProj proj = gtk_rtg_camera_get_projection(cam);
	GtkRtgCanvasViewPredefined predefined = gtk_rtg_canvas_view_get_predefined(tripod->canvas);

	switch(ax_id)
	{
		case AX_X:	if(proj == GTK_RTG_CAM_PROJ_PERSP)
					{
						/* x-y */
						tripod->selected == SELECTED_XYZ || tripod->selected == SELECTED_PLANE_XY ? glColor3usv(&tripod->colors[AXES].red)
																				  	  	  	  	  : glColor3usv(&tripod->colors[ax_id].red);
						RENDER_ARCS_LINE_FIRST_HALF(sg, PLANE_XY);

						/* x-z */
						tripod->selected == SELECTED_XYZ || tripod->selected == SELECTED_PLANE_XZ ? glColor3usv(&tripod->colors[AXES].red)
																				  	  	  	  	  : glColor3usv(&tripod->colors[ax_id].red);
						RENDER_ARCS_LINE_FIRST_HALF(sg, PLANE_XZ);
					}else
					switch(predefined)
					{
						case GTK_RTG_CANVAS_VIEW_TOP:
						case GTK_RTG_CANVAS_VIEW_BOTTOM: /* x-y */
														tripod->selected == SELECTED_XYZ || tripod->selected == SELECTED_PLANE_XY ? glColor3usv(&tripod->colors[AXES].red)
																												  	  	  	  	  : glColor3usv(&tripod->colors[ax_id].red);
														RENDER_ARCS_LINE_FIRST_HALF(sg, PLANE_XY);
														break;

						case GTK_RTG_CANVAS_VIEW_FRONT:
						case GTK_RTG_CANVAS_VIEW_BACK:	/* x-z */
														tripod->selected == SELECTED_XYZ || tripod->selected == SELECTED_PLANE_XZ ? glColor3usv(&tripod->colors[AXES].red)
																												  	  	  	  	  : glColor3usv(&tripod->colors[ax_id].red);
														RENDER_ARCS_LINE_FIRST_HALF(sg, PLANE_XZ);
														break;
						default: break;
					};

					break;

		case AX_Y:	if(proj == GTK_RTG_CAM_PROJ_PERSP)
					{
						/* x-y */
						tripod->selected == SELECTED_XYZ || tripod->selected == SELECTED_PLANE_XY ? glColor3usv(&tripod->colors[AXES].red)
																				  	  	  	  	  : glColor3usv(&tripod->colors[ax_id].red);
						RENDER_ARCS_LINE_SECOND_HALF(sg, PLANE_XY);

						/* y-z */
						tripod->selected == SELECTED_XYZ || tripod->selected == SELECTED_PLANE_YZ ? glColor3usv(&tripod->colors[AXES].red)
																				  	  	  	  	  : glColor3usv(&tripod->colors[ax_id].red);
						RENDER_ARCS_LINE_FIRST_HALF(sg, PLANE_YZ);

					}else
					switch(predefined)
					{
						case GTK_RTG_CANVAS_VIEW_TOP:
						case GTK_RTG_CANVAS_VIEW_BOTTOM:/* x-y */
														tripod->selected == SELECTED_XYZ || tripod->selected == SELECTED_PLANE_XY ? glColor3usv(&tripod->colors[AXES].red)
																																  : glColor3usv(&tripod->colors[ax_id].red);
														RENDER_ARCS_LINE_SECOND_HALF(sg, PLANE_XY);
														break;
						case GTK_RTG_CANVAS_VIEW_LEFT:
						case GTK_RTG_CANVAS_VIEW_RIGHT:	/* y-z */
														tripod->selected == SELECTED_XYZ || tripod->selected == SELECTED_PLANE_YZ ? glColor3usv(&tripod->colors[AXES].red)
																																  : glColor3usv(&tripod->colors[ax_id].red);
														RENDER_ARCS_LINE_FIRST_HALF(sg, PLANE_YZ);
														break;
						default: break;
					};

					break;

		case AX_Z:	if(proj == GTK_RTG_CAM_PROJ_PERSP)
					{
						/* x-z */
						tripod->selected == SELECTED_XYZ || tripod->selected == SELECTED_PLANE_XZ ? glColor3usv(&tripod->colors[AXES].red)
																				  	  	  	  	  : glColor3usv(&tripod->colors[ax_id].red);
						RENDER_ARCS_LINE_SECOND_HALF(sg, PLANE_XZ);

						/* y-z */
						tripod->selected == SELECTED_XYZ || tripod->selected == SELECTED_PLANE_YZ ? glColor3usv(&tripod->colors[AXES].red)
																				  	  	  	      : glColor3usv(&tripod->colors[ax_id].red);
						RENDER_ARCS_LINE_SECOND_HALF(sg, PLANE_YZ);

					}else
					switch(predefined)
					{
						case GTK_RTG_CANVAS_VIEW_FRONT:
						case GTK_RTG_CANVAS_VIEW_BACK:	/* x-z */
														tripod->selected == SELECTED_XYZ || tripod->selected == SELECTED_PLANE_XZ ? glColor3usv(&tripod->colors[AXES].red)
																																  : glColor3usv(&tripod->colors[ax_id].red);
														RENDER_ARCS_LINE_SECOND_HALF(sg, PLANE_XZ);
														break;

						case GTK_RTG_CANVAS_VIEW_LEFT:
						case GTK_RTG_CANVAS_VIEW_RIGHT:	/* y-z */
														tripod->selected == SELECTED_XYZ ||  tripod->selected == SELECTED_PLANE_YZ ? glColor3usv(&tripod->colors[AXES].red)
																																   : glColor3usv(&tripod->colors[ax_id].red);
														RENDER_ARCS_LINE_SECOND_HALF(sg, PLANE_YZ);
														break;
						default: break;
					};

					break;
	};

	RENDER_POINT((&tripod->ax[ax_id]), tripod->colors[ax_id].red);
}

static void
gtk_rtg_scale_gizmo_render(GtkRtgTripod *tripod)
{
	GtkRtgScaleGizmo *sg = GTK_RTG_SCALE_GIZMO(tripod);
	GtkRtgGizmo *gizmo = GTK_RTG_GIZMO(tripod);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glColor4f(gizmo->color_plane[0], gizmo->color_plane[1], gizmo->color_plane[2], gizmo->color_plane[3]);

	switch(tripod->selected)
	{
		case SELECTED_XYZ:	for(guint i=0; i<GIZMO_PLANES; ++i)
							{
								glBegin(GL_TRIANGLES);
									for(guint j=0; j<18; j+=3)
									{
										glVertex2i(sg->vec_poly2d[i][tris_i[j  ]].x, sg->vec_poly2d[i][tris_i[j  ]].y);
                                        glVertex2i(sg->vec_poly2d[i][tris_i[j+1]].x, sg->vec_poly2d[i][tris_i[j+1]].y);
                                        glVertex2i(sg->vec_poly2d[i][tris_i[j+2]].x, sg->vec_poly2d[i][tris_i[j+2]].y);
									}
								glEnd();
							}
							break;

		case SELECTED_PLANE_XY: glBegin(GL_QUADS);
									for(guint i=0; i<SCALE_POLY_QUADS*4; i+=4)
									{
										glVertex2i(sg->vec_poly2d[PLANE_XY][quad_i[i  ]].x, sg->vec_poly2d[PLANE_XY][quad_i[i  ]].y);
										glVertex2i(sg->vec_poly2d[PLANE_XY][quad_i[i+1]].x, sg->vec_poly2d[PLANE_XY][quad_i[i+1]].y);
										glVertex2i(sg->vec_poly2d[PLANE_XY][quad_i[i+2]].x, sg->vec_poly2d[PLANE_XY][quad_i[i+2]].y);
										glVertex2i(sg->vec_poly2d[PLANE_XY][quad_i[i+3]].x, sg->vec_poly2d[PLANE_XY][quad_i[i+3]].y);
									}
								glEnd();
								break;

		case SELECTED_PLANE_XZ: glBegin(GL_QUADS);
									for(guint i=0; i<SCALE_POLY_QUADS*4; i+=4)
									{
										glVertex2i(sg->vec_poly2d[PLANE_XZ][quad_i[i  ]].x, sg->vec_poly2d[PLANE_XZ][quad_i[i  ]].y);
										glVertex2i(sg->vec_poly2d[PLANE_XZ][quad_i[i+1]].x, sg->vec_poly2d[PLANE_XZ][quad_i[i+1]].y);
										glVertex2i(sg->vec_poly2d[PLANE_XZ][quad_i[i+2]].x, sg->vec_poly2d[PLANE_XZ][quad_i[i+2]].y);
										glVertex2i(sg->vec_poly2d[PLANE_XZ][quad_i[i+3]].x, sg->vec_poly2d[PLANE_XZ][quad_i[i+3]].y);
									}
								glEnd();
								break;

		case SELECTED_PLANE_YZ: glBegin(GL_QUADS);
									for(guint i=0; i<SCALE_POLY_QUADS*4; i+=4)
									{
										glVertex2i(sg->vec_poly2d[PLANE_YZ][quad_i[i  ]].x, sg->vec_poly2d[PLANE_YZ][quad_i[i  ]].y);
										glVertex2i(sg->vec_poly2d[PLANE_YZ][quad_i[i+1]].x, sg->vec_poly2d[PLANE_YZ][quad_i[i+1]].y);
										glVertex2i(sg->vec_poly2d[PLANE_YZ][quad_i[i+2]].x, sg->vec_poly2d[PLANE_YZ][quad_i[i+2]].y);
										glVertex2i(sg->vec_poly2d[PLANE_YZ][quad_i[i+3]].x, sg->vec_poly2d[PLANE_YZ][quad_i[i+3]].y);
									}
								glEnd();
								break;
	};

	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
}

#define RENDER_ARCS_POLY_FIRST_HALF(sg, id) \
{ \
	/* outer half arc */ \
	glBegin(GL_QUADS); \
		for(guint i=0; i<12; i+=4) \
		{ \
			glVertex2i(sg->vec_poly2d[id][quad_i[i  ]].x, sg->vec_poly2d[id][quad_i[i  ]].y); \
			glVertex2i(sg->vec_poly2d[id][quad_i[i+1]].x, sg->vec_poly2d[id][quad_i[i+1]].y); \
			glVertex2i(sg->vec_poly2d[id][quad_i[i+2]].x, sg->vec_poly2d[id][quad_i[i+2]].y); \
			glVertex2i(sg->vec_poly2d[id][quad_i[i+3]].x, sg->vec_poly2d[id][quad_i[i+3]].y); \
		} \
	glEnd(); \
 \
	glColor3ubv((guchar*)&gizmo_sel_color[GIZMO_PLANES]); \
 \
	/* inner half arc */ \
	glBegin(GL_TRIANGLES); \
		for(guint i=0; i<9; i+=3) \
		{ \
			glVertex2i(sg->vec_poly2d[id][tris_i[i  ]].x, sg->vec_poly2d[id][tris_i[i  ]].y); \
			glVertex2i(sg->vec_poly2d[id][tris_i[i+1]].x, sg->vec_poly2d[id][tris_i[i+1]].y); \
			glVertex2i(sg->vec_poly2d[id][tris_i[i+2]].x, sg->vec_poly2d[id][tris_i[i+2]].y); \
		} \
	glEnd(); \
}

#define RENDER_ARCS_POLY_SECOND_HALF(sg, id) \
{ \
	/* outer half arc */ \
	glBegin(GL_QUADS); \
		for(guint i=12; i<24; i+=4) \
		{ \
			glVertex2i(sg->vec_poly2d[id][quad_i[i  ]].x, sg->vec_poly2d[id][quad_i[i  ]].y); \
			glVertex2i(sg->vec_poly2d[id][quad_i[i+1]].x, sg->vec_poly2d[id][quad_i[i+1]].y); \
			glVertex2i(sg->vec_poly2d[id][quad_i[i+2]].x, sg->vec_poly2d[id][quad_i[i+2]].y); \
			glVertex2i(sg->vec_poly2d[id][quad_i[i+3]].x, sg->vec_poly2d[id][quad_i[i+3]].y); \
		} \
	glEnd(); \
 \
	glColor3ubv((guchar*)&gizmo_sel_color[GIZMO_PLANES]); \
 \
	/* inner half arc */ \
	glBegin(GL_TRIANGLES); \
		for(guint i=9; i<18; i+=3) \
		{ \
			glVertex2i(sg->vec_poly2d[id][tris_i[i  ]].x, sg->vec_poly2d[id][tris_i[i  ]].y); \
			glVertex2i(sg->vec_poly2d[id][tris_i[i+1]].x, sg->vec_poly2d[id][tris_i[i+1]].y); \
			glVertex2i(sg->vec_poly2d[id][tris_i[i+2]].x, sg->vec_poly2d[id][tris_i[i+2]].y); \
		} \
	glEnd(); \
}


static void
gtk_rtg_scale_gizmo_render_axis_offscreen(GtkRtgTripod *tripod, guint ax_id, gpointer data)
{
	GtkRtgScaleGizmo *sg = GTK_RTG_SCALE_GIZMO(tripod);
	GtkRtgCamera *cam = gtk_rtg_canvas_camera_get_default(tripod->canvas);
	GtkRtgCamProj proj= gtk_rtg_camera_get_projection(cam);
	GtkRtgCanvasViewPredefined predefined = gtk_rtg_canvas_view_get_predefined(tripod->canvas);

	glDisable(GL_CULL_FACE);

    switch(ax_id)
	{
		case AX_X:	if(proj == GTK_RTG_CAM_PROJ_PERSP)
					{
						/* x-y */
						glColor3ubv((guchar*)&gizmo_sel_color[PLANE_XY]);
						RENDER_ARCS_POLY_FIRST_HALF(sg, PLANE_XY);

						/* x-z */
						glColor3ubv((guchar*)&gizmo_sel_color[PLANE_XZ]);
						RENDER_ARCS_POLY_FIRST_HALF(sg, PLANE_XZ);
					}else
					switch(predefined)
					{
						case GTK_RTG_CANVAS_VIEW_TOP:
						case GTK_RTG_CANVAS_VIEW_BOTTOM:/* x-y */
														glColor3ubv((guchar*)&gizmo_sel_color[PLANE_XY]);
														RENDER_ARCS_POLY_FIRST_HALF(sg, PLANE_XY);
														break;
						case GTK_RTG_CANVAS_VIEW_FRONT:
						case GTK_RTG_CANVAS_VIEW_BACK:	/* x-z */
														glColor3ubv((guchar*)&gizmo_sel_color[PLANE_XZ]);
														RENDER_ARCS_POLY_FIRST_HALF(sg, PLANE_XZ);
														break;
						default: break;
					};

					break;

		case AX_Y:	if(proj == GTK_RTG_CAM_PROJ_PERSP)
					{
                    	/* x-y */
                        glColor3ubv((guchar*)&gizmo_sel_color[PLANE_XY]);
                        RENDER_ARCS_POLY_SECOND_HALF(sg, PLANE_XY);

                        /* y-z */
                        glColor3ubv((guchar*)&gizmo_sel_color[PLANE_YZ]);
                        RENDER_ARCS_POLY_FIRST_HALF(sg, PLANE_YZ);
				}else
				switch(predefined)
				{
					case GTK_RTG_CANVAS_VIEW_TOP:
					case GTK_RTG_CANVAS_VIEW_BOTTOM:	/* x-y */
														glColor3ubv((guchar*)&gizmo_sel_color[PLANE_XY]);
                                                        RENDER_ARCS_POLY_SECOND_HALF(sg, PLANE_XY);
                                                        break;
					case GTK_RTG_CANVAS_VIEW_LEFT:
					case GTK_RTG_CANVAS_VIEW_RIGHT:		/* y-z */
                                                        glColor3ubv((guchar*)&gizmo_sel_color[PLANE_YZ]);
                                                        RENDER_ARCS_POLY_FIRST_HALF(sg, PLANE_YZ);
                                                        break;
					default: break;
				};

				break;

		case AX_Z:	if(proj == GTK_RTG_CAM_PROJ_PERSP)
					{
						/* x-z */
                        glColor3ubv((guchar*)&gizmo_sel_color[PLANE_XZ]);
                        RENDER_ARCS_POLY_SECOND_HALF(sg, PLANE_XZ);

                        /* y-z */
                        glColor3ubv((guchar*)&gizmo_sel_color[PLANE_YZ]);
                        RENDER_ARCS_POLY_SECOND_HALF(sg, PLANE_YZ);

				}else
				switch(predefined)
				{
					case GTK_RTG_CANVAS_VIEW_FRONT:
					case GTK_RTG_CANVAS_VIEW_BACK:	/* x-z */
													glColor3ubv((guchar*)&gizmo_sel_color[PLANE_XZ]);
                                                    RENDER_ARCS_POLY_SECOND_HALF(sg, PLANE_XZ);
                                                    break;

					case GTK_RTG_CANVAS_VIEW_LEFT:
					case GTK_RTG_CANVAS_VIEW_RIGHT:	/* y-z */
                                                    glColor3ubv((guchar*)&gizmo_sel_color[PLANE_YZ]);
                                                    RENDER_ARCS_POLY_SECOND_HALF(sg, PLANE_YZ);
                                                    break;
					default: break;
				};

				break;
	};

    glEnable(GL_CULL_FACE);

	/*
	 * Render dot atop of the axis.
	 */

	gfloat point_size;
	glGetFloatv(GL_POINT_SIZE, &point_size);
	glPointSize(GIZMO_POINT_SIZE);
	glColor3ubv((guchar*)&tripod_sel_color[ax_id]);
	glEnable(GL_POINT_SMOOTH);
	glBegin(GL_POINTS);
		glVertex2i(tripod->ax[ax_id].ax2d_offscreen[AX_END].x, tripod->ax[ax_id].ax2d_offscreen[AX_END].y);
	glEnd();

	if(point_size != GIZMO_POINT_SIZE)
		glPointSize(point_size);

	glDisable(GL_POINT_SMOOTH);
}


static void
gtk_rtg_scale_gizmo_process_motion(GtkRtgTripod *tripod, gint x, gint y)
{
	switch(tripod->color_id)
	{
		case COLOR_ID_GIZMO_PLANE_XY: tripod->selected |= SELECTED_PLANE_XY; break;
		case COLOR_ID_GIZMO_PLANE_XZ: tripod->selected |= SELECTED_PLANE_XZ; break;
		case COLOR_ID_GIZMO_PLANE_YZ: tripod->selected |= SELECTED_PLANE_YZ; break;
		case COLOR_ID_GIZMO_AXES:     tripod->selected |= SELECTED_XYZ;	     break;
	};
}


static void
gtk_rtg_scale_gizmo_process_drag(GtkRtgTripod *tripod, gint x, gint y)
{
	GtkRtgScaleGizmo *sg = GTK_RTG_SCALE_GIZMO(tripod);
	GtkRtgGizmo *gizmo = GTK_RTG_GIZMO(tripod);

	if(gizmo->intersect)
	{
		gfloat scale_factor = 1.f;

		if(tripod->selected != SELECTED_XYZ)
		{
			/* safety check */
			if(X(tripod->delta_offset) >  10.f)	X(tripod->delta_offset) =  10.f;
			else
			if(X(tripod->delta_offset) < -10.f)	X(tripod->delta_offset) = -10.f;

			if(Y(tripod->delta_offset) >  10.f)	Y(tripod->delta_offset) =  10.f;
			else
			if(Y(tripod->delta_offset) < -10.f)	Y(tripod->delta_offset) = -10.f;

			if(Z(tripod->delta_offset) >  10.f)	Z(tripod->delta_offset) =  10.f;
			else
			if(Z(tripod->delta_offset) < -10.f)	Z(tripod->delta_offset) = -10.f;

			/* get right scale factor (depending on camera's distance) */
			GtkRtgCamera *cam   = gtk_rtg_canvas_camera_get_default(tripod->canvas);
			gdouble       dist  = gtk_rtg_camera_view_get_distance(cam);
			gfloat	      span  = gtk_rtg_camera_projection_get_zfar(cam) - gtk_rtg_camera_projection_get_znear(cam);
			gfloat	      ds    = span > 0.f ? gfloat(dist)/span : 0.1f;
			scale_factor = ds > 0.f ? 0.0015f / ds : 0.015f;
		}

		if(gtk_rtg_canvas_view_get_predefined(tripod->canvas) == GTK_RTG_CANVAS_VIEW_CUSTOM)
		{
			switch(tripod->selected)
			{
				case SELECTED_AX_X:	X(tripod->delta_offset) = 1.f + X(tripod->delta_offset) * scale_factor * sg->ax_mirror[AX_X]; Y(tripod->delta_offset)=Z(tripod->delta_offset)=1.f; break;
				case SELECTED_AX_Y:	Y(tripod->delta_offset) = 1.f + Y(tripod->delta_offset) * scale_factor * sg->ax_mirror[AX_Y]; X(tripod->delta_offset)=Z(tripod->delta_offset)=1.f; break;
				case SELECTED_AX_Z:	Z(tripod->delta_offset) = 1.f + Z(tripod->delta_offset) * scale_factor * sg->ax_mirror[AX_Z]; X(tripod->delta_offset)=Y(tripod->delta_offset)=1.f; break;
				case SELECTED_PLANE_XY:
							X(tripod->delta_offset) = 1.f + X(tripod->delta_offset) * scale_factor * sg->ax_mirror[AX_X];
							Y(tripod->delta_offset) = 1.f + Y(tripod->delta_offset) * scale_factor * sg->ax_mirror[AX_Y];
							Z(tripod->delta_offset) = 1.f;
							break;
				case SELECTED_PLANE_XZ:
							X(tripod->delta_offset) = 1.f + X(tripod->delta_offset) * scale_factor * sg->ax_mirror[AX_X];
							Z(tripod->delta_offset) = 1.f + Z(tripod->delta_offset) * scale_factor * sg->ax_mirror[AX_Z];
							Y(tripod->delta_offset) = 1.f;
							break;
				case SELECTED_PLANE_YZ:
							Y(tripod->delta_offset) = 1.f + Y(tripod->delta_offset) * scale_factor * sg->ax_mirror[AX_Y];
							Z(tripod->delta_offset) = 1.f + Z(tripod->delta_offset) * scale_factor * sg->ax_mirror[AX_Z];
							X(tripod->delta_offset) = 1.f;
							break;
				case SELECTED_XYZ:
					{
						gfloat delta_x = gfloat(x-gizmo->point.x);
						gfloat delta_y = gfloat(y-gizmo->point.y);
						gfloat val = 0.f;
						if(fabs(delta_x) >
							fabs(delta_y))	val = 1.f + delta_x * 0.005f;
						else
							val = 1.f + delta_y * 0.005f;

						X(tripod->delta_offset) = Y(tripod->delta_offset) = Z(tripod->delta_offset) = val;

						break;
					}
			};
		}else
		{
			switch(tripod->selected)
			{
				case SELECTED_AX_X:	X(tripod->delta_offset) = 1.f + X(tripod->delta_offset) * scale_factor; Y(tripod->delta_offset)=Z(tripod->delta_offset)=1.f; break;
				case SELECTED_AX_Y:	Y(tripod->delta_offset) = 1.f + Y(tripod->delta_offset) * scale_factor; X(tripod->delta_offset)=Z(tripod->delta_offset)=1.f; break;
				case SELECTED_AX_Z:	Z(tripod->delta_offset) = 1.f + Z(tripod->delta_offset) * scale_factor; X(tripod->delta_offset)=Y(tripod->delta_offset)=1.f; break;
				case SELECTED_PLANE_XY:
							X(tripod->delta_offset) = 1.f + X(tripod->delta_offset) * scale_factor;
							Y(tripod->delta_offset) = 1.f + Y(tripod->delta_offset) * scale_factor;
							Z(tripod->delta_offset) = 1.f;
							break;
				case SELECTED_PLANE_XZ:
							X(tripod->delta_offset) = 1.f + X(tripod->delta_offset) * scale_factor;
							Z(tripod->delta_offset) = 1.f + Z(tripod->delta_offset) * scale_factor;
							Y(tripod->delta_offset) = 1.f;
							break;
				case SELECTED_PLANE_YZ:
							Y(tripod->delta_offset) = 1.f + Y(tripod->delta_offset) * scale_factor;
							Z(tripod->delta_offset) = 1.f + Z(tripod->delta_offset) * scale_factor;
							X(tripod->delta_offset) = 1.f;
							break;
				case SELECTED_XYZ:
					{
						gfloat delta_x = gfloat(x-gizmo->point.x);
						gfloat delta_y = gfloat(y-gizmo->point.y);

						gfloat	val = 0.f;
						if(fabs(delta_x) >
							fabs(delta_y))	val = 1.f + delta_x * 0.005f;
						else
							val = 1.f + delta_y * 0.005f;

						X(tripod->delta_offset) = Y(tripod->delta_offset) = Z(tripod->delta_offset) = val;

						break;
					}
			};
		}

		X(tripod->delta_offset) = fabs(X(tripod->delta_offset));
		Y(tripod->delta_offset) = fabs(Y(tripod->delta_offset));
		Z(tripod->delta_offset) = fabs(Z(tripod->delta_offset));

		if(X(tripod->delta_offset) < 0.2f) X(tripod->delta_offset) = 0.2f;
		else
		if(X(tripod->delta_offset) > 1.2f) X(tripod->delta_offset) = 1.2f;

		if(Y(tripod->delta_offset) < 0.2f) Y(tripod->delta_offset) = 0.2f;
		else
		if(Y(tripod->delta_offset) > 1.2f) Y(tripod->delta_offset) = 1.2f;

		if(Z(tripod->delta_offset) < 0.2f) Z(tripod->delta_offset) = 0.2f;
		else
		if(Z(tripod->delta_offset) > 1.2f) Z(tripod->delta_offset) = 1.2f;

		gizmo->point.x = x;
		gizmo->point.y = y;

	}else
		X(tripod->delta_offset)=Y(tripod->delta_offset)=Z(tripod->delta_offset)=1.f;
}
