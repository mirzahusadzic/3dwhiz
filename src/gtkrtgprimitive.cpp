#ifdef _MSC_VER
        #pragma warning(disable : 4786)
#endif

#include <gtkrtg/gtkrtgprimitive.h>
#include <gtkrtg/gtkrtgtexturemap.h>
#include "gtkrtgextras.h"
#include "gtkrtgutils.h"
#include "gtkrtgcolorid.h"

using namespace std;
using namespace gtkrtg;

#define GTK_RTG_PRIMITIVE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_RTG_TYPE_PRIMITIVE, GtkRtgPrimitivePrivate))

G_DEFINE_TYPE(GtkRtgPrimitive, gtk_rtg_primitive, G_TYPE_OBJECT)


#define PERC_OF_BBOX_SIDE .2f /* percents of bbox. sides to render selected-lines */

enum{ A, B, C, D, E, F, G, H, AB, AD, AG, BA, BC, BH, CB, CD, CE, DC, DA, DF, EH, EF, EC, FE, FG, FD, GF, GH, GA, HG, HE, HB, SELBOX_POINTS };

struct GtkRtgPrimitiveSelBox
{
	point3f  p[SELBOX_POINTS]; /* selbox points     */
	GdkColor color; /* selbox line color */
};

enum /* sub-primitive selections */
{
	SPS_POINT,
	SPS_EDGE,
	SPS_POLY,
	SP_SELS
};


struct GtkRtgPrimSubSelect
{
	gtkrtg::bitfield bit; /* bit-set    (display)    */
	set<guint_ind> res;	/* result-set (manipulate) */
};

struct _GtkRtgPrimitivePrivate
{
	/*
	 * mesh data (essentials)
	 *
	 */
    vector<point3f> *verts; /* vertices */
	vector<vector3f> *vnormals; /* vertex nomals */
	vector<point2f> *texcoords; /* texture coordinates */
	vector<guint_ind> *vindices; /* vertex indices */
	vector<guint_ind> *tindices; /* text. coord. indices */
	set<guint_ind2> *edges; /* edges */

	/* selection */
    GtkRtgPrimSubSelect *select[SP_SELS]; /* sub-primitive selections */

    guint color_id; /* meshes color id */
	GdkColor color; /* mesh color */
	gchar *name; /* mesh name */
	GdkColor point_color; /* mesh points color */
	GdkColor point_sel_color; /* mesh points color */
    point3f pivot; /* pivot point */
	GtkRtgPrimitiveSelBox selbox; /* selection box */
	GtkRtgPrimitiveBBox bbox; /* bounding bbox */
	GtkRtgPrimitiveBSphere bsphere; /* bounding sphere */

	/* textures */
    GtkRtgTexture *texture[MAX_TEXTURE_UNITS]; /* up to n units */

	/* clone */
	GtkRtgPrimitive *clone_inst; /* clone instance */

	/* editing */
	GtkRtgPrimEditMode editmode; /* edit mode (def. object)*/

	/* props */
    gfloat axes_ratio;

    /* flags */
	gboolean selected;
	gboolean frozen;
	gboolean visible;

	/*
	 * Transformations (translation, rotation and scale)
	 */

	vector3f scale;
	vector3f translation;
	quat_f quat;

	/* matrix */
	matrix4x4f mrigid;	/* rotation & translation */
};

