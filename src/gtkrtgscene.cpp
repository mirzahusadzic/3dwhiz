#ifdef _MSC_VER
        #pragma warning(disable : 4786)
#endif

#include <gtkrtg/gtkrtgtrimesh.h>
#include <gtkrtg/gtkrtgscene.h>
#include <gtkrtg/gtkrtgcamera.h>
#include <gtkrtg/gtkrtgcanvas.h>

#include <map>
#include <vector>
#include <algorithm>
#include <functional>

#include "gtkrtgresources.h"
#include "gtkrtgextras.h"
#include "gtkrtgutils.h"
#include "gtkrtg_gl_specs.h"
#include "marshalers.h"
#include "gtkrtgcolorid.h"

#define GTK_RTG_SCENE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_RTG_TYPE_SCENE, GtkRtgScenePrivate))

G_DEFINE_TYPE(GtkRtgScene, gtk_rtg_scene, G_TYPE_OBJECT)

using namespace std;
using namespace gtkrtg;

typedef map<guint, GtkRtgPrimitive*, uiCmp> m_PrimitiveType;
typedef set<GtkRtgPrimitive*> s_prim_type;
typedef map<GtkRtgPrimitive*, s_prim_type*, ptrCmp> m_CameraType;

struct _GtkRtgScenePrivate
{
	/* scene objects */
	m_PrimitiveType *m_prims; /* scene prims. map */
	s_prim_type *s_prims_all; /* scene prims. set */
	s_prim_type *s_prims; /* selected prims. */
	m_CameraType *m_cams; /* scene cameras */
	GtkRtgCamera *cam; /* default camera */
	GdkColor ee_color; /* exclusive prim. edge line color */
	gboolean backface; /* backface selection? */
	GtkRtgCamFrustum frustum;

	/* id tracker */
	gchar *id; /* scene id {instance ptr} */
};

static void	gtk_rtg_scene_finalize(GObject *object);
static void gtk_rtg_scene_camera_prioritize(GtkRtgCamera *camera, GtkRtgCanvas *canvas, gpointer data);
static void gtk_rtg_scene_camera_view(GtkRtgCamera *camera, GParamSpec *pspec, GtkRtgScene *scene);
static void gtk_rtg_scene_camera_projection(GtkRtgCamera *camera, GParamSpec *pspec, GtkRtgScene *scene);
/*static void gtk_rtg_scene_camera_proj_aspect_ratio (GtkRtgCamera *camera, GParamSpec *pspec, GtkRtgScene *scene);*/
static void	_gtk_rtg_scene_do_frustum_culling(GtkRtgScene *scene, GtkRtgCamera *camera);
static guint _gtk_rtg_scene_primitive_is_in_frustum(GtkRtgPrimitive *prim, GtkRtgCamFrustum *frustum);
static void	_gtk_rtg_primitve_transformed(GtkRtgPrimitive *prim, gpointer data);
static void gtk_rtg_scene_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gtk_rtg_scene_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void _gtk_rtg_scene_render_select(GtkRtgScene *scene, GtkRtgPrimitive *prim, GtkRtgPrimEditMode mode, GtkRtgCanvas  *canvas);
extern void gl_prepare_matrix_stack(GtkRtgPrimitive *prim);
extern point3f object2world(GtkRtgPrimitive *prim, point3f &point);
extern matrix4x4f get_rotation_matrix(GtkRtgPrimitive *prim);

/* properties */
enum
{
	PROP_0,
	PROP_TOGGLE_EDIT_MODE,
	LAST_ARG
};


enum
{
	S_PROP_TOGGLE_EDIT_MODE,
	S_LAST_PROP
};

static const gchar *prop[S_LAST_PROP]={"toggle-edit-mode"};

/* signals */
enum
{
	CAMERA_DEFAULT,
	PRIM_SEL_CHANGED,
	CONFIGURED,
	LAST_SIGNAL
};

static  gint signals[LAST_SIGNAL] = { 0 };

#define FRUSTUM_CULL(scene, camera) \
{ \
	_gtk_rtg_scene_do_frustum_culling(scene, camera); \
}


/* exclusive 'edit' mode */
#define SCENE_EXCLUSIVE(scene)(scene->priv->s_prims->size() == 1)

#define DESELECT_PRIMITIVES(scene) \
{ \
	if(scene->priv->s_prims->size()) \
    { \
		for(s_prim_type::const_iterator it = scene->priv->s_prims->begin(); it != scene->priv->s_prims->end(); ++it) \
        	SELECT(*it, FALSE); \
        \
        scene->priv->s_prims->clear();	\
    } \
}

#define SELECT_PRIMITIVE(scene, prim, select) \
{ \
	SELECT(prim, select); \
}


/*
 * Hide backface {points, edges, polys} with colorized primitive
 *
 */
#define OBSCURE_BACKFACE(scene, prim) \
{ \
	if(!IS_BACKFACE_SEL(scene)) \
	{ \
		gboolean select = IS_SELECTED(prim); \
		GtkRtgPrimEditMode m = EDIT_MODE(prim); \
		glColor3ub(255,255,254); /* fcon: fix? */ \
		gtk_rtg_primitive_set_edit_mode(prim, GTK_RTG_EDIT_OBJECT); /* do not render points */ \
		gtk_rtg_primitive_render(prim, GTK_RTG_RENDER_SOLID); \
		gtk_rtg_primitive_select(prim, select); /* do not render selection box */ \
		gtk_rtg_primitive_set_edit_mode(prim, m); \
	} \
}


#define MARQUEE_CHECK_BOUNDS(pb, start, end) \
{ \
	if(start.x < 0) start.x = 0; \
	if(end.y < 0) end.y = 0; \
	if(end.x > pb->width) end.x = pb->width-1; \
	if(start.y > pb->height) start.y = pb->height-1; \
}


