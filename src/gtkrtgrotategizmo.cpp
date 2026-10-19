#include "gtkrtgrotategizmo.h"
#include <gtkrtg/gtkrtgmath.h>
#include "gtkrtgglu.h"
#include "gtkrtgutils.h"
#include "trackball.h"

G_DEFINE_TYPE(GtkRtgRotateGizmo, gtk_rtg_rotate_gizmo, GTK_RTG_TYPE_GIZMO)

using namespace gtkrtg;
using namespace std;

static void	gtk_rtg_rotate_gizmo_project(GtkRtgTripod *tripod, point3f *offset, gpointer data);
static void	gtk_rtg_rotate_gizmo_render(GtkRtgTripod *tripod);
static void	gtk_rtg_rotate_gizmo_render_axis(GtkRtgTripod *tripod, guint ax_id, gpointer data);
static void	gtk_rtg_rotate_gizmo_render_last(GtkRtgTripod *tripod);
static void	gtk_rtg_rotate_gizmo_render_offscreen(GtkRtgTripod *tripod);
static void	gtk_rtg_rotate_gizmo_render_axis_offscreen(GtkRtgTripod *tripod, guint ax_id, gpointer data);
static void	gtk_rtg_rotate_gizmo_process_drag(GtkRtgTripod *tripod, gint x, gint y);
static void	gtk_rtg_tripod_update_selection(GtkRtgTripod *tripod);

static void render_circle_axes_persp(GtkRtgRotateGizmo *rg, guint id, gboolean offscreen=TRUE);
static void render_circle_axes_ortho(GtkRtgRotateGizmo *rg, guint id, gboolean offscreen=TRUE);
static void render_circle_offscreen(GtkRtgRotateGizmo *rg);

extern guint tripod_sel_color[AXES];
static guint gizmo_color[]={ COLOR_ID_GIZMO_PLANE_CAMERA,
                             COLOR_ID_GIZMO_CIRCLE };

static point3f point_ax3d[AXES][AXIS_POINTS] =
{
    { point3f(0.f, 0.f, 0.f), point3f(0.4f, 0.f,  0.f)},
    { point3f(0.f, 0.f, 0.f), point3f(0.f,  0.4f, 0.f)},
	{ point3f(0.f, 0.f, 0.f), point3f(0.f,  0.f,  0.4f)}
};
/*
static point3f point_ax3d_offscreen_ortho[AXES][AXIS_POINTS]=
{
	{ point3f( .4f, 0.f,  0.f),  point3f(0.4f, 0.f,  0.f)},
	{ point3f(0.f,   .4f, 0.f),  point3f(0.f,  0.4f, 0.f)},
	{ point3f(0.f,  0.f,   .4f), point3f(0.f,  0.f,  0.4f)}
};
*/

