//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include <app_framework/common.h>

namespace ml {
namespace app_framework {

namespace attribute_locations {
  static constexpr GLuint kPosition = 0;
  static constexpr GLuint kNormal = 1;
  static constexpr GLuint kTextureCoordinates = 2;
  static constexpr GLuint kColor = 3;
  static constexpr GLuint kConfidence = 4;
}

class UniformName final {
public:
  UniformName() = delete;
  static const std::string kMaterial;
  static const std::string kCamera;
  static const std::string kModel;
  static const std::string kLight;
};

struct VertexAttributeDescription {
  VertexAttributeDescription() : index(0), size(0), element_cnt(0), location(0), type(0) {}
  std::string name;
  uint32_t index;
  uint64_t size;
  uint32_t element_cnt;
  int32_t location;
  GLenum type;
};

struct UniformDescription {
  UniformDescription() : index(0), size(0), location(0), type(0), offset(0) {}
  std::string name;
  uint32_t index;
  uint64_t size;
  int32_t location;
  uint64_t offset;
  GLenum type;
};

struct UniformBlockDescription {
  UniformBlockDescription() : index(0), size(0), binding(0) {}
  std::string name;
  uint32_t index;
  uint64_t size;
  uint32_t binding;

  std::vector<UniformDescription> entries;
};

// GL program
class Program {
public:
  // Initialize the program with null-terminated code string
  Program(const char *code, GLenum type);
  virtual ~Program();

  inline GLuint GetGLProgram() const {
    return program_;
  }

  inline const std::unordered_map<std::string, UniformBlockDescription>& GetUniformBlocks() const {
    return uniform_blocks_by_name_;
  }

  inline const std::unordered_map<std::string, UniformDescription>& GetUniforms() const {
    return uniforms_by_name_;
  }

  inline GLenum GetGLProgramType() const {
    return type_;
  };

private:
  GLuint program_;
  GLenum type_;
  GLint uniform_cnt_;
  GLint uniform_blk_cnt_;

  static GLint sMaxUniformBinding;
  static GLint sVertBindingLocation;
  static GLint sGeomBindingLocation;
  static GLint sFragBindingLocation;

  std::unordered_map<std::string, UniformBlockDescription> uniform_blocks_by_name_;
  std::unordered_map<std::string, UniformDescription> uniforms_by_name_;
};
}
}
