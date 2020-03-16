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
#include <app_framework/graphics_context.h>
#include <app_framework/node.h>
#include <app_framework/render/texture.h>

#include <ml_types.h>

#include "imgui.h"
#include "examples/imgui_impl_opengl3.h"

#include <chrono>

struct MLInputControllerState;

namespace ml {
namespace app_framework {

// ui system integrated with Imgui
class Gui final {
public:
  static Gui &GetInstance() {
    static Gui gui;
    return gui;
  }

  void Initialize(GraphicsContext *g, MLHandle input_handle = ML_INVALID_HANDLE);
  void Cleanup();

  void BeginUpdate();
  void EndUpdate();

  std::shared_ptr<Node> GetNode() const {
    return gui_node_;
  }

  GLuint GetFrameBuffer() const {
    return imgui_framebuffer_;
  }

  std::shared_ptr<Texture> GetTexture() const {
    return off_screen_texture_;
  }

  bool GetShowImgui();
private:
  Gui();
  Gui(const Gui& other) = delete;
  Gui(Gui&& other) = delete;
  Gui& operator=(const Gui& other) = delete;
  Gui& operator=(Gui&& other) = delete;
  ~Gui() = default;

  std::shared_ptr<Texture> off_screen_texture_;
  MLHandle input_handle_;
  MLVec3f prev_touch_pos_and_force_;
  glm::vec2 cursor_pos_;
  enum class State { Hidden, Moving, Placed } state_;
  std::shared_ptr<Node> gui_node_;
  bool prev_toggle_state_ = false;

  bool owned_input_;
  GLuint imgui_color_texture_;
  GLuint imgui_framebuffer_;
  GLuint imgui_depth_renderbuffer_;
  std::chrono::steady_clock::time_point previous_time_;
  void UpdateState(const MLInputControllerState &input_state);
  void UpdateMousePosAndButtons(const MLInputControllerState &input_state);
  void UpdateMousePosAndButtons();
  void *window_;
};

}  // namespace app_framework
}  // namespace ml
