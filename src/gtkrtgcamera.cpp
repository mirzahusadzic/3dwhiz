#include <gtkrtg/gtkrtgcamera.h>
#include <gtkrtg/gtkrtgcanvas.h>
#include "gtkrtg_gl_specs.h"
#include "gtkrtgextras.h"
#include "gtkrtgglu.h"

#define GTK_RTG_CAMERA_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_RTG_TYPE_CAMERA, GtkRtgCameraPrivate))

G_DEFINE_TYPE(GtkRtgCamera, gtk_rtg_camera, GTK_RTG_TYPE_PRIMITIVE)

using namespace gtkrtg;

enum{ CAM_VIEW = 0x02, CAM_PROJ = 0x04, CAM_INIT = 0x06 };

struct _GtkRtgCameraPrivate
{
    guint init;

    /* view */
    matrix4x4d  mview;
    matrix4x4f  mviewf;
    point3d    *center;
    vector3d   *direction;
    vector3d   *up;
    point3d    *eye;
    GLdouble    distance;

    /* projection */
    matrix4x4f    mproj;
    GLfloat       fov;
    GLfloat       aspect;
    GLfloat       znear;
    GLfloat       zfar;
    GtkRtgCamProj projection;

    /* pre-multiplied matrices */
    gtkrtg::matrix4x4f mprojview;      /* mview * mproj */
    gtkrtg::matrix4x4f minverse;       /* inverse of mprojview */
    gboolean           calc_mprojview;
    gboolean           calc_minverse;

    /* frustum */
    GtkRtgCamFrustum frustum;
};

static void	gtk_rtg_camera_finalize	   (GObject *object);
static void gtk_rtg_camera_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gtk_rtg_camera_get_property(GObject *object, guint prop_id, GValue       *value, GParamSpec *pspec);

/* properties */
enum
{
    PROP_0,
    PROP_VIEW_CENTER,
    PROP_VIEW_DIR,
    PROP_VIEW_UP,
    PROP_VIEW_EYE,
    PROP_VIEW_DISTANCE,
    PROP_VIEW_MATRIX,
    PROP_PROJ_FOV,
    PROP_PROJ_ASPECT,
    PROP_PROJ_ZNEAR,
    PROP_PROJ_ZFAR,
    PROP_PROJ_TYPE,
    PROP_PROJ_MATRIX,
    LAST_ARG
};

enum
{
    S_PROP_VIEW_CENTER,
    S_PROP_VIEW_DIR,
    S_PROP_VIEW_UP,
    S_PROP_VIEW_EYE,
    S_PROP_VIEW_DISTANCE,
    S_PROP_VIEW_MATRIX,
    S_PROP_PROJ_FOV,
    S_PROP_PROJ_ASPECT,
    S_PROP_PROJ_ZNEAR,
    S_PROP_PROJ_ZFAR,
    S_PROP_PROJ_TYPE,
    S_PROP_PROJ_MATRIX,
    S_LAST_PROP
};

static const gchar *prop[S_LAST_PROP]={"view-center",
                                       "view-direction",
                                       "view-up",
                                       "view-eye",
                                       "view-distance",
                                       "view-matrix",
                                       "proj-fov",
                                       "proj-aspect",
                                       "proj-znear",
                                       "proj-zfar",
                                       "proj-type",
                                       "proj-matrix"	};
/* signals */
enum
{
    CAMERA_PRIORITIZE,
    LAST_SIGNAL
};

static  gint signals[LAST_SIGNAL] = { 0 };

