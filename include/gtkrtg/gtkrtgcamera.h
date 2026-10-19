#ifndef GTK_RTG_CAMERA_H
#define GTK_RTG_CAMERA_H

#include <gtkrtg/gtkrtgbase.h>
#include <gtkrtg/gtkrtgprimitive.h>

/*! \file */

G_BEGIN_DECLS

#define GTK_RTG_TYPE_CAMERA             (gtk_rtg_camera_get_type())
#define GTK_RTG_CAMERA(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_RTG_TYPE_CAMERA, GtkRtgCamera))
#define GTK_RTG_CAMERA_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass),  GTK_RTG_TYPE_CAMERA, GtkRtgCameraClass))
#define GTK_RTG_IS_CAMERA(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_RTG_TYPE_CAMERA))
#define GTK_RTG_IS_CAMERA_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass),  GTK_RTG_TYPE_CAMERA))
#define GTK_RTG_CAMERA_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),  GTK_RTG_TYPE_CAMERA, GtkRtgCameraClass))

typedef struct _GtkRtgCameraPrivate	GtkRtgCameraPrivate;

struct GtkRtgCanvas;

struct GtkRtgCamera
{
	GtkRtgPrimitive		 parent;
	GtkRtgCameraPrivate	*priv;

};

struct GtkRtgCameraClass
{
	GtkRtgPrimitiveClass parent_class;

	/* signal(s) */
	void (* prioritize)	(GtkRtgCamera *camera, GtkRtgCanvas *canvas);
};


/*! \defgroup GtkRtgCamera GtkRtgCamera
    \c GtkRtgCamera is derived from the \ref GtkRtgPrimitive. \n \n

    The \c GtkRtgCamera is a target based camera. A target can be any point in 3D space which is viewer's point of interest. That point is defined as \c center point
    at which viewer is oriented to look at. The direction in 3D space from which a viewer is oriented to the \c center is defined by the \c direction vector.
    The distance from the viewer's position, or \c eye point, to the \c center is defined with the \c distance value. Definition of what is up or down when looking from
    the camera is defined with the \c up vector. \n
    In case a camera's projection \ref GtkRtgCamProj is set to be an perspective a shape of a camera's prism volume, or an area which viewer 'covers', is
    controlled by the \c FOV angle, \c znear and \c zfar planes distances.
    In case a camera's projection is set to be an orthographic a shape of a volume is a box, in which case a \c FOV angle is ignored. \n
    A camera keeps internally both an view and projection matrices as \ref gtkrtg::matrix4x4f.

  \section properties Properties
  - \b view-center \c gtkrtg::point3f Read / Write\n
    A point at which viewer is directed to look at.\n
    Default Value: (0, 0, 0)

    - \b view-direction \c gtkrtg::vector3f Read / Write\n
    The direction vector in 3D space from which a viewer is oriented to the center point. \n
    Default Value: (-1, -1, -1)

    - \b view-up \c gtkrtg::vector3f Read / Write\n
    The vector which determines a viewer up direction.\n
    Default Value: (0, 1, 0)

    - \b view-eye \c gtkrtg::point3f Read / Write\n
    A viewer position in 3D space.\n
    Default Value: (-10, -10, -10)

    - \b view-distance \c gfloat Read / Write\n
    The distance from the viewer to the point he looking at.\n
    Default Value: 10

    - \b view-matrix \c gtkrtg::matrix4x4f Read / Write\n
    A view matrix.\n
    Default Value: identity

    - \b proj-fov \c gfloat Read / Write\n
    A field of view angle, in degrees, in y direction.\n
    Default Value: 30

    - \b proj-aspect \c gfloat Read / Write\n
    The aspect ratio that determines a field of view in the x-direction. \n
    Default Value: 1.33

    - \b proj-znear \c gfloat Read / Write\n
    The distance from the viewer to the near clipping plane. \n
    Default Value: 1

    - \b proj-zfar \c gfloat Read / Write\n
    The distance from the viewer to the near clipping plane. \n
    Default Value: 1000

    - \b proj-type \c GtkRtgCamProj Read / Write\n
    The projection type. \n
    Default Value: \ref GTK_RTG_CAM_PROJ_PERSP

    - \b proj-matrix \c gtkrtg::matrix4x4f Read / Write\n
    The projection matrix.\n
    Default Value: identity

    \section signals Signals
    - \b prioritize \n
    user_function (camera, canvas, user_data) Run Last / Action\n
    A camera emits this signal when it gets prioritized. \n
    It is used when \ref GtkRtgScene has more than one camera set to exchange between them. The prioritized camera becomes the scene's default camera which forwards
    it up to the \ref GtkRtgCanvas. \n
    The canvas uses the scenes default camera as it views operating camera.
*/

