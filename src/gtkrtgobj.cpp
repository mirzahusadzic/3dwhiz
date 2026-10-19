#include "gtkrtgobj.h"
#include <cmath>
#include <fstream>
#include <limits>
#include <locale>
#include <map>
#include <sstream>

using namespace gtkrtg;

namespace
{
    const size_t missing = std::numeric_limits<size_t>::max();
    const guint_ind missing_texture = std::numeric_limits<guint_ind>::max();

    void
    require(bool condition, size_t line, const char *message)
    {
        if(!condition)
        {
            std::string detail = "OBJ line " + std::to_string(line) + ": " + message;
            throw GtkRtgException(detail.c_str(), __FILE__, __LINE__);
        }
    }

    size_t
    index_of(const std::string &text, size_t count, size_t line)
    {
        std::istringstream input(text);
        input.imbue(std::locale::classic());
        long long index = 0;
        require(bool(input >> index) && input.peek() == EOF && index != 0, line, "invalid face index");
        require(index >= -static_cast<long long>(count) && index <= static_cast<long long>(count), line,
                "face index out of range");
        return index < 0 ? count - static_cast<size_t>(-index) : static_cast<size_t>(index - 1);
    }

    struct Corner
    {
        size_t vertex = missing;
        size_t texture = missing;
        size_t normal = missing;
    };

    Corner
    read_corner(const std::string &text, size_t vertices, size_t textures, size_t normals, size_t line)
    {
        Corner corner;
        size_t first = text.find('/');
        corner.vertex = index_of(text.substr(0, first), vertices, line);
        if(first == std::string::npos)
            return corner;
        size_t second = text.find('/', first + 1);
        std::string texture =
            text.substr(first + 1, second == std::string::npos ? second : second - first - 1);
        if(!texture.empty())
            corner.texture = index_of(texture, textures, line);
        if(second != std::string::npos)
            corner.normal = index_of(text.substr(second + 1), normals, line);
        else
            require(!texture.empty(), line, "missing texture index after slash");
        return corner;
    }
} // namespace

ObjMesh
gtkrtg::read_wavefront_obj(const char *path)
{
    std::ifstream input(path);
    require(input.is_open(), 0, "cannot open file");
    std::vector<point3f> positions;
    std::vector<vector3f> normals;
    ObjMesh mesh;
    std::map<std::pair<size_t, size_t>, guint_ind> remapped;
    bool has_normals = false, has_textures = false;
    std::string line;
    size_t line_number = 0;
    while(std::getline(input, line))
    {
        ++line_number;
        size_t comment = line.find('#');
        if(comment != std::string::npos)
            line.erase(comment);
        std::istringstream row(line);
        row.imbue(std::locale::classic());
        std::string kind;
        if(!(row >> kind))
            continue;
        if(kind == "v" || kind == "vn")
        {
            float x, y, z;
            require(bool(row >> x >> y >> z) && std::isfinite(x) && std::isfinite(y) && std::isfinite(z),
                    line_number, "invalid 3D coordinate");
            if(kind == "v")
            {
                row >> std::ws;
                if(!row.eof())
                {
                    float w;
                    require(bool(row >> w) && std::isfinite(w) && w != 0, line_number,
                            "invalid homogeneous coordinate");
                    x /= w;
                    y /= w;
                    z /= w;
                }
                require(std::isfinite(x) && std::isfinite(y) && std::isfinite(z), line_number,
                        "coordinate overflow");
                positions.push_back(point3f(x, y, z));
            }
            else
                normals.push_back(vector3f(x, y, z));
        }
        else if(kind == "vt")
        {
            float u, v = 0;
            require(bool(row >> u) && std::isfinite(u), line_number, "invalid texture coordinate");
            row >> std::ws;
            if(!row.eof())
                require(bool(row >> v) && std::isfinite(v), line_number, "invalid texture coordinate");
            require(mesh.texcoords.size() < missing_texture, line_number, "too many texture coordinates");
            mesh.texcoords.push_back(point2f(u, v));
        }
        else if(kind == "f")
        {
            std::vector<Corner> face;
            std::string token;
            while(row >> token)
                face.push_back(
                    read_corner(token, positions.size(), mesh.texcoords.size(), normals.size(), line_number));
            require(face.size() >= 3, line_number, "face needs at least three corners");

            /* Fan triangulation supports convex polygons; reject concave input. */
            if(face.size() > 3)
            {
                vector3f reference(0.f, 0.f, 0.f);
                for(size_t i = 0; i < face.size(); ++i)
                {
                    const point3f &a = positions[face[i].vertex];
                    const point3f &b = positions[face[(i + 1) % face.size()].vertex];
                    const point3f &c = positions[face[(i + 2) % face.size()].vertex];
                    vector3f turn = (b - a).cross(c - b);
                    if(reference.mag() == 0.f)
                        reference = turn;
                    require(reference.dot(turn) >= 0.f, line_number,
                            "concave polygon: triangulate before importing");
                }
            }

            std::vector<guint_ind> indices;
            for(const auto &corner : face)
            {
                auto key = std::make_pair(corner.vertex, corner.normal);
                auto found = remapped.find(key);
                if(found == remapped.end())
                {
                    require(mesh.vertices.size() < std::numeric_limits<guint_ind>::max(), line_number,
                            "mesh exceeds the configured vertex index limit");
                    guint_ind index = static_cast<guint_ind>(mesh.vertices.size());
                    found = remapped.emplace(key, index).first;
                    mesh.vertices.push_back(positions[corner.vertex]);
                    mesh.normals.push_back(corner.normal == missing ? vector3f(0.f, 0.f, 0.f)
                                                                    : normals[corner.normal]);
                }
                indices.push_back(found->second);
                has_normals |= corner.normal != missing;
                has_textures |= corner.texture != missing;
            }
            for(size_t i = 1; i + 1 < face.size(); ++i)
            {
                for(size_t j : {size_t(0), i, i + 1})
                {
                    mesh.indices.push_back(indices[j]);
                    mesh.texindices.push_back(face[j].texture == missing
                                                  ? missing_texture
                                                  : static_cast<guint_ind>(face[j].texture));
                }
                for(auto edge :
                    {std::make_pair(indices[0], indices[i]), std::make_pair(indices[i], indices[i + 1]),
                     std::make_pair(indices[i + 1], indices[0])})
                {
                    guint_ind2 a = edge.first, b = edge.second;
                    mesh.edges.insert((std::max(a, b) << IND_BITS) | std::min(a, b));
                }
            }
        }
        /* Groups, materials and smoothing groups are not interpreted. */
    }
    require(!input.bad(), line_number, "read error");
    require(!mesh.indices.empty(), line_number, "no polygon faces found");
    if(!has_normals)
        mesh.normals.clear();
    if(!has_textures)
        mesh.texindices.clear();
    return mesh;
}