static void
gtk_rtg_camera_class_init(GtkRtgCameraClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;

    object_class->set_property  = gtk_rtg_camera_set_property;
    object_class->get_property  = gtk_rtg_camera_get_property;
    object_class->finalize	    = gtk_rtg_camera_finalize;

    /*
     * GtkRtgCamera: view-center
     *
     * A point at which viewer is directed to look at.
     *
     */
    g_object_class_install_property(object_class,
                                    PROP_VIEW_CENTER,
                                    g_param_spec_pointer(prop[S_PROP_VIEW_CENTER],
                                                         prop[S_PROP_VIEW_CENTER],
                                                         "A point at which viewer is directed to look at.",
                                                         GTK_RTG_PARAM_READWRITE));
    /*
     * GtkRtgCamera: view-direction
     *
     * The direction vector in 3D space from which a viewer is oriented to the center point.
     *
     */
    g_object_class_install_property(object_class,
                                    PROP_VIEW_DIR,
                                    g_param_spec_pointer(prop[S_PROP_VIEW_DIR],
                                                         prop[S_PROP_VIEW_DIR],
                                                         "The direction vector in 3D space from which a viewer is oriented to the center point.",
                                                         GTK_RTG_PARAM_READWRITE));
    /*
     * GtkRtgCamera: view-up
     *
     * The vector which determines a viewer up direction.
     *
     */
    g_object_class_install_property(object_class,
                                    PROP_VIEW_UP,
                                    g_param_spec_pointer(prop[S_PROP_VIEW_UP],
                                                         prop[S_PROP_VIEW_UP],
                                                         "The vector which determines a viewer up direction.",
                                                         GTK_RTG_PARAM_READWRITE));
    /*
     * GtkRtgCamera: view-eye
     *
     * A viewer position in 3D space.
     *
     */

    g_object_class_install_property(object_class,
                                    PROP_VIEW_EYE,
                                    g_param_spec_pointer(prop[S_PROP_VIEW_EYE],
                                                         prop[S_PROP_VIEW_EYE],
                                                         "A viewer position in 3D space.",
                                                         GTK_RTG_PARAM_READWRITE));
    /*
     * GtkRtgCamera: view-dist
     *
     * The distance from the viewer to the point he looking at.
     *
     */
    g_object_class_install_property(object_class,
                                    PROP_VIEW_DISTANCE,
                                    g_param_spec_double(prop[S_PROP_VIEW_DISTANCE],
                                                        prop[S_PROP_VIEW_DISTANCE],
                                                        "The distance from the viewer to the point he looking at.",
                                                        0.0,
                                                        G_MAXDOUBLE,
                                                        10.0,
                                                        GTK_RTG_PARAM_READWRITE));
    /*
     * GtkRtgCamera: view-matrix
     *
     * A view matrix.
     *
     */
    g_object_class_install_property(object_class,
                                    PROP_VIEW_MATRIX,
                                    g_param_spec_pointer(prop[S_PROP_VIEW_MATRIX],
                                                         prop[S_PROP_VIEW_MATRIX],
                                                         "A view matrix.",
                                                         GTK_RTG_PARAM_READWRITE));
    /*
     * GtkRtgCamera: proj-fov
     *
     * A field of view angle, in degrees, in y direction.
     *
     */
    g_object_class_install_property(object_class,
                                    PROP_PROJ_FOV,
                                    g_param_spec_float(prop[S_PROP_PROJ_FOV],
                                                       prop[S_PROP_PROJ_FOV],
                                                       "A field of view angle, in degrees, in y direction.",
                                                       0.f,
                                                       150.f,
                                                       30.f,
                                                       GTK_RTG_PARAM_READWRITE));
    /*
     * GtkRtgCamera: proj-aspect
     *
     * The aspect ratio that determines a field of view in the x-direction.
     *
     */

    g_object_class_install_property(object_class,
                                    PROP_PROJ_ASPECT,
                                    g_param_spec_float(prop[S_PROP_PROJ_ASPECT],
                                                       prop[S_PROP_PROJ_ASPECT],
                                                       "The aspect ratio that determines a field of view in the x-direction.",
                                                       0.f,
                                                       G_MAXFLOAT,
                                                       1.33f,
                                                       GTK_RTG_PARAM_READWRITE));
    /*
     * GtkRtgCamera: proj-znear
     *
     * The distance from the viewer to the near clipping plane.
     *
     */
    g_object_class_install_property(object_class,
                                    PROP_PROJ_ZNEAR,
                                    g_param_spec_float(prop[S_PROP_PROJ_ZNEAR],
                                                       prop[S_PROP_PROJ_ZNEAR],
                                                       "The distance from the viewer to the near clipping plane.",
                                                       0.f,
                                                       G_MAXFLOAT,
                                                       1.f,
                                                       GTK_RTG_PARAM_READWRITE));
    /*
     * GtkRtgCamera: proj-zfar
     *
     * The distance from the viewer to the near clipping plane.
     *
     *
     */
    g_object_class_install_property(object_class,
                                    PROP_PROJ_ZFAR,
                                    g_param_spec_float(prop[S_PROP_PROJ_ZFAR],
                                                      prop[S_PROP_PROJ_ZFAR],
                                                      "The distance from the viewer to the near clipping plane.",
                                                      1.f,
                                                      G_MAXFLOAT,
                                                      1000.f,
                                                      GTK_RTG_PARAM_READWRITE));
    /*
     * GtkRtgCamera: proj-type
     *
     * The projection type.
     *
     *
     */
    g_object_class_install_property(object_class,
                                    PROP_PROJ_TYPE,
                                    g_param_spec_pointer(prop[S_PROP_PROJ_TYPE],
                                                         prop[S_PROP_PROJ_TYPE],
                                                         "The projection type.",
                                                         GTK_RTG_PARAM_READWRITE));
    /*
     * GtkRtgCamera: proj-matrix
     *
     * The projection matrix.
     *
     */
    g_object_class_install_property(object_class,
                                    PROP_PROJ_MATRIX,
                                    g_param_spec_pointer(prop[S_PROP_PROJ_MATRIX],
                                                         prop[S_PROP_PROJ_MATRIX],
                                                         "The projection matrix.",
                                                         GTK_RTG_PARAM_READWRITE));
    /*
     * Signal(s)
     *
     *
     */

    /*
     * GtkRtgCamera: prioritize
     *
     * A camera emits this signal when it gets prioritized.
     *
     */
    signals[CAMERA_PRIORITIZE] = g_signal_new("prioritize",
                                              G_OBJECT_CLASS_TYPE(klass),
                                              (GSignalFlags)G_SIGNAL_RUN_FIRST,
                                              G_STRUCT_OFFSET(GtkRtgCameraClass, prioritize),
                                              NULL,
                                              NULL,
                                              g_cclosure_marshal_VOID__OBJECT,
                                              G_TYPE_NONE,
                                              1,
                                              G_TYPE_OBJECT);

    g_type_class_add_private(G_OBJECT_CLASS(klass), sizeof(GtkRtgCameraPrivate));
}