/* signals */
enum
{
	TRANSFORMED,
	RENDER_EVENT,
	LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = { 0 };

/* global incremental primitives color id */
static guint GTK_RTG_PRIMITIVE_COLOR_ID = COLOR_ID_PRIMITIVE;

/* defaults */
#define GTK_RTG_PRIMITIVE_COLOR "salmon"
#define GTK_RTG_PRIMITIVE_POINT_COLOR "#5454f0"
#define GTK_RTG_PRIMITIVE_POINT_SEL_COLOR "#d00000"
#define GTK_RTG_PRIMITIVE_SELBBOX_COLOR "white"

static void	gtk_rtg_primitive_finalize(GObject *object);

/* privs. */
static void	_gtk_rtg_primitive_render_selected(GtkRtgPrimitive *prim);
static void	_gtk_rtg_primitive_render_points(GtkRtgPrimitive *prim);
void calc_bounding_sphere(GtkRtgPrimitive *prim);

typedef set<guint_ind2>	s_edge_type;

/* macros */
void
gl_prepare_matrix_stack(GtkRtgPrimitive *prim)
{
	glTranslatef(X(prim->priv->translation), Y(prim->priv->translation), Z(prim->priv->translation));
    glScalef(X(prim->priv->scale), Y(prim->priv->scale), Z(prim->priv->scale));
	glMultMatrixf(prim->priv->mrigid.m);
}

point3f
object2world(GtkRtgPrimitive *prim, point3f &point)
{
	point3f p = prim->priv->mrigid * point;

	X(p) *= X(prim->priv->scale);
	Y(p) *= Y(prim->priv->scale);
	Z(p) *= Z(prim->priv->scale);

	p += prim->priv->translation;

	return p;
}

point3f
world2object(GtkRtgPrimitive *prim, point3f &point)
{
	matrix4x4f inv = prim->priv->mrigid.inverse();
	point3f p = point - prim->priv->translation;
    X(p) /= X(prim->priv->scale);
    Y(p) /= Y(prim->priv->scale);
    Z(p) /= Z(prim->priv->scale);
    p = inv * p;

	return p;
}

matrix4x4f
get_rotation_matrix(GtkRtgPrimitive *prim)
{
	matrix4x4f mrigid = prim->priv->mrigid;
	mrigid[tx]=mrigid[ty]=mrigid[tz]=0.f;
	return mrigid;
}


static void
gtk_rtg_primitive_class_init(GtkRtgPrimitiveClass *klass)
{
	GObjectClass    *object_class = (GObjectClass *) klass;

	object_class->finalize = gtk_rtg_primitive_finalize;

	/*
     * Signal(s)
     *
     *
     */

	/*
	 * GtkRtgPrimitive: render-event
	 *
	 * Primitive emits this signal after transfomation matrix has been pushed to the matrix stack.
	 *
	 */

    signals[RENDER_EVENT] = g_signal_new("render-event",
                                         G_OBJECT_CLASS_TYPE(klass),
                                         (GSignalFlags)G_SIGNAL_RUN_FIRST,
                                         G_STRUCT_OFFSET(GtkRtgPrimitiveClass, render_event),
                                         NULL,
                                         NULL,
                                         g_cclosure_marshal_VOID__UINT,
                                         G_TYPE_NONE,
                                         1,
                                         G_TYPE_UINT);
	/*
	 * GtkRtgPrimitive: transformed
	 *
	 * Primitve emits this signal after setup of transformation matrix (translate, scale or rotate).
	 *
	 */

    signals[TRANSFORMED] = g_signal_new("transformed",
                                        G_OBJECT_CLASS_TYPE(klass),
                                        (GSignalFlags)G_SIGNAL_RUN_FIRST,
                                        G_STRUCT_OFFSET(GtkRtgPrimitiveClass, transformed),
                                        NULL,
                                        NULL,
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE,
                                        0);
	klass->render_event = NULL;

	g_type_class_add_private(G_OBJECT_CLASS(klass), sizeof(GtkRtgPrimitivePrivate));
}


GtkRtgPrimitive *
gtk_rtg_primitive_new()
{
	return GTK_RTG_PRIMITIVE(g_object_new(GTK_RTG_TYPE_PRIMITIVE, NULL));
}


void
gtk_rtg_primitive_clone(GtkRtgPrimitive *prim, GtkRtgPrimitive *dest, gboolean instance) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	if(instance)
	{
		/*
		 * Instantiate (ptr) primitive (mesh) data
		 *
		 */

		delete dest->priv->verts;
		delete dest->priv->vnormals;
		delete dest->priv->vindices;
		delete dest->priv->texcoords;
		delete dest->priv->tindices;
		delete dest->priv->edges;

		/* save & incref clone source */
		dest->priv->clone_inst = prim;
		g_object_ref(dest->priv->clone_inst);

		dest->priv->verts     = prim->priv->verts;
		dest->priv->vnormals  = prim->priv->vnormals;
		dest->priv->vindices  = prim->priv->vindices;
		dest->priv->texcoords = prim->priv->texcoords;
		dest->priv->tindices  = prim->priv->tindices;
		dest->priv->edges     = prim->priv->edges;
	}else
	{
		/*
		 * Copy mesh data
		 *
		 */

		dest->priv->verts->resize(prim->priv->verts->size());
		copy(prim->priv->verts->begin(), prim->priv->verts->end(), dest->priv->verts->begin());

		dest->priv->vnormals->resize(prim->priv->vnormals->size());
		copy(prim->priv->vnormals->begin(), prim->priv->vnormals->end(), dest->priv->vnormals->begin());

		dest->priv->vindices->resize(prim->priv->vindices->size());
		copy(prim->priv->vindices->begin(), prim->priv->vindices->end(), dest->priv->vindices->begin());

		dest->priv->texcoords->resize(prim->priv->texcoords->size());
		copy(prim->priv->texcoords->begin(), prim->priv->texcoords->end(), dest->priv->texcoords->begin());

		dest->priv->tindices->resize(prim->priv->tindices->size());
		copy(prim->priv->tindices->begin(), prim->priv->tindices->end(), dest->priv->tindices->begin());

		dest->priv->edges->clear();
		for(s_edge_type::const_iterator it = prim->priv->edges->begin(); it != prim->priv->edges->end(); ++it)
			dest->priv->edges->insert(*it);
	}

