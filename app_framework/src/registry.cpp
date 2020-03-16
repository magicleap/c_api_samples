//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#include <app_framework/registry.h>

namespace ml {
namespace app_framework {

void Registry::Initialize() {
  pool_.reset(new ResourcePool());
  pool_->InitializePresetResources();
}

void Registry::Cleanup() {
  if (pool_) {
    pool_.reset();
  }
}

const std::unique_ptr<Registry>& Registry::GetInstance() {
  static std::unique_ptr<Registry> instance(new Registry());
  return instance;
}

}
}
