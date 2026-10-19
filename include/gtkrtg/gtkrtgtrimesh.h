#ifndef GTK_RTG_TRI_MESH_H
#define GTK_RTG_TRI_MESH_H

#include <gtkrtg/gtkrtgbase.h>
#include <gtkrtg/gtkrtgexception.h>
#include <gtkrtg/gtkrtgmath.h>
#include <gtkrtg/gtkrtgprimitive.h>

/*! \file */

G_BEGIN_DECLS

#define GTK_RTG_TYPE_TRI_MESH            (gtk_rtg_tri_mesh_get_type())
#define GTK_RTG_TRI_MESH(obj)		     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_RTG_TYPE_TRI_MESH, GtkRtgTriMesh))
#define GTK_RTG_TRI_MESH_CLASS(klass)	 (G_TYPE_CHECK_CLASS_CAST ((klass),  GTK_RTG_TYPE_TRI_MESH, GtkRtgTriMeshClass))
#define GTK_RTG_IS_TRI_MESH(obj)		 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_RTG_TYPE_TRI_MESH))
#define GTK_RTG_IS_TRI_MESH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GTK_RTG_TYPE_TRI_MESH))
#define GTK_RTG_TRI_MESH_GET_CLASS(obj)	 (G_TYPE_INSTANCE_GET_CLASS ((obj),  GTK_RTG_TYPE_TRI_MESH, GtkRtgTriMeshClass))

typedef struct _GtkRtgTriMesh		 GtkRtgTriMesh;
typedef struct _GtkRtgTriMeshClass	 GtkRtgTriMeshClass;
typedef struct _GtkRtgTriMeshPrivate GtkRtgTriMeshPrivate;

struct _GtkRtgTriMesh
{
	GtkRtgPrimitive		  parent;
	GtkRtgTriMeshPrivate *priv;
};

struct _GtkRtgTriMeshClass
{
	GtkRtgPrimitiveClass parent_class;
};

/*! \defgroup GtkRtgTriMesh GtkRtgTriMesh
    \c GtkRtgTriMesh is derived from the \ref GtkRtgPrimitive. \n \n
    The \c GtkRtgTriMesh is an object which holds a triangle mesh. The triangle mesh is a shape which is consisted only of triangles. The mesh can be
    loaded/saved from/as 'Alias WaveFront obj' format. The 'obj' is a simple textual and human readable mesh format which is supported by all modern
    3d modeling software.
*/

/* instantiate */
GType gtk_rtg_tri_mesh_get_type(void) G_GNUC_CONST;

/*!
    \ingroup GtkRtgTriMesh
    \brief Creates a new \c GtkRtgTriMesh.
    \return - \c A GtkRtgPrimitive.
*/
GtkRtgPrimitive	*gtk_rtg_tri_mesh_new();

/*!
    \ingroup GtkRtgTriMesh
    \brief Clones a \c GtkRtgTriMesh.
    \param trimesh - A mesh to clone.
    \return - A cloned mesh.
    \param instance - if TRUE then mesh will be cloned by instance, or FALSE to be copied.
    \attention - In case \c trimesh fails on integrity check a \ref GtkRtgException will be thrown.
*/
GtkRtgPrimitive *gtk_rtg_tri_mesh_clone(GtkRtgTriMesh *trimesh, gboolean instance=TRUE) throw (GtkRtgException);