/* projection types */

/** \ingroup GtkRtgCamera
 *  GtkRtgCamProj
 *  \brief A camera's projection modes.
 */
typedef enum
{
	GTK_RTG_CAM_PROJ_ORTHO,	/**< Orthographic projection.  */
	GTK_RTG_CAM_PROJ_PERSP	/**< Perspective projection.   */
}GtkRtgCamProj;


/* frustum */
enum{ FPLANE_0, FPLANE_1, FPLANE_2, FPLANE_3, FPLANE_4, FPLANE_5, FRUSTUM_PLANES };

/** \ingroup GtkRtgCamera
 *  GtkRtgCamFrustum
 *  \brief A camera's frustum.
 */
struct GtkRtgCamFrustum
{
	gtkrtg::plane_f p[FRUSTUM_PLANES]; /**< A six frustum planes defined by normal and 'd' value.  */
};


GType gtk_rtg_camera_get_type(void) G_GNUC_CONST;

/*!
    \ingroup GtkRtgCamera
    \brief Creates a new \c GtkRtgCamera.
    \return - A \ref GtkRtgPrimitive.
*/
GtkRtgPrimitive  *gtk_rtg_camera_new();

/* view */

/*!
    \ingroup GtkRtgCamera
    \brief Sets up a point at which viewer is directed to look at. \n
    Additionally a camera's \c eye position will be recalculated.
    \param camera - A \c GtkRtgCamera.
    \param center - A \ref gtkrtg::point3f to set.
    \attention - In case \c camera fails on integrity check, or a \c NULL is passed for \c center a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_view_set_center(GtkRtgCamera *camera, const gtkrtg::point3d *center) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Returns a point at which viewer is directed to look at.
    \param camera - A \c GtkRtgCamera.
    \return - A \ref gtkrtg::point3f.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
gtkrtg::point3d *gtk_rtg_camera_view_get_center(GtkRtgCamera *camera) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Sets up a direction vector in 3D space from which a viewer is oriented to the center point. \n
    Additionally a camera's \c eye position will be recalculated.
    \param camera - A \c GtkRtgCamera.
    \param direction - The direction in 3D space from which a viewer is oriented to the center point.
    \attention - In case \c camera fails on integrity check, or a \c NULL is passed for \c direction a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_view_set_direction(GtkRtgCamera *camera, const gtkrtg::vector3d *direction)	throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Returns a direction vector in 3D space from which a viewer is oriented to the center point.
    \return - a \ref gtkrtg::vector3f value.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
gtkrtg::vector3d *gtk_rtg_camera_view_get_direction(GtkRtgCamera *camera) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Sets up a vector which determines a viewer up direction.
    \param camera - A \c GtkRtgCamera.
    \param up - A vector which determines a viewer up direction.
    \attention - In case \c camera fails on integrity check, or a \c NULL is passed for \c up a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_view_set_up(GtkRtgCamera *camera, const gtkrtg::vector3d *up) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Returns a vector which determines a viewer up direction.
    \param camera - A \c GtkRtgCamera.
    \return - A \ref gtkrtg::vector3f.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
gtkrtg::vector3d *gtk_rtg_camera_view_get_up(GtkRtgCamera *camera) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Sets up a distance from the viewer to the point he looking at. \n
    If distance is less than \c znear distance, then it's set to a \c znear distance. \n
    Additionally a camera's \c eye position will be recalculated.
    \param camera - A \ref GtkRtgCamera.
    \param distance - A a distance from the \c eye position to the \c center point.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_view_set_distance(GtkRtgCamera *camera, const gdouble distance)	throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Returns a distance from the viewer to the point he looking at.
    \param camera - A \c GtkRtgCamera.
    \return - A \c gfloat.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
gdouble gtk_rtg_camera_view_get_distance(GtkRtgCamera *camera) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Sets up a viewer position in 3D space. \n
    Additionally a camera's \c distance as well as \c direction vector will be recalculated.
    \param camera - A \c GtkRtgCamera.
    \param eye - A viewer position in 3D space.
    \attention - In case \c camera fails on integrity check, or a \c NULL is passed for \c eye a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_view_set_eye(GtkRtgCamera *camera, const gtkrtg::point3d *eye) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Returns a viewer position in 3D space.
    \param camera - A \c GtkRtgCamera.
    \return - A \ref gtkrtg::point3f.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
gtkrtg::point3d	 *gtk_rtg_camera_view_get_eye(GtkRtgCamera *camera)	throw (GtkRtgException);

/* calculate/set view matrix */

