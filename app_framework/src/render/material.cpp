//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#include "material.h"

namespace ml {
namespace app_framework {

Material::Material(const Material& rhs) {
  dirty_ = true;
  SetVertexProgram(rhs.vert_);
  SetGeometryProgram(rhs.geom_);
  SetFragmentProgram(rhs.frag_);

  // Copy all uniform values
  for (const auto rhs_pair : rhs.variables_by_name_) {
    auto lhs_it = variables_by_name_.find(rhs_pair.first);
    lhs_it->second->CopyValue(rhs_pair.second);
  }
}

Material::~Material() {
  if(uniform_buffer_) {
    glDeleteBuffers(1, &uniform_buffer_);
  }
}

void Material::UpdateMaterialUniformBuffer() {
  // Pack the buffer
  for (const auto& des : blk_desc_.entries) {
    auto variable = variables_by_name_[des.name];
    const auto& variable_size = variable->GetSize();
    const auto& variable_name = variable->GetName();

    ML_LOG_IF(Fatal, des.name != variable_name,
              "Name of the entry in the shader(%s) and variable(%s) are different",
              des.name.c_str(), variable_name.c_str());
    ML_LOG_IF(Fatal, des.size != variable_size,
              "Size of the entry in the shader(%" PRIu64 ") and variable(%" PRIu64 ") are different",
              des.size, variable_size);
    memcpy(ubo_cache_.data() + des.offset, variable->GetMemoryPtr(), variable_size);
  }
  glBindBuffer(GL_UNIFORM_BUFFER, uniform_buffer_);
  glBufferData(GL_UNIFORM_BUFFER, ubo_cache_.size(), ubo_cache_.data(), GL_DYNAMIC_DRAW);
  dirty_ = false;
}

void Material::UpdateMaterialUniforms() {
  // Update texture
  for(int i = 0; i < textures_des_.size(); ++i) {
    const auto& des = textures_des_[i];
    auto variable = variables_by_name_[des.name];
    std::shared_ptr<Texture> tex = variable->GetValue<std::shared_ptr<Texture>>();
    if (!tex) {
      continue;
    }
    glActiveTexture(GL_TEXTURE0 + i);
    glBindTexture(tex->GetTextureType(), tex->GetGLTexture());
  }
}

void Material::BuildVariables() {
  const auto& fragment_ubo_blk_list = frag_->GetUniformBlocks();
  auto fragment_ubo_it = fragment_ubo_blk_list.find(UniformName::kMaterial);
  if (fragment_ubo_it != fragment_ubo_blk_list.end()) {
    blk_desc_ = fragment_ubo_it->second;
    glGenBuffers(1, &uniform_buffer_);
    ubo_cache_.resize(blk_desc_.size);
    variables_by_name_.clear();

    for (const auto& des: blk_desc_.entries) {
      std::shared_ptr<Variable> variable = MakeVariableFromGLType(des.name, des.type);
      variables_by_name_[variable->GetName()] = variable;
    }
  }

  // Build for texture binding
  const auto& fragment_ubo_list = frag_->GetUniforms();
  for (const auto& pair : fragment_ubo_list) {
    const auto& des = pair.second;
    if (des.type == GL_SAMPLER_2D || des.type == GL_SAMPLER_2D_ARRAY) {
      textures_des_.push_back(des);
      glUseProgram(frag_->GetGLProgram());
      glUniform1i(des.location, textures_des_.size() - 1);
      std::shared_ptr<Variable> variable = MakeVariableFromGLType(des.name, des.type);
      variables_by_name_[variable->GetName()] = variable;
      glUseProgram(0);
    }
  }
}

}
}