static point3f circle[GIZMO_PLANES+1][CIRCLE_VERTS] =
{
	{/* PLANE_XY */
		point3f( 0.95f,  0.00f, 0.00f), point3f( 0.93f,  0.19f, 0.00f), point3f( 0.87f,  0.37f, 0.00f), point3f( 0.79f,  0.53f, 0.00f),
		point3f( 0.67f,  0.67f, 0.00f), point3f( 0.53f,  0.79f, 0.00f), point3f( 0.37f,  0.87f, 0.00f), point3f( 0.19f,  0.93f, 0.00f),
		point3f( 0.00f,  0.95f, 0.00f), point3f(-0.19f,  0.93f, 0.00f), point3f(-0.37f,  0.87f, 0.00f), point3f(-0.53f,  0.79f, 0.00f),
		point3f(-0.67f,  0.67f, 0.00f), point3f(-0.79f,  0.53f, 0.00f), point3f(-0.87f,  0.37f, 0.00f), point3f(-0.93f,  0.19f, 0.00f),
		point3f(-0.95f,  0.00f, 0.00f), point3f(-0.93f, -0.19f, 0.00f), point3f(-0.87f, -0.37f, 0.00f), point3f(-0.79f, -0.53f, 0.00f),
		point3f(-0.67f, -0.67f, 0.00f), point3f(-0.53f, -0.79f, 0.00f), point3f(-0.37f, -0.87f, 0.00f), point3f(-0.19f, -0.93f, 0.00f),
		point3f( 0.00f, -0.95f, 0.00f), point3f( 0.19f, -0.93f, 0.00f), point3f( 0.37f, -0.87f, 0.00f), point3f( 0.53f, -0.79f, 0.00f),
		point3f( 0.67f, -0.67f, 0.00f), point3f( 0.79f, -0.53f, 0.00f), point3f( 0.87f, -0.37f, 0.00f), point3f( 0.93f, -0.19f, 0.00f)
	},
	{/* PLANE_XZ */
		point3f( 0.95f, 0.00f,  0.00f), point3f( 0.93f, 0.00f,  0.19f), point3f( 0.87f, 0.00f,  0.37f), point3f( 0.79f, 0.00f,  0.53f),
		point3f( 0.67f, 0.00f,  0.67f), point3f( 0.53f, 0.00f,  0.79f), point3f( 0.37f, 0.00f,  0.87f), point3f( 0.19f, 0.00f,  0.93f),
		point3f( 0.00f, 0.00f,  0.95f), point3f(-0.19f, 0.00f,  0.93f), point3f(-0.37f, 0.00f,  0.87f), point3f(-0.53f, 0.00f,  0.79f),
		point3f(-0.67f, 0.00f,  0.67f), point3f(-0.79f, 0.00f,  0.53f), point3f(-0.87f, 0.00f,  0.37f), point3f(-0.93f, 0.00f,  0.19f),
		point3f(-0.95f, 0.00f,  0.00f), point3f(-0.93f, 0.00f, -0.19f), point3f(-0.87f, 0.00f, -0.37f), point3f(-0.79f, 0.00f, -0.53f),
		point3f(-0.67f, 0.00f, -0.67f), point3f(-0.53f, 0.00f, -0.79f), point3f(-0.37f, 0.00f, -0.87f), point3f(-0.19f, 0.00f, -0.93f),
		point3f( 0.00f, 0.00f, -0.95f), point3f( 0.19f, 0.00f, -0.93f), point3f( 0.37f, 0.00f, -0.87f), point3f( 0.53f, 0.00f, -0.79f),
		point3f( 0.67f, 0.00f, -0.67f), point3f( 0.79f, 0.00f, -0.53f), point3f( 0.87f, 0.00f, -0.37f), point3f( 0.93f, 0.00f, -0.19f)
	},
	{/* PLANE_YZ */
		point3f(0.00f,  0.00f, -0.95f), point3f(0.00f,  0.19f, -0.93f), point3f(0.00f,  0.37f, -0.87f), point3f(0.00f,  0.53f, -0.79f),
		point3f(0.00f,  0.67f, -0.67f), point3f(0.00f,  0.79f, -0.53f), point3f(0.00f,  0.87f, -0.37f), point3f(0.00f,  0.93f, -0.19f),
		point3f(0.00f,  0.95f,  0.00f), point3f(0.00f,  0.93f,  0.19f), point3f(0.00f,  0.87f,  0.37f), point3f(0.00f,  0.79f,  0.53f),
		point3f(0.00f,  0.67f,  0.67f), point3f(0.00f,  0.53f,  0.79f), point3f(0.00f,  0.37f,  0.87f), point3f(0.00f,  0.19f,  0.93f),
		point3f(0.00f,  0.00f,  0.95f), point3f(0.00f, -0.19f,  0.93f), point3f(0.00f, -0.37f,  0.87f), point3f(0.00f, -0.53f,  0.79f),
		point3f(0.00f, -0.67f,  0.67f), point3f(0.00f, -0.79f,  0.53f), point3f(0.00f, -0.87f,  0.37f), point3f(0.00f, -0.93f,  0.19f),
		point3f(0.00f, -0.95f,  0.00f), point3f(0.00f, -0.93f, -0.19f), point3f(0.00f, -0.87f, -0.37f), point3f(0.00f, -0.79f, -0.53f),
		point3f(0.00f, -0.67f, -0.67f), point3f(0.00f, -0.53f, -0.79f), point3f(0.00f, -0.37f, -0.87f), point3f(0.00f, -0.19f, -0.93f)
	},
	{/* PLANE_CAMERA */
		point3f( 1.17f,  0.00f, 0.00f), point3f( 1.15f,  0.23f, 0.00f), point3f( 1.08f,  0.46f, 0.00f), point3f( 0.97f,  0.66f, 0.00f),
		point3f( 0.83f,  0.83f, 0.00f), point3f( 0.66f,  0.97f, 0.00f), point3f( 0.46f,  1.08f, 0.00f), point3f( 0.23f,  1.15f, 0.00f),
		point3f( 0.00f,  1.17f, 0.00f), point3f(-0.23f,  1.15f, 0.00f), point3f(-0.46f,  1.08f, 0.00f), point3f(-0.66f,  0.97f, 0.00f),
		point3f(-0.83f,  0.83f, 0.00f), point3f(-0.97f,  0.66f, 0.00f), point3f(-1.08f,  0.46f, 0.00f), point3f(-1.15f,  0.23f, 0.00f),
		point3f(-1.17f,  0.00f, 0.00f), point3f(-1.15f, -0.23f, 0.00f), point3f(-1.08f, -0.46f, 0.00f), point3f(-0.97f, -0.66f, 0.00f),
		point3f(-0.83f, -0.83f, 0.00f), point3f(-0.66f, -0.97f, 0.00f), point3f(-0.46f, -1.08f, 0.00f), point3f(-0.23f, -1.15f, 0.00f),
		point3f( 0.00f, -1.17f, 0.00f), point3f( 0.23f, -1.15f, 0.00f), point3f( 0.46f, -1.08f, 0.00f), point3f( 0.66f, -0.97f, 0.00f),
		point3f( 0.83f, -0.83f, 0.00f), point3f( 0.97f, -0.66f, 0.00f), point3f( 1.08f, -0.46f, 0.00f), point3f( 1.15f, -0.23f, 0.00f)
	}
};

/* circle radius */
#define R_CIRCLE_PLANE_X        0.95f
#define R_CIRCLE_PLANE_Y        R_CIRCLE_PLANE_X
#define R_CIRCLE_PLANE_Z        R_CIRCLE_PLANE_X
#define R_CIRCLE_PLANE_CAMERA   1.17f
#define POINT_SIZE              4.5f

#define PLANE_CAMERA	        GIZMO_PLANES
#define AX_DIMMED_COLOR         "#636363"
#define AX_OUTER_COLOR          "#aaaaaa"
#define LINE_WIDTH				1.f
#define LINE_WIDTH_OFFSCREEN    24.f

