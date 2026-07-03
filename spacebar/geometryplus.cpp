#include "geometryplus.hpp"

#include "tiny_obj_loader.h"
#include "glm/vec3.hpp"
#include <iostream>

Geometry GeometryPlus::from_file(std::filesystem::path path, bool normalize, bool centralize) {
    const std::string extension = path.extension().generic_string();

    if (extension == ".obj") {
        tinyobj::ObjReader reader;

        if (!reader.ParseFromFile(path.generic_string())) {
            if (!reader.Error().empty()) {
                std::cerr << "TinyObjReader: " << reader.Error();
            }
        }

        if (!reader.Warning().empty()) {
            std::cout << "TinyObjReader: " << reader.Warning();
        }

        auto& attrib = reader.GetAttrib();
        auto& shapes = reader.GetShapes();
        auto& materials = reader.GetMaterials();

        // Take only the first shape found
        const tinyobj::shape_t& shape = shapes[0];

        std::vector<float> positions;
        std::vector<float> normals;
        std::vector<float> tex_coords;

        glm::vec3 min{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
        glm::vec3 max{std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()};

        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            // Loop over vertices in the face.
            for (size_t v = 0; v < 3; v++) {
                // Access to vertex
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

                tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];

                tinyobj::real_t nx;
                tinyobj::real_t ny;
                tinyobj::real_t nz;
                if (!attrib.normals.empty()) {
                    nx = attrib.normals[3 * idx.normal_index + 0];
                    ny = attrib.normals[3 * idx.normal_index + 1];
                    nz = attrib.normals[3 * idx.normal_index + 2];
                } else {
                    nx = 0.0;
                    ny = 0.0;
                    nz = 0.0;
                }

                tinyobj::real_t tx;
                tinyobj::real_t ty;
                if (!attrib.texcoords.empty()) {
                    tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                    ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                } else {
                    tx = 0.0;
                    ty = 0.0;
                }

                min.x = std::min(min.x, vx);
                min.y = std::min(min.y, vy);
                min.z = std::min(min.z, vz);
                max.x = std::max(max.x, vx);
                max.y = std::max(max.y, vy);
                max.z = std::max(max.z, vz);

                positions.insert(positions.end(), {vx, vy, vz});
                normals.insert(normals.end(), {nx, ny, nz});
                tex_coords.insert(tex_coords.end(), {tx, ty});
            }
            index_offset += 3;
        }

            glm::vec3 diff = max - min;
            glm::vec3 center = min + 0.5f * diff;
            min -= center;
            max -= center;
            if (normalize || centralize)
            {
                for (int i = 0; i < positions.size(); i += 3) {
                    if (centralize)
                    {
                        positions[i + 0] -= center.x;
                        positions[i + 1] -= center.y;
                        positions[i + 2] -= center.z;
                    }
                    if (normalize)
                    {
                        positions[i + 0] /= std::max(std::max(diff.x, diff.y), diff.z);
                        positions[i + 1] /= std::max(std::max(diff.x, diff.y), diff.z);
                        positions[i + 2] /= std::max(std::max(diff.x, diff.y), diff.z);
                    }
                }
            }

        const int elements_per_vertex = 3 + (!normals.empty() ? 3 : 0) + (!tex_coords.empty() ? 2 : 0);

        return Geometry{GL_TRIANGLES, positions, {/*indices*/}, normals, {/*colors*/}, tex_coords};
    }
    std::cerr << "Extension " << extension << " not supported" << std::endl;

    return Geometry{};
}