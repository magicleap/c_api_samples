//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#include "geometry_program.h"
#include "gl_type_size.h"

namespace ml {
namespace app_framework {

GeometryProgram::GeometryProgram(const char *code) : Program(code, GL_GEOMETRY_SHADER) {
  // Parse parameters
  glGetProgramiv(GetGLProgram(), GL_GEOMETRY_INPUT_TYPE, &type_);
}
}
}
