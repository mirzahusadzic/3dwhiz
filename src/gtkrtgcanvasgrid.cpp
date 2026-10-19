#include <gtkrtg/gtkrtgcanvasgrid.h>
#include <gtkrtg/gtkrtgcanvas.h>
#include <gtkrtg/gtkrtgcamera.h>
#include "gtkrtg_gl_specs.h"
#include "gtkrtgextras.h"
#include "gtkrtgutils.h"
#include "gtkrtgglu.h"

#define GTK_RTG_CANVAS_GRID_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_RTG_TYPE_CANVAS_GRID, GtkRtgCanvasGridPrivate))

G_DEFINE_TYPE(GtkRtgCanvasGrid, gtk_rtg_canvas_grid, G_TYPE_OBJECT)

using namespace gtkrtg;

struct _GtkRtgCanvasGridPrivate
{
	gint lines;
	gfloat line_width;
	GdkColor line_color;
	gboolean fade_line_color;
	GdkColor cross_line_color;
	gfloat spacing;
    gfloat spacing_2;
    gfloat spacing_4;
	gfloat bottomleftpoint;
	gfloat square;
	GtkRtgColor4f basecolor;
	GtkRtgColor4f color;
	guint changes;
    gboolean snap;
    point3f snap_point;
	GdkColor c_snap;
    gboolean snap_init;
};

static void gtk_rtg_canvas_grid_set_property  (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gtk_rtg_canvas_grid_get_property  (GObject *object, guint prop_id, GValue       *value, GParamSpec *pspec);
static void	_gtk_rtg_canvas_grid_changed	  (GtkRtgCanvasGrid *grid);

/* properties */
enum
{
    PROP_0,
	PROP_LINES,
    PROP_LINE_WIDTH,
    PROP_LINE_COLOR,
	PROP_FADE_LINE_COLOR,
    PROP_CROSS_LINE_COLOR,
    PROP_LINE_SPACING,
    PROP_SNAP_CURSOR,
    LAST_ARG
};

enum
{
	S_PROP_LINES,
	S_PROP_LINE_WIDTH,
	S_PROP_LINE_COLOR,
	S_PROP_FADE_LINE_COLOR,
	S_PROP_CROSS_LINE_COLOR,
	S_PROP_LINE_SPACING,
    S_PROP_SNAP_CURSOR,
	S_LAST_PROP
};

static const gchar *prop[S_LAST_PROP]={"grid-lines",
									   "line-width",
									   "line-color",
									   "fade-line-color",
									   "cross-line-color",
									   "line-spacing",
									   "snap-cursor" };
enum
{
	CHANGE_COLOR	 = 0x02,
	CHANGE_DIMENSION = 0x04
};

/* constants */
#define LINE_COLOR_DEFAULT			"#606060"
#define CROSS_LINE_COLOR_DEFAULT	"black"
#define NUMBER_OF_GRID_LINES        15
#define LINE_WIDTH                  1.f
#define LINE_SPACING                1.f

static void
gtk_rtg_canvas_grid_class_init(GtkRtgCanvasGridClass *klass)
{
	GObjectClass  *object_class = (GObjectClass *) klass;

    object_class->set_property = gtk_rtg_canvas_grid_set_property;
    object_class->get_property = gtk_rtg_canvas_grid_get_property;

	/*
     * GtkRtgCanvasGrid: grid-lines
     *
     * The number of grids vertical and horizontal lines.
     *
     */
    g_object_class_install_property(object_class,
									PROP_LINES,
									g_param_spec_int(prop[S_PROP_LINES],
													 prop[S_PROP_LINES],
													 "The number of grids vertical/horizontal lines.",
													 0,
													 1000000,
													 NUMBER_OF_GRID_LINES,
													 GTK_RTG_PARAM_READWRITE));
	/*
     * GtkRtgCanvasGrid: line-width
     *
     * The width of grid line.
     *
     */
    g_object_class_install_property(object_class,
									PROP_LINE_WIDTH,
									g_param_spec_float(prop[S_PROP_LINE_WIDTH],
													   prop[S_PROP_LINE_WIDTH],
													   "The width of grid line.",
													   0.f,
													   10.f,
													   LINE_WIDTH,
													   GTK_RTG_PARAM_READWRITE));
 	/*
     * GtkRtgCanvasGrid: line-color
     *
     * The color of a grid lines.
     *
     */
	g_object_class_install_property(object_class,
									PROP_LINE_COLOR,
									g_param_spec_pointer(prop[S_PROP_LINE_COLOR],
														 prop[S_PROP_LINE_COLOR],
														 "The color of a grid lines.",
														 GTK_RTG_PARAM_READWRITE));
    /*
     * GtkRtgCanvasGrid: fade-line-color
     *
     * Whether line color should fade, depending on distance from viewer to the grid.
     *
     */
    g_object_class_install_property(object_class,
									PROP_FADE_LINE_COLOR,
									g_param_spec_boolean(prop[S_PROP_FADE_LINE_COLOR],
														 prop[S_PROP_FADE_LINE_COLOR],
														 "Whether line color should be faded depending on distance from viewer to the grid.",
														 TRUE,
														 GTK_RTG_PARAM_READWRITE));
	/*
     * GtkRtgCanvasGrid: cross-line-color
     *
     * The color of a grid cross lines.
     *
     */
	g_object_class_install_property(object_class,
									PROP_CROSS_LINE_COLOR,
									g_param_spec_pointer(prop[S_PROP_CROSS_LINE_COLOR],
														 prop[S_PROP_CROSS_LINE_COLOR],
														 "The color of a grid cross lines.",
														 GTK_RTG_PARAM_READWRITE));
	/*
     * GtkRtgCanvasGrid: line-spacing
     *
     * Distance between neighbor lines in units.
     *
     */
    g_object_class_install_property(object_class,
									PROP_LINE_SPACING,
									g_param_spec_float(prop[S_PROP_LINE_SPACING],
													   prop[S_PROP_LINE_SPACING],
													   "Distance between neighbor lines in units.",
													   0.f,
													   100.f,
													   LINE_SPACING,
													   GTK_RTG_PARAM_READWRITE));
 	/*
     * GtkRtgCanvasGrid: snap-cursor
     *
     * Whether to draw snap-cursor.
     *
     */
    g_object_class_install_property(object_class,
									PROP_SNAP_CURSOR,
									g_param_spec_boolean(prop[S_PROP_SNAP_CURSOR],
														 prop[S_PROP_SNAP_CURSOR],
														 "Whether to draw snap cursor.",
														 FALSE,
														 GTK_RTG_PARAM_READWRITE));

    g_type_class_add_private(G_OBJECT_CLASS(klass), sizeof(GtkRtgCanvasGridPrivate));
}


GtkRtgCanvasGrid *
gtk_rtg_canvas_grid_new()
{
	return GTK_RTG_CANVAS_GRID(g_object_new(GTK_RTG_TYPE_CANVAS_GRID, NULL));
}


static void
gtk_rtg_canvas_grid_init(GtkRtgCanvasGrid *grid)
{
	grid->priv = GTK_RTG_CANVAS_GRID_GET_PRIVATE(grid);

	grid->priv->lines = NUMBER_OF_GRID_LINES;
	grid->priv->line_width = LINE_WIDTH;
	grid->priv->spacing = LINE_SPACING;
    grid->priv->spacing_2 = grid->priv->spacing / 2.f;
    grid->priv->spacing_4 = grid->priv->spacing / 4.f;
    grid->priv->fade_line_color = TRUE;

	gdk_color_parse(LINE_COLOR_DEFAULT,	  	  &grid->priv->line_color);
	gdk_color_parse(CROSS_LINE_COLOR_DEFAULT, &grid->priv->cross_line_color);
    gdk_color_parse("gold",                   &grid->priv->c_snap);

	grid->priv->changes = CHANGE_COLOR | CHANGE_DIMENSION;

	/* init dimensions */
	_gtk_rtg_canvas_grid_changed(grid);
}


static void
gtk_rtg_canvas_grid_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GtkRtgCanvasGrid *grid = GTK_RTG_CANVAS_GRID(object);

	switch(prop_id)
	{
		case	PROP_LINES:				gtk_rtg_canvas_grid_set_lines(grid, g_value_get_int(value)); break;
		case	PROP_LINE_WIDTH:		gtk_rtg_canvas_grid_set_line_width(grid, g_value_get_float(value)); break;
		case	PROP_LINE_COLOR:		gtk_rtg_canvas_grid_set_line_color(grid, (GdkColor*)g_value_get_pointer(value)); break;
		case	PROP_FADE_LINE_COLOR:	gtk_rtg_canvas_grid_set_fade_line_color(grid, g_value_get_boolean(value)); break;
		case	PROP_CROSS_LINE_COLOR:	gtk_rtg_canvas_grid_set_cross_line_color(grid, (GdkColor*)g_value_get_pointer(value)); break;
		case	PROP_LINE_SPACING:      gtk_rtg_canvas_grid_set_spacing(grid, g_value_get_float(value)); break;
	};
}


