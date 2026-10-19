#ifndef GTK_RTG_CANVAS_H
#define GTK_RTG_CANVAS_H

#include <gtk/gtk.h>
#include <gtkrtg/gtkrtgbase.h>
#include <gtkrtg/gtkrtgexception.h>
#include <gtkrtg/gtkrtgcanvasgrid.h>
#include <gtkrtg/gtkrtgtext.h>
#include <gtkrtg/gtkrtgmath.h>

/*! \file */

G_BEGIN_DECLS

#define GTK_RTG_TYPE_CANVAS             (gtk_rtg_canvas_get_type())
#define GTK_RTG_CANVAS(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_RTG_TYPE_CANVAS, GtkRtgCanvas))
#define GTK_RTG_CANVAS_CLASS(klass)	    (G_TYPE_CHECK_CLASS_CAST ((klass),  GTK_RTG_TYPE_CANVAS, GtkRtgCanvasClass))
#define GTK_RTG_IS_CANVAS(obj)		    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_RTG_TYPE_CANVAS))
#define GTK_RTG_IS_CANVAS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass),  GTK_RTG_TYPE_CANVAS))
#define GTK_RTG_CANVAS_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),  GTK_RTG_TYPE_CANVAS, GtkRtgCanvasClass))

typedef struct _GtkRtgCanvasClass	GtkRtgCanvasClass;
typedef struct _GtkRtgCanvasPrivate	GtkRtgCanvasPrivate;


struct GtkRtgCanvas
{
	GtkDrawingArea		 parent;
	GtkRtgCanvasPrivate	*priv;
};


struct _GtkRtgCanvasClass
{
	GtkDrawingAreaClass parent_class;

	void (* focused)	  	  (GtkRtgCanvas *canvas);
	void (* render_offscreen) (GtkRtgCanvas *canvas);
	void (* gizmo_position)	  (GtkRtgCanvas *canvas, gtkrtg::point3f *pos);
};

struct GtkRtgCamera;
struct GtkRtgScene;


/*! \defgroup GtkRtgCanvas GtkRtgCanvas
     \c GtkRtgCanvas is \c GtkDrawingArea widget with OpenGL capabilities. \n \n
     The \c GtkRtgCanvas is a surface to render 2D/3D geometric primitives and to interact with. \n
     The canvas can be used in let say real-time and editor modes. The real-time mode targets to be used in a way more like SDL, where adequate callbacks are added / registered,
     and their implementations are user controlled. \n
     In editor, or interactive mode, the canvas is intended to be used as front-end for editing and rendering scene objects. \n
     Moreover, the real-time and editor modes should not be considered as two distinct ways of using an \c GtkRtgCanvas. It is recommended to mix them. \n
     For example, if you want to setup lighting for your scene you should use 'init' and 'render' callbacks to do that. \n
     By default canvas cames without camera support. To enable interactive viewing (pan, zoom, rotate) at least one \ref GtkRtgCamera should be in a \ref GtkRtgScene,
     which is pushed into the canvas. The canvas uses the scene's default camera as it views operating camera.

     <br><br>
     \image html gtkrtgcanvas.png
     <br>
     <center>\c GtkRtgCanvas in 'editor mode'. </center>
     <br>
     <br>

  \section properties Properties
  - \b bg-color \c GdkColor Read / Write\n
    Background color.\n
    Default Value: \c 'darkslategray'

    - \b render-offscreen \n
    user_function (canvas, user_data) Run Last / Action\n
    The canvas emits this signal after the scene is rendered for the picking (offscreen buffer).

    - \b gizmo-position \n
    user_function (canvas, point, user_data) Run Last / Action\n
    The canvas emits this signal after the scene is rendered for the picking (offscreen buffer).
*/


/** \ingroup GtkRtgCanvas
 *  GtkRtgCanvasViewPredefined
 *  \brief The predefined view angles / projection.
 */