/*!
    \ingroup GtkRtgCamera
    \brief Builds up a view matrix. \n
    After matrix is built a notification 'view-matrix' will be sent.
    \param camera - A \c GtkRtgCamera.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_view_do_matrix(GtkRtgCamera *camera) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Sets up a view matrix.
    \param camera - A \c GtkRtgCamera.
    \param matrix - A \c gtkrtg::matrix4x4f to set.
    \attention - In case \c camera fails on integrity check, or a \c NULL is passed for \c matrix a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_view_set_matrix(GtkRtgCamera *camera, const gtkrtg::matrix4x4f *matrix) throw (GtkRtgException);

/* view matrix op.*/

/*!
    \ingroup GtkRtgCamera
    \brief Returns a view matrix.
    \param camera - A \c GtkRtgCamera.
    \return - A \ref gtkrtg::matrix4x4f.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
gtkrtg::matrix4x4f	&gtk_rtg_camera_view_get_matrix(GtkRtgCamera *camera) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Loads a view matrix onto the stack. \n
    The view matrix will be loaded onto the model-view matrix stack at current level. \n
    The current matrix mode will be preserved.
    \param camera - A \c GtkRtgCamera.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_view_load_matrix(GtkRtgCamera *camera) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Pushes up a view matrix onto the stack. \n
    The model-view matrix stack depth will be pushed before matrix is loaded onto the stack. \n
    The current matrix mode will be preserved.
    \param camera - A \c GtkRtgCamera.
    \attention - In case \c camera fails on integrity check, or a model-view matrix stack is full a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_view_push_matrix(GtkRtgCamera *camera) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Pops up a view matrix from the stack. \n
    The current matrix mode will be preserved.
    \param camera - A \c GtkRtgCamera.
    \attention - In case \c camera fails on integrity check, or a model-view matrix stack contains a single matrix a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_view_pop_matrix(GtkRtgCamera *camera) throw (GtkRtgException);

/* projection */

