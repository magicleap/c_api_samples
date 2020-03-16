// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once

#include <app_framework/input/input_handler.h>

#include <ml_input.h>

namespace ml {
namespace app_framework {

class MlInputHandler : public InputHandler {
public:
  void Initialize(MLHandle input_handle = ML_INVALID_HANDLE);
  void Cleanup();

private:
  static void OnKeyDown(MLKeyCode, uint32_t, void *);
  static void OnChar(uint32_t, void *);
  static void OnKeyUp(MLKeyCode, uint32_t, void *);

  MLHandle input_handle_;
  bool owned_input_;
};

}  // namespace app_framework
}  // namespace ml