static void
gtk_rtg_canvas_grid_get_property(GObject *object, guint prop_id, GValue	*value, GParamSpec *pspec)
{
	GtkRtgCanvasGrid *grid = GTK_RTG_CANVAS_GRID(object);

	switch(prop_id)
	{                                   /* getters without extra function calls */

		case	PROP_LINES:				g_value_set_int(value, grid->priv->lines); break;
		case	PROP_LINE_WIDTH:		g_value_set_float(value, grid->priv->line_width); break;
		case	PROP_LINE_COLOR:		g_value_set_pointer(value, &grid->priv->line_color); break;
		case	PROP_FADE_LINE_COLOR:	g_value_set_boolean(value, grid->priv->fade_line_color); break;
		case	PROP_CROSS_LINE_COLOR:	g_value_set_pointer(value, &grid->priv->cross_line_color); break;
		case	PROP_LINE_SPACING:		g_value_set_float(value, grid->priv->spacing); break;
	};
}


void
gtk_rtg_canvas_grid_render_snap_cursor(GtkRtgCanvasGrid *grid, GtkRtgCanvas *canvas) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_CANVAS_GRID(grid));
	g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));

    /* skip if not initialised (prevent redrawing on old position) */
    if(grid->priv->snap_init == FALSE)
        return;

    /* The cursor and its mouse ray intersect the same world-space grid plane. */
    const gfloat plane_depth = 0.f;
    const GLboolean depth_test = glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);
	GLint matrix_mode = 0;
	GLfloat line_width;

	glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);
	glGetFloatv(GL_LINE_WIDTH, &line_width);

	/* set width */
	glLineWidth(grid->priv->line_width);

	if(matrix_mode != GL_MODELVIEW)
		glMatrixMode(GL_MODELVIEW);

	gboolean a,b,c,d,e;
	DISABLE_GL_COLOR_MODIFIERS(a,b,c,d,e);

    glColor3usv(&grid->priv->c_snap.red);

    switch(gtk_rtg_canvas_view_get_up(canvas))
    {
        /*
         * Ensure for 'predefined' views that lines are
         * rendered orthogonal to the viewer (as grid lines).
         *
         */

        case GTK_RTG_CANVAS_VIEW_UP_Y:

            switch(gtk_rtg_canvas_view_get_predefined(canvas))
            {
                case GTK_RTG_CANVAS_VIEW_FRONT:
                case GTK_RTG_CANVAS_VIEW_BACK:

                    glBegin(GL_LINES);

                    /*
                     * Cross lines
                     */

                    /* horizontal */
                    glVertex3f(plane_depth, Y(grid->priv->snap_point), Z(grid->priv->snap_point) - grid->priv->spacing);
                    glVertex3f(plane_depth, Y(grid->priv->snap_point), Z(grid->priv->snap_point) + grid->priv->spacing);

                    /* vertical */
                    glVertex3f(plane_depth, Y(grid->priv->snap_point) - grid->priv->spacing, Z(grid->priv->snap_point));
                    glVertex3f(plane_depth, Y(grid->priv->snap_point) + grid->priv->spacing, Z(grid->priv->snap_point));


                    /*
                     * Square
                     */

                    /* horizontals */
                    glVertex3f(plane_depth, Y(grid->priv->snap_point)-grid->priv->spacing_4, Z(grid->priv->snap_point) - grid->priv->spacing_4);
                    glVertex3f(plane_depth, Y(grid->priv->snap_point)-grid->priv->spacing_4, Z(grid->priv->snap_point) + grid->priv->spacing_4);

                    glVertex3f(plane_depth, Y(grid->priv->snap_point)+grid->priv->spacing_4, Z(grid->priv->snap_point) - grid->priv->spacing_4);
                    glVertex3f(plane_depth, Y(grid->priv->snap_point)+grid->priv->spacing_4, Z(grid->priv->snap_point) + grid->priv->spacing_4);

                    /* verticals */
                    glVertex3f(plane_depth, Y(grid->priv->snap_point)-grid->priv->spacing_4, Z(grid->priv->snap_point) - grid->priv->spacing_4);
                    glVertex3f(plane_depth, Y(grid->priv->snap_point)+grid->priv->spacing_4, Z(grid->priv->snap_point) - grid->priv->spacing_4);

                    glVertex3f(plane_depth, Y(grid->priv->snap_point)-grid->priv->spacing_4, Z(grid->priv->snap_point) + grid->priv->spacing_4);
                    glVertex3f(plane_depth, Y(grid->priv->snap_point)+grid->priv->spacing_4, Z(grid->priv->snap_point) + grid->priv->spacing_4);

                    glEnd();

                    break;

                case GTK_RTG_CANVAS_VIEW_LEFT:
                case GTK_RTG_CANVAS_VIEW_RIGHT:

                    glBegin(GL_LINES);

                    /*
                     * Cross lines
                     */

                    /* horizontal */
                    glVertex3f(X(grid->priv->snap_point) - grid->priv->spacing, Y(grid->priv->snap_point), plane_depth);
                    glVertex3f(X(grid->priv->snap_point) + grid->priv->spacing, Y(grid->priv->snap_point), plane_depth);

                    /* vertical */
                    glVertex3f(X(grid->priv->snap_point), Y(grid->priv->snap_point) - grid->priv->spacing, plane_depth);
                    glVertex3f(X(grid->priv->snap_point), Y(grid->priv->snap_point) + grid->priv->spacing, plane_depth);


                    /*
                     * Square
                     */

                    /* horizontals */
                    glVertex3f(X(grid->priv->snap_point) - grid->priv->spacing_4, Y(grid->priv->snap_point)-grid->priv->spacing_4, plane_depth);
                    glVertex3f(X(grid->priv->snap_point) + grid->priv->spacing_4, Y(grid->priv->snap_point)-grid->priv->spacing_4, plane_depth);

                    glVertex3f(X(grid->priv->snap_point) - grid->priv->spacing_4, Y(grid->priv->snap_point)+grid->priv->spacing_4, plane_depth);
                    glVertex3f(X(grid->priv->snap_point) + grid->priv->spacing_4, Y(grid->priv->snap_point)+grid->priv->spacing_4, plane_depth);

                    /* verticals */
                    glVertex3f(X(grid->priv->snap_point) - grid->priv->spacing_4, Y(grid->priv->snap_point)-grid->priv->spacing_4, plane_depth);
                    glVertex3f(X(grid->priv->snap_point) - grid->priv->spacing_4, Y(grid->priv->snap_point)+grid->priv->spacing_4, plane_depth);

                    glVertex3f(X(grid->priv->snap_point) + grid->priv->spacing_4, Y(grid->priv->snap_point)-grid->priv->spacing_4, plane_depth);
                    glVertex3f(X(grid->priv->snap_point) + grid->priv->spacing_4, Y(grid->priv->snap_point)+grid->priv->spacing_4, plane_depth);

                    glEnd();

                    break;

                case GTK_RTG_CANVAS_VIEW_TOP:
                case GTK_RTG_CANVAS_VIEW_BOTTOM:
                case GTK_RTG_CANVAS_VIEW_CUSTOM:

                    glBegin(GL_LINES);

                    /*
                     * Cross lines
                     */

                    /* horizontal */
                    glVertex3f(X(grid->priv->snap_point) - grid->priv->spacing, plane_depth, Z(grid->priv->snap_point));
                    glVertex3f(X(grid->priv->snap_point) + grid->priv->spacing, plane_depth, Z(grid->priv->snap_point));

                    /* vertical */
                    glVertex3f(X(grid->priv->snap_point), plane_depth, Z(grid->priv->snap_point) - grid->priv->spacing);
                    glVertex3f(X(grid->priv->snap_point), plane_depth, Z(grid->priv->snap_point) + grid->priv->spacing);

                    /*
                     * Square
                     */

                    /* horizontals */
                    glVertex3f(X(grid->priv->snap_point) - grid->priv->spacing_4, plane_depth, Z(grid->priv->snap_point)-grid->priv->spacing_4);
                    glVertex3f(X(grid->priv->snap_point) + grid->priv->spacing_4, plane_depth, Z(grid->priv->snap_point)-grid->priv->spacing_4);

                    glVertex3f(X(grid->priv->snap_point) - grid->priv->spacing_4, plane_depth, Z(grid->priv->snap_point)+grid->priv->spacing_4);
                    glVertex3f(X(grid->priv->snap_point) + grid->priv->spacing_4, plane_depth, Z(grid->priv->snap_point)+grid->priv->spacing_4);

                    /* verticals */
                    glVertex3f(X(grid->priv->snap_point) - grid->priv->spacing_4, plane_depth, Z(grid->priv->snap_point)-grid->priv->spacing_4);
                    glVertex3f(X(grid->priv->snap_point) - grid->priv->spacing_4, plane_depth, Z(grid->priv->snap_point)+grid->priv->spacing_4);

                    glVertex3f(X(grid->priv->snap_point) + grid->priv->spacing_4, plane_depth, Z(grid->priv->snap_point)-grid->priv->spacing_4);
                    glVertex3f(X(grid->priv->snap_point) + grid->priv->spacing_4, plane_depth, Z(grid->priv->snap_point)+grid->priv->spacing_4);

                    glEnd();

                    break;
            };

            break;

        case GTK_RTG_CANVAS_VIEW_UP_Z:

            switch(gtk_rtg_canvas_view_get_predefined(canvas))
            {
                case GTK_RTG_CANVAS_VIEW_FRONT:
                case GTK_RTG_CANVAS_VIEW_BACK:

                    glBegin(GL_LINES);

                    /*
                     * Cross lines
                     */

                    /* horizontal */
                    glVertex3f(X(grid->priv->snap_point) - grid->priv->spacing, plane_depth, Z(grid->priv->snap_point));
                    glVertex3f(X(grid->priv->snap_point) + grid->priv->spacing, plane_depth, Z(grid->priv->snap_point));

                    /* vertical */
                    glVertex3f(X(grid->priv->snap_point), plane_depth, Z(grid->priv->snap_point) - grid->priv->spacing);
                    glVertex3f(X(grid->priv->snap_point), plane_depth, Z(grid->priv->snap_point) + grid->priv->spacing);

                    /*
                     * Square
                     */

                    /* horizontals */
                    glVertex3f(X(grid->priv->snap_point) - grid->priv->spacing_4, plane_depth, Z(grid->priv->snap_point)-grid->priv->spacing_4);
                    glVertex3f(X(grid->priv->snap_point) + grid->priv->spacing_4, plane_depth, Z(grid->priv->snap_point)-grid->priv->spacing_4);

                    glVertex3f(X(grid->priv->snap_point) - grid->priv->spacing_4, plane_depth, Z(grid->priv->snap_point)+grid->priv->spacing_4);
                    glVertex3f(X(grid->priv->snap_point) + grid->priv->spacing_4, plane_depth, Z(grid->priv->snap_point)+grid->priv->spacing_4);

                    /* verticals */
                    glVertex3f(X(grid->priv->snap_point) - grid->priv->spacing_4, plane_depth, Z(grid->priv->snap_point)-grid->priv->spacing_4);
                    glVertex3f(X(grid->priv->snap_point) - grid->priv->spacing_4, plane_depth, Z(grid->priv->snap_point)+grid->priv->spacing_4);

                    glVertex3f(X(grid->priv->snap_point) + grid->priv->spacing_4, plane_depth, Z(grid->priv->snap_point)-grid->priv->spacing_4);
                    glVertex3f(X(grid->priv->snap_point) + grid->priv->spacing_4, plane_depth, Z(grid->priv->snap_point)+grid->priv->spacing_4);

                    glEnd();

                    break;

                case GTK_RTG_CANVAS_VIEW_LEFT:
                case GTK_RTG_CANVAS_VIEW_RIGHT:

                    glBegin(GL_LINES);

                    /*
                     * Cross lines
                     */

                    /* horizontal */
                    glVertex3f(plane_depth, Y(grid->priv->snap_point), Z(grid->priv->snap_point) - grid->priv->spacing);
                    glVertex3f(plane_depth, Y(grid->priv->snap_point), Z(grid->priv->snap_point) + grid->priv->spacing);

                    /* vertical */
                    glVertex3f(plane_depth, Y(grid->priv->snap_point) - grid->priv->spacing, Z(grid->priv->snap_point));
                    glVertex3f(plane_depth, Y(grid->priv->snap_point) + grid->priv->spacing, Z(grid->priv->snap_point));

                    /*
                     * Square
                     */

                    /* horizontals */
                    glVertex3f(plane_depth, Y(grid->priv->snap_point)-grid->priv->spacing_4, Z(grid->priv->snap_point) - grid->priv->spacing_4);
                    glVertex3f(plane_depth, Y(grid->priv->snap_point)-grid->priv->spacing_4, Z(grid->priv->snap_point) + grid->priv->spacing_4);

                    glVertex3f(plane_depth, Y(grid->priv->snap_point)+grid->priv->spacing_4, Z(grid->priv->snap_point) - grid->priv->spacing_4);
                    glVertex3f(plane_depth, Y(grid->priv->snap_point)+grid->priv->spacing_4, Z(grid->priv->snap_point) + grid->priv->spacing_4);

                    /* verticals */
                    glVertex3f(plane_depth, Y(grid->priv->snap_point)-grid->priv->spacing_4, Z(grid->priv->snap_point) - grid->priv->spacing_4);
                    glVertex3f(plane_depth, Y(grid->priv->snap_point)+grid->priv->spacing_4, Z(grid->priv->snap_point) - grid->priv->spacing_4);

                    glVertex3f(plane_depth, Y(grid->priv->snap_point)-grid->priv->spacing_4, Z(grid->priv->snap_point) + grid->priv->spacing_4);
                    glVertex3f(plane_depth, Y(grid->priv->snap_point)+grid->priv->spacing_4, Z(grid->priv->snap_point) + grid->priv->spacing_4);

                    glEnd();

                    break;

                case GTK_RTG_CANVAS_VIEW_TOP:
                case GTK_RTG_CANVAS_VIEW_BOTTOM:
                case GTK_RTG_CANVAS_VIEW_CUSTOM:

                    glBegin(GL_LINES);

                    /*
                     * Cross lines
                     */

                    /* horizontal */
                    glVertex3f(X(grid->priv->snap_point) - grid->priv->spacing, Y(grid->priv->snap_point), plane_depth);
                    glVertex3f(X(grid->priv->snap_point) + grid->priv->spacing, Y(grid->priv->snap_point), plane_depth);

                    /* vertical */
                    glVertex3f(X(grid->priv->snap_point), Y(grid->priv->snap_point) - grid->priv->spacing, plane_depth);
                    glVertex3f(X(grid->priv->snap_point), Y(grid->priv->snap_point) + grid->priv->spacing, plane_depth);

                    /*
                     * Square
                     */

                    /* horizontals */
                    glVertex3f(X(grid->priv->snap_point) - grid->priv->spacing_4, Y(grid->priv->snap_point)-grid->priv->spacing_4, plane_depth);
                    glVertex3f(X(grid->priv->snap_point) + grid->priv->spacing_4, Y(grid->priv->snap_point)-grid->priv->spacing_4, plane_depth);

                    glVertex3f(X(grid->priv->snap_point) - grid->priv->spacing_4, Y(grid->priv->snap_point)+grid->priv->spacing_4, plane_depth);
                    glVertex3f(X(grid->priv->snap_point) + grid->priv->spacing_4, Y(grid->priv->snap_point)+grid->priv->spacing_4, plane_depth);

                    /* verticals */
                    glVertex3f(X(grid->priv->snap_point) - grid->priv->spacing_4, Y(grid->priv->snap_point)-grid->priv->spacing_4, plane_depth);
                    glVertex3f(X(grid->priv->snap_point) - grid->priv->spacing_4, Y(grid->priv->snap_point)+grid->priv->spacing_4, plane_depth);

                    glVertex3f(X(grid->priv->snap_point) + grid->priv->spacing_4, Y(grid->priv->snap_point)-grid->priv->spacing_4, plane_depth);
                    glVertex3f(X(grid->priv->snap_point) + grid->priv->spacing_4, Y(grid->priv->snap_point)+grid->priv->spacing_4, plane_depth);

                    glEnd();

                    break;
            };

            break;
    };

	ENABLE_GL_COLOR_MODIFIERS(a,b,c,d,e);
    if(depth_test)
    {
        glEnable(GL_DEPTH_TEST);
    }

	/* preserve line width and matrix mode */
	glLineWidth(line_width);

	if(matrix_mode != GL_MODELVIEW)
		glMatrixMode(matrix_mode);
}


