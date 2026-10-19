#include "gtkrtgtripod.h"
#include "gtkrtgextras.h"
#include "gtkrtgutils.h"
#include "gtkrtgglu.h"
#include <gtkrtg/gtkrtgcamera.h>
#include "gtkrtgcolorid.h"
#include "marshalers.h"

G_DEFINE_TYPE(GtkRtgTripod, gtk_rtg_tripod, G_TYPE_OBJECT)

										  /*  x	        y	    z	    selected */
static const gchar *AXIS_COLOR[AXES+1] = {"#c80000", "#00b500", "#0000c3", "#f2d900"};

/* axes graph (default) */
#define AXIS_GRAPH_SIZE 51
#define AXIS_ALPHA (.5f * AXIS_GRAPH_SIZE - .5f)
#define AXIS_BETA AXIS_ALPHA

/* picking tolerance */
#define GIZMO_OFFSCREEN_LINE_WIDTH 10

using namespace gtkrtg;

static void	gtk_rtg_tripod_finalize(GObject *object);
static void gtk_rtg_tripod_constructed(GObject *object);
static void gtk_rtg_tripod_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
extern void _gtk_rtg_canvas_text_push_real(GtkRtgCanvas *canvas, GtkRtgText *text, m_txtType *m_text);
void _gtk_rtg_tripod_project_real(GtkRtgTripod *tripod, point3f *offset);
static void _gtk_rtg_tripod_update_selection_real(GtkRtgTripod *tripod);

vector3f point_ax3d[AXES] = { vector3f(1.f, 0.f, 0.f),
							  vector3f(0.f, 1.f, 0.f),
							  vector3f(0.f, 0.f, 1.f) };

guint tripod_sel_color[AXES] = { COLOR_ID_GIZMO_AX_X,
								 COLOR_ID_GIZMO_AX_Y,
								 COLOR_ID_GIZMO_AX_Z };

/* properties */
enum
{
	PROP_0,
	PROP_CANVAS,
	PROP_FONT_DESC,
	LAST_ARG
};

/* signals */
enum
{
	PROJECT,
	RENDER,
	RENDER_AXIS,
	RENDER_LAST,
	RENDER_OFFSCREEN,
	RENDER_AXIS_OFFSCREEN,
	PROCESS_MOTION,
	PROCESS_DRAG,
	UPDATE_SELECTION,
	LAST_SIGNAL
};

static  gint signals[LAST_SIGNAL] = { 0 };

