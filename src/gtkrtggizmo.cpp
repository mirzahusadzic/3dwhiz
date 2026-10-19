#include "gtkrtggizmo.h"
#include "gtkrtgrotategizmo.h"
#include "gtkrtgglu.h"
#include "gtkrtgutils.h"
#include <gtkrtg/gtkrtgcamera.h>

G_DEFINE_TYPE(GtkRtgGizmo, gtk_rtg_gizmo, GTK_RTG_TYPE_TRIPOD)

												/* normal */ /* dark */ /* darkest */
static const gchar *AXIS_COLOR[AXES][COLORS] = { {"#c80000",  "#b30000", "#9e0000"},
												 {"#00b500",  "#00a800", "#009c00"},
												 {"#0000c3",  "#00009c", "#000075"}  };
#define GIZMO_ARROW_TRIS	4
#define GIZMO_ARROW_QUADS	1

using namespace gtkrtg;
extern guint	 tripod_sel_color[AXES];

guint gizmo_sel_color[GIZMO_PLANES+1] = { COLOR_ID_GIZMO_PLANE_XY,
										  COLOR_ID_GIZMO_PLANE_XZ,
										  COLOR_ID_GIZMO_PLANE_YZ,
										  COLOR_ID_GIZMO_AXES	   };

static point3f vec_arrow[AXES][GIZMO_ARROW_VERTS] =
{
	{ point3f( .7525f,  -.038f,  .038f),  point3f(.7525f, -.038f,  -.038f),  point3f( .7525f, .038f,   .038f),  point3f(.7525f, .038f,  -.038f),  point3f(1.025f, 0.f,    0.f)},
	{ point3f(-.038f,    .7525f, .038f),  point3f(.038f,   .7525f,  .038f),  point3f(-.038f,  .7525f, -.038f),  point3f(.038f,  .7525f, -.038f),  point3f(0.f,    1.025f, 0.f)},
	{ point3f(-.038f,   -.038f,  .7525f), point3f(.038f,  -.038f,   .7525f), point3f(-.038f,  .038f,   .7525f), point3f(.038f,  .038f,   .7525f), point3f(0.f,    0.f,    1.025f)}
};

static vector3f norm_arrow[AXES][GIZMO_ARROW_VERTS] =
{
	{ vector3f(-1.f,  0.f,  0.f), vector3f(0.12f, -0.99f, 0.f),   vector3f(0.12f, 0.f,  -0.99f), vector3f(0.12f, 0.99f,  0.f),   vector3f( 0.12f, 0.f,   0.99f)},
	{ vector3f( 0.f, -1.f,  0.f), vector3f(0.f,    0.12f, 0.99f), vector3f(0.99f, 0.12f, 0.f),   vector3f(0.f,   0.12f, -0.99f), vector3f(-0.99f, 0.12f, 0.f)  },
	{ vector3f( 0.f,  0.f, -1.f), vector3f(0.f,   -0.99f, 0.12f), vector3f(0.99f, 0.f,   0.12f), vector3f(0.f,   0.99f,  0.12f), vector3f(-0.99f, 0.f,   0.12f)}
};


/* text position */
static point3f text_pos_persp[AXES] = { point3f(1.05f, 0.f,  0.f), point3f(0.f,  1.05f, 0.f), point3f(0.f,  0.f,  1.05f) };
static point3f text_pos_ortho[AXES] = { point3f(1.05f, 0.f,  0.f), point3f(0.f,  1.05f, 0.f), point3f(0.f,  0.f,  1.05f) };


static guint it_arrow[GIZMO_ARROW_TRIS*3]  = { 0, 1, 4, 1, 3, 4, 3, 2, 4, 2, 0, 4 };
static guint iq_arrow[GIZMO_ARROW_QUADS*4] = { 0, 2, 3, 1 };

static void	gtk_rtg_gizmo_finalize(GObject *object);
static void	gtk_rtg_gizmo_project(GtkRtgTripod *tripod, point3f *offset);
static void	gtk_rtg_gizmo_render_axis(GtkRtgTripod *tripod, guint ax_id);
static void	gtk_rtg_gizmo_render_axis_offscreen(GtkRtgTripod *tripod, guint ax_id);
static void	gtk_rtg_gizmo_process_drag(GtkRtgTripod *tripod, gint x, gint y);

