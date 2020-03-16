//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once

namespace ml {
namespace app_framework {

static const char *kSimpleTexturedFragmentShader = R"GLSL(
  #version 410 core

  uniform sampler2D Texture0;

  layout (location = 0) in vec2 in_tex_coords;

  layout (location = 0) out vec4 out_color;

  void main() {
    out_color = texture(Texture0, in_tex_coords);
  }
)GLSL";

}
}
