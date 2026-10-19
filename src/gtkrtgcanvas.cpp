#ifdef _MSC_VER
#pragma warning(disable : 4786)
#pragma warning(disable : 4290)
#endif

#include <gtkrtg/gtkrtgcamera.h>
#include <gtkrtg/gtkrtgscene.h>
#include <gtkrtg/gtkrtgcanvas.h>
#include <gtkrtg/gtkrtgmath.h>
#include "gtkrtg_gl_specs.h"
#include "gtkrtgextras.h"
#include "gtkrtgutils.h"
#include <glib/gprintf.h>
#include <gdk/gdkkeysyms.h>
#include "gtkrtgtripod.h"
#include "gtkrtgmovegizmo.h"
#include "gtkrtgscalegizmo.h"
#include "gtkrtgrotategizmo.h"

#include <map>
#include <vector>
#include <algorithm>
#include <functional>

#include "gtkrtgresources.h"
#include "gtkrtgglbackend.h"

#define GTK_RTG_CANVAS_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_RTG_TYPE_CANVAS, GtkRtgCanvasPrivate))

G_DEFINE_TYPE(GtkRtgCanvas, gtk_rtg_canvas, GTK_TYPE_DRAWING_AREA)

    using namespace std;
    using namespace gtkrtg;

    /* view, gizmo & camera(s) */
    typedef struct
{
    /*
     * altitude - azimuth view (fixed up vector).
     */

    vector3d *dir;	 /* direction vector */
    vector3d *up;	 /* up vector	 */
    gdouble	  alt;	 /* altitude	 */
    gdouble	  az;	 /* azimuth	     */
    gdouble	  mx;	 /* last mouse x */
    gdouble	  my;	 /* last mouse y */
    gdouble	  depth; /* zoom depth	 */

}GtkRtgCanvasViewAltAz;


#define VIEW_GIZMO_POINTS	64
#define VIEW_GIZMO_RECTS	4
#define VIEW_GIZMO_SCALE	 .75f

typedef struct
{
    GLfloat	 width;
    GLfloat	 height;
    GLfloat	 width_2;
    GLfloat	 height_2;
    GLfloat	 radius;
    GLfloat	 theta;
    GdkColor color;
    point2f	 point[VIEW_GIZMO_POINTS];
    point2f *rect[VIEW_GIZMO_RECTS];
}GtkRtgCanvasViewGizmo;

typedef struct
{
    GtkRtgCamera		       *cam;		/* views camera            */
    GtkRtgPickingBuffer         pickbuff;   /* picking buffer          */
    GtkRtgCanvasViewAltAz	    altaz;      /* altaz support           */
    GtkRtgCanvasViewGizmo	    gizmo;      /* gizmo                   */
    GtkRtgCanvasViewUp          up;         /* up direction (ax)		*/
    GtkRtgCanvasViewPredefined	predefined; /* predefined view		    */
    GtkRtgText                 *text;       /* predefined text object	*/
    GtkRtgTripod *tripod;
    GtkRtgTripod *gizmo_move;
    GtkRtgTripod *gizmo_scale;
    GtkRtgTripod *gizmo_rotate;

    /* flags */
    guint	pan_init:1;
    point3f pan_pos;
    guint	sel_postponed:1;
    guint   can_drag:1;

}GtkRtgCanvasView;


typedef struct
{
    GdkPoint start;
    GdkPoint end;
    GdkColor color;
}GtkRtgCanvasMarquee;


enum	/* callbacks data-indexes */
{
    GTK_RTG_INIT,
    GTK_RTG_RENDER,
    GTK_RTG_DESTROY,
    GTK_RTG_RESIZE,
    GTK_RTG_TIMEOUT,
    GTK_RTG_FUNCTIONS
};

struct _GtkRtgCanvasPrivate
{
    /* callbacks */
    GTK_RTG_FUNCT_OBJECT init;
    GTK_RTG_FUNCT_PURE	 render;
    GTK_RTG_FUNCT_OBJECT destroy;
    GTK_RTG_FUNCT_PURE	 timeout;
    GTK_RTG_FUNCT_RESIZE resize;

    gpointer data[GTK_RTG_FUNCTIONS];

    /* animation handling */
    guint timeout_interval;
    guint timeout_id;

    /* mouse wheel zoom */
    guint	wheel;
    gdouble zmultiplier;

    /* OpenGL context */
    GtkRtgGLBackend *backend;
    gint buffer_width, buffer_height;
    guint wheel_timeout_id;

    /* real objects */
    GtkRtgCanvasGrid *grid;
    m_txtType		 *m_text;	/* pushed texts to the render map  */

    /* meta types (containers) */
    GtkRtgScene		 *scene;	/* pushed scene object	*/
    GtkRtgCanvasView  view;

    /* widget cursors */
    GtkRtgCanvasCursor	curs_id;

    /* id tracker */
    gchar		  *id;           /* canvas id {instance ptr, :}	*/
    vector<guint> *v_obj_id;	 /* list of available object ids	*/
    guint		   id_peek;		 /* current id counter	(peek of pushed objects) */

    /* flags */
    guint	 display;
    gboolean is_configured;
    gboolean is_animated;
    gboolean is_view;

    /* scene */
    GtkRtgSceneRenderMode render_mode;
    guint viewframe:1;
    guint solid_edge:1;

    /* interactive mode */
    GtkRtgCanvasInteractiveMode	mode;
    GtkRtgCanvasMarquee marquee;		/* marquee selection */

    /* colors */
    GdkColor c_focused;
    GdkColor c_border;
    GdkColor c_background;
};


enum	/* DISPLAY FLAGS */
{
    DISPLAY_GRID  = 0x0000002,
    DISPLAY_SCENE = 0x0000004,
    DISPLAY_GRID_SNAP_CURSOR = 0x0000008,

    /* orthographic view begins from here  */
    DISPLAY_ORTHO = 0x0010000,

    DISPLAY_TEXT		= 0x0020000,
    DISPLAY_VIEW_GIZMO	= 0x0040000,
    DISPLAY_FOCUSED		= 0x0080000,
    DISPLAY_MARQUEE		= 0x0100000,

    /* custom projection */
    DISPLAY_TRIPOD		 = 0x0200000,
    DISPLAY_MOVE_GIZMO   = 0x0400000,
    DISPLAY_SCALE_GIZMO  = 0x0800000,
    DISPLAY_ROTATE_GIZMO = 0x1000000
};


/* properties */
enum
{
    PROP_0,
    PROP_BG_COLOR,
    LAST_ARG
};

/* signals */
enum
{
    FOCUSED,
    RENDER_OFFSCREEN,
    GIZMO_POSITION,
    LAST_SIGNAL
};

static  gint signals[LAST_SIGNAL] = { 0 };

/* constants */
#define BG_COLOR_DEFAULT "#7d7d7d"

/*
 * Predefined views direction vectors
 *
 */
static vector3d p_view_dir[GTK_RTG_CANVAS_VIEW_UP_Z+1][GTK_RTG_CANVAS_VIEW_CUSTOM] =
{
    {       /* GTK_RTG_CANVAS_VIEW_UP_Y */

        vector3d( 1.0,  0.0,  0.0),	/* GTK_RTG_CANVAS_VIEW_FRONT  */
        vector3d(-1.0,  0.0,  0.0),	/* GTK_RTG_CANVAS_VIEW_BACK   */
        vector3d( 0.0, -1.0,  0.1e-20),	/* GTK_RTG_CANVAS_VIEW_TOP    */
        vector3d( 0.0,  1.0,  0.1e-20),	/* GTK_RTG_CANVAS_VIEW_BOTTOM */
        vector3d( 0.0,  0.0,  1.0),	/* GTK_RTG_CANVAS_VIEW_LEFT   */
        vector3d( 0.0,  0.0, -1.0)	/* GTK_RTG_CANVAS_VIEW_RIGHT  */
    },

    {       /* GTK_RTG_CANVAS_VIEW_UP_Z */

        vector3d( 0.0,  1.0,	  0.0),	/* GTK_RTG_CANVAS_VIEW_FRONT  */
        vector3d( 0.0, -1.0,	  0.0),	/* GTK_RTG_CANVAS_VIEW_BACK   */
        vector3d( 0.0,  0.1e-20, -1.0),	/* GTK_RTG_CANVAS_VIEW_TOP    */
        vector3d( 0.0,  0.1e-20,  1.0),	/* GTK_RTG_CANVAS_VIEW_BOTTOM */
        vector3d( 1.0,	0.0,	  0.0),	/* GTK_RTG_CANVAS_VIEW_LEFT   */
        vector3d(-1.0,	0.0,	  0.0)	/* GTK_RTG_CANVAS_VIEW_RIGHT  */
    }
};


/*
 * Predefined plane normals used for various tests (i.e. mouse ray segment vs. plane).
 *
 */
static vector3f p_plane_norm[GTK_RTG_CANVAS_VIEW_UP_Z+1][GTK_RTG_CANVAS_VIEW_CUSTOM] =
{
    {	/* GTK_RTG_CANVAS_VIEW_UP_Y */

        vector3f(-1.f,  0.f,  0.f),	/* GTK_RTG_CANVAS_VIEW_FRONT  */
        vector3f( 1.f,  0.f,  0.f),	/* GTK_RTG_CANVAS_VIEW_BACK   */
        vector3f( 0.f,  1.f,  0.f),	/* GTK_RTG_CANVAS_VIEW_TOP    */
        vector3f( 0.f, -1.f,  0.f),	/* GTK_RTG_CANVAS_VIEW_BOTTOM */
        vector3f( 0.f,  0.f, -1.f),	/* GTK_RTG_CANVAS_VIEW_LEFT   */
        vector3f( 0.f,  0.f,  1.f)	/* GTK_RTG_CANVAS_VIEW_RIGHT  */
    },

    {	/* GTK_RTG_CANVAS_VIEW_UP_Z */

        vector3f( 0.f, -1.f,  0.f),	/* GTK_RTG_CANVAS_VIEW_FRONT  */
        vector3f( 0.f,  1.f,  0.f),	/* GTK_RTG_CANVAS_VIEW_BACK   */
        vector3f( 0.f,  0.f,  1.f),	/* GTK_RTG_CANVAS_VIEW_TOP    */
        vector3f( 0.f,  0.f, -1.f),	/* GTK_RTG_CANVAS_VIEW_BOTTOM */
        vector3f(-1.f,	0.f,  0.f),	/* GTK_RTG_CANVAS_VIEW_LEFT   */
        vector3f( 1.f,	0.f,  0.f)	/* GTK_RTG_CANVAS_VIEW_RIGHT  */
    }
};


point3f*
canvas_grid_get_face_normal(GtkRtgCanvas *canvas)
{
    point3f *normal = NULL;

    switch(canvas->priv->view.predefined)
    {
        case GTK_RTG_CANVAS_VIEW_FRONT:
        case GTK_RTG_CANVAS_VIEW_BACK:
        case GTK_RTG_CANVAS_VIEW_TOP:
        case GTK_RTG_CANVAS_VIEW_BOTTOM:
        case GTK_RTG_CANVAS_VIEW_LEFT:
        case GTK_RTG_CANVAS_VIEW_RIGHT: normal = &p_plane_norm[canvas->priv->view.up][canvas->priv->view.predefined]; break;
        case GTK_RTG_CANVAS_VIEW_CUSTOM:normal = &p_plane_norm[canvas->priv->view.up][GTK_RTG_CANVAS_VIEW_TOP]; break;
    };

    return normal;
}

/*
 * predefined text
 *
 */
static const gchar *p_view_text[GTK_RTG_CANVAS_VIEW_CUSTOM+1] = {"Front",
                                                                 "Back",
                                                                 "Top",
                                                                 "Bottom",
                                                                 "Left",
                                                                 "Right",
                                                                 "Custom"};

/* macros */
#define IS_MODE_VIEW(canvas)	(canvas->priv->mode == GTK_RTG_CANVAS_INT_MODE_VIEW)
#define IS_MODE_OBJECT(canvas)	(canvas->priv->mode > GTK_RTG_CANVAS_INT_MODE_VIEW)
#define SWITCH_CONTEXT(canvas) gtk_rtg_gl_backend_make_current(canvas->priv->backend)
#define PROCESS_WHEEL_ZOOM(canvas) \
if(canvas->priv->wheel > 0) canvas->priv->wheel += 2; \
else \
{ \
    canvas->priv->wheel = 5; \
    canvas->priv->wheel_timeout_id = g_timeout_add(5, (GSourceFunc)wheel_zoom_anim, canvas); \
}


/* signals */
static void     gtk_rtg_canvas_unrealize		 (GtkWidget *widget, gpointer data);
static gboolean gtk_rtg_canvas_button_press		 (GtkWidget *widget, GdkEventButton	*event);
static gboolean gtk_rtg_canvas_button_release	 (GtkWidget *widget, GdkEventButton	*event);
static gboolean gtk_rtg_canvas_motion_notify	 (GtkWidget *widget, GdkEventMotion	*event);
static gboolean gtk_rtg_canvas_expose			 (GtkWidget *widget, cairo_t *cr, gpointer data);
gboolean        gtk_rtg_canvas_scroll_event		 (GtkWidget *widget, GdkEventScroll     *event);

static void	gtk_rtg_canvas_camera_view		 	 (GtkRtgCamera *camera, GParamSpec *pspec, GtkRtgCanvas *canvas);
static void	gtk_rtg_canvas_camera_view_up		 (GtkRtgCamera *camera, GParamSpec *pspec, GtkRtgCanvas *canvas);
static void	gtk_rtg_canvas_camera_projection	 (GtkRtgCamera *camera, GParamSpec *pspec, GtkRtgCanvas *canvas);
static void	gtk_rtg_canvas_camera_view_distance	 (GtkRtgCamera *camera, GParamSpec *pspec, GtkRtgCanvas *canvas);
static void	gtk_rtg_canvas_camera_view_direction (GtkRtgCamera *camera, GParamSpec *pspec, GtkRtgCanvas *canvas);
static void gtk_rtg_canvas_grid_changed      	 (GtkRtgCanvasGrid *grid, GParamSpec *pspec, GtkRtgCanvas *canvas);
static void	gtk_rtg_canvas_scene_camera_default	 (GObject *object, GtkRtgCamera *camera, GtkRtgCanvas *_canvas, gpointer data);
static void	gtk_rtg_canvas_scene_configured		 (GtkRtgScene  *scene, gpointer data);
static void	gtk_rtg_canvas_scene_prim_sel_changed(GtkRtgScene  *scene, GtkRtgCanvas *canvas);
static void	gtk_rtg_canvas_render_offscreen      (GtkRtgCanvas *canvas);


