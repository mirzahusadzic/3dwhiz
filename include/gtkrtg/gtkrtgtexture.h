#ifndef GTK_RTG_TEXTURE_H
#define GTK_RTG_TEXTURE_H

#include <gtkrtg/gtkrtgbase.h>
#include <gtkrtg/gtkrtgexception.h>

/*! \file */

G_BEGIN_DECLS

#define GTK_RTG_TYPE_TEXTURE            (gtk_rtg_texture_get_type())
#define GTK_RTG_TEXTURE(obj)		    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_RTG_TYPE_TEXTURE, GtkRtgTexture))
#define GTK_RTG_TEXTURE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass),  GTK_RTG_TYPE_TEXTURE, GtkRtgTextureClass))
#define GTK_RTG_IS_TEXTURE(obj)		    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_RTG_TYPE_TEXTURE))
#define GTK_RTG_IS_TEXTURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GTK_RTG_TYPE_TEXTURE))
#define GTK_RTG_TEXTURE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),  GTK_RTG_TYPE_TEXTURE, GtkRtgTextureClass))

typedef struct _GtkRtgTexture        GtkRtgTexture;
typedef struct _GtkRtgTextureClass	 GtkRtgTextureClass;
typedef struct _GtkRtgTexturePrivate GtkRtgTexturePrivate;

struct _GtkRtgTexture
{
    GObject		 	  	  parent;
    GtkRtgTexturePrivate *priv;
};

struct _GtkRtgTextureClass
{
    GObjectClass parent_class;

    /* signals */
    void (* bind) (GtkRtgTexture *texture, guint unit);
};

/*! \defgroup GtkRtgTexture GtkRtgTexture
    \c GtkRtgTexture is derived from the \c GtkObject. \n \n
    The \c GtkRtgTexture is an object which represents a texture. The texture is requested from the \ref GtkRtgTextureMap. \n
    For the complete list of supported file formats see \c gdk_pixbuf_new_from_file.
*/


GType gtk_rtg_texture_get_type(void) G_GNUC_CONST;

/*!
    \ingroup GtkRtgTexture
    \brief Returns a texture id. \n
    The texture id is generated texture name by the \c glGenTextures.
    \param texture - A \c GtkRtgTexture.
    \return - A \c guint.
    \attention - In case \c texture fails on integrity check a \ref GtkRtgException will be thrown.
*/
guint gtk_rtg_texture_get_id(GtkRtgTexture *texture) throw (GtkRtgException);

/*!
    \ingroup GtkRtgTexture
    \brief Returns a texture filename. \n
    The texture \c filename is an absolute path to the texture.
    \param texture - A \c GtkRtgTexture.
    \return - A \c guchar.
    \attention - In case \c texture fails on integrity check a \ref GtkRtgException will be thrown.
*/
gchar *gtk_rtg_texture_get_filename(GtkRtgTexture *texture) throw (GtkRtgException);

/*!
    \ingroup GtkRtgTexture
    \brief Binds a texture to the unit. \n
    The 'bind' signal will be emitted which is processed by the \ref GtkRtgTextureMap.
    \param texture - A \c GtkRtgTexture.
    \param unit - A unit to bind texture.
    \attention - In case \c texture fails on integrity check, or \c unit exceeds the number of maximum texture units a \ref GtkRtgException will be thrown.
*/
void  gtk_rtg_texture_bind(GtkRtgTexture *texture, guint unit=0) throw (GtkRtgException);

/* handy macros */
#define TEXTURE_ID(texture)   (gtk_rtg_texture_get_id(GTK_RTG_TEXTURE(texture)))
#define TEXTURE_FILE(texture) (gtk_rtg_texture_get_filename(GTK_RTG_TEXTURE(texture)))
#define BIND(texture, unit)	  (gtk_rtg_texture_bind(GTK_RTG_TEXTURE(texture), unit))

G_END_DECLS

#endif /* GTK_RTG_TEXTURE_H */
