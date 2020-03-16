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

namespace ml {
namespace app_framework {

class Texture final {
public:
  Texture(GLint texture_type, GLuint tex, int32_t width, int32_t height, bool owned = false)
      : texture_type_(texture_type),
        texture_(tex),
        owned_(owned),
        width_(width),
        height_(height) {}

  Texture() : Texture(GL_TEXTURE_2D, 0, 0, 0, false) {}
  ~Texture();

  void SetWidth(int32_t width) {
    width_ = width;
  }

  void SetHeight(int32_t height) {
    height_ = height;
  }

  int32_t GetWidth() const {
    return width_;
  }

  int32_t GetHeight() const {
    return height_;
  }

  GLenum GetTextureType() const {
    return texture_type_;
  }

  GLuint GetGLTexture() const {
    return texture_;
  }

private:
  GLuint texture_;
  GLint texture_type_;
  int32_t width_;
  int32_t height_;
  bool owned_;
};
}
}