/* text position */
static point3f text_pos_persp[AXES] = { point3f(0.45f, 0.f,  0.f), point3f(0.f, 0.45f, 0.f), point3f(0.f,  0.f,  0.45f) };
static point3f text_pos_ortho[AXES] = { point3f(0.45f, 0.f,  0.f), point3f(0.f, 0.45f, 0.f), point3f(0.f,  0.f,  0.45f) };

static void
gtk_rtg_rotate_gizmo_class_init(GtkRtgRotateGizmoClass *klass)
{
	GtkRtgTripodClass *tripod_class = GTK_RTG_TRIPOD_CLASS(klass);

	tripod_class->render = gtk_rtg_rotate_gizmo_render;
	tripod_class->render_last = gtk_rtg_rotate_gizmo_render_last;
	tripod_class->update_selection = gtk_rtg_tripod_update_selection;
	tripod_class->process_drag = gtk_rtg_rotate_gizmo_process_drag;
}

GtkRtgRotateGizmo *
gtk_rtg_rotate_gizmo_new(GtkRtgCanvas *canvas, gchar *font_desc)
{
	GtkRtgRotateGizmo *rg = GTK_RTG_ROTATE_GIZMO(g_object_new(GTK_RTG_TYPE_ROTATE_GIZMO, "font-desc", font_desc,
											     	 	 	 	 	 	 	 	 	 	 "canvas",    canvas, NULL));
	GtkRtgTripod *tripod = GTK_RTG_TRIPOD(rg);

	/* overwrite text colors */
	for(guint i=0; i<AXES; ++i)
		g_object_set(tripod->ax[i].text, "color", &rg->ax_dimmed_color, NULL);

	return rg;
}

static void
gtk_rtg_rotate_gizmo_init(GtkRtgRotateGizmo *rg)
{
	GtkRtgTripod *tripod = GTK_RTG_TRIPOD(rg);
	GtkRtgGizmo  *gizmo  = GTK_RTG_GIZMO(rg);

	g_signal_connect(rg, "project",					G_CALLBACK(gtk_rtg_rotate_gizmo_project),				NULL);
	g_signal_connect(rg, "render-axis",				G_CALLBACK(gtk_rtg_rotate_gizmo_render_axis),			NULL);
	g_signal_connect(rg, "render-offscreen",		G_CALLBACK(gtk_rtg_rotate_gizmo_render_offscreen),		NULL);
	g_signal_connect(rg, "render-axis-offscreen",	G_CALLBACK(gtk_rtg_rotate_gizmo_render_axis_offscreen),	NULL);

	/* set axis */
	memcpy(gizmo->point_ax3d_ortho, point_ax3d, sizeof(point_ax3d));
	memcpy(gizmo->point_ax3d_persp, point_ax3d, sizeof(point_ax3d));

	/* text pos */
	memcpy(gizmo->text_pos_persp, text_pos_persp, sizeof(text_pos_persp));
	memcpy(gizmo->text_pos_ortho, text_pos_ortho, sizeof(text_pos_ortho));

	/* no arrows */
	gizmo->arrow = 0;

	/* do not render offscreen axes */
	tripod->offscreen = 0;

	/* Dragging the object uses the camera-oriented trackball. */
	tripod->selected = SELECTED_CIRCLE;
	tripod->sel_fallback = SELECTED_CIRCLE;

	/* render parallel also */
	tripod->skip_parallel_ax = 0;

	/* outline */
	rg->outline = 1;

	/* ax dimmed color */
	gdk_color_parse(AX_DIMMED_COLOR, &rg->ax_dimmed_color);

	/* outer color */
	gdk_color_parse(AX_OUTER_COLOR, &rg->ax_outer_color);

	/* set dimmed colors */
	for(guint i=0; i<AXES; ++i)
		tripod->ax[i].c_ax = &rg->ax_dimmed_color;

	/* swap x-z colors */
	rg->colors[AX_X] = &tripod->colors[AX_Z];
	rg->colors[AX_Y] = &tripod->colors[AX_Y];
	rg->colors[AX_Z] = &tripod->colors[AX_X];

	/* identity */
	rg->quat.identity();
}

/*
 * Publics (lib only)
 *
 */
