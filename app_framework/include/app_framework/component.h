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

namespace ml {
namespace app_framework {

class Node;

class Component : public RuntimeType {
  RUNTIME_TYPE_REGISTER(Component)
public:
  Component() = default;
  virtual ~Component() = default;

  void SetNode(std::shared_ptr<Node> node) {
    node_ = node;
  }

  std::shared_ptr<Node> GetNode() const {
    return node_;
  }

private:
  std::shared_ptr<Node> node_;
};
}
}
