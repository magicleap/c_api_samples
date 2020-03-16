//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once
#include "program.h"
#include <app_framework/common.h>

namespace ml {
namespace app_framework {

// GL Vertex program
class VertexProgram final : public Program {
public:
  // Initialize the program with null-terminated code string
  VertexProgram(const char *code) : Program(code, GL_VERTEX_SHADER) {}
  virtual ~VertexProgram() = default;
};
}  // namespace app_framework
}  // namespace ml