/*
 * Publics
 *
 */
void
gtk_rtg_canvas_grid_render(GtkRtgCanvasGrid *grid, GtkRtgCanvas *canvas) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_CANVAS_GRID(grid));
	g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));

	GtkRtgCamera *camera = gtk_rtg_canvas_camera_get_default(canvas);

	gdouble	view_distance = 0.f;
	gfloat z_far = 0.f;

	if(camera)
    {
		/* query view distance from camera */
		g_object_get(G_OBJECT(camera), "view-distance", &view_distance, NULL);

        g_object_get(G_OBJECT(camera), "proj-zfar", &z_far, NULL);

        /* shift 25% closer to the z_near plane */
        z_far *= 0.75f;

        switch(gtk_rtg_canvas_view_get_predefined(canvas))
        {
            case GTK_RTG_CANVAS_VIEW_BACK:
            case GTK_RTG_CANVAS_VIEW_RIGHT:
            case GTK_RTG_CANVAS_VIEW_TOP:    z_far *= -1.f; break;
            case GTK_RTG_CANVAS_VIEW_CUSTOM: z_far  =  0.f; break;

            default: break;
        };
    }

    gint half_lines  = (grid->priv->lines >> 1) - 1;
	GLint matrix_mode = 0;
	GLfloat line_width;

	glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);
	glGetFloatv(GL_LINE_WIDTH, &line_width);

	/* set width */
	glLineWidth(grid->priv->line_width);

	if(matrix_mode != GL_MODELVIEW)
		glMatrixMode(GL_MODELVIEW);

	gboolean a,b,c,d,e;
	DISABLE_GL_COLOR_MODIFIERS(a,b,c,d,e);

	/* lines are projected onto 2D plane */
	for(gint n=0; n<grid->priv->lines; n++)
	{
		gfloat part = grid->priv->spacing * (gfloat)n;

		gfloat v1 = n < half_lines ?  grid->priv->bottomleftpoint < 0 ? grid->priv->bottomleftpoint + part
																	  : grid->priv->bottomleftpoint - part
								   :  grid->priv->bottomleftpoint < 0 ? grid->priv->bottomleftpoint + part
										   	   	   	   	   	   	   	  :  part - grid->priv->bottomleftpoint;
		if(n != half_lines+1)
		{
			/* skip drawing grid lines for big zoom outs */
			if(view_distance > grid->priv->square * 15.f /*factor*/)
				continue;
			else
			/* normal 'colorized' gird lines */
				glColor3f(grid->priv->color.r, grid->priv->color.g, grid->priv->color.b);
		}else
		/* center line color */
			glColor3usv(&grid->priv->cross_line_color.red);

		/* render lines */
		glBegin(GL_LINES);

        switch(gtk_rtg_canvas_view_get_up(canvas))
        {
			/*
			 * Ensure for 'predefined' views that lines are
			 * rendered orthogonal to the viewer.
			 *
			 */

			case GTK_RTG_CANVAS_VIEW_UP_Y:

				switch(gtk_rtg_canvas_view_get_predefined(canvas))
				{
					case GTK_RTG_CANVAS_VIEW_FRONT:
					case GTK_RTG_CANVAS_VIEW_BACK:

						/* horizontal line */
						glVertex3f( z_far,  grid->priv->bottomleftpoint, v1);
						glVertex3f( z_far, -grid->priv->bottomleftpoint, v1);

						/* vertical line */
						glVertex3f( z_far, v1,  grid->priv->bottomleftpoint);
						glVertex3f( z_far, v1, -grid->priv->bottomleftpoint);
						break;

					case GTK_RTG_CANVAS_VIEW_LEFT:
					case GTK_RTG_CANVAS_VIEW_RIGHT:

						/* horizontal line */
						glVertex3f(  grid->priv->bottomleftpoint, v1, z_far);
						glVertex3f( -grid->priv->bottomleftpoint, v1, z_far);

						/* vertical line */
						glVertex3f( v1,  grid->priv->bottomleftpoint, z_far);
						glVertex3f( v1, -grid->priv->bottomleftpoint, z_far);

						break;

					case GTK_RTG_CANVAS_VIEW_TOP:
					case GTK_RTG_CANVAS_VIEW_BOTTOM:
					case GTK_RTG_CANVAS_VIEW_CUSTOM:

						/* horizontal line */
						glVertex3f(  grid->priv->bottomleftpoint, z_far, v1);
						glVertex3f( -grid->priv->bottomleftpoint, z_far, v1);

						/* vertical line */
						glVertex3f( v1, z_far,  grid->priv->bottomleftpoint);
						glVertex3f( v1, z_far, -grid->priv->bottomleftpoint);

						break;
				};

				break;

			case GTK_RTG_CANVAS_VIEW_UP_Z:

				switch(gtk_rtg_canvas_view_get_predefined(canvas))
				{
                    case GTK_RTG_CANVAS_VIEW_FRONT:
                    case GTK_RTG_CANVAS_VIEW_BACK:

						/* horizontal line */
						glVertex3f(  grid->priv->bottomleftpoint, z_far, v1);
						glVertex3f( -grid->priv->bottomleftpoint, z_far, v1);

						/* vertical line */
						glVertex3f( v1,  z_far,  grid->priv->bottomleftpoint);
						glVertex3f( v1,  z_far, -grid->priv->bottomleftpoint);

						break;

					case GTK_RTG_CANVAS_VIEW_LEFT:
                    case GTK_RTG_CANVAS_VIEW_RIGHT:

						/* horizontal line */
						glVertex3f( z_far,  grid->priv->bottomleftpoint, v1);
						glVertex3f( z_far, -grid->priv->bottomleftpoint, v1);

						/* vertical line */
						glVertex3f( z_far, v1,  grid->priv->bottomleftpoint);
						glVertex3f( z_far, v1, -grid->priv->bottomleftpoint);

						break;

                    case GTK_RTG_CANVAS_VIEW_TOP:
                    case GTK_RTG_CANVAS_VIEW_BOTTOM:
                    case GTK_RTG_CANVAS_VIEW_CUSTOM:

						/* horizontal line */
						glVertex3f(  grid->priv->bottomleftpoint, v1, z_far);
						glVertex3f( -grid->priv->bottomleftpoint, v1, z_far);

						/* vertical line */
						glVertex3f( v1,   grid->priv->bottomleftpoint,  z_far);
						glVertex3f( v1,  -grid->priv->bottomleftpoint,  z_far);

						break;
				};

                break;
        };

		glEnd();

	}

	ENABLE_GL_COLOR_MODIFIERS(a,b,c,d,e);

	/* preserve line width and matrix mode */
	glLineWidth(line_width);

	if(matrix_mode != GL_MODELVIEW)
		glMatrixMode(matrix_mode);
}