static void
gtk_rtg_gizmo_class_init(GtkRtgGizmoClass *klass)
{
	GObjectClass	  *object_class = G_OBJECT_CLASS(klass);
	GtkRtgTripodClass *tripod_class = GTK_RTG_TRIPOD_CLASS(klass);

	object_class->finalize		    	= gtk_rtg_gizmo_finalize;
	tripod_class->project		    	= gtk_rtg_gizmo_project;
	tripod_class->render_axis	    	= gtk_rtg_gizmo_render_axis;
	tripod_class->render_axis_offscreen = gtk_rtg_gizmo_render_axis_offscreen;
	tripod_class->process_drag	    	= gtk_rtg_gizmo_process_drag;
}

GtkRtgGizmo *
gtk_rtg_gizmo_new(GtkRtgCanvas *canvas, gchar *font_desc)
{
	return GTK_RTG_GIZMO(g_object_new(GTK_RTG_TYPE_GIZMO, "font-desc", font_desc,
														  "canvas",    canvas, NULL));
}

static void
gtk_rtg_gizmo_init(GtkRtgGizmo *gizmo)
{
	GtkRtgTripod *tripod = GTK_RTG_TRIPOD(gizmo);

	for(guint i=0; i<AXES; ++i)
	{
		/* arrow colors */
		gdk_color_parse(AXIS_COLOR[i][COLOR_NORMAL],  &gizmo->ax[i].c_arrow[COLOR_NORMAL]);
		gdk_color_parse(AXIS_COLOR[i][COLOR_DARKER],  &gizmo->ax[i].c_arrow[COLOR_DARKER]);
		gdk_color_parse(AXIS_COLOR[i][COLOR_DARKEST], &gizmo->ax[i].c_arrow[COLOR_DARKEST]);

		/* poly render map */
		gizmo->ax[i].m_apoly = new m_arrowPolyType;
	}

	/* get plane color */
	gizmo->color_plane[0] = gfloat(tripod->colors[AXES].red)   / 65535.f;
	gizmo->color_plane[1] = gfloat(tripod->colors[AXES].green) / 65535.f;
	gizmo->color_plane[2] = gfloat(tripod->colors[AXES].blue)  / 65535.f;
	gizmo->color_plane[3] = 0.25f;

	gizmo->arrow = 1;

	g_signal_connect_after(gizmo, "render-axis", G_CALLBACK(gtk_rtg_gizmo_render_axis), NULL);

	memcpy(gizmo->text_pos_persp, text_pos_persp, sizeof(text_pos_persp));
	memcpy(gizmo->text_pos_ortho, text_pos_ortho, sizeof(text_pos_ortho));
}

static void
gtk_rtg_gizmo_finalize(GObject *object)
{
	GtkRtgGizmo *gizmo = GTK_RTG_GIZMO(object);

	for(guint i=0; i<AXES; ++i)
		if(gizmo->ax[i].m_apoly)
		{
			delete gizmo->ax[i].m_apoly;
			gizmo->ax[i].m_apoly = NULL;
		}

	G_OBJECT_CLASS(gtk_rtg_gizmo_parent_class)->finalize(object);
}


/*
 * Publics (lib only)
 *
 */