static void
gtk_rtg_rotate_gizmo_project(GtkRtgTripod *tripod, point3f *offset, gpointer data)
{
	GtkRtgRotateGizmo *rg = GTK_RTG_ROTATE_GIZMO(tripod);
	GtkRtgGizmo *gizmo = GTK_RTG_GIZMO(tripod);
	GtkRtgCamera *camera = gtk_rtg_canvas_camera_get_default(tripod->canvas);

	if(PROJECTION(camera) == GTK_RTG_CAM_PROJ_PERSP)
	{
		vector3f dir = cast3<float, double>(*gtk_rtg_camera_view_get_eye(camera)) - *offset;

		matrix4x4f orient;
		vector3f v = vector3f(0.f, 0.f, 1.f);
		utils_setup_orientation_matrix<float>(dir.norm(), v, orient);

		orient.set_translation(*offset);
		for(guint i=0; i<CIRCLE_VERTS; ++i)
		{
			/* outline */
			point3f point_orient = orient * (circle[PLANE_XY][i] * gizmo->persp_scale);
			gluProjectf(point_orient, tripod->matrixproject.m, gizmo->viewport, rg->circle_outline[i]);

			/* outer */
			point_orient = orient * (circle[PLANE_CAMERA][i] * gizmo->persp_scale);
			gluProjectf(point_orient, tripod->matrixproject.m, gizmo->viewport, rg->circle_outer[i]);
		}

		/* planes */
		for(guint i=0; i<GIZMO_PLANES; ++i)
			for(guint j=0; j<CIRCLE_VERTS; ++j)
			{
				point3f  point_orient = (circle[i][j] * gizmo->persp_scale) + *offset;
				vector3f vec_orient   = point_orient - *offset;

				gluProjectf(point_orient, tripod->matrixproject.m, gizmo->viewport, rg->circle_plane[i][j].point_2d);

				if(dir.dot(vec_orient.norm()) > 0.f)	/* test if point is at the viewers side of a plane */
					rg->circle_plane[i][j].front = 1;
				else
					rg->circle_plane[i][j].front = 0;
			}
	}else
	/* orthographic */
	{
		/* planes */
		for(guint i=0; i<GIZMO_PLANES; ++i)
			for(guint j=0; j<CIRCLE_VERTS; ++j)
			{
				point3f  point_orient = gizmo->matrixproject * circle[i][j];

				utils_unproject_point2screen(point_orient, gizmo->alphabeta, gizmo->alphabeta, rg->circle_plane[i][j].point_2d);
				rg->circle_plane[i][j].point_2d.y = gizmo->size - rg->circle_plane[i][j].point_2d.y;

				/* translate to offset */
				rg->circle_plane[i][j].point_2d.x += gizmo->xt;
				rg->circle_plane[i][j].point_2d.y += gizmo->yt;
			}
	}

	/*
	 * Get fall-back selection
	 *
	 */
	if(tripod->fallback)
		tripod->sel_fallback = tripod->selected = SELECTED_CIRCLE;

	/*
	 * get inverse.
	 */
	__gluInvertMatrixf(M_VIEW(camera).m, rg->mcam2world.m);
	rg->mcam2world[tx]=rg->mcam2world[ty]=rg->mcam2world[tz]=0.f;

	/*
	 * get center
	 */
	point3f point = tripod->offset;
	gluProjectf(point, tripod->matrixproject.m, gizmo->viewport, rg->center);
}


static void
gtk_rtg_rotate_gizmo_render_axis(GtkRtgTripod *tripod, guint ax_id, gpointer data)
{
	GtkRtgRotateGizmo *rg = GTK_RTG_ROTATE_GIZMO(tripod);
	GtkRtgCamera *camera = gtk_rtg_canvas_camera_get_default(tripod->canvas);

	if(PROJECTION(camera) == GTK_RTG_CAM_PROJ_PERSP)
		render_circle_axes_persp(rg, ax_id, FALSE);
	else
	{
		/* order matters */
		switch(gtk_rtg_canvas_view_get_predefined(tripod->canvas))
		{
			case GTK_RTG_CANVAS_VIEW_TOP:
			case GTK_RTG_CANVAS_VIEW_BOTTOM: switch(ax_id)
											 {
												case AX_X: render_circle_axes_ortho(rg, AX_Y, FALSE); break;
												case AX_Y: render_circle_axes_ortho(rg, AX_Z, FALSE); break;
												case AX_Z: render_circle_axes_ortho(rg, AX_X, FALSE); break;
											 };
											 break;

			case GTK_RTG_CANVAS_VIEW_LEFT:
			case GTK_RTG_CANVAS_VIEW_RIGHT:	 switch(ax_id)
											 {
												case AX_X: render_circle_axes_ortho(rg, AX_Z, FALSE); break;
												case AX_Y: render_circle_axes_ortho(rg, AX_X, FALSE); break;
												case AX_Z: render_circle_axes_ortho(rg, AX_Y, FALSE); break;
											 };
											 break;

			default: render_circle_axes_ortho(rg, ax_id, FALSE); break;
		};
	}
}

#define RENDER_MARKER(rg) \
{ \
        glColor3usv(&GTK_RTG_TRIPOD(rg)->colors[AXES].red); \
 \
        glBegin(GL_POINTS); \
                glVertex2i(rg->pc.x, rg->pc.y); \
        glEnd(); \
 \
        glColor3usv(&rg->ax_dimmed_color.red); \
 \
        glBegin(GL_LINES); \
        glVertex2i(rg->center.x, rg->center.y); \
        glVertex2i(rg->pc.x, rg->pc.y); \
        glEnd(); \
}

