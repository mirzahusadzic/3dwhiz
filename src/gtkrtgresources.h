#ifndef GTK_RTG_RESOURCES_H
#define GTK_RTG_RESOURCES_H

#include <gtk/gtk.h>
#include <gtkrtg/gtkrtgexception.h>
#include <gtkrtg/gtkrtgtext.h>
#include <string.h>

#include <map>
#include <algorithm>
#include <functional>

struct GtkRtgCanvasFont{ gint list_base; gint chars_count; };

struct strCmp	 { bool operator()( const char  *s1, const char  *s2 ) const { return strcmp( s1, s2 ) < 0; } };
struct uiCmp	 { bool operator()( const guint  n1, const guint  n2 ) const { return n1 < n2; } };
struct floatCmp  { bool operator()( const float  n1, const float  n2 ) const { return n1 < n2; } };
struct doubleCmp { bool operator()( const double n1, const double n2 ) const { return n1 < n2; } };
struct ptrCmp	 { bool operator()( const void *p1, const void *p2 ) const { return p1 < p2; } };

/* font bases (fonts as generated display lists) */
typedef std::map<char *, GtkRtgCanvasFont*, strCmp>     m_fbType;

typedef enum
{
	CURS_INVALID = -1,
	CURS_NORMAL,
	CURS_CROSS,
	CURS_MOVE,
    CURS_SCALE_UNIFORM,
    CURS_SCALE_NONUNIFORM,
	CURS_VIEW_ROTATE,
	CURS_VIEW_ZOOM,
	CURS_VIEW_AXIS_H,
	CURS_VIEW_AXIS_V,
    CURS_GIZMO_ROTATE,
	CURSORS
}GtkRtgCanvasCursor;

struct GtkRtgCanvas;

void gtk_rtg_resources_init();
void gtk_rtg_resources_destroy();
void gtk_rtg_resource_fonts_release(GtkRtgCanvas *canvas);
GdkCursor *gtk_rtg_resource_get_cursor(GtkRtgCanvasCursor curs_id);
m_fbType *gtk_rtg_resource_get_font_map(GtkRtgCanvas *canvas);
gint gtk_rtg_resource_font_register(GtkRtgCanvas *canvas, GtkRtgText *text, gchar *key) throw (GtkRtgException);
void gtk_rtg_resource_icons_require_all();
void gtk_rtg_resource_icons_clean_up();

#endif /* GTK_RTG_RESOURCES_H */
