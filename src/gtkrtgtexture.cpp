#include <gtkrtg/gtkrtgtexture.h>
#include <gtkrtg/gtkrtgtexturemap.h>
#include "gtkrtgextras.h"

#define GTK_RTG_TEXTURE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_RTG_TYPE_TEXTURE, GtkRtgTexturePrivate))

G_DEFINE_TYPE(GtkRtgTexture, gtk_rtg_texture, G_TYPE_OBJECT)

struct _GtkRtgTexturePrivate
{
    guint  id;
    gchar *filename;
};

/* signals */
enum
{
    BIND,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = { 0 };

static void	gtk_rtg_texture_finalize(GObject *object);

static void
gtk_rtg_texture_class_init(GtkRtgTextureClass *klass)
{
    GObjectClass    *object_class = (GObjectClass *) klass;

    object_class->finalize = gtk_rtg_texture_finalize;

    /*
     * GtkRtgTexture: bind
     *
     * Texture emits this signal when bind is initiated.
     *
     */
    signals[BIND] = g_signal_new("bind",
                                 G_OBJECT_CLASS_TYPE(klass),
                                 (GSignalFlags)G_SIGNAL_RUN_FIRST,
                                 G_STRUCT_OFFSET(GtkRtgTextureClass, bind),
                                 NULL,
                                 NULL,
                                 g_cclosure_marshal_VOID__UINT,
                                 G_TYPE_NONE,
                                 1,
                                 G_TYPE_UINT);

    g_type_class_add_private(G_OBJECT_CLASS(klass), sizeof(GtkRtgTexturePrivate));
}


GtkRtgTexture *
gtk_rtg_texture_new(gchar *filename, guint id)
{
    GtkRtgTexture *texture = GTK_RTG_TEXTURE(g_object_new(GTK_RTG_TYPE_TEXTURE, NULL));

    if(texture)
    {
        texture->priv->id = id;
        texture->priv->filename = g_strdup(filename);
    }

    return texture;
}


static void
gtk_rtg_texture_init(GtkRtgTexture *texture)
{
    texture->priv = GTK_RTG_TEXTURE_GET_PRIVATE(texture);
}


static void
gtk_rtg_texture_finalize(GObject *object)
{
    GtkRtgTexture *texture = GTK_RTG_TEXTURE(object);

    if(texture->priv->filename)
    {
        g_free(texture->priv->filename);
        texture->priv->filename = NULL;
    }

    G_OBJECT_CLASS(gtk_rtg_texture_parent_class)->finalize(object);
}


/*
 * Publics
 *
 */

guint
gtk_rtg_texture_get_id(GtkRtgTexture *texture) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_TEXTURE(texture));
    return texture->priv->id;
}


void
gtk_rtg_texture_bind(GtkRtgTexture *texture, guint unit) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_TEXTURE(texture));
    g_throw_if_fail(unit < MAX_TEXTURE_UNITS);

    g_signal_emit(texture, signals[BIND], 0, unit);
}


gchar *
gtk_rtg_texture_get_filename(GtkRtgTexture *texture) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_TEXTURE(texture));
    return texture->priv->filename;
}
