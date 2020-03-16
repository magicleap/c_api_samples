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
#include <app_framework/render/material.h>
#include <app_framework/render/mesh.h>
#include <app_framework/component.h>

namespace ml {
namespace app_framework {

// Renderable component
class RenderableComponent final : public Component {
  RUNTIME_TYPE_REGISTER(RenderableComponent)
public:
  RenderableComponent(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material)
      : mesh_(mesh), material_(material), visible_(true) {}
  ~RenderableComponent() = default;

  void SetVisible(bool visible) {
    visible_ = visible;
  }

  void SetMesh(std::shared_ptr<Mesh> mesh) {
    mesh_ = mesh;
  }

  void SetMaterial(std::shared_ptr<Material> material) {
    material_ = material;
  }

  bool GetVisible() const {
    return visible_;
  }

  std::shared_ptr<Mesh> GetMesh() const {
    return mesh_;
  }

  std::shared_ptr<Material> GetMaterial() const {
    return material_;
  }

private:
  bool visible_;
  std::shared_ptr<Mesh> mesh_;
  std::shared_ptr<Material> material_;
};

}
}