	/*
	 * Common
	 *
	 */

	dest->priv->pivot = prim->priv->pivot;

	memcpy(&dest->priv->selbox, &prim->priv->selbox, sizeof(GtkRtgPrimitiveSelBox));
	memcpy(&dest->priv->bbox, &prim->priv->bbox, sizeof(GtkRtgPrimitiveBBox));
	memcpy(&dest->priv->bsphere, &prim->priv->bsphere, sizeof(GtkRtgPrimitiveBSphere));

	dest->priv->axes_ratio = prim->priv->axes_ratio;
	dest->priv->selected = prim->priv->selected;
	dest->priv->scale = prim->priv->scale;

	dest->priv->quat = prim->priv->quat;
	dest->priv->mrigid = prim->priv->mrigid;
}


GtkRtgPrimitive *
gtk_rtg_primitive_get_clone_instance(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return prim->priv->clone_inst;
}


static void
gtk_rtg_primitive_init(GtkRtgPrimitive *prim)
{
	prim->priv = GTK_RTG_PRIMITIVE_GET_PRIVATE(prim);

	/* associate color id */
	prim->priv->color_id  = GTK_RTG_PRIMITIVE_COLOR_ID++;

	/* get colors */
	gdk_color_parse(GTK_RTG_PRIMITIVE_COLOR, &prim->priv->color);
	gdk_color_parse(GTK_RTG_PRIMITIVE_POINT_COLOR, &prim->priv->point_color);
	gdk_color_parse(GTK_RTG_PRIMITIVE_POINT_SEL_COLOR, &prim->priv->point_sel_color);
	gdk_color_parse(GTK_RTG_PRIMITIVE_SELBBOX_COLOR, &prim->priv->selbox.color);

	/* essentials */
	prim->priv->verts     = new vector<point3f>;
	prim->priv->vnormals  = new vector<point3f>;
	prim->priv->vindices  = new vector<guint_ind>;
	prim->priv->texcoords = new vector<point2f>;
	prim->priv->tindices  = new vector<guint_ind>;
	prim->priv->edges     = new set<guint_ind2>;

	/* identity matrix */
	prim->priv->mrigid.identity();

	/* default edit mode */
	prim->priv->editmode = GTK_RTG_EDIT_OBJECT;

	/* selections */
	for(guint i=0; i<SP_SELS; i++)
			prim->priv->select[i] = new GtkRtgPrimSubSelect;

	/* set visible */
	prim->priv->visible = TRUE;

	/* identity */
	prim->priv->quat.identity();

	X(prim->priv->scale)=Y(prim->priv->scale)=Z(prim->priv->scale)=1.f;
}


static void
gtk_rtg_primitive_finalize(GObject *object)
{
	GtkRtgPrimitive *prim = GTK_RTG_PRIMITIVE(object);

	if(IS_CLONE(prim))
	{
		g_object_unref(prim->priv->clone_inst);
		prim->priv->clone_inst = NULL;
	}else
	{
		if(prim->priv->verts)    { delete prim->priv->verts;     prim->priv->verts     = NULL; }
		if(prim->priv->vnormals) { delete prim->priv->vnormals;  prim->priv->vnormals  = NULL; }
		if(prim->priv->vindices) { delete prim->priv->vindices;  prim->priv->vindices  = NULL; }
		if(prim->priv->texcoords){ delete prim->priv->texcoords; prim->priv->texcoords = NULL; }
		if(prim->priv->tindices) { delete prim->priv->tindices;  prim->priv->tindices  = NULL; }
		if(prim->priv->edges)    { delete prim->priv->edges;     prim->priv->edges     = NULL; }
		if(prim->priv->name)	 { g_free(prim->priv->name);	 prim->priv->name      = NULL; }

		for(guint i=0; i<SP_SELS; i++)
				if(prim->priv->select[i]) delete prim->priv->select[i];
	}

	/*
	 * Release texture objects
	 *
	 */

	for(guint i=0; i<8; ++i)

		if(prim->priv->texture[i])
		{
			g_object_unref(G_OBJECT(prim->priv->texture[i]));
			prim->priv->texture[i] = NULL;
		}

	G_OBJECT_CLASS(gtk_rtg_primitive_parent_class)->finalize(object);
}


/*
 * Privs.
 *
 *
 */