/*!
    \ingroup GtkRtgCamera
    \brief Sets up a field of view angle.
    \param camera - A \ref GtkRtgCamera.
    \param fov - A \c gfloat angle in degrees, in y direction.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_projection_set_fov(GtkRtgCamera *camera, const GLfloat fov)	throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Returns a field of view angle.
    \param camera - A \ref GtkRtgCamera.
    \return - A \c gfloat angle in degrees, in y direction.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
GLfloat	 gtk_rtg_camera_projection_get_fov(GtkRtgCamera *camera) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Sets up a aspect ratio that determines a field of view in the x-direction.
    \param camera - A \ref GtkRtgCamera.
    \param aspect - The aspect ratio that determines the field of view in the x-direction. The aspect ratio is the ratio of i.e. viewport's x (width) to y (height).
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_projection_set_aspect(GtkRtgCamera *camera, const GLfloat aspect) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Returns a aspect ratio that determines the field of view in the x-direction.
    \param camera - A \ref GtkRtgCamera.
    \return - A \c gfloat.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
GLfloat	 gtk_rtg_camera_projection_get_aspect(GtkRtgCamera *camera)	throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Sets up a distance from the viewer to the near clipping plane.
    \param camera - A \ref GtkRtgCamera.
    \param znear - The distance from the viewer to the near clipping plane.
    \attention - In case \c camera fails on integrity check, or a negative value is passed for \c znear a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_projection_set_znear(GtkRtgCamera *camera, const GLfloat znear)	throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Returns a distance from the viewer to the near clipping plane.
    \param camera - A \ref GtkRtgCamera.
    \return - A \c gfloat.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
GLfloat	 gtk_rtg_camera_projection_get_znear(GtkRtgCamera *camera)	throw (GtkRtgException);


/*!
    \ingroup GtkRtgCamera
    \brief Sets up a distance from the viewer to the far clipping plane.
    \param camera - A \ref GtkRtgCamera.
    \param zfar - The distance from the viewer to the far clipping plane.
    \attention - In case \c camera fails on integrity check, or a negative value is passed for \c zfar a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_projection_set_zfar(GtkRtgCamera *camera, const GLfloat zfar) throw (GtkRtgException);


/*!
    \ingroup GtkRtgCamera
    \brief Returns a distance from the viewer to the far clipping plane.
    \param camera - A \ref GtkRtgCamera.
    \return - A \c gfloat.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
GLfloat	 gtk_rtg_camera_projection_get_zfar(GtkRtgCamera *camera) throw (GtkRtgException);

/* projection type */

/*!
    \ingroup GtkRtgCamera
    \brief Sets up a projection type. \n
    The default projection type is \ref GTK_RTG_CAM_PROJ_PERSP.
    \param camera - A \ref GtkRtgCamera.
    \param projection - A \ref GtkRtgCamProj projection type.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_set_projection(GtkRtgCamera *camera, GtkRtgCamProj projection)  throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Returns a projection type.
    \param camera - A \ref GtkRtgCamera.
    \return - A \c GtkRtgCamProj.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
GtkRtgCamProj gtk_rtg_camera_get_projection(GtkRtgCamera *camera) throw (GtkRtgException);

/* calculate/set projection matrix */

/*!
    \ingroup GtkRtgCamera
    \brief Builds up a projection matrix. \n
    After matrix is built a notification 'proj-matrix' will be sent.
    \param camera - A \c GtkRtgCamera.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_projection_do_matrix(GtkRtgCamera *camera)	throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Sets up a projection matrix.
    \param camera - A \c GtkRtgCamera.
    \param matrix - A \c gtkrtg::matrix4x4f to set.
    \attention - In case \c camera fails on integrity check, or a \c NULL is passed for \c matrix a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_projection_set_matrix(GtkRtgCamera *camera, gtkrtg::matrix4x4f *matrix)	throw (GtkRtgException);

/* projection matrix op. */

/*!
    \ingroup GtkRtgCamera
    \brief Returns a projection matrix.
    \param camera - A \c GtkRtgCamera.
    \return - A \ref gtkrtg::matrix4x4f.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
gtkrtg::matrix4x4f	&gtk_rtg_camera_projection_get_matrix(GtkRtgCamera *camera)	throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Loads a projection matrix onto the stack. \n
    The projection matrix will be loaded onto the projection matrix stack at current level. \n
    The current matrix mode will be preserved.
    \param camera - A \c GtkRtgCamera.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_projection_load_matrix(GtkRtgCamera *camera) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Pushes up a projection matrix onto the stack. \n
    The projection matrix stack depth will be pushed before matrix is loaded onto the stack. \n
    The current matrix mode will be preserved.
    \param camera - A \c GtkRtgCamera.
    \attention - In case \c camera fails on integrity check, or a projection matrix stack is full a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_projection_push_matrix(GtkRtgCamera *camera) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Pops up a projection matrix from the stack. \n
    The current matrix mode will be preserved.
    \param camera - A \c GtkRtgCamera.
    \attention - In case \c camera fails on integrity check, or a projection matrix stack contains a single matrix a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_projection_pop_matrix(GtkRtgCamera *camera)	throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Returns a frustum.
    \param camera - A \c GtkRtgCamera.
    \return - A \ref GtkRtgCamFrustum.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
GtkRtgCamFrustum *gtk_rtg_camera_projection_get_frustum(GtkRtgCamera *camera) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Builds up a frustum from the viewport.
    \param camera - A \c GtkRtgCamera.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_do_frustum(GtkRtgCamera *camera) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Builds up a frustum from the start and end points.
    \param camera - A \c GtkRtgCamera.
    \param start - A \c GdkPoint start point.
    \param end - A \c GdkPoint end point.
    \param frustum - A \c GtkRtgCamFrustum to built.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_build_frustum(GtkRtgCamera *camera, GdkPoint &start,
								  GdkPoint &end, GtkRtgCamFrustum &frustum) throw (GtkRtgException);

/* emit signal: 'prioritize' camera */