/* callbacks */
static void     gtk_rtg_canvas_realize			 (GtkWidget *widget);
static void     gtk_rtg_canvas_map			 	 (GtkWidget *widget);
static void     gtk_rtg_canvas_unmap			 (GtkWidget *widget);
static gboolean gtk_rtg_canvas_configure		 (GtkWidget *widget, GdkEventConfigure	*event);
static gboolean gtk_rtg_canvas_visibility_notify (GtkWidget *widget, GdkEventVisibility	*event, gpointer data);
static void     gtk_rtg_canvas_destroy			 (GtkWidget *object);
static gboolean gtk_rtg_canvas_key_press		 (GtkWidget *widget, GdkEventKey		*event);
static void     gtk_rtg_canvas_set_property		 (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void     gtk_rtg_canvas_get_property		 (GObject *object, guint prop_id, GValue       *value, GParamSpec *pspec);

/* privs */
static gboolean _gtk_rtg_canvas_timeout			 (GtkWidget *widget);
static void	_gtk_rtg_canvas_timeout_add		 	 (GtkRtgCanvas *canvas);
static void	_gtk_rtg_canvas_timeout_remove		 (GtkRtgCanvas *canvas);
static gchar   *_gtk_rtg_canvas_create_object_id (GtkRtgCanvas *canvas);
static void     _gtk_rtg_canvas_release_object_id(GtkRtgCanvas *canvas, gchar **canvas_obj_id)    throw (GtkRtgException);
static void	_gtk_rtg_canvas_set_cursor		 	 (GtkRtgCanvas *canvas, GtkRtgCanvasCursor curs_id);
static void	_gtk_rtg_canvas_camera_altaz_update_mxmy (GtkRtgCanvas *canvas, gdouble mx, gdouble my);
static void	_gtk_rtg_canvas_camera_altaz_init	 (GtkRtgCanvas *canvas);
static void	_gtk_rtg_canvas_camera_altaz_rotate	 (GtkRtgCanvas *canvas, gdouble mx, gdouble my);
static void	_gtk_rtg_canvas_camera_altaz_zoom	 (GtkRtgCanvas *canvas, gdouble my);
static void	_gtk_rtg_canvas_camera_altaz_pan	 (GtkRtgCanvas *canvas, gdouble mx, gdouble my);
static void	_gtk_rtg_canvas_camera_altaz_gizmo_init  (GtkRtgCanvas *canvas);
static void	_gtk_rtg_canvas_camera_altaz_gizmo_update(GtkRtgCanvas *canvas);
static gboolean _gtk_rtg_canvas_camera_altaz_gizmo_r_mice(GtkRtgCanvas *canvas, point2f *point, GdkEventMotion *event);
static void     _gtk_rtg_canvas_camera_set_safe_view_up  (GtkRtgCanvas *canvas, GObject *camera);
static void     _gtk_rtg_canvas_camera_view_push_text    (GtkRtgCanvas *canvas);
void     _gtk_rtg_canvas_text_push_real           (GtkRtgCanvas *canvas, GtkRtgText *text, m_txtType *m_text);


static void	_gtk_rtg_canvas_render_text		 		 (GtkRtgCanvas *canvas);
static void	_gtk_rtg_canvas_camera_altaz_render_gizmo(GtkRtgCanvas *canvas);
static void	_gtk_rtg_canvas_render_border		 	 (GtkRtgCanvas *canvas, GdkColor &color);
static void	_gtk_rtg_canvas_render_marquee_selection (GtkRtgCanvas *canvas);

static void     _gtk_rtg_canvas_realloc_picking_buffer   (GtkRtgCanvas *canvas, gint width, gint height);
static void     _gtk_rtg_canvas_primitives_get_pmin_pmax (GtkRtgCanvas *canvas,  const set<GtkRtgPrimitive *> *prims, point3f &pmin, point3f &pmax);

static void	_gtk_rtg_gizmo_calc_offset (GtkRtgGizmo *gizmo, GtkRtgCanvas *canvas);

/* externals */
extern void     gtk_rtg_canvas_grid_fade_line_color (GtkRtgCanvasGrid *grid, gfloat distance);
extern point3f  object2world				 		(GtkRtgPrimitive *prim, point3f &point);
extern point3f	world2object				 		(GtkRtgPrimitive *prim, point3f &point);
extern void	calc_bounding_sphere			 		(GtkRtgPrimitive *prim);
extern void     _gtk_rtg_canvas_grid_process_motion (GtkRtgCanvasGrid *grid, GtkRtgCanvas *canvas, gfloat x, gfloat y);

static void
gtk_rtg_canvas_class_init(GtkRtgCanvasClass *klass)
{
    GtkWidgetClass  *widget_class = GTK_WIDGET_CLASS(klass);
    GObjectClass	*o_class      = G_OBJECT_CLASS(klass);

    widget_class->configure_event	   = gtk_rtg_canvas_configure;
    widget_class->realize			   = gtk_rtg_canvas_realize;
    widget_class->map				   = gtk_rtg_canvas_map;
    widget_class->unmap				   = gtk_rtg_canvas_unmap;
    widget_class->key_press_event	   = gtk_rtg_canvas_key_press;
    widget_class->button_press_event   = gtk_rtg_canvas_button_press;
    widget_class->button_release_event = gtk_rtg_canvas_button_release;
    widget_class->motion_notify_event  = gtk_rtg_canvas_motion_notify;
    widget_class->scroll_event         = gtk_rtg_canvas_scroll_event;
    widget_class->destroy 			   = gtk_rtg_canvas_destroy;

    o_class->set_property = gtk_rtg_canvas_set_property;
    o_class->get_property = gtk_rtg_canvas_get_property;

    klass->render_offscreen = gtk_rtg_canvas_render_offscreen;

    /*
       canvas (view) properties:

       view-interactive 	(true/false)
       view-predefined		GtkRtgCanvasViewPredefined
       view-gizmo			(true/false)
       view-up				GtkRtgCanvasViewUp


       marquee-color			GdkColor
       view-background-color	GdkColor
       view-gizmo-color			GdkColor
       view-focused-color		GdkColor
       view-focused-line-width	gfloat
     */

    /*
     * GtkRtgCanvas: bg-color
     *
     * Background color.
     *
     *
     */
    g_object_class_install_property(o_class,
                                    PROP_BG_COLOR,
                                    g_param_spec_pointer("bg-color",
                                                         "Bg-Color",
                                                         "Background color",
                                                         GTK_RTG_PARAM_READWRITE));
    /*
     * Signal(s)
     *
     *
     */

    /*
     * GtkRtgCanvas: focused
     *
     * Canvas emits this signal when it gets focused (i.e. mouse click or explicity set).
     *
     *
     */
    signals[FOCUSED] = g_signal_new("focused",
                                    G_OBJECT_CLASS_TYPE(klass),
                                    (GSignalFlags)G_SIGNAL_RUN_FIRST,
                                    G_STRUCT_OFFSET(GtkRtgCanvasClass, focused),
                                    NULL,
                                    NULL,
                                    g_cclosure_marshal_VOID__VOID,
                                    G_TYPE_NONE,
                                    0);
    /*
     * GtkRtgCanvas: render-offscreen
     *
     * The canvas emits this signal after the scene is rendered for the selection (offscreen buffer).
     *
     *
     */
    signals[RENDER_OFFSCREEN] = g_signal_new("render-offscreen",
                                             G_OBJECT_CLASS_TYPE(klass),
                                             (GSignalFlags)G_SIGNAL_RUN_FIRST,
                                             G_STRUCT_OFFSET(GtkRtgCanvasClass, render_offscreen),
                                             NULL,
                                             NULL,
                                             g_cclosure_marshal_VOID__VOID,
                                             G_TYPE_NONE,
                                             0);
    /*
     * GtkRtgCanvas: gizmo-position
     *
     * The canvas emits this signal after the gizmo position is changed.
     *
     *
     */
    signals[GIZMO_POSITION] = g_signal_new("gizmo-position",
                                           G_OBJECT_CLASS_TYPE(klass),
                                           (GSignalFlags)G_SIGNAL_RUN_FIRST,
                                           G_STRUCT_OFFSET(GtkRtgCanvasClass, gizmo_position),
                                           NULL,
                                           NULL,
                                           g_cclosure_marshal_VOID__POINTER,
                                           G_TYPE_NONE,
                                           1,
                                           G_TYPE_POINTER);

    g_type_class_add_private(G_OBJECT_CLASS(klass), sizeof(GtkRtgCanvasPrivate));
}


GtkWidget *
gtk_rtg_canvas_new() throw (GtkRtgException)
{
    GtkWidget *widget = GTK_WIDGET(g_object_new(GTK_RTG_TYPE_CANVAS, NULL));


    return widget;
}


static void
gtk_rtg_canvas_init(GtkRtgCanvas *canvas)
{
    canvas->priv = GTK_RTG_CANVAS_GET_PRIVATE(canvas);
    gtk_widget_set_can_focus(GTK_WIDGET(canvas), TRUE);

    /* build canvas id: {instance ptr, :} */
    canvas->priv->id = g_strdup_printf("%p%c", (void *)canvas, ':');

    /*
     * Signals
     *
     * - Note: Connected signals have higher priority over overridden call-backs, so invoking signal handlers in this way
     *   guarantees that outside (user) connection will not be called first!
     *
     * - 'realize' handler is reserved by GtkGLExt library, so it should be passed first to the library (using callback).
     *
     **/

    /* chain 'unrealize' signal before 'gtk_widget_set_gl_capability' (it will destroy GdkGLContext and GdkGLDrawable) */
    g_signal_connect(G_OBJECT(canvas), "unrealize",				  G_CALLBACK(gtk_rtg_canvas_unrealize),			NULL);
    g_signal_connect(G_OBJECT(canvas), "draw",			  G_CALLBACK(gtk_rtg_canvas_expose),			NULL);
    g_signal_connect(G_OBJECT(canvas), "visibility-notify-event", G_CALLBACK(gtk_rtg_canvas_visibility_notify), NULL);

    /* force heap allocation to invoke constuctors */
    canvas->priv->m_text         = new m_txtType;
    canvas->priv->view.altaz.dir = new vector3d();
    canvas->priv->view.altaz.up  = new vector3d(0.0, 1.0, 0.0);
    canvas->priv->view.up        = GTK_RTG_CANVAS_VIEW_UP_Y;

    /* objects id tracker */
    canvas->priv->v_obj_id = new vector<guint>;

    /* cursor */
    canvas->priv->curs_id = CURS_INVALID;

    /* init 'gizmo' */
    _gtk_rtg_canvas_camera_altaz_gizmo_init(canvas);

    /* colors */
    gdk_color_parse("gold",	   &canvas->priv->c_focused);
    gdk_color_parse("#363f4b", &canvas->priv->c_border);
    gdk_color_parse("cyan",	   &canvas->priv->marquee.color);

    /* set default canvas interactive mode to off */
    canvas->priv->mode = GTK_RTG_CANVAS_INT_MODE_OFF;

    /* enable tripod */
    canvas->priv->display |= DISPLAY_TRIPOD;

    /* set default views angle / projection */
    canvas->priv->view.predefined = GTK_RTG_CANVAS_VIEW_CUSTOM;

    /* scene render mode */
    canvas->priv->render_mode = GTK_RTG_RENDER_SCENE_SOLID;

    /* default bg color */
    gdk_color_parse(BG_COLOR_DEFAULT, &canvas->priv->c_background);

    /* picking buffer */
    g_object_set_data(G_OBJECT(canvas), "pickbuff", &canvas->priv->view.pickbuff);
}

gboolean
gtk_rtg_canvas_make_current(GtkRtgCanvas *canvas)
{
    g_return_val_if_fail(GTK_RTG_IS_CANVAS(canvas), FALSE);
    return gtk_rtg_gl_backend_make_current(canvas->priv->backend);
}


static void
gtk_rtg_canvas_destroy(GtkWidget *object)
{
    GtkRtgCanvas	*canvas = GTK_RTG_CANVAS(object);

    /*
     * Turn off if animated.
     *
     */
    _gtk_rtg_canvas_timeout_remove(canvas);

    /*
     * Disconnect signal  handlers
     *
     */
    g_signal_handlers_disconnect_by_func(canvas, (void *)gtk_rtg_canvas_unrealize,	NULL);
    g_signal_handlers_disconnect_by_func(canvas, (void *)gtk_rtg_canvas_expose,		NULL);

    if (canvas->priv->wheel_timeout_id) {
        g_source_remove(canvas->priv->wheel_timeout_id);
        canvas->priv->wheel_timeout_id = 0;
        canvas->priv->wheel = 0;
    }
    if (canvas->priv->backend) {
        gtk_rtg_gl_backend_make_current(canvas->priv->backend);
        if (canvas->priv->destroy) {
            canvas->priv->destroy(canvas, canvas->priv->data[GTK_RTG_DESTROY]);
            canvas->priv->destroy = NULL;
        }
        gtk_rtg_resource_fonts_release(canvas);
    }

    /* clear buffer */
    if(canvas->priv->view.cam)
        gtk_rtg_canvas_scene_camera_default(NULL, NULL, NULL, canvas);

    /*
     * Free text render map.
     * - Pop-out if any un-poped text.
     */
    if(canvas->priv->m_text)
    {
        for(m_txtType::const_iterator it = canvas->priv->m_text->begin(); it != canvas->priv->m_text->end(); ++it)
        {
            /* pop-out */
            gchar *canvas_obj_id = (gchar*) g_object_get_data(G_OBJECT(it->second), canvas->priv->id);
            if(canvas_obj_id)
            {
                g_free(canvas_obj_id); canvas_obj_id = NULL;
                g_object_set_data(G_OBJECT(it->second), canvas->priv->id, canvas_obj_id);
            }

            g_object_unref(it->second);
        }

        delete canvas->priv->m_text; canvas->priv->m_text = NULL;
    }

    /*
     * Free id vector
     *
     */
    if(canvas->priv->v_obj_id){ delete canvas->priv->v_obj_id; canvas->priv->v_obj_id = NULL; }


    /*
     * AltAz dir-up vectors.
     *
     */
    if(canvas->priv->view.altaz.dir){ delete canvas->priv->view.altaz.dir; canvas->priv->view.altaz.dir = NULL; }
    if(canvas->priv->view.altaz.up) { delete canvas->priv->view.altaz.up;  canvas->priv->view.altaz.up  = NULL; }


    /*
     * Grid.
     *
     */
    if(canvas->priv->grid)
    {
        /* pop-out */
        gchar *canvas_obj_id = (gchar*) g_object_get_data(G_OBJECT(canvas->priv->grid), canvas->priv->id);
        if(canvas_obj_id)
        {
            g_free(canvas_obj_id); canvas_obj_id = NULL;
            g_object_set_data(G_OBJECT(canvas->priv->grid), canvas->priv->id, canvas_obj_id);
        }

        g_signal_handlers_disconnect_by_func(canvas->priv->grid, (void *)gtk_rtg_canvas_grid_changed, canvas);
        g_object_unref(canvas->priv->grid);
        canvas->priv->grid = NULL;
    }

    /*
     * Scene.
     *
     */
    if(canvas->priv->scene)
    {
        g_signal_handlers_disconnect_by_func(canvas->priv->scene, (void *)gtk_rtg_canvas_scene_configured, canvas);
        g_signal_handlers_disconnect_by_func(canvas->priv->scene, (void *)gtk_rtg_canvas_scene_camera_default, canvas);
        g_signal_handlers_disconnect_by_func(canvas->priv->scene, (void *)gtk_rtg_canvas_scene_prim_sel_changed, canvas);

        /* pop-out */
        gchar *canvas_obj_id = (gchar*) g_object_get_data(G_OBJECT(canvas->priv->scene), canvas->priv->id);
        if(canvas_obj_id)
        {
            g_free(canvas_obj_id); canvas_obj_id = NULL;
            g_object_set_data(G_OBJECT(canvas->priv->scene), canvas->priv->id, canvas_obj_id);
        }

        g_object_unref(canvas->priv->scene); canvas->priv->scene = NULL;
    }

    /*
     * Views 'prefefined' text.
     *
     */
    if(canvas->priv->view.text)
    {
        g_object_unref(canvas->priv->view.text);
        canvas->priv->view.text = NULL;
    }


    /*
     * Canvas id.
     *
     * Note: This should be freed last, as it's used in upper code.
     *
     */
    if(canvas->priv->id){ g_free(canvas->priv->id ); canvas->priv->id = NULL; };


    /*
     * Picking.
     *
     */
    if(canvas->priv->view.pickbuff.buff)
    {
        g_free(canvas->priv->view.pickbuff.buff);
        canvas->priv->view.pickbuff.buff = NULL;
    }

    /* picking buffer */
    g_object_set_data(G_OBJECT(canvas), "pickbuff", NULL);

    if(canvas->priv->view.tripod)
    {
        g_object_unref(canvas->priv->view.tripod);
        canvas->priv->view.tripod = NULL;
    }

    if(canvas->priv->view.gizmo_move)
    {
        g_object_unref(canvas->priv->view.gizmo_move);
        canvas->priv->view.gizmo_move = NULL;
    }


    if(canvas->priv->view.gizmo_scale)
    {
        g_object_unref(canvas->priv->view.gizmo_scale);
        canvas->priv->view.gizmo_scale = NULL;
    }


    if(canvas->priv->view.gizmo_rotate)
    {
        g_object_unref(canvas->priv->view.gizmo_rotate);
        canvas->priv->view.gizmo_rotate = NULL;
    }


    gtk_rtg_gl_backend_free(canvas->priv->backend);
    canvas->priv->backend = NULL;
    canvas->priv->is_configured = FALSE;
    GTK_WIDGET_CLASS(gtk_rtg_canvas_parent_class)->destroy(object);
}





/***
 ***
 *** Callbacks.
 ***
 ***
 ***
 ***
 ***
 ***/

/***
 *** The "configure_event" signal handler. Any processing required when
 *** the OpenGL-capable drawing area is re-configured should be done here.
 *** Almost always it will be used to resize the OpenGL viewport when
 *** the window is resized.
 ***/
static gboolean
gtk_rtg_canvas_configure(GtkWidget *widget, GdkEventConfigure	*event)
{
    GtkRtgCanvas	*canvas = GTK_RTG_CANVAS(widget);

    if (!canvas->priv->backend) return FALSE;
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    GtkAllocation *alloc = &allocation;
    alloc->width = MAX(alloc->width, 1);
    alloc->height = MAX(alloc->height, 1);
    if (canvas->priv->buffer_width == alloc->width && canvas->priv->buffer_height == alloc->height)
        return FALSE;
    if (!gtk_rtg_gl_backend_resize(canvas->priv->backend, alloc->width, alloc->height))
        return FALSE;
    canvas->priv->buffer_width = alloc->width;
    canvas->priv->buffer_height = alloc->height;
    canvas->priv->is_configured = TRUE;

    /* set viewport */
    glViewport(0, 0, alloc->width, alloc->height);

    /* picking buffer */
    canvas->priv->view.pickbuff.update = 1;
    _gtk_rtg_canvas_realloc_picking_buffer(canvas, alloc->width, alloc->height);

    /* User provided callback */
    if(canvas->priv->resize)
        canvas->priv->resize(alloc->width, alloc->height, canvas->priv->data[GTK_RTG_RESIZE]);

    /* update camera */
    if(canvas->priv->is_view)
    {
        /* update gizmo */
        _gtk_rtg_canvas_camera_altaz_gizmo_update(canvas);

        /* update aspect ratio for the cameras projection */
        g_object_set(canvas->priv->view.cam, "proj-aspect", (gfloat)alloc->width/(gfloat)alloc->height, NULL);

        /* rebuild projection matrix */
        gtk_rtg_camera_projection_do_matrix(canvas->priv->view.cam);
    }

    // Context stays current for the original camera and gizmo callbacks.
    /*** OpenGL END ***/


    return FALSE;
}


/***
 *** The "realize" signal handler. All the OpenGL initialization
 *** should be performed here, such as default background colour,
 *** certain states etc.
 ***/
static void
gtk_rtg_canvas_realize(GtkWidget *widget)
{
    GtkRtgCanvas	*canvas = GTK_RTG_CANVAS(widget);

    (* GTK_WIDGET_CLASS(gtk_rtg_canvas_parent_class)->realize)(widget);

    GError *error = NULL;
    canvas->priv->backend = gtk_rtg_gl_backend_new(widget, &error);
    if (!canvas->priv->backend) {
        g_warning("3dwhiz viewport: %s", error ? error->message : "OpenGL unavailable");
        g_clear_error(&error);
        return;
    }
    gtk_rtg_canvas_configure(widget, NULL);


    /* add event masks */
    gtk_widget_add_events(widget, GDK_POINTER_MOTION_MASK |
                                  GDK_BUTTON1_MOTION_MASK |
                                  GDK_BUTTON2_MOTION_MASK |
                                  GDK_BUTTON3_MOTION_MASK |
                                  GDK_BUTTON_PRESS_MASK	|
                                  GDK_BUTTON_RELEASE_MASK |
                                  GDK_KEY_PRESS_MASK	|
                                  GDK_KEY_RELEASE_MASK	|
                                  GDK_VISIBILITY_NOTIFY_MASK | GDK_SCROLL_MASK | GDK_SMOOTH_SCROLL_MASK);

    /*
     * Views 'predefined' and axes text
     */

    if(!canvas->priv->view.text)
        _gtk_rtg_canvas_camera_view_push_text(canvas);

    if(!canvas->priv->view.tripod)
    {
        gchar *font_desc = g_strdup_printf("%s 11", GTK_RTG_CANVAS_VIEW_PREDEFINED_FONT);
        canvas->priv->view.tripod = gtk_rtg_tripod_new(canvas, font_desc);
        g_free(font_desc);

        font_desc = g_strdup_printf("%s 11", GTK_RTG_CANVAS_VIEW_PREDEFINED_FONT);
        canvas->priv->view.gizmo_move = GTK_RTG_TRIPOD(gtk_rtg_move_gizmo_new(canvas, font_desc));
        canvas->priv->view.gizmo_scale = GTK_RTG_TRIPOD(gtk_rtg_scale_gizmo_new(canvas, font_desc));
        g_free(font_desc);

        font_desc = g_strdup_printf("%s 11", GTK_RTG_CANVAS_VIEW_PREDEFINED_FONT);
        canvas->priv->view.gizmo_rotate = GTK_RTG_TRIPOD(gtk_rtg_rotate_gizmo_new(canvas, font_desc));
        g_free(font_desc);
    }

    /* A recreated context needs new display lists for retained text objects. */
    for(const auto &entry : *canvas->priv->m_text)
        _gtk_rtg_canvas_text_push_real(canvas, entry.second, NULL);
    for(auto *tripod : {canvas->priv->view.tripod, canvas->priv->view.gizmo_move,
                       canvas->priv->view.gizmo_scale, canvas->priv->view.gizmo_rotate})
        for(guint axis = 0; axis < AXES; ++axis)
            _gtk_rtg_canvas_text_push_real(canvas, tripod->ax[axis].text, NULL);

    /* OpenGL BEGIN */
    if(!gtk_rtg_gl_backend_make_current(canvas->priv->backend))
        return;

    /* bg color */
    glClearColor((gfloat)canvas->priv->c_background.red / 65535.f, (gfloat)canvas->priv->c_background.green / 65535.f, (gfloat)canvas->priv->c_background.blue / 65535.f, 1.f);

    /* User provided callback */
    if(canvas->priv->init)
        canvas->priv->init(canvas, canvas->priv->data[GTK_RTG_INIT]);

    // Context stays current for the original camera and gizmo callbacks.
    /* OpenGL END */

    /* set cursor */
    _gtk_rtg_canvas_set_cursor(canvas, CURS_NORMAL);
}


/***
 *** The "map_event" signal handler. Any processing required when the
 *** OpenGL-capable drawing area is mapped should be done here.
 ***/
static void
gtk_rtg_canvas_map(GtkWidget *widget)
{
    GtkRtgCanvas	*canvas = GTK_RTG_CANVAS(widget);

    (* GTK_WIDGET_CLASS(gtk_rtg_canvas_parent_class)->map)(widget);

    if(canvas->priv->is_animated)
        _gtk_rtg_canvas_timeout_add(canvas);
}


/***
 *** The "unmap_event" signal handler. Any processing required when the
 *** OpenGL-capable drawing area is unmapped should be done here.
 ***/
static void
gtk_rtg_canvas_unmap(GtkWidget *widget)
{
    (* GTK_WIDGET_CLASS(gtk_rtg_canvas_parent_class)->unmap)(widget);

    _gtk_rtg_canvas_timeout_remove(GTK_RTG_CANVAS(widget));
}


/***
 *** The "visibility_notify_event" signal handler. Any processing required
 *** when the OpenGL-capable drawing area is visually obscured should be
 *** done here.
 ***/
static gboolean
gtk_rtg_canvas_visibility_notify(GtkWidget *widget, GdkEventVisibility	*event, gpointer data)
{
    if(event->state == GDK_VISIBILITY_FULLY_OBSCURED)
        _gtk_rtg_canvas_timeout_add(GTK_RTG_CANVAS(widget));
    else
        _gtk_rtg_canvas_timeout_remove(GTK_RTG_CANVAS(widget));

    return FALSE;
}


static gboolean
gtk_rtg_canvas_key_press(GtkWidget *widget, GdkEventKey		*event)
{
    GtkRtgCanvas	*canvas = GTK_RTG_CANVAS(widget);

    switch(event->keyval)
    {
        /* edit mesh */
        case GDK_KEY_0:	gtk_rtg_scene_select_backface(canvas->priv->scene, !gtk_rtg_scene_is_backface_selection(canvas->priv->scene));
                        break;

        case GDK_KEY_1:	gtk_rtg_scene_toggle_edit_mode(canvas->priv->scene, GTK_RTG_EDIT_POINTS);
                        gtk_rtg_canvas_render(canvas);
                        break;

        case GDK_KEY_2:	gtk_rtg_scene_toggle_edit_mode(canvas->priv->scene, GTK_RTG_EDIT_EDGES);
                        gtk_rtg_canvas_render(canvas);
                        break;

        case GDK_KEY_3:	gtk_rtg_scene_toggle_edit_mode(canvas->priv->scene, GTK_RTG_EDIT_POLYS);
                        gtk_rtg_canvas_render(canvas);
                        break;

                        /* predefined views */
        case GDK_KEY_p:
        case GDK_KEY_P:
                        gtk_rtg_camera_set_projection(canvas->priv->view.cam, (GtkRtgCamProj)!PROJECTION(canvas->priv->view.cam));
                        gtk_rtg_camera_projection_do_matrix(canvas->priv->view.cam);
                        gtk_rtg_canvas_render(canvas);
                        break;

        case GDK_KEY_f:
        case GDK_KEY_F:	gtk_rtg_canvas_view_set_predefined(canvas, GTK_RTG_CANVAS_VIEW_FRONT);  break;

        case GDK_KEY_b:
        case GDK_KEY_B:	gtk_rtg_canvas_view_set_predefined(canvas, GTK_RTG_CANVAS_VIEW_BACK);   break;

        case GDK_KEY_l:
        case GDK_KEY_L:	gtk_rtg_canvas_view_set_predefined(canvas, GTK_RTG_CANVAS_VIEW_LEFT);   break;

        case GDK_KEY_r:
        case GDK_KEY_R:	gtk_rtg_canvas_view_set_predefined(canvas, GTK_RTG_CANVAS_VIEW_RIGHT);  break;

        case GDK_KEY_t:
        case GDK_KEY_T:	gtk_rtg_canvas_view_set_predefined(canvas, GTK_RTG_CANVAS_VIEW_TOP);    break;

        case GDK_KEY_m:
        case GDK_KEY_M:	gtk_rtg_canvas_view_set_predefined(canvas, GTK_RTG_CANVAS_VIEW_BOTTOM); break;

                        /* user view */
        case GDK_KEY_c:
        case GDK_KEY_C:	gtk_rtg_canvas_view_set_predefined(canvas, GTK_RTG_CANVAS_VIEW_CUSTOM); break;


                        /* view mode */
        case GDK_KEY_v:
        case GDK_KEY_V:	IS_MODE_VIEW(canvas) ? gtk_rtg_canvas_interactive_set_mode(canvas, GTK_RTG_CANVAS_INT_MODE_OFF)
                                              : gtk_rtg_canvas_interactive_set_mode(canvas, GTK_RTG_CANVAS_INT_MODE_VIEW);

                        gtk_rtg_canvas_render(canvas); break;

                        /* object mode */
        case GDK_KEY_o:
        case GDK_KEY_O:	IS_MODE_OBJECT(canvas) ? gtk_rtg_canvas_interactive_set_mode(canvas, GTK_RTG_CANVAS_INT_MODE_OFF)
                                                  : gtk_rtg_canvas_interactive_set_mode(canvas, GTK_RTG_CANVAS_INT_MODE_SELECT);

                        gtk_rtg_canvas_render(canvas); break;

                        /* grid */
        case GDK_KEY_g:
        case GDK_KEY_G:	if(canvas->priv->display & DISPLAY_GRID)
                            canvas->priv->display &= ~DISPLAY_GRID;
                        else
                            canvas->priv->display |=  DISPLAY_GRID;

                        gtk_rtg_canvas_render(canvas); break;

                        /* views gizmo */
        case GDK_KEY_z:
        case GDK_KEY_Z:	gtk_rtg_canvas_view_gizmo_show(canvas, !(canvas->priv->display & DISPLAY_VIEW_GIZMO));
                        gtk_rtg_canvas_render(canvas); break;


                        /* toggle viewframe */
        case GDK_KEY_F3:
                        canvas->priv->viewframe = !canvas->priv->viewframe;

                        canvas->priv->render_mode = canvas->priv->viewframe ? GTK_RTG_RENDER_SCENE_EDGES
                                                                            : canvas->priv->solid_edge ? GTK_RTG_RENDER_SCENE_SOLID_EDGES
                                                                                                       : GTK_RTG_RENDER_SCENE_SOLID;
                        gtk_rtg_canvas_render(canvas); break;

                        /* toggle solid edges  */
        case GDK_KEY_F4:
                        if(!canvas->priv->viewframe)
                        {
                            canvas->priv->solid_edge = !canvas->priv->solid_edge;

                            canvas->priv->render_mode = canvas->priv->solid_edge ? GTK_RTG_RENDER_SCENE_SOLID_EDGES
                                                                                 : GTK_RTG_RENDER_SCENE_SOLID;
                            gtk_rtg_canvas_render(canvas);
                        }

                        break;

        case GDK_KEY_Escape:

                        {
                            GdkEvent *event_quit;
                            event_quit                 = gdk_event_new (GDK_DELETE);
                            event_quit->any.window     = GDK_WINDOW(g_object_ref(gtk_widget_get_parent_window(widget)));
                            event_quit->any.send_event = TRUE;
                            gtk_main_do_event(event_quit);
                            gdk_event_free(event_quit);

                            break;
                        }
    }

    return FALSE;
}


static void
gtk_rtg_canvas_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GtkRtgCanvas *canvas = GTK_RTG_CANVAS(object);

    switch(prop_id)
    {
        case PROP_BG_COLOR:	canvas->priv->c_background = (*(GdkColor*)(g_value_get_pointer(value))); break;
    };
}


