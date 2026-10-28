#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#include <StaticMesh.h>
#include <Material.h>

#include <memory>

#include <glm/matrix.hpp>

namespace OM3D {

class SceneObject {

    public:
        SceneObject(std::shared_ptr<StaticMesh> mesh = nullptr, std::shared_ptr<Material> material = nullptr);

        void render(bool depth_only = false) const;

        const Material& material() const;

        void set_transform(const glm::mat4& tr);
        const glm::mat4& transform() const;

        bool is_visible(const Camera& camera, const Frustum& frustum) const;

    private:
        glm::mat4 _transform = glm::mat4(1.0f);

        std::shared_ptr<StaticMesh> _mesh;
        std::shared_ptr<Material> _material;
};

}

#endif // SCENEOBJECT_H