/*!
    \ingroup GtkRtgTriMesh
    \brief Loads a mesh from 'Alias WaveFront obj'. \n
    The \c trimesh which is cloned by instance cannot be changed by this call (see \ref gtk_rtg_primitive_get_clone_instance). \n
    Additionally, the mesh edges will be constructed.
    \param trimesh - A \c GtkRtgTriMesh.
    \param file - An 'obj' file to load.
    \attention - In case \c trimesh fails on integrity check, or \c file is \c NULL, or trimesh is cloned by instance a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_tri_mesh_load_from_wavefront_obj(GtkRtgTriMesh *trimesh, const gchar *file) throw (GtkRtgException);

/*!
    \ingroup GtkRtgTriMesh
    \brief Saves a mesh as 'Alias WaveFront obj' file.
    \param trimesh - A \c GtkRtgTriMesh.
    \param file - An 'obj' file to save a mesh.
    \attention - In case \c trimesh fails on integrity check, or \c file is \c NULL a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_tri_mesh_save_as_wavefront_obj(GtkRtgTriMesh *trimesh, const gchar *file) throw (GtkRtgException);

/*!
    \ingroup GtkRtgTriMesh
    \brief Calculates a new vertex normals. \n
    The face normals are mandatory for calculation of vertex normals and will be recalculated. \n
    The default \c crease_angle is 45.0 degrees. \n
    The crease angle is used in averaging process to eject face normals where angle between face and calculated vertex normal exceeds \c crease_angle. This means that
    at lower angles i.e. up to 50 degrees an average mesh will look more solid, and will have hard edges, where higher angles produces smoother edges, or more organic look.
    \param trimesh - A \c GtkRtgTriMesh.
    \param crease_angle - An angle in degrees.
    \attention - In case \c trimesh fails on integrity check a \ref GtkRtgException will be thrown.
*/
void  gtk_rtg_tri_mesh_calc_vertex_normals(GtkRtgTriMesh *trimesh, gfloat crease_angle = 45.f) throw (GtkRtgException);

/*!
    \ingroup GtkRtgTriMesh
    \brief Calculates a face normals.
    \param trimesh - A \c GtkRtgTriMesh.
    \attention - In case \c trimesh fails on integrity check a \ref GtkRtgException will be thrown.
*/
 void gtk_rtg_tri_mesh_calc_face_normals(GtkRtgTriMesh *trimesh) throw (GtkRtgException);

/*!
    \ingroup GtkRtgTriMesh
    \brief Returns a face normals.
    \param trimesh - A \c GtkRtgTriMesh.
    \attention - In case \c trimesh fails on integrity check a \ref GtkRtgException will be thrown.
*/
 std::vector<gtkrtg::vector3f> *gtk_rtg_tri_mesh_get_face_normals(GtkRtgTriMesh *trimesh) throw (GtkRtgException);

/*!
    \ingroup GtkRtgTriMesh
    \brief Sets up an array of triangle strip indices.
    \param trimesh - A \c GtkRtgTriMesh.
    \param sindices - A \c std::vector<guint_ind> array to set.
    \attention - In case \c trimesh fails on integrity check a \ref GtkRtgException will be thrown.
*/
void gtk_rtg_tri_mesh_set_sindices(GtkRtgTriMesh *trimesh, std::vector<guint_ind> *sindices) throw (GtkRtgException);

/*!
    \ingroup GtkRtgTriMesh
    \brief Returns an array of triangle strip indices. \n
    The array is owned by the library.
    \param trimesh - A \c GtkRtgTriMesh.
    \return - A \c std::vector<guint_ind>.
    \attention - In case \c trimesh fails on integrity check a \ref GtkRtgException will be thrown.
*/
std::vector<guint_ind>	*gtk_rtg_tri_mesh_get_sindices(GtkRtgTriMesh *trimesh) throw (GtkRtgException);

/* handy macros */
#define FNORMALS(trimesh)				(gtk_rtg_tri_mesh_get_face_normals(GTK_RTG_TRI_MESH(trimesh)));
#define CALC_VNORMALS(trimesh, crease)	(gtk_rtg_tri_mesh_calc_vertex_normals(GTK_RTG_TRI_MESH(trimesh), crease))
#define SINDICES(trimesh)               (gtk_rtg_tri_mesh_get_sindices(GTK_RTG_TRI_MESH(trimesh)))

G_END_DECLS

#endif /* GTK_RTG_TRI_MESH_H */