static void
gtk_rtg_canvas_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GtkRtgCanvas *canvas = GTK_RTG_CANVAS(object);

    switch(prop_id)
    {						/* getters without extra function calls */
        case PROP_BG_COLOR:	g_value_set_pointer(value, &canvas->priv->c_background); break;
    };
}


/***
 ***
 *** Signals.
 ***
 ***
 ***
 ***/

static void
gtk_rtg_canvas_unrealize(GtkWidget *widget, gpointer data)
{
    GtkRtgCanvas *canvas = GTK_RTG_CANVAS(widget);
    _gtk_rtg_canvas_timeout_remove(canvas);
    if(canvas->priv->wheel_timeout_id)
    {
        g_source_remove(canvas->priv->wheel_timeout_id);
        canvas->priv->wheel_timeout_id = 0;
        canvas->priv->wheel = 0;
    }
    if(canvas->priv->backend)
    {
        gtk_rtg_gl_backend_make_current(canvas->priv->backend);
        if(canvas->priv->destroy)
            canvas->priv->destroy(canvas, canvas->priv->data[GTK_RTG_DESTROY]);
        gtk_rtg_resource_fonts_release(canvas);
        gtk_rtg_gl_backend_free(canvas->priv->backend);
        canvas->priv->backend = NULL;
    }
    canvas->priv->is_configured = FALSE;
    canvas->priv->buffer_width = canvas->priv->buffer_height = 0;
}


static gboolean
wheel_zoom_anim(GtkRtgCanvas *canvas)
{
    if(canvas->priv->view.cam && canvas->priv->wheel > 0 && --canvas->priv->wheel > 0)
    {
        gdouble distance;
        g_object_get(canvas->priv->view.cam, "view-distance", &distance, NULL);

        /* get new distance */
        distance = distance * canvas->priv->zmultiplier;

        /* will check for boundaries */
        gtk_rtg_camera_view_set_distance(canvas->priv->view.cam, distance);

        /* recreate view matrix */
        gtk_rtg_camera_view_do_matrix(canvas->priv->view.cam);

        return TRUE;
    }

    canvas->priv->wheel_timeout_id = 0;
    canvas->priv->wheel = 0;
    return FALSE;
}

/*
 * Zoom on mouse wheel
 */
gboolean
gtk_rtg_canvas_scroll_event(GtkWidget *widget, GdkEventScroll     *event)
{
    GtkRtgCanvas	*canvas = GTK_RTG_CANVAS(widget);

    if(IS_MODE_VIEW(canvas))
    {
        switch(event->direction)
        {
            case GDK_SCROLL_UP:     canvas->priv->zmultiplier = 0.96;
                                    PROCESS_WHEEL_ZOOM(canvas);
                                    break;

            case GDK_SCROLL_DOWN:   canvas->priv->zmultiplier = 1.04;
                                    PROCESS_WHEEL_ZOOM(canvas);
                                    break;
            case GDK_SCROLL_SMOOTH: {
                gdouble dx, dy;
                if (gdk_event_get_scroll_deltas(reinterpret_cast<GdkEvent*>(event), &dx, &dy) && dy != 0) {
                    canvas->priv->zmultiplier = pow(1.04, dy);
                    PROCESS_WHEEL_ZOOM(canvas);
                }
                break;
            }
            default: break;
        };
    }

    return TRUE;
}


static gboolean
gtk_rtg_canvas_button_press(GtkWidget *widget, GdkEventButton	*event)
{
    GtkRtgCanvas *canvas = GTK_RTG_CANVAS(widget);

    /* focus if not focused. */
    gtk_rtg_canvas_set_focused(canvas, TRUE);

    if(IS_MODE_VIEW(canvas))
    {
        if(event->type == GDK_BUTTON_PRESS)
        {
            canvas->priv->view.altaz.depth = event->y;
            _gtk_rtg_canvas_camera_altaz_update_mxmy(canvas, event->x, event->y);
        }
    }else
    if(IS_MODE_OBJECT(canvas) && (canvas->priv->display & DISPLAY_SCENE))
    {
        if(SELECTED(canvas->priv->scene))
        {
            /*
             * Test gizmo's
             */

            /* clicking on move gizmo? */
            if((canvas->priv->display & DISPLAY_MOVE_GIZMO) && canvas->priv->view.gizmo_move->sel_hit)
            {
                /* update fall-back */
                canvas->priv->view.gizmo_move->sel_fallback = canvas->priv->view.gizmo_move->selected;
                canvas->priv->view.gizmo_move->fallback = 0;
                canvas->priv->view.can_drag = 1;
                return FALSE;
            }else
            if((canvas->priv->display & DISPLAY_SCALE_GIZMO) && canvas->priv->view.gizmo_scale->sel_hit)
            {
                /* update fall-back */
                canvas->priv->view.gizmo_scale->sel_fallback = canvas->priv->view.gizmo_scale->selected;
                canvas->priv->view.gizmo_scale->fallback = 0;
                canvas->priv->view.can_drag = 1;
                return FALSE;
            }else
            if((canvas->priv->display & DISPLAY_ROTATE_GIZMO) && canvas->priv->view.gizmo_rotate->sel_hit)
            {
                /* update fall-back */
                canvas->priv->view.gizmo_rotate->sel_fallback = canvas->priv->view.gizmo_rotate->selected;
                canvas->priv->view.gizmo_rotate->fallback = 0;
                canvas->priv->view.can_drag = 1;
                return FALSE;
            }
        }

        GdkPoint point = {(gint)event->x, gtk_widget_get_allocated_height(widget) - (gint)event->y};

        /* set start point (marquee) */
        canvas->priv->marquee.start.x = point.x;
        canvas->priv->marquee.start.y = point.y;

        if(canvas->priv->view.pickbuff.update)
            SWITCH_CONTEXT(canvas);

        /* perform the selection test */
        gint id = gtk_rtg_scene_point_select(canvas->priv->scene, point, canvas, TRUE);

        if(id != -1) /* there is a primitive/vertex/edge/poly under a mouse? */
        {
            GtkRtgPrimitive *prim = gtk_rtg_scene_get_primitive(canvas->priv->scene, id);

            if(prim && (canvas->priv->display & DISPLAY_ROTATE_GIZMO))
            {
                /* An object-body drag starts a free rotation, even after using a ring. */
                GtkRtgTripod *tripod = canvas->priv->view.gizmo_rotate;
                tripod->fallback = TRUE;
                tripod->selected = tripod->sel_fallback = SELECTED_CIRCLE;
            }

            if(prim && !IS_SELECTED(prim)) /* if primitive is not selected */
            {
                /* perform actual select */
                gtk_rtg_scene_point_select(canvas->priv->scene, point, canvas);

                /* update cursor */
                if(canvas->priv->display & DISPLAY_MOVE_GIZMO)
                    _gtk_rtg_canvas_set_cursor(canvas, CURS_MOVE);

                gtk_rtg_canvas_render(canvas);
            }else
            {
                /* postpone the selection to the release event (expecting drag?) */
                canvas->priv->view.sel_postponed = 1;
            }

            canvas->priv->view.can_drag = 1;
        }else
        /* background is selected */
        {
            canvas->priv->view.can_drag = 0;

            /* check for selected objects, deselect if any */
            if(gtk_rtg_scene_deselect_all(canvas->priv->scene))
                /* meshes are deselected, redraw the scene */
                gtk_rtg_canvas_render(canvas);

        }
    }

    return FALSE;
}


