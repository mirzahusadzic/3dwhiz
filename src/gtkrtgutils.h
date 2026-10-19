#ifndef GTK_RTG_UTILS_H
#define GTK_RTG_UTILS_H

#include <gtk/gtk.h>
#include <gtkrtg/gtkrtgcamera.h>
#include <gtkrtg/gtkrtgtext.h>
#include "gtkrtgextras.h"
#include "gtkrtgglu.h"

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

namespace gtkrtg
{

	template<typename T> inline void memset(T *p, T val, guint len){ register guint i=0; for (; i<len; ++i) *p++ = val; }

	/*
 	 * bitfield / bitfield_bit by tom vrx.
 	 *                                              tvrdoljak@yahoo.com
 	 *
 	 * - Fast 'boolean' container to set/get bits.
 	 * - Reallocates smartly when initial size is exceeded.
 	 *
	 **/

	#define INITIAL_SIZE32 2048 /* this is suitable for 65536 entries stored in 8kb */
	#define INITIAL_BITS INITIAL_SIZE32 << 5

	class bitfield_bit
	{
		public:
			bitfield_bit(unsigned int* block_ptr, unsigned int bit_index)
			{
				bit_value = ((*block_ptr & (1u<<bit_index))) >> bit_index;
				base_ptr  = block_ptr;
				bit_mask  = 1u << bit_index;
				bit_shift = bit_index;
			};

			bitfield_bit& operator=(unsigned int value)
			{
				value     = (value != 0);
				bit_value = value;
				if(value) *base_ptr |=  (value<<bit_shift);
				else      *base_ptr &= ~(1u<<bit_shift);
				return *this;
			}

			inline operator int() { return bit_value; };
			inline operator bool(){ return bit_value ? true : false; };

		public:
			unsigned int	bit_value;		/* this is read directly by printf kind of functions */
			unsigned int*	base_ptr;		/* this is a ptr to the base class member memory     */
			unsigned int	bit_mask;		/* this is needed when reading the value             */
			unsigned int	bit_shift;		/* this is needed when writing the value             */
	};


    class bitfield
    {
        public:
            bitfield()
            {
                max_bits = INITIAL_BITS;
                storage = g_new0(unsigned int, INITIAL_SIZE32);
            }

            ~bitfield() { g_free(storage); }
            bitfield(const bitfield &) = delete;
            bitfield &operator=(const bitfield &) = delete;

            inline bitfield_bit operator[](unsigned int index)
            {
                if(index >= max_bits)
                {
                    const size_t requested = std::max(size_t(index) + 1,
                                                     size_t(max_bits) + max_bits / 2);
                    g_throw_if_fail(requested <= G_MAXUINT - 31u);
                    g_throw_if_fail(resize(static_cast<unsigned int>(requested)));
                }
                return bitfield_bit(&storage[index >> 5], index & 31);
            }

            inline void clear() { memset<unsigned int>(storage, 0, max_bits >> 5); }

            inline bool resize(unsigned int bits = INITIAL_BITS)
            {
                if(bits > G_MAXUINT - 31u)
                    return false;
                const unsigned int capacity = std::max(32u, (bits + 31u) & ~31u);
                auto *buffer = static_cast<unsigned int*>(
                    g_try_realloc_n(storage, capacity >> 5, sizeof(unsigned int)));
                if(!buffer)
                    return false;
                storage = buffer;
                if(capacity > max_bits)
                    memset<unsigned int>(storage + (max_bits >> 5), 0, (capacity - max_bits) >> 5);
                max_bits = capacity;
                return true;
            }

            inline unsigned int size() { return max_bits; }

        public:
            unsigned int max_bits;
            unsigned int *storage;
    };

	inline guint
	utils_test_point_frustum(vector3f &p, GtkRtgCamFrustum *frustum)
	{
		/* test all 6 sides of the frustum */
		for(guint j=0; j<FRUSTUM_PLANES; ++j)
		{
			/*
			 * calculate distance from point 'p' to the plane
			 */

			/* test point against the planes */
			gfloat distance = frustum->p[j].normal.dot(p) - frustum->p[j].d;
			if(distance < 0.f)
				return TEST_OUT;
		}

		return TEST_INSIDE;
	}