GtkRtgPrimitive *
gtk_rtg_camera_new()
{
    return GTK_RTG_PRIMITIVE(g_object_new(GTK_RTG_TYPE_CAMERA, NULL));
}


static void
gtk_rtg_camera_init(GtkRtgCamera *camera)
{
    camera->priv = GTK_RTG_CAMERA_GET_PRIVATE(camera);

    /* force heap allocation to invoke constructors */
    camera->priv->center = new point3d();
    camera->priv->direction	= new vector3d(-1.0, -1.0, -1.0);
    camera->priv->up = new vector3d(0.0,  1.0,  0.0);
    camera->priv->eye = new point3d(-10.0,-10.0,-10.0);
    camera->priv->distance = 10.0;

    camera->priv->direction->norm();
    camera->priv->up->norm();

    /* normalize */
    camera->priv->direction->norm();
    camera->priv->up->norm();

    /* projection */
    camera->priv->fov = 30.f;
    camera->priv->aspect = 1.33f;
    camera->priv->znear = 1.f;
    camera->priv->zfar = 1000.f;
    camera->priv->projection = GTK_RTG_CAM_PROJ_PERSP;

    /* init to identity */
    camera->priv->mview.identity(); camera->priv->mproj.identity();
}


static void
gtk_rtg_camera_finalize(GObject *object)
{
    GtkRtgCamera *camera = GTK_RTG_CAMERA(object);

    if(camera->priv->center)    { delete camera->priv->center;		camera->priv->center	= NULL; }
    if(camera->priv->direction) { delete camera->priv->direction;	camera->priv->direction = NULL; }
    if(camera->priv->up)	    { delete camera->priv->up;			camera->priv->up		= NULL; }
    if(camera->priv->eye)	    { delete camera->priv->eye;			camera->priv->eye		= NULL; }

    G_OBJECT_CLASS(gtk_rtg_camera_parent_class)->finalize(object);
}


static void
gtk_rtg_camera_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GtkRtgCamera *camera = GTK_RTG_CAMERA(object);

    switch(prop_id)
    {
        case PROP_VIEW_CENTER:	 gtk_rtg_camera_view_set_center(camera, (vector3d*)(g_value_get_pointer(value))); break;
        case PROP_VIEW_DIR:	 	 gtk_rtg_camera_view_set_direction(camera, (vector3d*)(g_value_get_pointer(value))); break;
        case PROP_VIEW_UP:	 	 gtk_rtg_camera_view_set_up(camera, (vector3d*)(g_value_get_pointer(value))); break;
        case PROP_VIEW_EYE:	 	 gtk_rtg_camera_view_set_eye(camera, (vector3d*)(g_value_get_pointer(value))); break;
        case PROP_VIEW_DISTANCE: gtk_rtg_camera_view_set_distance(camera, g_value_get_double(value)); break;
        case PROP_VIEW_MATRIX:	 gtk_rtg_camera_view_set_matrix(camera, (matrix4x4f*)(g_value_get_pointer(value))); break;
        case PROP_PROJ_FOV:	 	 gtk_rtg_camera_projection_set_fov(camera, g_value_get_float(value)); break;
        case PROP_PROJ_ASPECT:	 gtk_rtg_camera_projection_set_aspect(camera, g_value_get_float(value)); break;
        case PROP_PROJ_ZNEAR:	 gtk_rtg_camera_projection_set_znear(camera, g_value_get_float(value)); break;
        case PROP_PROJ_ZFAR:	 gtk_rtg_camera_projection_set_zfar(camera, g_value_get_float(value)); break;
        case PROP_PROJ_TYPE:	 gtk_rtg_camera_set_projection(camera, (*(GtkRtgCamProj*)(g_value_get_pointer(value)))); break;
        case PROP_PROJ_MATRIX:	 gtk_rtg_camera_projection_set_matrix(camera, (matrix4x4f*)(g_value_get_pointer(value))); break;
    };

}