#define XY_CHECK_BOUNDS(pb, point) \
{ \
	if(point.x > pb->width)	point.x = pb->width-1; \
	else \
	if(point.x < 0) point.x = 0; \
	if(point.y > pb->height) point.y = pb->height-1; \
	else \
	if(point.y < 0) point.y = 0; \
}

/* defaults */
#define GTK_RTG_SCENE_EXCLUSIVE_EDGE_COLOR      "#ffffff"
#define GTK_RTG_SCENE_EXCLUSIVE_EDGE_LINE_WIDTH .55f


static void
gtk_rtg_scene_class_init(GtkRtgSceneClass *klass)
{
	GObjectClass  *object_class = (GObjectClass *) klass;
	object_class->set_property  = gtk_rtg_scene_set_property;
	object_class->get_property  = gtk_rtg_scene_get_property;
	object_class->finalize	    = gtk_rtg_scene_finalize;

	/*
	 * GtkRtgScene: toggle-edit-mode
	 *
	 * Alternate between scene edit modes.
	 *
	 */
	g_object_class_install_property(object_class,
									PROP_TOGGLE_EDIT_MODE,
                                    g_param_spec_uint(prop[S_PROP_TOGGLE_EDIT_MODE],
													  prop[S_PROP_TOGGLE_EDIT_MODE],
													  "Alternate between scene edit modes.",
													  0,
													  GTK_RTG_EDIT_MODES,
													  0,
													  G_PARAM_WRITABLE));
	/*
	 * Signal(s)
	 *
	 *
	 */

	/*
	 * GtkRtgScene: camera-default
	 *
	 * Scene emits this signal when default/active or no camera is set.
	 *
	 */

    signals[CAMERA_DEFAULT] = g_signal_new("camera-default",
                                           G_OBJECT_CLASS_TYPE(klass),
                                           (GSignalFlags)G_SIGNAL_RUN_FIRST,
                                           G_STRUCT_OFFSET(GtkRtgSceneClass, camera_default),
                                           NULL,
                                           NULL,
                                           gtkrtg_marshal__VOID__OBJECT_OBJECT,
                                           G_TYPE_NONE,
                                           2,
                                           G_TYPE_OBJECT,
                                           G_TYPE_OBJECT);
	/*
	 * GtkRtgScene: prim-sel-changed
	 *
	 * Scene emits this signal when primitive selection is changed.
	 *
	 */
    signals[PRIM_SEL_CHANGED] = g_signal_new("prim-sel-changed",
                                             G_OBJECT_CLASS_TYPE(klass),
                                             (GSignalFlags)G_SIGNAL_RUN_FIRST,
                                             G_STRUCT_OFFSET(GtkRtgSceneClass, prim_sel_changed),
                                             NULL,
                                             NULL,
                                             g_cclosure_marshal_VOID__VOID,
                                             G_TYPE_NONE,
                                             0);
	/*
	 * GtkRtgScene: configured
	 *
	 * A scene emits this signal when it gets configured (i.e. edit mode changes).
	 *
	 */

    signals[CONFIGURED] = g_signal_new("configured",
                                       G_OBJECT_CLASS_TYPE(klass),
                                       (GSignalFlags)G_SIGNAL_RUN_FIRST,
                                       G_STRUCT_OFFSET(GtkRtgSceneClass, configured),
                                       NULL,
                                       NULL,
                                       g_cclosure_marshal_VOID__VOID,
                                       G_TYPE_NONE,
                                       0);

	g_type_class_add_private(G_OBJECT_CLASS(klass), sizeof(GtkRtgScenePrivate));
}


GtkRtgScene *
gtk_rtg_scene_new()
{
	return GTK_RTG_SCENE(g_object_new(GTK_RTG_TYPE_SCENE, NULL));
}


static void
gtk_rtg_scene_init(GtkRtgScene *scene)
{
	scene->priv = GTK_RTG_SCENE_GET_PRIVATE(scene);

	/* build canvas id: {instance ptr, :} */
	scene->priv->id = g_strdup_printf("%p%c", (void *)scene, ':');

	/* alloc */
	scene->priv->m_prims     = new m_PrimitiveType;
	scene->priv->s_prims_all = new s_prim_type;
	scene->priv->s_prims     = new s_prim_type;
	scene->priv->m_cams      = new m_CameraType;

	/* get colors */
	gdk_color_parse(GTK_RTG_SCENE_EXCLUSIVE_EDGE_COLOR, &scene->priv->ee_color);
}


static void
gtk_rtg_scene_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GtkRtgScene *scene = GTK_RTG_SCENE(object);

	switch(prop_id)
	{
		case PROP_TOGGLE_EDIT_MODE: gtk_rtg_scene_toggle_edit_mode(scene, (GtkRtgPrimEditMode)g_value_get_uint(value));	break;
	};

}


static void
gtk_rtg_scene_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	/*
	switch(prop_id)
	{
	};
	*/
}


