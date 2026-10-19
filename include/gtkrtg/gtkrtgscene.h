#ifndef GTK_RTG_SCENE_H
#define GTK_RTG_SCENE_H

#include <gtkrtg/gtkrtgbase.h>
#include <gtkrtg/gtkrtgexception.h>
#include <gtkrtg/gtkrtgprimitive.h>
#include <vector>

/*! \file */

G_BEGIN_DECLS

#define GTK_RTG_TYPE_SCENE            (gtk_rtg_scene_get_type())
#define GTK_RTG_SCENE(obj)		      (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_RTG_TYPE_SCENE, GtkRtgScene))
#define GTK_RTG_SCENE_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass),  GTK_RTG_TYPE_SCENE, GtkRtgSceneClass))
#define GTK_RTG_IS_SCENE(obj)		  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_RTG_TYPE_SCENE))
#define GTK_RTG_IS_SCENE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GTK_RTG_TYPE_SCENE))
#define GTK_RTG_SCENE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GTK_RTG_TYPE_SCENE, GtkRtgSceneClass))


typedef struct _GtkRtgScenePrivate GtkRtgScenePrivate;

/* max defaults.

f2 - shade selected faces toggle - will be implemented in mesh.
f3 - viewframe / smooth + highlight (edges, solid)
f4 - view edged faces toggle (edges)

*/

struct GtkRtgCamera;
struct GtkRtgCanvas;

struct GtkRtgScene
{
	GObject			 	 parent;
	GtkRtgScenePrivate	*priv;
};

struct GtkRtgSceneClass
{
	GObjectClass parent_class;

	/* signal(s) */
	void (* camera_default)	  (GtkRtgScene *scene, GtkRtgCamera *camera, GtkRtgCanvas *canvas);
	void (* prim_sel_changed) (GtkRtgScene *scene);
	void (* configured)	  	  (GtkRtgScene *scene);
};


/*! \defgroup GtkRtgScene GtkRtgScene
    \c GtkRtgScene is derived from the \c GObject. \n \n
    The \c GtkRtgScene is a container for the \ref GtkRtgPrimitive. The primitives are packed into scene in order to be displayed at the \ref GtkRtgCanvas.
    To enable interactivity with the scene objects at least one \ref GtkRtgCamera must be in the scene. The first camera set in the scene, or one of scene camera's which
    gets prioritized becomes a default scene camera. The default scene camera may be used by the \ref GtkRtgCanvas to be an active view camera. The scene can be pushed
    in more than one canvas in which case a scene camera's are prioritized for the specified canvas (see \ref gtk_rtg_camera_prioritize).

    \section properties Properties
    - \b toggle-edit-mode \ref GtkRtgPrimEditMode Write Only\n
    Toggle \ref GtkRtgPrimitive edit modes.\n
    It is used to alternate edit modes of single selected primitive. \n
    Additionally, the \b 'configured' signal will be emitted. \n

    \section signals Signals
    - \b camera-default \n
    user_function (scene, camera, canvas, user_data) Run Last / Action\n
    A scene emits this signal when first camera is pushed (\c canvas is NULL) or when one of scene camera's gets prioritized. \n
    It is used when \ref GtkRtgScene has more than one \ref GtkRtgCamera set to exchange between them. The prioritized camera becomes the scene's default camera which forwards
    it up to the \ref GtkRtgCanvas. \n
    The \c canvas uses the scenes default camera as it views operating camera.

    - \b configured \n
    user_function (scene, user_data) Run Last / Action\n
    A scene emits this signal when it gets configured (i.e. edit mode changes).

    - \b prim-sel-changed \n
    user_function (scene, user_data) Run Last / Action\n
    A scene emits this signal when primitive selection is changed.
*/


/** \ingroup GtkRtgScene
 *  GtkRtgSceneRenderMode
 *  \brief A scene's render modes.
 */
typedef enum
{
	GTK_RTG_RENDER_SCENE_SOLID,		  /**< solid mesh i.e. colorized or textured (depending on textures settings) */
	GTK_RTG_RENDER_SCENE_EDGES,		  /**< edge lines (wire-frame) */
	GTK_RTG_RENDER_SCENE_SOLID_EDGES, /**< solid mesh and a top edge lines (wire-frame) */

	/* end */
	GTK_RTG_RENDER_SCENE_MODES
}GtkRtgSceneRenderMode;

GType gtk_rtg_scene_get_type(void) G_GNUC_CONST;

/*!
    \ingroup GtkRtgScene
    \brief Creates a new \c GtkRtgScene.
    \return - A \c GtkRtgScene.
*/
GtkRtgScene	*gtk_rtg_scene_new();