static void
gtk_rtg_camera_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GtkRtgCamera *camera = GTK_RTG_CAMERA(object);

    switch(prop_id)
    {				 /* getters without extra function calls */

        case PROP_VIEW_CENTER:	 g_value_set_pointer(value, camera->priv->center);	break;
        case PROP_VIEW_DIR:	 	 g_value_set_pointer(value, camera->priv->direction); break;
        case PROP_VIEW_UP:	 	 g_value_set_pointer(value, camera->priv->up); break;
        case PROP_VIEW_EYE:	 	 g_value_set_pointer(value, camera->priv->eye); break;
        case PROP_VIEW_DISTANCE: g_value_set_double(value,   camera->priv->distance); break;
        case PROP_VIEW_MATRIX:	 g_value_set_pointer(value, &camera->priv->mview); break;
        case PROP_PROJ_FOV:	 	 g_value_set_float(value, camera->priv->fov); break;
        case PROP_PROJ_ASPECT:	 g_value_set_float(value, camera->priv->aspect); break;
        case PROP_PROJ_ZNEAR:	 g_value_set_float(value, camera->priv->znear); break;
        case PROP_PROJ_ZFAR:	 g_value_set_float(value, camera->priv->zfar); break;
        case PROP_PROJ_TYPE:	 g_value_set_pointer(value, &camera->priv->projection); break;
        case PROP_PROJ_MATRIX:	 g_value_set_pointer(value, &camera->priv->mproj); break;
    };
}

/*
 * Publics
 *
 *
 */

void
gtk_rtg_camera_view_set_center(GtkRtgCamera *camera, const point3d *center) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    g_throw_if_fail(center != NULL);
    *camera->priv->center = *center;
    g_object_notify(G_OBJECT(camera), prop[S_PROP_VIEW_CENTER]);

    /*
     * Update an eye position.
     */

    *camera->priv->eye  = *camera->priv->direction;
    *camera->priv->eye *= -camera->priv->distance;
    *camera->priv->eye += *camera->priv->center;
    g_object_notify(G_OBJECT(camera), prop[S_PROP_VIEW_EYE]);
}

point3d	*
gtk_rtg_camera_view_get_center(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    return camera->priv->center;
}


void
gtk_rtg_camera_view_set_direction(GtkRtgCamera *camera, const vector3d *direction) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    g_throw_if_fail(direction != NULL);

    *camera->priv->direction = *direction;
    camera->priv->direction->norm();
    g_object_notify(G_OBJECT(camera), prop[S_PROP_VIEW_DIR]);

    /*
     * Update an eye position.
     */

    *camera->priv->eye  = *camera->priv->direction;
    *camera->priv->eye *= -camera->priv->distance;
    *camera->priv->eye += *camera->priv->center;
    g_object_notify(G_OBJECT(camera), prop[S_PROP_VIEW_EYE]);
}

vector3d *
gtk_rtg_camera_view_get_direction(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    return camera->priv->direction;
}


void
gtk_rtg_camera_view_set_up(GtkRtgCamera *camera, const vector3d *up) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    g_throw_if_fail(up != NULL);

    *camera->priv->up = *up;
    camera->priv->up->norm();

    g_object_notify(G_OBJECT(camera), prop[S_PROP_VIEW_UP]);
}


vector3d *
gtk_rtg_camera_view_get_up(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    return camera->priv->up;
}