static void
gtk_rtg_scene_finalize(GObject *object)
{
	GtkRtgScene *scene = GTK_RTG_SCENE(object);

	/*
	 * Free primitives render map.
	 * - Pop-out if any unpoped primitive.
	 */

	if(scene->priv->m_prims)
	{
		for(m_PrimitiveType::const_iterator it = scene->priv->m_prims->begin(); it != scene->priv->m_prims->end(); ++it)
		{
			/* pop-out */
			gchar *scene_prim_id = (gchar*) g_object_get_data(G_OBJECT(it->second), scene->priv->id);
			if(scene_prim_id)
			{
				delete scene_prim_id; scene_prim_id = NULL;
				g_object_set_data(G_OBJECT(it->second), scene->priv->id, scene_prim_id);
			}

			if(GTK_RTG_IS_CAMERA(it->second))
			{
				g_signal_handlers_disconnect_by_func(it->second, (void *)gtk_rtg_scene_camera_prioritize,	 scene);
				g_signal_handlers_disconnect_by_func(it->second, (void *)gtk_rtg_scene_camera_view,		 scene);
				g_signal_handlers_disconnect_by_func(it->second, (void *)gtk_rtg_scene_camera_projection,	 scene);
				/*g_signal_handlers_disconnect_by_func(it->second, gtk_rtg_scene_camera_proj_aspect_ratio, scene);*/

				if(scene->priv->cam == GTK_RTG_CAMERA(it->second))
					scene->priv->cam = NULL;
			}

			/* disconnect 'transformed' */
			g_signal_handlers_disconnect_by_func(it->second, (void *)_gtk_rtg_primitve_transformed, scene);

			g_object_unref(it->second);
		}

                delete scene->priv->m_prims; scene->priv->m_prims = NULL;
	}

	/*
	 * Free set of all primitives.
	 *
	 */
	if(scene->priv->s_prims_all){ delete scene->priv->s_prims_all; scene->priv->s_prims_all = NULL; }

	/*
	 * Free set of selected primitives.
	 *
	 */
	if(scene->priv->s_prims){ delete scene->priv->s_prims; scene->priv->s_prims = NULL; }

	/*
	 * Free map of cameras culled primitives.
	 *
	 */
	if(scene->priv->m_cams)
	{
		for(m_CameraType::const_iterator it = scene->priv->m_cams->begin(); it != scene->priv->m_cams->end(); ++it)
			delete it->second;

		delete scene->priv->m_cams;
		scene->priv->m_cams = NULL;
	}

	/*
	 * Scene id.
	 *
	 */
	if(scene->priv->id){ g_free(scene->priv->id ); scene->priv->id = NULL; }

	G_OBJECT_CLASS(gtk_rtg_scene_parent_class)->finalize(object);
}

/*
 * Publics
 *
 *
 */


/***
 *** This function render mesh(es) from the render map.
 ***
 ***/
void
gtk_rtg_scene_render(GtkRtgScene *scene, GtkRtgSceneRenderMode mode, GtkRtgCamera *camera) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_SCENE(scene));

	std::set<GtkRtgPrimitive*> *prims = NULL;

	if(camera)
	{
		m_CameraType::iterator  it = scene->priv->m_cams->find(GTK_RTG_PRIMITIVE(camera));

		if(it!= scene->priv->m_cams->end())
			prims = it->second;
		else
			g_warning("The camera is not found in the scene map. Use 'gtk_rtg_scene_push' to add camera to the scene. Rendering whole scene!\n");
	}

	if(!prims) prims = scene->priv->s_prims_all;

	switch(mode)
	{
		case GTK_RTG_RENDER_SCENE_SOLID:
			{
				gboolean a,b,c,d;
				GL_ENABLE_POLYS(a,b,c,d);

				glEnable(GL_POLYGON_OFFSET_FILL);
				glPolygonOffset(1.5f, 1.5f);

				for(s_prim_type::const_iterator it  = prims->begin(); it != prims->end(); ++it)
				{
					glColor3usv(&COLOR(*it)->red);
					gtk_rtg_primitive_render(*it, GTK_RTG_RENDER_SOLID);
				}

				glDisable(GL_POLYGON_OFFSET_FILL);

				GL_DISABLE_POLYS(a,b,c,d);
				break;
			}

		case GTK_RTG_RENDER_SCENE_EDGES:
			{
				gboolean a,b,c,d,e,f;
				GLint bsrc, bdst;

				GL_ENABLE_LINES(a,b,c,d,e,f, bsrc, bdst);

				GLfloat	line_width;
				glGetFloatv(GL_LINE_WIDTH, &line_width);
				glLineWidth(0.75f);

				for(s_prim_type::const_iterator it  = prims->begin(); it != prims->end(); ++it)
				{
					if(IS_SELECTED(*it))
						glColor3usv(&scene->priv->ee_color.red);
					else
						glColor3usv(&COLOR(*it)->red);

					gtk_rtg_primitive_render(*it, GTK_RTG_RENDER_EDGES);
				}

				glLineWidth(line_width);

				GL_DISABLE_LINES(a,b,c,d,e,f, bsrc, bdst);

				break;
			}

		case GTK_RTG_RENDER_SCENE_SOLID_EDGES:
			{
				glEnable(GL_POLYGON_OFFSET_FILL);
				glPolygonOffset(1.5f, 1.5f);

				for(s_prim_type::const_iterator it  = prims->begin(); it != prims->end(); ++it)
				{
					glColor3usv(&COLOR(*it)->red);
					gtk_rtg_primitive_render(*it, GTK_RTG_RENDER_SOLID);
				}
				glDisable(GL_POLYGON_OFFSET_FILL);

				gboolean a,b,c,d,e,f;
				GLint bsrc, bdst;

				GL_ENABLE_LINES(a,b,c,d,e,f, bsrc, bdst);

				GLfloat	line_width;
				glGetFloatv(GL_LINE_WIDTH, &line_width);
				glLineWidth(GTK_RTG_SCENE_EXCLUSIVE_EDGE_LINE_WIDTH);

				glPolygonOffset(1.f, 1.f);

				for(s_prim_type::const_iterator it = prims->begin(); it != prims->end(); ++it)
				{
					if(IS_SELECTED(*it))
						glColor3usv(&scene->priv->ee_color.red);
					else
						glColor3usv(&COLOR(*it)->red);

					gtk_rtg_primitive_render(*it, GTK_RTG_RENDER_EDGES);
				}

				glDisable(GL_POLYGON_OFFSET_FILL);
				glLineWidth(line_width);
				GL_DISABLE_LINES(a,b,c,d,e,f, bsrc, bdst);

				break;
			}
		default: break;
	};
}


/***
 *** This function pushes primitive to the primitives map.
 *** Pushed primitive cannot be pushed again until is poped. Such push will
 *** throw GtkRtgException.
 ***/
