//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#include "render_target.h"

namespace ml {
namespace app_framework {

RenderTarget::~RenderTarget() {
  if (gl_framebuffer_) {
    glDeleteFramebuffers(1, &gl_framebuffer_);
  }
}

void RenderTarget::InitializeFramebuffer() {
  gl_framebuffer_ = 0;
  if (color_) {
    width_ = color_->GetWidth();
    height_ = color_->GetHeight();
    glGenFramebuffers(1, &gl_framebuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_framebuffer_);

    auto color_texture_type = color_->GetTextureType();
    if (color_texture_type == GL_TEXTURE_2D_ARRAY) {
      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color_->GetGLTexture(), 0,
                                color_layer_index_);
    } else if (color_texture_type == GL_TEXTURE_2D) {
      glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color_->GetGLTexture(), 0);
    }

    // Only do depth when we have color
    if (depth_) {
      auto depth_texture_type = depth_->GetTextureType();
      if (depth_texture_type == GL_TEXTURE_2D_ARRAY) {
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_->GetGLTexture(), 0,
                                  depth_layer_index_);
      } else if (depth_texture_type == GL_TEXTURE_2D) {
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_->GetGLTexture(), 0);
      }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
}

}
}
