#include <gtkrtg/gtkrtgtexturemap.h>
#include "gtkrtg_gl_specs.h"
#include "gtkrtgextras.h"

#include "gtkrtgresources.h"

#include <map>
#include <algorithm>
#include <functional>

/* privs/signals */
static void _gtk_rtg_texture_bind(GtkRtgTexture *texture, guint unit, gpointer data);
static void _gtk_rtg_texture_destroy(GObject *object, gpointer data);
static void _gtk_rtg_texture_bind_real(GtkRtgTexture *texture, guint unit, gpointer data);


/* text map */
typedef std::map<gchar *, GtkRtgTexture*, strCmp>  m_textureType;
static m_textureType m_texture;
static guint texture_on_unit[MAX_TEXTURE_UNITS];

extern GtkRtgTexture *gtk_rtg_texture_new(gchar *filename, guint id);

/*
 * Publics
 *
 */

GtkRtgTexture *
gtk_rtg_texture_map_get_texture(gchar *filename, guint unit) throw (GtkRtgException)
{
    g_throw_if_fail(filename != NULL);
	g_throw_if_fail(g_path_is_absolute(filename));
	g_throw_if_fail(unit < MAX_TEXTURE_UNITS);

	/* query from the map */
    m_textureType::const_iterator it = m_texture.find(filename);

    /* texture already exist */
    if(it != m_texture.end())
    {
        g_signal_connect(it->second, "bind",    G_CALLBACK(_gtk_rtg_texture_bind),    NULL);
        g_signal_connect(it->second, "destroy", G_CALLBACK(_gtk_rtg_texture_destroy), NULL);

		_gtk_rtg_texture_bind_real(it->second, unit, NULL);

        return it->second;
    }

    /*
     * Load texture and bind to the unit
     *
     */

    GdkPixbuf *pix;
    g_throw_if_fail((pix = gdk_pixbuf_new_from_file(filename, NULL)));

    GLuint text_id;
    glGenTextures(1, &text_id);
    //fcon: glActiveTexture(unit);
    glBindTexture(GL_TEXTURE_2D, text_id);
    texture_on_unit[unit] = text_id;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);

    gluBuild2DMipmaps(GL_TEXTURE_2D,
					  gdk_pixbuf_get_has_alpha(pix) ? GL_RGBA : GL_RGB,
					  gdk_pixbuf_get_width(pix),
					  gdk_pixbuf_get_height(pix),
					  gdk_pixbuf_get_has_alpha(pix) ? GL_RGBA : GL_RGB,
					  GL_UNSIGNED_BYTE,
					  gdk_pixbuf_get_pixels(pix));

    g_object_unref(pix);

    GL_CHECK_ERROR;

    GtkRtgTexture *texture = gtk_rtg_texture_new(filename, text_id);

    /* add to the map */
    m_texture.insert(std::make_pair(filename, texture));

    g_signal_connect(texture, "bind",     G_CALLBACK(_gtk_rtg_texture_bind), NULL);
    g_signal_connect(texture, "destroy",  G_CALLBACK(_gtk_rtg_texture_destroy), NULL);

    return texture;
}

guint
gtk_rtg_texture_map_get_max_texture_units()
{
	return MAX_TEXTURE_UNITS;
}


/*
 * Privs.
 *
 */

static void
_gtk_rtg_texture_bind_real(GtkRtgTexture *texture, guint unit, gpointer data)
{
	/* only if current text-id is not already bound to the unit */
	if(texture_on_unit[unit] != TEXTURE_ID(texture))
	{
		//fcon: glActiveTexture(unit);
		glBindTexture(GL_TEXTURE_2D, TEXTURE_ID(texture));
	}
}

/*
 * Signals
 *
 */

static void
_gtk_rtg_texture_bind(GtkRtgTexture *texture, guint unit, gpointer data)
{
	_gtk_rtg_texture_bind_real(texture, unit, data);
}


static void
_gtk_rtg_texture_destroy(GObject *object, gpointer data)
{
    GtkRtgTexture *texture = GTK_RTG_TEXTURE(object);

    /*
     * Remove texture from the map and driver
     *
     */

    /* query from the map */
    m_textureType::const_iterator it = m_texture.find(TEXTURE_FILE(texture));

    if(it != m_texture.end())
    {
        /* remove from the map */
        m_texture.erase(TEXTURE_FILE(texture));

        guint id = TEXTURE_ID(texture);

        /* delete from the driver */
        glDeleteTextures(1, &id);

        GL_CHECK_ERROR;
    }

    g_signal_handlers_disconnect_by_func(object, (void *)_gtk_rtg_texture_bind, data);
    g_signal_handlers_disconnect_by_func(object, (void *)_gtk_rtg_texture_destroy, data);
}
