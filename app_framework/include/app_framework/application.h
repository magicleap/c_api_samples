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
#include <array>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <csignal>
#include <mutex>
#include <queue>

#include <app_framework/graphics_context.h>

#include <app_framework/node.h>
#include <app_framework/render/renderer.h>

#include <app_framework/components/camera_component.h>
#include <app_framework/components/renderable_component.h>
#include <app_framework/render/renderer.h>
#include <app_framework/resource_pool.h>
#include <app_framework/render/texture.h>
#include <app_framework/registry.h>

#include <ml_graphics.h>
#include <ml_lifecycle.h>
#include <ml_perception.h>

using ApplicationClock = std::chrono::steady_clock;

namespace std {
  template<>
  struct hash<std::pair<MLHandle,uint32_t>> {
    size_t operator()(const std::pair<MLHandle,uint32_t> & key) const {
      return key.first << 32 | key.second;
    }
  };
};

namespace ml {
namespace app_framework {

/*! Application status */
enum class ApplicationStatus {
  Stopped, /**< Stopped */
  Paused,  /**< Paused */
  Running  /**< Running */
};

/*! The base class for sample applications */
class Application {
public:
  /*!
    \brief Constructor. Uses gflags to parse the arguments.
    \param argc Number of arguments.
    \param argv Pointer to the first element of the arguments.
  */
  Application(int argc = 0, char **argv = nullptr);
  /*! Destructor */
  virtual ~Application();

  /*!
    \brief Runs the application.
  */
  void RunApp();

  /*!
    \brief Enables and disables Gfx. By default, Gfx is enabled.
    \param use_gfx true to enable, false to disable.
  */
  void SetUseGfx(bool use_gfx);

  /*!
    \brief Sets the title of the GLWindow on host.
    \param title The UTF-8 encoded window title.
  */
  void SetTitle(const char* title);

  std::shared_ptr<Node> GetRoot() const {
    return root_;
  }

  std::shared_ptr<Node> GetDefaultLightNode() const {
    return light_node_;
  }

  /*!
    \brief Update the graphics frame params. This includes near and
    far clip. Take a look at ml_graphics.h MLGraphicsFrameParamsExInit
    function which has defaults for the struct members.
  */
  void UpdateFrameParams(const MLGraphicsFrameParamsEx &frame_params);

protected:
  void StopApp();  /*!< Sends a signal to stop the application. */
  const MLLifecycleSelfInfo &GetLifecycleInfo() const;
  const std::vector<std::string> &GetLifecycleStartupArgUri() const;  /*!< Reads the lifecycle init args consumed by app_framework at startup */
  const std::vector<std::string> &GetLifecycleArgUri();  /*!< Queries init args from Lifecycle. */

  virtual void OnStart() {}
  virtual void OnPause() {}
  virtual void OnResume() {}
  virtual void OnStop() {}
  virtual void OnUpdate(float delta_time_scale) {}
  virtual void OnRender(MLGraphicsVirtualCameraInfo const &current_camera) {}
  virtual void OnRenderCamera(std::shared_ptr<CameraComponent> cam) {}

  GraphicsContext * GetGraphicsContext() {
    return graphics_context_.get();
  }

private:
  void Initialize();
  void InitializeLifecycle();
  void InitializeLifecycleSelfInfo();
  void InitializePerception();
  void InitializeGraphics();
  void InitializeSignalHandler();

  void Terminate();
  void TerminateLifecycle();
  void TerminatePerception();
  void TerminateGraphics();
  void TerminateSignalHandler();

  void Update();
  void Render();
  void SyncAppStatus();
  void Sleep();
  void WakeUp();

  void ParseArgs(int argc, char **argv);
  static void KeyCallback(void* window, int key, int scancode, int action, int mods);

  // App Info
  ApplicationStatus app_status_;
  ApplicationClock::time_point prev_update_time_;
  bool use_gfx_ = true;
  bool interrupt_sleep_;
  std::mutex sleep_mutex_;
  std::condition_variable sleep_cv_;
  static std::atomic<bool> exit_signal_;
  std::string window_title_;

  // Graphics
  std::unique_ptr<GraphicsContext> graphics_context_;
  MLGraphicsOptions graphics_options_;
  MLHandle opengl_context_;
  MLHandle graphics_client_;
  MLGraphicsFrameParamsEx frame_params_;
  size_t dropped_frames_ = 0;

  // Lifecycle
  std::atomic<ApplicationStatus> lifecycle_status_;
  MLLifecycleSelfInfo *lifecycle_info_ = nullptr;
  std::vector<std::string> lifecycle_startup_arg_uri_;
  std::vector<std::string> lifecycle_arg_uri_;

  void UpdateMLCamera(const MLGraphicsFrameInfo &frame_info);
  MLHandle frame_handle_;
  std::array<MLHandle, 2> ml_sync_objs_;
  // Texture handle, and layer index as the key
  std::unordered_map<std::pair<MLHandle,uint32_t> , std::shared_ptr<RenderTarget>> ml_render_target_cache_;
  std::shared_ptr<RenderTarget> default_render_target_;

  // Renderer
  std::unique_ptr<app_framework::Renderer> renderer_;

  // Node
  std::shared_ptr<Node> light_node_;
  std::shared_ptr<Node> root_;
  std::queue<std::shared_ptr<Node>> queue_;
  void VisitNode(std::shared_ptr<Node> node);

  // Default ml camera
  std::array<std::shared_ptr<Node>, 2> camera_nodes_;
  void InternalRenderCamCallback(std::shared_ptr<CameraComponent> cam);

  // this is for periodically logging graphics performance info
  ApplicationClock::time_point prev_gfx_perf_log_;

  // this only logs the framerate, but it's useful for both graphical and non-graphical apps
  ApplicationClock::time_point prev_update_perf_log_;
  uint32_t num_frames_ = 0;
};

}  // namespace app_framework
}  // namespace ml