static void
gtk_rtg_rotate_gizmo_render_last(GtkRtgTripod *tripod)
{
	GtkRtgRotateGizmo *rg = GTK_RTG_ROTATE_GIZMO(tripod);
	GtkRtgGizmo *gizmo = GTK_RTG_GIZMO(tripod);
	GtkRtgCamera *camera = gtk_rtg_canvas_camera_get_default(tripod->canvas);

	gfloat line_width;
	glGetFloatv(GL_LINE_WIDTH, &line_width);
	glLineWidth(LINE_WIDTH);

	gfloat point_size;
	glGetFloatv(GL_POINT_SIZE, &point_size);
	glPointSize(POINT_SIZE);
	glEnable(GL_POINT_SMOOTH);


	if(PROJECTION(camera) == GTK_RTG_CAM_PROJ_PERSP)
	{
		/*
		 * A tangent line
		 */

		if(tripod->drag && tripod->selected != SELECTED_CIRCLE)
		{
			if(tripod->selected != SELECTED_PLANE_CAMERA)
			{
				if(gizmo->cam_offset_angle < 0.99f)
				{
					glColor3usv(&tripod->colors[AXES].red);

					glBegin(GL_POINTS);
						glVertex2i(rg->pc.x, rg->pc.y);
					glEnd();

					glColor3usv(&rg->ax_dimmed_color.red);

					glBegin(GL_LINES);
						glVertex2i(rg->center.x, rg->center.y);
						glVertex2i(rg->pc.x, rg->pc.y);
					glEnd();
				}
			}else
			{
					glColor3usv(&tripod->colors[AXES].red);

					glBegin(GL_POINTS);
						glVertex2i(rg->pc.x, rg->pc.y);
					glEnd();

					glColor3usv(&rg->ax_dimmed_color.red);

					glBegin(GL_LINES);
						glVertex2i(rg->center.x, rg->center.y);
						glVertex2i(rg->pc.x, rg->pc.y);
					glEnd();
			}
		}
	}else
	/* ORTHOGRAPHIC */
	{
		if(tripod->drag)
		{
			switch(gtk_rtg_canvas_view_get_predefined(tripod->canvas))
			{
				case GTK_RTG_CANVAS_VIEW_TOP:
				case GTK_RTG_CANVAS_VIEW_BOTTOM:  if(tripod->selected == SELECTED_AX_X) RENDER_MARKER(rg);
												  break;

				case GTK_RTG_CANVAS_VIEW_LEFT:
				case GTK_RTG_CANVAS_VIEW_RIGHT:	  if(tripod->selected == SELECTED_AX_Z) RENDER_MARKER(rg);
												  break;

				case GTK_RTG_CANVAS_VIEW_FRONT:
				case GTK_RTG_CANVAS_VIEW_BACK:	  if(tripod->selected == SELECTED_AX_Y) RENDER_MARKER(rg);
												  break;
				default: break;
			};
		}
	}

	if(line_width != LINE_WIDTH)
		glLineWidth(line_width);

	glDisable(GL_POINT_SMOOTH);

	if(point_size != POINT_SIZE)
		glPointSize(point_size);

	glColor3f(1.f, 1.f, 1.f);
}


static void
gtk_rtg_rotate_gizmo_render(GtkRtgTripod *tripod)
{
	GtkRtgRotateGizmo *rg = GTK_RTG_ROTATE_GIZMO(tripod);
	GtkRtgGizmo *gizmo = GTK_RTG_GIZMO(tripod);
	GtkRtgCamera *camera = gtk_rtg_canvas_camera_get_default(tripod->canvas);

	if(PROJECTION(camera) == GTK_RTG_CAM_PROJ_PERSP)
	{
		gfloat line_width;
		glGetFloatv(GL_LINE_WIDTH, &line_width);
		glLineWidth(LINE_WIDTH);

		/* outline */
		if(rg->outline)
		{
			glColor3usv(&rg->ax_dimmed_color.red);
			glBegin(GL_LINE_LOOP);
				for(guint i=0; i<CIRCLE_VERTS; ++i)
					glVertex2i(rg->circle_outline[i].x, rg->circle_outline[i].y);
			glEnd();
		}

		if(tripod->selected & SELECTED_CIRCLE)
		{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_BLEND);
				glDisable(GL_CULL_FACE);
				glColor4f(gizmo->color_plane[0], gizmo->color_plane[1], gizmo->color_plane[2], 0.175f);

				glBegin(GL_TRIANGLE_FAN);
				for(guint i=0; i<CIRCLE_VERTS; ++i)
						glVertex2i(rg->circle_outline[i].x, rg->circle_outline[i].y);
				glEnd();

				glDisable(GL_BLEND);
				glEnable(GL_CULL_FACE);
		}

		/* outer */
		if(tripod->selected & SELECTED_PLANE_CAMERA)
			glColor3usv(&tripod->colors[AXES].red);
		else
			glColor3usv(&rg->ax_outer_color.red);

		glBegin(GL_LINE_LOOP);
			for(guint i=0; i<CIRCLE_VERTS; ++i)
				glVertex2i(rg->circle_outer[i].x, rg->circle_outer[i].y);
		glEnd();

		/* reset flag */
		if(!rg->outline) rg->outline = 1;

		if(line_width != LINE_WIDTH)
			glLineWidth(line_width);

	}else
	/* ORTHOGRAPHIC */
	{
		if(tripod->selected & SELECTED_CIRCLE)
		{
			guint id = 0;
			switch(gtk_rtg_canvas_view_get_predefined(tripod->canvas))
			{
				case GTK_RTG_CANVAS_VIEW_TOP:
				case GTK_RTG_CANVAS_VIEW_BOTTOM: id = gtk_rtg_canvas_view_get_up(tripod->canvas) == GTK_RTG_CANVAS_VIEW_UP_Y ? AX_Y : AX_X; break;

				case GTK_RTG_CANVAS_VIEW_FRONT:
				case GTK_RTG_CANVAS_VIEW_BACK:   id = gtk_rtg_canvas_view_get_up(tripod->canvas) == GTK_RTG_CANVAS_VIEW_UP_Y ? AX_Z : AX_Y; break;

				case GTK_RTG_CANVAS_VIEW_LEFT:
				case GTK_RTG_CANVAS_VIEW_RIGHT:	 id = gtk_rtg_canvas_view_get_up(tripod->canvas) == GTK_RTG_CANVAS_VIEW_UP_Y ? AX_X : AX_Z; break;

				default: break;
			};

			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
			glDisable(GL_CULL_FACE);
			glColor4f(gizmo->color_plane[0], gizmo->color_plane[1], gizmo->color_plane[2], 0.175f);

			glBegin(GL_TRIANGLE_FAN);
				for(guint i=0; i<CIRCLE_VERTS; ++i)
					glVertex2i(rg->circle_plane[id][i].point_2d.x, rg->circle_plane[id][i].point_2d.y);
			glEnd();

			glDisable(GL_BLEND);
			glEnable(GL_CULL_FACE);
		}
	}

	glColor3f(1.f, 1.f, 1.f);
}

