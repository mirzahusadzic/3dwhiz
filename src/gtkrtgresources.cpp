#include "gtkrtgresources.h"
#include <gtkrtg/gtkrtgcanvas.h>
#include <pango/pangocairo.h>
#include <vector>
#include <algorithm>

static GdkCursor *cursors[CURSORS] = {};
static std::map<GtkRtgCanvas *, m_fbType *> font_maps;

void
gtk_rtg_resources_init()
{
    static const GdkCursorType types[CURSORS] = {GDK_LEFT_PTR,
                                                 GDK_CROSSHAIR,
                                                 GDK_FLEUR,
                                                 GDK_SIZING,
                                                 GDK_SIZING,
                                                 GDK_EXCHANGE,
                                                 GDK_SB_V_DOUBLE_ARROW,
                                                 GDK_SB_H_DOUBLE_ARROW,
                                                 GDK_SB_V_DOUBLE_ARROW,
                                                 GDK_EXCHANGE};
    for(int i = 0; i < CURSORS; ++i)
        if(!cursors[i])
            cursors[i] = gdk_cursor_new_for_display(gdk_display_get_default(), types[i]);
}

void
gtk_rtg_resources_destroy()
{
    for(auto &cursor : cursors)
        g_clear_object(&cursor);
}

GdkCursor *
gtk_rtg_resource_get_cursor(GtkRtgCanvasCursor id)
{
    return id >= 0 && id < CURSORS ? cursors[id] : nullptr;
}

m_fbType *
gtk_rtg_resource_get_font_map(GtkRtgCanvas *canvas)
{
    auto found = font_maps.find(canvas);
    return found == font_maps.end() ? nullptr : found->second;
}

void
gtk_rtg_resource_fonts_release(GtkRtgCanvas *canvas)
{
    auto found = font_maps.find(canvas);
    if(found == font_maps.end())
        return;
    for(const auto &entry : *found->second)
    {
        glDeleteLists(entry.second->list_base, entry.second->chars_count);
        g_free(entry.first);
        delete entry.second;
    }
    delete found->second;
    font_maps.erase(found);
}

// GtkGLExt's Pango font helper disappeared with GTK 2. Generate its equivalent
// bitmap display lists from Pango/Cairo so the original raster text still works.
gint
gtk_rtg_resource_font_register(GtkRtgCanvas *canvas, GtkRtgText *text, gchar *key) throw(GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
    gchar *description = nullptr;
    gint first = 0, count = 0;
    g_object_get(text, "font-desc", &description, "start-char", &first, "chars-count", &count, nullptr);
    g_throw_if_fail(count > 0 && count <= 65536 && first >= 0 && first + count <= 0x110000);
    auto *font = pango_font_description_from_string(description ? description : "Sans 10");
    g_free(description);
    auto *layout = gtk_widget_create_pango_layout(GTK_WIDGET(canvas), nullptr);
    pango_layout_set_font_description(layout, font);
    pango_font_description_free(font);
    GLuint lists = glGenLists(first + count);
    if(!lists)
    {
        g_object_unref(layout);
        g_throw_if_fail(lists != 0);
    }
    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
    for(int i = first; i < first + count; ++i)
    {
        gchar utf8[8] = {};
        gunichar character = (i < 32 || !g_unichar_validate(i)) ? ' ' : i;
        g_unichar_to_utf8(character, utf8);
        pango_layout_set_text(layout, utf8, -1);
        PangoRectangle ink, logical;
        pango_layout_get_pixel_extents(layout, &ink, &logical);
        const int width = std::max(ink.width, 1), height = std::max(ink.height, 1);
        auto *surface = cairo_image_surface_create(CAIRO_FORMAT_A8, width, height);
        auto *cr = cairo_create(surface);
        cairo_set_source_rgba(cr, 1, 1, 1, 1);
        cairo_translate(cr, -ink.x, -ink.y);
        pango_cairo_show_layout(cr, layout);
        cairo_surface_flush(surface);
        const auto *data = cairo_image_surface_get_data(surface);
        const int stride = cairo_image_surface_get_stride(surface);
        const int row_bytes = (width + 7) / 8;
        std::vector<GLubyte> bitmap(static_cast<size_t>(row_bytes) * height, 0);
        for(int y = 0; y < height; ++y)
            for(int x = 0; x < width; ++x)
                if(data[y * stride + x] >= 96)
                    bitmap[(height - 1 - y) * row_bytes + x / 8] |= 0x80 >> (x % 8);
        const int baseline = PANGO_PIXELS(pango_layout_get_baseline(layout));
        glNewList(lists + i, GL_COMPILE);
        glBitmap(width, height, -ink.x, ink.y + height - baseline, logical.width, 0, bitmap.data());
        glEndList();
        cairo_destroy(cr);
        cairo_surface_destroy(surface);
    }
    glPopClientAttrib();
    g_object_unref(layout);
    auto *map = gtk_rtg_resource_get_font_map(canvas);
    if(!map)
    {
        map = new m_fbType;
        font_maps[canvas] = map;
    }
    (*map)[g_strdup(key)] = new GtkRtgCanvasFont{static_cast<gint>(lists), first + count};
    return lists;
}

// Kept for source compatibility. Toolbar images now come from GTK's icon theme.
void
gtk_rtg_resource_icons_require_all()
{
}
void
gtk_rtg_resource_icons_clean_up()
{
}
