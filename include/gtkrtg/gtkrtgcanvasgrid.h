#ifndef GTK_RTG_CANVAS_GRID_H
#define GTK_RTG_CANVAS_GRID_H

#include <gtkrtg/gtkrtgbase.h>
#include <gtkrtg/gtkrtgexception.h>
#include <gtkrtg/gtkrtgmath.h>

/*! \file */

G_BEGIN_DECLS

#define GTK_RTG_TYPE_CANVAS_GRID            (gtk_rtg_canvas_grid_get_type())
#define GTK_RTG_CANVAS_GRID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_RTG_TYPE_CANVAS_GRID, GtkRtgCanvasGrid))
#define GTK_RTG_CANVAS_GRID_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass),  GTK_RTG_TYPE_CANVAS_GRID, GtkRtgCanvasGridClass))
#define GTK_RTG_IS_CANVAS_GRID(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_RTG_TYPE_CANVAS_GRID))
#define GTK_RTG_IS_CANVAS_GRID_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass),  GTK_RTG_TYPE_CANVAS_GRID))
#define GTK_RTG_CANVAS_GRID_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),  GTK_RTG_TYPE_CANVAS_GRID, GtkRtgCanvasGridClass))

typedef struct _GtkRtgCanvasGrid        GtkRtgCanvasGrid;
typedef struct _GtkRtgCanvasGridClass	GtkRtgCanvasGridClass;
typedef struct _GtkRtgCanvasGridPrivate	GtkRtgCanvasGridPrivate;

struct GtkRtgCanvas;

struct _GtkRtgCanvasGrid
{
	GObject			 		 parent;
	GtkRtgCanvasGridPrivate	*priv;
};

struct _GtkRtgCanvasGridClass
{
	GObjectClass parent_class;
};

/*! \defgroup GtkRtgCanvasGrid GtkRtgCanvasGrid
    \c GtkRtgCanvasGrid is derived from the \c GtkObject. \n \n
    The \c GtkRtgCanvasGrid is an object which represents a grid. A grid is a two-dimensional structure made up of a series of intersecting vertical and horizontal axes
    used to structure (2D/3D world). The grid serves as an armature on which a primitives and text are organized in a rational, easy to absorb manner. The origin of the grids
    axes is always at (0,0,0). The scale between lines is in units which is a measure of spacing between neighbor lines. A grid is used in combination with \ref GtkRtgCanvas
    which is used to display it.

    <br><br>
    \image html gtkrtgcanvasgrid.png
    <br>
    <center>\c GtkRtgCanvaGrid displayed on \ref GtkRtgCanvas. </center>
    <br>
    <br>

    \section properties Properties
    - \b grid-lines \c gint Read / Write\n
    The number of grids vertical/horizontal lines. \n
    Default Value: 15

    - \b line-spacing \c gfloat Read / Write\n
    Distance between neighbor lines in units. \n
    Default Value: 1.0

    - \b line-width \c gfloat Read / Write\n
    The width of grid line. \n
    Default Value: 1.0

    - \b line-color \c GdkColor Read / Write\n
    The color of a grid lines. \n
    Default Value: 'lightslategray'

    - \b cross-line-color \c GdkColor Read / Write\n
    The color of a cross lines. \n
    Default Value: 'darkslateblue'

    - \b  fade-line-color \c gboolean Read / Write\n
    Whether the lines color should fade, depending on distance from viewer to the grid. \n
    Default Value: \c TRUE

    - \b  snap-cursor \c gboolean Read / Write\n
    Whether the draw snap cursor. \n
    Default Value: \c FALSE
*/

GType gtk_rtg_canvas_grid_get_type(void) G_GNUC_CONST;

/*!
    \ingroup GtkRtgCanvasGrid
    \brief Creates a new \c GtkRtgCanvasGrid.
    \return - A \c GtkRtgCanvasGrid.
*/
GtkRtgCanvasGrid *gtk_rtg_canvas_grid_new();

/*!
    \ingroup GtkRtgCanvasGrid
    \brief Renders a grid.
    \param grid - A \c GtkRtgCanvasGrid.
    \param canvas - A \ref GtkRtgCanvas to render a grid.
    \attention - In case \c grid, or \c canvas fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_grid_render(GtkRtgCanvasGrid *grid, GtkRtgCanvas *canvas) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvasGrid
    \brief Sets up a number of lines. \n
    Default number of lines is 15.
    \param grid - A \c GtkRtgCanvasGrid.
    \param lines - A number of vertical/horizontal lines (lines >= 3).
    \attention - In case \c grid fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_grid_set_lines(GtkRtgCanvasGrid *grid, gint lines) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvasGrid
    \brief Returns a number of lines.
    \param grid - A \c GtkRtgCanvasGrid.
    \return - A \c gint.
    \attention - In case \c grid fails on integrity check a \ref GtkRtgException will be thrown.
*/
gint gtk_rtg_canvas_grid_get_lines(GtkRtgCanvasGrid *grid) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvasGrid
    \brief Sets up a lines width.\n
    Default width is 1.0.
    \param grid - A \c GtkRtgCanvasGrid.
    \param width - A width of vertical/horizontal lines (lines >= 1.0).
    \attention - In case \c grid fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_grid_set_line_width(GtkRtgCanvasGrid *grid, gfloat width) throw (GtkRtgException);