void
gtk_rtg_camera_view_set_distance(GtkRtgCamera *camera, const GLdouble distance) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    distance < camera->priv->znear ? camera->priv->distance = camera->priv->znear : camera->priv->distance = distance;
    g_object_notify(G_OBJECT(camera), prop[S_PROP_VIEW_DISTANCE]);

    /*
     * Update an eye position.
     */

    *camera->priv->eye  = *camera->priv->direction;
    *camera->priv->eye *= -camera->priv->distance;
    *camera->priv->eye += *camera->priv->center;
    g_object_notify(G_OBJECT(camera), prop[S_PROP_VIEW_EYE]);
}


GLdouble
gtk_rtg_camera_view_get_distance(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    return camera->priv->distance;
}


void
gtk_rtg_camera_view_set_eye(GtkRtgCamera *camera, const point3d *eye) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    g_throw_if_fail(eye != NULL);
    *camera->priv->eye = *eye;
    g_object_notify(G_OBJECT(camera), prop[S_PROP_VIEW_EYE]);

    vector3d direction = *eye - *camera->priv->center;

    /* update distance */
    camera->priv->distance = direction.mag();
    g_object_notify(G_OBJECT(camera), prop[S_PROP_VIEW_DISTANCE]);

    /* update direction */
    *camera->priv->direction = direction.norm();
    g_object_notify(G_OBJECT(camera), prop[S_PROP_VIEW_DIR]);
}


point3d	*
gtk_rtg_camera_view_get_eye(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    return camera->priv->eye;
}


    void
gtk_rtg_camera_view_do_matrix(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));

    GLdouble  *m = camera->priv->mview.m;
    point3d&  eye = *camera->priv->eye;
    vector3d& dir = *camera->priv->direction;
    vector3d _cross = dir.cross(*camera->priv->up).norm();
    vector3d _up = _cross.cross(dir).norm();

      m[_11]= X(_cross); m[_21]= Y(_cross); m[_31]= Z(_cross); m[tx]=-(X(eye)*m[_11] + Y(eye)*m[_21] + Z(eye)*m[_31]);
    m[_12]= X(_up);	   m[_22]= Y(_up);    m[_32]= Z(_up);	 m[ty]=-(X(eye)*m[_12] + Y(eye)*m[_22] + Z(eye)*m[_32]);
    m[_13]=-X(dir);	   m[_23]=-Y(dir);    m[_33]=-Z(dir);    m[tz]=-(X(eye)*m[_13] + Y(eye)*m[_23] + Z(eye)*m[_33]);
    m[_14]= 0.f;	   m[_24]= 0.f;	      m[_34]= 0.f;	 m[tw]= 1.f;

    camera->priv->mviewf = cast4x4<float, double>(camera->priv->mview);

    if(!(camera->priv->init & CAM_VIEW))
        camera->priv->init |= CAM_VIEW;

    /* re-calc both */
    camera->priv->calc_mprojview = camera->priv->calc_minverse = TRUE;

    g_object_notify(G_OBJECT(camera), prop[S_PROP_VIEW_MATRIX]);
}

void
gtk_rtg_camera_view_set_matrix(GtkRtgCamera *camera, const matrix4x4f *matrix) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    g_throw_if_fail(matrix != NULL);

    camera->priv->mviewf = *matrix;
    camera->priv->mview = cast4x4<double, float>(camera->priv->mviewf);

    g_object_notify(G_OBJECT(camera), prop[S_PROP_VIEW_MATRIX]);
}

matrix4x4f &
gtk_rtg_camera_view_get_matrix(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    return camera->priv->mviewf;
}

void
gtk_rtg_camera_view_load_matrix(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));

    /* get current matrix mode */
    GLint matrix_mode = 0;
    glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);

    if(matrix_mode != GL_MODELVIEW)
    {
        /*
         * Matrix mode change is required
         */

        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixd(camera->priv->mview.m);
        glMatrixMode(matrix_mode);
    }else
        glLoadMatrixd(camera->priv->mview.m);
}

void
gtk_rtg_camera_view_push_matrix(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));

    GLint view_stack_depth = 0;
    GLint max_view_stack_depth = 0;

    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH,	    &view_stack_depth);
    glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH, &max_view_stack_depth);

    /* can model-view matrix stack be pushed? */
    g_throw_if_fail(view_stack_depth < max_view_stack_depth);

    /* get current matrix mode */
    GLint matrix_mode = 0;
    glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);

    if(matrix_mode != GL_MODELVIEW)
    {
        /*
         * Matrix mode change is required
         */

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadMatrixd(camera->priv->mview.m);
        glMatrixMode(matrix_mode);
    }else
    {
        glPushMatrix();
        glLoadMatrixd(camera->priv->mview.m);
    }
}