void
gtk_rtg_canvas_grid_set_lines(GtkRtgCanvasGrid *grid, gint lines) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS_GRID(grid));

	/* the number of vertical/horizontal lines should be (>3) and (mod 2 !=0) */
	(lines < 3) ? grid->priv->lines = 3
				: (lines % 2) ? grid->priv->lines = lines
							  : grid->priv->lines = lines + 1;

	grid->priv->changes |= CHANGE_DIMENSION;

    _gtk_rtg_canvas_grid_changed(grid);

	g_object_notify(G_OBJECT(grid), prop[S_PROP_LINES]);
}


gint
gtk_rtg_canvas_grid_get_lines(GtkRtgCanvasGrid *grid) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS_GRID(grid));
	return grid->priv->lines;
}


void
gtk_rtg_canvas_grid_set_line_width(GtkRtgCanvasGrid *grid, gfloat width) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS_GRID(grid));
	width <= 0.f ? grid->priv->line_width = 1.f : grid->priv->line_width = width;

    _gtk_rtg_canvas_grid_changed(grid);

	g_object_notify(G_OBJECT(grid), prop[S_PROP_LINE_WIDTH]);
}


gfloat
gtk_rtg_canvas_grid_get_line_width(GtkRtgCanvasGrid *grid) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS_GRID(grid));
	return grid->priv->line_width;
}