static gboolean
gtk_rtg_canvas_motion_notify(GtkWidget *widget, GdkEventMotion	*event)
{
    GtkRtgCanvas *canvas = GTK_RTG_CANVAS(widget);

    /* get snap point */
    if(canvas->priv->grid && gtk_rtg_canvas_grid_get_snap_cursor(canvas->priv->grid))
    {
        _gtk_rtg_canvas_grid_process_motion(canvas->priv->grid, canvas, (gfloat)event->x, (gfloat)event->y);
        gtk_widget_queue_draw(widget);
    }

    if(IS_MODE_VIEW(canvas))
    {
        /* Pure motion mode (no-dragging) */
        if(    (canvas->priv->display & DISPLAY_VIEW_GIZMO)
            && (event->state & GDK_BUTTON1_MASK) == 0
            && (event->state & GDK_BUTTON2_MASK) == 0
            && (event->state & GDK_BUTTON3_MASK) == 0
          )
        {
            /*
             * Check all 4 gizmos rect's against mouse position.
             *
             */

            if(	   _gtk_rtg_canvas_camera_altaz_gizmo_r_mice(canvas, canvas->priv->view.gizmo.rect[0], event)
                || _gtk_rtg_canvas_camera_altaz_gizmo_r_mice(canvas, canvas->priv->view.gizmo.rect[2], event)
              )
            {
                _gtk_rtg_canvas_set_cursor(canvas, CURS_VIEW_AXIS_H);
                return TRUE;
            }else
            if(    _gtk_rtg_canvas_camera_altaz_gizmo_r_mice(canvas, canvas->priv->view.gizmo.rect[1], event)
                || _gtk_rtg_canvas_camera_altaz_gizmo_r_mice(canvas, canvas->priv->view.gizmo.rect[3], event)
              )
            {
                _gtk_rtg_canvas_set_cursor(canvas, CURS_VIEW_AXIS_V);
                return TRUE;
            }else
                _gtk_rtg_canvas_set_cursor(canvas, CURS_NORMAL);
        }

        /*
         * Dragging motion mode {rotate, pan, zoom}
         *
         */
        if(event->state & GDK_BUTTON1_MASK)
        {
            switch(canvas->priv->curs_id)
            {
                case CURS_NORMAL:      _gtk_rtg_canvas_set_cursor(canvas, CURS_VIEW_ROTATE);			break;
                case CURS_VIEW_AXIS_H: _gtk_rtg_canvas_camera_altaz_update_mxmy(canvas, 0.0, event->y); break;
                case CURS_VIEW_AXIS_V: _gtk_rtg_canvas_camera_altaz_update_mxmy(canvas, event->x, 0.0); break;

                default: break;
            };

            _gtk_rtg_canvas_camera_altaz_rotate(canvas, event->x, event->y);
            return TRUE;
        }else
        if(event->state & GDK_BUTTON2_MASK)
        {
            _gtk_rtg_canvas_camera_altaz_pan(canvas, event->x, event->y);
            return TRUE;
        }else
        if(event->state & GDK_BUTTON3_MASK)
        {
            _gtk_rtg_canvas_camera_altaz_zoom(canvas, event->y);
            return TRUE;
        }
    }else
    if(IS_MODE_OBJECT(canvas))
    {
        /*
         * Dragging motion mode {marquee select}
         *
         */
        if(event->state & GDK_BUTTON1_MASK)
        {
            if(canvas->priv->display & DISPLAY_MOVE_GIZMO)
            {
                SWITCH_CONTEXT(canvas);

                if(canvas->priv->view.can_drag)
                {
                    _gtk_rtg_canvas_set_cursor(canvas, CURS_MOVE);

                    GdkPoint point = {(gint)event->x, gtk_widget_get_allocated_height(widget) - (gint)event->y};
                    const vector3f &offset = gtk_rtg_tripod_process_drag(canvas->priv->view.gizmo_move, point.x, point.y);

                    g_signal_emit(canvas, signals[GIZMO_POSITION], 0, &canvas->priv->view.gizmo_move->offset);

                    const set<GtkRtgPrimitive*> *prims = gtk_rtg_scene_get_selected(canvas->priv->scene);

                    for(set<GtkRtgPrimitive*>::const_iterator it=prims->begin(); it!=prims->end(); ++it)
                        gtk_rtg_primitive_translate(*it, (vector3f&)offset);

                    /* redraw the scene */
                    gtk_rtg_canvas_render(canvas);

                    return FALSE;
                }
            }else
            if(canvas->priv->display & DISPLAY_SCALE_GIZMO)
            {
                SWITCH_CONTEXT(canvas);

                if(canvas->priv->view.can_drag)
                {
                    _gtk_rtg_canvas_set_cursor(canvas, canvas->priv->view.gizmo_scale->selected == SELECTED_XYZ ? CURS_SCALE_UNIFORM : CURS_SCALE_NONUNIFORM);

                    GdkPoint point = {(gint)event->x, gtk_widget_get_allocated_height(widget) - (gint)event->y};
                    const vector3f &offset = gtk_rtg_tripod_process_drag(canvas->priv->view.gizmo_scale, point.x, point.y);

                    const set<GtkRtgPrimitive*> *prims = gtk_rtg_scene_get_selected(canvas->priv->scene);
                    for(set<GtkRtgPrimitive*>::const_iterator it=prims->begin(); it!=prims->end(); ++it)
                        gtk_rtg_primitive_scale(*it, (vector3f&)offset);

                    /* redraw the scene */
                    gtk_rtg_canvas_render(canvas);

                    return FALSE;
                }
            }else
            if(canvas->priv->display & DISPLAY_ROTATE_GIZMO)
            {
                SWITCH_CONTEXT(canvas);

                if(canvas->priv->view.can_drag)
                {
                    GdkPoint point = {(gint)event->x, gtk_widget_get_allocated_height(widget) - (gint)event->y};
                    gtk_rtg_tripod_process_drag(canvas->priv->view.gizmo_rotate, point.x, point.y);

                    GtkRtgRotateGizmo *rg = GTK_RTG_ROTATE_GIZMO(canvas->priv->view.gizmo_rotate);

                    const set<GtkRtgPrimitive*> *prims = gtk_rtg_scene_get_selected(canvas->priv->scene);
                    for(set<GtkRtgPrimitive*>::const_iterator it=prims->begin(); it!=prims->end(); ++it)
                    {
                        /* Convert the world-space drag axis into the object's local basis. */
                        matrix4x4f basis = gtk_rtg_primitive_get_transformation_matrix(*it);
                        basis[tx] = basis[ty] = basis[tz] = 0.f;
                        matrix4x4f inverse = basis.inverse();
                        matrix4x4f delta;
                        delta.from_quaternion(rg->quat);
                        matrix4x4f local = basis * delta;
                        local = local * inverse;
                        quat_f q = local.to_quaternion();

                        gtk_rtg_primitive_rotate_from_quaternion(*it, q);
                    }

                    /* redraw the scene */
                    gtk_rtg_canvas_render(canvas);

                    return FALSE;
                }
            }

            if(!(canvas->priv->display & DISPLAY_MARQUEE))
                canvas->priv->display |= DISPLAY_MARQUEE;

            /* set end point */
            canvas->priv->marquee.end.x = (gint)event->x;
            canvas->priv->marquee.end.y = gtk_widget_get_allocated_height(widget) - (gint)event->y;

            /* redraw the scene with updated marquee */
            gtk_rtg_canvas_render(canvas);
        }else
        /*
         * Non-dragging motion mode {marquee select}
         *
         */
        {
            GdkPoint point = {(gint)event->x, gtk_widget_get_allocated_height(widget) - (gint)event->y};

            if(canvas->priv->view.pickbuff.update)
                SWITCH_CONTEXT(canvas);

            /* test gizmo's */
            if((canvas->priv->display & DISPLAY_MOVE_GIZMO) && SELECTED(canvas->priv->scene))
            {
                guint ret;

                if((ret = gtk_rtg_tripod_process_motion(canvas->priv->view.gizmo_move, point.x, point.y)))
                {
                    if(ret & GIZMO_PROCESS_CURSOR)
                        _gtk_rtg_canvas_set_cursor(canvas, CURS_MOVE);
                    else
                        _gtk_rtg_canvas_set_cursor(canvas, CURS_NORMAL);

                    if(ret & GIZMO_PROCESS_REDRAW)
                        gtk_rtg_canvas_render(canvas);

                    return FALSE;
                }
            }else
            if((canvas->priv->display & DISPLAY_SCALE_GIZMO) && SELECTED(canvas->priv->scene))
            {
                guint ret;

                if((ret = gtk_rtg_tripod_process_motion(canvas->priv->view.gizmo_scale, point.x, point.y)))
                {
                    if(ret & GIZMO_PROCESS_CURSOR)
                        _gtk_rtg_canvas_set_cursor(canvas, canvas->priv->view.gizmo_scale->selected == SELECTED_XYZ ? CURS_SCALE_UNIFORM
                                                                                                                    : CURS_SCALE_NONUNIFORM);
                    else
                        _gtk_rtg_canvas_set_cursor(canvas, CURS_NORMAL);

                    if(ret & GIZMO_PROCESS_REDRAW)
                        gtk_rtg_canvas_render(canvas);

                    return FALSE;
                }
            }else
            if((canvas->priv->display & DISPLAY_ROTATE_GIZMO) && SELECTED(canvas->priv->scene))
            {
                guint ret;

                if((ret = gtk_rtg_tripod_process_motion(canvas->priv->view.gizmo_rotate, point.x, point.y)))
                {
                    if(ret & GIZMO_PROCESS_CURSOR)
                        _gtk_rtg_canvas_set_cursor(canvas, CURS_GIZMO_ROTATE);
                    else
                        _gtk_rtg_canvas_set_cursor(canvas, CURS_NORMAL);

                    if(ret & GIZMO_PROCESS_REDRAW)
                        gtk_rtg_canvas_render(canvas);

                    return FALSE;
                }
            }

            /* test selection */
            gint id = gtk_rtg_scene_point_select(canvas->priv->scene, point, canvas, TRUE);

            if(id == -1) _gtk_rtg_canvas_set_cursor(canvas, CURS_NORMAL);	/* background is selected */
            else
            {
                GtkRtgPrimitive *prim = gtk_rtg_scene_get_primitive(canvas->priv->scene, id);

                if(prim && IS_SELECTED(prim))
                {
                    if(canvas->priv->mode == GTK_RTG_CANVAS_INT_MODE_MOVE)
                    {
                        _gtk_rtg_canvas_set_cursor(canvas, CURS_MOVE);
                        return FALSE;
                    }else
                    if(canvas->priv->mode == GTK_RTG_CANVAS_INT_MODE_SCALE)
                    {
                        _gtk_rtg_canvas_set_cursor(canvas, canvas->priv->view.gizmo_scale->selected ==
                                                            SELECTED_XYZ ? CURS_SCALE_UNIFORM
                                                                         : CURS_SCALE_NONUNIFORM);
                        return FALSE;
                    }else
                    if(canvas->priv->mode == GTK_RTG_CANVAS_INT_MODE_ROTATE)
                    {
                        _gtk_rtg_canvas_set_cursor(canvas, CURS_GIZMO_ROTATE);
                        return FALSE;
                    }
                }

                _gtk_rtg_canvas_set_cursor(canvas, CURS_CROSS);
            }
        }
    }

    return FALSE;
}

static gboolean
gtk_rtg_canvas_button_release(GtkWidget *widget, GdkEventButton	*event)
{
    GtkRtgCanvas *canvas = GTK_RTG_CANVAS(widget);

    if(IS_MODE_VIEW(canvas))
    {
        if(canvas->priv->view.pan_init)
        {
            canvas->priv->view.pan_init = 0;
        }

        _gtk_rtg_canvas_set_cursor(canvas, CURS_NORMAL);

    }else
    if(IS_MODE_OBJECT(canvas))
    {
        SWITCH_CONTEXT(canvas);

        if(canvas->priv->display & DISPLAY_MOVE_GIZMO)
        {
            if(canvas->priv->view.gizmo_move->drag)
            {
                gtk_rtg_tripod_end_process_drag(canvas->priv->view.gizmo_move);

                canvas->priv->view.gizmo_move->drag = 0;
                canvas->priv->view.can_drag = 0;
                canvas->priv->view.sel_postponed = 0;
            }else
            if(canvas->priv->view.gizmo_move->sel_hit)
                return FALSE;
        }else
        if(canvas->priv->display & DISPLAY_SCALE_GIZMO)
        {
            if(canvas->priv->view.gizmo_scale->drag)
            {
                gtk_rtg_tripod_end_process_drag(canvas->priv->view.gizmo_scale);

                canvas->priv->view.gizmo_scale->drag = 0;
                canvas->priv->view.can_drag = 0;
                canvas->priv->view.sel_postponed = 0;

                GtkRtgGizmo *gizmo = GTK_RTG_GIZMO(canvas->priv->view.gizmo_scale);
                _gtk_rtg_gizmo_calc_offset(gizmo, canvas);
                gtk_rtg_tripod_project(canvas->priv->view.gizmo_scale, &canvas->priv->view.gizmo_scale->offset);
                gtk_rtg_canvas_render(canvas);

                const set<GtkRtgPrimitive*> *prims = gtk_rtg_scene_get_selected(canvas->priv->scene);
                for(set<GtkRtgPrimitive*>::const_iterator it=prims->begin(); it!=prims->end(); ++it)
                    calc_bounding_sphere(*it);

                return FALSE;
            }else
            if(canvas->priv->view.gizmo_scale->sel_hit)
                return FALSE;
        }else
        if(canvas->priv->display & DISPLAY_ROTATE_GIZMO)
        {
            if(canvas->priv->view.gizmo_rotate->drag)
            {
                gtk_rtg_tripod_end_process_drag(canvas->priv->view.gizmo_rotate);

                canvas->priv->view.gizmo_rotate->drag = 0;
                canvas->priv->view.can_drag = 0;
                canvas->priv->view.sel_postponed = 0;

                gtk_rtg_canvas_render(canvas);
                return FALSE;
            }else
            if(canvas->priv->view.gizmo_rotate->sel_hit)
                return FALSE;
        }

        GdkPoint point = {(gint)event->x, gtk_widget_get_allocated_height(widget) - (gint)event->y};

        if(canvas->priv->view.pickbuff.update)
            SWITCH_CONTEXT(canvas);

        if(canvas->priv->view.sel_postponed)
        {
            canvas->priv->view.sel_postponed = 0;

            gint id = gtk_rtg_scene_point_select(canvas->priv->scene, point, canvas);

            if(id == -1) /* background is selected */
            {
                /* check for seleced objects, deselect if any */
                if(gtk_rtg_scene_deselect_all(canvas->priv->scene))
                    /* meshes are deselected, redraw the scene */
                    gtk_rtg_canvas_render(canvas);

            }else
                gtk_rtg_canvas_render(canvas);

            return FALSE;
        }

        /* test selection */
        gint id = gtk_rtg_scene_point_select(canvas->priv->scene, point, canvas, TRUE);

        /* background is selected */
        if(id == -1)	_gtk_rtg_canvas_set_cursor(canvas, CURS_NORMAL);
        else
        {
            if(canvas->priv->mode == GTK_RTG_CANVAS_INT_MODE_MOVE)
                _gtk_rtg_canvas_set_cursor(canvas, CURS_MOVE);
            else
            if(canvas->priv->mode == GTK_RTG_CANVAS_INT_MODE_SCALE)
                _gtk_rtg_canvas_set_cursor(canvas, canvas->priv->view.gizmo_scale->selected ==
                                                    SELECTED_XYZ ? CURS_SCALE_UNIFORM
                                                                 : CURS_SCALE_NONUNIFORM);
            else
            if(canvas->priv->mode == GTK_RTG_CANVAS_INT_MODE_ROTATE)
                _gtk_rtg_canvas_set_cursor(canvas, CURS_GIZMO_ROTATE);
            else
                _gtk_rtg_canvas_set_cursor(canvas, CURS_CROSS);
        }

        if(canvas->priv->display & DISPLAY_MARQUEE)
        {
            /* hide marquee */
            canvas->priv->display &= ~DISPLAY_MARQUEE;

            /*
             * Select from scene
             *
             */

            gtk_rtg_scene_marquee_select(canvas->priv->scene, canvas->priv->marquee.start,
                                         canvas->priv->marquee.end, canvas);
            /* updated the scene */
            gtk_rtg_canvas_render(canvas);
        }
    }

    return FALSE;
}