void
gtk_rtg_camera_view_pop_matrix(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    GLint view_stack_depth = 0;

    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &view_stack_depth);

    /* can model-view matrix stack be popped? */
    g_throw_if_fail(view_stack_depth > 1);

    /* get current matrix mode */
    GLint matrix_mode = 0;
    glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);

    if(matrix_mode != GL_MODELVIEW)
    {
        /*
         * Matrix mode change is required
         */

        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glMatrixMode(matrix_mode);
    }else
        glPopMatrix();
}


void
gtk_rtg_camera_projection_set_fov(GtkRtgCamera *camera, const GLfloat fov) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));

    camera->priv->fov = fov;

    g_object_notify(G_OBJECT(camera), prop[S_PROP_PROJ_FOV]);
}


GLfloat
gtk_rtg_camera_projection_get_fov(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    return camera->priv->fov;
}


void
gtk_rtg_camera_projection_set_aspect(GtkRtgCamera *camera, const GLfloat aspect) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    g_throw_if_fail(aspect > 0.f);
    camera->priv->aspect = aspect;

    g_object_notify(G_OBJECT(camera), prop[S_PROP_PROJ_ASPECT]);
}


GLfloat
gtk_rtg_camera_projection_get_aspect(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    return camera->priv->aspect;
}


void
gtk_rtg_camera_projection_set_znear(GtkRtgCamera *camera, const GLfloat znear) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    g_throw_if_fail(znear >= 0.f);

    camera->priv->znear = znear;

    g_object_notify(G_OBJECT(camera), prop[S_PROP_PROJ_ZNEAR]);
}


GLfloat
gtk_rtg_camera_projection_get_znear(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    return camera->priv->znear;
}


void
gtk_rtg_camera_projection_set_zfar(GtkRtgCamera *camera, const GLfloat zfar) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    g_throw_if_fail(zfar >= 0.f);

    camera->priv->zfar = zfar;

    g_object_notify(G_OBJECT(camera), prop[S_PROP_PROJ_ZFAR]);
}


GLfloat
gtk_rtg_camera_projection_get_zfar(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    return camera->priv->zfar;
}


void
gtk_rtg_camera_projection_do_matrix(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));

    GLint proj_stack_depth = 0;
    GLint max_proj_stack_depth = 0;

    glGetIntegerv(GL_PROJECTION_STACK_DEPTH,     &proj_stack_depth);
    glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH, &max_proj_stack_depth);

    /* can projection matrix stack be pushed any more? */
    g_throw_if_fail(proj_stack_depth < max_proj_stack_depth);

    /* preserve current matrix mode */
    GLint matrix_mode = 0;
    glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);

    /*
     * Build projection matrix with helpers (GL & GLU).
     *
     */

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    switch(camera->priv->projection)
    {
        case GTK_RTG_CAM_PROJ_ORTHO:
            {
                /* approximate the field of view. */
                const gdouble y = camera->priv->distance * camera->priv->fov / 112.5;
                const gdouble x = y * camera->priv->aspect;

                glOrtho( -x, x, -y, y, camera->priv->znear, camera->priv->zfar); break;
            }

        case GTK_RTG_CAM_PROJ_PERSP:	gluPerspective(camera->priv->fov, camera->priv->aspect,
                                                          camera->priv->znear, camera->priv->zfar); break;
        default: break;
    };

    /* get matrix */
    glGetFloatv(GL_PROJECTION_MATRIX, camera->priv->mproj.m);

    glPopMatrix();
    glMatrixMode(matrix_mode);

    if(!(camera->priv->init & CAM_PROJ))
        camera->priv->init |= CAM_PROJ;

    /* re-calc both */
    camera->priv->calc_mprojview = camera->priv->calc_minverse = TRUE;

    g_object_notify(G_OBJECT(camera), prop[S_PROP_PROJ_MATRIX]);
}


void
gtk_rtg_camera_projection_set_matrix(GtkRtgCamera *camera, matrix4x4f *matrix) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    g_throw_if_fail(matrix != NULL);

    camera->priv->mproj = *matrix;

    g_object_notify(G_OBJECT(camera), prop[S_PROP_PROJ_MATRIX]);
}


matrix4x4f &
gtk_rtg_camera_projection_get_matrix(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    return camera->priv->mproj;
}


void
gtk_rtg_camera_projection_load_matrix(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));

    /* get current matrix mode */
    GLint matrix_mode = 0;
    glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);

    if(matrix_mode != GL_PROJECTION)
    {
        /*
         * Matrix mode change is required
         */

        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(camera->priv->mproj.m);
        glMatrixMode(matrix_mode);
    }else
        glLoadMatrixf(camera->priv->mproj.m);
}


