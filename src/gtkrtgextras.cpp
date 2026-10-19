#include "gtkrtgextras.h"
#include <math.h>

namespace gtkrtg
{

void
hsb_from_color(const GtkRtgColor4f	*color,
		 	   gfloat *hue,
               gfloat *saturation,
               gfloat *brightness )
{
	gfloat min, max, delta;
	gfloat red, green, blue;

	red = color->r;
	green = color->g;
	blue = color->b;

	if(red > green)
	{
		max = MAX(red, blue);
		min = MIN(green, blue);
	}else
	{
		max = MAX(green, blue);
		min = MIN(red, blue);
	}

	*brightness = (max + min) / 2;

	if(fabs(max - min) < 0.0001)
	{
		*hue = 0;
		*saturation = 0;
	}else
	{
		if(*brightness <= 0.5)
			*saturation = (max - min) / (max + min);
		else
			*saturation = (max - min) / (2 - max - min);

		delta = max -min;

		if(red == max)
			*hue = (green - blue) / delta;
		else
		if (green == max)
			*hue = 2 + (blue - red) / delta;
		else
		if (blue == max)
			*hue = 4 + (red - green) / delta;

		*hue *= 60;
		if(*hue < 0.0)
			*hue += 360;
	}
}


static inline gfloat
modula(gfloat number, gint divisor)
{
        gint   whole = gint(number);
        gfloat left  = number - gfloat(whole);
        gfloat res   = gfloat(whole % divisor) + left;
        return res;
}


void
color_from_hsb(gfloat		 	 hue,
               gfloat		 	 saturation,
               gfloat		 	 brightness,
               GtkRtgColor4f    *color           )
{
	gint i;
	gfloat hue_shift[3], color_shift[3];
	gfloat m1, m2, m3;

	if (!color) return;

	if (brightness <= 0.5)
		m2 = brightness * (1 + saturation);
	else
		m2 = brightness + saturation - brightness * saturation;

	m1 = 2 * brightness - m2;

	hue_shift[0] = hue + 120;
	hue_shift[1] = hue;
	hue_shift[2] = hue - 120;

	color_shift[0] = color_shift[1] = color_shift[2] = brightness;

	i = (saturation == 0)?3:0;

	for(; i < 3; i++)
	{
		m3 = hue_shift[i];

		if(m3 > 360)
			m3 = modula(m3, 360);
		else
		if (m3 < 0)
			m3 = 360 - modula(ABS(m3), 360);

		if(m3 < 60)
			color_shift[i] = m1 + (m2 - m1) * m3 / 60;
		else
		if(m3 < 180)
			color_shift[i] = m2;
		else
		if (m3 < 240)
			color_shift[i] = m1 + (m2 - m1) * (240 - m3) / 60;
		else
			color_shift[i] = m1;
	}

	color->r = color_shift[0];
	color->g = color_shift[1];
	color->b = color_shift[2];
	color->a = 1.0;
}


void
shade_color(const GtkRtgColor4f *base,
	        gfloat shade_ratio,
	        GtkRtgColor4f *composite) throw (GtkRtgException)
{
	gfloat hue = 0;
	gfloat saturation = 0;
	gfloat brightness = 0;

	g_throw_if_fail(base && composite);

	if(shade_ratio == 1.0)
	{
		composite->r = base->r;
		composite->g = base->g;
		composite->b = base->b;
		composite->a = base->a;

		return;
	}

	hsb_from_color(base, &hue, &saturation, &brightness);

	brightness = MIN(brightness*shade_ratio, 1.f);
	brightness = MAX(brightness, 0.f);

	saturation = MIN(saturation*shade_ratio, 1.f);
	saturation = MAX(saturation, 0.f);

	color_from_hsb (hue, saturation, brightness, composite);
	composite->a = base->a;
}


void
glColor_from_pointer(void *ptr, guint alpha)
{
	unsigned long val = (long)ptr;

	// hash the pointer up nicely
	val = (val+0x7ed55d16) + (val<<12);
	val = (val^0xc761c23c) ^ (val>>19);
	val = (val+0x165667b1) + (val<<5);
	val = (val+0xd3a2646c) ^ (val<<9);
	val = (val+0xfd7046c5) + (val<<3);
	val = (val^0xb55a4f09) ^ (val>>16);

	GLubyte r = (val>>0) & 0xFF;
	GLubyte g = (val>>8) & 0xFF;
	GLubyte b = (val>>16) & 0xFF;

	GLubyte max = r>g ? (r>b ? r : b) : (g>b ? g : b);

	const int mult = 127;
	const int add = 63;
	r = GLubyte((r*mult)/max + add);
	g = GLubyte((g*mult)/max + add);
	b = GLubyte((b*mult)/max + add);

	glColor4ub(r, g, b, (GLubyte)alpha);
}

} // namespace gtkrtg
