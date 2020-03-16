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
#include <app_framework/render/vertex_program.h>

namespace ml {
namespace app_framework {

static const char *kMagicLeapMeshGeometryShader = R"GLSL(
  #version 410 core
  layout (triangles) in;
  layout (line_strip, max_vertices = 12) out;

  in gl_PerVertex {
    vec4 gl_Position;
  } gl_in[];
  layout (location = 0) in vec4 colors[];
  layout (location = 1) in vec4 normals[];

  out gl_PerVertex {
    vec4 gl_Position;
  };
  layout (location = 0) out vec4 out_color;

  const float MAGNITUDE = 0.05;

  void GenerateLine(vec4 pos1, vec4 pos2, vec4 color1, vec4 color2) {
    gl_Position = pos1;
    out_color = color1;
    EmitVertex();
    gl_Position = pos2;
    out_color = color2;
    EmitVertex();
    EndPrimitive();
  }

  void GenerateLineNormal(int index) {
    GenerateLine(gl_in[index].gl_Position,
                 gl_in[index].gl_Position + MAGNITUDE * normals[index],
                 vec4(1.0), vec4(1.0));
  }

  void main() {
    GenerateLineNormal(0); // first vertex normal
    GenerateLineNormal(1); // second vertex normal
    GenerateLineNormal(2); // third vertex normal

    GenerateLine(gl_in[0].gl_Position,
                 gl_in[1].gl_Position,
                 colors[0], colors[1]);

    GenerateLine(gl_in[1].gl_Position,
                 gl_in[2].gl_Position,
                 colors[1], colors[2]);

    GenerateLine(gl_in[2].gl_Position,
                 gl_in[0].gl_Position,
                 colors[2], colors[0]);
  }
)GLSL";
}
}
