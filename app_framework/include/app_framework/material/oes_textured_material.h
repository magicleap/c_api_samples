//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once

#if ML_LUMIN

#include <app_framework/common.h>
#include <app_framework/registry.h>
#include <app_framework/shader/oes_textured_fs_program.h>
#include <app_framework/shader/simple_textured_vs_program.h>
#include <app_framework/render/material.h>
#include <app_framework/render/texture.h>

namespace ml {
namespace app_framework {

class OESTexturedMaterial final : public Material {
public:
  OESTexturedMaterial(std::shared_ptr<Texture> tex) {
    SetVertexProgram(Registry::GetInstance()->GetResourcePool()->LoadShaderFromCode<VertexProgram>(kSimpleTexturedVertexShader));
    SetFragmentProgram(Registry::GetInstance()->GetResourcePool()->LoadShaderFromCode<FragmentProgram>(kOESTexturedFragmentShader));
    SetTexture0(tex);
  }
  ~OESTexturedMaterial() = default;

  MATERIAL_VARIABLE_DECLARE(std::shared_ptr<Texture>, Texture0);
};

}
}

#endif