	inline guint
	utils_test_bbox_frustum(GtkRtgPrimitiveBBox *bbox, GtkRtgCamFrustum *frustum)
	{
		guint total_in = 0;

		/* test all 8 corners against the 6 sides of frustum. If all points are behind 1
		   specific plane, we are out if we are in with all points, then we are fully in */
		for(guint j=0; j<FRUSTUM_PLANES; ++j)
		{
			guint in   = BBOX_CENTER;
			guint side = 1;

			/* bounding box points */
			for(guint i=0; i<BBOX_CENTER; ++i)
			{
				/*
				 * calculate distance from point 'p' to the plane
				 */

				/* test this point against the planes */
				gfloat distance = bbox->p[i].dot(frustum->p[j].normal) - frustum->p[j].d;
				if(distance < 0.f)
				{
					side = 0;
					in--;
				}
			}

			/* were all the points outside of plane p? */
			if(in == 0)
				return TEST_OUT;

			/* check if they were all on the right side of the plane */
			total_in += side;
		}

		if(total_in == FRUSTUM_PLANES)
			return TEST_INSIDE;             /* fully inside */

		return TEST_INTERSECT; /* intersect */
	}

	/*
 	 * Un-projects 3D point onto 2D screen.
 	 * The screen is defined by alpha and beta parameter which describes screen dimensions and center point.
 	 * The 'z' is a value where projection plane is located. It must be a real positive number.
 	 * Aspect is an screen's aspect ratio (viewport_width/viewport_height).
 	 * The screens origin is assumed to be at top left corner (consider flipping screen.y for use in OpengGL).
 	 */
	inline void
	utils_unproject_point2screen(point3f  &p,			/* the point should be in camera space (pre-multiplied by model-view matrix) */
								gfloat    alpha,		/* (0.5*viewport_width-0.5)	  */
								gfloat    beta,			/* (0.5*viewport_height-0.5)      */
								GdkPoint &screen,		/* screen (out) x,y coordinates	  */
								gfloat   z      = 1.f,	/* plane to project	     */
								gfloat   aspect = 1.f)	/* screen aspect ratio (w/h) */
	{
		gfloat  x = z != 1.f ? X(p) / z : X(p);
		gfloat	y = z != 1.f ? Y(p) * aspect / z : Y(p) * aspect;

		screen.x = gint(alpha + alpha*x);
		screen.y = gint(beta  - beta*y);
	}

	inline void
	utils_render_view_axes_text(GtkRtgText *text)
	{
		GdkColor	*color;
		GdkPoint	*position;

		/* query text info */
		g_object_get(G_OBJECT(text), "color", &color, "position", &position, NULL);

		/* set font color */
		glColor3usv(&color->red);

		/* position raster */
		glRasterPos2i(position->x, position->y);

		/* get font-base display list */
		gint *base = (gint*)g_object_get_data(G_OBJECT(text), "font-base");

		/* display list to be used */
		glListBase(*base);

		/* get string */
		gchar	*string = gtk_rtg_text_get_string(text);

		/* execute list */
		glCallLists(strlen(string), GL_UNSIGNED_BYTE, string);
	}

	template<typename T>
	inline static gboolean
	intersect_segment_plane(vector3<T> &a, vector3<T> &b, plane<T> &p, vector3<T> &q)
	{
		/* Compute the t value for the directed line ab intersecting the plane */
		vector3<T> ab = b - a;
		T	   t  = (p.d - p.normal.dot(a)) / p.normal.dot(ab);

		/* If t in [0..1] compute and return intersection point */
		if(t >= 0.f && t <= 1.f)
		{
			q = a + (ab * t);
			return TRUE;
		}

		return FALSE; /* no intersection */
	}

	template<typename T>
	void
	utils_setup_orientation_matrix(vector3<T> &dir, vector3<T> &up, matrix4x4<T> &m)
	{
		vector3<T> _cross =  dir.cross(up).norm();
		vector3<T> _up    = _cross.cross(dir).norm();

		using namespace gtkrtg;

		/* parent 2 local */
		m[_11]= X(_cross); m[_21]= Y(_cross); m[_31]= Z(_cross);
		m[_12]= X(_up);    m[_22]= Y(_up);    m[_32]= Z(_up);
		m[_13]=-X(dir);    m[_23]=-Y(dir);    m[_33]=-Z(dir);
		m[_14]= 0.;	   m[_24]= 0.;	      m[_34]= 0.;

		__gluInvertMatrixf(m.m, m.m);
	}

} /* namespace gtkrtg */
#endif /* GTK_RTG_UTILS_H */
