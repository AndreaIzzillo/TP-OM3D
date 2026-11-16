#include "Material.h"

#include <glad/gl.h>

#include <algorithm>

namespace OM3D {

Material::Material() {
}

void Material::set_program(std::shared_ptr<Program> prog) {
    _program = std::move(prog);
}

void Material::set_blend_mode(BlendMode blend) {
    _blend_mode = blend;
}

void Material::set_depth_test_mode(DepthTestMode depth) {
    _depth_test_mode = depth;
}

void Material::set_depth_write(bool depth_write) {
    _depth_write = depth_write;
}

void Material::set_cull_mode(CullMode cull) {
    _cull_mode = cull;
}

void Material::set_double_sided(bool double_sided) {
    _cull_mode = double_sided ? CullMode::None : CullMode::Back;
}

void Material::set_texture(u32 slot, std::shared_ptr<Texture> tex) {
    if(const auto it = std::find_if(_textures.begin(), _textures.end(), [&](const auto& t) { return t.first == slot; }); it != _textures.end()) {
        it->second = std::move(tex);
    } else {
        _textures.emplace_back(slot, std::move(tex));
    }
}

bool Material::is_opaque() const {
    return _blend_mode == BlendMode::None;
}

void Material::set_stored_uniform(u32 name_hash, UniformValue value) {
    for(auto& [h, v] : _uniforms) {
        if(h == name_hash) {
            v = value;
            return;
        }
    }
    _uniforms.emplace_back(name_hash, std::move(value));
}

void Material::bind(bool depth_only, bool forward) const {
    switch(_cull_mode) {
        case CullMode::None:
            glDisable(GL_CULL_FACE);
        break;

        case CullMode::Back:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glFrontFace(GL_CCW);
        break;

        case CullMode::Front:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            glFrontFace(GL_CCW);
        break;
    }

    switch(_blend_mode) {
        case BlendMode::None:
            glDisable(GL_BLEND);
        break;

        case BlendMode::Alpha:
            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_ADD);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;

        case BlendMode::Additive:
            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_ADD);
            glBlendFunc(GL_ONE, GL_ONE);
        break;
    }

    switch(depth_only ? DepthTestMode::Standard : _depth_test_mode) {
        case DepthTestMode::None:
            glDisable(GL_DEPTH_TEST);
        break;

        case DepthTestMode::Equal:
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_EQUAL);
        break;

        case DepthTestMode::Standard:
            glEnable(GL_DEPTH_TEST);
            // We are using reverse-Z
            glDepthFunc(GL_GEQUAL);
        break;

        case DepthTestMode::Reversed:
            glEnable(GL_DEPTH_TEST);
            // We are using reverse-Z
            glDepthFunc(GL_LEQUAL);
        break;
    }

    glDepthMask(depth_only || _depth_write);

    for(const auto& texture : _textures) {
        texture.second->bind(texture.first);
    }

    const auto& program = depth_only ? _depth_program : (forward && _forward_program ? _forward_program : _program);
    for(const auto& [h, v] : _uniforms) {
        program->set_uniform(h, v);
    }

    program->bind();
}

Material Material::textured_pbr_material(bool alpha_test, bool gbuffer) {
    Material material;

    std::vector<std::string> defines;
    if(alpha_test) {
        defines.emplace_back("ALPHA_TEST");
    }

    material._program = Program::from_files(gbuffer ? "gbuffer.frag" : "lit.frag", "basic.vert", defines);
    material._depth_program = Program::from_files("depth.frag", "basic.vert", defines);
    if(gbuffer) {
        material._forward_program = Program::from_files("lit.frag", "basic.vert", defines);
    }
    material.set_depth_test_mode(DepthTestMode::Equal);
    material.set_depth_write(false);

    material.set_stored_uniform(HASH("base_color_factor"), glm::vec4(1.0f));
    material.set_stored_uniform(HASH("metal_rough_factor"), glm::vec2(1.0f));
    material.set_stored_uniform(HASH("emissive_factor"), glm::vec3(0.0f));

    material.set_texture(0u, default_white_texture());
    material.set_texture(1u, default_normal_texture());
    material.set_texture(2u, default_metal_rough_texture());
    material.set_texture(3u, default_white_texture());

    return material;
}


}