inline static void
_gtk_rtg_primitive_render_selected(GtkRtgPrimitive *prim)
{
	GLfloat	line_width;

	glGetFloatv(GL_LINE_WIDTH, &line_width);
	glLineWidth(1.f);

	glColor3usv(&prim->priv->selbox.color.red);

	gboolean a,b,c,d,e,f;
	GLint bsrc, bdst;

	GL_ENABLE_LINES(a,b,c,d,e,f, bsrc, bdst);

	/*
	 * Render 'incomplete' bounding box volume around primitives mesh.
	 *
	 */

	glBegin(GL_LINES);

		for(guint i=0; i<8; i++)
		{
			guint idx = 8 + (i*3);

			for(guint j=0; j<3; j++)
			{
				glVertex3fv(VALUES(prim->priv->selbox.p[i]));
				glVertex3fv(VALUES(prim->priv->selbox.p[idx + j]));
			}
		}

	glEnd();

	/* rewind back line width */
	if(line_width != 1.f)
		glLineWidth(line_width);

	GL_DISABLE_LINES(a,b,c,d,e,f, bsrc, bdst);
}

/*
static void
_gtk_rtg_primitive_render_pivot(GtkRtgPrimitive *prim)
{
 	GLfloat ps;
	glGetFloatv(GL_POINT_SIZE, &ps);
	glPointSize(12.75f);

	gboolean a,b,c,d,e;
	DISABLE_GL_COLOR_MODIFIERS(a,b,c,d,e);
	glEnable(GL_POINT_SMOOTH);

	glBegin(GL_POINTS);
		glColor3usv(&prim->priv->point_color.red);
		glVertex3fv(VALUES(prim->priv->pivot));
	glEnd();

	glDisable(GL_POINT_SMOOTH);
	glPointSize(ps);
	ENABLE_GL_COLOR_MODIFIERS(a,b,c,d,e);
}
*/

static void
_gtk_rtg_primitive_render_points(GtkRtgPrimitive *prim)
{
	GLfloat ps;
	glGetFloatv(GL_POINT_SIZE, &ps);
	glPointSize(2.75f);

	gboolean a,b,c,d,e;
	DISABLE_GL_COLOR_MODIFIERS(a,b,c,d,e);
	glEnable(GL_POINT_SMOOTH);

	glBegin(GL_POINTS);

		for(guint_ind i=0; i<prim->priv->verts->size(); ++i)
		{
			if(prim->priv->select[SPS_POINT]->bit[i])
				glColor3usv(&prim->priv->point_sel_color.red);
			else
				glColor3usv(&prim->priv->point_color.red);

			glVertex3fv(VALUES((*prim->priv->verts)[i]));
		}
	glEnd();

	glDisable(GL_POINT_SMOOTH);
	glPointSize(ps);
	ENABLE_GL_COLOR_MODIFIERS(a,b,c,d,e);
}

/*
 * Publics
 *
 */

void
gtk_rtg_primitive_set_vertices(GtkRtgPrimitive *prim, vector<point3f> *verts) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	prim->priv->verts->clear();
	prim->priv->verts->resize(verts->size());
	copy(verts->begin(), verts->end(), prim->priv->verts->begin());
}


vector<point3f>  *
gtk_rtg_primitive_get_vertices(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return prim->priv->verts;
}


void
gtk_rtg_primitive_set_vnormals(GtkRtgPrimitive *prim, vector<vector3f> *vnormals) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	prim->priv->vnormals->clear();
	prim->priv->vnormals->resize(vnormals->size());
	copy(vnormals->begin(), vnormals->end(), prim->priv->vnormals->begin());
}


vector<vector3f> *
gtk_rtg_primitive_get_vnormals(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return prim->priv->vnormals;
}


void
gtk_rtg_primitive_set_vindices(GtkRtgPrimitive *prim, vector<guint_ind> *vindices) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	prim->priv->vindices->clear();
	prim->priv->vindices->resize(vindices->size());
	copy(vindices->begin(), vindices->end(), prim->priv->vindices->begin());
}


vector<guint_ind> *
gtk_rtg_primitive_get_vindices(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return prim->priv->vindices;
}


void
gtk_rtg_primitive_set_texcoords(GtkRtgPrimitive *prim, vector<point2f> *texcoords) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	prim->priv->texcoords->clear();
	prim->priv->texcoords->resize(texcoords->size());
	copy(texcoords->begin(), texcoords->end(), prim->priv->texcoords->begin());
}


vector<point2f> *
gtk_rtg_primitive_get_texcoords(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return prim->priv->texcoords;
}


void
gtk_rtg_primitive_set_tindices(GtkRtgPrimitive *prim, vector<guint_ind> *tindices) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	prim->priv->tindices->clear();
	prim->priv->tindices->resize(tindices->size());
	copy(tindices->begin(), tindices->end(), prim->priv->tindices->begin());
}


vector<guint_ind> *
gtk_rtg_primitive_get_tindices(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return prim->priv->tindices;
}


