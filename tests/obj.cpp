#include <gtkrtg/gtkrtg.h>
#include "../src/gtkrtgutils.h"
#include "../src/gtkrtgglu.h"
#include <cmath>
#include <fstream>
#include <limits>
#include <string>
#include <unistd.h>

using namespace gtkrtg;

struct Fixture
{
    GtkRtgTriMesh *mesh = GTK_RTG_TRI_MESH(gtk_rtg_tri_mesh_new());
    gchar *path = nullptr;

    Fixture()
    {
        int fd = g_file_open_tmp("3dwhiz-obj-XXXXXX", &path, nullptr);
        g_assert_cmpint(fd, >=, 0);
        close(fd);
    }

    ~Fixture()
    {
        g_object_unref(mesh);
        unlink(path);
        g_free(path);
    }

    void
    load(const std::string &data)
    {
        std::ofstream file(path);
        file << data;
        file.close();
        gtk_rtg_tri_mesh_load_from_wavefront_obj(mesh, path);
    }
};

static const char *triangle = "v 0 0 0\nv 1 0 0\nv 0 1 0\n";

static void
plain_and_roundtrip()
{
    Fixture fixture;
    fixture.load(std::string("# ") + std::string(600, 'x') + "\n\tv\t0 0 0\n v  1 0 0\nv 0 1 0\nf 1 2 3");
    g_assert_cmpuint(VERTS(fixture.mesh)->size(), ==, 3);
    g_assert_cmpuint(VINDICES(fixture.mesh)->size(), ==, 3);
    g_assert_true(TINDICES(fixture.mesh)->empty());
    g_assert_true(VNORMALS(fixture.mesh)->empty());
    gtk_rtg_tri_mesh_save_as_wavefront_obj(fixture.mesh, fixture.path);
    gtk_rtg_tri_mesh_load_from_wavefront_obj(fixture.mesh, fixture.path);
    g_assert_cmpuint(EDGES(fixture.mesh)->size(), ==, 3);
}

static void
negative_quad()
{
    Fixture fixture;
    fixture.load("v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\n"
                 "vt 0\nvt 1 0\nvt 1 1\nvt 0 1\n"
                 "f -4/-4 -3/-3 -2/-2 -1/-1 # a quad\n");
    g_assert_cmpuint(VINDICES(fixture.mesh)->size(), ==, 6);
    g_assert_cmpuint(TINDICES(fixture.mesh)->size(), ==, 6);
    gtk_rtg_tri_mesh_save_as_wavefront_obj(fixture.mesh, fixture.path);
    gtk_rtg_tri_mesh_load_from_wavefront_obj(fixture.mesh, fixture.path);
    g_assert_cmpuint(TEXCOORDS(fixture.mesh)->size(), ==, 4);
}

static void
independent_normals()
{
    Fixture fixture;
    fixture.load(std::string(triangle) + "vn 0 0 -1\nvn 0 0 1\nvt 0 0\n"
                                         "f 1//2 2//2 3//2\nf 3/1/1 2/1/1 1/1/1\nf 1 2 3\n");
    g_assert_cmpuint(VERTS(fixture.mesh)->size(), ==, 9);
    g_assert_cmpfloat(Z((*VNORMALS(fixture.mesh))[0]), ==, 1.f);
    g_assert_cmpfloat(Z((*VNORMALS(fixture.mesh))[3]), ==, -1.f);
    g_assert_cmpfloat((*VNORMALS(fixture.mesh))[6].mag(), ==, 0.f);
    g_assert_cmpuint((*TINDICES(fixture.mesh))[0], ==, std::numeric_limits<guint_ind>::max());
    gtk_rtg_tri_mesh_save_as_wavefront_obj(fixture.mesh, fixture.path);
    gtk_rtg_tri_mesh_load_from_wavefront_obj(fixture.mesh, fixture.path);
    g_assert_cmpfloat(Z((*VNORMALS(fixture.mesh))[3]), ==, -1.f);
}

static void
invalid_preserves_mesh()
{
    Fixture fixture;
    fixture.load(std::string(triangle) + "f 1 2 3\n");
    for(const auto &bad :
        {"f 0 2 3\n", "f 1 2 4\n", "f -4 -2 -1\n", "f 1 2\n", "f 1/ 2/ 3/\n", "f 1//0 2//0 3//0\n",
         "f 1///2 2 3\n", "f 99999999999999999999999 2 3\n", "v inf 0 0\nf 1 2 3\n", "v 1 0\nf 1 2 3\n", ""})
    {
        bool rejected = false;
        try
        {
            fixture.load(std::string(triangle) + bad);
        }
        catch(const GtkRtgException &error)
        {
            rejected = true;
            g_assert_nonnull(strstr(error.what(), "OBJ line"));
        }
        g_assert_true(rejected);
        g_assert_cmpuint(VINDICES(fixture.mesh)->size(), ==, 3);
        g_assert_cmpfloat(X((*VERTS(fixture.mesh))[1]), ==, 1.f);
    }
}

