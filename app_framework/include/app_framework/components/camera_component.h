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
#include <app_framework/render/render_target.h>
#include <app_framework/component.h>

namespace ml {
namespace app_framework {

// Camera component
class CameraComponent final : public Component {
  RUNTIME_TYPE_REGISTER(CameraComponent)
public:
  CameraComponent() = default;
  ~CameraComponent() = default;

  glm::mat4 GetProjectionMatrix() const {
    return proj_mat_;
  }

  void SetProjectionMatrix(const glm::mat4 &mat) {
    proj_mat_ = mat;
  }

  glm::vec4 GetViewport() const {
    return viewport_;
  }

  void SetViewport(const glm::vec4 &viewport) {
    viewport_ = viewport;
  }

  std::shared_ptr<RenderTarget> GetRenderTarget() const {
    return render_target_;
  }

  void SetRenderTarget(std::shared_ptr<RenderTarget> render_target) {
    render_target_ = render_target;
  }

  std::shared_ptr<RenderTarget> GetBlitTarget() const {
    return blit_target_;
  }

  void SetBlitTarget(std::shared_ptr<RenderTarget> blit_target) {
    blit_target_ = blit_target;
  }

private:
  std::shared_ptr<RenderTarget> render_target_;
  std::shared_ptr<RenderTarget> blit_target_;
  glm::mat4 proj_mat_;
  glm::vec4 viewport_;
};
}
}
