#include <gtkrtg/gtkrtgtext.h>
#include "gtkrtg_gl_specs.h"
#include "gtkrtgextras.h"

#define GTK_RTG_TEXT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_RTG_TYPE_TEXT, GtkRtgTextPrivate))

G_DEFINE_TYPE(GtkRtgText, gtk_rtg_text, G_TYPE_OBJECT)

struct _GtkRtgTextPrivate
{
	gchar *font_desc;
	gint start_char;
	gint chars_count;
	GdkColor color;
	gchar *string;
	GdkPoint position;
    gboolean pos_flipped;
    gboolean visible;
};

static void	gtk_rtg_text_finalize(GObject *object);
static void gtk_rtg_text_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gtk_rtg_text_get_property(GObject *object, guint prop_id, GValue       *value, GParamSpec *pspec);

/* privs */
static void _gtk_rtg_text_key_regenerate (GtkRtgText *text);

/* properties */
enum
{
    PROP_0,
    PROP_FONT_DESC,
    PROP_START_CHAR,
    PROP_CHARS_COUNT,
    PROP_COLOR,
    PROP_STRING,
    PROP_POSITION,
    PROP_POSITION_FLIPPED,
    LAST_ARG
};

static void
gtk_rtg_text_class_init(GtkRtgTextClass *klass)
{
	GObjectClass  *object_class = (GObjectClass *) klass;

    object_class->set_property = gtk_rtg_text_set_property;
    object_class->get_property = gtk_rtg_text_get_property;
    object_class->finalize	   = gtk_rtg_text_finalize;

	/*
     * GtkRtgText: font-desc
     *
     * Font description (i.e. family, type {normal,bold,italic} and size).
     *
     */
    g_object_class_install_property(object_class,
									PROP_FONT_DESC,
									g_param_spec_string("font-desc",
														"Font decrition",
														"Font description (i.e. family, type {normal,bold,italic} and size).",
														NULL,
														GTK_RTG_PARAM_READWRITE));
	/*
     * GtkRtgText: start-char
     *
     * Fonts starting character (i.e. ASCII code).
     *
     */
    g_object_class_install_property(object_class,
									PROP_START_CHAR,
									g_param_spec_int("start-char",
													 "Start char",
													 "Fonts starting character (i.e. ASCII code).",
													 0,
													 10000000,
													 0,
													 GTK_RTG_PARAM_READWRITE));
	/*
     * GtkRtgText: chars-count
     *
     * Fonts total characters.
     *
     */
    g_object_class_install_property(object_class,
									PROP_CHARS_COUNT,
									g_param_spec_int("chars-count",
													 "Chars count",
													 "Fonts total characters.",
													 0,
													 10000000,
													 255,
													 GTK_RTG_PARAM_READWRITE));
	/*
     * GtkRtgText: color
     *
     * Text color.
     *
     */
	g_object_class_install_property(object_class,
									PROP_COLOR,
									g_param_spec_pointer("color",
														 "Color",
														 "Text color.",
														 GTK_RTG_PARAM_READWRITE));
	/*
     * GtkRtgText: string
     *
     * Text string.
     *
     */
    g_object_class_install_property(object_class,
									PROP_STRING,
									g_param_spec_string("string",
														"String",
														"Text string.",
														NULL,
														GTK_RTG_PARAM_READWRITE));
	/*
     * GtkRtgText: position
     *
     * Text position on the canvas.
     *
     */
	g_object_class_install_property(object_class,
									PROP_POSITION,
									g_param_spec_pointer("position",
														 "Position",
														 "Text position on the canvas.",
														 GTK_RTG_PARAM_READWRITE));
	/*
     * GtkRtgText: pos-flipped
     *
     * Whether the text position should originate from top-left or bottom-left corner.
     *
     */
    g_object_class_install_property(object_class,
									PROP_POSITION_FLIPPED,
									g_param_spec_boolean("pos-flipped",
														 "Position flipped",
														 "Whether the text position should originate from top-left or bottom-left corner.",
														 TRUE,
														 GTK_RTG_PARAM_READWRITE));

	g_type_class_add_private(G_OBJECT_CLASS(klass), sizeof(GtkRtgTextPrivate));
}


GtkRtgText *
gtk_rtg_text_new()
{
	return GTK_RTG_TEXT(g_object_new(GTK_RTG_TYPE_TEXT, NULL));
}


static void
gtk_rtg_text_init(GtkRtgText *text)
{
	text->priv = GTK_RTG_TEXT_GET_PRIVATE(text);

	/* default text */
	text->priv->font_desc = g_strdup_printf("%s 12", GTK_RTG_CANVAS_VIEW_PREDEFINED_FONT);
	text->priv->chars_count = 128;
	text->priv->pos_flipped = FALSE;
	text->priv->string = g_strdup("");
    text->priv->visible = TRUE;

	gdk_color_parse("white", &text->priv->color);

    g_object_set_data(G_OBJECT(text), "key", g_strdup(""));
    g_object_set_data(G_OBJECT(text), "font-base", new gint(-1));

    _gtk_rtg_text_key_regenerate(text);
}


static void
gtk_rtg_text_finalize(GObject *object)
{
	GtkRtgText *text = GTK_RTG_TEXT(object);

	if(text->priv->font_desc){ g_free(text->priv->font_desc); text->priv->font_desc	= NULL; }
    if(text->priv->string)	 { g_free(text->priv->string);	  text->priv->string    = NULL; }

    /* free dynamic props associator(s) */

    gchar *key = (gchar*) g_object_get_data(G_OBJECT(text), "key");
    if(key)
	{
		g_free(key);
		key = NULL;
		g_object_set_data(G_OBJECT(text), "key", key);
	};

    gint *base = (gint*)g_object_get_data(G_OBJECT(text), "font-base");
    if(base)
	{	delete base;
		base = NULL;
		g_object_set_data(G_OBJECT(text), "font-base",  base);
	};

	G_OBJECT_CLASS(gtk_rtg_text_parent_class)->finalize(object);
}