/*!
    \ingroup GtkRtgScene
    \brief Pushes up a primitive into the scene.
    \param scene - A \c GtkRtgScene.
    \param prim - A \ref GtkRtgPrimitive to push.
    \attention - In case \c scene or \c prim fails on integrity check, or the \c prim is already pushed a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_scene_primitive_push(GtkRtgScene *scene, GtkRtgPrimitive *prim) throw (GtkRtgException);

/*!
    \ingroup GtkRtgScene
    \brief Pops up a primitive from the scene.
    \param scene - A \c GtkRtgScene.
    \param prim - A \ref GtkRtgPrimitive to pop.
    \attention - In case \c scene fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_scene_primitive_pop(GtkRtgScene *scene, GtkRtgPrimitive *prim) throw (GtkRtgException);

/*!
    \ingroup GtkRtgScene
    \brief Checks up whether a primitive is pushed into the scene.
    \param scene - A \c GtkRtgScene.
    \param prim - A \ref GtkRtgPrimitive to check.
    \return - \c TRUE if \c prim is pushed, or \c FALSE if not.
    \attention - In case \c scene or \c prim fails on integrity check a \ref GtkRtgException will be thrown.
*/
gboolean gtk_rtg_scene_primitive_is_pushed(GtkRtgScene *scene, GtkRtgPrimitive *prim) throw (GtkRtgException);

/*!
    \ingroup GtkRtgScene
    \brief Returns an array of frustum-culled primitives. \n
    The returned array is owned by the library.
    \param scene - A \c GtkRtgScene.
    \param camera - A camera to cull primitives or \c NULL to cull from scene default camera.
    \return - An array \c std::set<GtkRtgPrimitive*> or NULL if there is no camera to cull from.
    \attention - In case \c scene fails on integrity check a \ref GtkRtgException will be thrown.
*/
const std::set<GtkRtgPrimitive*> *gtk_rtg_scene_get_culled_prims(GtkRtgScene *scene, GtkRtgCamera *camera) throw (GtkRtgException);

/*!
    \ingroup GtkRtgScene
    \brief Returns an array of the primitives. \n
    The returned array is owned by the library.
    \param scene - A \c GtkRtgScene.
    \return - A \c std::set<GtkRtgPrimitive*>.
    \attention - In case \c scene fails on integrity check a \ref GtkRtgException will be thrown.
*/
const std::set<GtkRtgPrimitive*> *gtk_rtg_scene_get_all_prims(GtkRtgScene *scene) throw (GtkRtgException);

/*!
    \ingroup GtkRtgScene
    \brief Render a whole scene or culled part of a scene. \n
    In case \c camera is not found in the scene map the whole scene will be rendered. \n
    The default render mode is \ref GTK_RTG_RENDER_SCENE_SOLID.
    \param scene - A \c GtkRtgScene.
    \param mode - A render mode to process.
    \param camera - A camera to render culled primitives or \c NULL to render whole scene.
    \attention - In case \c scene fails on integrity check a \ref GtkRtgException will be thrown.
*/
void  gtk_rtg_scene_render(GtkRtgScene *scene, GtkRtgSceneRenderMode mode = GTK_RTG_RENDER_SCENE_SOLID,
						   GtkRtgCamera *camera = NULL) throw (GtkRtgException);

/* handy macros */
#define IS_BACKFACE_SEL(scene)	   (gtk_rtg_scene_is_backface_selection(GTK_RTG_SCENE(scene)))
#define IS_SCENE_PRIM(scene, prim) (gtk_rtg_scene_primitive_is_pushed(GTK_RTG_SCENE(scene), prim))
#define DESELECT(scene, prim)	   (gtk_rtg_scene_deselect(GTK_RTG_SCENE(scene), prim))
#define FIRST_SELECTED(scene)	   (*gtk_rtg_scene_get_selected(GTK_RTG_SCENE(scene))->begin())
#define SELECTED(scene)		   	   (gtk_rtg_scene_get_selected(GTK_RTG_SCENE(scene))->size())
#define CULLED_PRIMS(scene, cam)   (gtk_rtg_scene_get_culled_prims(GTK_RTG_SCENE(scene), cam))
#define ALL_PRIMS(scene)	   	   (gtk_rtg_scene_get_all_prims(GTK_RTG_SCENE(scene)))

/*!
    \ingroup GtkRtgScene
    \brief Toggle \ref GtkRtgPrimitive edit modes. \n
    It is used to alternate edit modes of single selected primitive. \n
    Additionally, the \b 'configured' signal will be emitted.
    \param scene - A \c GtkRtgScene.
    \param mode - An edit mode to toggle on.
    \attention - In case \c scene fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_scene_toggle_edit_mode(GtkRtgScene *scene, GtkRtgPrimEditMode mode) throw (GtkRtgException);

/*!
    \ingroup GtkRtgScene
    \brief Performs up a single selection. \n
    This method is used to select single primitive or primitive's vertex, edge or polygon depending on selected primitive's edit mode. \n
    If there is already a single primitive in selection (see \ref gtk_rtg_scene_get_selected) the method will first inspect the primitive's
    edit mode and perform a selection of the attribute (\ref GTK_RTG_EDIT_POINTS, \ref GTK_RTG_EDIT_EDGES or \ref GTK_RTG_EDIT_POLYS) listed in the mode.
    If the edit mode is \ref GTK_RTG_EDIT_OBJECT then it is possible to select a primitive.
    It is mandatory to supply a \c canvas with default camera set.
    \param scene - A \c GtkRtgScene.
    \param point - A screen point.
    \param canvas - A \ref GtkRtgCanvas on which selection is performed.
    \param test - \c TRUE to test without selection, or \c FALSE to ignore test.
    \return For object selection: Returns a selected primitive's color id, or -1 if nothing is selected. \n
	    For sub-primitive selection: Returns a selected primitive's color id, or 0 if nothing is selected.
    \attention - In case \c scene or \c canvas fails on integrity check, or \c canvas default camera is \c NULL a \ref GtkRtgException will be thrown.
*/
gint gtk_rtg_scene_point_select(GtkRtgScene *scene, GdkPoint &point, GtkRtgCanvas *canvas, gboolean test=FALSE) throw (GtkRtgException);

