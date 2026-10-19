#ifndef GTK_RTG_TEXTURE_MAP_H
#define GTK_RTG_TEXTURE_MAP_H

#include <gtkrtg/gtkrtgbase.h>
#include <gtkrtg/gtkrtgexception.h>
#include <gtkrtg/gtkrtgtexture.h>

/*! \file */

G_BEGIN_DECLS


/*! \defgroup GtkRtgTextureMap GtkRtgTextureMap

    The \c GtkRtgTextureMap is a container for \c GtkRtgTexture. The textures are requested by the texture filename. Requested textures are loaded into graphics
    hardware and kept in texture map. Whenever used texture is requested again it will be queried from the texture map. The texture map keeps internally
    references to the \ref GtkRtgTexture and after the last texture reference is de-referenced (texture becomes unused) the texture is deleted from the driver
    and from the texture map. Whenever a texture request bind to the unit a texture map inspects if the texture is already bound to the unit which avoid rebounding the
    same texture again.
*/


/*!
    \ingroup GtkRtgTextureMap
    \brief Requests a texture for the texture unit. \n
    Loads \ref GtkRtgTexture from the \c 'filename' and binds it to the texture 'unit'. The path to the \c filename must be absolute. \n
    If the used texture is requested, then it will be returned from the map. Unused textures will be removed from the map and deleted from the driver.
    \param filename - A texture file.
    \param unit - A unit to bind texture.
    \return - A \c GtkRtgTexture.
    \attention - In case \c filename is \c NULL, or path to \c filename is relative, or \c unit exceeds the number of maximum texture units a \ref GtkRtgException will be thrown.
*/
GtkRtgTexture *gtk_rtg_texture_map_get_texture(gchar *filename, guint unit=0) throw (GtkRtgException);

/*!
    \ingroup GtkRtgTextureMap
    \brief Returns the number of maximum texture units.
    \return - A \c guint.
*/
guint gtk_rtg_texture_map_get_max_texture_units();

/* handy macros */
#define MAP_TEXTURE(filename, unit) (gtk_rtg_texture_map_get_texture(filename, unit))

G_END_DECLS

#endif /* GTK_RTG_TEXTURE_MAP_H */