static void
many_faces_and_unused_vertex()
{
    Fixture fixture;
    std::string data = triangle;
    for(int i = 0; i < 30000; ++i)
        data += "f 1 2 3\n";
    fixture.load(data);
    VERTS(fixture.mesh)->push_back(point3f(9.f, 9.f, 9.f));
    gtk_rtg_tri_mesh_calc_vertex_normals(fixture.mesh, 180.f);
    g_assert_cmpuint(VINDICES(fixture.mesh)->size(), ==, 90000);
    g_assert_cmpfloat_with_epsilon(Z((*VNORMALS(fixture.mesh))[0]), 1.f, .0001f);
    g_assert_cmpfloat((*VNORMALS(fixture.mesh))[3].mag(), ==, 0.f);
}

extern point3f object2world(GtkRtgPrimitive *, point3f &);
extern point3f world2object(GtkRtgPrimitive *, point3f &);

static void
transform_roundtrip()
{
    Fixture fixture;
    fixture.load(std::string(triangle) + "v 0 0 1\nf 1 2 3\nf 1 2 4\n");
    auto *mesh = GTK_RTG_PRIMITIVE(fixture.mesh);
    vector3f scale(2.f, 3.f, 4.f), translation(3.f, 7.f, -2.f), axis(0.f, 0.f, 1.f);
    gtk_rtg_primitive_rotate_from_axis_angle(mesh, axis, 35.f);
    gtk_rtg_primitive_scale(mesh, scale);
    gtk_rtg_primitive_translate(mesh, translation);
    point3f local(.7f, -.5f, 1.3f);
    point3f expected = gtk_rtg_primitive_get_transformation_matrix(mesh) * local;
    X(expected) *= X(scale);
    Y(expected) *= Y(scale);
    Z(expected) *= Z(scale);
    expected += translation;
    point3f world = object2world(mesh, local);
    g_assert_cmpfloat((expected - world).mag(), <, .00001f);
    g_assert_cmpfloat((world2object(mesh, world) - local).mag(), <, .00001f);
}

static void
bitfield_growth()
{
    bitfield bits;
    bits[31] = 1;
    bits[65535] = 1;
    bits[100000] = 1;
    g_assert_cmpint(int(bits[31]), ==, 1);
    g_assert_cmpint(int(bits[65535]), ==, 1);
    g_assert_cmpint(int(bits[65536]), ==, 0);
    g_assert_cmpint(int(bits[100000]), ==, 1);
    g_assert_true(bits.resize(33));
    g_assert_cmpint(int(bits[31]), ==, 1);
    g_assert_true(bits.resize(70001));
    g_assert_cmpint(int(bits[70000]), ==, 0);
    bits.clear();
    g_assert_cmpint(int(bits[31]), ==, 0);
}

static void
rotation_roundtrip()
{
    for(auto axis : {vector3f(1.f, 0.f, 0.f), vector3f(0.f, 1.f, 0.f),
                     vector3f(0.f, 0.f, 1.f), vector3f(1.f, .3f, -.2f)})
        for(float angle : {0.f, 37.f, 120.f, 180.f, -150.f})
        {
            axis.norm();
            quat_f rotation;
            rotation.rotate_axis(axis, deg2rad(angle));
            matrix4x4f m4;
            matrix3x3f m3;
            m4.from_quaternion(rotation);
            m3.from_inertial2object(rotation);
            const auto q4 = m4.to_quaternion();
            const auto q3 = m3.to_quaternion();
            g_assert_cmpfloat_with_epsilon(q4.dot(q4), 1.f, .00001f);
            g_assert_cmpfloat_with_epsilon(q3.dot(q3), 1.f, .00001f);
            g_assert_cmpfloat_with_epsilon(fabs(q4.dot(rotation)), 1.f, .00001f);
            g_assert_cmpfloat_with_epsilon(fabs(q3.dot(rotation)), 1.f, .00001f);
        }
}

static void
axial_camera_inverse()
{
    const GLfloat view[16] = {1, 0, 0, 0, 0, 1.e-21f, 1, 0,
                              0, -1, 1.e-21f, 0, -2, 3, -20, 1};
    GLfloat inverse[16], identity[16];
    g_assert_true(__gluInvertMatrixf(view, inverse));
    __gluMultMatricesf(view, inverse, identity);
    for(int i = 0; i < 16; ++i)
        g_assert_cmpfloat_with_epsilon(identity[i], i % 5 == 0 ? 1.f : 0.f, .0001f);
    g_assert_true(__gluInvertMatrixf(inverse, inverse));
    for(int i = 0; i < 16; ++i)
        g_assert_cmpfloat_with_epsilon(inverse[i], view[i], .0001f);

    const GLfloat singular[16] = {};
    g_assert_false(__gluInvertMatrixf(singular, inverse));
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, nullptr);
    g_test_add_func("/obj/plain-roundtrip", plain_and_roundtrip);
    g_test_add_func("/obj/negative-quad", negative_quad);
    g_test_add_func("/obj/independent-normals", independent_normals);
    g_test_add_func("/obj/invalid-preserves-mesh", invalid_preserves_mesh);
    g_test_add_func("/mesh/many-faces-unused-vertex", many_faces_and_unused_vertex);
    g_test_add_func("/mesh/transform-roundtrip", transform_roundtrip);
    g_test_add_func("/mesh/bitfield-growth", bitfield_growth);
    g_test_add_func("/math/axial-camera-inverse", axial_camera_inverse);
    g_test_add_func("/math/rotation-roundtrip", rotation_roundtrip);
    return g_test_run();
}
