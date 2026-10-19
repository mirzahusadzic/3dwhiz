#ifndef GTK_RTG_GL_BACKEND_H
#define GTK_RTG_GL_BACKEND_H

#include <gtkrtg/gtkrtgbase.h>

// Owns a compatibility OpenGL context and an offscreen color/depth buffer.
// GTK 3 composites its pixels through Cairo, including on Quartz where
// GtkGLArea only offers core OpenGL and cannot run the recovered renderer.
struct GtkRtgGLBackend;
GtkRtgGLBackend *gtk_rtg_gl_backend_new(GtkWidget *widget, GError **error);
gboolean gtk_rtg_gl_backend_make_current(GtkRtgGLBackend *backend);
gboolean gtk_rtg_gl_backend_resize(GtkRtgGLBackend *backend, int width, int height);
void gtk_rtg_gl_backend_paint(GtkRtgGLBackend *backend, cairo_t *cr);
void gtk_rtg_gl_backend_free(GtkRtgGLBackend *backend);

#endif
