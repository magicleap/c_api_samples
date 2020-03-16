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

static const char *kSolidColorFragmentShader = R"GLSL(
  #version 410 core

  layout(std140) uniform Material {
    vec4 Color;
    bool OverrideVertexColor;
  } material;

  layout (location = 0) in vec4 in_color;

  layout (location = 0) out vec4 out_color;

  void main() {
    if (material.OverrideVertexColor) {
      out_color = material.Color;
    } else {
      out_color = in_color;
    }
  }
)GLSL";
}
}