static gboolean
gtk_rtg_canvas_expose(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    GtkRtgCanvas *canvas = GTK_RTG_CANVAS(widget);
    gtk_rtg_canvas_configure(widget, NULL);

    /*** OpenGL BEGIN ***/
    if(!gtk_rtg_gl_backend_make_current(canvas->priv->backend))
        return FALSE;

    /* prepare for drawing */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* render grid */
    if(canvas->priv->display & DISPLAY_GRID)
        gtk_rtg_canvas_grid_render(canvas->priv->grid, canvas);

    /* render the scene objects (meshes) */
    if(canvas->priv->display & DISPLAY_SCENE)
        gtk_rtg_scene_render(canvas->priv->scene, canvas->priv->render_mode, canvas->priv->view.cam);

    /* User provided callback (i.e. model transformations and geometry) */
    if(canvas->priv->render)
        canvas->priv->render(canvas->priv->data[GTK_RTG_RENDER]);

    /* render grid snap cursor */
    if(canvas->priv->display & DISPLAY_GRID_SNAP_CURSOR)
        gtk_rtg_canvas_grid_render_snap_cursor(canvas->priv->grid, canvas);


    if(canvas->priv->display > DISPLAY_ORTHO)
    {
        /*
         * Orthographic projection, pure 2D.
         *
         * Top stacks projection and model-view matrix will be left untouched.
         * Current matrix mode will be preserved.
         **/

        GLint matrix_mode = 0;

        glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);

        gboolean a,b,c,d,e;
        DISABLE_GL_COLOR_MODIFIERS(a,b,c,d,e);

        if(matrix_mode != GL_PROJECTION)
            glMatrixMode(GL_PROJECTION);

        glPushMatrix();
        glLoadIdentity();
        glOrtho(0.0, gdouble(gtk_widget_get_allocated_width(widget)), 0.0,
                     gdouble(gtk_widget_get_allocated_height(widget)), 0.0, 1.0);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        /* tripod */
        if(canvas->priv->display & DISPLAY_TRIPOD)
            gtk_rtg_tripod_render(canvas->priv->view.tripod, canvas->priv->view.predefined);

        if(canvas->priv->scene && SELECTED(canvas->priv->scene)) /* any primitive selected? */
        {
            /*
             * Render custom gizmo's
             *
             */
            if(canvas->priv->display & DISPLAY_MOVE_GIZMO)
                gtk_rtg_tripod_render(canvas->priv->view.gizmo_move, canvas->priv->view.predefined);
            else
            if(canvas->priv->display & DISPLAY_SCALE_GIZMO)
                gtk_rtg_tripod_render(canvas->priv->view.gizmo_scale, canvas->priv->view.predefined);
            else
            if(canvas->priv->display & DISPLAY_ROTATE_GIZMO)
                gtk_rtg_tripod_render(canvas->priv->view.gizmo_rotate, canvas->priv->view.predefined);
        }

        /* text */
        if(canvas->priv->display & DISPLAY_TEXT)
            _gtk_rtg_canvas_render_text(canvas);

        if(canvas->priv->display & DISPLAY_FOCUSED)
        {
            /* view-gizmo */
            if(canvas->priv->display & DISPLAY_VIEW_GIZMO)
                _gtk_rtg_canvas_camera_altaz_render_gizmo(canvas);

            /* focused */
            _gtk_rtg_canvas_render_border(canvas, canvas->priv->c_focused);
        }else
        /* un-focused */
            _gtk_rtg_canvas_render_border(canvas, canvas->priv->c_border);

        /* selection marquee */
        if(canvas->priv->display & DISPLAY_MARQUEE)
            _gtk_rtg_canvas_render_marquee_selection(canvas);

        glPopMatrix();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();

        /* rewind matrix mode */
        if(matrix_mode != GL_PROJECTION)
            glMatrixMode(matrix_mode);

        ENABLE_GL_COLOR_MODIFIERS(a,b,c,d,e);
    }

    gtk_rtg_gl_backend_paint(canvas->priv->backend, cr);

    /* Keep the viewport caption antialiased at the GTK display scale. */
    GtkRtgText *caption = canvas->priv->view.text;
    if(caption && gtk_rtg_text_is_visible(caption))
    {
        PangoLayout *layout = gtk_widget_create_pango_layout(widget, gtk_rtg_text_get_string(caption));
        PangoFontDescription *font = pango_font_description_from_string(gtk_rtg_text_get_font_desc(caption));
        pango_layout_set_font_description(layout, font);
        pango_font_description_free(font);
        const GdkPoint *position = gtk_rtg_text_get_position(caption);
        const GdkColor *color = gtk_rtg_text_get_color(caption);
        const double top = position->y - double(pango_layout_get_baseline(layout)) / PANGO_SCALE;
        cairo_save(cr);
        cairo_set_source_rgba(cr, 0., 0., 0., .65);
        cairo_move_to(cr, position->x + 1, top + 1);
        pango_cairo_show_layout(cr, layout);
        cairo_set_source_rgb(cr, color->red / 65535., color->green / 65535., color->blue / 65535.);
        cairo_move_to(cr, position->x, top);
        pango_cairo_show_layout(cr, layout);
        cairo_restore(cr);
        g_object_unref(layout);
    }

    GL_CHECK_ERROR;

    // Context stays current for the original camera and gizmo callbacks.
    /*** OpenGL END ***/

    return FALSE;
}

static void
gtk_rtg_canvas_camera_view(GtkRtgCamera *camera, GParamSpec *pspec, GtkRtgCanvas *canvas)
{
    /* update */
    if(!canvas->priv->view.pickbuff.update)
        canvas->priv->view.pickbuff.update = 1;

    SWITCH_CONTEXT(canvas);

    if(canvas->priv->view.tripod)
        gtk_rtg_tripod_project(canvas->priv->view.tripod);

    if(SELECTED(canvas->priv->scene) && (canvas->priv->display & DISPLAY_MOVE_GIZMO))
        gtk_rtg_tripod_project(canvas->priv->view.gizmo_move, &canvas->priv->view.gizmo_move->offset);
    else
    if(SELECTED(canvas->priv->scene) && (canvas->priv->display & DISPLAY_SCALE_GIZMO))
        gtk_rtg_tripod_project(canvas->priv->view.gizmo_scale, &canvas->priv->view.gizmo_scale->offset);
    else
    if(SELECTED(canvas->priv->scene) && (canvas->priv->display & DISPLAY_ROTATE_GIZMO))
        gtk_rtg_tripod_project(canvas->priv->view.gizmo_rotate, &canvas->priv->view.gizmo_rotate->offset);

    gtk_rtg_camera_view_load_matrix(canvas->priv->view.cam);
    gtk_rtg_canvas_render(canvas);
}


static void
_gtk_rtg_canvas_camera_set_safe_view_up(GtkRtgCanvas *canvas, GObject *camera)
{
    vector3d *up = NULL;
    g_object_get(camera, "view-up", &up, NULL);

    /* is correction needed? */
    if(*up != *canvas->priv->view.altaz.up)
        *up = *canvas->priv->view.altaz.up;
}


static void
gtk_rtg_canvas_camera_view_up(GtkRtgCamera *camera, GParamSpec *pspec, GtkRtgCanvas *canvas)
{
    _gtk_rtg_canvas_camera_set_safe_view_up(canvas, G_OBJECT(camera));
}


static void
gtk_rtg_canvas_camera_projection(GtkRtgCamera *camera, GParamSpec *pspec, GtkRtgCanvas *canvas)
{
    /* update */
    if(!canvas->priv->view.pickbuff.update)
        canvas->priv->view.pickbuff.update = 1;

    /* use only the views camera */
    if(camera != canvas->priv->view.cam)
        return;

    SWITCH_CONTEXT(canvas);

    if(canvas->priv->view.tripod)
        gtk_rtg_tripod_project(canvas->priv->view.tripod);

    if(SELECTED(canvas->priv->scene) && (canvas->priv->display & DISPLAY_MOVE_GIZMO))
        gtk_rtg_tripod_project(canvas->priv->view.gizmo_move, &canvas->priv->view.gizmo_move->offset);
    else
    if(SELECTED(canvas->priv->scene) && (canvas->priv->display & DISPLAY_SCALE_GIZMO))
        gtk_rtg_tripod_project(canvas->priv->view.gizmo_scale, &canvas->priv->view.gizmo_scale->offset);
    else
    if(SELECTED(canvas->priv->scene) && (canvas->priv->display & DISPLAY_ROTATE_GIZMO))
        gtk_rtg_tripod_project(canvas->priv->view.gizmo_rotate, &canvas->priv->view.gizmo_rotate->offset);

    gtk_rtg_camera_projection_load_matrix(canvas->priv->view.cam);
    gtk_rtg_canvas_render(canvas);
}


static void
gtk_rtg_canvas_camera_view_distance(GtkRtgCamera *camera, GParamSpec *pspec, GtkRtgCanvas *canvas)
{
    /* use only the views camera */
    if(camera != canvas->priv->view.cam)
        return;

    gdouble	       distance;
    GtkRtgCamProj *projection;

    g_object_get(G_OBJECT(camera), "view-distance", &distance,
                                   "proj-type",		&projection, NULL);
    /* fade grid color */
    if(canvas->priv->grid)
        gtk_rtg_canvas_grid_fade_line_color(canvas->priv->grid, gfloat(distance));

    /* recreate 'projection' matrix for orthographic proj. */
    if(*projection == GTK_RTG_CAM_PROJ_ORTHO)
        gtk_rtg_camera_projection_do_matrix(camera);
}


static void
gtk_rtg_canvas_camera_view_direction(GtkRtgCamera *camera, GParamSpec *pspec, GtkRtgCanvas *canvas)
{
    _gtk_rtg_canvas_camera_altaz_init(canvas);
}


static void
gtk_rtg_canvas_grid_changed(GtkRtgCanvasGrid *grid, GParamSpec *pspec, GtkRtgCanvas *canvas)
{
    /* trigger cameras 'view-distance' signal to setup grid color */
    if(canvas->priv->is_view)
    {
        gtk_rtg_camera_view_set_distance(canvas->priv->view.cam,
        gtk_rtg_camera_view_get_distance(canvas->priv->view.cam));
    }

    /* set flag */
    gtk_rtg_canvas_grid_get_snap_cursor(grid) ? canvas->priv->display |=  DISPLAY_GRID_SNAP_CURSOR
                                              : canvas->priv->display &= ~DISPLAY_GRID_SNAP_CURSOR;
    gtk_rtg_canvas_render(canvas);
}

static void
gtk_rtg_canvas_render_offscreen(GtkRtgCanvas *canvas)
{
    if(SELECTED(canvas->priv->scene))
    {
        if(canvas->priv->display & DISPLAY_MOVE_GIZMO)
            gtk_rtg_tripod_render_offscreen(canvas->priv->view.gizmo_move, canvas->priv->view.predefined);
        else
        if(canvas->priv->display & DISPLAY_SCALE_GIZMO)
            gtk_rtg_tripod_render_offscreen(canvas->priv->view.gizmo_scale, canvas->priv->view.predefined);
        else
        if(canvas->priv->display & DISPLAY_ROTATE_GIZMO)
            gtk_rtg_tripod_render_offscreen(canvas->priv->view.gizmo_rotate, canvas->priv->view.predefined);
    }
}


static void
gtk_rtg_canvas_scene_configured(GtkRtgScene *scene, gpointer data)
{
    GtkRtgCanvas    *canvas = GTK_RTG_CANVAS(data);

    /* update */
    if(!canvas->priv->view.pickbuff.update)
        canvas->priv->view.pickbuff.update = 1;
}


static void
gtk_rtg_canvas_scene_camera_default(GObject *object, GtkRtgCamera *camera, GtkRtgCanvas *_canvas, gpointer data)
{
    GtkRtgCanvas	*canvas = GTK_RTG_CANVAS(data);

    if(canvas->priv->view.cam)
    {
        /* ignore if camera is not for this canvas */
        if(_canvas && _canvas != canvas)
            return;

        /* disconnect */
        g_signal_handlers_disconnect_by_func(canvas->priv->view.cam, (void *)gtk_rtg_canvas_camera_view,	       canvas);
        g_signal_handlers_disconnect_by_func(canvas->priv->view.cam, (void *)gtk_rtg_canvas_camera_view_up,	       canvas);
        g_signal_handlers_disconnect_by_func(canvas->priv->view.cam, (void *)gtk_rtg_canvas_camera_view_distance,  canvas);
        g_signal_handlers_disconnect_by_func(canvas->priv->view.cam, (void *)gtk_rtg_canvas_camera_view_direction, canvas);
        g_signal_handlers_disconnect_by_func(canvas->priv->view.cam, (void *)gtk_rtg_canvas_camera_projection,	   canvas);

        canvas->priv->is_view  =  FALSE;
        canvas->priv->view.cam =  NULL;
        canvas->priv->mode     =  GTK_RTG_CANVAS_INT_MODE_OFF;
        canvas->priv->display &= ~DISPLAY_VIEW_GIZMO;
    }

    if(camera)
    {
        /* connect to signals */
        g_signal_connect(camera, "notify::view-matrix",	   G_CALLBACK(gtk_rtg_canvas_camera_view),	         canvas);
        g_signal_connect(camera, "notify::view-up",	   	   G_CALLBACK(gtk_rtg_canvas_camera_view_up),        canvas);
        g_signal_connect(camera, "notify::view-distance",  G_CALLBACK(gtk_rtg_canvas_camera_view_distance),  canvas);
        g_signal_connect(camera, "notify::view-direction", G_CALLBACK(gtk_rtg_canvas_camera_view_direction), canvas);
        g_signal_connect(camera, "notify::proj-matrix",	   G_CALLBACK(gtk_rtg_canvas_camera_projection),	 canvas);

        /* set aspect ratio */
        g_object_set(camera, "proj-aspect", (gfloat)gtk_widget_get_allocated_width(GTK_WIDGET(canvas))/
                                            (gfloat)gtk_widget_get_allocated_height(GTK_WIDGET(canvas)), NULL);

        canvas->priv->view.cam = camera;
        canvas->priv->is_view  = TRUE;

        /* mark update */
        if(!canvas->priv->view.pickbuff.update)
            canvas->priv->view.pickbuff.update = 1;

        /* update text */
        g_object_set(canvas->priv->view.text, "string", p_view_text[canvas->priv->view.predefined], NULL);

        /* make safe view-up vector */
        _gtk_rtg_canvas_camera_set_safe_view_up(canvas, G_OBJECT(camera));

        /* init 'altaz' */
        _gtk_rtg_canvas_camera_altaz_init(canvas);

        /* update gizmo */
        _gtk_rtg_canvas_camera_altaz_gizmo_update(canvas);

        /* build cameras matrices and load on stacks */
        gtk_rtg_camera_view_do_matrix(canvas->priv->view.cam);
        gtk_rtg_camera_projection_do_matrix(canvas->priv->view.cam);

        /* trigger cameras 'view-distance' signal to setup grid color */
        if(canvas->priv->grid)
            gtk_rtg_camera_view_set_distance(canvas->priv->view.cam,
                                             gtk_rtg_camera_view_get_distance(canvas->priv->view.cam));
    }
}


static void
_gtk_rtg_gizmo_calc_offset(GtkRtgGizmo *gizmo, GtkRtgCanvas *canvas)
{
    if(SELECTED(canvas->priv->scene) == 1)
    {
        GtkRtgPrimitive *prim = FIRST_SELECTED(canvas->priv->scene);
        GTK_RTG_TRIPOD(gizmo)->offset = object2world(prim, *gtk_rtg_primitive_get_pivot(prim));
    }else
    {
        /*
         * Multiple selection
         *
         */

        const set<GtkRtgPrimitive *> *prims = gtk_rtg_scene_get_selected(canvas->priv->scene);
        point3f pmin, pmax;

        _gtk_rtg_canvas_primitives_get_pmin_pmax(canvas, prims, pmin, pmax);

        /* d_2 */
        point3f d_2 = (pmax - pmin) * .5f;

        /* the selected obj's center */
        GTK_RTG_TRIPOD(gizmo)->offset = pmin + d_2;
    }
}