static void
gtk_rtg_gizmo_project(GtkRtgTripod *tripod, point3f *offset)
{
	GtkRtgGizmo  *gizmo  = GTK_RTG_GIZMO(tripod);
	GtkRtgCamera *camera = gtk_rtg_canvas_camera_get_default(tripod->canvas);
	matrix4x4f   &m_view = M_VIEW(camera);
	matrix4x4f   &m_proj = M_PROJ(camera);

	glGetIntegerv(GL_VIEWPORT, gizmo->viewport);

	/* get center */
	gluProjectf(X(*offset),
		    Y(*offset),
		    Z(*offset),
		    m_view.m,
		    m_proj.m,
		    gizmo->viewport,
		   	&tripod->center.x,
		   	&tripod->center.y);

	/* clear render map */
	if(tripod->m_ax->size()) tripod->m_ax->clear();

	/* get direction vector */
	gizmo->cam_offset = cast3<float, double>(*EYE_POINT(camera)) - *offset;
	gizmo->cam_offset.norm();
    gizmo->cam_offset_angle = fabsf(gizmo->cam_offset.dot(vector3f(0.f, 0.f, 1.f)));

    /* get projection matrix */
    __gluMultMatricesf(m_view.m, m_proj.m, tripod->matrixproject.m);

    if(PROJECTION(camera) == GTK_RTG_CAM_PROJ_PERSP)
	{
		gfloat   fov_2	   = tanf(deg2rad(gtk_rtg_camera_projection_get_fov(camera) * .5f));
		vector3f distance  = cast3<float, double>(*EYE_POINT(camera)) - *offset;
		gizmo->persp_scale = distance.mag() * 0.25f * fov_2;

		for(guint i=0; i<AXES; ++i)
		{
			/* scale */
			gfloat scale_ax = tripod->ax[i].scale * gizmo->persp_scale;

			/* project axis */
            point3f ax_start    =  gizmo->point_ax3d_persp[i][AX_START] * scale_ax;
			point3f ax_end      =  gizmo->point_ax3d_persp[i][AX_END]   * scale_ax;
			point3f ax_end_orig =  tripod->matrixproject   * ax_end;
			point3f tpos	    =  gizmo->text_pos_persp[i] * scale_ax + *offset;

			point3f ax_org	    = (tripod->matrixproject * ax_start) - ax_end_orig;

			ax_start += *offset; ax_end += *offset;

            gluProjectf(ax_start, tripod->matrixproject.m, gizmo->viewport, tripod->ax[i].ax2d[AX_START]);
			gluProjectf(ax_end,   tripod->matrixproject.m, gizmo->viewport, tripod->ax[i].ax2d[AX_END]);
			gluProjectf(tpos,     tripod->matrixproject.m, gizmo->viewport, gizmo->ax[i].text_pos);

            /* update off-screen axis */
            tripod->ax[i].ax2d_offscreen[AX_START] = tripod->ax[i].ax2d[AX_START];
            tripod->ax[i].ax2d_offscreen[AX_END]   = tripod->ax[i].ax2d[AX_END];

            /* add into render map (ordered by the angle between oriented axis and up z vector) */
			gfloat dot = ax_org.norm().dot(vector3f(0.f, 0.f, 1.f));

			/* with offset */
			vector3f ax         = ax_start - ax_end;
			tripod->ax[i].angle = ax.norm().dot(vector3f(0.f, 0.f, -1.f));

            /* mirror */
            tripod->ax[i].mirror = tripod->ax[i].angle > 0.f;

			/* sort render list by angle without offset */
			tripod->m_ax->insert(std::make_pair(dot, &tripod->ax[i]));

			/* clear arrow polys map */
			if(gizmo->ax[i].m_apoly->size()) gizmo->ax[i].m_apoly->clear();

			for(guint j=0; j<GIZMO_ARROW_VERTS; ++j)
			{
				point3f point_orient = vec_arrow[i][j] * scale_ax + *offset;
				gluProjectf(point_orient, tripod->matrixproject.m, gizmo->viewport, gizmo->ax[i].arrow2d[j]);

				/*
				 * Visibility culling (back-face removal)
				 * Get an angle between camera direction and quad/face normal vectors.
				 * If angle is positive then quad/face is oriented front-face.
				 */

				dot = norm_arrow[i][j].dot(gizmo->cam_offset);

				/* add to arrow polys render map */
				if(dot > 0.f)
					gizmo->ax[i].m_apoly->insert(std::make_pair(dot, j));
			}
		}
	}else
		/* orthographic */
	{
		/* get projection matrix */
		gizmo->matrixproject	 = m_view;
		gizmo->matrixproject[tx] = gizmo->matrixproject[ty]=gizmo->matrixproject[tz]=0.f;
		gizmo->matrixproject[tw] = 1.f;

		/*
		 * Get gizmo's dimensions (approx. to match size of perspective gizmo).
		 *
		 */

		gizmo->size = MIN(gizmo->viewport[2], gizmo->viewport[3]) >> 2; /* get 25% of shorter viewport axis */

		if(gizmo->size % 2 == 0) gizmo->size +=1;	/* better approximation at 'x' axis */

		gizmo->alphabeta = .5f * gfloat(gizmo->size) -.5f;
		gizmo->xt        = tripod->center.x - gint(gizmo->alphabeta);
		gizmo->yt        = tripod->center.y - gint(gizmo->alphabeta);

		for(guint i=0; i<AXES; ++i)
		{
            point3f ax_start = gizmo->matrixproject * (gizmo->point_ax3d_ortho[i][AX_START] * tripod->ax[i].scale);
            utils_unproject_point2screen(ax_start, gizmo->alphabeta, gizmo->alphabeta, tripod->ax[i].ax2d[AX_START]);
            tripod->ax[i].ax2d[AX_START].y = gizmo->size - tripod->ax[i].ax2d[AX_START].y;

			/* translate to offset */
			tripod->ax[i].ax2d[AX_START].x += gizmo->xt;
			tripod->ax[i].ax2d[AX_START].y += gizmo->yt;

            point3f ax_end = gizmo->matrixproject * (gizmo->point_ax3d_ortho[i][AX_END] * tripod->ax[i].scale);
			utils_unproject_point2screen(ax_end, gizmo->alphabeta, gizmo->alphabeta, tripod->ax[i].ax2d[AX_END]);
			tripod->ax[i].ax2d[AX_END].y = gizmo->size - tripod->ax[i].ax2d[AX_END].y;

			/* translate to offset */
			tripod->ax[i].ax2d[AX_END].x += gizmo->xt;
			tripod->ax[i].ax2d[AX_END].y += gizmo->yt;

            point3f tpos = gizmo->matrixproject * (gizmo->text_pos_ortho[i] * tripod->ax[i].scale);
			utils_unproject_point2screen(tpos, gizmo->alphabeta, gizmo->alphabeta, gizmo->ax[i].text_pos);
			gizmo->ax[i].text_pos.y = gizmo->size - gizmo->ax[i].text_pos.y;

			/* translate to offset */
			gizmo->ax[i].text_pos.x += gizmo->xt;
			gizmo->ax[i].text_pos.y += gizmo->yt;

			/* update off-screen axis */
            tripod->ax[i].ax2d_offscreen[AX_START] = tripod->ax[i].ax2d[AX_START];
            tripod->ax[i].ax2d_offscreen[AX_END]   = tripod->ax[i].ax2d[AX_END];

            /* add into render map (ordered by the angle between oriented axis and up z vector) */
			tripod->ax[i].angle = ax_end.norm().dot(vector3f(0.f, 0.f, 1.f));
			tripod->m_ax->insert(std::make_pair(tripod->ax[i].angle, &tripod->ax[i]));

			/* clear arrow polys map */
			if(gizmo->ax[i].m_apoly->size()) gizmo->ax[i].m_apoly->clear();

			for(guint j=0; j<GIZMO_ARROW_VERTS; ++j)
			{
				point3f point_orient = gizmo->matrixproject * (vec_arrow[i][j] * tripod->ax[i].scale);
				utils_unproject_point2screen(point_orient, gizmo->alphabeta, gizmo->alphabeta, gizmo->ax[i].arrow2d[j]);
				gizmo->ax[i].arrow2d[j].y = gizmo->size - gizmo->ax[i].arrow2d[j].y;

				/* translate to offset */
				gizmo->ax[i].arrow2d[j].x += gizmo->xt;
				gizmo->ax[i].arrow2d[j].y += gizmo->yt;

				/*
				 * Visibility culling (back-face removal)
				 * Get an angle between camera direction and quad/face normal vectors.
				 * If angle is positive then quad/face is oriented front-face.
				 */

				gfloat dot = norm_arrow[i][j].dot(gizmo->cam_offset);

				/* add to arrow polys render map */
				if(dot > 0.f)
					gizmo->ax[i].m_apoly->insert(std::make_pair(dot, j));
			}
		}
	}

	/*
	 * Update text position
	 */
	for(guint i=0; i<AXES; ++i)
	{
		GdkPoint pos = gizmo->ax[i].text_pos;
		g_object_set(tripod->ax[i].text, "position", &pos, NULL);
	}

	/*
	 * get inverse.
	 */
	__gluInvertMatrixf(tripod->matrixproject.m, gizmo->minverse.m);
}


