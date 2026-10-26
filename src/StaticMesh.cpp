#include "StaticMesh.h"

#include <algorithm>
#include <glad/gl.h>
#include <glm/geometric.hpp>

namespace OM3D {

extern bool audit_bindings_before_draw;

StaticMesh::StaticMesh(const MeshData& data) :
    _vertex_buffer(data.vertices),
    _index_buffer(data.indices) {
    if (!data.vertices.empty()) {
        glm::vec3 min = data.vertices[0].position;
        glm::vec3 max = data.vertices[0].position;

        for (const auto& v : data.vertices) {
            min = glm::min(min, v.position);
            max = glm::max(max, v.position);
        }

        _bounds.center = (min + max) * 0.5f;
        _bounds.radius = glm::max(glm::length(max - _bounds.center), glm::length(min - _bounds.center));
    } else {
        _bounds.center = glm::vec3(0.0f);
        _bounds.radius = 0.0f;
    }
}

const BoundingSphere& StaticMesh::bounds() const {
    return _bounds;
}

void StaticMesh::draw() const {
    _vertex_buffer.bind(BufferUsage::Attribute);
    _index_buffer.bind(BufferUsage::Index);

    // Vertex position
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), nullptr);
    // Vertex normal
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(3 * sizeof(float)));
    // Vertex uv
    glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(6 * sizeof(float)));
    // Tangent / bitangent sign
    glVertexAttribPointer(3, 4, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(8 * sizeof(float)));
    // Vertex color
    glVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(12 * sizeof(float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);

    if(audit_bindings_before_draw) {
        audit_bindings();
    }

    glDrawElements(GL_TRIANGLES, int(_index_buffer.element_count()), GL_UNSIGNED_INT, nullptr);
}

}