void
gtk_rtg_scene_primitive_push(GtkRtgScene *scene, GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_SCENE(scene));
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	/* check whether primitive is already pushed */
	guint *scene_prim_id = (guint*) g_object_get_data(G_OBJECT(prim), scene->priv->id);
	g_throw_if_fail(scene_prim_id == NULL);

	/* create objects scene id */
	scene_prim_id = CREATE_PRIMITIVE_ID(COLOR_ID(prim));

	/* set to the object */
	g_object_set_data(G_OBJECT(prim), scene->priv->id, scene_prim_id);

	/* add to the map */
	scene->priv->m_prims->insert(make_pair(*scene_prim_id, prim));

	/* add to the set */
	scene->priv->s_prims_all->insert(prim);

	/* connect to 'transformed' event */
	g_signal_connect(prim, "transformed", G_CALLBACK(_gtk_rtg_primitve_transformed), scene);

	if(GTK_RTG_IS_CAMERA(prim))
	{
		/* add to the map */
		s_prim_type *s_prims = new s_prim_type;
		scene->priv->m_cams->insert(make_pair(prim, s_prims));

		/* connect */
		g_signal_connect(G_OBJECT(prim), "prioritize",          G_CALLBACK(gtk_rtg_scene_camera_prioritize),        scene);
		g_signal_connect(G_OBJECT(prim), "notify::view-matrix", G_CALLBACK(gtk_rtg_scene_camera_view),	            scene);
		g_signal_connect(G_OBJECT(prim), "notify::proj-matrix", G_CALLBACK(gtk_rtg_scene_camera_projection),	    scene);

		/* setup 'default' camera */
		if(!scene->priv->cam)
		{
			scene->priv->cam = GTK_RTG_CAMERA(prim);
			/*g_signal_connect(scene->priv->cam, "notify::proj-aspect", G_CALLBACK(gtk_rtg_scene_camera_proj_aspect_ratio), scene);*/
			g_signal_emit(scene, signals[CAMERA_DEFAULT], 0, scene->priv->cam, NULL);
		}
	}

	/* cull scene */
	if(scene->priv->cam)
		FRUSTUM_CULL(scene, scene->priv->cam);

	/* inref primitive */
	g_object_ref(prim);
}


/***
 *** This function pops primitives from the render map.
 ***
 ***/
void
gtk_rtg_scene_primitive_pop(GtkRtgScene *scene, GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_SCENE(scene));
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	/* check whether mesh is pushed */
	guint *scene_prim_id = (guint*) g_object_get_data(G_OBJECT(prim), scene->priv->id);

	if(scene_prim_id)
	{
		m_PrimitiveType::iterator iter = scene->priv->m_prims->find(*scene_prim_id);

		if(iter != scene->priv->m_prims->end())
		{
			delete scene_prim_id; scene_prim_id = NULL;
			g_object_set_data(G_OBJECT(prim), scene->priv->id, scene_prim_id);

			/* erase from culled maps */
			for(m_CameraType::const_iterator it = scene->priv->m_cams->begin(); it != scene->priv->m_cams->end(); ++it)
			{
				if(it->second->find(iter->second) != it->second->end())
					it->second->erase(iter->second);
			}

			/* erase */
			if(scene->priv->s_prims_all->find(iter->second) != scene->priv->s_prims_all->end())
				scene->priv->s_prims_all->erase(iter->second);

			if(scene->priv->s_prims->find(iter->second) != scene->priv->s_prims->end())
				scene->priv->s_prims->erase(iter->second);

			scene->priv->m_prims->erase(iter);

			if(GTK_RTG_IS_CAMERA(prim))
			{
				/* erase from cameras map */
				m_CameraType::iterator it = scene->priv->m_cams->find(prim);

				if(it != scene->priv->m_cams->end())
				{
					delete it->second;
					scene->priv->m_cams->erase(it);
				}

				g_signal_handlers_disconnect_by_func(prim, (void *)gtk_rtg_scene_camera_prioritize, scene);
				g_signal_handlers_disconnect_by_func(prim, (void *)gtk_rtg_scene_camera_view,	    scene);
				g_signal_handlers_disconnect_by_func(prim, (void *)gtk_rtg_scene_camera_projection, scene);
				/*g_signal_handlers_disconnect_by_func(prim, gtk_rtg_scene_camera_proj_aspect_ratio, scene);*/

				/* 'default' camera is lost, emit NULL */
				if(scene->priv->cam == GTK_RTG_CAMERA(prim))
				{
					scene->priv->cam = NULL;
					g_signal_emit(scene, signals[CAMERA_DEFAULT], 0, NULL, NULL);
				}
			}

			/* disconnect 'transformed' */
			g_signal_handlers_disconnect_by_func(prim, (void *)_gtk_rtg_primitve_transformed, scene);

			/* decref primitive */
			g_object_unref(prim);
		}
	}
}


/***
 *** Check whether primitive is already pushed to the scene.
 ***
 ***/
gboolean
gtk_rtg_scene_primitive_is_pushed(GtkRtgScene *scene, GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_SCENE(scene));
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	guint *scene_prim_id  = (guint*) g_object_get_data(G_OBJECT(prim), scene->priv->id);
	return scene_prim_id != NULL;
}


const set<GtkRtgPrimitive*> *
gtk_rtg_scene_get_culled_prims(GtkRtgScene *scene, GtkRtgCamera *camera) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_SCENE(scene));

	if(camera)
	{
		g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));

		m_CameraType::iterator it = scene->priv->m_cams->find(GTK_RTG_PRIMITIVE(camera));

		if(it != scene->priv->m_cams->end())
		{
			FRUSTUM_CULL(scene, camera);
			return it->second;
		}
	}else
	if(scene->priv->cam)
	{
		m_CameraType::iterator it = scene->priv->m_cams->find(GTK_RTG_PRIMITIVE(scene->priv->cam));

		if(it != scene->priv->m_cams->end())
		{
			FRUSTUM_CULL(scene, scene->priv->cam);
			return it->second;
		}
	}

	return NULL;
}


