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

// GL fragment program
class FragmentProgram final : public Program {
public:
  // Initialize the program with null-terminated code string
  FragmentProgram(const char *code) : Program(code, GL_FRAGMENT_SHADER) {}
  virtual ~FragmentProgram() = default;
};
}
}