static void
gtk_rtg_rotate_gizmo_render_offscreen(GtkRtgTripod *tripod)
{
	GtkRtgRotateGizmo *rg = GTK_RTG_ROTATE_GIZMO(tripod);

	gfloat line_width;
	glGetFloatv(GL_LINE_WIDTH, &line_width);
	glLineWidth(LINE_WIDTH_OFFSCREEN);

	render_circle_offscreen(rg);

	if(line_width != LINE_WIDTH_OFFSCREEN)
		glLineWidth(line_width);
}

static void
gtk_rtg_rotate_gizmo_render_axis_offscreen(GtkRtgTripod *tripod, guint ax_id, gpointer data)
{
	GtkRtgRotateGizmo *rg = GTK_RTG_ROTATE_GIZMO(tripod);
	GtkRtgCamera *camera = gtk_rtg_canvas_camera_get_default(tripod->canvas);

	if(PROJECTION(camera) == GTK_RTG_CAM_PROJ_PERSP)
	{
		if(!rg->outline)
			rg->outline = 1;

		render_circle_axes_persp(rg, ax_id);
	}else
		render_circle_axes_ortho(rg, ax_id);
}


static void
gtk_rtg_tripod_update_selection(GtkRtgTripod *tripod)
{
	GtkRtgRotateGizmo *rg	= GTK_RTG_ROTATE_GIZMO(tripod);

	/* deselect */
	for(guint i=0; i<AXES; ++i)
	{
		tripod->ax[i].c_ax = &rg->ax_dimmed_color;
		g_object_set(tripod->ax[i].text, "color", &rg->ax_dimmed_color, NULL);
	}

	/* select appropriate */
	switch(tripod->selected)
	{
		case SELECTED_AX_X:  tripod->ax[AX_Z].c_ax = &tripod->colors[AX_Z];
							 g_object_set(tripod->ax[AX_Z].text, "color", &tripod->colors[AX_Z], NULL);
							 break;

		case SELECTED_AX_Y:  tripod->ax[AX_Y].c_ax = &tripod->colors[AX_Y];
							 g_object_set(tripod->ax[AX_Y].text, "color", &tripod->colors[AX_Y], NULL);
							 break;

		case SELECTED_AX_Z:  tripod->ax[AX_X].c_ax = &tripod->colors[AX_X];
							 g_object_set(tripod->ax[AX_X].text, "color", &tripod->colors[AX_X], NULL);
							 break;
	}
}


/*
 * privs
 *
 */

inline static void
connect_circle_front_back(GtkRtgRotateGizmo *rg, guint id)
{
	/*
	 * Connect front and back rings by filling 2 gaps
	 *
	 */

	guint cnt=0;

	/* check for first / last scenario */
	if(rg->circle_plane[id][0].front != rg->circle_plane[id][CIRCLE_VERTS-1].front)
	{
		glBegin(GL_LINES);
			glVertex2i(rg->circle_plane[id][ 0].point_2d.x, rg->circle_plane[id][ 0].point_2d.y);
			glVertex2i(rg->circle_plane[id][CIRCLE_VERTS-1].point_2d.x, rg->circle_plane[id][CIRCLE_VERTS-1].point_2d.y);
		glEnd();

		cnt++;
	}

	/* the rest */
	for(guint i=0; i<CIRCLE_VERTS-1; ++i)
		if(rg->circle_plane[id][i].front != rg->circle_plane[id][i+1].front)
		{
			glBegin(GL_LINES);
				glVertex2i(rg->circle_plane[id][i  ].point_2d.x, rg->circle_plane[id][i  ].point_2d.y);
				glVertex2i(rg->circle_plane[id][i+1].point_2d.x, rg->circle_plane[id][i+1].point_2d.y);
			glEnd();

			if(++cnt==2)
				break;	/* done */
		}
}


inline static void
render_circle_persp_plane(GtkRtgRotateGizmo  *rg, guint id, gboolean front)
{
	gint last_pt = -1;
	gint first_pt = -1;
	gint i = 0;

	glBegin(GL_LINE_STRIP);
		for(; i<CIRCLE_VERTS; ++i)
			if(rg->circle_plane[id][i].front==front)
			{
				if(first_pt == -1)
					first_pt = i;

				if(last_pt != -1 && last_pt != i-1)
					break;

				glVertex2i(rg->circle_plane[id][i].point_2d.x, rg->circle_plane[id][i].point_2d.y);
				last_pt=i;
			}
	glEnd();

	/* continued */
	if(i!=CIRCLE_VERTS)
	{
		guint connect = 0;
		/* second segment */
		glBegin(GL_LINE_STRIP);
			for(; i<CIRCLE_VERTS; ++i)
				if(rg->circle_plane[id][i].front==front)
				{
					glVertex2i(rg->circle_plane[id][i].point_2d.x, rg->circle_plane[id][i].point_2d.y);
					connect = i;
				}
		glEnd();

		/* connection */
		glBegin(GL_LINES);
			glVertex2i(rg->circle_plane[id][first_pt].point_2d.x, rg->circle_plane[id][first_pt].point_2d.y);
			glVertex2i(rg->circle_plane[id][connect].point_2d.x, rg->circle_plane[id][connect].point_2d.y);
		glEnd();
	}
}


