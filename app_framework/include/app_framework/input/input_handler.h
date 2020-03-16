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

#include <functional>

namespace ml {
namespace app_framework {

class InputHandler {
public:
  struct EventArgs {
    uint32_t key_char;
    uint32_t key_code;  // see MLKeyCode
    uint32_t key_modifier;  // see MLKeyModifier
  };
  typedef std::function<void(EventArgs)> KeyCallback;

  InputHandler() = default;
  virtual ~InputHandler() = default;

  void SetEventHandlers(KeyCallback on_key_down, KeyCallback on_char, KeyCallback on_key_up) {
    on_key_down_ = on_key_down;
    on_char_ = on_char;
    on_key_up_ = on_key_up;
  }

protected:
  KeyCallback GetOnKeyDownCallback() const {
    return on_key_down_;
  }

  KeyCallback GetOnCharCallback() const {
    return on_char_;
  }

  KeyCallback GetOnKeyUpCallback() const {
    return on_key_up_;
  }

private:
  KeyCallback on_key_down_;
  KeyCallback on_char_;
  KeyCallback on_key_up_;
};

}  // namespace app_framework
}  // namespace ml