const std::set<GtkRtgPrimitive*> *
gtk_rtg_scene_get_all_prims(GtkRtgScene *scene) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_SCENE(scene));
	return scene->priv->s_prims_all;
}


GtkRtgCamera *
gtk_rtg_scene_camera_get_default(GtkRtgScene *scene) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_SCENE(scene));
	return scene->priv->cam;
}


/*
 * Privs.
 *
 */

inline void
_gtk_rtg_scene_render_select(GtkRtgScene        *scene,
                             GtkRtgPrimitive    *prim,
                             GtkRtgPrimEditMode  mode,
                             GtkRtgCanvas		*canvas)
{
	GtkRtgCamera *camera = gtk_rtg_canvas_camera_get_default(canvas);
	GtkRtgPickingBuffer *pb = (GtkRtgPickingBuffer*) g_object_get_data(G_OBJECT(canvas), "pickbuff");

	/* no update is required */
	if(!pb->update) return;

	m_CameraType::iterator  it = scene->priv->m_cams->find(GTK_RTG_PRIMITIVE(camera));

	const std::set<GtkRtgPrimitive*> *prims = NULL;

	if(it!= scene->priv->m_cams->end())
		prims = it->second;

	if(!prims) prims = scene->priv->s_prims_all;

	GLfloat color[4];
	glGetFloatv(GL_COLOR_CLEAR_VALUE, color);

	/* pure white */
	glClearColor(1.f, 1.f, 1.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	gboolean a,b,c,d,e;
	DISABLE_GL_COLOR_MODIFIERS(a,b,c,d,e);

	gint shade_model;
	glGetIntegerv(GL_SHADE_MODEL, &shade_model);
	glShadeModel(GL_FLAT);


	switch(mode)
	{
		/* culled primitives */
		case GTK_RTG_EDIT_OBJECT:
			 {
				/* render prims. in select mode */
				 for(s_prim_type::const_iterator iter = prims->begin(); iter != prims->end(); ++iter)
				 {
					 gboolean select = IS_SELECTED(*iter);
					 GtkRtgPrimEditMode m = EDIT_MODE(*iter);

					 gtk_rtg_primitive_render(*iter, GTK_RTG_RENDER_SELECT);

					 gtk_rtg_primitive_select(*iter, select);
					 gtk_rtg_primitive_set_edit_mode(*iter, m);
				}

				break;
			 }

		/* selected primitive */
		case GTK_RTG_EDIT_POINTS:
			 {
				OBSCURE_BACKFACE(scene, prim);

				GLfloat ps;
				glGetFloatv(GL_POINT_SIZE, &ps);
				glPointSize(5.0f);

				vector<point3f> &verts = *VERTS(prim);

				glPushMatrix();
					gl_prepare_matrix_stack(prim);

					glBegin(GL_POINTS);

						for(guint i=0; i<verts.size(); ++i)
						{
							guint color_id = COLOR_ID_PRIMITIVE + i;
							glColor3ubv((guchar*)&color_id);
							glVertex3fv(VALUES(verts[i]));
						}
					glEnd();

				glPopMatrix();

				glPointSize(ps);

				break;
			 }

		case GTK_RTG_EDIT_EDGES:
			 {
				OBSCURE_BACKFACE(scene, prim);
				break;
			 }


		case GTK_RTG_EDIT_POLYS:
			 {
				OBSCURE_BACKFACE(scene, prim);
				break;
			 }

		default: break;
	};

	/* emit signal render-offscreen */
	g_signal_emit_by_name(canvas, "render-offscreen");

	ENABLE_GL_COLOR_MODIFIERS(a,b,c,d,e);

	/* rewind clear color */
	glClearColor(color[0], color[1], color[2], color[3]);

	if(shade_model != GL_FLAT)
		glShadeModel(shade_model);

	/* refresh selection buffer */
	glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glReadPixels(0, 0, pb->width, pb->height, GL_RGBA, GL_UNSIGNED_BYTE, pb->buff);
	pb->update = 0;
}


inline static gint
_xy_select_any_primitive(GtkRtgScene *scene, GdkPoint &point, GtkRtgCanvas *canvas, gboolean test)
{
	GtkRtgPickingBuffer *pb = (GtkRtgPickingBuffer*) g_object_get_data(G_OBJECT(canvas), "pickbuff");
	gint ret = -1;

	/*
	 * selection buffer is uninitialized (default camera is not set)
	 */

	if(pb->buff == NULL)
		return 0;

	XY_CHECK_BOUNDS(pb, point);

	/* read pixel from the back buffer */
	guint pos  = point.y ? (point.y-1) * 4 * pb->width : 0;
	      pos += point.x * 4;

	GLubyte *pixel = &pb->buff[pos];

	/* pack color id */
	guint color_id = pixel[2] << 16 | pixel[1] << 8 | pixel[0];

	/* query from mesh map */
	m_PrimitiveType::iterator iter = scene->priv->m_prims->find(color_id);

	if(iter != scene->priv->m_prims->end())
	{
		/* not actually select */
		if(test) return IS_NOT_FROZEN(iter->second) ? color_id : -1;

                /* deselect meshes and clear 'sel-vector' */
	        DESELECT_PRIMITIVES(scene);

		if(IS_NOT_FROZEN(iter->second))
		{
			/* add to the list of selected meshes */
			scene->priv->s_prims->insert(iter->second);

			/* notify mesh that is selected */
			SELECT_PRIMITIVE(scene, iter->second, TRUE);

			/* color id */
			ret = COLOR_ID(iter->second);
		}

		/* emit signal */
		g_signal_emit(scene, signals[PRIM_SEL_CHANGED], 0);
	}

	/*
	 * Background is clicked (no selection)
	 *
	 */

	return ret;
}



inline static gint
_xy_select_any_point(GtkRtgScene *scene, GtkRtgPrimitive *prim, GdkPoint &point, GtkRtgCanvas *canvas, gboolean test)
{
	GtkRtgPickingBuffer *pb = (GtkRtgPickingBuffer*) g_object_get_data(G_OBJECT(canvas), "pickbuff");

	/*
	* selection buffer is uninitialized (default camera is not set)
	*/

	if(pb->buff == NULL)
		return 0;

	XY_CHECK_BOUNDS(pb, point);

	if(!test)
                /* fcon: implement ctrl */
		gtk_rtg_primitive_vertices_deselect_all(prim);

	/* read pixel from the back buffer */
	guint pos  = point.y ? (point.y-1) * 4 * pb->width : 0;
	      pos += point.x * 4;

	GLubyte *pixel = &pb->buff[pos];

	/* pack color id */
	guint color_id  = pixel[2] << 16 | pixel[1] << 8 | pixel[0];
	      color_id -= COLOR_ID_PRIMITIVE;

	vector<point3f> &verts = *VERTS(prim);

	if(color_id < verts.size())
	{
		/* not actualy select */
		if(test) return color_id;

		/* select vertex */
		gtk_rtg_primitive_select_vertex(prim, (guint_ind)color_id);

		return color_id;
	}

	return test ? -1 : 0;
}



/***
 ***  This method picks any (single) of {object, point, edge or poly} depending on
 ***  primitive 'edit' mode.
 ***  If there is only one mesh selected ('scene exclusive') then primitive
 ***  edit mode is checked.
 ***  Function returns '-1' if background is clicked for object selection ('_xy_select_any').
 ***  When primitive is edited if nothing is clicked return value is '0'.
 ***/
gint
gtk_rtg_scene_point_select(GtkRtgScene   *scene,
                           GdkPoint      &point,
                           GtkRtgCanvas *canvas,
                           gboolean      test) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_SCENE(scene));
	g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
	g_throw_if_fail(gtk_rtg_canvas_camera_get_default(canvas) != NULL);

	gint ret = 0;

	if(SCENE_EXCLUSIVE(scene))
	{
		 /* render to selection buffer */
		 _gtk_rtg_scene_render_select(scene,
									  *scene->priv->s_prims->begin(),
									  EDIT_MODE(*scene->priv->s_prims->begin()),
									  canvas);

		/* pick any of {object, point, edge or poly} */
		switch(EDIT_MODE(*scene->priv->s_prims->begin()))
		{
									  /* pick any primitive from the scene */
			case GTK_RTG_EDIT_OBJECT: ret = _xy_select_any_primitive(scene, point, canvas, test);
			  break;

									  /* pick any vertex from the selected primitive */
			case GTK_RTG_EDIT_POINTS: ret = _xy_select_any_point(scene, *scene->priv->s_prims->begin(), point, canvas, test);
									  break;

									  /* pick any edge from the selected primitive */
			case GTK_RTG_EDIT_EDGES:  break;

									  /* pick any poly (tri, quad) from the selected primitive */
			case GTK_RTG_EDIT_POLYS:  break;

			default: break;
		};

	}else
	{
		/* pick any primitive from the scene */
		_gtk_rtg_scene_render_select(scene, NULL, GTK_RTG_EDIT_OBJECT, canvas);
		ret = _xy_select_any_primitive(scene, point, canvas, test);
	}

	return ret;
}