/*!
    \ingroup GtkRtgCanvasGrid
    \brief Returns width of a line.
    \param grid - A \c GtkRtgCanvasGrid.
    \return - A \c gfloat.
    \attention - In case \c grid fails on integrity check a \ref GtkRtgException will be thrown.
*/
gfloat gtk_rtg_canvas_grid_get_line_width(GtkRtgCanvasGrid *grid) throw (GtkRtgException);


/*!
    \ingroup GtkRtgCanvasGrid
    \brief Sets up a lines color.\n
    Default color is 'lightslategray'.
    \param grid - A \c GtkRtgCanvasGrid.
    \param color - A \c GdkColor.
    \attention - In case \c grid fails on integrity check, or a \c color is \c NULL a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_grid_set_line_color(GtkRtgCanvasGrid *grid, GdkColor *color) throw (GtkRtgException);


/*!
    \ingroup GtkRtgCanvasGrid
    \brief Returns color of a line.
    \param grid - A \c GtkRtgCanvasGrid.
    \return - A \c GdkColor.
    \attention - In case \c grid fails on integrity check a \ref GtkRtgException will be thrown.
*/
GdkColor *gtk_rtg_canvas_grid_get_line_color(GtkRtgCanvasGrid *grid) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvasGrid
    \brief Whether to fade lines color.
    Default value is \c TRUE.
    \param grid - A \c GtkRtgCanvasGrid.
    \param fade - \c TRUE to fade lines, or \c FALSE to ignore.
    \attention - In case \c grid fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_grid_set_fade_line_color(GtkRtgCanvasGrid *grid, gboolean fade) throw (GtkRtgException);


/*!
    \ingroup GtkRtgCanvasGrid
    \brief Checks up whether color of a line fades. \n
    \param grid - A \c GtkRtgCanvasGrid.
    \return - A \c gboolean.
    \attention - In case \c grid fails on integrity check a \ref GtkRtgException will be thrown.
*/
gboolean gtk_rtg_canvas_grid_get_fade_line_color(GtkRtgCanvasGrid *grid) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvasGrid
    \brief Sets up a cross-lines color.\n
    Default color is 'darkslateblue'.
    \param grid - A \c GtkRtgCanvasGrid.
    \param color - A \c GdkColor.
    \attention - In case \c grid fails on integrity check, or a \c color is \c NULL a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_grid_set_cross_line_color(GtkRtgCanvasGrid *grid, GdkColor *color) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvasGrid
    \brief Returns color of a cross-lines.
    \param grid - A \c GtkRtgCanvasGrid.
    \return - A \c GdkColor.
    \attention - In case \c grid fails on integrity check a \ref GtkRtgException will be thrown.
*/
GdkColor *gtk_rtg_canvas_grid_get_cross_line_color(GtkRtgCanvasGrid *grid) throw (GtkRtgException);


/*!
    \ingroup GtkRtgCanvasGrid
    \brief Sets up a distance between neighbor lines.
    Default spacing is 1.0.
    \param grid - A \c GtkRtgCanvasGrid.
    \param spacing - A spacing between neighbor lines in units (scale >= 0.0).
    \attention - In case \c grid fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_grid_set_spacing(GtkRtgCanvasGrid *grid, gfloat spacing) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvasGrid
    \brief Returns a spacing between lines.
    \param grid - A \c GtkRtgCanvasGrid.
    \return - A \c gfloat.
    \attention - In case \c grid fails on integrity check a \ref GtkRtgException will be thrown.
*/
gfloat  gtk_rtg_canvas_grid_get_spacing(GtkRtgCanvasGrid *grid) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvasGrid
    \brief Whether to draw snap cursor.
    Default value is \c FALSE.
    \param grid - A \c GtkRtgCanvasGrid.
    \param snap - \c TRUE to draw cursor, or \c FALSE to not.
    \attention - In case \c grid fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_grid_set_snap_cursor(GtkRtgCanvasGrid *grid, gboolean snap) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvasGrid
    \brief Query a value for snap. \n
    \param grid - A \c GtkRtgCanvasGrid.
    \return - A \c gboolean.
    \attention - In case \c grid fails on integrity check a \ref GtkRtgException will be thrown.
*/
gboolean gtk_rtg_canvas_grid_get_snap_cursor(GtkRtgCanvasGrid *grid) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvasGrid
    \brief Renders a snap cursor.
    \param grid - A \c GtkRtgCanvasGrid.
    \param canvas - A \ref GtkRtgCanvas to render a cursor.
    \attention - In case \c grid, or \c canvas fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_canvas_grid_render_snap_cursor(GtkRtgCanvasGrid *grid, GtkRtgCanvas *canvas) throw (GtkRtgException);

/*!
    \ingroup GtkRtgCanvasGrid
    \brief Returns a grid snap point.
    \param grid - A \c GtkRtgCanvasGrid.
    \param return - A \c gtkrtg::point3f as snap point.
    \attention - In case \c grid fails on integrity check a \ref GtkRtgException will be thrown.
*/
gtkrtg::point3f& gtk_rtg_canvas_grid_get_snap_point(GtkRtgCanvasGrid *grid) throw (GtkRtgException);

G_END_DECLS

#endif /* GTK_RTG_CANVAS_GRID_H */
