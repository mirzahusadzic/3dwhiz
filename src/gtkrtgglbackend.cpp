#include "gtkrtgglbackend.h"
#include <algorithm>
#include <vector>
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
static CGLContextObj shared_context = nullptr;
static unsigned context_count = 0;
#endif

struct GtkRtgGLBackend
{
#ifdef __APPLE__
    CGLContextObj context = nullptr;
#else
    GdkGLContext *context = nullptr;
#endif
    GLuint framebuffer = 0;
    GLuint color = 0;
    GLuint depth = 0;
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels;
};

GtkRtgGLBackend *
gtk_rtg_gl_backend_new(GtkWidget *widget, GError **error)
{
    auto *backend = new GtkRtgGLBackend;
#ifdef __APPLE__
    const CGLPixelFormatAttribute attributes[] = {kCGLPFAAccelerated,
                                                  kCGLPFAAllowOfflineRenderers,
                                                  kCGLPFAColorSize,
                                                  static_cast<CGLPixelFormatAttribute>(24),
                                                  kCGLPFAAlphaSize,
                                                  static_cast<CGLPixelFormatAttribute>(8),
                                                  kCGLPFADepthSize,
                                                  static_cast<CGLPixelFormatAttribute>(24),
                                                  static_cast<CGLPixelFormatAttribute>(0)};
    CGLPixelFormatObj format = nullptr;
    GLint count = 0;
    CGLError result = CGLChoosePixelFormat(attributes, &format, &count);
    if(result == kCGLNoError && format)
        result = CGLCreateContext(format, shared_context, &backend->context);
    if(format)
        CGLDestroyPixelFormat(format);
    if(result != kCGLNoError || !backend->context)
    {
        g_set_error(error, GDK_GL_ERROR, GDK_GL_ERROR_NOT_AVAILABLE,
                    "Cannot create compatibility OpenGL context: %s", CGLErrorString(result));
        delete backend;
        return nullptr;
    }
    if(!shared_context)
        shared_context = CGLRetainContext(backend->context);
    ++context_count;
#else
    backend->context = gdk_window_create_gl_context(gtk_widget_get_window(widget), error);
    if(!backend->context)
    {
        delete backend;
        return nullptr;
    }
    gdk_gl_context_set_use_es(backend->context, FALSE);
    gdk_gl_context_set_forward_compatible(backend->context, FALSE);
    gdk_gl_context_set_required_version(backend->context, 3, 2);
    if(!gdk_gl_context_realize(backend->context, error))
    {
        g_object_unref(backend->context);
        delete backend;
        return nullptr;
    }
    if(!gdk_gl_context_is_legacy(backend->context))
    {
        g_set_error_literal(error, GDK_GL_ERROR, GDK_GL_ERROR_UNSUPPORTED_PROFILE,
                            "3dwhiz requires desktop compatibility OpenGL. Run with GDK_GL=legacy.");
        g_object_unref(backend->context);
        delete backend;
        return nullptr;
    }
#endif
    gtk_rtg_gl_backend_make_current(backend);
    glGenFramebuffersEXT(1, &backend->framebuffer);
    glGenRenderbuffersEXT(1, &backend->color);
    glGenRenderbuffersEXT(1, &backend->depth);
    return backend;
}

gboolean
gtk_rtg_gl_backend_make_current(GtkRtgGLBackend *backend)
{
    if(!backend)
        return FALSE;
#ifdef __APPLE__
    if(CGLSetCurrentContext(backend->context) != kCGLNoError)
        return FALSE;
#else
    gdk_gl_context_make_current(backend->context);
#endif
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, backend->framebuffer);
    if(backend->framebuffer)
    {
        glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
        glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    }
    return TRUE;
}

gboolean
gtk_rtg_gl_backend_resize(GtkRtgGLBackend *backend, int width, int height)
{
    if(!gtk_rtg_gl_backend_make_current(backend))
        return FALSE;
    width = std::max(width, 1);
    height = std::max(height, 1);
    if(backend->width == width && backend->height == height)
        return TRUE;
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, backend->color);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, width, height);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT,
                                 backend->color);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, backend->depth);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT,
                                 backend->depth);
    if(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
        return FALSE;
    backend->width = width;
    backend->height = height;
    backend->pixels.resize(static_cast<size_t>(width) * height * 4);
    return TRUE;
}

void
gtk_rtg_gl_backend_paint(GtkRtgGLBackend *backend, cairo_t *cr)
{
    if(!backend || backend->pixels.empty())
        return;
    gtk_rtg_gl_backend_make_current(backend);
    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glReadPixels(0, 0, backend->width, backend->height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                 backend->pixels.data());
    glPopClientAttrib();
    auto *surface = cairo_image_surface_create_for_data(backend->pixels.data(), CAIRO_FORMAT_RGB24,
                                                        backend->width, backend->height, backend->width * 4);
    cairo_save(cr);
    cairo_translate(cr, 0, backend->height);
    cairo_scale(cr, 1, -1);
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);
    cairo_restore(cr);
    cairo_surface_destroy(surface);
}

void
gtk_rtg_gl_backend_free(GtkRtgGLBackend *backend)
{
    if(!backend)
        return;
    gtk_rtg_gl_backend_make_current(backend);
    glDeleteRenderbuffersEXT(1, &backend->depth);
    glDeleteRenderbuffersEXT(1, &backend->color);
    glDeleteFramebuffersEXT(1, &backend->framebuffer);
#ifdef __APPLE__
    CGLSetCurrentContext(nullptr);
    CGLReleaseContext(backend->context);
    if(--context_count == 0)
    {
        CGLReleaseContext(shared_context);
        shared_context = nullptr;
    }
#else
    gdk_gl_context_clear_current();
    g_object_unref(backend->context);
#endif
    delete backend;
}
