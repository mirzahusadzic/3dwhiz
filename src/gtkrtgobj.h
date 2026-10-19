#ifndef GTK_RTG_OBJ_H
#define GTK_RTG_OBJ_H

#include <gtkrtg/gtkrtgprimitive.h>

namespace gtkrtg
{
    struct ObjMesh
    {
        std::vector<point3f> vertices;
        std::vector<vector3f> normals;
        std::vector<point2f> texcoords;
        std::vector<guint_ind> indices;
        std::vector<guint_ind> texindices;
        std::set<guint_ind2> edges;
    };

    ObjMesh read_wavefront_obj(const char *path);
} // namespace gtkrtg

#endif
