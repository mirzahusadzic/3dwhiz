#ifndef GTK_RTG_PRIMITIVE_H
#define GTK_RTG_PRIMITIVE_H

#include <gtkrtg/gtkrtgbase.h>
#include <gtkrtg/gtkrtgexception.h>
#include <gtkrtg/gtkrtgmath.h>
#include <gtkrtg/gtkrtgtexture.h>

#include <vector>
#include <set>

/*! \file */

G_BEGIN_DECLS

#define GTK_RTG_TYPE_PRIMITIVE            (gtk_rtg_primitive_get_type())
#define GTK_RTG_PRIMITIVE(obj)		      (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_RTG_TYPE_PRIMITIVE, GtkRtgPrimitive))
#define GTK_RTG_PRIMITIVE_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass),  GTK_RTG_TYPE_PRIMITIVE, GtkRtgPrimitiveClass))
#define GTK_RTG_IS_PRIMITIVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_RTG_TYPE_PRIMITIVE))
#define GTK_RTG_IS_PRIMITIVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GTK_RTG_TYPE_PRIMITIVE))
#define GTK_RTG_PRIMITIVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GTK_RTG_TYPE_PRIMITIVE, GtkRtgPrimitiveClass))

typedef struct _GtkRtgPrimitive		   GtkRtgPrimitive;
typedef struct _GtkRtgPrimitiveClass   GtkRtgPrimitiveClass;
typedef struct _GtkRtgPrimitivePrivate GtkRtgPrimitivePrivate;

/*! \defgroup GtkRtgPrimitive GtkRtgPrimitive
    \c GtkRtgPrimitive is derived from the \c GObject. \n \n
    The \c GtkRtgPrimitive is a base object of all geometric primitives or shapes. It keeps arrays of constructive mesh data from which a shape is developed, but
    doesn't define any procedure to render a shape from polygons (see \ref GtkRtgTriMesh). \n
    Besides, the \c GtkRtgPrimitive's \c is able to render a shape in point representation, or even edges as wire-frame. Rendering of colored or textured polygonal shapes
    i.e. triangle mesh, quad mesh etc. should be done in sub-classes (i.e. by overriding \c 'render-event' signal). \n
    The \c GtkRtgPrimitive performs per-vertex tranformations such are translate, scale and rotate in matrix form, from which a rigid matrix is built. Transformation
    matrix is then contcatenated from a rigid and non-rigid (scale matrix) matrices. In case it gets rendered the primitive emits \c 'render-event' signal when rendering mode is
    not \ref GTK_RTG_RENDER_POINTS or \ref GTK_RTG_RENDER_EDGES. At the moment of the signal emmitance the primitives transformation matrix is pushed to the model-view matrix stack. \n
    Sub-primitive selections and editing are done at three levels. The primitives points, edges and polygons can be selected and edited by choosing one of \ref GtkRtgPrimEditMode modes. \n
    Selections are implemented in \ref GtkRtgScene. By instantiation of a new \c GtkRtgPrimitive sub-class a \ref GtkRtgPrimitiveBBox and \ref GtkRtgPrimitiveBSphere should be maintained
    manually in order to be used in \ref GtkRtgScene (see \ref gtk_rtg_primitive_calc_bbox_bsphere).

    \section signals Signals
    - \b render-event \n
    user_function (primitive, render_mode, user_data) Run Last / Action\n
    A primitive emits this signal in order to be rendered. Transformation matrix has been pushed to the model-view matrix stack. \n

    - \b transformed \n
    user_function (primitive, user_data) Run Last / Action\n
    A primitve emits this signal after transformation matrix is changed (i.e. It is used in \ref GtkRtgScene to update (render) off-screen buffer for picking).
*/

struct _GtkRtgPrimitive
{
	GObject			 		 parent;
	GtkRtgPrimitivePrivate	*priv;
};


struct _GtkRtgPrimitiveClass
{
	GObjectClass parent_class;

	/* signals */
	void (* render_event) (GtkRtgPrimitive *prim, guint mode);
	void (* transformed)  (GtkRtgPrimitive *prim);
};


