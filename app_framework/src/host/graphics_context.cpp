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
#include <app_framework/graphics_context.h>
#include <app_framework/ml_macros.h>

#include <GLFW/glfw3.h>

#include <cstdlib>

namespace ml {
namespace app_framework {

class GraphicsContextCallbacks {
public:
  static void OnClose(GraphicsContext *instance) {
    if (instance->close_callback_) {
      instance->close_callback_();
    }
  }
  static void OnResize(GraphicsContext *instance, int width, int height) {
    instance->frame_buffer_dimensions_.first = width;
    instance->frame_buffer_dimensions_.second = height;
  }

  static void OnKey(GraphicsContext *instance, GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, true);
    }
  }
};

GraphicsContext::GraphicsContext(const char *title, bool window_visible, int32_t window_width, int32_t window_height) {
  if (!glfwInit()) {
    ML_LOG(Fatal, "glfwInit() failed");
    std::exit(1);
  }
  glfwSetErrorCallback(
      [](int error, const char *description) { ML_LOG(Fatal, "GLFW error (%X):%s", error, description); });

  glfwWindowHint(GLFW_VISIBLE, window_visible);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
#if ML_OSX
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#else
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);

  window_handle_ = glfwCreateWindow(window_width, window_height, title, NULL, NULL);
  if (!window_handle_) {
    ML_LOG(Fatal, "glfwCreateWindow() failed");
    std::exit(1);
  }

  glfwSetWindowUserPointer(window_handle_, this);
  glfwGetFramebufferSize(window_handle_, &frame_buffer_dimensions_.first, &frame_buffer_dimensions_.second);

  // register windows close callback function
  glfwSetWindowCloseCallback(window_handle_, [](GLFWwindow *window) {
    GraphicsContext *this_context = static_cast<GraphicsContext *>(glfwGetWindowUserPointer(window));
    GraphicsContextCallbacks::OnClose(this_context);
  });

  // register windows key callback function to listen for the ESC key
  glfwSetKeyCallback(window_handle_, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
    GraphicsContext *this_context = static_cast<GraphicsContext *>(glfwGetWindowUserPointer(window));
    GraphicsContextCallbacks::OnKey(this_context, window, key, scancode, action, mods);
  });

  // register frame buffer resize callback function
  glfwSetFramebufferSizeCallback(window_handle_, [](GLFWwindow *window, int width, int height) {
    GraphicsContext *this_context = static_cast<GraphicsContext *>(glfwGetWindowUserPointer(window));
    GraphicsContextCallbacks::OnResize(this_context, width, height);
  });

  glfwMakeContextCurrent(window_handle_);
  glfwSwapInterval(0);
}

void GraphicsContext::MakeCurrent() {
  glfwMakeContextCurrent(window_handle_);
}

void GraphicsContext::UnMakeCurrent() {}

std::pair<int32_t, int32_t> GraphicsContext::GetFramebufferDimensions() {
  return frame_buffer_dimensions_;
}

void GraphicsContext::SetWindowCloseCallback(std::function<void(void)> callback) {
  close_callback_ = callback;
}

void GraphicsContext::SwapBuffers() {
  glfwSwapBuffers(window_handle_);
  glfwPollEvents();
}

void GraphicsContext::SetTitle(const char *title) {
  glfwSetWindowTitle(window_handle_, title);
}

MLHandle GraphicsContext::GetContextHandle() const {
  return reinterpret_cast<MLHandle>(glfwGetCurrentContext());
}

void *GraphicsContext::GetDisplayHandle() const {
  return window_handle_;
}

GraphicsContext::~GraphicsContext() {
  glfwDestroyWindow(window_handle_);
  window_handle_ = nullptr;
}

GraphicsContext::gl_proc_t GraphicsContext::GetProcAddress(char const *procname) {
  return glfwGetProcAddress(procname);
}
}  // namespace app_framework
}  // namespace ml