typedef enum
{
	GTK_RTG_CANVAS_VIEW_FRONT,	/**< Orthographic front view.  */
	GTK_RTG_CANVAS_VIEW_BACK,	/**< Orthographic back view.   */
	GTK_RTG_CANVAS_VIEW_TOP,	/**< Orthographic top view.    */
	GTK_RTG_CANVAS_VIEW_BOTTOM,	/**< Orthographic bottom view. */
	GTK_RTG_CANVAS_VIEW_LEFT,	/**< Orthographic left view.   */
	GTK_RTG_CANVAS_VIEW_RIGHT,	/**< Orthographic right	view.  */
    GTK_RTG_CANVAS_VIEW_CUSTOM	/**< Perspective custom view.  */
}GtkRtgCanvasViewPredefined;

/** \ingroup GtkRtgCanvas
 *  GtkRtgCanvasViewUp
 *  \brief The predefined view up vector orientations.
 */
typedef enum
{
	GTK_RTG_CANVAS_VIEW_UP_Y,	/**< Up vector in +y direction. */
	GTK_RTG_CANVAS_VIEW_UP_Z	/**< Up vector in +z direction. */
}GtkRtgCanvasViewUp;

/** \ingroup GtkRtgCanvas
 *  GtkRtgCanvasInteractiveMode
 *  \brief The canvas interactive modes.
 */
typedef enum
{
	GTK_RTG_CANVAS_INT_MODE_OFF,	 /**< Interactive mode off.                     */
	GTK_RTG_CANVAS_INT_MODE_VIEW,	 /**< View control mode (Zoom, Pan and Rotate). */
	GTK_RTG_CANVAS_INT_MODE_SELECT,  /**< Select object or sub-object mode.         */
    GTK_RTG_CANVAS_INT_MODE_MOVE,    /**< Move object or sub-object mode.           */
    GTK_RTG_CANVAS_INT_MODE_SCALE,   /**< Scale object or sub-object mode.          */
    GTK_RTG_CANVAS_INT_MODE_ROTATE   /**< Rotate object or sub-object mode.         */
}GtkRtgCanvasInteractiveMode;


/** \ingroup GtkRtgCanvas
 *  \brief Callback function pointer prototype
 */
typedef void (*GTK_RTG_FUNCT_PURE)	(gpointer);

/** \ingroup GtkRtgCanvas
 *  \brief Callback function pointer prototype
 */
typedef void (*GTK_RTG_FUNCT_OBJECT)	(GtkRtgCanvas *, gpointer);

/** \ingroup GtkRtgCanvas
 *  \brief Callback function pointer prototype
 */
typedef void (*GTK_RTG_FUNCT_RESIZE)	(gint, gint, gpointer);

GType gtk_rtg_canvas_get_type	(void) G_GNUC_CONST;

/*!
    \ingroup GtkRtgCanvas
    \brief Creates a new \c GtkRtgCanvas.
    \return - A \c GtkWidget.
    \attention - In case a frame-buffer fails to configure a \ref GtkRtgException will be thrown.
*/
GtkWidget* gtk_rtg_canvas_new() throw (GtkRtgException);

/* Make this canvas's compatibility context current for client OpenGL calls. */
gboolean gtk_rtg_canvas_make_current(GtkRtgCanvas *canvas);


/* R.T.G callbacks */