static void
gtk_rtg_text_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GtkRtgText *text = GTK_RTG_TEXT(object);

    switch(prop_id)
    {
		case PROP_FONT_DESC: gtk_rtg_text_set_font_desc(text, g_value_get_string(value)); break;
		case PROP_START_CHAR: gtk_rtg_text_set_start_char(text, g_value_get_int(value)); break;
		case PROP_CHARS_COUNT: gtk_rtg_text_set_chars_count(text, g_value_get_int(value)); break;
		case PROP_COLOR: gtk_rtg_text_set_color(text, (GdkColor*)(g_value_get_pointer(value))); break;
		case PROP_STRING: gtk_rtg_text_set_string(text, g_value_get_string(value)); break;
		case PROP_POSITION: gtk_rtg_text_set_position(text, (GdkPoint*)(g_value_get_pointer(value))); break;
		case PROP_POSITION_FLIPPED: gtk_rtg_text_set_text_pos_flipped(text, g_value_get_boolean(value)); break;
	};
}


static void
gtk_rtg_text_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GtkRtgText *text = GTK_RTG_TEXT(object);

    switch(prop_id)
    {							/* getters without extra function calls */
		case PROP_FONT_DESC: g_value_set_string(value, text->priv->font_desc); break;
		case PROP_START_CHAR: g_value_set_int(value, text->priv->start_char); break;
		case PROP_CHARS_COUNT: g_value_set_int(value, text->priv->chars_count); break;
		case PROP_COLOR: g_value_set_pointer(value, &text->priv->color); break;
		case PROP_STRING: g_value_set_string(value, text->priv->string); break;
		case PROP_POSITION: g_value_set_pointer(value, &text->priv->position); break;
		case PROP_POSITION_FLIPPED:	g_value_set_boolean(value, text->priv->pos_flipped); break;
	};
}


/*
 * Publics
 *
 *
 */
void
gtk_rtg_text_set_font_desc(GtkRtgText *text, const gchar *font_desc) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_TEXT(text));
	g_throw_if_fail(font_desc != NULL);

	if(text->priv->font_desc)
		g_free(text->priv->font_desc);

	text->priv->font_desc = g_strdup(font_desc);
    _gtk_rtg_text_key_regenerate(text);
}


gchar *
gtk_rtg_text_get_font_desc(GtkRtgText *text) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_TEXT(text));
	return text->priv->font_desc;
}

void
gtk_rtg_text_set_start_char(GtkRtgText *text, const gint start_char) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_TEXT(text));
	text->priv->start_char = start_char;
    _gtk_rtg_text_key_regenerate(text);
}


gint
gtk_rtg_text_get_start_char(GtkRtgText *text) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_TEXT(text));
	return text->priv->start_char;
}


void
gtk_rtg_text_set_chars_count(GtkRtgText *text, const gint chars_count) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_TEXT(text));
	text->priv->chars_count = chars_count;
    _gtk_rtg_text_key_regenerate(text);
}


gint
gtk_rtg_text_get_chars_count(GtkRtgText *text) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_TEXT(text));
	return text->priv->chars_count;
}


void
gtk_rtg_text_set_color(GtkRtgText *text, const GdkColor *color) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_TEXT(text));
	g_throw_if_fail(color != NULL);
	text->priv->color = *color;
}


GdkColor *
gtk_rtg_text_get_color(GtkRtgText *text) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_TEXT(text));
	return &text->priv->color;
}


void
gtk_rtg_text_set_string(GtkRtgText *text, const gchar *string) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_TEXT(text));
	g_throw_if_fail(string != NULL);

	if(text->priv->string)
        g_free(text->priv->string);

	text->priv->string = g_strdup(string);
}


gchar *
gtk_rtg_text_get_string(GtkRtgText *text) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_TEXT(text));
	return text->priv->string;
}


void
gtk_rtg_text_set_position(GtkRtgText *text, const GdkPoint *pos) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_TEXT(text));
	g_throw_if_fail(pos != NULL);
	text->priv->position = *pos;
}


GdkPoint *
gtk_rtg_text_get_position(GtkRtgText *text) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_TEXT(text));
	return &text->priv->position;
}


void
gtk_rtg_text_set_text_pos_flipped(GtkRtgText *text, const gboolean flipped) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_TEXT(text));
	text->priv->pos_flipped = flipped;

}


gboolean
gtk_rtg_text_get_text_pos_flipped(GtkRtgText *text) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_TEXT(text));
	return text->priv->pos_flipped;
}

void
gtk_rtg_text_set_visible(GtkRtgText *text, gboolean visible) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_TEXT(text));
    text->priv->visible = visible;
}

gboolean
gtk_rtg_text_is_visible(GtkRtgText *text) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_TEXT(text));
    return text->priv->visible;
}

/* privs. */
static void
_gtk_rtg_text_key_regenerate(GtkRtgText *text)
{
    gchar *key = (gchar*) g_object_get_data(G_OBJECT(text), "key");
    g_free(key);
    key = g_strdup_printf("%s%d%d", text->priv->font_desc, text->priv->start_char, text->priv->chars_count);
    g_object_set_data(G_OBJECT(text), "key", key);
}
