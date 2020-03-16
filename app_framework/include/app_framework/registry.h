//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once
#include "common.h"
#include "resource_pool.h"

namespace ml {
namespace app_framework {

class Registry final {
public:
  Registry() = default;
  ~Registry() = default;

  static const std::unique_ptr<Registry>& GetInstance();

  void Initialize();
  void Cleanup();

  const std::unique_ptr<ResourcePool>& GetResourcePool() const {
    return pool_;
  }
private:
  std::unique_ptr<ResourcePool> pool_;
};
}
}