void
gtk_rtg_canvas_grid_set_line_color(GtkRtgCanvasGrid *grid, GdkColor *color) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS_GRID(grid));
	g_throw_if_fail(color != NULL);
	grid->priv->line_color = *color;

	grid->priv->changes |= CHANGE_COLOR;

    _gtk_rtg_canvas_grid_changed(grid);

	g_object_notify(G_OBJECT(grid), prop[S_PROP_LINE_COLOR]);
}


GdkColor *
gtk_rtg_canvas_grid_get_line_color(GtkRtgCanvasGrid *grid) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS_GRID(grid));
	return &grid->priv->line_color;
}


void
gtk_rtg_canvas_grid_set_fade_line_color(GtkRtgCanvasGrid *grid, gboolean fade) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS_GRID(grid));
	grid->priv->fade_line_color = fade;

	if(fade == FALSE)
		grid->priv->changes |= CHANGE_COLOR;

    _gtk_rtg_canvas_grid_changed(grid);

	g_object_notify(G_OBJECT(grid), prop[S_PROP_FADE_LINE_COLOR]);
}


gboolean
gtk_rtg_canvas_grid_get_fade_line_color(GtkRtgCanvasGrid *grid) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS_GRID(grid));
	return grid->priv->fade_line_color;
}


void
gtk_rtg_canvas_grid_fade_line_color(GtkRtgCanvasGrid *grid, gfloat distance)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS_GRID(grid));

	if(grid->priv->fade_line_color)
	{
		if(distance < 1.f)
			distance = 1.f;

		gfloat	ratio = (grid->priv->square*.5f)/distance;

		if(ratio > .3f)
			ratio = .3f;

		ratio = 1.3f - ratio;

		/*
		 * Fade grid-line color (depending on grid size and view distance)
		 * the color spectar which is interesting is from 1 (original color) - 1.3 (brighter)
		 */
		GtkRtgColor4f	 composite;
		shade_color(&grid->priv->basecolor, ratio, &composite);
		grid->priv->color = composite;
	}
}


