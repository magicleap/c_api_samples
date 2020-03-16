//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once
#include <vector>

#include "common.h"
#include "geometry/axis_mesh.h"
#include "geometry/cube_mesh.h"
#include "geometry/quad_mesh.h"

namespace ml {
namespace app_framework {

struct PresetResource {
  PresetResource() :
    meshes{
      std::make_shared<Axis>(),
      std::make_shared<CubeMesh>(),
      std::make_shared<QuadMesh>()
    } {}
  std::vector<std::shared_ptr<Mesh>> meshes;
};

}
}
