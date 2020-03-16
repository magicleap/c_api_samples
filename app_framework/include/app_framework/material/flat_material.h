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
#include <app_framework/registry.h>
#include <app_framework/shader/solid_color_fs_program.h>
#include <app_framework/shader/solid_color_vs_program.h>
#include <app_framework/render/material.h>

namespace ml {
namespace app_framework {

class FlatMaterial final : public Material {
public:
  FlatMaterial(const glm::vec4 &color) {
    SetVertexProgram(Registry::GetInstance()->GetResourcePool()->LoadShaderFromCode<VertexProgram>(kSolidColorVertexShader));
    SetFragmentProgram(Registry::GetInstance()->GetResourcePool()->LoadShaderFromCode<FragmentProgram>(kSolidColorFragmentShader));
    SetColor(color);
  }
  ~FlatMaterial() = default;

  MATERIAL_VARIABLE_DECLARE(bool, OverrideVertexColor);
  MATERIAL_VARIABLE_DECLARE(glm::vec4, Color);
};
}
}