/*!
    \ingroup GtkRtgCanvas
    \brief Sets up a initialization function handler. \n
     A function to be called after the canvas is realized and OpenGL context created. Therefore, it is suitable for any of OpenGL initializations.
    \param canvas - A \c GtkRtgCanvas.
    \param funct - A \ref GTK_RTG_FUNCT_OBJECT function to set.
    \param data - A pointer to user data or \c NULL.
    \attention - In case \c canvas fails on integrity check, or a \c NULL is passed for \c funct a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_init_funct_add(GtkRtgCanvas *canvas, GTK_RTG_FUNCT_OBJECT funct, gpointer data) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvas
    \brief Sets up a resize function handler. \n
     A function to be called whenever canvas is (re)configured (i.e. startup, resize). \n
     The OpenGL viewport will be settled before handler invokes.
    \param canvas - A \c GtkRtgCanvas.
    \param funct - A \ref GTK_RTG_FUNCT_RESIZE function to set.
    \param data - A pointer to user data or \c NULL.
    \attention - In case \c canvas fails on integrity check, or a \c NULL is passed for \c funct a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_resize_funct_add (GtkRtgCanvas *canvas, GTK_RTG_FUNCT_RESIZE funct, gpointer data) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvas
    \brief Sets up a render function handler. \n
     A function to be called whenever canvas is rendered (i.e. reqular or forced expose, timer etc.). \n
     The scene objects will be rendered before handler invokes.
    \param canvas - A \c GtkRtgCanvas.
    \param funct - A \ref GTK_RTG_FUNCT_PURE function to set.
    \param data - A pointer to user data or \c NULL.
    \attention - In case \c canvas fails on integrity check, or a \c NULL is passed for \c funct a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_render_funct_add (GtkRtgCanvas *canvas, GTK_RTG_FUNCT_PURE   funct, gpointer data) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvas
    \brief Sets up a destroy function handler. \n
     A function to be called when canvas is unrealized. \n
     The OpenGL context will be presented, therefore it is suitable for any of OpenGL un-initializations / cleanups.
    \param canvas - A \c GtkRtgCanvas.
    \param funct - A \ref GTK_RTG_FUNCT_OBJECT function to set.
    \param data - A pointer to user data or \c NULL.
    \attention - In case \c canvas fails on integrity check, or a \c NULL is passed for \c funct a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_destroy_funct_add(GtkRtgCanvas *canvas, GTK_RTG_FUNCT_OBJECT funct, gpointer data) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvas
    \brief Sets up a timeout function handler. \n
     A function to be called at regular intervals. It is suitable for the animation (run with high-priority). \n
     The canvas will be rendered after handler invokes.
    \param canvas - A \c GtkRtgCanvas.
    \param funct - A \ref GTK_RTG_FUNCT_PURE function to set.
    \param interval - A time interval between calls to the function, in milliseconds (1/1000ths of a second).
    \param animate - \c TRUE if the function will be called (at the end of the first interval), or \c FALSE to postpone.
    \param data - A pointer to user data or \c NULL.
    \attention - In case \c canvas fails on integrity check, or a \c NULL is passed for \c funct a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_timeout_funct_add(GtkRtgCanvas *canvas, GTK_RTG_FUNCT_PURE funct,
									  guint interval, gboolean animate, gpointer data) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvas
    \brief Starts up the animation. \n
     The first call to the 'timeout function handler' will be at the end of the first \c interval.
    \param canvas - A \c GtkRtgCanvas.
    \attention - In case \c canvas fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_animation_start(GtkRtgCanvas *canvas) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvas
    \brief Stops the animation. \n
     Immediately stops calling the 'timeout function handler'.
    \param canvas - A \c GtkRtgCanvas.
    \attention - In case \c canvas fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_animation_stop(GtkRtgCanvas *canvas) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvas
    \brief Checks up whether an animation is ongoing.
    \param canvas - A \c GtkRtgCanvas.
    \return - \c TRUE if an animation is ongoing, or \c FALSE if not.
    \attention - In case \c canvas fails on integrity check a \ref GtkRtgException will be thrown.
*/
gboolean gtk_rtg_canvas_is_animated(GtkRtgCanvas *canvas) throw (GtkRtgException);

/* Grid */

/*!
    \ingroup GtkRtgCanvas
    \brief Pushes up a grid into the canvas. \n
     If there is a \c grid already pushed it will be popped and replaced by the new one.
    \param canvas - A \c GtkRtgCanvas.
    \param grid - A \ref GtkRtgCanvasGrid to push.
    \attention - In case \c canvas or \c grid fails on integrity check, or the \c grid is already pushed a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_grid_push(GtkRtgCanvas *canvas, GtkRtgCanvasGrid *grid) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvas
    \brief Pops up a grid from the canvas.
    \param canvas - A \c GtkRtgCanvas.
    \attention - In case \c canvas fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_grid_pop(GtkRtgCanvas *canvas) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvas
    \brief Checks up whether a grid is pushed into the canvas.
    \param canvas - A \c GtkRtgCanvas.
    \param grid - A \ref GtkRtgCanvasGrid to check.
    \return - \c TRUE if \c grid is pushed, or \c FALSE if not.
    \attention - In case \c canvas or \c grid fails on integrity check a \ref GtkRtgException will be thrown.
*/
gboolean gtk_rtg_canvas_grid_is_pushed(GtkRtgCanvas *canvas, GtkRtgCanvasGrid *grid) throw (GtkRtgException);

