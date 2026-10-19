#ifndef GTK_RTG_EXTRAS_H
#define GTK_RTG_EXTRAS_H

#include <gtkrtg/gtkrtgbase.h>
#include <gtkrtg/gtkrtgmath.h>
#include <gtkrtg/gtkrtgexception.h>
#include <gtkrtg/gtkrtgcanvas.h>
#include <cstdlib>
#include <map>
#include "gtkrtgresources.h"

#if GLIB_CHECK_VERSION(2, 8, 0)
#define GTK_RTG_PARAM_READWRITE (GParamFlags)(G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB)
#else
#define GTK_RTG_PARAM_READWRITE G_PARAM_READWRITE
#endif

#ifdef _WIN32
#define GTK_RTG_CANVAS_VIEW_PREDEFINED_FONT "Arial"
#else
/* get some 'a must exist' linux font */
#define GTK_RTG_CANVAS_VIEW_PREDEFINED_FONT "Sans"
#endif

typedef std::map<char *, GtkRtgText*,   strCmp>      m_txtType;

#define MAX_TEXTURE_UNITS 8

namespace gtkrtg
{

typedef struct
{
	GLubyte *buff;	   /* buffer used for primitive and sub-primitive selections */
	gint	 width;    /* width  {view-port} */
	gint	 height;   /* height {view-port} */
	guint	 update:1; /* should update buffer? (read pixels) */
}GtkRtgPickingBuffer;


typedef struct
{
	gfloat r;
	gfloat g;
	gfloat b;
	gfloat a;

}GtkRtgColor4f;

void shade_color(const GtkRtgColor4f *base, gfloat shade_ratio, GtkRtgColor4f *composite)  throw (GtkRtgException);
void color_from_hsb(gfloat hue, gfloat saturation, gfloat brightness, GtkRtgColor4f *color);
void hsb_from_color(const GtkRtgColor4f *color, gfloat *hue, gfloat *saturation, gfloat *brightness);
void glColor_from_pointer(void *ptr, guint alpha = 0xff);

} /* gtkrtg */


#define GL_ENABLE_LINES(a,b,c,d,e,f, bsrc, bdst) \
{ \
	a = FALSE; b = FALSE; \
	if(!glIsEnabled(GL_BLEND)) { a = TRUE; glEnable(GL_BLEND); } \
	/*if(!glIsEnabled(GL_LINE_SMOOTH)) { b = TRUE; glEnable(GL_LINE_SMOOTH); glHint(GL_LINE_SMOOTH_HINT, GL_NICEST); } */ \
	if((c=glIsEnabled(GL_LIGHTING))) glDisable(GL_LIGHTING); \
	if((d=glIsEnabled(GL_TEXTURE_2D))) glDisable(GL_TEXTURE_2D); \
	if((e=glIsEnabled(GL_FOG))) glDisable(GL_FOG); \
	if((f=glIsEnabled(GL_DITHER))) glDisable(GL_DITHER); \
	glGetIntegerv(GL_BLEND_SRC, &bsrc); \
	glGetIntegerv(GL_BLEND_DST, &bdst); \
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); \
}

#define GL_DISABLE_LINES(a,b,c,d,e,f, bsrc, bdst) \
{ \
	if(a) glDisable(GL_BLEND); \
	if(b) glDisable(GL_LINE_SMOOTH); \
	if(c) glEnable(GL_LIGHTING); \
	if(d) glEnable(GL_TEXTURE_2D); \
	if(e) glEnable(GL_FOG); \
	if(f) glEnable(GL_DITHER); \
	glBlendFunc(bsrc, bdst); \
}

#define GL_ENABLE_POLYS(a,b,c,d) \
{ \
	a = FALSE; b = FALSE; \
	if(!glIsEnabled(GL_BLEND)) { a = TRUE; glEnable(GL_BLEND); } \
	if(!glIsEnabled(GL_POLYGON_SMOOTH)) { b = TRUE; glEnable(GL_POLYGON_SMOOTH); glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST); } \
	if((c=glIsEnabled(GL_FOG))) glDisable(GL_FOG); \
	if((d=glIsEnabled(GL_DITHER))) glDisable(GL_DITHER); \
}

#define GL_DISABLE_POLYS(a,b,c,d) \
{ \
	if(a) glDisable(GL_BLEND); \
	if(b) glDisable(GL_POLYGON_SMOOTH); \
	if(c) glEnable(GL_FOG); \
	if(d) glEnable(GL_DITHER); \
}

#define DISABLE_GL_COLOR_MODIFIERS(a,b,c,d,e) \
{ \
	if((a=glIsEnabled(GL_LIGHTING))) glDisable(GL_LIGHTING); \
	if((b=glIsEnabled(GL_TEXTURE_2D))) glDisable(GL_TEXTURE_2D); \
	if((c=glIsEnabled(GL_BLEND))) glDisable(GL_BLEND); \
	if((d=glIsEnabled(GL_FOG))) glDisable(GL_FOG); \
	if((e=glIsEnabled(GL_DITHER))) glDisable(GL_DITHER); \
}

#define ENABLE_GL_COLOR_MODIFIERS(a,b,c,d,e) \
{ \
	if(a) glEnable(GL_LIGHTING); \
	if(b) glEnable(GL_TEXTURE_2D); \
	if(c) glEnable(GL_BLEND); \
	if(d) glEnable(GL_FOG); \
	if(e) glEnable(GL_DITHER); \
}

#define CREATE_PRIMITIVE_ID(color_id)(new guint(color_id))

#define SPHERES_AXIS_RATIO .75f

enum
{
	TEST_OUT,
	TEST_INTERSECT,
	TEST_INSIDE
};

#endif /* GTK_RTG_EXTRAS_H */