void
gtk_rtg_camera_projection_push_matrix(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));

    GLint proj_stack_depth = 0;
    GLint max_proj_stack_depth = 0;

    glGetIntegerv(GL_PROJECTION_STACK_DEPTH,     &proj_stack_depth);
    glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH, &max_proj_stack_depth);

    /* can model-view matrix stack be pushed? */
    g_throw_if_fail(proj_stack_depth < max_proj_stack_depth);

    /* get current matrix mode */
    GLint matrix_mode = 0;
    glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);

    if(matrix_mode != GL_PROJECTION)
    {
        /*
         * Matrix mode change is required
         */

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadMatrixf(camera->priv->mproj.m);
        glMatrixMode(matrix_mode);
    }else
    {
        glPushMatrix();
        glLoadMatrixf(camera->priv->mproj.m);
    }
}


void
gtk_rtg_camera_projection_pop_matrix(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));

    GLint proj_stack_depth = 0;

    glGetIntegerv(GL_PROJECTION_STACK_DEPTH, &proj_stack_depth);

    /* can model-view matrix stack be popped? */
    g_throw_if_fail(proj_stack_depth > 1);

    /* get current matrix mode */
    GLint matrix_mode = 0;
    glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);

    if(matrix_mode != GL_PROJECTION)
    {
        /*
         * Matrix mode change is required
         */
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(matrix_mode);
    }else
        glPopMatrix();

}

gtkrtg::matrix4x4f&
gtk_rtg_camera_projview_get_matrix(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));

    if(camera->priv->calc_mprojview)
    {
        __gluMultMatricesf(camera->priv->mviewf.m, camera->priv->mproj.m, camera->priv->mprojview.m);
        camera->priv->calc_mprojview = FALSE;
    }

    return camera->priv->mprojview;
}

gtkrtg::matrix4x4f&
gtk_rtg_camera_projview_inverse_get_matrix(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));

    if(camera->priv->calc_mprojview)
    {
        __gluMultMatricesf(camera->priv->mviewf.m, camera->priv->mproj.m, camera->priv->mprojview.m);
        camera->priv->calc_mprojview = FALSE;
    }

    if(camera->priv->calc_minverse)
    {
        __gluInvertMatrixf(camera->priv->mprojview.m, camera->priv->minverse.m);
        camera->priv->calc_minverse = FALSE;
    }

    return camera->priv->minverse;
}


void
gtk_rtg_camera_prioritize(GtkRtgCamera *camera, GtkRtgCanvas *canvas) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));

    /**
     * Emit 'prioritize' signal
     *
     */

    g_signal_emit(camera, signals[CAMERA_PRIORITIZE], 0, canvas);
}


void
gtk_rtg_camera_set_projection(GtkRtgCamera *camera, GtkRtgCamProj projection) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    g_throw_if_fail(projection == GTK_RTG_CAM_PROJ_ORTHO || projection == GTK_RTG_CAM_PROJ_PERSP);
    camera->priv->projection = projection;

    g_object_notify(G_OBJECT(camera), prop[S_PROP_PROJ_TYPE]);
}


GtkRtgCamProj
gtk_rtg_camera_get_projection(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    return camera->priv->projection;
}


GtkRtgCamFrustum *
gtk_rtg_camera_projection_get_frustum(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    return &camera->priv->frustum;
}


gboolean
gtk_rtg_camera_is_initialized(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));
    return (camera->priv->init == CAM_INIT);
}



