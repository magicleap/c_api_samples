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

#ifndef EGL_EGLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace ml {
namespace app_framework {
GraphicsContext::GraphicsContext(const char *, bool, int32_t, int32_t) {
  display_handle_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  EGLint major = 4;
  EGLint minor = 3;
  eglInitialize(display_handle_, &major, &minor);
  eglBindAPI(EGL_OPENGL_API);

  EGLint config_attribs[] = {EGL_RED_SIZE,   5,  EGL_GREEN_SIZE,   6, EGL_BLUE_SIZE, 5, EGL_ALPHA_SIZE, 0,
                             EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8, EGL_NONE};
  EGLConfig egl_config = nullptr;
  EGLint config_size = 0;
  eglChooseConfig(display_handle_, config_attribs, &egl_config, 1, &config_size);

  EGLint context_attribs[] = {EGL_CONTEXT_MAJOR_VERSION_KHR, 3, EGL_CONTEXT_MINOR_VERSION_KHR, 0, EGL_NONE};
  context_handle_ = eglCreateContext(display_handle_, egl_config, EGL_NO_CONTEXT, context_attribs);
}

void GraphicsContext::MakeCurrent() {
  eglMakeCurrent(display_handle_, EGL_NO_SURFACE, EGL_NO_SURFACE, context_handle_);
}

void GraphicsContext::UnMakeCurrent() {
  eglMakeCurrent(NULL, EGL_NO_SURFACE, EGL_NO_SURFACE, NULL);
}

std::pair<int32_t, int32_t> GraphicsContext::GetFramebufferDimensions() {
  return std::make_pair(0, 0);
}

void GraphicsContext::SetWindowCloseCallback(std::function<void(void)> call_back) {}

void GraphicsContext::SwapBuffers() {
  // buffer swapping is implicit on device (MLGraphicsEndFrame)
}

void GraphicsContext::SetTitle(const char *title) {
  // No gl window exists on device
}

MLHandle GraphicsContext::GetContextHandle() const {
  return reinterpret_cast<MLHandle>(context_handle_);
}

void *GraphicsContext::GetDisplayHandle() const {
  return nullptr;
}

GraphicsContext::~GraphicsContext() {
  eglDestroyContext(display_handle_, context_handle_);
  eglTerminate(display_handle_);
}

GraphicsContext::gl_proc_t GraphicsContext::GetProcAddress(char const *procname) {
  return eglGetProcAddress(procname);
}

}  // namespace app_framework
}  // namespace ml
