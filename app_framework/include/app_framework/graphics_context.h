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

#include <ml_api.h>

#include <functional>
#include <mutex>
#include <utility>

#if !ML_LUMIN
struct GLFWwindow;
#endif
namespace ml {
namespace app_framework {

class GraphicsContextCallbacks;
class GraphicsContext {
public:
  GraphicsContext(const char *title = "", bool window_visible = false, int32_t window_width = 1,
                  int32_t window_height = 1);
  ~GraphicsContext();

  void MakeCurrent();
  void SwapBuffers();
  void UnMakeCurrent();
  void SetTitle(const char *title);
  MLHandle GetContextHandle() const;
  void *GetDisplayHandle() const;
  void SetWindowCloseCallback(std::function<void(void)> call_back);
  std::pair<int32_t, int32_t> GetFramebufferDimensions();
  using gl_proc_t = void (*)();
  static gl_proc_t GetProcAddress(char const *procname);

private:
  std::function<void(void)> close_callback_ = nullptr;
#if ML_LUMIN
  void *display_handle_;
  void *context_handle_;
#else
  GLFWwindow *window_handle_;
#endif
  std::pair<int32_t, int32_t> frame_buffer_dimensions_;
  friend class GraphicsContextCallbacks;
};

}  // namespace app_framework
}  // namespace ml
