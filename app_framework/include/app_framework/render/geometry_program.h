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
#include "program.h"

namespace ml {
namespace app_framework {

// GL program, it includes the basic transform UBO metadata if the shader type is Vertex
class GeometryProgram final : public Program {
public:
  // Initialize the program with null-terminated code string
  GeometryProgram(const char *code);
  virtual ~GeometryProgram() = default;

  inline GLint GetInputPrimitiveType() {
    return type_;
  }

private:
  GLint type_;
};
}
}