void
gtk_rtg_primitive_set_edges(GtkRtgPrimitive *prim, set<guint_ind2> *edges) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	prim->priv->edges->clear();
	for(s_edge_type::const_iterator it = edges->begin(); it != edges->end(); ++it)
		prim->priv->edges->insert(*it);
}


set<guint_ind2> *
gtk_rtg_primitive_get_edges(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return prim->priv->edges;
}


void
gtk_rtg_primitive_translate(GtkRtgPrimitive *prim, gtkrtg::vector3f &trans) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	/* offset relative */
	prim->priv->translation += trans;

	/* emit 'transformed' event */
	g_signal_emit(prim, signals[TRANSFORMED], 0);
}


void
gtk_rtg_primitive_scale(GtkRtgPrimitive *prim, gtkrtg::vector3f &scale) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	g_throw_if_fail(X(scale) != 0.f);
	g_throw_if_fail(Y(scale) != 0.f);
	g_throw_if_fail(Z(scale) != 0.f);

	/*
	 * Scale safety check.
	 * - not allowed to go smaller than 0.001f in any direction (x,y,z).
	 */

	/* get bbox diagonal */
	vector3f d = prim->priv->bbox.p[BBOX_A] - prim->priv->bbox.p[BBOX_E];

	/* scale diagonal */
	X(d) *= X(scale); Y(d) *= Y(scale); Z(d) *= Z(scale);

	/* safety check */
	if(fabs(X(d)) < 0.001f || fabs(Y(d)) < 0.001f || fabs(Z(d)) < 0.001f)
		return;

	/*
	 * Scale ok
	 */

	/* scale relative */
	X(prim->priv->scale) *= X(scale);
	Y(prim->priv->scale) *= Y(scale);
	Z(prim->priv->scale) *= Z(scale);

	/* emit 'transformed' event */
	g_signal_emit(prim, signals[TRANSFORMED], 0);
}


void
gtk_rtg_primitive_rotate_from_axis_angle(GtkRtgPrimitive *prim, gtkrtg::vector3f &axes, gfloat angle_deg) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	quat_f quat;
	quat.rotate_axis(axes, deg2rad(angle_deg));

	matrix4x4f orient;
	orient.from_quaternion(quat);

	/* concatenate */
	prim->priv->mrigid = orient * prim->priv->mrigid;

	prim->priv->quat = prim->priv->mrigid.to_quaternion();

	/* emit 'transformed' event */
	g_signal_emit(prim, signals[TRANSFORMED], 0);
}


void
gtk_rtg_primitive_rotate_from_quaternion(GtkRtgPrimitive *prim, gtkrtg::quat_f &quat) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	matrix4x4f orient;
	orient.from_quaternion(quat);

	/* offset by pivot */
	point3f pivot = prim->priv->pivot * (-1.f);
	matrix4x4f t1, t2;
	t1.set_translation(pivot);
	t2.set_translation(prim->priv->pivot);

	orient = t1 * orient * t2;

	/* concatenate */
	prim->priv->mrigid = orient * prim->priv->mrigid;

	prim->priv->quat = prim->priv->mrigid.to_quaternion();

	/* emit 'transformed' event */
	g_signal_emit(prim, signals[TRANSFORMED], 0);
}


gtkrtg::vector3f &
gtk_rtg_primitive_get_translation(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return prim->priv->translation;
}


gtkrtg::quat_f &
gtk_rtg_primitive_get_orientation(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return prim->priv->quat;
}


matrix4x4f &
gtk_rtg_primitive_get_transformation_matrix(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return prim->priv->mrigid;
}


void
gtk_rtg_primitive_render(GtkRtgPrimitive *prim, GtkRtgPrimitiveRenderMode mode) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	if(prim->priv->visible)
	{
		glPushMatrix();

			gl_prepare_matrix_stack(prim);

			/* emit 'render-event' (fcon: handle return value) */
			g_signal_emit(prim, signals[RENDER_EVENT], 0, mode);

			switch(mode)
			{
				case    GTK_RTG_RENDER_POINTS:	_gtk_rtg_primitive_render_points(prim);		break;
				case    GTK_RTG_RENDER_EDGES:
						{
							for(s_edge_type::const_iterator it  = prim->priv->edges->begin(); it != prim->priv->edges->end(); ++it)
							{
								/* get indices */
								EDGE_INDICES(it);

								glBegin(GL_LINES);

									glVertex3fv(VALUES((*prim->priv->verts)[i0]));
									glVertex3fv(VALUES((*prim->priv->verts)[i1]));

								glEnd();
							}

							break;
						}

				/* do nothing */
				case    GTK_RTG_RENDER_SOLID:
				case	GTK_RTG_RENDER_SELECT:  break;

				default: break;
			};

			/* render 'editing' points */
			if(prim->priv->editmode == GTK_RTG_EDIT_POINTS && mode != GTK_RTG_RENDER_POINTS)
				_gtk_rtg_primitive_render_points(prim);

			/* render selection (selbox) */
			if(prim->priv->selected)
				_gtk_rtg_primitive_render_selected(prim);

			/* render pivot */
            //_gtk_rtg_primitive_render_pivot(prim);

		glPopMatrix();
	}
}