inline static void
render_circle_axes_persp(GtkRtgRotateGizmo *rg, guint id, gboolean offscreen)
{
    GtkRtgTripod *tripod = GTK_RTG_TRIPOD(rg);
	gfloat line_width;

	glGetFloatv(GL_LINE_WIDTH, &line_width);

	if(offscreen)
		glLineWidth(LINE_WIDTH_OFFSCREEN);
	else
		glLineWidth(LINE_WIDTH);

	/* reset backline */
	if(rg->backline[id]) rg->backline[id] = FALSE;

	/* set color */
	if(offscreen)
		glColor3ubv((guchar*)&tripod_sel_color[id]);
	else
	{
		if(tripod->selected & tripod->ax[id].sel_flag)
			glColor3usv(&tripod->colors[AXES].red);
		else
			glColor3usv(&rg->colors[id]->red);
	}

	/* render front */
	render_circle_persp_plane(rg, id, TRUE);

	/*
	 * Check weather axis back line will match better than outline
	 *
	 */

	if(rg->outline)
	{
		/* test arc-cosines */
		if(   (tripod->ax[id].angle >  0.707f && tripod->ax[id].angle <=  1.f)		/* angle >=0  && angle <  45.f  */
		   || (tripod->ax[id].angle < -0.707f && tripod->ax[id].angle >= -1.f))		/* angle >13 && angle <= 180.f */
		{
			guint ax_id=AX_X; /* case AX_Z: */
			switch(id)
			{
				/* map ax id */
				case AX_X: ax_id = AX_Z; break;
				case AX_Y: ax_id = AX_Y; break;
			}

			/* backline rendered */
			rg->backline[ax_id] = TRUE;

			/* set color */
			if(offscreen)
				glColor3ubv((guchar*)&tripod_sel_color[ax_id]);
			else
			{
				if(tripod->selected & tripod->ax[ax_id].sel_flag)
					glColor3usv(&tripod->colors[AXES].red);
				else
					glColor3usv(&rg->colors[ax_id]->red);
			}

			/* render back-outline */
			render_circle_persp_plane(rg, ax_id, FALSE);

			/* connect */
			connect_circle_front_back(rg, ax_id);

			if(!offscreen)
				rg->outline = 0;
		}
	}

	if(offscreen)
	{
		if(LINE_WIDTH_OFFSCREEN != line_width)
			glLineWidth(line_width);
	}else
	{
		/*
		 * Dragging: render backline in dimmed color.
		 */
		if(tripod->drag && (tripod->selected & tripod->ax[id].sel_flag) && rg->backline[id] == FALSE)
		{
			glColor3usv(&rg->ax_dimmed_color.red);
			render_circle_persp_plane(rg, id, FALSE);
			connect_circle_front_back(rg, id);
		}

		if(LINE_WIDTH != line_width)
			glLineWidth(line_width);
	}
}


inline static void
render_circle_axes_ortho(GtkRtgRotateGizmo *rg, guint id, gboolean offscreen)
{
	GtkRtgTripod *tripod = GTK_RTG_TRIPOD(rg);
	gfloat line_width;

	glGetFloatv(GL_LINE_WIDTH, &line_width);

	if(offscreen)
	{
		glLineWidth(LINE_WIDTH_OFFSCREEN);
		glColor3ubv((guchar*)&tripod_sel_color[id]);
	}else
	{
		glLineWidth(LINE_WIDTH);
		if(tripod->selected & tripod->ax[id].sel_flag)
			glColor3usv(&tripod->colors[AXES].red);
		else
			glColor3usv(&rg->colors[id]->red);
	}

	glBegin(GL_LINE_LOOP);
		for(guint i=0; i<CIRCLE_VERTS; ++i)
			glVertex2i(rg->circle_plane[id][i].point_2d.x, rg->circle_plane[id][i].point_2d.y);
	glEnd();

	if(offscreen)
	{
		if(LINE_WIDTH_OFFSCREEN != line_width)
			glLineWidth(line_width);
	}else
	{
		if(LINE_WIDTH != line_width)
			glLineWidth(line_width);
	}
}

inline static void
render_circle_offscreen(GtkRtgRotateGizmo *rg)
{
        GtkRtgTripod *tripod = GTK_RTG_TRIPOD(rg);
        GtkRtgCamera *camera = gtk_rtg_canvas_camera_get_default(tripod->canvas);

        glDisable(GL_CULL_FACE);
        glColor3ubv((guchar*)&gizmo_color[1]);

        if(PROJECTION(camera) == GTK_RTG_CAM_PROJ_PERSP)
        {
			glColor3ubv((guchar*)&gizmo_color[0]);

			glBegin(GL_LINE_LOOP);
				for(guint i=0; i<CIRCLE_VERTS; ++i)
					glVertex2i(rg->circle_outer[i].x, rg->circle_outer[i].y);
			glEnd();

			glColor3ubv((guchar*)&gizmo_color[1]);

			glBegin(GL_TRIANGLE_FAN);
				for(guint i=0; i<CIRCLE_VERTS; ++i)
					glVertex2i(rg->circle_outline[i].x, rg->circle_outline[i].y);
			glEnd();
        }else
        {
			guint id;
			switch(gtk_rtg_canvas_view_get_predefined(tripod->canvas))
			{
				case GTK_RTG_CANVAS_VIEW_TOP:
				case GTK_RTG_CANVAS_VIEW_BOTTOM: id = gtk_rtg_canvas_view_get_up(tripod->canvas) == GTK_RTG_CANVAS_VIEW_UP_Y ? AX_Y : AX_X; break;

				case GTK_RTG_CANVAS_VIEW_FRONT:
				case GTK_RTG_CANVAS_VIEW_BACK:   id = gtk_rtg_canvas_view_get_up(tripod->canvas) == GTK_RTG_CANVAS_VIEW_UP_Y ? AX_Z : AX_Y; break;

				default:                         id = gtk_rtg_canvas_view_get_up(tripod->canvas) == GTK_RTG_CANVAS_VIEW_UP_Y ? AX_X : AX_Z; break;
			};

			glBegin(GL_TRIANGLE_FAN);
				for(guint i=0; i<CIRCLE_VERTS; ++i)
					glVertex2i(rg->circle_plane[id][i].point_2d.x, rg->circle_plane[id][i].point_2d.y);
			glEnd();
        }

        glEnable(GL_CULL_FACE);
}

