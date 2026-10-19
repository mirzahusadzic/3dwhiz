#ifdef _MSC_VER
#pragma warning(disable : 4786)
#pragma warning(disable : 4996)
#endif

#include <gtkrtg/gtkrtgtrimesh.h>
#include <gtkrtg/gtkrtgmath.h>
#include "gtkrtg_gl_specs.h"
#include "gtkrtgextras.h"
#include "gtkrtgobj.h"
#include <iomanip>
#include <limits>
#include <locale>

#include <vector>
#include <set>
#include <fstream>

using namespace std;
using namespace gtkrtg;


#define GTK_RTG_TRI_MESH_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_RTG_TYPE_TRI_MESH, GtkRtgTriMeshPrivate))

G_DEFINE_TYPE(GtkRtgTriMesh, gtk_rtg_tri_mesh, GTK_RTG_TYPE_PRIMITIVE)

struct _vfindices /* faces per vertex (by index) */
{
	std::vector<size_t> face;
};

struct _GtkRtgTriMeshPrivate
{
	vector<vector3f> *fnormals;	/* face normals */
	_vfindices *vfindices;	/* face indices by vertex */
    vector<guint_ind> *sindices;	/* vertex indices (triangle strip) */
};


static void	gtk_rtg_tri_mesh_finalize(GObject *object);
static void gtk_rtg_tri_mesh_render(GtkRtgPrimitive *prim, guint mode);
static void	_gtk_rtg_tri_mesh_build_list_faces_per_vertex(GtkRtgTriMesh *mesh);

static void
gtk_rtg_tri_mesh_class_init(GtkRtgTriMeshClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	GtkRtgPrimitiveClass *pclass = (GtkRtgPrimitiveClass*) klass;

    object_class->finalize = gtk_rtg_tri_mesh_finalize;
    pclass->render_event   = gtk_rtg_tri_mesh_render;

	g_type_class_add_private(G_OBJECT_CLASS(klass), sizeof(GtkRtgTriMeshPrivate));
}


GtkRtgPrimitive *
gtk_rtg_tri_mesh_new()
{
	return GTK_RTG_PRIMITIVE(g_object_new(GTK_RTG_TYPE_TRI_MESH, NULL));
}


GtkRtgPrimitive *
gtk_rtg_tri_mesh_clone(GtkRtgTriMesh *trimesh, gboolean instance) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_TRI_MESH(trimesh));

	GtkRtgTriMesh *mesh = GTK_RTG_TRI_MESH(g_object_new(GTK_RTG_TYPE_TRI_MESH, NULL));

	if(instance)
	{
		/*
		 * Instantiate (ptr) trimesh data
		 *
		 */

		/* free */
		delete	mesh->priv->fnormals;

		/* instantiate data */
		mesh->priv->fnormals = trimesh->priv->fnormals;
		mesh->priv->vfindices = trimesh->priv->vfindices;
	}else
	{
		/*
		 * Copy trimesh data
		 *
		 */

		if(trimesh->priv->vfindices)
		{
			guint_ind verts = (guint_ind)VERTS(trimesh)->size();
			mesh->priv->vfindices = new _vfindices[verts];

			for(guint i=0; i<verts; ++i)
			{
				mesh->priv->vfindices[i].face.resize(trimesh->priv->vfindices[i].face.size());

				copy(trimesh->priv->vfindices[i].face.begin(),
				     	trimesh->priv->vfindices[i].face.end(),
				     	mesh->priv->vfindices[i].face.begin());
			}
		}

		mesh->priv->fnormals->resize(trimesh->priv->fnormals->size());

		copy(trimesh->priv->fnormals->begin(), trimesh->priv->fnormals->end(), mesh->priv->fnormals->begin());
	}

	gtk_rtg_primitive_clone(GTK_RTG_PRIMITIVE(trimesh), GTK_RTG_PRIMITIVE(mesh), instance);

	return GTK_RTG_PRIMITIVE(mesh);
}


static void
gtk_rtg_tri_mesh_init(GtkRtgTriMesh *trimesh)
{
	trimesh->priv = GTK_RTG_TRI_MESH_GET_PRIVATE(trimesh);

	/* force heap allocation to invoke constuctors */
	trimesh->priv->fnormals = new vector<vector3f>;
    trimesh->priv->sindices = new vector<guint_ind>;
}