inline static gint
marquee_select_any_primitive(GtkRtgScene *scene, GdkPoint &start, GdkPoint &end, GtkRtgCanvas *canvas)
{
	GdkPoint s = start;
	GdkPoint e = end;

	GtkRtgCamera *camera = gtk_rtg_canvas_camera_get_default(canvas);
	GtkRtgPickingBuffer *pb = (GtkRtgPickingBuffer*) g_object_get_data(G_OBJECT(canvas), "pickbuff");

	std::set<GtkRtgPrimitive*> *prims = NULL;

	if(!camera) prims = scene->priv->s_prims_all;
	else
	{
		m_CameraType::iterator it = scene->priv->m_cams->find(GTK_RTG_PRIMITIVE(camera));
		if(it != scene->priv->m_cams->end())
				prims = it->second;
	}

	start.x = MIN(s.x, e.x); end.x = MAX(s.x, e.x);
	start.y = MAX(s.y, e.y); end.y = MIN(s.y, e.y);

	MARQUEE_CHECK_BOUNDS(pb, start, end);

	gtk_rtg_camera_build_frustum(camera ? camera : scene->priv->cam, start, end, scene->priv->frustum);

	/*
	 * Make fast selection depending on the bbox or bsphere.
	 */
	for(s_prim_type::const_iterator it = prims->begin(); it != prims->end(); ++it)
	{
		if(IS_NOT_FROZEN(*it) && _gtk_rtg_scene_primitive_is_in_frustum(*it, &scene->priv->frustum))
			scene->priv->s_prims->insert(*it);
	}

	/*
	 * Filter primitives depending on real vertices.
	 */
	s_prim_type erase;
	for(s_prim_type::const_iterator it = scene->priv->s_prims->begin(); it != scene->priv->s_prims->end(); ++it)
	{
		vector<point3f>  &verts    = *VERTS(*it);

		guint i=0;
		for(; i<verts.size(); ++i)
		{
			/* transform point */
			point3f p = object2world(*it, verts[i]);

			/* test if point is in frustum? */
			if(utils_test_point_frustum(p, &scene->priv->frustum) == TEST_INSIDE)
			{
				SELECT_PRIMITIVE(scene, *it, TRUE);
				break;
			}
		}

		/* false hit: all points are tested, erase from list */
		if(i==verts.size())
		{
			SELECT_PRIMITIVE(scene, *it, FALSE); /* deselect */
			erase.insert(*it);
		}
	}

	for(s_prim_type::const_iterator it = erase.begin(); it != erase.end(); ++it)
		scene->priv->s_prims->erase(*it);    /* erase */

	/* emit signal */
	g_signal_emit(scene, signals[PRIM_SEL_CHANGED], 0);

	return scene->priv->s_prims->size() ? scene->priv->s_prims->size() : -1;
}


