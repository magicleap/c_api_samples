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
#include <app_framework/shader/magicleap_mesh_gs_program.h>
#include <app_framework/shader/magicleap_mesh_vs_program.h>
#include <app_framework/shader/solid_color_fs_program.h>
#include <app_framework/render/material.h>

namespace ml {
namespace app_framework {

// Magic Leap Mesh component for mesh visualization
class MagicLeapMeshVisualizationMaterial final : public Material {
public:
  MagicLeapMeshVisualizationMaterial() : Material() {
    SetVertexProgram(Registry::GetInstance()->GetResourcePool()->LoadShaderFromCode<VertexProgram>(kMagicLeapMeshVertexShader));
    SetFragmentProgram(Registry::GetInstance()->GetResourcePool()->LoadShaderFromCode<FragmentProgram>(kSolidColorFragmentShader));
    SetGeometryProgram(Registry::GetInstance()->GetResourcePool()->LoadShaderFromCode<GeometryProgram>(kMagicLeapMeshGeometryShader));
    SetOverrideVertexColor(false);
  }
  ~MagicLeapMeshVisualizationMaterial() = default;

private:
  MATERIAL_VARIABLE_DECLARE(bool, OverrideVertexColor);
};
}
}