static void
gtk_rtg_tri_mesh_finalize(GObject *object)
{
	if(IS_NOT_CLONE(GTK_RTG_PRIMITIVE(object)))
	{
		GtkRtgTriMesh *trimesh = GTK_RTG_TRI_MESH(object);

		if(trimesh->priv->fnormals) { delete	trimesh->priv->fnormals;	trimesh->priv->fnormals  = NULL; }
		if(trimesh->priv->vfindices){ delete []	trimesh->priv->vfindices;	trimesh->priv->vfindices = NULL; }
        if(trimesh->priv->sindices) { delete    trimesh->priv->sindices; 	trimesh->priv->sindices  = NULL; }
    }

	G_OBJECT_CLASS(gtk_rtg_tri_mesh_parent_class)->finalize(object);
}

/*
 * Privs.
 *
 *
 */
static void
_gtk_rtg_tri_mesh_build_list_faces_per_vertex(GtkRtgTriMesh *trimesh)
{
    const size_t count = VERTS(trimesh)->size();
    const auto &indices = *VINDICES(trimesh);
    g_throw_if_fail(indices.size() % 3 == 0);
    delete [] trimesh->priv->vfindices;
    trimesh->priv->vfindices = new _vfindices[count];
    for(size_t i = 0; i < indices.size(); ++i)
    {
        g_throw_if_fail(indices[i] < count);
        trimesh->priv->vfindices[indices[i]].face.push_back(i / 3);
    }
}


static void
gtk_rtg_tri_mesh_render(GtkRtgPrimitive *prim, guint mode)
{
    if(mode != GTK_RTG_RENDER_SELECT && mode != GTK_RTG_RENDER_SOLID)
        return;

    if(mode == GTK_RTG_RENDER_SELECT)
    {
        guint id = COLOR_ID(prim);
        glColor3ubv(reinterpret_cast<guchar*>(&id));
    }

    const auto &vertices = *VERTS(prim);
    const auto &indices = *VINDICES(prim);
    const auto &normals = *VNORMALS(prim);
    const auto &texcoords = *TEXCOORDS(prim);
    const auto &texindices = *TINDICES(prim);

    glBegin(GL_TRIANGLES);
    for(size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        if(indices[i] >= vertices.size() || indices[i + 1] >= vertices.size() || indices[i + 2] >= vertices.size())
            continue;

        vector3f normal = (vertices[indices[i + 1]] - vertices[indices[i]]).cross(vertices[indices[i + 2]] - vertices[indices[i]]);
        if(normal.mag() > 0.f)
            normal.norm();

        for(size_t j = i; j < i + 3; ++j)
        {
            glNormal3fv(indices[j] < normals.size() && normals[indices[j]].mag() > 0.f
                           ? VALUES(normals[indices[j]]) : VALUES(normal));
            if(j < texindices.size() && texindices[j] < texcoords.size())
                glTexCoord2fv(VALUES(texcoords[texindices[j]]));
            glVertex3fv(VALUES(vertices[indices[j]]));
        }
    }
    glEnd();
}


/*
 * Publics
 *
 *
 */

void
gtk_rtg_tri_mesh_load_from_wavefront_obj(GtkRtgTriMesh *trimesh, const gchar *file) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_TRI_MESH(trimesh));
    g_throw_if_fail(file != NULL);
    g_throw_if_fail(IS_NOT_CLONE(GTK_RTG_PRIMITIVE(trimesh)));

    /* Parse and validate before replacing the current mesh. */
    ObjMesh mesh = read_wavefront_obj(file);
    VERTS(trimesh)->swap(mesh.vertices);
    VNORMALS(trimesh)->swap(mesh.normals);
    TEXCOORDS(trimesh)->swap(mesh.texcoords);
    VINDICES(trimesh)->swap(mesh.indices);
    TINDICES(trimesh)->swap(mesh.texindices);
    EDGES(trimesh)->swap(mesh.edges);
    trimesh->priv->fnormals->clear();
    trimesh->priv->sindices->clear();
    delete [] trimesh->priv->vfindices;
    trimesh->priv->vfindices = NULL;
    gtk_rtg_primitive_calc_bbox_bsphere(GTK_RTG_PRIMITIVE(trimesh));
    gtk_rtg_primitive_set_pivot(GTK_RTG_PRIMITIVE(trimesh), &BBOX(trimesh)->p[BBOX_CENTER]);
}


