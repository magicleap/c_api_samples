//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#include <app_framework/render/mesh.h>

#include <app_framework/render/program.h>

namespace ml {
namespace app_framework {

Mesh::Mesh(Buffer::Category buffer_category, GLenum index_buffer_element_type) {
  vert_buffer_ = std::make_shared<VertexBuffer>(buffer_category, GL_FLOAT, 3);
  normal_buffer_ = std::make_shared<VertexBuffer>(buffer_category, GL_FLOAT, 3);
  tex_coords_buffer_ =
      std::make_shared<VertexBuffer>(buffer_category, GL_FLOAT, 2);
  index_buffer_ = std::make_shared<IndexBuffer>(buffer_category, index_buffer_element_type);

  glGenVertexArrays(1, &gl_vertex_array_);

  auto vert_buffers = {
      std::make_pair(attribute_locations::kPosition, vert_buffer_),
      std::make_pair(attribute_locations::kNormal, normal_buffer_),
      std::make_pair(attribute_locations::kTextureCoordinates, tex_coords_buffer_),
  };

  glBindVertexArray(gl_vertex_array_);
  for (auto &loc_buffer_pair : vert_buffers) {
    auto &location = loc_buffer_pair.first;
    auto &buffer = loc_buffer_pair.second;

    glBindBuffer(GL_ARRAY_BUFFER, buffer->GetGLBuffer());
    glVertexAttribPointer(location, buffer->GetElementCount(), buffer->GetElementType(), GL_FALSE,
                          buffer->GetVertexSize(), (void *)0);
    glDisableVertexAttribArray(location);
  }
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_->GetGLBuffer());
  glBindVertexArray(0);
}

Mesh::~Mesh() {
  glDeleteVertexArrays(1, &gl_vertex_array_);
}

void Mesh::SetCustomBuffer(GLuint location, std::shared_ptr<VertexBuffer> buffer) {
  custom_buffers_.push_back(buffer);
  glBindVertexArray(gl_vertex_array_);
  glBindBuffer(GL_ARRAY_BUFFER, buffer->GetGLBuffer());
  glVertexAttribPointer(location, buffer->GetElementCount(), buffer->GetElementType(), GL_FALSE,
                        buffer->GetVertexSize(), (void *)0);
  glEnableVertexAttribArray(location);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::UpdateMesh(glm::vec3 const *vertices, glm::vec3 const *normals, size_t num_vertices, void const *indices,
                      size_t num_indices) {
  glBindVertexArray(gl_vertex_array_);
  if (vertices) {
    vert_buffer_->UpdateBuffer((char *)vertices, num_vertices * 3 * sizeof(float));
    glEnableVertexAttribArray(attribute_locations::kPosition);
  } else {
    glDisableVertexAttribArray(attribute_locations::kPosition);
  }
  if (normals) {
    normal_buffer_->UpdateBuffer((char *)normals, num_vertices * 3 * sizeof(float));
    glEnableVertexAttribArray(attribute_locations::kNormal);
  } else {
    glDisableVertexAttribArray(attribute_locations::kNormal);
  }
  glBindVertexArray(0);

  if (indices) {
    index_buffer_->UpdateBuffer((char *)indices, num_indices * index_buffer_->GetIndexSize());
  }

  num_vertices_ = num_vertices;
}

void Mesh::UpdateTexCoordsBuffer(glm::vec2 const *tex_coords) {
  glBindVertexArray(gl_vertex_array_);
  if (tex_coords) {
    tex_coords_buffer_->UpdateBuffer((char *)tex_coords, num_vertices_ * 2 * sizeof(float));
    glEnableVertexAttribArray(attribute_locations::kTextureCoordinates);
  } else {
    glDisableVertexAttribArray(attribute_locations::kTextureCoordinates);
  }
  glBindVertexArray(0);
}

}  // namespace app_framework
}  // namespace ml