/* Text */
/*!
    \ingroup GtkRtgCanvas
    \brief Pushes up a text into the canvas. \n
     Font will be queried from the font map. If not found, new font type will be registered and associated with the text.
    \param canvas - A \c GtkRtgCanvas.
    \param text - A \ref GtkRtgText to push.
    \attention - In case \c canvas or \c text fails on integrity check, or the \c text is already pushed a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_text_push(GtkRtgCanvas *canvas, GtkRtgText *text) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvas
    \brief Pops up a text from the canvas.
    \param canvas - A \c GtkRtgCanvas.
    \param text - A \ref GtkRtgText to pop.
    \attention - In case \c canvas or \c text fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_text_pop(GtkRtgCanvas *canvas, GtkRtgText *text) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvas
    \brief Checks up whether a text is pushed into the canvas.
    \param canvas - A \c GtkRtgCanvas.
    \param text - A \ref GtkRtgText to check.
    \return - \c TRUE if \c text is pushed, or \c FALSE if not.
    \attention - In case \c canvas or \c text fails on integrity check a \ref GtkRtgException will be thrown.
*/
gboolean gtk_rtg_canvas_text_is_pushed(GtkRtgCanvas *canvas, GtkRtgText *text) throw (GtkRtgException);

/* Scene */

/*!
    \ingroup GtkRtgCanvas
    \brief Pushes up a scene into the canvas. \n
    If there is a \c scene already pushed it will be popped and replaced by the new one.
    \param canvas - A \c GtkRtgCanvas.
    \param scene - A \ref GtkRtgScene to push.
    \attention - In case \c canvas or \c scene fails on integrity check, or the \c scene is already pushed a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_scene_push(GtkRtgCanvas *canvas, GtkRtgScene *scene) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvas
    \brief Pops up a scene from the canvas.
    \param canvas - A \c GtkRtgCanvas.
    \attention - In case \c canvas fails on integrity check a \ref GtkRtgException will be thrown.
*/
void  gtk_rtg_canvas_scene_pop(GtkRtgCanvas *canvas) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvas
    \brief Checks up whether a scene is pushed into the canvas.
    \param canvas - A \c GtkRtgCanvas.
    \param scene - A \ref GtkRtgScene to check.
    \return - \c TRUE if \c text is pushed, or \c FALSE if not.
    \attention - In case \c canvas or \c scene fails on integrity check a \ref GtkRtgException will be thrown.
*/
gboolean gtk_rtg_canvas_scene_is_pushed(GtkRtgCanvas *canvas, GtkRtgScene *scene) throw (GtkRtgException);


/*
 * View
 *
 */

/* interactive modes */


/*!
    \ingroup GtkRtgCanvas
    \brief Sets up a interactive canvas mode. \n
    The default mode is \ref GTK_RTG_CANVAS_INT_MODE_OFF.
    \param canvas - A \c GtkRtgCanvas.
    \param mode - A \ref GtkRtgCanvasInteractiveMode to set.
    \attention - In case \c canvas fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_interactive_set_mode(GtkRtgCanvas *canvas, GtkRtgCanvasInteractiveMode mode) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvas
    \brief Returns a interactive canvas mode.
    \param canvas - A \c GtkRtgCanvas.
    \return - A \ref GtkRtgCanvasInteractiveMode.
    \attention - In case \c canvas fails on integrity check a \ref GtkRtgException will be thrown.
*/
GtkRtgCanvasInteractiveMode	gtk_rtg_canvas_interactive_get_mode(GtkRtgCanvas *canvas) throw (GtkRtgException);

/* gizmo show on/off */

