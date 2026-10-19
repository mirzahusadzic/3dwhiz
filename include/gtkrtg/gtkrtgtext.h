#ifndef GTK_RTG_TEXT_H
#define GTK_RTG_TEXT_H

#include <gtkrtg/gtkrtgbase.h>
#include <gtkrtg/gtkrtgexception.h>

/*! \file */

G_BEGIN_DECLS

#define GTK_RTG_TYPE_TEXT            (gtk_rtg_text_get_type())
#define GTK_RTG_TEXT(obj)		     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_RTG_TYPE_TEXT, GtkRtgText))
#define GTK_RTG_TEXT_CLASS(klass)	 (G_TYPE_CHECK_CLASS_CAST ((klass),  GTK_RTG_TYPE_TEXT, GtkRtgTextClass))
#define GTK_RTG_IS_TEXT(obj)		 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_RTG_TYPE_TEXT))
#define GTK_RTG_IS_TEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GTK_RTG_TYPE_TEXT))
#define GTK_RTG_TEXT_GET_CLASS(obj)	 (G_TYPE_INSTANCE_GET_CLASS ((obj),  GTK_RTG_TYPE_TEXT, GtkRtgTextClass))


typedef struct _GtkRtgText		  GtkRtgText;
typedef struct _GtkRtgTextClass	  GtkRtgTextClass;
typedef struct _GtkRtgTextPrivate GtkRtgTextPrivate;


struct _GtkRtgText
{
	GObject			   parent;
	GtkRtgTextPrivate *priv;
};

struct _GtkRtgTextClass
{
	GObjectClass parent_class;
};


/*! \defgroup GtkRtgText GtkRtgText
    \c GtkRtgText is derived from the \c GtkObject. \n \n
    The \c GtkRtgText is an object which represents a text.
*/

GType gtk_rtg_text_get_type(void) G_GNUC_CONST;
GtkRtgText *gtk_rtg_text_new();
void gtk_rtg_text_set_font_desc(GtkRtgText *text, const gchar *font_desc) throw (GtkRtgException);
gchar *gtk_rtg_text_get_font_desc(GtkRtgText *text) throw (GtkRtgException);
void gtk_rtg_text_set_start_char(GtkRtgText *text, const gint start_char) throw (GtkRtgException);
gint gtk_rtg_text_get_start_char(GtkRtgText *text) throw (GtkRtgException);
void gtk_rtg_text_set_chars_count(GtkRtgText *text, const gint chars_count) throw (GtkRtgException);
gint gtk_rtg_text_get_chars_count(GtkRtgText *text) throw (GtkRtgException);
void gtk_rtg_text_set_color(GtkRtgText *text, const GdkColor *color) throw (GtkRtgException);
GdkColor *gtk_rtg_text_get_color(GtkRtgText *text) throw (GtkRtgException);
void gtk_rtg_text_set_string(GtkRtgText *text, const gchar *string) throw (GtkRtgException);
gchar *gtk_rtg_text_get_string(GtkRtgText *text) throw (GtkRtgException);
void gtk_rtg_text_set_position(GtkRtgText *text, const GdkPoint *pos) throw (GtkRtgException);
GdkPoint *gtk_rtg_text_get_position(GtkRtgText *text) throw (GtkRtgException);
void gtk_rtg_text_set_text_pos_flipped(GtkRtgText *text, const gboolean flipped) throw (GtkRtgException);
gboolean gtk_rtg_text_get_text_pos_flipped(GtkRtgText *text) throw (GtkRtgException);
void gtk_rtg_text_set_visible(GtkRtgText *text, gboolean visible) throw (GtkRtgException);
gboolean gtk_rtg_text_is_visible(GtkRtgText *text) throw (GtkRtgException);

G_END_DECLS

#endif /* GTK_RTG_TEXT_H */