static void
gtk_rtg_canvas_scene_prim_sel_changed(GtkRtgScene *scene, GtkRtgCanvas *canvas)
{
    SWITCH_CONTEXT(canvas);

    /*
     * TODO (mark buff update only if gizmo is activated!!!!).
     *
     */

    /*
     * Mark a picking buffer to be updated
     */
    if(!canvas->priv->view.pickbuff.update)
        canvas->priv->view.pickbuff.update = 1;

    if(SELECTED(scene) && (canvas->priv->display & DISPLAY_MOVE_GIZMO))
    {
        _gtk_rtg_gizmo_calc_offset(GTK_RTG_GIZMO(canvas->priv->view.gizmo_move), canvas);
        gtk_rtg_tripod_project(canvas->priv->view.gizmo_move, &canvas->priv->view.gizmo_move->offset);
    }else
    if(SELECTED(scene) && (canvas->priv->display & DISPLAY_SCALE_GIZMO))
    {
        _gtk_rtg_gizmo_calc_offset(GTK_RTG_GIZMO(canvas->priv->view.gizmo_scale), canvas);
        gtk_rtg_tripod_project(canvas->priv->view.gizmo_scale, &canvas->priv->view.gizmo_scale->offset);
    }
    else
    if(SELECTED(scene) && (canvas->priv->display & DISPLAY_ROTATE_GIZMO))
    {
        _gtk_rtg_gizmo_calc_offset(GTK_RTG_GIZMO(canvas->priv->view.gizmo_rotate), canvas);
        gtk_rtg_tripod_project(canvas->priv->view.gizmo_rotate, &canvas->priv->view.gizmo_rotate->offset);

        const set<GtkRtgPrimitive*> *prims = gtk_rtg_scene_get_selected(canvas->priv->scene);
        for(set<GtkRtgPrimitive*>::const_iterator it=prims->begin(); it!=prims->end(); ++it)
        {
            point3f p = canvas->priv->view.gizmo_rotate->offset;
            p = world2object(*it, p);
            gtk_rtg_primitive_set_pivot(*it, &p);
        }

    }
}


/***
 ***
 *** Publics.
 ***
 ***
 ***
 ***
 ***
 ***/

void
gtk_rtg_canvas_init_funct_add(GtkRtgCanvas *canvas, GTK_RTG_FUNCT_OBJECT funct, gpointer data) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    g_throw_if_fail(funct != NULL);

    canvas->priv->init = funct;
    canvas->priv->data[GTK_RTG_INIT] = data;
}


void
gtk_rtg_canvas_resize_funct_add(GtkRtgCanvas *canvas, GTK_RTG_FUNCT_RESIZE funct, gpointer data) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    g_throw_if_fail(funct != NULL);

    canvas->priv->resize = funct;
    canvas->priv->data[GTK_RTG_RESIZE] = data;
}


void
gtk_rtg_canvas_render_funct_add(GtkRtgCanvas *canvas, GTK_RTG_FUNCT_PURE funct, gpointer data) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    g_throw_if_fail(funct != NULL);

    canvas->priv->render = funct;
    canvas->priv->data[GTK_RTG_RENDER] = data;
}


void
gtk_rtg_canvas_destroy_funct_add(GtkRtgCanvas *canvas, GTK_RTG_FUNCT_OBJECT funct, gpointer data) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    g_throw_if_fail(funct != NULL);

    canvas->priv->destroy = funct;
    canvas->priv->data[GTK_RTG_DESTROY] = data;
}


void
gtk_rtg_canvas_timeout_funct_add(GtkRtgCanvas	     *canvas,
                                 GTK_RTG_FUNCT_PURE   funct,
                                 guint 				  interval,
                                 gboolean 			  animate,
                                 gpointer	      	  data		) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    g_throw_if_fail(funct != NULL);

    canvas->priv->timeout = funct;
    canvas->priv->data[GTK_RTG_TIMEOUT] = data;
    canvas->priv->timeout_interval = interval;
    canvas->priv->is_animated = animate;

    if(animate)
        _gtk_rtg_canvas_timeout_add(canvas);
}


void
gtk_rtg_canvas_animation_start(GtkRtgCanvas *canvas) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    _gtk_rtg_canvas_timeout_add(canvas);
}


void
gtk_rtg_canvas_animation_stop(GtkRtgCanvas *canvas) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    _gtk_rtg_canvas_timeout_remove(canvas);
}


gboolean
gtk_rtg_canvas_is_animated(GtkRtgCanvas *canvas) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    return canvas->priv->is_animated;
}


void
gtk_rtg_canvas_render(GtkRtgCanvas *canvas) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));

    if(canvas->priv->is_configured)
        gtk_widget_queue_draw(GTK_WIDGET(canvas));
}


/***
 *** This function pushes text to the render map. Font is queried
 *** from the font map. If not found, new font type will be registered
 *** and associated with the text.
 *** There can be (theoretically) max. 'int' texts pushed.
 *** Pushed text cannot be pushed again until is poped. Such push will
 *** throw GtkRtgException.
 ***/
void
gtk_rtg_canvas_text_push(GtkRtgCanvas *canvas, GtkRtgText *text) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    g_throw_if_fail(gtk_widget_get_realized(GTK_WIDGET(canvas)));
    g_throw_if_fail(GTK_RTG_IS_TEXT(text));

    _gtk_rtg_canvas_text_push_real(canvas, text, canvas->priv->m_text);
}


/***
 *** This function pops text from the render map.
 *** Released object id is pushed back to the vector.
 ***
 ***/
void
gtk_rtg_canvas_text_pop(GtkRtgCanvas *canvas, GtkRtgText *text) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    g_throw_if_fail(GTK_RTG_IS_TEXT(text));

    /* check whether text is pushed */
    gchar *canvas_obj_id = (gchar*) g_object_get_data(G_OBJECT(text), canvas->priv->id);

    if(canvas_obj_id)
    {
        m_txtType::iterator iter = canvas->priv->m_text->find(canvas_obj_id);

        if(iter != canvas->priv->m_text->end())
        {
            _gtk_rtg_canvas_release_object_id(canvas, &canvas_obj_id);
            g_object_set_data(G_OBJECT(text), canvas->priv->id, canvas_obj_id);

            /* erase from render map */
            canvas->priv->m_text->erase(iter);

            /* decref text */
            g_object_unref(text);
        }
    }

    /* no more texts to display? */
    if(canvas->priv->m_text->empty())
        canvas->priv->display &= ~DISPLAY_TEXT;
}


/***
 *** Check whether text is already pushed to the canvas.
 ***
 ***/
gboolean
gtk_rtg_canvas_text_is_pushed(GtkRtgCanvas *canvas, GtkRtgText *text) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    g_throw_if_fail(GTK_RTG_IS_TEXT(text));

    gchar *canvas_obj_id  = (gchar*) g_object_get_data(G_OBJECT(text), canvas->priv->id);
    return canvas_obj_id != NULL;
}


void
gtk_rtg_canvas_interactive_set_mode(GtkRtgCanvas *canvas, GtkRtgCanvasInteractiveMode mode) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));

    /* no effect if camera is not set.*/
    if(!canvas->priv->is_view)
        return;

    /* turn off gizmo's */
    canvas->priv->display &= ~(DISPLAY_VIEW_GIZMO | DISPLAY_MOVE_GIZMO | DISPLAY_SCALE_GIZMO | DISPLAY_ROTATE_GIZMO);

    SWITCH_CONTEXT(canvas);

    /* enable gizmo */
    if(mode == GTK_RTG_CANVAS_INT_MODE_VIEW)	canvas->priv->display |= DISPLAY_VIEW_GIZMO;
    else
    if(mode == GTK_RTG_CANVAS_INT_MODE_MOVE)
    {
        canvas->priv->display |= DISPLAY_MOVE_GIZMO;

        if(canvas->priv->scene && SELECTED(canvas->priv->scene))
        {
            GtkRtgGizmo *gizmo = GTK_RTG_GIZMO(canvas->priv->view.gizmo_move);
            _gtk_rtg_gizmo_calc_offset(gizmo, canvas);

            gtk_rtg_tripod_project(canvas->priv->view.gizmo_move, &canvas->priv->view.gizmo_move->offset);

            /* update off-screen */
            canvas->priv->view.pickbuff.update = 1;
        }
    }else
    if(mode == GTK_RTG_CANVAS_INT_MODE_SCALE)
    {
        canvas->priv->display |= DISPLAY_SCALE_GIZMO;

        if(canvas->priv->scene && SELECTED(canvas->priv->scene))
        {
            GtkRtgGizmo *gizmo = GTK_RTG_GIZMO(canvas->priv->view.gizmo_scale);
            _gtk_rtg_gizmo_calc_offset(gizmo, canvas);

            gtk_rtg_tripod_project(canvas->priv->view.gizmo_scale, &canvas->priv->view.gizmo_scale->offset);

            /* update off-screen */
            canvas->priv->view.pickbuff.update = 1;
        }
    }else
    if(mode == GTK_RTG_CANVAS_INT_MODE_ROTATE)
    {
        canvas->priv->display |= DISPLAY_ROTATE_GIZMO;

        if(canvas->priv->scene && SELECTED(canvas->priv->scene))
        {
            GtkRtgGizmo *gizmo = GTK_RTG_GIZMO(canvas->priv->view.gizmo_rotate);
            _gtk_rtg_gizmo_calc_offset(gizmo, canvas);

            gtk_rtg_tripod_project(canvas->priv->view.gizmo_rotate, &canvas->priv->view.gizmo_rotate->offset);

            /* update off-screen */
            canvas->priv->view.pickbuff.update = 1;
        }
    }else
    if(mode == GTK_RTG_CANVAS_INT_MODE_SELECT)
        _gtk_rtg_canvas_set_cursor(canvas, CURS_NORMAL);   /* set cursor to normal */

    canvas->priv->mode = mode;

    if(gtk_widget_get_visible(GTK_WIDGET(canvas)))
        gtk_rtg_canvas_render(canvas);
}


GtkRtgCanvasInteractiveMode
gtk_rtg_canvas_interactive_get_mode(GtkRtgCanvas *canvas) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    return canvas->priv->mode;
}


/***
 *** This function pushes grid into the canvas.
 *** Pushed grid cannot be pushed again until is popped. Such push will
 *** throw GtkRtgException.
 ***
 ***/
void
gtk_rtg_canvas_grid_push(GtkRtgCanvas *canvas, GtkRtgCanvasGrid *grid) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    g_throw_if_fail(GTK_RTG_IS_CANVAS_GRID(grid));

    /* check whether grid is already pushed */
    gchar *canvas_obj_id = (gchar*) g_object_get_data(G_OBJECT(grid), canvas->priv->id);
    g_throw_if_fail(canvas_obj_id == NULL);

    /* get (free) obj-id */
    canvas_obj_id = _gtk_rtg_canvas_create_object_id(canvas);

    /* set to the object */
    g_object_set_data(G_OBJECT(grid), canvas->priv->id, canvas_obj_id);

    /* pop if set */
    if(canvas->priv->grid)
        gtk_rtg_canvas_grid_pop(canvas);

    /* connect to signals */
    g_signal_connect(G_OBJECT(grid), "notify::grid-lines",       G_CALLBACK(gtk_rtg_canvas_grid_changed),  canvas);
    g_signal_connect(G_OBJECT(grid), "notify::grid-scale",       G_CALLBACK(gtk_rtg_canvas_grid_changed),  canvas);
    g_signal_connect(G_OBJECT(grid), "notify::line-width",       G_CALLBACK(gtk_rtg_canvas_grid_changed),  canvas);
    g_signal_connect(G_OBJECT(grid), "notify::line-color",       G_CALLBACK(gtk_rtg_canvas_grid_changed),  canvas);
    g_signal_connect(G_OBJECT(grid), "notify::cross-line-color", G_CALLBACK(gtk_rtg_canvas_grid_changed),  canvas);
    g_signal_connect(G_OBJECT(grid), "notify::fade-line-color",  G_CALLBACK(gtk_rtg_canvas_grid_changed),  canvas);
    g_signal_connect(G_OBJECT(grid), "notify::snap-cursor",      G_CALLBACK(gtk_rtg_canvas_grid_changed),  canvas);  /* redraw only */

    /* set as default */
    canvas->priv->grid = grid;
    canvas->priv->display |= DISPLAY_GRID;

    /* set flag */
    gtk_rtg_canvas_grid_get_snap_cursor(grid) ? canvas->priv->display |=  DISPLAY_GRID_SNAP_CURSOR
                                              : canvas->priv->display &= ~DISPLAY_GRID_SNAP_CURSOR;

    /* incref grid */
    g_object_ref(grid);

    /* trigger cameras 'view-distance' signal to setup grid color */
    if(canvas->priv->is_view)
        gtk_rtg_camera_view_set_distance(canvas->priv->view.cam,
                                         gtk_rtg_camera_view_get_distance(canvas->priv->view.cam));
}

/***
 *** This function pops grid from the canvas.
 *** Released object id is pushed back to the vector.
 ***
 ***/
void
gtk_rtg_canvas_grid_pop(GtkRtgCanvas *canvas) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));

    if(canvas->priv->grid)
    {
        gchar *canvas_obj_id = (gchar*) g_object_get_data(G_OBJECT(canvas->priv->grid), canvas->priv->id);
        _gtk_rtg_canvas_release_object_id(canvas, &canvas_obj_id);
        g_object_set_data(G_OBJECT(canvas->priv->grid), canvas->priv->id, canvas_obj_id);

        g_object_unref(canvas->priv->grid);
        canvas->priv->grid = NULL;
        canvas->priv->display &= ~DISPLAY_GRID;
        canvas->priv->display &= ~DISPLAY_GRID_SNAP_CURSOR;
    }
}


/***
 *** Check whether grid is already pushed to the canvas.
 ***
 ***/
gboolean
gtk_rtg_canvas_grid_is_pushed(GtkRtgCanvas *canvas, GtkRtgCanvasGrid *grid) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    g_throw_if_fail(GTK_RTG_IS_CANVAS_GRID(grid));

    gchar *canvas_obj_id = (gchar*) g_object_get_data(G_OBJECT(grid), canvas->priv->id);
    return canvas_obj_id != NULL;
}


/***
 *** This function pushes scene to the canvas.
 *** Pushed scene cannot be pushed again until is poped. Such push will
 *** throw GtkRtgException.
 ***
 ***/
void
gtk_rtg_canvas_scene_push(GtkRtgCanvas *canvas, GtkRtgScene *scene) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    g_throw_if_fail(GTK_RTG_IS_SCENE(scene));

    /* check whether scene is already pushed */
    gchar *canvas_obj_id = (gchar*) g_object_get_data(G_OBJECT(scene), canvas->priv->id);
    g_throw_if_fail(canvas_obj_id == NULL);

    /* get (free) obj-id */
    canvas_obj_id = _gtk_rtg_canvas_create_object_id(canvas);

    /* set to the object */
    g_object_set_data(G_OBJECT(scene), canvas->priv->id, canvas_obj_id);

    /* pop if set */
    if(canvas->priv->scene)
        gtk_rtg_canvas_scene_pop(canvas);

    /* set as default */
    canvas->priv->scene = scene;
    canvas->priv->display |= DISPLAY_SCENE;

    /* connect to 'camera-default' */
    g_signal_connect(G_OBJECT(scene), "configured",	      G_CALLBACK(gtk_rtg_canvas_scene_configured), 		 canvas);
    g_signal_connect(G_OBJECT(scene), "camera-default",   G_CALLBACK(gtk_rtg_canvas_scene_camera_default), 	 canvas);
    g_signal_connect(G_OBJECT(scene), "prim-sel-changed", G_CALLBACK(gtk_rtg_canvas_scene_prim_sel_changed), canvas);

    /* setup camera */
    gtk_rtg_canvas_scene_camera_default(NULL, gtk_rtg_scene_camera_get_default(scene), NULL, canvas);

    /* incref scene */
    g_object_ref(scene);
}


/***
 *** This function pops scene from the canvas.
 *** Released object id is pushed back to the vector.
 ***
 ***/
void
gtk_rtg_canvas_scene_pop(GtkRtgCanvas *canvas) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));

    if(canvas->priv->scene)
    {
        gchar *canvas_obj_id = (gchar*) g_object_get_data(G_OBJECT(canvas->priv->scene), canvas->priv->id);
        _gtk_rtg_canvas_release_object_id(canvas, &canvas_obj_id);
        g_object_set_data(G_OBJECT(canvas->priv->scene), canvas->priv->id, canvas_obj_id);

        g_signal_handlers_disconnect_by_func(canvas->priv->scene, (void *)gtk_rtg_canvas_scene_configured, 		 canvas);
        g_signal_handlers_disconnect_by_func(canvas->priv->scene, (void *)gtk_rtg_canvas_scene_camera_default, 	 canvas);
        g_signal_handlers_disconnect_by_func(canvas->priv->scene, (void *)gtk_rtg_canvas_scene_prim_sel_changed, canvas);

        g_object_unref(canvas->priv->scene);
        canvas->priv->scene = NULL;
        canvas->priv->display &= ~DISPLAY_SCENE;
    }
}


/***
 *** Check whether scene is already pushed to the canvas.
 ***
 ***/
gboolean
gtk_rtg_canvas_scene_is_pushed(GtkRtgCanvas *canvas, GtkRtgScene *scene) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    g_throw_if_fail(GTK_RTG_IS_SCENE(scene));

    gchar *canvas_obj_id = (gchar*) g_object_get_data(G_OBJECT(scene), canvas->priv->id);
    return canvas_obj_id != NULL;
}