static void
gtk_rtg_tripod_class_init(GtkRtgTripodClass *klass)
{
	GObjectClass  *object_class = (GObjectClass *) klass;

	object_class->finalize = gtk_rtg_tripod_finalize;
	object_class->constructed = gtk_rtg_tripod_constructed;
	object_class->set_property = gtk_rtg_tripod_set_property;
    klass->project = _gtk_rtg_tripod_project_real;
	klass->update_selection = _gtk_rtg_tripod_update_selection_real;

	g_object_class_install_property(object_class,
            					    PROP_CANVAS,
									g_param_spec_pointer("canvas",
														 "canvas",
														 "The canvas to render tripod.",
														 (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));
	g_object_class_install_property(object_class,
									PROP_FONT_DESC,
									g_param_spec_string("font-desc",
														"font-desc",
														"The font description to label axes on tripod.",
														NULL,
														(GParamFlags)(G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));
	/*
     * Signal(s)
     *
     *
     */
    signals[PROJECT] = g_signal_new("project",
                                    G_OBJECT_CLASS_TYPE(klass),
                                    (GSignalFlags)G_SIGNAL_RUN_FIRST,
                                    G_STRUCT_OFFSET(GtkRtgTripodClass, project),
                                    NULL,
                                    NULL,
                                    g_cclosure_marshal_VOID__POINTER,
                                    G_TYPE_NONE,
                                    1,
                                    G_TYPE_POINTER);

    signals[RENDER] = g_signal_new("render",
                                   G_OBJECT_CLASS_TYPE(klass),
                                   (GSignalFlags)G_SIGNAL_RUN_FIRST,
                                   G_STRUCT_OFFSET(GtkRtgTripodClass, render),
                                   NULL,
                                   NULL,
                                   g_cclosure_marshal_VOID__VOID,
                                   G_TYPE_NONE,
                                   0);

    signals[RENDER_AXIS] = g_signal_new("render-axis",
                                        G_OBJECT_CLASS_TYPE(klass),
                                        (GSignalFlags)G_SIGNAL_RUN_FIRST,
                                        G_STRUCT_OFFSET(GtkRtgTripodClass, render_axis),
                                        NULL,
                                        NULL,
                                        g_cclosure_marshal_VOID__UINT,
                                        G_TYPE_NONE,
                                        1,
                                        G_TYPE_UINT);

    signals[RENDER_LAST] = g_signal_new("render-last",
                                        G_OBJECT_CLASS_TYPE(klass),
                                        (GSignalFlags)G_SIGNAL_RUN_FIRST,
                                        G_STRUCT_OFFSET(GtkRtgTripodClass, render_last),
                                        NULL,
                                        NULL,
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE,
                                        0);

    signals[RENDER_OFFSCREEN] = g_signal_new("render-offscreen",
                                             G_OBJECT_CLASS_TYPE(klass),
                                             (GSignalFlags)G_SIGNAL_RUN_FIRST,
                                             G_STRUCT_OFFSET(GtkRtgTripodClass, render_offscreen),
                                             NULL,
                                             NULL,
                                             g_cclosure_marshal_VOID__VOID,
                                             G_TYPE_NONE,
                                             0);

    signals[RENDER_AXIS_OFFSCREEN] = g_signal_new("render-axis-offscreen",
                                                  G_OBJECT_CLASS_TYPE(klass),
                                                  (GSignalFlags)G_SIGNAL_RUN_FIRST,
                                                  G_STRUCT_OFFSET(GtkRtgTripodClass, render_axis_offscreen),
                                                  NULL,
                                                  NULL,
                                                  g_cclosure_marshal_VOID__UINT,
                                                  G_TYPE_NONE,
                                                  1,
                                                  G_TYPE_UINT);

    signals[PROCESS_MOTION] = g_signal_new("process-motion",
                                           G_OBJECT_CLASS_TYPE(klass),
                                           (GSignalFlags)G_SIGNAL_RUN_FIRST,
                                           G_STRUCT_OFFSET(GtkRtgTripodClass, process_motion),
                                           NULL,
                                           NULL,
                                           gtkrtg_marshal__VOID__INT_INT,
                                           G_TYPE_NONE,
                                           2,
                                           G_TYPE_INT,
                                           G_TYPE_INT);

    signals[PROCESS_DRAG] = g_signal_new("process-drag",
                                         G_OBJECT_CLASS_TYPE(klass),
                                         (GSignalFlags)G_SIGNAL_RUN_FIRST,
                                         G_STRUCT_OFFSET(GtkRtgTripodClass, process_drag),
                                         NULL,
                                         NULL,
                                         gtkrtg_marshal__VOID__INT_INT,
                                         G_TYPE_NONE,
                                         2,
                                         G_TYPE_INT,
                                         G_TYPE_INT);

    signals[UPDATE_SELECTION] = g_signal_new("update-selection",
                                             G_OBJECT_CLASS_TYPE(klass),
                                             (GSignalFlags)G_SIGNAL_RUN_FIRST,
                                             G_STRUCT_OFFSET(GtkRtgTripodClass, update_selection),
                                             NULL,
                                             NULL,
                                             g_cclosure_marshal_VOID__VOID,
                                             G_TYPE_NONE,
                                             0);
}

static void
_gtk_rtg_tripod_register_text(GtkRtgTripod *tripod)
{
	/*
	 * Text
	 */

	/* push text */
	for(guint i=0; i<AXES; ++i)
	{
        if(tripod->ax[i].text)
            g_object_unref(tripod->ax[i].text);

		tripod->ax[i].text = gtk_rtg_text_new();

		g_object_set(tripod->ax[i].text, "font-desc",  tripod->font_desc,
										 "color",     &tripod->ax[i].c_ax, NULL);

		/* associate text base with the text */
		_gtk_rtg_canvas_text_push_real(tripod->canvas, tripod->ax[i].text, NULL);
	}

	/* string  */
	g_object_set(tripod->ax[AX_X].text, "string", "x", NULL);
	g_object_set(tripod->ax[AX_Y].text, "string", "y", NULL);
	g_object_set(tripod->ax[AX_Z].text, "string", "z", NULL);
}


GtkRtgTripod *
gtk_rtg_tripod_new(GtkRtgCanvas *canvas, gchar *font_desc)
{
	return GTK_RTG_TRIPOD(g_object_new(GTK_RTG_TYPE_TRIPOD, "font-desc", font_desc,
															"canvas", canvas, NULL));
}

static void
gtk_rtg_tripod_init(GtkRtgTripod *tripod)
{
	/*
	 * Init axes
	 *
	 */
	guint i=0;
	for(; i<AXES; ++i)
	{
		/* colors */
		gdk_color_parse(AXIS_COLOR[i],  &tripod->colors[i]);
		tripod->ax[i].c_ax = &tripod->colors[i];

		tripod->ax[i].id = i;
		tripod->ax[i].scale = 1.0f;

        tripod->ax[i].ax2d[AX_START].x = gint(AXIS_ALPHA) + 2;
        tripod->ax[i].ax2d[AX_START].y = gint(AXIS_BETA) + 2;
    }

	/* selection flags */
	tripod->ax[AX_X].sel_flag = SELECTED_AX_X;
	tripod->ax[AX_Y].sel_flag = SELECTED_AX_Y;
	tripod->ax[AX_Z].sel_flag = SELECTED_AX_Z;

	/* color for the selected state */
	gdk_color_parse(AXIS_COLOR[i],  &tripod->colors[i]);

	/* render text */
	tripod->text = 1;

	/* create render map */
	tripod->m_ax = new m_axesType;

    /* update fallback */
    tripod->fallback = 1;

	/* render offscreen axes */
	tripod->offscreen = 1;

	/* skip rendering ax parallel to the camera direction vector */
	tripod->skip_parallel_ax = 1;
}


static void
gtk_rtg_tripod_finalize(GObject *object)
{
	GtkRtgTripod *tripod = GTK_RTG_TRIPOD(object);

    for(guint i = 0; i < AXES; ++i)
        g_clear_object(&tripod->ax[i].text);

	if(tripod->m_ax)
	{
		delete tripod->m_ax;
		tripod->m_ax = NULL;
	}

	if(tripod->font_desc)
	{
		g_free(tripod->font_desc);
		tripod->font_desc = NULL;
	}

	G_OBJECT_CLASS(gtk_rtg_tripod_parent_class)->finalize(object);
}

static void
gtk_rtg_tripod_constructed(GObject *object)
{
    G_OBJECT_CLASS(gtk_rtg_tripod_parent_class)->constructed(object);
    GtkRtgTripod *tripod = GTK_RTG_TRIPOD(object);
    if(!tripod->font_desc)
        tripod->font_desc = g_strdup("Sans 10");
    if(tripod->canvas)
        _gtk_rtg_tripod_register_text(tripod);
}

static void
gtk_rtg_tripod_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GtkRtgTripod *tripod = GTK_RTG_TRIPOD(object);

	switch(prop_id)
	{
		case PROP_CANVAS:       tripod->canvas = (GtkRtgCanvas *)g_value_get_pointer(value);
								tripod->pb     = (GtkRtgPickingBuffer*) g_object_get_data(G_OBJECT(tripod->canvas), "pickbuff");
                                break;

		case PROP_FONT_DESC:	tripod->font_desc = g_strdup(g_value_get_string(value));
								break;

	};
}

void
_gtk_rtg_tripod_project_real(GtkRtgTripod *tripod, point3f *offset)
{
    GtkRtgCamera *camera = gtk_rtg_canvas_camera_get_default(tripod->canvas);

    tripod->matrixproject = M_VIEW(camera);

    /* no translation */
    tripod->matrixproject[tx]=tripod->matrixproject[ty]=tripod->matrixproject[tz]=0.f;
    tripod->matrixproject[tw]=1.f;

    if(tripod->m_ax->size()) tripod->m_ax->clear();		/* clear render map */

    /* get direction vector */
    vector3f cam_dir = cast3<float, double>(*EYE_POINT(camera) - *gtk_rtg_camera_view_get_center(camera));
    cam_dir.norm();

    for(guint i=0; i<AXES; ++i)
    {
        point3f vec_orient = tripod->matrixproject * (point_ax3d[i] * tripod->ax[i].scale);
        utils_unproject_point2screen(vec_orient, AXIS_ALPHA, AXIS_BETA, tripod->ax[i].ax2d[AX_END]);
        tripod->ax[i].ax2d[AX_END].y = AXIS_GRAPH_SIZE - tripod->ax[i].ax2d[AX_END].y;

		/* shift out of border */
		tripod->ax[i].ax2d[AX_END].x +=2; tripod->ax[i].ax2d[AX_END].y +=2;

        /* update text position */
        GdkPoint pos = tripod->ax[i].ax2d[AX_END]; pos.x +=1; pos.y +=3;
        g_object_set(tripod->ax[i].text, "position", &pos, NULL);

        /* add into render map (ordered by the angle between oriented axis and up z vector) */
        tripod->ax[i].angle = vec_orient.norm().dot(vector3f(0.f, 0.f, 1.f));
        tripod->m_ax->insert(std::make_pair(tripod->ax[i].angle, &tripod->ax[i]));
    }
}

/*
 * Publics (lib only)
 *
 */
void
gtk_rtg_tripod_project(GtkRtgTripod *tripod, point3f *offset)
{
    g_signal_emit(tripod, signals[PROJECT], 0, offset);
	tripod->selected = tripod->sel_fallback;
	g_signal_emit(tripod, signals[UPDATE_SELECTION], 0);

    /* save offset */
    if(offset)      tripod->offset = *offset;
}

void
gtk_rtg_tripod_render(GtkRtgTripod *tripod, GtkRtgCanvasViewPredefined predefined)
{
    GLfloat	 l_width;

    /* preserve current line width */
    glGetFloatv(GL_LINE_WIDTH, &l_width);

    glLineWidth(1.25f);

	/* emit render signal */
	g_signal_emit(tripod, signals[RENDER_LAST], 0);

    /* render axis lines */
    for(m_axesType::reverse_iterator it=tripod->m_ax->rbegin(); it!=tripod->m_ax->rend(); ++it)
    {
        TripodAx *ax = it->second;

        if(tripod->skip_parallel_ax) /* test for parallel axis */
		{
			if(fabsf(ax->angle) < 0.99999f) /* skip drawing tiny dot axes (for angles -1.0 and 1.0) */
			{
				if(tripod->text)
					utils_render_view_axes_text(ax->text);

				/* emit render axis event */
				g_signal_emit(tripod, signals[RENDER_AXIS], 0, ax->id);

				/* render axes */
				glColor3usv(&ax->c_ax->red);

				glBegin(GL_LINES);
				glVertex2i(ax->ax2d[AX_START].x, ax->ax2d[AX_START].y);
				glVertex2i(ax->ax2d[AX_END].x,   ax->ax2d[AX_END].y);
				glEnd();

			}else
			/* only text for parallel */
			if(tripod->text && predefined == GTK_RTG_CANVAS_VIEW_CUSTOM)
				utils_render_view_axes_text(ax->text);
		}else
		/* without test: render all axes */
		{
			if(tripod->text)
				utils_render_view_axes_text(ax->text);

			/* emit render axis event */
			g_signal_emit(tripod, signals[RENDER_AXIS], 0, ax->id);

			/* render axes */
			glColor3usv(&ax->c_ax->red);

			glBegin(GL_LINES);
			glVertex2i(ax->ax2d[AX_START].x, ax->ax2d[AX_START].y);
			glVertex2i(ax->ax2d[AX_END].x,   ax->ax2d[AX_END].y);
			glEnd();
		}
    }

	/* emit render signal */
	g_signal_emit(tripod, signals[RENDER], 0);

    glLineWidth(l_width);
    glColor3f(1.f, 1.f, 1.f);
}


void
gtk_rtg_tripod_render_offscreen(GtkRtgTripod *tripod, GtkRtgCanvasViewPredefined predefined)
{
	GLfloat	 l_width;

	GtkWidget *widget = GTK_WIDGET(tripod->canvas);

	/* preserve current line width */
	glGetFloatv(GL_LINE_WIDTH, &l_width);
	glLineWidth(GIZMO_OFFSCREEN_LINE_WIDTH);

	/*
	 * Orthographic mode for drawing gizmo's.
	 *
	 */

	GLint matrix_mode = 0;
	glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);

	if(matrix_mode != GL_PROJECTION)
		glMatrixMode(GL_PROJECTION);

	glPushMatrix();
	glLoadIdentity();

	glOrtho(0.0, gdouble(gtk_widget_get_allocated_width(widget)), 0.0, gdouble(gtk_widget_get_allocated_height(widget)), 0.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	/* render axis lines */
	for(m_axesType::reverse_iterator it=tripod->m_ax->rbegin(); it!=tripod->m_ax->rend(); ++it)
	{
		TripodAx *ax = it->second;

		if(tripod->skip_parallel_ax) /* should test for parallel? */
		{
			if(fabsf(ax->angle) < 0.99999f) /* skip drawing tiny dot axes (for angles -1.0 and 1.0) */
			{
				/* emit render axis off-screen event */
				g_signal_emit(tripod, signals[RENDER_AXIS_OFFSCREEN], 0, ax->id);

				if(tripod->offscreen)
				{
					/* set color */
					glColor3ubv((guchar*)&tripod_sel_color[ax->id]);

					glBegin(GL_LINES);
					glVertex2i(ax->ax2d_offscreen[AX_START].x, ax->ax2d_offscreen[AX_START].y);
					glVertex2i(ax->ax2d_offscreen[AX_END].x,   ax->ax2d_offscreen[AX_END].y);
					glEnd();
				}
			}
		}else /* render all axes */
		{
			/* emit render axis off-screen event */
			g_signal_emit(tripod, signals[RENDER_AXIS_OFFSCREEN], 0, ax->id);

			if(tripod->offscreen)
			{
				/* set color */
				glColor3ubv((guchar*)&tripod_sel_color[ax->id]);

				glBegin(GL_LINES);
				glVertex2i(ax->ax2d_offscreen[AX_START].x, ax->ax2d_offscreen[AX_START].y);
				glVertex2i(ax->ax2d_offscreen[AX_END].x,   ax->ax2d_offscreen[AX_END].y);
				glEnd();
			}
		}
	}

	/* emit render-offscreen signal */
	g_signal_emit(tripod, signals[RENDER_OFFSCREEN], 0);

	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	/* rewind matrix mode */
	if(matrix_mode != GL_PROJECTION)
		glMatrixMode(matrix_mode);

	glLineWidth(l_width);
	glColor3f(1.f, 1.f, 1.f);
}

	static void
_gtk_rtg_tripod_update_selection_real(GtkRtgTripod *tripod)
{
	for(guint i=0; i<AXES; ++i)
	{
		/* deselect */
		tripod->ax[i].c_ax = &tripod->colors[i];
		g_object_set(tripod->ax[i].text, "color", tripod->ax[i].c_ax, NULL);

		guint sel_id = i ? i<<2 : SELECTED_AX_X;

		/* select */
		if(tripod->selected & sel_id)
		{
			/* select */
			tripod->ax[i].c_ax = &tripod->colors[AXES];
			g_object_set(tripod->ax[i].text, "color", tripod->ax[i].c_ax, NULL);
		}
	}
}

	guint
gtk_rtg_tripod_process_motion(GtkRtgTripod *tripod, gint x, gint y)
{
	guint value = GIZMO_NO_PROCESSING;

	if(tripod->pb->buff == NULL)
		return value;

	/* check bounds */
	if(x > tripod->pb->width)
		x = tripod->pb->width-1;
	else
	if(x < 0)
		x = 0;

	if(y > tripod->pb->height)
		y = tripod->pb->height-1;
	else
	if(y < 0)
		y = 0;

	/* read pixel from the back buffer */
	guint pos  = y ? (y-1) * 4 * tripod->pb->width : 0;
	pos += x * 4;

	GLubyte *pixel = &tripod->pb->buff[pos];

	/* pack color id */
	tripod->color_id = pixel[2] << 16 | pixel[1] << 8 | pixel[0];

	guint old_selection = tripod->selected; tripod->selected = 0;

	switch(tripod->color_id)
	{
		case COLOR_ID_GIZMO_AX_X:	  	  tripod->selected |= SELECTED_AX_X;	     break;
		case COLOR_ID_GIZMO_AX_Y:	  	  tripod->selected |= SELECTED_AX_Y;	     break;
		case COLOR_ID_GIZMO_AX_Z:	  	  tripod->selected |= SELECTED_AX_Z;	     break;
		case COLOR_ID_GIZMO_PLANE_CAMERA: tripod->selected |= SELECTED_PLANE_CAMERA; break;
        case COLOR_ID_GIZMO_CIRCLE:       tripod->selected |= SELECTED_CIRCLE;       break;
	};

	/* emit process motion event */
	g_signal_emit(tripod, signals[PROCESS_MOTION], 0, x, y);

	/*
	 * Update selection (compose return value)
	 *
	 */

	if(tripod->selected)			/* mouse over tripod */
		value |= GIZMO_PROCESS_CURSOR;
	else
	if(tripod->sel_hit)			/* leaving tripod    */
		value |= GIZMO_PROCESS_REDRAW;

	/* save hit info */
	tripod->sel_hit = tripod->selected;

	if(tripod->selected != old_selection)
	{
		if(tripod->selected)		/* new selection */
		{
			value |= GIZMO_PROCESS_REDRAW;
			g_signal_emit(tripod, signals[UPDATE_SELECTION], 0);
		}else
		{
			tripod->selected  = tripod->sel_fallback;

			if(old_selection != tripod->sel_fallback)
				g_signal_emit(tripod, signals[UPDATE_SELECTION], 0);
		}
	}

    return value;
}

/*
 * This function returns a delta translate.
 */
const gtkrtg::vector3f &
gtk_rtg_tripod_process_drag(GtkRtgTripod *tripod, gint x, gint y)
{
	/* emit process motion event */
	g_signal_emit(tripod, signals[PROCESS_DRAG], 0, x, y);
	return tripod->delta_offset;
}

void
gtk_rtg_tripod_end_process_drag(GtkRtgTripod *tripod)
{
	X(tripod->delta_offset) = Y(tripod->delta_offset) = Z(tripod->delta_offset) = 0.f;
	tripod->drag = 0;
}