/** \ingroup GtkRtgPrimitive
 *  GtkRtgPrimitiveRenderMode
 *  \brief A primitives render modes.
 */
typedef enum
{	/* primitives */
	GTK_RTG_RENDER_POINTS=0, /**< points				*/
	GTK_RTG_RENDER_EDGES,	 /**< edges (i.e. wire-frame)		*/
	GTK_RTG_RENDER_SOLID,	 /**< solid (i.e. colored or textured)  */

	/* picking */
	GTK_RTG_RENDER_SELECT,	/**< selection (i.e. colored unique)	*/

	/* end */
	GTK_RTG_RENDER_MODES

}GtkRtgPrimitiveRenderMode;


/** \ingroup GtkRtgPrimitive
 *  GtkRtgPrimEditMode
 *  \brief A primitives edit modes.
 */
typedef enum
{
	GTK_RTG_EDIT_OBJECT, /**< object   */
	GTK_RTG_EDIT_POINTS, /**< points   */
	GTK_RTG_EDIT_EDGES,  /**< edges    */
	GTK_RTG_EDIT_POLYS,  /**< polygons */

	GTK_RTG_EDIT_MODES

}GtkRtgPrimEditMode;


/* bounding box */
enum{ BBOX_A, BBOX_B, BBOX_C, BBOX_D, BBOX_E, BBOX_F, BBOX_G, BBOX_H, BBOX_CENTER, BBOX_POINTS };
    /* max */                        /* min */                                      /* end */

/** \ingroup GtkRtgPrimitive
 *  GtkRtgPrimitiveBBox
 *  \brief A primitives bounding box.
 */
struct GtkRtgPrimitiveBBox{ gtkrtg::point3f p[BBOX_POINTS]; };


/** \ingroup GtkRtgPrimitive
 *  GtkRtgPrimitiveBSphere
 *  \brief A primitives bounding sphere.
 */
struct GtkRtgPrimitiveBSphere{  gtkrtg::point3f center; gfloat r; };

/*
 * Indices (tris, quads, edges and texture coo.'s) storage
 *
 *      - These types are used for primitive and trimesh
 */

#ifdef  GTK_RTG_VERTS_HIGH
        typedef guint32         guint_ind;              /* indices base type               */
        typedef guint64         guint_ind2;             /* pack edge indices as ihigh:ilow */
        #define IND_BITS        32
        #define IND2_HIGH_BITS  0xffffffff00000000
        #define IND2_LOW_BITS   0x00000000ffffffff
#else/* GTK_RTG_VERTS_LOW  - (default) */
        typedef guint16 guint_ind;                      /* indices base type               */
        typedef guint32 guint_ind2;                     /* pack edge indices as ihigh:ilow */
        #define IND_BITS        16
        #define IND2_HIGH_BITS  0xffff0000
        #define IND2_LOW_BITS   0x0000ffff
#endif

/*
 * Use this macro to extract vertex indices as 'i0 and i1' from edge type (guint_ind2)
 */
#define EDGE_INDICES(edge) guint_ind i0 = guint_ind((*edge & IND2_HIGH_BITS) >> IND_BITS); \
                           guint_ind i1 = guint_ind(*edge & IND2_LOW_BITS);

/*
 * instantiate
 */

GType gtk_rtg_primitive_get_type(void) G_GNUC_CONST;

/*!
    \ingroup GtkRtgPrimitive
    \brief Creates a new \c GtkRtgPrimitive.
    \return - A \ref GtkRtgPrimitive.
*/
GtkRtgPrimitive *gtk_rtg_primitive_new();

/*!
    \ingroup GtkRtgPrimitive
    \brief Clones a \c GtkRtgPrimitive.
    \param prim - A clone source.
    \param dest - A clone destination.
    \param instance - if TRUE then primitive will be cloned by instance, or FALSE to be copied.
*/
void gtk_rtg_primitive_clone(GtkRtgPrimitive *prim, GtkRtgPrimitive *dest, gboolean instance=TRUE)throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Returns a cloned instance.
    \param prim - A \c GtkRtgPrimitive.
    \return - NULL if the primitive is not cloned by instance, or not NULL if it is.
