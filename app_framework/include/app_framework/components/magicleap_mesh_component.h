//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once
#include <app_framework/common.h>
#include <app_framework/render/mesh.h>
#include <app_framework/render/program.h>
#include <app_framework/render/vertex_buffer.h>

namespace ml {
namespace app_framework {

// Magic Leap Mesh component for mesh data
class MagicLeapMeshComponent : public Component {
  RUNTIME_TYPE_REGISTER(MagicLeapMeshComponent)
public:
  MagicLeapMeshComponent() {
    confidence_buffer_ = std::make_shared<VertexBuffer>(Buffer::Category::Dynamic, GL_FLOAT, 1);
    mesh_ = std::make_shared<Mesh>(Buffer::Category::Dynamic, GL_UNSIGNED_SHORT);
    mesh_->SetCustomBuffer(attribute_locations::kConfidence, confidence_buffer_);
  }

  ~MagicLeapMeshComponent() = default;

  std::shared_ptr<Mesh> GetMesh() const {
    return mesh_;
  }

  void UpdateMeshWithConfidence(glm::vec3 const *vertices, glm::vec3 const *normals, float const *confidences,
                                size_t num_vertices, uint16_t const *indices, size_t num_indices) {
    mesh_->UpdateMesh(vertices, normals, num_vertices, indices, num_indices);
    auto buffer = confidences;
    if (!buffer) {
      if (dummy_buffer_.size() < num_vertices) {
        dummy_buffer_.resize(num_vertices);
      }
      buffer = dummy_buffer_.data();
    }
    confidence_buffer_->UpdateBuffer((char *)buffer, num_vertices * sizeof(float));
  }

private:
  std::shared_ptr<VertexBuffer> confidence_buffer_;
  std::vector<float> dummy_buffer_;
  std::shared_ptr<Mesh> mesh_;
};

}  // namespace app_framework
}  // namespace ml