inline static gint
_marquee_select_any_point(GtkRtgScene *scene, GtkRtgPrimitive *prim, GdkPoint &start, GdkPoint &end, GtkRtgCanvas *canvas)
{
	GdkPoint s = start;
	GdkPoint e = end;

	GtkRtgPickingBuffer *pb = (GtkRtgPickingBuffer*) g_object_get_data(G_OBJECT(canvas), "pickbuff");
	GtkRtgCamera *camera = gtk_rtg_canvas_camera_get_default(canvas);

	if(!camera)
		camera = scene->priv->cam;

	start.x = MIN(s.x, e.x); end.x = MAX(s.x, e.x);
	start.y = MAX(s.y, e.y); end.y = MIN(s.y, e.y);

	MARQUEE_CHECK_BOUNDS(pb, start, end);

	/* fcon: implement ctrl */
	gtk_rtg_primitive_vertices_deselect_all(prim);

	gtk_rtg_camera_build_frustum(camera, start, end, scene->priv->frustum);

	/* rotation matrix only (for normals) */
	matrix4x4f m_rot = get_rotation_matrix(prim);

	vector<point3f>  &verts    = *VERTS(prim);
	vector<vector3f> &vnormals = *VNORMALS(prim);
	point3f           cam_eye  =  cast3<float, double>(*EYE_POINT(camera));

	for(guint_ind i=0; i<verts.size(); ++i)
	{
		/* transform vertex into world space */
		point3f p = object2world(prim, verts[i]);

		/* eject backface vert? */
		if(!scene->priv->backface && vnormals.size() /* are vert. normals available? */)
		{
			/* test if point is back-facing? */
			vector3f eye2pt = cam_eye - p;
			vector3f n = m_rot * vnormals[i];	        /* orient normal */
			gfloat   dot = n.dot(eye2pt.norm());

			/*
			 *  check angle between vertex normal
			 *  and vector from the camera to the vertex
			 *  - eject if angle is obtuse.
			 */

			if(dot < 0.f)
				continue;
		}

		/* test point against plane normals */
		guint j=0;
		for(; j<FRUSTUM_PLANES; j++)
		{
			/*
			 * calculate distance from point 'p' to the plane
			 *
			 */

			gfloat distance = scene->priv->frustum.p[j].normal.dot(p) - scene->priv->frustum.p[j].d;

			if(distance < 0.f)
				break;
		}

		/* yap: the point is in frustum */
		if(j==FRUSTUM_PLANES)
			gtk_rtg_primitive_select_vertex(prim, i);
	}

	return SEL_VERTS(prim)->size() ? SEL_VERTS(prim)->size() : -1;
}


/***
 ***  This method picks any (multiple) of {objects, points, edges or polys} depending on
 ***  primitive 'edit' mode.
 ***  If there is only one mesh selected ('scene exclusive') then primitive
 ***  edit mode is checked.
 ***  Function returns '-1' if background is selected for object selection ('_marquee_select_any').
 ***  When primitive is edited if nothing is selected return value is '0'.
 ***/
gint
gtk_rtg_scene_marquee_select(GtkRtgScene *scene, GdkPoint &start, GdkPoint &end, GtkRtgCanvas *canvas) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_SCENE(scene));
	g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));
	g_throw_if_fail(gtk_rtg_canvas_camera_get_default(canvas) != NULL);

	gint ret = 0;

	if(SCENE_EXCLUSIVE(scene))
	{
		/* pick any of {objects, points, edges or polys} */
		switch(EDIT_MODE((*scene->priv->s_prims->begin())))
		{
									  /* pick primitives from the scene */
			case GTK_RTG_EDIT_OBJECT: ret = marquee_select_any_primitive(scene, start, end, canvas); break;

									  /* pick vertices from the selected primitive */
			case GTK_RTG_EDIT_POINTS: ret = _marquee_select_any_point(scene, *scene->priv->s_prims->begin(), start, end, canvas);
									  break;

									  /* pick edges from the selected primitive */
			case GTK_RTG_EDIT_EDGES:  break;

									  /* pick polys (tris, quads) from the selected primitive */
			case GTK_RTG_EDIT_POLYS:  break;

			default: break;
		};

	}else
	/* pick primitives from the scene */
		ret = marquee_select_any_primitive(scene, start, end, canvas);

	return ret;
}


const set<GtkRtgPrimitive*> *
gtk_rtg_scene_get_selected(GtkRtgScene *scene) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_SCENE(scene));
	return scene->priv->s_prims;
}


gboolean
gtk_rtg_scene_deselect_all(GtkRtgScene *scene) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_SCENE(scene));
	gboolean size = scene->priv->s_prims->size();

	/* deselect meshes and clear 'sel-vector' */
	if(size)
	{
		DESELECT_PRIMITIVES(scene);

		/* emit signal */
		g_signal_emit(scene, signals[PRIM_SEL_CHANGED], 0);
	}

	return size;
}


gboolean
gtk_rtg_scene_deselect(GtkRtgScene *scene, GtkRtgPrimitive *prim) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_SCENE(scene));
	g_throw_if_fail(GTK_RTG_IS_PRIMITIVE(prim));

	if(scene->priv->s_prims->empty())
		return FALSE;

	gboolean ret = scene->priv->s_prims->erase(prim) > 0;

	SELECT_PRIMITIVE(scene, prim, FALSE);

	/* emit signal */
	g_signal_emit(scene, signals[PRIM_SEL_CHANGED], 0);

	return ret;
}