void
gtk_rtg_canvas_grid_set_cross_line_color(GtkRtgCanvasGrid *grid, GdkColor *color) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS_GRID(grid));
	g_throw_if_fail(color != NULL);
	grid->priv->cross_line_color = *color;

    _gtk_rtg_canvas_grid_changed(grid);

	g_object_notify(G_OBJECT(grid), prop[S_PROP_CROSS_LINE_COLOR]);
}


GdkColor *
gtk_rtg_canvas_grid_get_cross_line_color(GtkRtgCanvasGrid *grid) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS_GRID(grid));
	return &grid->priv->cross_line_color;
}


void
gtk_rtg_canvas_grid_set_spacing(GtkRtgCanvasGrid *grid, gfloat spacing) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS_GRID(grid));
	spacing <= 0.f ? grid->priv->spacing = 1.f : grid->priv->spacing = spacing;

    grid->priv->spacing_2 = grid->priv->spacing / 2.f;
    grid->priv->spacing_4 = grid->priv->spacing / 4.f;

	grid->priv->changes |= CHANGE_DIMENSION;

    _gtk_rtg_canvas_grid_changed(grid);

    g_object_notify(G_OBJECT(grid), prop[S_PROP_LINE_SPACING]);
}


gfloat
gtk_rtg_canvas_grid_get_spacing(GtkRtgCanvasGrid *grid) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS_GRID(grid));
	return grid->priv->spacing;
}