/*!
    \ingroup GtkRtgCanvas
    \brief Whether to show or hide a views gizmo. \n
    This function has effect only if interactive view mode is set to \ref GTK_RTG_CANVAS_INT_MODE_VIEW.
    \param canvas - A \c GtkRtgCanvas.
    \param gizmo - \c TRUE to show, or \c FALSE to hide gizmo.
    \attention - In case \c canvas fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_view_gizmo_show(GtkRtgCanvas *canvas, gboolean gizmo) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvas
    \brief Checks up whether a views gizmo is shown.
    \param canvas - A \c GtkRtgCanvas.
    \return - \c TRUE if gizmo is shown, or \c FALSE if hidden.
    \attention - In case \c canvas fails on integrity check a \ref GtkRtgException will be thrown.
*/
gboolean gtk_rtg_canvas_view_gizmo_is_showed(GtkRtgCanvas *canvas) throw (GtkRtgException);

/* view setup */

/*!
    \ingroup GtkRtgCanvas
    \brief Sets up a predefined views angle. \n
    The default mode is \ref GTK_RTG_CANVAS_VIEW_CUSTOM. \n
    This function has effect only if \ref GtkRtgScene containing at least one \ref GtkRtgCamera is already pushed into the canvas. \n
    An orthographic projection is used for non-custom modes.
    \param canvas - A \c GtkRtgCanvas.
    \param predefined - A \ref GtkRtgCanvasViewPredefined to set.
    \attention - In case \c canvas fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_view_set_predefined(GtkRtgCanvas *canvas, GtkRtgCanvasViewPredefined predefined) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvas
    \brief Returns a predefined views angle.
    \param canvas - A \c GtkRtgCanvas.
    \return - A \ref GtkRtgCanvasViewPredefined.
    \attention - In case \c canvas fails on integrity check a \ref GtkRtgException will be thrown.
*/
GtkRtgCanvasViewPredefined gtk_rtg_canvas_view_get_predefined(GtkRtgCanvas *canvas) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvas
    \brief Sets up a views up vector direction. \n
    The default direction is \ref GTK_RTG_CANVAS_VIEW_UP_Y.
    \param canvas - A \c GtkRtgCanvas.
    \param up - A \ref GtkRtgCanvasViewUp to set.
    \attention - In case \c canvas fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_view_set_up(GtkRtgCanvas *canvas, GtkRtgCanvasViewUp up) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvas
    \brief Returns a views up vector.
    \param canvas - A \c GtkRtgCanvas.
    \return - A \ref GtkRtgCanvasViewUp.
    \attention - In case \c canvas fails on integrity check a \ref GtkRtgException will be thrown.
*/
GtkRtgCanvasViewUp gtk_rtg_canvas_view_get_up(GtkRtgCanvas *canvas) throw (GtkRtgException);


/* utils */

/*!
    \ingroup GtkRtgCanvas
    \brief Forces a canvas to be rendered. \n
    If \ref GtkRtgScene is pushed into the canvas it will trigger render handlers of all scene primitives (including \ref GtkRtgText).
    \param canvas - A \c GtkRtgCanvas.
    \attention - In case \c canvas fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_render(GtkRtgCanvas *canvas) throw (GtkRtgException);


/* camera */
GtkRtgCamera  *gtk_rtg_canvas_camera_get_default(GtkRtgCanvas *canvas) throw (GtkRtgException);


/* undocumented */

GtkRtgCanvasGrid *gtk_rtg_canvas_get_grid(GtkRtgCanvas *canvas) throw (GtkRtgException);
gboolean gtk_rtg_canvas_view_zoom_extents(GtkRtgCanvas *canvas, gboolean selected = FALSE) throw (GtkRtgException);
void gtk_rtg_canvas_set_focused(GtkRtgCanvas *canvas, gboolean focused) throw (GtkRtgException);
gboolean gtk_rtg_canvas_is_focused(GtkRtgCanvas *canvas) throw (GtkRtgException);
void gtk_rtg_canvas_gizmo_update_position(GtkRtgCanvas *canvas, gtkrtg::point3f *pos) throw (GtkRtgException);

G_END_DECLS

#endif /* GTK_RTG_CANVAS_H */