*/
GtkRtgPrimitive *gtk_rtg_primitive_get_clone_instance(GtkRtgPrimitive *prim)	throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Sets up an array of vertices.
    \param prim - A \c GtkRtgPrimitive.
    \param verts - A \c std::vector<gtkrtg::point3f> array to set.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_set_vertices(GtkRtgPrimitive *prim, std::vector<gtkrtg::point3f> *verts)	throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Returns an array of vertices. \n
    The array is owned by the library.
    \param prim - A \c GtkRtgPrimitive.
    \return - A \c std::vector<gtkrtg::point3f>.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
std::vector<gtkrtg::point3f> *gtk_rtg_primitive_get_vertices(GtkRtgPrimitive *prim)	throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Sets up an array of vertex normals.
    \param prim - A \c GtkRtgPrimitive.
    \param vnormals - A \c std::vector<gtkrtg::vector3f> array to set.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_set_vnormals(GtkRtgPrimitive *prim, std::vector<gtkrtg::vector3f> *vnormals) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Returns an array of vertex normals. \n
    The array is owned by the library.
    \param prim - A \c GtkRtgPrimitive.
    \return - A \c std::vector<gtkrtg::vector3f>.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
std::vector<gtkrtg::vector3f> *gtk_rtg_primitive_get_vnormals(GtkRtgPrimitive *prim) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Sets up an array of vertex indices.
    \param prim - A \c GtkRtgPrimitive.
    \param vindices - A \c std::vector<guint_ind> array to set.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_set_vindices(GtkRtgPrimitive *prim, std::vector<guint_ind> *vindices) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Returns an array of vertex indices. \n
    The array is owned by the library.
    \param prim - A \c GtkRtgPrimitive.
    \return - A \c std::vector<guint_ind>.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
std::vector<guint_ind>	*gtk_rtg_primitive_get_vindices(GtkRtgPrimitive *prim)	throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Sets up an array of texture coordinates.
    \param prim - A \c GtkRtgPrimitive.
    \param texcoords - A \c std::vector<gtkrtg::point2f> array to set.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_set_texcoords(GtkRtgPrimitive *prim, std::vector<gtkrtg::point2f> *texcoords) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Returns an array of texture coordinates. \n
    The array is owned by the library.
    \param prim - A \c GtkRtgPrimitive.
    \return - A \c std::vector<gtkrtg::point2f>.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
std::vector<gtkrtg::point2f> *gtk_rtg_primitive_get_texcoords(GtkRtgPrimitive *prim)	throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Sets up an array of texture coordinate indices.
    \param prim - A \c GtkRtgPrimitive.
    \param tindices - A \c std::vector<guint_ind> array to set.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_set_tindices(GtkRtgPrimitive *prim, std::vector<guint_ind> *tindices)	throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Returns an array of texture coordinate indices. \n
    The array is owned by the library.
    \param prim - A \c GtkRtgPrimitive.
    \return - A \c std::vector<guint_ind>.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
std::vector<guint_ind>	*gtk_rtg_primitive_get_tindices(GtkRtgPrimitive *prim)	throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Sets up an array of edges.
    \param prim - A \c GtkRtgPrimitive.
    \param edges - A \c std::set<guint_ind> array to set.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_set_edges(GtkRtgPrimitive *prim, std::set<guint_ind2> *edges) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Returns an array of edges. \n
    The array is owned by the library.
    \param prim - A \c GtkRtgPrimitive.
    \return - A \c std::set<guint_ind>.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
std::set<guint_ind2> *gtk_rtg_primitive_get_edges(GtkRtgPrimitive *prim)	throw (GtkRtgException);


