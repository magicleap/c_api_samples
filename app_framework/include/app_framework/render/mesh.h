//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once
#include <unordered_map>

#include <app_framework/common.h>
#include <app_framework/render/index_buffer.h>
#include <app_framework/render/vertex_buffer.h>

#include <vector>

namespace ml {
namespace app_framework {

class ResourcePool;

// Geometry data
class Mesh : public RuntimeType {
  RUNTIME_TYPE_REGISTER(Mesh)
public:
  Mesh(Buffer::Category buffer_category, GLenum index_buffer_element_type = GL_UNSIGNED_INT);
  Mesh(const Mesh &other) = delete;
  Mesh &operator=(const Mesh &other) = delete;
  virtual ~Mesh();

  void SetCustomBuffer(GLuint location, std::shared_ptr<VertexBuffer> buffer);

  void UpdateMesh(glm::vec3 const *vertices, glm::vec3 const *normals, size_t num_vertices, void const *indices,
      size_t num_indices);

  void UpdateTexCoordsBuffer(glm::vec2 const *tex_coords);

  bool UsesIndexedRendering() const {
    return GL_POINTS == primitive_type_ || !index_buffer_ || index_buffer_->GetIndexCount() == 0;
  }

  GLuint GetNumVertices() const {
    return num_vertices_;
  }

  GLuint GetNumIndices() {
    return index_buffer_->GetIndexCount();
  }

  GLenum GetIndexType() const {
    return index_buffer_->GetIndexType();
  }

  GLuint GetVertexArrayObject() {
    return gl_vertex_array_;
  }

  GLint GetPrimitiveType() const {
    return primitive_type_;
  }

  GLint SetPrimitiveType(GLint primitive_type) {
    return primitive_type_ = primitive_type;
  }

  GLfloat GetPointSize() const {
    return point_size_;
  }

  GLfloat SetPointSize(GLfloat point_size) {
    return point_size_ = point_size;
  }

private:
  std::shared_ptr<VertexBuffer> normal_buffer_;
  std::shared_ptr<IndexBuffer> index_buffer_;
  std::shared_ptr<VertexBuffer> vert_buffer_;
  std::shared_ptr<VertexBuffer> tex_coords_buffer_;
  GLint primitive_type_ = GL_TRIANGLES;
  GLfloat point_size_ = 1.f;

  GLuint gl_vertex_array_ = 0;
  GLsizeiptr num_vertices_ = 0;

  std::vector<std::shared_ptr<VertexBuffer>> custom_buffers_;
};
}  // namespace app_framework
}  // namespace ml