static void
gtk_rtg_gizmo_render_axis(GtkRtgTripod *tripod, guint ax_id)
{
	GtkRtgGizmo *gizmo = GTK_RTG_GIZMO(tripod);

	if(gizmo->arrow)
	{
		GizmoAx	*ax	  = &gizmo->ax[ax_id];
		guint	 color_id =  0;

		/* arrows */
		for(m_arrowPolyType::reverse_iterator iter = ax->m_apoly->rbegin(); iter!=ax->m_apoly->rend(); ++iter)
		{
			glColor3usv(&ax->c_arrow[color_id < COLORS ? color_id++ : COLOR_DARKEST].red);

			if(iter->second != 0)
			{
				guint ind = (iter->second-1)*3;
				glBegin(GL_TRIANGLES);
				glVertex2i(ax->arrow2d[it_arrow[ind  ]].x, ax->arrow2d[it_arrow[ind  ]].y);
				glVertex2i(ax->arrow2d[it_arrow[ind+1]].x, ax->arrow2d[it_arrow[ind+1]].y);
				glVertex2i(ax->arrow2d[it_arrow[ind+2]].x, ax->arrow2d[it_arrow[ind+2]].y);
				glEnd();
			}else
			{
				glBegin(GL_QUADS);
				glVertex2i(ax->arrow2d[iq_arrow[0]].x, ax->arrow2d[iq_arrow[0]].y);
				glVertex2i(ax->arrow2d[iq_arrow[1]].x, ax->arrow2d[iq_arrow[1]].y);
				glVertex2i(ax->arrow2d[iq_arrow[2]].x, ax->arrow2d[iq_arrow[2]].y);
				glVertex2i(ax->arrow2d[iq_arrow[3]].x, ax->arrow2d[iq_arrow[3]].y);
				glEnd();
			}
		}
	}
}


