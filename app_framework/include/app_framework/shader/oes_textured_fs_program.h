//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once

#if ML_LUMIN

namespace ml {
namespace app_framework {

static const char *kOESTexturedFragmentShader = R"GLSL(
  #version 310 es
  #define GLES_VERSION 310
  #extension GL_OES_EGL_image_external_essl3 : enable
  #extension GL_OES_EGL_image_external : enable
  precision mediump float;

  uniform samplerExternalOES Texture0;

  layout (location = 0) in vec2 in_tex_coords;

  layout (location = 0) out vec4 out_color;

  void main() {
    out_color = texture(Texture0, vec2(in_tex_coords.x, 1.0f - in_tex_coords.y));
  }
)GLSL";

}
}

#endif