/* handy macros */
#define VERTS(prim)             (gtk_rtg_primitive_get_vertices(GTK_RTG_PRIMITIVE(prim)))
#define VNORMALS(prim)          (gtk_rtg_primitive_get_vnormals(GTK_RTG_PRIMITIVE(prim)))
#define VINDICES(prim)          (gtk_rtg_primitive_get_vindices(GTK_RTG_PRIMITIVE(prim)))
#define TINDICES(prim)          (gtk_rtg_primitive_get_tindices(GTK_RTG_PRIMITIVE(prim)))
#define TEXCOORDS(prim)         (gtk_rtg_primitive_get_texcoords(GTK_RTG_PRIMITIVE(prim)))
#define EDGES(prim)             (gtk_rtg_primitive_get_edges(GTK_RTG_PRIMITIVE(prim)))
#define SEL_VERTS(prim)         (gtk_rtg_primitive_get_selected_vertices(GTK_RTG_PRIMITIVE(prim)))
#define SEL_EDGES(prim)         (gtk_rtg_primitive_get_selected_edges(GTK_RTG_PRIMITIVE(prim)))
#define SEL_POLYS(prim)         (gtk_rtg_primitive_get_selected_polys(GTK_RTG_PRIMITIVE(prim)))
#define BBOX(prim)              (gtk_rtg_primitive_get_bbox(GTK_RTG_PRIMITIVE(prim)))
#define BSPHERE(prim)           (gtk_rtg_primitive_get_bsphere(GTK_RTG_PRIMITIVE(prim)))
#define R_AXIS(prim)            (gtk_rtg_primitive_get_axes_ratio(GTK_RTG_PRIMITIVE(prim)))
#define COLOR(prim)             (gtk_rtg_primitive_get_color(GTK_RTG_PRIMITIVE(prim)))
#define COLOR_ID(prim)          (gtk_rtg_primitive_get_color_id(GTK_RTG_PRIMITIVE(prim)))
#define NAME(prim)	        	(gtk_rtg_primitive_get_name(GTK_RTG_PRIMITIVE(prim)))
#define NAME_SET(prim, name)    (gtk_rtg_primitive_set_name(GTK_RTG_PRIMITIVE(prim), name))
#define SELECT(prim,val)        (gtk_rtg_primitive_select(GTK_RTG_PRIMITIVE(prim), val))
#define IS_SELECTED(prim)       (gtk_rtg_primitive_is_selected(GTK_RTG_PRIMITIVE(prim)))
#define FREEZE(prim, val)       (gtk_rtg_primitive_freeze(GTK_RTG_PRIMITIVE(prim), val))
#define IS_FROZEN(prim)	        (gtk_rtg_primitive_is_frozen(GTK_RTG_PRIMITIVE(prim)))
#define IS_NOT_FROZEN(prim)     (!gtk_rtg_primitive_is_frozen(GTK_RTG_PRIMITIVE(prim)))
#define VISIBLE(prim, val)		(gtk_rtg_primitive_set_visible(GTK_RTG_PRIMITIVE(prim), val))
#define IS_VISIBLE(prim)		(gtk_rtg_primitive_is_visible(GTK_RTG_PRIMITIVE(prim)))
#define SET_TRANS(prim, trans)  (gtk_rtg_primitive_translate(GTK_RTG_PRIMITIVE(prim), trans))
#define SET_SCALE(prim, s)		(gtk_rtg_primitive_scale(GTK_RTG_PRIMITIVE(prim), s))
#define CLONE_INST(prim)        (gtk_rtg_primitive_get_clone_instance(GTK_RTG_PRIMITIVE(prim)))
#define IS_CLONE(prim)	        (CLONE_INST(GTK_RTG_PRIMITIVE(prim)) != NULL)
#define IS_NOT_CLONE(prim)      (CLONE_INST(GTK_RTG_PRIMITIVE(prim)) == NULL)
#define EDIT_MODE(prim)	        (gtk_rtg_primitive_get_edit_mode(GTK_RTG_PRIMITIVE(prim)))
#define PIVOT(prim)	        	(gtk_rtg_primitive_get_pivot(GTK_RTG_PRIMITIVE(prim)))
#define TEXTURE(prim, u)        (gtk_rtg_primitive_get_texture(GTK_RTG_PRIMITIVE(prim), u))
#define TEXTURE_SET(prim, t, u) (gtk_rtg_primitive_set_texture(GTK_RTG_PRIMITIVE(prim), t, u))

