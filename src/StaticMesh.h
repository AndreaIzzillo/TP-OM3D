#ifndef STATICMESH_H
#define STATICMESH_H

#include <graphics.h>
#include <Camera.h>
#include <TypedBuffer.h>
#include <Vertex.h>

#include <vector>

namespace OM3D {

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
};

struct BoundingSphere {
    glm::vec3 center;
    float radius;
};

class StaticMesh : NonCopyable {

    public:
        StaticMesh() = default;
        StaticMesh(StaticMesh&&) = default;
        StaticMesh& operator=(StaticMesh&&) = default;

        StaticMesh(const MeshData& data);

        const BoundingSphere& bounds() const;

        void draw() const;

    private:
        TypedBuffer<Vertex> _vertex_buffer;
        TypedBuffer<u32> _index_buffer;

        BoundingSphere _bounds;
};

}

#endif // STATICMESH_H