void
gtk_rtg_scene_toggle_edit_mode(GtkRtgScene *scene, GtkRtgPrimEditMode mode) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_SCENE(scene));

	/* if only one primitive is selected */
	if(SCENE_EXCLUSIVE(scene))
	{
		gtk_rtg_primitive_set_edit_mode((*scene->priv->s_prims->begin()), mode);
		g_object_notify(G_OBJECT(scene), prop[S_PROP_TOGGLE_EDIT_MODE]);
		g_signal_emit(scene, signals[CONFIGURED], 0);
	}
}


void
gtk_rtg_scene_select_backface(GtkRtgScene *scene, gboolean backface) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_SCENE(scene));
	scene->priv->backface = backface;
}


gboolean
gtk_rtg_scene_is_backface_selection(GtkRtgScene *scene) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_SCENE(scene));
	return scene->priv->backface;
}


GtkRtgPrimitive *
gtk_rtg_scene_get_primitive(GtkRtgScene *scene, guint color_id) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_SCENE(scene));

	/* query from mesh map */
	m_PrimitiveType::iterator iter = scene->priv->m_prims->find(color_id);

	if(iter != scene->priv->m_prims->end())
		return iter->second;

	return NULL;
}

/*
 * Privs.
 *
 *
 */


/***
 *** When camera is 'prioritized' the signal 'camera-set' is
 *** emitted.
 ***
 ***/
static void
gtk_rtg_scene_camera_prioritize(GtkRtgCamera *camera, GtkRtgCanvas *canvas, gpointer data)
{
	GtkRtgScene  *scene = GTK_RTG_SCENE(data);
	scene->priv->cam = camera;
	g_signal_emit(scene, signals[CAMERA_DEFAULT], 0, camera, canvas);
}


static void
gtk_rtg_scene_camera_view(GtkRtgCamera *camera, GParamSpec *pspec, GtkRtgScene *scene)
{
	FRUSTUM_CULL(scene, camera);
}


static void
gtk_rtg_scene_camera_projection(GtkRtgCamera *camera, GParamSpec *pspec, GtkRtgScene *scene)
{
	FRUSTUM_CULL(scene, camera);
}

/*
static void
gtk_rtg_scene_camera_proj_aspect_ratio(GtkRtgCamera *camera, GParamSpec *pspec, GtkRtgScene *scene)
{
	FRUSTUM_CULL(scene, camera);
}
*/

inline static guint
_gtk_rtg_scene_primitive_is_in_frustum(GtkRtgPrimitive *prim, GtkRtgCamFrustum *frustum)
{
	if(R_AXIS(prim) < SPHERES_AXIS_RATIO) /* bounding box */
	{
		GtkRtgPrimitiveBBox *bbox = BBOX(prim);

		guint total_in = 0;

		/* test all 8 corners against the 6 sides
		   if all points are behind 1 specific plane, we are out
		   if we are in with all points, then we are fully in */
		for(guint j=0; j<FRUSTUM_PLANES; ++j)
		{
			guint in   = BBOX_CENTER;
			guint side = 1;

			/* bounding box points */
			for(guint i=0; i<BBOX_CENTER; ++i)
			{
				/* translate bbox's point into world space */
				point3f p = object2world(prim, bbox->p[i]);

				/*
				 * calculate distance from point 'p' to the plane
				 */

				/* test this point against the planes */
				gfloat distance = frustum->p[j].normal.dot(p) - frustum->p[j].d;
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
	}else
	/* bounding sphere */
	{
		GtkRtgPrimitiveBSphere *bsphere = BSPHERE(prim);

		/* translate bsphere's point into world space */
		point3f sphere_center = object2world(prim, bsphere->center);

		/* test point against plane normals */
		guint j=0;
		for(; j<FRUSTUM_PLANES; ++j)
		{
			/*
			 * calculate distance from point 'p' to the plane
			 */

			gfloat distance = sphere_center.dot(frustum->p[j].normal) - frustum->p[j].d;

			/* if this distance is < -sphere.radius, we are outside */
			if(distance < -bsphere->r)
				return TEST_OUT;   /* out */

			/* else if the distance is between +- radius, then we intersect */
/*          if(fabsf(distance) <= BSPHERE(prim)->r)
            	return TEST_INTERSECT;
*/
		}

		return TEST_INSIDE; /* fully inside or intersect */
	}
}


inline static void
_gtk_rtg_scene_do_frustum_culling(GtkRtgScene *scene, GtkRtgCamera *camera)
{
	g_throw_if_fail(GTK_RTG_IS_SCENE(scene));
	g_throw_if_fail(GTK_RTG_IS_CAMERA(camera));

	if(IS_CAM_INIT(camera))
		gtk_rtg_camera_do_frustum(camera);

	m_CameraType::iterator iter = scene->priv->m_cams->find(GTK_RTG_PRIMITIVE(camera));

	GtkRtgCamFrustum *frustum = FRUSTUM(camera);

	if(iter != scene->priv->m_cams->end())
	{
		if(!iter->second->empty())
			iter->second->clear();

		for(m_PrimitiveType::const_iterator it = scene->priv->m_prims->begin(); it != scene->priv->m_prims->end(); ++it)
		{
			if(GTK_RTG_IS_CAMERA(it->second))
				continue;

			if(_gtk_rtg_scene_primitive_is_in_frustum(it->second, frustum))
				iter->second->insert(it->second);
		}
	}
}


static void
_gtk_rtg_primitve_transformed(GtkRtgPrimitive *prim, gpointer data)
{
	GtkRtgScene *scene = GTK_RTG_SCENE(data);

	/*
	 * Emit configured
	 *
	 */
	g_signal_emit(scene, signals[CONFIGURED], 0);
}