/***
 *** Show/Hide views gizmo.
 ***
 *** Note: this function will have effect only if view is in interactive mode.
 ***/
void
gtk_rtg_canvas_view_gizmo_show(GtkRtgCanvas *canvas, gboolean gizmo) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));

    /* no manipulation for non-interactive view mode */
    if(IS_MODE_VIEW(canvas))
    {
        if(gizmo)
            canvas->priv->display |= DISPLAY_VIEW_GIZMO;
        else
            canvas->priv->display &= ~DISPLAY_VIEW_GIZMO;

    }
}



/***
 *** Get show on/off views gizmo.
 ***
 ***/
gboolean
gtk_rtg_canvas_view_gizmo_is_showed(GtkRtgCanvas *canvas) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    return canvas->priv->display & DISPLAY_VIEW_GIZMO;
}



/***
 *** Sets views up vector.
 ***
 *** Note: this function have effect only if view is set.
 ***
 ***/
void
gtk_rtg_canvas_view_set_up(GtkRtgCanvas *canvas, GtkRtgCanvasViewUp up) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));

    if(canvas->priv->is_view)
    {

        switch(up)
        {
            case GTK_RTG_CANVAS_VIEW_UP_Y:  X(*canvas->priv->view.altaz.up) = 0.0;
                                            Y(*canvas->priv->view.altaz.up) = 1.0;
                                            Z(*canvas->priv->view.altaz.up) = 0.0;

                                            break;
            case GTK_RTG_CANVAS_VIEW_UP_Z:
                                            X(*canvas->priv->view.altaz.up) = 0.0;
                                            Y(*canvas->priv->view.altaz.up) = 0.0;
                                            Z(*canvas->priv->view.altaz.up) = 1.0;

                                            break;
        }

        canvas->priv->view.up = up;

        /* fix. camera */
        _gtk_rtg_canvas_camera_set_safe_view_up(canvas, G_OBJECT(canvas->priv->view.cam));

        /* Keep axial presets aligned before publishing a new view matrix. */
        if(canvas->priv->view.predefined != GTK_RTG_CANVAS_VIEW_CUSTOM)
            gtk_rtg_camera_view_set_direction(canvas->priv->view.cam,
                                               &p_view_dir[up][canvas->priv->view.predefined]);
        for(GtkRtgTripod *tripod : {canvas->priv->view.gizmo_move,
                                   canvas->priv->view.gizmo_scale,
                                   canvas->priv->view.gizmo_rotate})
            if(tripod)
                tripod->fallback = TRUE;

        /* init 'altaz' */
        _gtk_rtg_canvas_camera_altaz_init(canvas);

        /* recreate view matrix */
        gtk_rtg_camera_view_do_matrix(canvas->priv->view.cam);
    }
}


/***
 *** Gets views up vector.
 ***
 ***
 ***/
GtkRtgCanvasViewUp
gtk_rtg_canvas_view_get_up(GtkRtgCanvas *canvas) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    return canvas->priv->view.up;
}


/***
 *** Sets views predefined.
 ***
 *** Note: this function have effect only if view is set.
 ***
 ***/
void
gtk_rtg_canvas_view_set_predefined(GtkRtgCanvas *canvas, GtkRtgCanvasViewPredefined predefined) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));

    if(canvas->priv->is_view)
    {
        /*
         * Check for predefined view switch.
         *
         * Reset gizmo's fallback selection.
         */

        if(canvas->priv->view.predefined != predefined)
        {
            canvas->priv->view.gizmo_move->fallback = 1;
            canvas->priv->view.gizmo_scale->fallback = 1;
            canvas->priv->view.gizmo_rotate->fallback = 1;
        }

        /* update new state */
        canvas->priv->view.predefined = predefined;

        /* entering new 'predefined' view */
        if(predefined != GTK_RTG_CANVAS_VIEW_CUSTOM)
        {
            /* set an orthographic proj. and view direction */
            gtk_rtg_camera_set_projection(canvas->priv->view.cam, GTK_RTG_CAM_PROJ_ORTHO);
            gtk_rtg_camera_view_set_direction(canvas->priv->view.cam, &p_view_dir[canvas->priv->view.up][predefined]);

            gboolean ret = FALSE;

            if(canvas->priv->scene)
                ret = gtk_rtg_canvas_view_zoom_extents(canvas, SELECTED(canvas->priv->scene));

            if(!ret)
            {
                point3d center(0.0, 0.0, 0.0);

                /* update center */
                gtk_rtg_camera_view_set_center(canvas->priv->view.cam, &center);

                /* recreate matrices */
                gtk_rtg_camera_view_do_matrix(canvas->priv->view.cam);
                gtk_rtg_camera_projection_do_matrix(canvas->priv->view.cam);
            }
        }else
        /* entering new 'custom' view */
        {
            /* recreate 'projection' matrix for perspective proj. */
            gtk_rtg_camera_set_projection(canvas->priv->view.cam, GTK_RTG_CAM_PROJ_PERSP);
            gtk_rtg_camera_projection_do_matrix(canvas->priv->view.cam);

            if(canvas->priv->scene)
                gtk_rtg_canvas_view_zoom_extents(canvas, SELECTED(canvas->priv->scene));

            /* init 'altaz' */
            _gtk_rtg_canvas_camera_altaz_init(canvas);
        }

        /* update text */
        g_object_set(canvas->priv->view.text, "string", p_view_text[canvas->priv->view.predefined], NULL);
    }
}


/***
 *** Gets views predefined.
 ***
 ***/
GtkRtgCanvasViewPredefined
gtk_rtg_canvas_view_get_predefined(GtkRtgCanvas *canvas) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    return canvas->priv->view.predefined;
}


GtkRtgCamera *
gtk_rtg_canvas_camera_get_default(GtkRtgCanvas *canvas) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    return canvas->priv->view.cam;
}


gboolean
gtk_rtg_canvas_view_zoom_extents(GtkRtgCanvas *canvas, gboolean selected) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    if(!canvas->priv->view.cam || !canvas->priv->scene)
        return FALSE;

    const auto *primitives = selected && SELECTED(canvas->priv->scene)
        ? gtk_rtg_scene_get_selected(canvas->priv->scene) : ALL_PRIMS(canvas->priv->scene);
    point3f pmin, pmax;
    _gtk_rtg_canvas_primitives_get_pmin_pmax(canvas, primitives, pmin, pmax);
    if(X(pmin) > X(pmax) || Y(pmin) > Y(pmax) || Z(pmin) > Z(pmax))
        return FALSE;

    GtkRtgCamera *camera = canvas->priv->view.cam;
    point3d center = cast3<double, float>((pmin + pmax) * .5f);
    const double radius = std::max(double((pmax - pmin).mag()) * .525, .01);
    const double aspect = std::max(double(gtk_rtg_camera_projection_get_aspect(camera)), .001);
    const double fov = gtk_rtg_camera_projection_get_fov(camera);
    double distance;

    if(PROJECTION(camera) == GTK_RTG_CAM_PROJ_ORTHO)
        distance = radius * 112.5 / (fov * std::min(1.0, aspect));
    else
    {
        const double half_vertical = deg2rad(fov) * .5;
        const double half_horizontal = atan(tan(half_vertical) * aspect);
        distance = radius / sin(std::min(half_vertical, half_horizontal));
    }
    distance = std::max(distance, double(ZNEAR(camera)) + radius);

    SWITCH_CONTEXT(canvas);
    gtk_rtg_camera_view_set_center(camera, &center);
    gtk_rtg_camera_view_set_distance(camera, distance);
    gtk_rtg_camera_view_do_matrix(camera);
    gtk_rtg_camera_projection_do_matrix(camera);
    canvas->priv->view.pickbuff.update = 1;
    gtk_rtg_canvas_render(canvas);
    return TRUE;
}


void
gtk_rtg_canvas_set_focused(GtkRtgCanvas *canvas, gboolean focused) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));

    if(focused)
        gtk_widget_grab_focus(GTK_WIDGET(canvas));

    if((canvas->priv->display & DISPLAY_FOCUSED) && focused==FALSE)
    {
        canvas->priv->display &= ~DISPLAY_FOCUSED;
        gtk_rtg_canvas_render(canvas);
    }else
    if(!(canvas->priv->display & DISPLAY_FOCUSED) && focused)
    {
        canvas->priv->display |= DISPLAY_FOCUSED;
        gtk_rtg_canvas_render(canvas);

        /* emit 'focused' event */
        g_signal_emit(canvas, signals[FOCUSED], 0);
    }
}


gboolean
gtk_rtg_canvas_is_focused(GtkRtgCanvas *canvas) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    return canvas->priv->display & DISPLAY_FOCUSED;
}


void
gtk_rtg_canvas_gizmo_update_position(GtkRtgCanvas *canvas, gtkrtg::point3f *pos) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));

    SWITCH_CONTEXT(canvas);

    if(canvas->priv->display & DISPLAY_MOVE_GIZMO)
        gtk_rtg_tripod_project(canvas->priv->view.gizmo_move, pos);
    else
    if(canvas->priv->display & DISPLAY_SCALE_GIZMO)
        gtk_rtg_tripod_project(canvas->priv->view.gizmo_scale, pos);
    else
    if(canvas->priv->display & DISPLAY_ROTATE_GIZMO)
        gtk_rtg_tripod_project(canvas->priv->view.gizmo_rotate, pos);
}


GtkRtgCanvasGrid *
gtk_rtg_canvas_get_grid(GtkRtgCanvas *canvas) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    return canvas->priv->grid;
}


/***
 ***
 *** Privs.
 ***
 ***
 ***
 ***/

static gboolean
_gtk_rtg_canvas_timeout(GtkWidget *widget)
{
    GtkRtgCanvas *canvas = GTK_RTG_CANVAS(widget);

    /* User provided callback */
    if(canvas->priv->timeout)
        canvas->priv->timeout(canvas->priv->data[GTK_RTG_TIMEOUT]);

    gtk_rtg_canvas_render(canvas);

    return TRUE;
}


static void
_gtk_rtg_canvas_timeout_add(GtkRtgCanvas *canvas)
{
    if((canvas->priv->timeout_id == 0) && canvas->priv->timeout)
    {
        canvas->priv->timeout_id = g_timeout_add_full(G_PRIORITY_HIGH, canvas->priv->timeout_interval,
                                                      (GSourceFunc) _gtk_rtg_canvas_timeout, GTK_WIDGET(canvas), NULL);
        canvas->priv->is_animated = TRUE;
    }
}


static void
_gtk_rtg_canvas_timeout_remove(GtkRtgCanvas *canvas)
{
    if(canvas->priv->timeout_id)
    {
        g_source_remove(canvas->priv->timeout_id);
        canvas->priv->timeout_id = 0;
        canvas->priv->is_animated = FALSE;
    }
}



/***
 *** This function creates object's canvas-object id.
 *** Id is assembled from canvas and objects ids.
 *** Object id is first queried from the vector of free object ids.
 *** If not found, the peek object id is incremented and used
 *** as new object id.
 ***
 *** - it is used for cams and texts.
 ***
 ***/
static gchar *
_gtk_rtg_canvas_create_object_id(GtkRtgCanvas *canvas)
{
    guint obj_id = 0;

    if(canvas->priv->v_obj_id->empty())
        obj_id = canvas->priv->id_peek++;
    else
    {
        obj_id = canvas->priv->v_obj_id->back();
        canvas->priv->v_obj_id->pop_back();
    }

    /* allocate objects canvas-object id */
    gchar *canvas_obj_id = g_strdup_printf("%s%d", canvas->priv->id, obj_id);
    return canvas_obj_id;
}


/***
 *** This function frees object's canvas-object id.
 *** Object id is pushed back to the vector of free object ids.
 ***
 ***/
static void
_gtk_rtg_canvas_release_object_id(GtkRtgCanvas *canvas, gchar **canvas_obj_id) throw (GtkRtgException)
{
    gchar *p = strchr(*canvas_obj_id, ':');

    g_throw_if_fail(p!=NULL);

    gint obj_id = atoi(++p);

    /* push to the vector of free ids */
    canvas->priv->v_obj_id->push_back(obj_id);

    /* deallocate objects canvas-object id */
    g_free(*canvas_obj_id); *canvas_obj_id = NULL;
}


static void
_gtk_rtg_canvas_set_cursor(GtkRtgCanvas *canvas, GtkRtgCanvasCursor curs_id)
{
    if(canvas->priv->curs_id != curs_id)
    {
        GdkCursor *cursor = gtk_rtg_resource_get_cursor(curs_id);

        if(cursor)
        {
            gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(canvas)), cursor);
            canvas->priv->curs_id = curs_id;
        }
    }
}


static void
_gtk_rtg_canvas_camera_altaz_update_mxmy(GtkRtgCanvas *canvas, gdouble mx, gdouble my)
{
    /* zero is reserved to not update when dragging view-gizmo's ax */
    if(mx != 0.f) canvas->priv->view.altaz.mx = mx;
    if(my != 0.f) canvas->priv->view.altaz.my = my;
}


static void
_gtk_rtg_canvas_camera_altaz_init(GtkRtgCanvas *canvas)
{
    /* save normalized direction vector */
    vector3d *dir;

    g_object_get(canvas->priv->view.cam, "view-direction", &dir, NULL);

    *canvas->priv->view.altaz.dir = *dir;

    canvas->priv->view.altaz.dir->norm();
    canvas->priv->view.altaz.up->norm();

    /* Our initial altitude angle is computed as:
     *   1. Take the dot product of the up and view dir vectors.
     *      This yields the cosine of the angle between them.
     *   2. The arccosine of the dot product produces the angle
     *      in radian's of the view dir away from the "pole".
     *   3. To get angle away from the "equator", subtract the angle
     *      computed in step 2 from PI/2.
     */

    canvas->priv->view.altaz.alt = _PIover2 - acos(canvas->priv->view.altaz.up->dot(*canvas->priv->view.altaz.dir));

    /* Next, compute a default direction vector that is at the
     * "equator" (PI/2 radians away from the "pole").
     */

    vector3d cross = canvas->priv->view.altaz.dir->cross(*canvas->priv->view.altaz.up);
    *canvas->priv->view.altaz.dir = canvas->priv->view.altaz.up->cross(cross);

    /* Our default azimuth angle is initially zero. */
    canvas->priv->view.altaz.az = 0.0;
}

static void
_gtk_rtg_canvas_camera_altaz_rotate(GtkRtgCanvas *canvas, gdouble mx, gdouble my)
{
    gdouble delta_x = mx - canvas->priv->view.altaz.mx;
    gdouble delta_y = my - canvas->priv->view.altaz.my;

    _gtk_rtg_canvas_camera_altaz_update_mxmy(canvas, mx, my);

    /* Turn the deltas into radians. Multiplying by 0.01 means,
     * for example, that moving the mouse 314 pixels will
     * rotate by (approximately) PI radians.
     */


    /*
     * Fine tune the delta motion to minimize 'jumps' on
     * big motion differences between axis movements.
     *
     * - make verticals moves a bit slower than horizontal's.
     */

    if(delta_x != 0.0)
    {
        gdouble ratio_x = abs(delta_y / delta_x);

        if(ratio_x < 3.0)
            delta_x *= -0.0065;
        else
            delta_x *= -0.0001;
    }


    if(delta_y != 0.0)
    {
        gdouble ratio_y = abs(delta_x / delta_y);

        if(ratio_y < 30.0)
            delta_y *= -0.0055;
        else
            delta_y *= -0.0001;
    }


    /* Compute the new altitude angle. Clamp to avoid wrapping
     * around the up vector (or its negation).
     */

    canvas->priv->view.altaz.alt += delta_y;

    if(canvas->priv->view.altaz.alt > _PIover2)
        canvas->priv->view.altaz.alt = _PIover2 - 0.1e-5;
     else
    if(canvas->priv->view.altaz.alt < -_PIover2)
        canvas->priv->view.altaz.alt = -_PIover2 + 0.1e-5;

    /* Compute the new azimuth angle in the range 0 < az < 2*PI. */
    canvas->priv->view.altaz.az += delta_x;

    while(canvas->priv->view.altaz.az >= _2PI)
        canvas->priv->view.altaz.az -= _2PI;

    while(canvas->priv->view.altaz.az < 0.0)
        canvas->priv->view.altaz.az += _2PI;

    canvas->priv->view.altaz.az -= 0.1e-5;

    /* Compute the view direction vector by starting with the
     * original direction vector, and rotating it by our
     * updated altitude and azimuth angles.
     */

    vector3d cross = canvas->priv->view.altaz.dir->cross(*canvas->priv->view.altaz.up);

    vector3d dir = *canvas->priv->view.altaz.dir;
    dir.rot( cross.norm(), canvas->priv->view.altaz.alt );
    dir.rot( *canvas->priv->view.altaz.up, canvas->priv->view.altaz.az );
    dir.norm();

    /*
     * Update direction vector.
     */

    vector3d *_dir;
    g_object_get(canvas->priv->view.cam, "view-direction", &_dir, NULL);
    *_dir = dir;

    /*
     * Update an eye position.
     */

    dir *= -gtk_rtg_camera_view_get_distance(canvas->priv->view.cam);
    dir += *gtk_rtg_camera_view_get_center(canvas->priv->view.cam);
    g_object_get(canvas->priv->view.cam, "view-eye", &_dir, NULL);
    *_dir = dir;

    /* recreate view matrix */
    gtk_rtg_camera_view_do_matrix(canvas->priv->view.cam);

    /* update view state */
    if(canvas->priv->view.predefined != GTK_RTG_CANVAS_VIEW_CUSTOM)
    {
        canvas->priv->view.predefined = GTK_RTG_CANVAS_VIEW_CUSTOM;
        g_object_set(canvas->priv->view.text, "string", p_view_text[canvas->priv->view.predefined], NULL);

        /* recreate 'projection' matrix for orthographic proj. */
        GtkRtgCamProj *projection;
        g_object_get(canvas->priv->view.cam, "proj-type", &projection, NULL);

        if(*projection == GTK_RTG_CAM_PROJ_ORTHO)
        {
            /*
             * Reset gizmo's fallback selection.
             */

            if(SELECTED(canvas->priv->scene))
            {
                canvas->priv->view.gizmo_move->fallback   = 1;
                canvas->priv->view.gizmo_scale->fallback  = 1;
                canvas->priv->view.gizmo_rotate->fallback = 1;
            }

            *projection = GTK_RTG_CAM_PROJ_PERSP;
            gtk_rtg_camera_projection_do_matrix(canvas->priv->view.cam);
        }
    }
}


