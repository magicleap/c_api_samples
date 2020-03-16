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

static const char *kSolidColorVertexShader = R"GLSL(
  #version 410 core

  layout(std140) uniform Camera {
    mat4 view_proj;
    vec4 world_position;
  } camera;

  layout(std140) uniform Model {
    mat4 transform;
  } model;

  layout (location = 0) in vec3 position;
  layout (location = 3) in vec4 color;

  out gl_PerVertex {
      vec4 gl_Position;
  };

  layout (location = 0) out vec4 out_color;

  void main() {
    gl_Position = camera.view_proj * model.transform * vec4(position, 1.0);
    out_color = color;
  }
)GLSL";
}
}