/*!
    \ingroup GtkRtgScene
    \brief Performs up a multiple selection. \n
    This method is used to select multiple scene primitives or primitive's vertices, edges or polygons depending on selected primitive's edit mode. \n
    If there is already a single primitive in selection (see \ref gtk_rtg_scene_get_selected) the method will first inspect the primitive's
    edit mode and perform a selection of the attributes (\ref GTK_RTG_EDIT_POINTS, \ref GTK_RTG_EDIT_EDGES or \ref GTK_RTG_EDIT_POLYS) listed in the mode. \n
    If the edit mode is \ref GTK_RTG_EDIT_OBJECT then it is possible to select a primitives.
    It is mandatory to supply a \c canvas with default camera set.
    \param scene - A \c GtkRtgScene.
    \param start - A screen start point.
    \param end - A screen end point.
    \param canvas - A \ref GtkRtgCanvas on which selection is performed.
    \return For object selection: Returns a count of primitives, or -1 if nothing is selected. \n
	    For sub-primitive selection: Returns a count of vertices, edges or polygons selected.
    \attention - In case \c scene or \c canvas fails on integrity check, or \c canvas default camera is \c NULL a \ref GtkRtgException will be thrown.
*/
gint gtk_rtg_scene_marquee_select(GtkRtgScene *scene, GdkPoint &start, GdkPoint &end, GtkRtgCanvas *canvas) throw (GtkRtgException);

/*!
    \ingroup GtkRtgScene
    \brief Deselects all primitives.
    \param scene - A \c GtkRtgScene.
    \return \c TRUE if operation succeeded or \c FALSE if nothing is deselected.
    \attention - In case \c scene fails on integrity check a \ref GtkRtgException will be thrown.
*/
gboolean gtk_rtg_scene_deselect_all(GtkRtgScene *scene)	throw (GtkRtgException);

/*!
    \ingroup GtkRtgScene
    \brief Returns an array of selected primitives.
    \param scene - A \c GtkRtgScene.
    \return A \c std::set<GtkRtgPrimitive*>.
    \attention - In case \c scene fails on integrity check a \ref GtkRtgException will be thrown.
*/
const std::set<GtkRtgPrimitive*> *gtk_rtg_scene_get_selected(GtkRtgScene *scene) throw (GtkRtgException);

/*!
    \ingroup GtkRtgScene
    \brief Deselects a single primitive.
    \param scene - A \c GtkRtgScene.
    \param prim - A \c GtkRtgPrimitive to deselect.
    \return \c TRUE if operation succeeded or \c FALSE if nothing is deselected.
    \attention - In case \c scene fails on integrity check a \ref GtkRtgException will be thrown.
*/
gboolean gtk_rtg_scene_deselect(GtkRtgScene *scene, GtkRtgPrimitive *prim) throw (GtkRtgException);

/*!
    \ingroup GtkRtgScene
    \brief Enable or disable a back-face selection. \n
    By default the scene selection (\ref gtk_rtg_scene_point_select and \ref gtk_rtg_scene_marquee_select) will not include back-face hits (vertices, edges and polys) in selection.
    \param scene - A \c GtkRtgScene.
    \param backface - \c TRUE to include backface in selection, or \c FALSE to ignore.
    \attention - In case \c scene fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_scene_select_backface(GtkRtgScene *scene, gboolean backface=TRUE) throw (GtkRtgException);

/*!
    \ingroup GtkRtgScene
    \brief Checks up whether a backface selection is enabled.
    \param scene - A \c GtkRtgScene.
    \return - \c TRUE if backface is enabled, or \c FALSE if not.
    \attention - In case \c scene fails on integrity check a \ref GtkRtgException will be thrown.
*/
gboolean gtk_rtg_scene_is_backface_selection(GtkRtgScene *scene) throw (GtkRtgException);

/*!
    \ingroup GtkRtgScene
    \brief Returns a scene default camera.
    \param scene - A \c GtkRtgScene.
    \return - A \c GtkRtgCamera or \c NULL.
    \attention - In case \c scene fails on integrity check a \ref GtkRtgException will be thrown.
*/
GtkRtgCamera *gtk_rtg_scene_camera_get_default(GtkRtgScene *scene) throw (GtkRtgException);

GtkRtgPrimitive *gtk_rtg_scene_get_primitive(GtkRtgScene *scene, guint color_id) throw (GtkRtgException);

G_END_DECLS

#endif /* GTK_RTG_SCENE_H */
