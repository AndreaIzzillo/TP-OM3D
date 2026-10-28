#include "SceneObject.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace OM3D {

SceneObject::SceneObject(std::shared_ptr<StaticMesh> mesh, std::shared_ptr<Material> material) :
    _mesh(std::move(mesh)),
    _material(std::move(material)) {
}

void SceneObject::render(bool depth_only) const {
    if(!_material || !_mesh) {
        return;
    }

    _material->set_uniform(HASH("model"), transform());
    _material->bind(depth_only);
    _mesh->draw();
}

const Material& SceneObject::material() const {
    DEBUG_ASSERT(_material);
    return *_material;
}

void SceneObject::set_transform(const glm::mat4& tr) {
    _transform = tr;
}

const glm::mat4& SceneObject::transform() const {
    return _transform;
}

bool SceneObject::is_visible(const Camera& camera, const Frustum& frustum) const {
    if(!_mesh) {
        return false;
    }

    const auto& bounds = _mesh->bounds();
    const glm::vec3 world_center = glm::vec3(_transform * glm::vec4(bounds.center, 1.0f));

    const glm::mat3 linear_transform = glm::transpose(glm::mat3(_transform));

    const glm::vec3 normals[] = {
        frustum._near_normal,
        frustum._left_normal,
        frustum._right_normal,
        frustum._top_normal,
        frustum._bottom_normal
    };

    for (int i = 0; i < 5; i++) {
        float distance = glm::dot(normals[i], world_center - camera.position());

        if (i == 0) {
            distance -= camera.projection_matrix()[3][2];
        }

        const float radius = bounds.radius * glm::length(linear_transform * normals[i]);

        if (distance < -radius) {
            return false;
        }
    }

    return true;
}

BoundingSphere SceneObject::bounds() const {
    if(!_mesh) {
        return BoundingSphere();
    }
    
    const auto& local_bounds = _mesh->bounds();
    BoundingSphere world_bounds;
    world_bounds.center = glm::vec3(_transform * glm::vec4(local_bounds.center, 1.0f));
    const float scale_x = glm::length(glm::vec3(_transform[0]));
    const float scale_y = glm::length(glm::vec3(_transform[1]));
    const float scale_z = glm::length(glm::vec3(_transform[2]));
    world_bounds.radius = local_bounds.radius * std::max({scale_x, scale_y, scale_z});

    return world_bounds;
}

}