guint
gtk_rtg_primitive_get_color_id(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return prim->priv->color_id;
}


void
calc_bounding_sphere(GtkRtgPrimitive *prim)
{
	guint verts = prim->priv->verts->size();

	if(!verts) return;

	point3f pmin = object2world(prim, (*prim->priv->verts)[0]);
	point3f pmax = object2world(prim, (*prim->priv->verts)[0]);

	/* find min & max points */
	for(guint i=1; i<verts; ++i)
	{
		vector3f v = object2world(prim, (*prim->priv->verts)[i]);

		/* get min point */
		if(X(v) < X(pmin)) X(pmin) = X(v);
		if(Y(v) < Y(pmin)) Y(pmin) = Y(v);
		if(Z(v) < Z(pmin)) Z(pmin) = Z(v);

		/* get max point */
		if(X(v) > X(pmax)) X(pmax) = X(v);
		if(Y(v) > Y(pmax)) Y(pmax) = Y(v);
		if(Z(v) > Z(pmax)) Z(pmax) = Z(v);
	}

	/* diagonal / 2 */
	point3f d_2 = (pmax - pmin) * .5f;

	/* get axes ratio x:y:z (1.f is a perfect sphere) */
	gfloat daxes = MAX3(X(d_2), Y(d_2), Z(d_2));
	prim->priv->axes_ratio = (X(d_2) + Y(d_2) + Z(d_2)) / daxes;
	prim->priv->axes_ratio *= .3333333f;

	/*
	 * calculate bounding sphere
	 *
	 */

	/* set spheres center as bbox's center */
	prim->priv->bsphere.center = prim->priv->bbox.p[BBOX_E] + d_2;

	/* find most distant point from the spheres center */
	prim->priv->bsphere.r = 0.f;

	/* transformed center */
	point3f center = object2world(prim, prim->priv->bsphere.center);

	for(guint i=0; i<verts; ++i)
	{
		/* calculate real radius as radius cannot be recalculated later */
		vector3f v = object2world(prim, (*prim->priv->verts)[i]);
		float len = (center - v).mag();

		if(len > prim->priv->bsphere.r)
			prim->priv->bsphere.r = len;
	}
}


