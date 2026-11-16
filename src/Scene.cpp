#include "Scene.h"

#include <TypedBuffer.h>

#include <shader_structs.h>

#include <algorithm>

namespace OM3D {

Scene::Scene() {
    _sky_material.set_program(Program::from_files("sky.frag", "screen.vert"));
    _sky_material.set_depth_test_mode(DepthTestMode::None);
    _sky_material.set_depth_write(false);

    _envmap = std::make_shared<Texture>(Texture::empty_cubemap(4, ImageFormat::RGBA8_UNORM));
}

void Scene::add_object(SceneObject obj) {
    _objects.emplace_back(std::move(obj));
}

void Scene::add_light(PointLight obj) {
    _point_lights.emplace_back(std::move(obj));
}

Span<const SceneObject> Scene::objects() const {
    return _objects;
}

Span<const PointLight> Scene::point_lights() const {
    return _point_lights;
}

Camera& Scene::camera() {
    return _camera;
}

const Camera& Scene::camera() const {
    return _camera;
}

void Scene::set_envmap(std::shared_ptr<Texture> env) {
    _envmap = std::move(env);
}

void Scene::set_ibl_intensity(float intensity) {
    _ibl_intensity = intensity;
}

void Scene::set_sun(float altitude, float azimuth, glm::vec3 color) {
    // Convert from degrees to radians
    const float alt = glm::radians(altitude);
    const float azi = glm::radians(azimuth);
    // Convert from polar to cartesian
    _sun_direction = glm::vec3(sin(azi) * cos(alt), sin(alt), cos(azi) * cos(alt));
    _sun_color = color;
}

void Scene::set_sun_bias(float bias) {
    _sun_bias = bias;
}

const glm::vec3& Scene::sun_direction() const {
    return _sun_direction;
}

BoundingSphere Scene::bounds() const {
    if (_objects.empty()) {
        return BoundingSphere();
    }

    BoundingSphere scene_bounds;
    glm::vec3 scene_min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 scene_max = glm::vec3(std::numeric_limits<float>::lowest());
    for (const SceneObject& obj : _objects) {
        const BoundingSphere obj_bounds = obj.bounds();
        const glm::vec3 obj_min = obj_bounds.center - glm::vec3(obj_bounds.radius);
        const glm::vec3 obj_max = obj_bounds.center + glm::vec3(obj_bounds.radius);
        scene_min = glm::min(scene_min, obj_min);
        scene_max = glm::max(scene_max, obj_max);
    }
    scene_bounds.center = (scene_min + scene_max) * 0.5f;
    scene_bounds.radius = 0.0f;
    for (const SceneObject& obj : _objects) {
        const BoundingSphere obj_bounds = obj.bounds();
        const float new_radius = glm::distance(scene_bounds.center, obj_bounds.center) + obj_bounds.radius;
        scene_bounds.radius = glm::max(scene_bounds.radius, new_radius);
    }
    return scene_bounds;
}

void Scene::shadow_pass(const Camera& shadow_camera) const {
    TypedBuffer<shader::FrameData> buffer(nullptr, 1);
    {
        auto mapping = buffer.map(AccessType::WriteOnly);
        mapping[0].camera.view_proj = shadow_camera.view_proj_matrix();
        mapping[0].camera.inv_view_proj = glm::inverse(shadow_camera.view_proj_matrix());
        mapping[0].camera.position = shadow_camera.position();
        mapping[0].point_light_count = u32(_point_lights.size());
        mapping[0].sun_color = _sun_color;
        mapping[0].sun_dir = glm::normalize(_sun_direction);
        mapping[0].ibl_intensity = _ibl_intensity;
    }
    buffer.bind(BufferUsage::Uniform, 0);

    for(const SceneObject& obj : _objects) {
        if(obj.material().is_opaque()) {
            obj.render(true);
        }
    }
}

void Scene::render_lighting(const glm::mat4& shadow_view_proj, const Material& material, const Material& point_light_material, const std::shared_ptr<StaticMesh>& sphere_mesh) const {
    TypedBuffer<shader::FrameData> buffer(nullptr, 1);
    {
        auto mapping = buffer.map(AccessType::WriteOnly);
        mapping[0].camera.view_proj = _camera.view_proj_matrix();
        mapping[0].camera.inv_view_proj = glm::inverse(_camera.view_proj_matrix());
        mapping[0].camera.position = _camera.position();
        mapping[0].point_light_count = u32(_point_lights.size());
        mapping[0].sun_color = _sun_color;
        mapping[0].sun_dir = glm::normalize(_sun_direction);
        mapping[0].ibl_intensity = _ibl_intensity;
        mapping[0].shadow_view_proj = shadow_view_proj;
        mapping[0].sun_bias = _sun_bias;
    }
    buffer.bind(BufferUsage::Uniform, 0);

    DEBUG_ASSERT(_envmap && !_envmap->is_null());
    _envmap->bind(4);
    brdf_lut().bind(5);

    material.bind();
    draw_full_screen_triangle();

    if(_point_lights.empty()) {
        return;
    }

    TypedBuffer<shader::PointLight> light_buffer(nullptr, _point_lights.size());
    {
        auto mapping = light_buffer.map(AccessType::WriteOnly);
        for(size_t i = 0; i != _point_lights.size(); ++i) {
            const auto& light = _point_lights[i];
            mapping[i] = {
                light.position(),
                light.radius(),
                light.color(),
                0.0f
            };
        }
    }
    light_buffer.bind(BufferUsage::Storage, 1);

    const Frustum frustum = _camera.build_frustum();
    SceneObject volume(sphere_mesh);

    point_light_material.bind();
    for(size_t i = 0; i != _point_lights.size(); ++i) {
        const auto& light = _point_lights[i];

        glm::mat4 model = glm::translate(glm::mat4(1.0f), light.position());
        model = glm::scale(model, glm::vec3(light.radius()));

        volume.set_transform(model);
        if(!volume.is_visible(_camera, frustum)) {
            continue;
        }

        point_light_material.set_uniform(HASH("model"), model);
        point_light_material.set_uniform(HASH("light_index"), u32(i));
        sphere_mesh->draw();
    }
}

void Scene::render(const glm::mat4& shadow_view_proj, bool depth_only, bool opaque_only, bool transparent_only) const {
    // Fill and bind frame data buffer
    TypedBuffer<shader::FrameData> buffer(nullptr, 1);
    {
        auto mapping = buffer.map(AccessType::WriteOnly);
        mapping[0].camera.view_proj = _camera.view_proj_matrix();
        mapping[0].camera.inv_view_proj = glm::inverse(_camera.view_proj_matrix());
        mapping[0].camera.position = _camera.position();
        mapping[0].point_light_count = u32(_point_lights.size());
        mapping[0].sun_color = _sun_color;
        mapping[0].sun_dir = glm::normalize(_sun_direction);
        mapping[0].ibl_intensity = _ibl_intensity;
        mapping[0].shadow_view_proj = shadow_view_proj;
        mapping[0].sun_bias = _sun_bias;
    }
    buffer.bind(BufferUsage::Uniform, 0);

    if(depth_only || opaque_only) {
        const Frustum frustum = _camera.build_frustum();
        for(const SceneObject& obj : _objects) {
            if(obj.material().is_opaque() && obj.is_visible(_camera, frustum)) {
                obj.render(depth_only);
            }
        }
        return;
    }

    // Fill and bind lights buffer
    TypedBuffer<shader::PointLight> light_buffer(nullptr, std::max(_point_lights.size(), size_t(1)));
    {
        auto mapping = light_buffer.map(AccessType::WriteOnly);
        for(size_t i = 0; i != _point_lights.size(); ++i) {
            const auto& light = _point_lights[i];
            mapping[i] = {
                light.position(),
                light.radius(),
                light.color(),
                0.0f
            };
        }
    }
    light_buffer.bind(BufferUsage::Storage, 1);

    // Bind envmap
    DEBUG_ASSERT(_envmap && !_envmap->is_null());
    _envmap->bind(4);

    // Bind brdf lut needed for lighting to scene rendering shaders
    brdf_lut().bind(5);

    // Render the sky
    if(!transparent_only) {
        _sky_material.bind();
        _sky_material.set_uniform(HASH("intensity"), _ibl_intensity);
        draw_full_screen_triangle();
    }

    // Render every object
    {
        const Frustum frustum = _camera.build_frustum();

        // Opaque first
        if(!transparent_only) {
            for(const SceneObject& obj : _objects) {
                if(obj.material().is_opaque() && obj.is_visible(_camera, frustum)) {
                    obj.render(false, true);
                }
            }
        }

        // Transparent after
        std::vector<const SceneObject*> transparent_objects;
        for(const SceneObject& obj : _objects) {
            if(!obj.material().is_opaque() && obj.is_visible(_camera, frustum)) {
                transparent_objects.push_back(&obj);
            }
        }

        const glm::vec3 camera_position = _camera.position();
        const glm::vec3 camera_forward = _camera.forward();
        std::sort(transparent_objects.begin(), transparent_objects.end(), [&](const SceneObject* a, const SceneObject* b) {
            const float distance_a = glm::dot(a->bounds().center - camera_position, camera_forward);
            const float distance_b = glm::dot(b->bounds().center - camera_position, camera_forward);
            return distance_a > distance_b;
        });

        for(const SceneObject* obj : transparent_objects) {
            obj->render(false, true);
        }
    }

}

}