static void
gtk_rtg_gizmo_render_axis_offscreen(GtkRtgTripod *tripod, guint ax_id)
{
	GtkRtgGizmo *gizmo = GTK_RTG_GIZMO(tripod);

	if(gizmo->arrow)
	{
		GizmoAx *ax = &GTK_RTG_GIZMO(tripod)->ax[ax_id];

		/* arrows */
		for(m_arrowPolyType::reverse_iterator iter = ax->m_apoly->rbegin(); iter!=ax->m_apoly->rend(); ++iter)
		{
			/* set color */
			glColor3ubv((guchar*)&tripod_sel_color[ax_id]);

			if(iter->second != 0)
			{
				guint ind = (iter->second-1)*3;
				glBegin(GL_TRIANGLES);
				glVertex2i(ax->arrow2d[it_arrow[ind  ]].x, ax->arrow2d[it_arrow[ind  ]].y);
				glVertex2i(ax->arrow2d[it_arrow[ind+1]].x, ax->arrow2d[it_arrow[ind+1]].y);
				glVertex2i(ax->arrow2d[it_arrow[ind+2]].x, ax->arrow2d[it_arrow[ind+2]].y);
				glEnd();
			}else
			{
				glBegin(GL_QUADS);
				glVertex2i(ax->arrow2d[iq_arrow[0]].x, ax->arrow2d[iq_arrow[0]].y);
				glVertex2i(ax->arrow2d[iq_arrow[1]].x, ax->arrow2d[iq_arrow[1]].y);
				glVertex2i(ax->arrow2d[iq_arrow[2]].x, ax->arrow2d[iq_arrow[2]].y);
				glVertex2i(ax->arrow2d[iq_arrow[3]].x, ax->arrow2d[iq_arrow[3]].y);
				glEnd();
			}
		}
	}
}


