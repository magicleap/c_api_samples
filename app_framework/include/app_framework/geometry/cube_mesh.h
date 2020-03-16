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

static const std::array<glm::vec3, 8> g_vertex_positions = {{
    {-0.5f, -0.5f, -0.5f},
    {-0.5f, -0.5f, +0.5f},
    {-0.5f, +0.5f, -0.5f},
    {-0.5f, +0.5f, +0.5f},
    {+0.5f, -0.5f, -0.5f},
    {+0.5f, -0.5f, +0.5f},
    {+0.5f, +0.5f, -0.5f},
    {+0.5f, +0.5f, +0.5f},
}};

static const std::array<glm::vec4, 8> g_vertex_colors = {{
    {0.0f, 0.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 1.0f, 1.0f},
    {0.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 1.0f, 1.0f},
    {1.0f, 0.0f, 0.0f, 1.0f},
    {1.0f, 0.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 1.0f, 1.0f},
}};

static const std::array<std::array<GLushort, 2>, 12> g_indices = {{
    // front
    {0, 4},
    {4, 6},
    {6, 2},
    {2, 0},
    // Back
    {1, 5},
    {5, 7},
    {7, 3},
    {3, 1},
    // Left
    {5, 4},
    {7, 6},
    // Right
    {1, 0},
    {3, 2},
}};

class CubeMesh final : public Mesh {
  RUNTIME_TYPE_REGISTER(CubeMesh)
public:
  CubeMesh() : Mesh(Buffer::Category::Static, GL_UNSIGNED_SHORT) {
    color_buffer_ = std::make_shared<VertexBuffer>(Buffer::Category::Static, GL_FLOAT, 4);
    color_buffer_->UpdateBuffer((char *)g_vertex_colors.data(), sizeof(g_vertex_colors));

    SetCustomBuffer(attribute_locations::kColor, color_buffer_);
    UpdateMesh(g_vertex_positions.data(), nullptr, g_vertex_positions.size(), (uint16_t *)g_indices.data(), 3 * 12);
  }
  ~CubeMesh() = default;

private:
  std::shared_ptr<VertexBuffer> color_buffer_;
};

}  // namespace app_framework
}  // namespace ml
