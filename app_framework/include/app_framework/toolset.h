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
#include <app_framework/components/text_component.h>
#include <app_framework/geometry/axis_mesh.h>
#include <app_framework/geometry/cube_mesh.h>
#include <app_framework/geometry/quad_mesh.h>
#include <app_framework/material/flat_material.h>
#include <app_framework/node.h>
#include <app_framework/render/mesh.h>

namespace ml {
namespace app_framework {

enum class NodeType { Cube, Text, Quad, Line, Axis };

// Helper function to simplify the work
inline std::shared_ptr<Node> CreatePresetNode(NodeType type) {
  std::shared_ptr<Node> node = std::make_shared<ml::app_framework::Node>();
  std::shared_ptr<Mesh> mesh;

  std::shared_ptr<FlatMaterial> material = std::make_shared<FlatMaterial>(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
  switch (type) {
    case NodeType::Cube: {
      mesh = Registry::GetInstance()->GetResourcePool()->GetMesh<CubeMesh>();
      material->SetOverrideVertexColor(false);
      material->SetPolygonMode(GL_LINE);
    } break;
    case NodeType::Text: {
      material->SetOverrideVertexColor(true);
      material->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
      std::shared_ptr<TextComponent> text_component = std::make_shared<TextComponent>();
      mesh = text_component->GetMesh();
      node->AddComponent(text_component);

      static constexpr size_t g_char_height_px = 8;
      static constexpr float g_font_size_m = 0.01f / g_char_height_px;
      node->SetLocalScale(glm::vec3(g_font_size_m, -g_font_size_m, 1.0f));
      material->SetPolygonMode(GL_FILL);
    } break;
    case NodeType::Quad: {
      mesh = Registry::GetInstance()->GetResourcePool()->GetMesh<QuadMesh>();
      material->SetOverrideVertexColor(true);
      material->SetPolygonMode(GL_FILL);
    } break;
    case NodeType::Line: {
      mesh = std::make_shared<Mesh>(Buffer::Category::Dynamic, GL_UNSIGNED_SHORT);
      material->SetOverrideVertexColor(true);
      material->SetColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
      material->SetPolygonMode(GL_LINE);
    } break;
    case NodeType::Axis: {
      mesh = Registry::GetInstance()->GetResourcePool()->GetMesh<Axis>();
      material->SetOverrideVertexColor(false);
      material->SetPolygonMode(GL_LINE);
    } break;
  }

  std::shared_ptr<RenderableComponent> renderable = std::make_shared<RenderableComponent>(mesh, material);
  node->AddComponent(renderable);

  switch (type) {
    case NodeType::Axis: {
      mesh->SetPrimitiveType(GL_LINE_STRIP);
    } break;
    case NodeType::Cube: {
      mesh->SetPrimitiveType(GL_LINES);
    } break;
    default: {
      break;
    }
  }

  return node;
}

}  // namespace app_framework
}  // namespace ml