static void
_gtk_rtg_canvas_camera_altaz_zoom(GtkRtgCanvas *canvas, gdouble my)
{
    const gdouble delta_y = my - canvas->priv->view.altaz.depth;

    /* save depth */
    canvas->priv->view.altaz.depth = my;

    /*  Change the view distance as a function of the current
     *  distance. This creates larger movement further away,
     *  and less movement as the eye distance shrinks.
     */

    gdouble distance;

    /* update 'distance' */
    g_object_get(canvas->priv->view.cam, "view-distance", &distance, NULL);

    if(delta_y < 250.0)
    {
        distance = distance * (1.0 - (delta_y / 250.0));

        g_object_set(canvas->priv->view.cam, "view-distance", distance, NULL);

        /* recreate view matrix */
        gtk_rtg_camera_view_do_matrix(canvas->priv->view.cam);

        /* set cursor */
        _gtk_rtg_canvas_set_cursor(canvas, CURS_VIEW_ZOOM);
    }
}


static void
_gtk_rtg_canvas_camera_altaz_pan(GtkRtgCanvas *canvas, gdouble mx, gdouble my)
{

    /* - Pan view is achieved in a way that views center is
     *   translated as a point lying in a plane defined by p, s and w vectors.
     *
     *   r = p + sv + tw	(any point in a plane)
     *
     *   where: p - views origin (0,0,0)
     *	    v - vector orthogonal to the direction and up vector
     *	    w - vector orthogonal to the direction and v vector (real up vector)
     *	    s - delta x motion
     *	    w - delta y motion
     */

    gdouble delta_x = canvas->priv->view.altaz.mx - mx;
    gdouble delta_y = canvas->priv->view.altaz.my - my;

    _gtk_rtg_canvas_camera_altaz_update_mxmy(canvas, mx, my);

    vector3d *dir;
    vector3d *center;
    vector3d *up;
    GLdouble  distance;

    g_object_get(canvas->priv->view.cam, "view-direction",	&dir,
                                         "view-center",		&center,
                                         "view-up",			&up,
                                         "view-distance",	&distance, NULL);

    /* approx. fine tuning (on smaller distances performs smaller moves) */
    gdouble tune = distance / 3001.0; /* magic number */

    /* fine tune motion */
    delta_x *= tune;	/* 0.002; */
    delta_y *= tune;	/* 0.002; */

    vector3d v = dir->cross(*up).norm();
    vector3d w = dir->cross(v).norm();

    v *= delta_x;
    w *= delta_y;

    /*
     * translate in a plane (set new views center)
     */
    *center += (v + w);

    /* recalculate an eye position */
    gtk_rtg_camera_view_set_center(canvas->priv->view.cam, center);

    /* recreate view matrix */
    gtk_rtg_camera_view_do_matrix(canvas->priv->view.cam);

    /* set cursor */
    _gtk_rtg_canvas_set_cursor(canvas, CURS_MOVE);
}



static void
_gtk_rtg_canvas_camera_altaz_gizmo_init(GtkRtgCanvas *canvas)
{
    /*
     * init view-gizmo
     *
     */

    canvas->priv->view.gizmo.width = 13.0;
    canvas->priv->view.gizmo.height = 13.0;
    canvas->priv->view.gizmo.width_2 = 6.0;
    canvas->priv->view.gizmo.height_2 = 6.0;
    canvas->priv->view.gizmo.theta = gfloat(_PI) / (VIEW_GIZMO_POINTS >> 1);

    gdk_color_parse("#dcca00", &canvas->priv->view.gizmo.color);

    gint step = VIEW_GIZMO_POINTS / VIEW_GIZMO_RECTS;
    gint i = 0;

    /* inti pointers */
    for(; i<VIEW_GIZMO_RECTS; i++)
        canvas->priv->view.gizmo.rect[i] = &canvas->priv->view.gizmo.point[step*i];
}


static void
_gtk_rtg_canvas_camera_altaz_gizmo_update(GtkRtgCanvas *canvas)
{
    guint i = 0;
    gfloat e_angle = 0.f;
    GLfloat cx = (GLfloat)(gtk_widget_get_allocated_width(GTK_WIDGET(canvas))  >> 1);
    GLfloat cy = (GLfloat)(gtk_widget_get_allocated_height(GTK_WIDGET(canvas)) >> 1);

    canvas->priv->view.gizmo.radius = cx < cy ? cx * VIEW_GIZMO_SCALE
                                              : cy * VIEW_GIZMO_SCALE;

    /* recalculate gizmos points */
    for(; i<VIEW_GIZMO_POINTS; i++)
    {
        X(canvas->priv->view.gizmo.point[i]) = floor(cx - cartesian_px(canvas->priv->view.gizmo.radius, e_angle));
        Y(canvas->priv->view.gizmo.point[i]) = floor(cy - cartesian_py(canvas->priv->view.gizmo.radius, e_angle));

        e_angle += canvas->priv->view.gizmo.theta;
    }
}


/***
 *** This function checks mouse position against
 *** view-gizmo's rect.
 *** Return true if mice pos. is in rect, otherwise
 *** it returns false.
 ***/
static gboolean
_gtk_rtg_canvas_camera_altaz_gizmo_r_mice(GtkRtgCanvas *canvas, point2f *point, GdkEventMotion *event)
{
    point2f rect = *point;

    X(rect) -= canvas->priv->view.gizmo.width_2;
    Y(rect) += canvas->priv->view.gizmo.height_2;

    if(    (event->x > X(rect) && event->x < (X(rect) + canvas->priv->view.gizmo.width))
        && (event->y < Y(rect) && event->y > (Y(rect) - canvas->priv->view.gizmo.height))
      )
        return TRUE;

    return FALSE;
}


/***
 *** This function render text(s) (ortho) from the render map.
 ***
 ***/
static void
_gtk_rtg_canvas_render_text(GtkRtgCanvas *canvas)
{
    for(m_txtType::const_iterator it = canvas->priv->m_text->begin(); it != canvas->priv->m_text->end(); ++it)

        if(it->second != canvas->priv->view.text && gtk_rtg_text_is_visible(it->second))
        {
            GdkColor *color;
            GdkPoint *position;
            gboolean  pos_flipped;

            /* query text info */
            g_object_get(G_OBJECT(it->second), "color",	  	  &color,
                                               "position",    &position,
                                               "pos-flipped", &pos_flipped, NULL);
            /* set font color */
            glColor3usv(&color->red);

            /* position raster */
            pos_flipped ? glRasterPos2i(position->x, gtk_widget_get_allocated_height(GTK_WIDGET(canvas)) - position->y)
                        : glRasterPos2i(position->x, position->y);

            /* get font-base display list */
            gint *base = (gint*)g_object_get_data(G_OBJECT(it->second), "font-base");

            /* display list to be used */
            glListBase(*base);

            /* get string */
            gchar *string = gtk_rtg_text_get_string(it->second);

            /* execute list */
            glCallLists(strlen(string), GL_UNSIGNED_BYTE, string);
        }
}


static void
_gtk_rtg_canvas_camera_altaz_render_gizmo(GtkRtgCanvas *canvas)
{
    guint i = 0;

    /*
     * Set color
     *
     **/

    glColor3usv(&canvas->priv->view.gizmo.color.red);

    /*
     * Render circle
     *
     **/

    glBegin(GL_LINE_LOOP);
    for(; i<VIEW_GIZMO_POINTS; i++)
        glVertex2fv(VALUES(canvas->priv->view.gizmo.point[i]));
    glEnd();

    /*
     * Render rects with 'x' inside of
     *
     **/

    for(i=0; i<VIEW_GIZMO_RECTS; i++)
    {
        vector2f point = *canvas->priv->view.gizmo.rect[i];

        X(point) -= canvas->priv->view.gizmo.width_2;
        Y(point) += canvas->priv->view.gizmo.height_2;

        /* rect */
        glBegin(GL_LINE_LOOP);
        glVertex2fv(VALUES(point));
        glVertex2f(X(point), Y(point) - canvas->priv->view.gizmo.height);
        glVertex2f(X(point) + canvas->priv->view.gizmo.width, Y(point) - canvas->priv->view.gizmo.height);
        glVertex2f(X(point) + canvas->priv->view.gizmo.width, Y(point));
        glEnd();

        /* 'x' */
        glBegin(GL_LINES);

        glVertex2f(X(point) + 2.f,                                     Y(point) - 2.f);
        glVertex2f(X(point) + canvas->priv->view.gizmo.width - 2.f,    Y(point) - canvas->priv->view.gizmo.height + 2.f);
        glVertex2f(X(point) + 2.f,                                     Y(point) - canvas->priv->view.gizmo.height + 2.f);
        glVertex2f(X(point) + canvas->priv->view.gizmo.width - 2.f,    Y(point) - 2.f);

        glEnd();
    }
}


static void
_gtk_rtg_canvas_render_border(GtkRtgCanvas *canvas, GdkColor &color)
{
    GLfloat	 line_width = 0.0f;

    /* preserve current line width */
    glGetFloatv(GL_LINE_WIDTH, &line_width);

    glLineWidth(3.5f);
    glColor3usv(&color.red);

    glBegin(GL_LINE_STRIP);

    glVertex2f(0.f, 0.f);
    glVertex2f(0.f, (gfloat)gtk_widget_get_allocated_height(GTK_WIDGET(canvas)));
    glVertex2f((gfloat)gtk_widget_get_allocated_width(GTK_WIDGET(canvas)),
               (gfloat)gtk_widget_get_allocated_height(GTK_WIDGET(canvas)));
    glVertex2f((gfloat)gtk_widget_get_allocated_width(GTK_WIDGET(canvas)), 0.f);
    glVertex2f(0.f, 0.f);

    glEnd();
    glLineWidth(line_width);
}


/*
 * Create and push views text used for 'predefined' view
 * description.
 */
static void
_gtk_rtg_canvas_camera_view_push_text(GtkRtgCanvas *canvas)
{
    canvas->priv->view.text = gtk_rtg_text_new();

    GdkColor color;
    GdkPoint position;

    gdk_color_parse("#eef2f6", &color);

    position.x = 9;
    position.y = 24;

    gchar *font_desc = g_strdup("Sans Bold 12");
    g_object_set(canvas->priv->view.text, "font-desc",   font_desc,
                                          "color",	     &color,
                                          "position",    &position,
                                          "string",       "",
                                          "pos-flipped",  TRUE, NULL);
    g_free(font_desc);
    gtk_rtg_canvas_text_push(canvas, canvas->priv->view.text);
}


static void
_gtk_rtg_canvas_render_marquee_selection(GtkRtgCanvas *canvas)
{
    /* enable stipple */
    gboolean stipple = glIsEnabled(GL_LINE_STIPPLE);
    if(!stipple) glEnable(GL_LINE_STIPPLE);

    gfloat line_width;
    glGetFloatv(GL_LINE_WIDTH, &line_width);

    /* set factor & pattern */
    glLineStipple(1, 0x1111);

    glColor3usv(&canvas->priv->marquee.color.red);

    glLineWidth(.5f);

    glBegin(GL_LINE_LOOP);

    glVertex2i(canvas->priv->marquee.start.x, canvas->priv->marquee.start.y);
    glVertex2i(canvas->priv->marquee.end.x,	  canvas->priv->marquee.start.y);
    glVertex2i(canvas->priv->marquee.end.x,	  canvas->priv->marquee.end.y);
    glVertex2i(canvas->priv->marquee.start.x, canvas->priv->marquee.end.y);

    glEnd();

    if(!stipple)
        glDisable(GL_LINE_STIPPLE);

    glLineWidth(line_width);
}


inline static void
_gtk_rtg_canvas_realloc_picking_buffer(GtkRtgCanvas *canvas, gint width, gint height)
{
    if(canvas->priv->view.pickbuff.buff == NULL)
    {
        canvas->priv->view.pickbuff.buff = (GLubyte*) g_malloc0(width*height*4);
        canvas->priv->view.pickbuff.width = width;
        canvas->priv->view.pickbuff.height = height;
        canvas->priv->view.pickbuff.update = 1;
    }else
    if(canvas->priv->view.pickbuff.width != width || canvas->priv->view.pickbuff.height != height)
    {
        GLubyte *new_sbuffer = (GLubyte*) g_realloc(canvas->priv->view.pickbuff.buff, width*height*4);

        if(new_sbuffer)
        {
            canvas->priv->view.pickbuff.buff = new_sbuffer;
            canvas->priv->view.pickbuff.width = width;
            canvas->priv->view.pickbuff.height = height;
            canvas->priv->view.pickbuff.update = 1;
        }
    }
}


void
_gtk_rtg_canvas_text_push_real(GtkRtgCanvas *canvas, GtkRtgText *text, m_txtType *m_text)
{
    gchar *canvas_obj_id = NULL;

    if(m_text)
    {
        /* check whether text is already pushed */
        canvas_obj_id = (gchar*) g_object_get_data(G_OBJECT(text), canvas->priv->id);
        g_throw_if_fail(canvas_obj_id == NULL);
    }

    /* get font key {font_desc|start_char|chars_count} */
    gchar *key = (gchar*) g_object_get_data(G_OBJECT(text), "key");

    /* get font-base for the canvas */
    m_fbType* fb = gtk_rtg_resource_get_font_map(canvas);
    if(fb) /* font-base exists (at least one font is already registered for the canvas) */
    {
        /* is font already exists? */
        m_fbType::iterator iter = fb->find(key);
        if(iter != fb->end())
        {
            /*
             * Font is already registered (associate font-base with the text object)
             *
             * - update the font base (push text).
             *
             **/

            gint *base = (gint*)g_object_get_data(G_OBJECT(text), "font-base");
            *base = iter->second->list_base;
        }else
            goto new_font;
    }else
    {
        new_font:

        /*
         * Register new font (type)
         *
         * - update the font base (push text).
         *
         **/

        gint *base = (gint*)g_object_get_data(G_OBJECT(text), "font-base");
        *base = gtk_rtg_resource_font_register(canvas, text, key);
    }

    if(m_text)
    {
        /*
         * Actual push to the render list
         *
         **/

        /* get (free) obj-id */
        canvas_obj_id = _gtk_rtg_canvas_create_object_id(canvas);

        /* set to the object */
        g_object_set_data(G_OBJECT(text), canvas->priv->id,   canvas_obj_id);

        /* add to the render list */
        m_text->insert(make_pair(canvas_obj_id, text));

        /* incref text */
        g_object_ref(text);

        /* set flag to display text  */
        if(!(canvas->priv->display & DISPLAY_TEXT))
            canvas->priv->display |= DISPLAY_TEXT;
    }
}

static void
_gtk_rtg_canvas_primitives_get_pmin_pmax(GtkRtgCanvas                 *canvas,
                                         const set<GtkRtgPrimitive *> *prims,
                                         point3f                      &pmin,
                                         point3f                      &pmax    )
{
    /*
     * Find (minimal) pmin and pmax around scene objects
     * - test bbox's vertices
     *
     */
    pmin = point3f( G_MAXFLOAT,  G_MAXFLOAT,  G_MAXFLOAT);
    pmax = point3f(-G_MAXFLOAT, -G_MAXFLOAT, -G_MAXFLOAT);

    for(set<GtkRtgPrimitive*>::const_iterator it=prims->begin(); it!=prims->end(); ++it)
    {
        if(!GTK_RTG_IS_CAMERA(*it) && gtk_rtg_primitive_is_visible(*it) && !VERTS(*it)->empty())
        {
            for(guint i=0; i<BBOX_CENTER; ++i)
            {
                vector3f v = object2world(*it, BBOX(*it)->p[i]);

                /* get min point */
                if(X(v) < X(pmin)) X(pmin) = X(v);
                if(Y(v) < Y(pmin)) Y(pmin) = Y(v);
                if(Z(v) < Z(pmin)) Z(pmin) = Z(v);

                /* get max point */
                if(X(v) > X(pmax)) X(pmax) = X(v);
                if(Y(v) > Y(pmax)) Y(pmax) = Y(v);
                if(Z(v) > Z(pmax)) Z(pmax) = Z(v);
            }
        }
    }
}