static gboolean
rotation_plane_point(GtkRtgGizmo *gizmo, gint x, gint y, plane<float> &plane, point3f &point)
{
	point3f near_point, far_point;
	if(!gluUnProjectf(x, y, 0.f, gizmo->minverse.m, gizmo->viewport,
					  &X(near_point), &Y(near_point), &Z(near_point)) ||
	   !gluUnProjectf(x, y, 1.f, gizmo->minverse.m, gizmo->viewport,
					  &X(far_point), &Y(far_point), &Z(far_point)))
		return FALSE;
	const vector3f ray = far_point - near_point;
	if(fabsf(plane.normal.dot(ray)) < ray.mag() * .0001f)
		return FALSE;
	return intersect_segment_plane(near_point, far_point, plane, point);
}

static void
gtk_rtg_rotate_gizmo_process_drag(GtkRtgTripod *tripod, gint x, gint y)
{
	GtkRtgRotateGizmo *rg = GTK_RTG_ROTATE_GIZMO(tripod);
	GtkRtgGizmo *gizmo = GTK_RTG_GIZMO(tripod);
	GtkRtgCamera *camera = gtk_rtg_canvas_camera_get_default(tripod->canvas);
	rg->quat.identity();
	gizmo->intersect = FALSE;

	if(!tripod->drag)
	{
		tripod->drag = TRUE;
		gizmo->point = {x, y};
		return;
	}

	if(tripod->selected == SELECTED_CIRCLE)
	{
		const float width = gtk_widget_get_allocated_width(GTK_WIDGET(tripod->canvas));
		const float height = gtk_widget_get_allocated_height(GTK_WIDGET(tripod->canvas));
		trackball(VALUES(rg->quat), (gizmo->point.x - rg->center.x) / width,
				  (gizmo->point.y - rg->center.y) / height,
				  (x - rg->center.x) / width, (y - rg->center.y) / height, rg->mcam2world);
		gizmo->intersect = TRUE;
	}
	else
	{
		/* Ring IDs follow the original XY, XZ, YZ plane ordering. */
		vector3f axis;
		switch(tripod->selected)
		{
			case SELECTED_AX_X: axis = vector3f(0.f, 0.f, 1.f); break;
			case SELECTED_AX_Y: axis = vector3f(0.f, 1.f, 0.f); break;
			case SELECTED_AX_Z: axis = vector3f(1.f, 0.f, 0.f); break;
			case SELECTED_PLANE_CAMERA:
				axis = cast3<float, double>(*EYE_POINT(camera)) - tripod->offset;
				axis.norm();
				break;
			default: gizmo->point = {x, y}; return;
		}
		plane<float> plane;
		plane.normal = axis;
		plane.d = axis.dot(tripod->offset);
		point3f previous, current;
		float angle = 0.f;
		if(rotation_plane_point(gizmo, gizmo->point.x, gizmo->point.y, plane, previous) &&
		   rotation_plane_point(gizmo, x, y, plane, current))
		{
			vector3f a = previous - tripod->offset;
			vector3f b = current - tripod->offset;
			if(a.mag() > .00001f && b.mag() > .00001f)
			{
				a.norm();
				b.norm();
				angle = atan2f(axis.dot(a.cross(b)), a.dot(b));
				gizmo->intersect = TRUE;
			}
		}
		else
		{
			/* An edge-on ring follows its visible front tangent. */
			vector3f toward_camera = cast3<float, double>(*EYE_POINT(camera)) - tripod->offset;
			vector3f radial = toward_camera - axis * axis.dot(toward_camera);
			if(radial.mag() > .00001f)
			{
				radial.norm();
				point3f tangent = tripod->offset + axis.cross(radial);
				GdkPoint screen;
				gluProjectf(tangent, tripod->matrixproject.m, gizmo->viewport, screen);
				const float dx = screen.x - tripod->center.x;
				const float dy = screen.y - tripod->center.y;
				const float length = sqrtf(dx * dx + dy * dy);
				if(length > .00001f)
				{
					const float radius = std::max(gtk_widget_get_allocated_height(GTK_WIDGET(tripod->canvas)) * .125f, 1.f);
					angle = ((x - gizmo->point.x) * dx + (y - gizmo->point.y) * dy) / (length * radius);
					gizmo->intersect = TRUE;
				}
			}
		}
		if(gizmo->intersect)
			rg->quat.rotate_axis(axis, angle);
	}
	rg->pc = {x, y};
	gizmo->point = {x, y};
}