void
gtk_rtg_primitive_calc_bbox_bsphere(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	guint verts = prim->priv->verts->size();

	if(!verts) return;

	point3f pmin = (*prim->priv->verts)[0];
	point3f pmax = (*prim->priv->verts)[0];

	/* find min & max points */
	for(guint i=1; i<verts; ++i)
	{
		vector3f& v = (*prim->priv->verts)[i];

		/* get min point */
		if(X(v) < X(pmin)) X(pmin) = X(v);
		if(Y(v) < Y(pmin)) Y(pmin) = Y(v);
		if(Z(v) < Z(pmin)) Z(pmin) = Z(v);

		/* get max point */
		if(X(v) > X(pmax)) X(pmax) = X(v);
		if(Y(v) > Y(pmax)) Y(pmax) = Y(v);
		if(Z(v) > Z(pmax)) Z(pmax) = Z(v);
	}

	/* diagonal / 2 */
	point3f d_2 = (pmax - pmin) * .5f;

	/* get axes ratio x:y:z (1.f is a perfect sphere) */
	gfloat   daxes          = MAX3(X(d_2), Y(d_2), Z(d_2));
	prim->priv->axes_ratio  = (X(d_2) + Y(d_2) + Z(d_2)) / daxes;
	prim->priv->axes_ratio *= .3333333f;

	/*
	 * calculate bounding sphere
	 *
	 */

	/* set spheres center as bbox's center */
	prim->priv->bsphere.center = pmin + d_2;

	/* find most distant point from the spheres center */
	prim->priv->bsphere.r = 0.f;
	for(guint j=0; j<verts; ++j)
	{
		vector3f& v = (*prim->priv->verts)[j];
		float len = (prim->priv->bsphere.center - v).mag();

		if(len > prim->priv->bsphere.r)
			prim->priv->bsphere.r = len;
	}

	/*
	 * construct 9pts bounding box and selbox
	 *
	 */

	prim->priv->bbox.p[BBOX_CENTER] = prim->priv->bsphere.center;
	prim->priv->selbox.p[A]  = prim->priv->bbox.p[BBOX_A] = pmax;
	prim->priv->selbox.p[B]  = prim->priv->bbox.p[BBOX_B] = point3f(X(pmin), Y(pmax), Z(pmax));
	prim->priv->selbox.p[C]  = prim->priv->bbox.p[BBOX_C] = point3f(X(pmin), Y(pmin), Z(pmax));
	prim->priv->selbox.p[D]  = prim->priv->bbox.p[BBOX_D] = point3f(X(pmax), Y(pmin), Z(pmax));
	prim->priv->selbox.p[E]  = prim->priv->bbox.p[BBOX_E] = pmin;
	prim->priv->selbox.p[F]  = prim->priv->bbox.p[BBOX_F] = point3f(X(pmax), Y(pmin), Z(pmin));
	prim->priv->selbox.p[G]  = prim->priv->bbox.p[BBOX_G] = point3f(X(pmax), Y(pmax), Z(pmin));
	prim->priv->selbox.p[H]  = prim->priv->bbox.p[BBOX_H] = point3f(X(pmin), Y(pmax), Z(pmin));
	prim->priv->selbox.p[AB] = prim->priv->selbox.p[A] - (prim->priv->selbox.p[A] - prim->priv->selbox.p[B]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[AD] = prim->priv->selbox.p[A] - (prim->priv->selbox.p[A] - prim->priv->selbox.p[D]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[AG] = prim->priv->selbox.p[A] - (prim->priv->selbox.p[A] - prim->priv->selbox.p[G]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[BA] = prim->priv->selbox.p[B] - (prim->priv->selbox.p[B] - prim->priv->selbox.p[A]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[BC] = prim->priv->selbox.p[B] - (prim->priv->selbox.p[B] - prim->priv->selbox.p[C]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[BH] = prim->priv->selbox.p[B] - (prim->priv->selbox.p[B] - prim->priv->selbox.p[H]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[CB] = prim->priv->selbox.p[C] - (prim->priv->selbox.p[C] - prim->priv->selbox.p[B]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[CD] = prim->priv->selbox.p[C] - (prim->priv->selbox.p[C] - prim->priv->selbox.p[D]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[CE] = prim->priv->selbox.p[C] - (prim->priv->selbox.p[C] - prim->priv->selbox.p[E]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[DC] = prim->priv->selbox.p[D] - (prim->priv->selbox.p[D] - prim->priv->selbox.p[C]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[DA] = prim->priv->selbox.p[D] - (prim->priv->selbox.p[D] - prim->priv->selbox.p[A]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[DF] = prim->priv->selbox.p[D] - (prim->priv->selbox.p[D] - prim->priv->selbox.p[F]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[EH] = prim->priv->selbox.p[E] - (prim->priv->selbox.p[E] - prim->priv->selbox.p[H]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[EF] = prim->priv->selbox.p[E] - (prim->priv->selbox.p[E] - prim->priv->selbox.p[F]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[EC] = prim->priv->selbox.p[E] - (prim->priv->selbox.p[E] - prim->priv->selbox.p[C]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[FE] = prim->priv->selbox.p[F] - (prim->priv->selbox.p[F] - prim->priv->selbox.p[E]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[FG] = prim->priv->selbox.p[F] - (prim->priv->selbox.p[F] - prim->priv->selbox.p[G]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[FD] = prim->priv->selbox.p[F] - (prim->priv->selbox.p[F] - prim->priv->selbox.p[D]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[GF] = prim->priv->selbox.p[G] - (prim->priv->selbox.p[G] - prim->priv->selbox.p[F]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[GH] = prim->priv->selbox.p[G] - (prim->priv->selbox.p[G] - prim->priv->selbox.p[H]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[GA] = prim->priv->selbox.p[G] - (prim->priv->selbox.p[G] - prim->priv->selbox.p[A]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[HG] = prim->priv->selbox.p[H] - (prim->priv->selbox.p[H] - prim->priv->selbox.p[G]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[HE] = prim->priv->selbox.p[H] - (prim->priv->selbox.p[H] - prim->priv->selbox.p[E]) * PERC_OF_BBOX_SIDE;
	prim->priv->selbox.p[HB] = prim->priv->selbox.p[H] - (prim->priv->selbox.p[H] - prim->priv->selbox.p[B]) * PERC_OF_BBOX_SIDE;
}


GtkRtgPrimitiveBBox *
gtk_rtg_primitive_get_bbox(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return &prim->priv->bbox;
}


GtkRtgPrimitiveBSphere *
gtk_rtg_primitive_get_bsphere(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return &prim->priv->bsphere;
}


void
gtk_rtg_primitive_select(GtkRtgPrimitive *prim, gboolean select) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	/* selection is ignored if primitive is already frozen */
	prim->priv->selected = prim->priv->frozen ? FALSE : select;
}


gboolean
gtk_rtg_primitive_is_selected(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return prim->priv->selected;
}


void
gtk_rtg_primitive_freeze(GtkRtgPrimitive *prim, gboolean freeze) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	prim->priv->frozen = freeze;
}


gboolean
gtk_rtg_primitive_is_frozen(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return prim->priv->frozen;
}

void
gtk_rtg_primitive_set_visible(GtkRtgPrimitive *prim, gboolean visible) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	prim->priv->visible = visible;
}

gboolean
gtk_rtg_primitive_is_visible(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return prim->priv->visible;
}

void
gtk_rtg_primitive_set_color(GtkRtgPrimitive *prim, const GdkColor *color) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	prim->priv->color = *color;
}


GdkColor *
gtk_rtg_primitive_get_color(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return &prim->priv->color;
}


void
gtk_rtg_primitive_set_name(GtkRtgPrimitive *prim, gchar *name) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	g_throw_if_fail(name != NULL);

	if(prim->priv->name)
		g_free(prim->priv->name);

	prim->priv->name = g_strdup(name);
}


gchar*
gtk_rtg_primitive_get_name(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return prim->priv->name ? g_strdup(prim->priv->name) : prim->priv->name;
}


void
gtk_rtg_primitive_set_pivot(GtkRtgPrimitive *prim, point3f *pivot) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	prim->priv->pivot = *pivot;
}


point3f *
gtk_rtg_primitive_get_pivot(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return &prim->priv->pivot;
}


gfloat
gtk_rtg_primitive_get_axes_ratio(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return prim->priv->axes_ratio;
}


void
gtk_rtg_primitive_set_edit_mode(GtkRtgPrimitive *prim, GtkRtgPrimEditMode mode) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	/* toggle modes */
	prim->priv->editmode = mode != prim->priv->editmode ? mode
						            					: GTK_RTG_EDIT_OBJECT;
}


GtkRtgPrimEditMode
gtk_rtg_primitive_get_edit_mode(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return prim->priv->editmode;
}


void
gtk_rtg_primitive_select_vertex(GtkRtgPrimitive *prim, guint_ind idx) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	g_throw_if_fail(idx < prim->priv->verts->size());

	prim->priv->select[SPS_POINT]->bit[idx] = 1;
	prim->priv->select[SPS_POINT]->res.insert(idx);
}


void
gtk_rtg_primitive_vertices_select_all(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
}


void
gtk_rtg_primitive_vertices_deselect_all(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	prim->priv->select[SPS_POINT]->bit.clear();
	prim->priv->select[SPS_POINT]->res.clear();
}


set<guint_ind> *
gtk_rtg_primitive_get_selected_vertices(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return &prim->priv->select[SPS_POINT]->res;
}


void
gtk_rtg_primitive_select_edge(GtkRtgPrimitive *prim, guint_ind idx) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	g_throw_if_fail(idx < prim->priv->edges->size());
}


void
gtk_rtg_primitive_egdes_select_all(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
}


void
gtk_rtg_primitive_egdes_deselect_all(GtkRtgPrimitive *prim)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	prim->priv->select[SPS_EDGE]->bit.clear();
	prim->priv->select[SPS_EDGE]->res.clear();
}



set<guint_ind> *
gtk_rtg_primitive_get_selected_edges(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return &prim->priv->select[SPS_EDGE]->res;
}


void
gtk_rtg_primitive_select_poly(GtkRtgPrimitive *prim, guint_ind idx) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
}


void
gtk_rtg_primitive_polys_select_all(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

}

void
gtk_rtg_primitive_polys_deselect_all(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	prim->priv->select[SPS_POLY]->bit.clear();
	prim->priv->select[SPS_POLY]->res.clear();
}


set<guint_ind> *
gtk_rtg_primitive_get_selected_polys(GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	return &prim->priv->select[SPS_POLY]->res;
}


void
gtk_rtg_primitive_set_texture(GtkRtgPrimitive *prim, GtkRtgTexture *texture, guint unit) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	g_throw_if_fail(GTK_RTG_IS_TEXTURE(texture));
	g_throw_if_fail(unit < MAX_TEXTURE_UNITS);

	/*
	 * Release 'old' texture from the unit
	 */

	if(prim->priv->texture[unit])
		g_object_unref(G_OBJECT(prim->priv->texture[unit]));

	/*
	 * Set primitive to be owner of the texture (on 1st set)
	 *
	 */

	if(g_object_is_floating(texture))
		g_object_ref_sink(texture);
	else
		g_object_ref(G_OBJECT(texture));

	/*
	 * Add 'new' texture to the unit
	 */

	prim->priv->texture[unit] = texture;
}


GtkRtgTexture *
gtk_rtg_primitive_get_texture(GtkRtgPrimitive *prim, guint unit) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));
	g_throw_if_fail(unit < MAX_TEXTURE_UNITS);
	return prim->priv->texture[unit];
}