/*!
    \ingroup GtkRtgCamera
    \brief Emit the 'prioritized' signal. \n
    It is used when \ref GtkRtgScene has more than one camera set to exchange between them. The prioritized camera becomes the scene's default camera which forwards
    it up to the \ref GtkRtgCanvas. \n
    The canvas uses the scenes default camera as it views operating camera.
    \param camera - A \c GtkRtgCamera.
    \param canvas - A \ref GtkRtgCanvas on which camera will prioritize, or \c NULL.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_camera_prioritize(GtkRtgCamera *camera, GtkRtgCanvas *canvas) throw (GtkRtgException);

/* utils  */

/*!
    \ingroup GtkRtgCamera
    \brief Checks up whether a camera is initialized. \n
    The camera becomes initialized when both, view and projection matrices are build.
    \param camera - A \c GtkRtgCamera.
    \return - \c TRUE if a camera is initialized, or \c FALSE if not.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
gboolean gtk_rtg_camera_is_initialized(GtkRtgCamera *camera) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Returns a pre-multiplied view and projection matrices.
    \param camera - A \c GtkRtgCamera.
    \return - A \ref gtkrtg::matrix4x4f.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
gtkrtg::matrix4x4f	&gtk_rtg_camera_projview_get_matrix(GtkRtgCamera *camera) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCamera
    \brief Returns an inverse of pre-multiplied view and projection matrices.
    \param camera - A \c GtkRtgCamera.
    \return - A \ref gtkrtg::matrix4x4f.
    \attention - In case \c camera fails on integrity check a \ref GtkRtgException will be thrown.
*/
gtkrtg::matrix4x4f	&gtk_rtg_camera_projview_inverse_get_matrix(GtkRtgCamera *camera) throw (GtkRtgException);

/* handy macros (getters) */
#define	M_VIEW(cam)	  	(gtk_rtg_camera_view_get_matrix(GTK_RTG_CAMERA(cam)))
#define	M_PROJ(cam)	  	(gtk_rtg_camera_projection_get_matrix(GTK_RTG_CAMERA(cam)))
#define M_PROJVIEW(cam) (gtk_rtg_camera_projview_get_matrix(GTK_RTG_CAMERA(cam)))
#define M_INVERSE(cam)  (gtk_rtg_camera_projview_inverse_get_matrix(GTK_RTG_CAMERA(cam)))
#define	ZNEAR(cam)	  	(gtk_rtg_camera_projection_get_znear(GTK_RTG_CAMERA(cam)))
#define	ZFAR(cam)	  	(gtk_rtg_camera_projection_get_zfar(GTK_RTG_CAMERA(cam)))
#define	PROJECTION(cam) (gtk_rtg_camera_get_projection(GTK_RTG_CAMERA(cam)))
#define	FRUSTUM(cam)	(gtk_rtg_camera_projection_get_frustum(GTK_RTG_CAMERA(cam)))
#define	IS_CAM_INIT(cam)(gtk_rtg_camera_is_initialized(GTK_RTG_CAMERA(cam)))
#define EYE_POINT(cam)  (gtk_rtg_camera_view_get_eye(GTK_RTG_CAMERA(cam)))

G_END_DECLS

#endif /* GTK_RTG_CAMERA_H */