/*
 * per vertex operations.
 *
 */

/* transformations */

/*!
    \ingroup GtkRtgPrimitive
    \brief Sets up a translation. \n
    It will update an translation part of a rigid matrix and the signal 'transformed' will be emitted.
    \param prim - A \c GtkRtgPrimitive.
    \param trans - Translation in vector form, where each ax is translated by the amount of a scalar for that ax.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_translate(GtkRtgPrimitive *prim, gtkrtg::vector3f &trans) throw (GtkRtgException);


/*!
    \ingroup GtkRtgPrimitive
    \brief Sets up a scale. \n
    It will update an scale matrix and the signal 'transformed' will be emitted.
    \param prim - A \c GtkRtgPrimitive.
    \param scale - Scale in vector form, where each ax is scaled for the amount of an ratio for that ax.
    \attention - In case \c primitive fails on integrity check or any of cardinal scale axis are zero a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_scale(GtkRtgPrimitive *prim, gtkrtg::vector3f &scale) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Sets up a rotation. \n
    It will update an rigid matrix and the signal 'transformed' will be emitted.
    \param prim - A \c GtkRtgPrimitive.
    \param axis - An axis of rotation.
    \param angle_deg - An angle in degrees.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_rotate_from_axis_angle(GtkRtgPrimitive *prim, gtkrtg::vector3f &axis, gfloat angle_deg) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Sets up a rotation. \n
    It will update an rigid matrix and the signal 'transformed' will be emitted.
    \param prim - A \c GtkRtgPrimitive.
    \param quat - A quaterion orientation.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_rotate_from_quaternion(GtkRtgPrimitive *prim, gtkrtg::quat_f &quat) throw (GtkRtgException);

gtkrtg::quat_f &gtk_rtg_primitive_get_orientation(GtkRtgPrimitive *prim) throw (GtkRtgException);

gtkrtg::vector3f &gtk_rtg_primitive_get_translation(GtkRtgPrimitive *prim) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Returns a transformation matrix. \n
    The transformation matrix will be built as product of rigid and scale matrices.
    \param prim - A \c GtkRtgPrimitive.
    \return - A \c gtkrtg::matrix4x4f.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
gtkrtg::matrix4x4f &gtk_rtg_primitive_get_transformation_matrix(GtkRtgPrimitive *prim) throw (GtkRtgException);


/*!
    \ingroup GtkRtgPrimitive
    \brief Renders a primitive. \n
    The default render mode is \ref GTK_RTG_RENDER_SOLID. \n
    The \c 'render-event' signal will be emitted after transformation matrix is pushed to the model-view matrix stack. \n
    After signal is emitted the render modes \ref GTK_RTG_RENDER_POINTS and \ref GTK_RTG_RENDER_EDGES are processed by the function.
    \param prim - A \c GtkRtgPrimitive.
    \param mode - A render mode to process.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_render(GtkRtgPrimitive *prim, GtkRtgPrimitiveRenderMode mode=GTK_RTG_RENDER_SOLID) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Sets up a texture for the texture unit. \n
    If there is a \c texture already set for the \c unit it will be unreferenced and replaced by the new one. \n
    The default unit is 0.
    \param prim - A \c GtkRtgPrimitive.
    \param texture - A \c GtkRtgTexture to set.
    \param unit - A unit to set the texture on (supported units up to \ref gtk_rtg_texture_map_get_max_texture_units).
    \attention - In case \c primitive fails on integrity check, or a \c NULL is passed for \c texture, or \c unit exceeds the number of maximum texture units a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_set_texture(GtkRtgPrimitive *prim, GtkRtgTexture *texture, guint unit=0) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Returns a texture from the texture unit. \n
    The default unit is 0.
    \param prim - A \c GtkRtgPrimitive.
    \param unit - A unit to get the texture from (supported units up to \ref gtk_rtg_texture_map_get_max_texture_units).
    \return - A \c GtkRtgTexture or \c NULL.
    \attention - In case \c primitive fails on integrity check, or \c unit exceeds the number of maximum texture units a \ref GtkRtgException will be thrown.
*/
GtkRtgTexture *gtk_rtg_primitive_get_texture(GtkRtgPrimitive *prim, guint unit=0) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Returns a color id. \n
    The unique color id is associated with the primitive per instantiation. The \ref GtkRtgTriMesh uses the primitives color id to
    implement \ref GTK_RTG_RENDER_SELECT mode.
    \param prim - A \c GtkRtgPrimitive.
    \return - A \c guint.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
