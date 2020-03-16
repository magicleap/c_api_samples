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
#include <app_framework/input/ml_input_handler.h>

namespace ml {
namespace app_framework {

void MlInputHandler::Initialize(MLHandle input_handle) {
  if (input_handle == ML_INVALID_HANDLE) {
    owned_input_ = true;
    MLInputCreate(nullptr, &input_handle_);
  } else {
    owned_input_ = false;
    input_handle_ = input_handle;
  }
  MLInputKeyboardCallbacks keyboard_callbacks;
  keyboard_callbacks.on_char = OnChar;
  keyboard_callbacks.on_key_up = OnKeyUp;
  keyboard_callbacks.on_key_down = OnKeyDown;
  MLInputSetKeyboardCallbacks(input_handle_, &keyboard_callbacks, this);
}

void MlInputHandler::Cleanup() {
  if (owned_input_) {
    MLInputDestroy(input_handle_);
  }
}

void MlInputHandler::OnKeyDown(MLKeyCode key_code, uint32_t key_modifier, void *context) {
  MlInputHandler *instance = static_cast<MlInputHandler *>(context);
  auto callback = instance->GetOnKeyDownCallback();
  if (!callback) {
    return;
  }

  EventArgs key_event_args = {0, static_cast<uint32_t>(key_code), key_modifier};
  callback(key_event_args);
}

void MlInputHandler::OnChar(uint32_t key_value, void *context) {
  MlInputHandler *instance = static_cast<MlInputHandler *>(context);
  auto callback = instance->GetOnCharCallback();
  if (!callback) {
    return;
  }

  EventArgs key_event_args = {key_value, MLKEYCODE_UNKNOWN, 0};
  callback(key_event_args);
}

void MlInputHandler::OnKeyUp(MLKeyCode key_code, uint32_t key_modifier, void *context) {
  MlInputHandler *instance = static_cast<MlInputHandler *>(context);
  auto callback = instance->GetOnKeyUpCallback();
  if (!callback) {
    return;
  }

  EventArgs key_event_args = {0, static_cast<uint32_t>(key_code), key_modifier};
  callback(key_event_args);
}

}  // namespace app_framework
}  // namespace ml
