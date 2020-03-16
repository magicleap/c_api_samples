//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#include <set>

#include "gl_type_size.h"
#include "program.h"

namespace ml {
namespace app_framework {

const std::string UniformName::kMaterial("Material");
const std::string UniformName::kCamera("Camera");
const std::string UniformName::kModel("Model");
const std::string UniformName::kLight("Lights");

GLint Program::sMaxUniformBinding = 0;
GLint Program::sVertBindingLocation = 0;
GLint Program::sGeomBindingLocation = 0;
GLint Program::sFragBindingLocation = 0;

Program::Program(const char *code, GLenum type) : program_(0), type_(type), uniform_cnt_(0), uniform_blk_cnt_(0) {
  GLint success = 0;
  char info_log[512]{};

  program_ = glCreateShaderProgramv(type_, 1, &code);
  glGetProgramiv(program_, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(program_, 512, nullptr, info_log);
    ML_LOG(Fatal, "Shader compilation failed: %s", info_log);
  }
  glUseProgram(program_);

  GLint binding = 0;
  if (sMaxUniformBinding == 0) {
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &sMaxUniformBinding);

    GLint slots = sMaxUniformBinding / 3;

    sVertBindingLocation = 0;
    sGeomBindingLocation = sVertBindingLocation + slots;
    sFragBindingLocation = sGeomBindingLocation + slots;
  }
  // Parse parameters
  std::vector<GLchar> name_buffer(512);

  if (type_ == GL_VERTEX_SHADER) {
    binding = sVertBindingLocation;
  } else if (type == GL_GEOMETRY_SHADER) {
    binding = sGeomBindingLocation;
  } else {
    binding = sFragBindingLocation;
  }

  std::set<GLint> excluded_set;

  glGetProgramiv(program_, GL_ACTIVE_UNIFORM_BLOCKS, &uniform_blk_cnt_);
  for (int32_t i = 0; i < uniform_blk_cnt_; ++i) {
    UniformBlockDescription data{};
    GLsizei char_written = 0;

    data.index = i;
    data.binding = binding++;
    glGetActiveUniformBlockName(program_, i, name_buffer.size(), &char_written, name_buffer.data());
    data.name = std::string((char *)name_buffer.data(), char_written);

    glGetActiveUniformBlockiv(program_, i, GL_UNIFORM_BLOCK_DATA_SIZE, (GLint *)&data.size);
    glUniformBlockBinding(program_, i, data.binding);

    GLint active_uniforms = 0;
    glGetActiveUniformBlockiv(program_, i, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &active_uniforms);

    std::vector<GLint> indices(active_uniforms);
    glGetActiveUniformBlockiv(program_, i, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, indices.data());

    std::vector<GLint> offsets(active_uniforms);
    glGetActiveUniformsiv(program_, indices.size(), (GLuint*)indices.data(), GL_UNIFORM_OFFSET, offsets.data());

    data.entries.resize(active_uniforms);
    for (int32_t j = 0; j < active_uniforms; j++) {
      UniformDescription member{};
      member.index = indices[j];
      excluded_set.insert(member.index);
      GLsizei char_written = 0;
      glGetActiveUniform(program_, member.index, name_buffer.size(), &char_written, (GLint *)&member.size, &member.type,
                         name_buffer.data());
      member.name = std::string((char *)name_buffer.data(), char_written);
      member.size = GetGLTypeSize(member.type) * member.size;
      member.location = glGetUniformLocation(program_, member.name.c_str());
      member.offset = offsets[j];

      auto it = member.name.find(data.name);
      if (it != std::string::npos) {
        member.name.replace(it, data.name.length() + 1, "");
      }

      ML_LOG(Debug, "Found uniform member: name(%s), index(%u), size(%" PRIu64 "), offset(%" PRIu64 ") type(%x)", member.name.c_str(),
             member.index, member.size, member.offset, member.type);

      data.entries[j] = member;
    }
    uniform_blocks_by_name_[data.name] = data;

    ML_LOG(Debug, "Found uniform: name(%s), index(%u), size(%" PRIu64 "), binding(%u)", data.name.c_str(),
           data.index, data.size, data.binding);
  }

  glGetProgramiv(program_, GL_ACTIVE_UNIFORMS, &uniform_cnt_);
  for (int32_t i = 0; i < uniform_cnt_; ++i) {
    if (excluded_set.find(i) != excluded_set.end()) {
      continue;
    }
    UniformDescription data{};
    data.index = i;
    GLsizei char_written = 0;
    glGetActiveUniform(program_, data.index, name_buffer.size(), &char_written, (GLint *)&data.size, &data.type,
                       name_buffer.data());
    data.name = std::string((char *)name_buffer.data(), char_written);
    data.size = GetGLTypeSize(data.type) * data.size;
    data.location = glGetUniformLocation(program_, data.name.c_str());

    ML_LOG(Debug, "Found uniform: name(%s), index(%u), size(%" PRIu64 "), type(%x)", data.name.c_str(), data.index,
           data.size, data.type);

    uniforms_by_name_[data.name] = data;
  }
  glUseProgram(0);
}

Program::~Program() {
  if (program_) {
    glDeleteProgram(program_);
    program_ = 0;
  }
}
}
}