guint gtk_rtg_primitive_get_color_id(GtkRtgPrimitive *prim) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Calculates the \ref GtkRtgPrimitiveBBox and the \ref GtkRtgPrimitiveBSphere. \n
    It is mandatory to calculate both the \ref GtkRtgPrimitiveBBox and \ref GtkRtgPrimitiveBBox when
    sub-classing the \c GtkRtgPrimitive in order to be used in \ref GtkRtgScene.
    \param prim - A \c GtkRtgPrimitive.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_calc_bbox_bsphere(GtkRtgPrimitive *prim) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Returns an axillary aligned bounding box (AABB).
    \param prim - A \c GtkRtgPrimitive.
    \return - A \c GtkRtgPrimitiveBBox.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
GtkRtgPrimitiveBBox *gtk_rtg_primitive_get_bbox(GtkRtgPrimitive *prim) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Returns a bounding sphere.
    \param prim - A \c GtkRtgPrimitive.
    \return - A \c GtkRtgPrimitiveBSphere.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
GtkRtgPrimitiveBSphere	*gtk_rtg_primitive_get_bsphere(GtkRtgPrimitive *prim) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Sets up a selection. \n
    The selection is ignored if primitive is frozen.
    \param prim - A \c GtkRtgPrimitive.
    \param select - TRUE to select primitive, or FALSE to deselect.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_select(GtkRtgPrimitive *prim, gboolean select) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Checks up selection.
    \param prim - A \c GtkRtgPrimitive.
    \return - TRUE if primitive is selected, or FALSE if not.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
gboolean gtk_rtg_primitive_is_selected(GtkRtgPrimitive *prim) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Sets up a freeze.
    \param prim - A \c GtkRtgPrimitive.
    \param freeze - TRUE to freeze primitive, or FALSE to unfreeze.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_freeze(GtkRtgPrimitive *prim, gboolean freeze) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Checks up freeze.
    \param prim - A \c GtkRtgPrimitive.
    \return - TRUE if primitive is frozen, or FALSE if not.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
gboolean gtk_rtg_primitive_is_frozen(GtkRtgPrimitive *prim) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Sets up a visibility.
    \param prim - A \c GtkRtgPrimitive.
    \param visible - TRUE to show primitive, or FALSE to hide.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_set_visible(GtkRtgPrimitive *prim, gboolean visible) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Checks up visibility.
    \param prim - A \c GtkRtgPrimitive.
    \return - TRUE if primitive is shown, or FALSE if hidden.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
gboolean gtk_rtg_primitive_is_visible(GtkRtgPrimitive *prim) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Sets up a color.
    \param prim - A \c GtkRtgPrimitive.
    \param color - A \c GdkColor to set.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_set_color(GtkRtgPrimitive *prim, const GdkColor *color) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Returns a color.
    \param prim - A \c GtkRtgPrimitive.
    \return - A \c GdkColor.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
GdkColor *gtk_rtg_primitive_get_color(GtkRtgPrimitive *prim) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Sets up a pivot.
    \param prim - A \c GtkRtgPrimitive.
    \param pivot - A \ref gtkrtg::point3f to set.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_set_pivot(GtkRtgPrimitive *prim, gtkrtg::point3f *pivot) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Returns a pivot.
    \param prim - A \c GtkRtgPrimitive.
    \return - A \ref  gtkrtg::point3f.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
