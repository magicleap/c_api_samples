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
#include "texture.h"

namespace ml {
namespace app_framework {

class RenderTarget final {
public:
  RenderTarget(int32_t width, int32_t height) : gl_framebuffer_(0), color_layer_index_(0), depth_layer_index_(0), width_(width), height_(height) {}
  RenderTarget(std::shared_ptr<Texture> color, std::shared_ptr<Texture> depth,
               uint32_t color_layer_index, uint32_t depth_layer_index)
    : color_layer_index_(color_layer_index), depth_layer_index_(depth_layer_index),
      color_(color), depth_(depth) {
      InitializeFramebuffer();
    }
  ~RenderTarget();

  std::shared_ptr<Texture> GetColorTexture() const {
    return color_;
  }

  std::shared_ptr<Texture> GetDepthTexture() const {
    return depth_;
  }

  GLuint GetGLFramebuffer() const {
    return gl_framebuffer_;
  }

  void SetWidth(int32_t width) {
    if (color_) {
      ML_LOG(Warning, "Setting render target size to a non-default framebuffer");
    }
    width_ = width;
  }

  void SetHeight(int32_t height) {
    if (color_) {
      ML_LOG(Warning, "Setting render target size to a non-default framebuffer");
    }
    height_ = height;
  }

  int32_t GetWidth() const {
    return width_;
  }

  int32_t GetHeight() const {
    return height_;
  }

  uint32_t GetColorTextureLayerIndex() const {
    return color_layer_index_;
  }

  uint32_t GetDepthTextureLayerIndex() const {
    return depth_layer_index_;
  }

private:
  void InitializeFramebuffer();
  std::shared_ptr<Texture> color_;
  std::shared_ptr<Texture> depth_;
  uint32_t color_layer_index_;
  uint32_t depth_layer_index_;
  int32_t width_;
  int32_t height_;
  GLuint gl_framebuffer_;
};

}  // namespace app_framework
}  // namespace ml