void
gtk_rtg_camera_build_frustum(GtkRtgCamera     *camera,
                             GdkPoint		   &start,
                             GdkPoint		   &end,
                             GtkRtgCamFrustum  &frustum) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));

    /*
       Frustum planes (unwrapped)
       ---------------------------------

       p0f p3f
       -------
       |  N1   |
       |       |
       p3f     p0f    |p0n p3n|     p3f
       ------- ------- ------- -------
       |       |       |       |       |
       |  FAR  |  N0   | NEAR  |  N2   |
       |       |       |       |       |
       ------- ------- ------- -------
       p2f     p1f    |p1n p2n|     p2f
       |       |
       |  N3   |
       -------
       p1f p2f
     */

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    enum{ P0_NEAR, P1_NEAR, P2_NEAR, P3_NEAR, P0_FAR, P1_FAR, P2_FAR, P3_FAR, FRUSTUM_POINTS };
    point3f fp[FRUSTUM_POINTS];

    /* frustum points */
    gluUnProjectf((GLfloat)start.x, (GLfloat)start.y, 0.f, camera->priv->mviewf.m, camera->priv->mproj.m, viewport, &X(fp[P0_NEAR]), &Y(fp[P0_NEAR]), &Z(fp[P0_NEAR]));
    gluUnProjectf((GLfloat)start.x, (GLfloat)start.y, 1.f, camera->priv->mviewf.m, camera->priv->mproj.m, viewport, &X(fp[P0_FAR]),  &Y(fp[P0_FAR]),  &Z(fp[P0_FAR]));
    gluUnProjectf((GLfloat)start.x, (GLfloat)end.y,   0.f, camera->priv->mviewf.m, camera->priv->mproj.m, viewport, &X(fp[P1_NEAR]), &Y(fp[P1_NEAR]), &Z(fp[P1_NEAR]));
    gluUnProjectf((GLfloat)start.x, (GLfloat)end.y,   1.f, camera->priv->mviewf.m, camera->priv->mproj.m, viewport, &X(fp[P1_FAR]),  &Y(fp[P1_FAR]),  &Z(fp[P1_FAR]));
    gluUnProjectf((GLfloat)end.x,   (GLfloat)end.y,   0.f, camera->priv->mviewf.m, camera->priv->mproj.m, viewport, &X(fp[P2_NEAR]), &Y(fp[P2_NEAR]), &Z(fp[P2_NEAR]));
    gluUnProjectf((GLfloat)end.x,   (GLfloat)end.y,   1.f, camera->priv->mviewf.m, camera->priv->mproj.m, viewport, &X(fp[P2_FAR]),  &Y(fp[P2_FAR]),  &Z(fp[P2_FAR]));
    gluUnProjectf((GLfloat)end.x,   (GLfloat)start.y, 0.f, camera->priv->mviewf.m, camera->priv->mproj.m, viewport, &X(fp[P3_NEAR]), &Y(fp[P3_NEAR]), &Z(fp[P3_NEAR]));
    gluUnProjectf((GLfloat)end.x,   (GLfloat)start.y, 1.f, camera->priv->mviewf.m, camera->priv->mproj.m, viewport, &X(fp[P3_FAR]),  &Y(fp[P3_FAR]),  &Z(fp[P3_FAR]));

    /* planes normals */

    /* side planes */
    frustum.p[FPLANE_0].normal = ((fp[P0_NEAR]-fp[P0_FAR]).cross(fp[P1_NEAR]-fp[P0_NEAR])).norm();
    frustum.p[FPLANE_1].normal = ((fp[P3_NEAR]-fp[P3_FAR]).cross(fp[P0_NEAR]-fp[P3_NEAR])).norm();
    frustum.p[FPLANE_2].normal = ((fp[P2_NEAR]-fp[P2_FAR]).cross(fp[P3_NEAR]-fp[P2_NEAR])).norm();
    frustum.p[FPLANE_3].normal = ((fp[P1_NEAR]-fp[P1_FAR]).cross(fp[P2_NEAR]-fp[P1_NEAR])).norm();
    /* near */
    frustum.p[FPLANE_4].normal = ((fp[P0_NEAR]-fp[P1_NEAR]).cross(fp[P3_NEAR]-fp[P0_NEAR])).norm();
    /* far */
    frustum.p[FPLANE_5].normal = ((fp[P3_FAR]-fp[P2_FAR]).cross(fp[P0_FAR]-fp[P3_FAR])).norm();

    /* planes 'd' */
    frustum.p[FPLANE_0].d = fp[P0_NEAR].dot(frustum.p[FPLANE_0].normal);
    frustum.p[FPLANE_1].d = fp[P3_NEAR].dot(frustum.p[FPLANE_1].normal);
    frustum.p[FPLANE_2].d = fp[P2_NEAR].dot(frustum.p[FPLANE_2].normal);
    frustum.p[FPLANE_3].d = fp[P1_NEAR].dot(frustum.p[FPLANE_3].normal);
    frustum.p[FPLANE_4].d = fp[P0_NEAR].dot(frustum.p[FPLANE_4].normal);
    frustum.p[FPLANE_5].d = fp[P3_FAR].dot(frustum.p[FPLANE_5].normal);
}


void
gtk_rtg_camera_do_frustum(GtkRtgCamera *camera) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    GdkPoint start, end;

    start.x = viewport[0];
    start.y = viewport[3];
    end.x   = viewport[2];
    end.y   = viewport[1];

    gtk_rtg_camera_build_frustum(camera, start, end, camera->priv->frustum);
}
