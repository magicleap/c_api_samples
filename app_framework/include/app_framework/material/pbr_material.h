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
#include <app_framework/shader/pbr_fs_program.h>
#include <app_framework/shader/pbr_vs_program.h>
#include <app_framework/render/material.h>

namespace ml {
namespace app_framework {

class PBRMaterial final : public Material {
public:
  PBRMaterial() {
    SetVertexProgram(Registry::GetInstance()->GetResourcePool()->LoadShaderFromCode<VertexProgram>(kPBRVertexShader));
    SetFragmentProgram(Registry::GetInstance()->GetResourcePool()->LoadShaderFromCode<FragmentProgram>(kPBRFragmentShader));
  }
  ~PBRMaterial() = default;

  MATERIAL_VARIABLE_DECLARE(std::shared_ptr<Texture>, Albedo);

  MATERIAL_VARIABLE_DECLARE(std::shared_ptr<Texture>, Metallic);

  MATERIAL_VARIABLE_DECLARE(std::shared_ptr<Texture>, Roughness);

  MATERIAL_VARIABLE_DECLARE(std::shared_ptr<Texture>, AmbientOcclusion);

  MATERIAL_VARIABLE_DECLARE(std::shared_ptr<Texture>, Emissive);

  MATERIAL_VARIABLE_DECLARE(std::shared_ptr<Texture>, Normals);

  MATERIAL_VARIABLE_DECLARE(int32_t, MetallicChannel);

  MATERIAL_VARIABLE_DECLARE(int32_t, RoughnessChannel);

  MATERIAL_VARIABLE_DECLARE(bool, HasNormals);

  MATERIAL_VARIABLE_DECLARE(bool, HasAlbedo);

  MATERIAL_VARIABLE_DECLARE(bool, HasNormalMap);

  MATERIAL_VARIABLE_DECLARE(bool, HasMetallic);

  MATERIAL_VARIABLE_DECLARE(bool, HasRoughness);

  MATERIAL_VARIABLE_DECLARE(bool, HasAmbientOcclusion);

  MATERIAL_VARIABLE_DECLARE(bool, HasEmissive);
};
}
}
