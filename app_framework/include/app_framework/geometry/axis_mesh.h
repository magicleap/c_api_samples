//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once
#include <array>

#include <app_framework/common.h>
#include <app_framework/render/mesh.h>
#include <app_framework/render/program.h>

namespace ml {
namespace app_framework {

class Axis final : public Mesh {
  RUNTIME_TYPE_REGISTER(Axis)

  const std::array<glm::vec3, 8> g_vertex_positions = {{
      {0.0f, 0.0f, 0.0f},
      {0.5f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.5f, 0.0f},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.5f},
  }};

  const std::array<glm::vec4, 8> g_vertex_colors = {{
      {1.0f, 0.0f, 0.0f, 1.0f},
      {1.0f, 0.0f, 0.0f, 1.0f},
      {0.0f, 1.0f, 0.0f, 1.0f},
      {0.0f, 1.0f, 0.0f, 1.0f},
      {0.0f, 0.0f, 1.0f, 1.0f},
      {0.0f, 0.0f, 1.0f, 1.0f},
  }};

public:
  Axis() : Mesh(Buffer::Category::Static) {
    color_buffer_ = std::make_shared<VertexBuffer>(Buffer::Category::Static, GL_FLOAT, 4);
    color_buffer_->UpdateBuffer((char *)g_vertex_colors.data(), sizeof(g_vertex_colors));

    SetCustomBuffer(attribute_locations::kColor, color_buffer_);
    UpdateMesh(g_vertex_positions.data(), nullptr, g_vertex_positions.size(), nullptr, 0);
  }
  ~Axis() = default;

private:
  std::shared_ptr<VertexBuffer> color_buffer_;
};
}  // namespace app_framework
}  // namespace ml