void
gtk_rtg_canvas_grid_set_snap_cursor(GtkRtgCanvasGrid *grid, gboolean snap)  throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS_GRID(grid));
    grid->priv->snap = snap;

    if(snap == FALSE)
        grid->priv->snap_init = snap;

    g_object_notify(G_OBJECT(grid), prop[S_PROP_SNAP_CURSOR]);
}


gboolean
gtk_rtg_canvas_grid_get_snap_cursor(GtkRtgCanvasGrid *grid) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS_GRID(grid));
    return grid->priv->snap;
}


point3f &
gtk_rtg_canvas_grid_get_snap_point(GtkRtgCanvasGrid *grid) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS_GRID(grid));
    return grid->priv->snap_point;
}


/*
 * Privs.
 *
 */

extern point3f* canvas_grid_get_face_normal(GtkRtgCanvas *canvas);

void
_gtk_rtg_canvas_grid_process_motion(GtkRtgCanvasGrid *grid, GtkRtgCanvas *canvas, gfloat x, gfloat y)
{
    /* Mouse coordinates and the framebuffer both use logical canvas pixels. */
    GtkWidget *widget = GTK_WIDGET(canvas);
    gint viewport[4] = {0, 0, gtk_widget_get_allocated_width(widget),
                             gtk_widget_get_allocated_height(widget)};
    y = viewport[3] - y;
    grid->priv->snap_init = FALSE;

    GtkRtgCamera *cam = gtk_rtg_canvas_camera_get_default(canvas);

	/*
	 * Get line segment.
	 *
	 */

    enum{SEGMENT_START, SEGMENT_END, SEGMENT_POINTS};
    point3f p[SEGMENT_POINTS];
    gluUnProjectf(x, y, 0.f, M_INVERSE(cam).m, viewport, &X(p[SEGMENT_START]), &Y(p[SEGMENT_START]), &Z(p[SEGMENT_START]));
    gluUnProjectf(x, y, 1.f, M_INVERSE(cam).m, viewport, &X(p[SEGMENT_END]),   &Y(p[SEGMENT_END]),   &Z(p[SEGMENT_END]));

	/*
	 * Get plane (normal) on which screen coordinates will be projected.
	 */
    plane<float> pl;
    pl.normal = *canvas_grid_get_face_normal(canvas);
    pl.d = 0.f;

    /*
     * Find actual point
     */
	if(intersect_segment_plane<float>(p[SEGMENT_START], p[SEGMENT_END], pl, grid->priv->snap_point))
	{
        /*
         * Do the snapping
         */

        gfloat x_left = fmodf(X(grid->priv->snap_point), grid->priv->spacing);
        gfloat y_left = fmodf(Y(grid->priv->snap_point), grid->priv->spacing);
        gfloat z_left = fmodf(Z(grid->priv->snap_point), grid->priv->spacing);

        /* floor to grid point */
        X(grid->priv->snap_point) = X(grid->priv->snap_point) - x_left;
        Y(grid->priv->snap_point) = Y(grid->priv->snap_point) - y_left;
        Z(grid->priv->snap_point) = Z(grid->priv->snap_point) - z_left;

        /* ceil to grid point */
        if(fabsf(x_left) > grid->priv->spacing_2)  X(grid->priv->snap_point) += x_left > 0.f ? grid->priv->spacing : -grid->priv->spacing;
        if(fabsf(y_left) > grid->priv->spacing_2)  Y(grid->priv->snap_point) += y_left > 0.f ? grid->priv->spacing : -grid->priv->spacing;
        if(fabsf(z_left) > grid->priv->spacing_2)  Z(grid->priv->snap_point) += z_left > 0.f ? grid->priv->spacing : -grid->priv->spacing;

        /* init */
        grid->priv->snap_init = TRUE;
    }
}


static void
_gtk_rtg_canvas_grid_changed(GtkRtgCanvasGrid *grid)
{
	/* Check whether grids color of dimensions should be set/recalculated */
    if(grid->priv->changes & CHANGE_DIMENSION)
	{
		grid->priv->bottomleftpoint = -((gfloat)(grid->priv->lines>>1) * grid->priv->spacing);
		grid->priv->square = (gfloat)grid->priv->lines * grid->priv->spacing;
		grid->priv->changes &= ~CHANGE_DIMENSION;
	}

	if(grid->priv->changes & CHANGE_COLOR)
	{
		grid->priv->basecolor.r = (gfloat) grid->priv->line_color.red   / 65535.f;
		grid->priv->basecolor.g = (gfloat) grid->priv->line_color.green / 65535.f;
		grid->priv->basecolor.b = (gfloat) grid->priv->line_color.blue  / 65535.f;
		grid->priv->basecolor.a = 1.f;

		grid->priv->color = grid->priv->basecolor;
		grid->priv->changes &= ~CHANGE_COLOR;
	}
}