gtkrtg::point3f *gtk_rtg_primitive_get_pivot(GtkRtgPrimitive *prim) throw (GtkRtgException);


/*!
    \ingroup GtkRtgPrimitive
    \brief Returns an axes ratio. \n
    The axes ratio is a ratio between three cardinal axis of an AABB. Perfect sphere would have an axes ratio of 1.0, where a ratio
    between cardinal axes x:y:z is 1:1:1. \n
    The axes ratio is used to distinguish which bounding volume better matches an object volume which it bounds.
    \param prim - A \c GtkRtgPrimitive.
    \return - A \c gfloat.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
gfloat gtk_rtg_primitive_get_axes_ratio(GtkRtgPrimitive *prim) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Sets up a name.
    \param prim - A \c GtkRtgPrimitive.
    \param name - A name to set.
    \attention - In case \c primitive fails on integrity check, or a \c NULL is passed for the \c name a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_set_name(GtkRtgPrimitive *prim, gchar *name) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Returns a name. \n
    The string is not owned by the library.
    \param prim - A \c GtkRtgPrimitive.
    \return - A string.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
gchar* gtk_rtg_primitive_get_name(GtkRtgPrimitive *prim) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Sets up a edit mode. \n
    The default mode is \ref GTK_RTG_EDIT_OBJECT.
    \param prim - A \c GtkRtgPrimitive.
    \param mode - A \ref GtkRtgPrimEditMode to set.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_set_edit_mode(GtkRtgPrimitive *prim, GtkRtgPrimEditMode mode=GTK_RTG_EDIT_OBJECT) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Returns an edit mode.
    \param prim - A \c GtkRtgPrimitive.
    \return - A \ref GtkRtgPrimEditMode.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
GtkRtgPrimEditMode gtk_rtg_primitive_get_edit_mode(GtkRtgPrimitive *prim) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Selects a vertex by index.
    \param prim - A \c GtkRtgPrimitive.
    \param idx - A vertex index to select.
    \attention - In case \c primitive fails on integrity check, or the \c idx is out of the vertex array boundaries a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_select_vertex(GtkRtgPrimitive *prim, guint_ind idx) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Selects vertices.
    \param prim - A \c GtkRtgPrimitive.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_vertices_select_all(GtkRtgPrimitive *prim) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Deselects vertices.
    \param prim - A \c GtkRtgPrimitive.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_primitive_vertices_deselect_all(GtkRtgPrimitive *prim) throw (GtkRtgException);

/*!
    \ingroup GtkRtgPrimitive
    \brief Returns a selected vertices. \n
    The array is owned by the library.
    \param prim - A \c GtkRtgPrimitive.
    \return - A \c std::set<guint_ind>.
    \attention - In case \c primitive fails on integrity check a \ref GtkRtgException will be thrown.
*/
std::set<guint_ind> *gtk_rtg_primitive_get_selected_vertices(GtkRtgPrimitive *prim) throw (GtkRtgException);

/* edges */
void gtk_rtg_primitive_select_edge(GtkRtgPrimitive *prim, guint_ind idx) throw (GtkRtgException);
void gtk_rtg_primitive_egdes_select_all(GtkRtgPrimitive *prim) throw (GtkRtgException);
void gtk_rtg_primitive_edges_deselect_all(GtkRtgPrimitive *prim) throw (GtkRtgException);
std::set<guint_ind> *gtk_rtg_primitive_get_selected_edges(GtkRtgPrimitive *prim) throw (GtkRtgException);

/* polys */
void gtk_rtg_primitive_select_poly(GtkRtgPrimitive *prim, guint_ind idx) throw (GtkRtgException);
void gtk_rtg_primitive_polys_select_all(GtkRtgPrimitive *prim) throw (GtkRtgException);
void gtk_rtg_primitive_polys_deselect_all(GtkRtgPrimitive *prim) throw (GtkRtgException);
std::set<guint_ind> *gtk_rtg_primitive_get_selected_polys(GtkRtgPrimitive *prim) throw (GtkRtgException);

G_END_DECLS

#endif /* GTK_RTG_PRIMITIVE_H */