static void
gtk_rtg_gizmo_process_drag(GtkRtgTripod *tripod, gint x, gint y)
{
	GtkRtgGizmo	*gizmo   = GTK_RTG_GIZMO(tripod);
	GtkRtgCamera	 *camera = gtk_rtg_canvas_camera_get_default(tripod->canvas);

	/*
	 * Get line segment.
	 *
	 */
    enum{SEGMENT_START, SEGMENT_END, SEGMENT_POINTS};
    point3f p[SEGMENT_POINTS];
    gluUnProjectf(gfloat(x), gfloat(y), 0.f, gizmo->minverse.m, gizmo->viewport, &X(p[SEGMENT_START]), &Y(p[SEGMENT_START]), &Z(p[SEGMENT_START]));
    gluUnProjectf(gfloat(x), gfloat(y), 1.f, gizmo->minverse.m, gizmo->viewport, &X(p[SEGMENT_END]),   &Y(p[SEGMENT_END]),   &Z(p[SEGMENT_END]));

	/*
	 * Get plane (normal) on which screen coordinates will be projected.
	 */
    plane<float> pl;

    if(GTK_RTG_IS_ROTATE_GIZMO(tripod))     /* special handling for rotate gizmo */
    {
        if(PROJECTION(camera) == GTK_RTG_CAM_PROJ_PERSP)
        {
            if(gizmo->cam_offset_angle > 0.99f)
            {
                if(fabsf(tripod->ax[AX_X].angle) > 0.9f)
                    pl.normal = vector3f(1.f, 0.f, 0.f);
                else
                    if(fabsf(tripod->ax[AX_Y].angle) > 0.9f)
                        pl.normal = vector3f(0.f, 1.f, 0.f);
                    else
                        if(fabsf(tripod->ax[AX_Z].angle) > 0.9f)
                            pl.normal = vector3f(0.f, 0.f, 1.f);
                        else
                            goto persp;
            }else
                goto persp;

        }else
            /* orthographic */
        {
            switch(gtk_rtg_canvas_view_get_predefined(tripod->canvas))
            {
                case GTK_RTG_CANVAS_VIEW_TOP:
                case GTK_RTG_CANVAS_VIEW_BOTTOM:  pl.normal = vector3f(0.f, 0.f, 1.f);  break;

                case GTK_RTG_CANVAS_VIEW_LEFT:
                case GTK_RTG_CANVAS_VIEW_RIGHT:	  pl.normal = vector3f(1.f, 0.f, 0.f);  break;

                case GTK_RTG_CANVAS_VIEW_FRONT:
                case GTK_RTG_CANVAS_VIEW_BACK:	  pl.normal = vector3f(0.f, 1.f, 0.f);  break;

                default:                          pl.normal = vector3f(0.f, 0.f, 1.f);  break;
            };
        }
    }else
    {
		persp:

        switch(tripod->selected)
        {
		    /* single-axis: check which plane is better to project on */
		    case  SELECTED_AX_X:	if(fabsf(tripod->ax[AX_Y].angle) >= fabsf(tripod->ax[AX_Z].angle))
		    							pl.normal = vector3f(0.f, 1.f, 0.f);
					        		else
					        			pl.normal = vector3f(0.f, 0.f, 1.f);
		    						break;

		    case  SELECTED_AX_Y:	if(fabsf(tripod->ax[AX_X].angle) >= fabsf(tripod->ax[AX_Z].angle))
		    							pl.normal = vector3f(1.f, 0.f, 0.f);
					        		else
					        			pl.normal = vector3f(0.f, 0.f, 1.f);
		    						break;

		    case  SELECTED_AX_Z:	if(fabsf(tripod->ax[AX_X].angle) >= fabsf(tripod->ax[AX_Y].angle))
		    							pl.normal = vector3f(1.f, 0.f, 0.f);
					        		else
					        			pl.normal = vector3f(0.f, 1.f, 0.f);
		    						break;

		        					/* multi axes */
		    case SELECTED_PLANE_XY: pl.normal = vector3f(0.f, 0.f, 1.f); break;
            case SELECTED_PLANE_XZ: pl.normal = vector3f(0.f, 1.f, 0.f); break;
            case SELECTED_PLANE_YZ: pl.normal = vector3f(1.f, 0.f, 0.f); break;
	    };
    }

	gizmo->intersect = 0;

	if(    tripod->selected == SELECTED_XYZ
	   	|| tripod->selected == SELECTED_CIRCLE
	   	|| tripod->selected == SELECTED_PLANE_CAMERA)
	{
		if(tripod->drag)
			gizmo->intersect = 1;
		else
		{
			tripod->drag   = 1;
			gizmo->point.x = x;
			gizmo->point.y = y;
		}
	}else
	{
		/* d */
		pl.d = tripod->offset.dot(pl.normal);

		/* clear delta */
		X(tripod->delta_offset) = Y(tripod->delta_offset) = Z(tripod->delta_offset) = 0.f;

		/*
		 * Test for the intersection (get point of intersection).
		 * - calculate 3D delta translate (offset).
		 *
		 */

		point3f new_offset;
		if(intersect_segment_plane<float>(p[SEGMENT_START], p[SEGMENT_END], pl, new_offset))
		{
			if(tripod->drag)
			{
				tripod->delta_offset = new_offset - tripod->current_offset;

				/* single axis check */
				switch(tripod->selected)
				{
					case SELECTED_AX_X: Y(tripod->delta_offset)=Z(tripod->delta_offset)=0.f; break;
					case SELECTED_AX_Y: X(tripod->delta_offset)=Z(tripod->delta_offset)=0.f; break;
					case SELECTED_AX_Z: X(tripod->delta_offset)=Y(tripod->delta_offset)=0.f; break;
				};

				gizmo->intersect = 1;

			}else
			{
				tripod->drag   = 1;
				gizmo->point.x = x;
				gizmo->point.y = y;
			}

			tripod->current_offset = new_offset;
		}
	}
}