void
gtk_rtg_tri_mesh_save_as_wavefront_obj(GtkRtgTriMesh *trimesh, const gchar *file) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_TRI_MESH(trimesh));
    g_throw_if_fail(file != NULL);
    const auto &verts = *VERTS(trimesh);
    const auto &normals = *VNORMALS(trimesh);
    const auto &texcoords = *TEXCOORDS(trimesh);
    const auto &indices = *VINDICES(trimesh);
    const auto &texindices = *TINDICES(trimesh);
    g_throw_if_fail(indices.size() % 3 == 0);
    for(auto index : indices)
        g_throw_if_fail(index < verts.size());
    std::ofstream output(file);
    g_throw_if_fail(output.is_open());
    output.imbue(std::locale::classic());
    output << std::setprecision(std::numeric_limits<float>::max_digits10);
    for(const auto &v : verts)
        output << "v " << X(v) << " " << Y(v) << " " << Z(v) << "\n";
    for(const auto &t : texcoords)
        output << "vt " << X(t) << " " << Y(t) << "\n";
    for(const auto &n : normals)
        output << "vn " << X(n) << " " << Y(n) << " " << Z(n) << "\n";
    for(size_t i = 0; i < indices.size(); i += 3)
    {
        output << "f";
        for(size_t j = i; j < i + 3; ++j)
        {
            const bool texture = j < texindices.size() && texindices[j] < texcoords.size();
            const bool normal = indices[j] < normals.size() && normals[indices[j]].mag() > 0.f;
            output << " " << indices[j] + 1;
            if(texture || normal)
            {
                output << "/";
                if(texture)
                    output << texindices[j] + 1;
                if(normal)
                    output << "/" << indices[j] + 1;
            }
        }
        output << "\n";
    }
    output.close();
    g_throw_if_fail(!output.fail());
}


void
gtk_rtg_tri_mesh_calc_face_normals(GtkRtgTriMesh *trimesh) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_TRI_MESH(trimesh));

    const auto &vertices = *VERTS(trimesh);
    const auto &indices = *VINDICES(trimesh);
    g_throw_if_fail(indices.size() % 3 == 0);
    trimesh->priv->fnormals->clear();
    for(size_t i = 0; i < indices.size(); i += 3)
    {
        g_throw_if_fail(indices[i] < vertices.size() && indices[i + 1] < vertices.size() &&
                        indices[i + 2] < vertices.size());
        vector3f normal = (vertices[indices[i + 1]] - vertices[indices[i]]).cross(
                           vertices[indices[i + 2]] - vertices[indices[i]]);
        if(normal.mag() > 0.f)
            normal.norm();
        trimesh->priv->fnormals->push_back(normal);
    }
}


void
gtk_rtg_tri_mesh_calc_vertex_normals(GtkRtgTriMesh *trimesh, gfloat crease_angle) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_TRI_MESH(trimesh));
    _gtk_rtg_tri_mesh_build_list_faces_per_vertex(trimesh);
    gtk_rtg_tri_mesh_calc_face_normals(trimesh);
    auto &normals = *VNORMALS(trimesh);
    normals.clear();
    const float threshold = cosf(deg2rad(crease_angle));
    for(size_t i = 0; i < VERTS(trimesh)->size(); ++i)
    {
        vector3f normal(0.f, 0.f, 0.f);
        const auto &faces = trimesh->priv->vfindices[i].face;
        if(!faces.empty())
        {
            const vector3f reference = (*trimesh->priv->fnormals)[faces[0]];
            normal = reference;
            for(size_t j = 1; j < faces.size(); ++j)
            {
                const vector3f &candidate = (*trimesh->priv->fnormals)[faces[j]];
                if(reference.dot(candidate) >= threshold)
                    normal += candidate;
            }
            if(normal.mag() > 0.f)
                normal.norm();
        }
        normals.push_back(normal);
    }
}


vector<vector3f> *
gtk_rtg_tri_mesh_get_face_normals(GtkRtgTriMesh *trimesh) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_TRI_MESH(trimesh));
    return trimesh->priv->fnormals;
}


_vfindices *
gtk_rtg_tri_mesh_get_vfindices(GtkRtgTriMesh *trimesh)
{
    g_throw_if_fail(GTK_RTG_IS_TRI_MESH(trimesh));
    return trimesh->priv->vfindices;
}


void
gtk_rtg_tri_mesh_set_sindices(GtkRtgTriMesh *trimesh, vector<guint_ind> *sindices) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_TRI_MESH(trimesh));

	trimesh->priv->sindices->clear();
	trimesh->priv->sindices->resize(sindices->size());
	copy(sindices->begin(), sindices->end(), trimesh->priv->sindices->begin());
}


vector<guint_ind> *
gtk_rtg_tri_mesh_get_sindices(GtkRtgTriMesh *trimesh) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_TRI_MESH(trimesh));
    return trimesh->priv->sindices;
}
