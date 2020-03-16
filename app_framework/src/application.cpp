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
#include <app_framework/application.h>
#include <app_framework/cli_args_parser.h>
#include <app_framework/geometry/quad_mesh.h>
#include <app_framework/material/textured_material.h>
#include <app_framework/ml_macros.h>

#if !ML_LUMIN
#include <GLFW/glfw3.h>
#endif

#include <gflags/gflags.h>
#include <glad/glad.h>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/gtx/quaternion.hpp>

#include <ml_logging.h>

#include <cstdlib>

namespace chrono = std::chrono;

namespace ml {
namespace app_framework {

DEFINE_int32(LogLevel, 3,
    "Enable output of log messages up to and including the log level.\n"
    "0: Fatal, 1: Error, 2: Warning, 3: Info, 4: Debug, 5: Verbose");

DEFINE_int32(gl_debug_severity, 2,
    "Enable OpenGL logging for messages up to and including the given severity.\n"
    "0: Off, 1: High, 2: Medium, 3: Low, 4: Notification");

DEFINE_bool(mirror_window, true,
    "In addition to streaming graphics to MLRemote, also make a native window to mirror the display");

DEFINE_int32(window_width, 800, "Width of the window on the host machine.");

DEFINE_int32(window_height, 600, "Height of the window on the host machine.");

DEFINE_int32(frame_timing_hint, 60,
    "Suggested rate for how frequently the application will render new frames. Can be either 60 or 120");

DEFINE_double(perf_log_rate, 0,
    "How often to log the performance information in seconds. A value of 0 will prevent logging this performance "
    "information at all. This also requires LogLevel to be 4 or greater.");

namespace {
static bool ValidateFrameTimeingHint(const char *flagname, gflags::int32 value) {
  bool valid = 60 == value || 120 == value;
  ML_LOG_IF(Error, !valid, "%s cannot be %d. It must be 60 or 120", flagname, value);
  return valid;
}
}  // namespace

DEFINE_validator(frame_timing_hint, &ValidateFrameTimeingHint);

#if ML_WINDOWS && ML_USE_HIGH_PERFORMANCE_GPU

// enable Nvidia and AMD specific symbols for high performance GPU
extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}

#endif

// the type must be lock-free
std::atomic<bool> Application::exit_signal_;

Application::Application(int argc, char **argv) {
  InitializeLifecycle();
  ParseArgs(argc, argv);
  SetTitle(gflags::ProgramInvocationShortName());
  MLLoggingEnableLogLevel(static_cast<MLLogLevel>(FLAGS_LogLevel));
  prev_gfx_perf_log_ = ApplicationClock::now();
  prev_update_perf_log_ = ApplicationClock::now();
  MLGraphicsFrameParamsExInit(&frame_params_);
}

Application::~Application() {}

void Application::SetUseGfx(bool use_gfx) {
  use_gfx_ = use_gfx;
}

void Application::SetTitle(const char *title) {
  window_title_ = std::string(title);
  if (graphics_context_) {
    graphics_context_->SetTitle(window_title_.c_str());
  }
}

const MLLifecycleSelfInfo &Application::GetLifecycleInfo() const {
  return *lifecycle_info_;
}

const std::vector<std::string> &Application::GetLifecycleArgUri() {
  lifecycle_arg_uri_.clear();
  MLLifecycleInitArgList *list = nullptr;
  MLResult result = MLLifecycleGetInitArgList(&list);
  if ((MLResult_Ok == result) && (nullptr != list)) {
    int64_t list_length = 0;
    result = MLLifecycleGetInitArgListLength(list, &list_length);
    if ((MLResult_Ok == result) && (list_length > 0)) {
      const MLLifecycleInitArg *iarg = nullptr;
      for (int64_t i = 0; i < list_length; ++i) {
        result = MLLifecycleGetInitArgByIndex(list, i, &iarg);
        if ((MLResult_Ok == result) && (nullptr != iarg)) {
          const char *uri = nullptr;
          result = MLLifecycleGetInitArgUri(iarg, &uri);
          if ((MLResult_Ok == result) && (nullptr != uri)) {
            lifecycle_arg_uri_.push_back(std::string(uri));
          }
        }
      }
    }
    MLLifecycleFreeInitArgList(&list);
  }
  return lifecycle_arg_uri_;
}

const std::vector<std::string> &Application::GetLifecycleStartupArgUri() const {
  return lifecycle_startup_arg_uri_;
}

void Application::Initialize() {
  exit_signal_ = false;
  InitializePerception();
  if (use_gfx_) {
    InitializeGraphics();
  }

  // The initialization is finished. The app is ready to go.
  UNWRAP_MLRESULT_FATAL(MLLifecycleSetReadyIndication());

  // InitializeSignalHandler should be the last to be called
  InitializeSignalHandler();
}

void Application::InitializeLifecycle() {
  // let system know our app has started
  MLLifecycleCallbacksEx lifecycle_callbacks = {};
  MLLifecycleCallbacksExInit(&lifecycle_callbacks);

  lifecycle_callbacks.on_stop = [](void *context) {
    ML_LOG(Info, "On stop called.");
    Application *this_app = static_cast<Application *>(context);
    this_app->lifecycle_status_ = ApplicationStatus::Stopped;
    this_app->WakeUp();
  };
  lifecycle_callbacks.on_pause = [](void *context) {
    ML_LOG(Info, "On pause called.");
    Application *this_app = static_cast<Application *>(context);
    this_app->lifecycle_status_ = ApplicationStatus::Paused;
  };
  lifecycle_callbacks.on_resume = [](void *context) {
    ML_LOG(Info, "On resume called.");
    Application *this_app = static_cast<Application *>(context);
    this_app->lifecycle_status_ = ApplicationStatus::Running;
    this_app->WakeUp();
  };

  lifecycle_callbacks.on_device_active = [](void *) { ML_LOG(Info, "On active called."); };
  lifecycle_callbacks.on_device_reality = [](void *) { ML_LOG(Info, "On reality called."); };
  lifecycle_callbacks.on_device_standby = [](void *) { ML_LOG(Info, "On standby called."); };
  UNWRAP_MLRESULT_FATAL(MLLifecycleInitEx(&lifecycle_callbacks, (void *)this));

  InitializeLifecycleSelfInfo();
}

void Application::InitializeLifecycleSelfInfo() {
  if (lifecycle_info_ == nullptr) {
    UNWRAP_MLRESULT_FATAL(MLLifecycleGetSelfInfo(&lifecycle_info_));
    ML_LOG(Debug, "writable_dir_path: %s", lifecycle_info_->writable_dir_path);
    ML_LOG(Debug, "package_dir_path: %s", lifecycle_info_->package_dir_path);
  }
}

void Application::InitializePerception() {
  MLPerceptionSettings perception_settings = {};
  UNWRAP_MLRESULT_FATAL(MLPerceptionInitSettings(&perception_settings));
  UNWRAP_MLRESULT_FATAL(MLPerceptionStartup(&perception_settings));
}

static void PrintGlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    const GLchar *message, const void *userParam) {
  MLLogLevel log_level = MLLogLevel_Error;
  switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH: log_level = MLLogLevel_Error; break;
    case GL_DEBUG_SEVERITY_MEDIUM: log_level = MLLogLevel_Error; break;
    case GL_DEBUG_SEVERITY_LOW: log_level = MLLogLevel_Warning; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: log_level = MLLogLevel_Debug; break;
    default: break;
  }
  MLLoggingLogVargs(log_level, ML_DEFAULT_LOG_TAG, "GL DEBUG(0x%X): %s", id, message);
}

void Application::InitializeGraphics() {
#ifdef ML_LUMIN
  graphics_context_.reset(new GraphicsContext());
#else
  graphics_context_.reset(
      new GraphicsContext(window_title_.c_str(), FLAGS_mirror_window, FLAGS_window_width, FLAGS_window_height));
#endif  // #ifdef ML_LUMIN
  graphics_context_->SetTitle(window_title_.c_str());
  graphics_context_->SetWindowCloseCallback([this]() { StopApp(); });
  graphics_context_->MakeCurrent();

  if (!gladLoadGLLoader((GLADloadproc)ml::app_framework::GraphicsContext::GetProcAddress)) {
    ML_LOG(Fatal, "gladLoadGLLoader() failed");
  }
#ifdef ML_LUMIN
  if (!gladLoadGLES2Loader((GLADloadproc)ml::app_framework::GraphicsContext::GetProcAddress)) {
    ML_LOG(Fatal, "gladLoadGLES2Loader() failed");
  }
#endif

  auto *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
  auto *vendor = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
  auto *render = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
  ML_LOG(Info, "OpenGL Version: %s", version);
  ML_LOG(Info, "OpenGL Vendor: %s", vendor);
  ML_LOG(Info, "OpenGL Render: %s", render);

#if !ML_OSX
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  // clang-format off
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, FLAGS_gl_debug_severity >= 1);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, FLAGS_gl_debug_severity >= 2);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, FLAGS_gl_debug_severity >= 3);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, FLAGS_gl_debug_severity >= 4);
  // clang-format on
  glDebugMessageCallback(PrintGlDebugMessage, nullptr);
#endif

  // Get ready to connect our GL context to the MLSDK graphics API
  graphics_options_ = {0, MLSurfaceFormat_RGBA8UNorm, MLSurfaceFormat_D32Float};
  opengl_context_ = graphics_context_->GetContextHandle();
  graphics_client_ = ML_INVALID_HANDLE;
  MLGraphicsCreateClientGL(&graphics_options_, opengl_context_, &graphics_client_);

  if (60 == FLAGS_frame_timing_hint) {
    MLGraphicsSetFrameTimingHint(graphics_client_, MLGraphicsFrameTimingHint_60Hz);
  } else if (120 == FLAGS_frame_timing_hint) {
    MLGraphicsSetFrameTimingHint(graphics_client_, MLGraphicsFrameTimingHint_120Hz);
  } else {
    ML_LOG(Fatal, "Invalid input for frame_timing_hint");
  }

  // Init ml render targets
  MLGraphicsRenderTargetsInfo targets{};
  MLGraphicsGetRenderTargets(graphics_client_, &targets);
  frame_params_.near_clip = targets.min_clip;

  for (int32_t i = 0; i < MLGraphics_BufferCount; ++i) {
    auto &buffer = targets.buffers[i];
    if (buffer.color.id == 0) {
      continue;
    }
    auto color_tex = std::make_shared<Texture>(
        GL_TEXTURE_2D_ARRAY, (GLuint)buffer.color.id, buffer.color.width, buffer.color.height, false);
    auto depth_tex = std::make_shared<Texture>(
        GL_TEXTURE_2D_ARRAY, (GLuint)buffer.depth.id, buffer.depth.width, buffer.depth.height, false);
    // Implicit assumption, left eye is always index 0
    auto left_render_target = std::make_shared<RenderTarget>(color_tex, depth_tex, 0, 0);
    auto right_render_target = std::make_shared<RenderTarget>(color_tex, depth_tex, 1, 1);

    // Only use color buffer as the key
    ml_render_target_cache_.insert(std::make_pair(std::make_pair(buffer.color.id, 0), left_render_target));
    ml_render_target_cache_.insert(std::make_pair(std::make_pair(buffer.color.id, 1), right_render_target));
  }

  // Initialize the new renderer and setting the post render camera callback
  renderer_.reset(new Renderer());
  auto cb = [this](std::shared_ptr<CameraComponent> camera) { Application::InternalRenderCamCallback(camera); };
  renderer_->SetPostRenderCameraCallback(cb);
  Registry::GetInstance()->Initialize();

  // Init nodes
  root_ = std::make_shared<Node>();
  for (int i = 0; i < camera_nodes_.size(); ++i) {
    camera_nodes_[i] = std::make_shared<Node>();
    auto camera = std::make_shared<CameraComponent>();
    camera_nodes_[i]->AddComponent(camera);
    root_->AddChild(camera_nodes_[i]);
  }

  // Setup light node
  light_node_ = std::make_shared<ml::app_framework::Node>();
  std::shared_ptr<LightComponent> light_component = std::make_shared<LightComponent>();
  light_component->SetLightStrength(5.0f);
  light_node_->AddComponent(light_component);
  root_->AddChild(light_node_);
}

void Application::InitializeSignalHandler() {
  auto signal_handler = [](int) { exit_signal_ = true; };

  if (SIG_ERR == std::signal(SIGINT, signal_handler)) {
    ML_LOG(Error, "Failed to register the signal handler(SIGINT)");
  }
  if (SIG_ERR == std::signal(SIGTERM, signal_handler)) {
    ML_LOG(Error, "Failed to register the signal handler(SIGTERM)");
  }
}

void Application::Terminate() {
  if (use_gfx_) {
    TerminateGraphics();
  }
  TerminatePerception();
  TerminateLifecycle();

  // TerminateSignalHandler should be the last to be called
  TerminateSignalHandler();
}

void Application::TerminateLifecycle() {
  if (lifecycle_info_) {
    MLResult ml_result = MLLifecycleFreeSelfInfo(&lifecycle_info_);
    if (ml_result != MLResult_Ok) {
      ML_LOG(Error, "MLLifecycleFreeSelfInfo returned %d - %s", ml_result, MLGetResultString(ml_result));
    }
  }
}

void Application::TerminatePerception() {
  MLResult ml_result = MLPerceptionShutdown();
  if (ml_result != MLResult_Ok) {
    ML_LOG(Error, "MLPerceptionShutdown returned %d - %s", ml_result, MLGetResultString(ml_result));
  }
}

void Application::TerminateGraphics() {
  graphics_context_->UnMakeCurrent();
  MLResult ml_result = MLGraphicsDestroyClient(&graphics_client_);
  if (ml_result != MLResult_Ok) {
    ML_LOG(Error, "MLGraphicsDestroyClient returned %d - %s", ml_result, MLGetResultString(ml_result));
  }
}

void Application::TerminateSignalHandler() {
  std::signal(SIGINT, SIG_DFL);
  std::signal(SIGTERM, SIG_DFL);
}

void Application::Update() {
  auto update_time = ApplicationClock::now();

  chrono::duration<float> delta_time = update_time - prev_update_time_;
  num_frames_++;
  auto delta = update_time - prev_update_perf_log_;
  if (FLAGS_perf_log_rate > 0 && delta >= chrono::duration<double>(FLAGS_perf_log_rate)) {
    auto fdelta = chrono::duration<float>(delta).count();
    ML_LOG(Debug, "%.4f s/frame (fps: %.4f)", fdelta / num_frames_, num_frames_ / fdelta);
    num_frames_ = 0;
    prev_update_perf_log_ += delta;
  }

  OnUpdate(delta_time.count());
  prev_update_time_ = update_time;
}

void Application::Render() {
  // Trivial BFS traversal, no partition
  VisitNode(root_);
  queue_.push(root_);
  while (!queue_.empty()) {
    auto current_node = queue_.front();
    auto children = current_node->GetChildren();
    for (auto child : children) {
      VisitNode(child);
      queue_.push(child);
    }
    queue_.pop();
  }

  MLGraphicsFrameInfo frame_info = {};
  MLGraphicsFrameInfoInit(&frame_info);
  MLResult out_result = MLGraphicsBeginFrameEx(graphics_client_, &frame_params_, &frame_info);
  if (MLResult_Pending == out_result || MLResult_Timeout == out_result) {
    ++dropped_frames_;
  } else if (MLResult_Ok == out_result) {
    frame_handle_ = frame_info.handle;
    UpdateMLCamera(frame_info);
    renderer_->Render();

    for (int i = 0; i < camera_nodes_.size(); ++i) {
      MLGraphicsSignalSyncObjectGL(graphics_client_, ml_sync_objs_[i]);
    }
    UNWRAP_MLRESULT(MLGraphicsEndFrame(graphics_client_, frame_handle_));
    graphics_context_->SwapBuffers();

    auto now = chrono::steady_clock::now();
    if (FLAGS_perf_log_rate > 0 && now - prev_gfx_perf_log_ >= chrono::duration<double>(FLAGS_perf_log_rate)) {
      MLGraphicsClientPerformanceInfo perf_info = {};
      MLGraphicsGetClientPerformanceInfo(graphics_client_, &perf_info);
      constexpr float ns_per_s = 1e9f;

      ML_LOG(Debug, "framerate: %.4f fps", ns_per_s / perf_info.frame_start_cpu_frame_start_cpu_ns);
      ML_LOG(Debug, "frame_start_cpu_comp_acquire_cpu: %.4fs", perf_info.frame_start_cpu_comp_acquire_cpu_ns / ns_per_s);
      ML_LOG(Debug, "frame_start_cpu_frame_end_gpu: %.4fs", perf_info.frame_start_cpu_frame_end_gpu_ns / ns_per_s);
      ML_LOG(Debug, "frame_start_cpu_frame_start_cpu: %.4fs", perf_info.frame_start_cpu_frame_start_cpu_ns / ns_per_s);
      ML_LOG(Debug, "frame_duration_cpu: %.4fs", perf_info.frame_duration_cpu_ns / ns_per_s);
      ML_LOG(Debug, "frame_duration_gpu: %.4fs", perf_info.frame_duration_gpu_ns / ns_per_s);
      ML_LOG(Debug, "frame_internal_duration_cpu: %.4fs", perf_info.frame_internal_duration_cpu_ns / ns_per_s);
      ML_LOG(Debug, "frame_internal_duration_gpu: %.4fs", perf_info.frame_internal_duration_gpu_ns / ns_per_s);

      prev_gfx_perf_log_ = now;
    }
  } else {
    UNWRAP_MLRESULT(out_result);
  }
  renderer_->ClearQueues();
}

void Application::SyncAppStatus() {
  // Check for exit signals
  bool windowShouldClose = false;
#if !ML_LUMIN
  if (graphics_context_ && graphics_context_->GetDisplayHandle()) {
    windowShouldClose = glfwWindowShouldClose((GLFWwindow *)graphics_context_->GetDisplayHandle());
  }
#endif

  if (exit_signal_ || windowShouldClose) {
    app_status_ = ApplicationStatus::Stopped;
    return;
  }

  // Read Lifecycle's current state
  ApplicationStatus app_status_new = lifecycle_status_;
  if (app_status_ == app_status_new) {
    return;
  }

  app_status_ = app_status_new;
  if (app_status_ == ApplicationStatus::Running) {
    OnResume();
  } else if (app_status_ == ApplicationStatus::Paused) {
    OnPause();
  }
}

void Application::Sleep() {
  ML_LOG(Verbose, "Start sleeping the main loop.");
  std::unique_lock<std::mutex> locker(sleep_mutex_);
  interrupt_sleep_ = false;
  sleep_cv_.wait(locker, [=] { return interrupt_sleep_; });
  ML_LOG(Verbose, "Start running the main loop.");
}

void Application::WakeUp() {
  {
    std::lock_guard<std::mutex> locker(sleep_mutex_);
    interrupt_sleep_ = true;
  }
  ML_LOG(Verbose, "Waking up the main loop.");
  sleep_cv_.notify_one();
}

void Application::ParseArgs(int argc, char **argv) {
#ifdef ML_LUMIN
  (void)argc;
  (void)argv;

  // get the list of arg uri via lifecycle
  lifecycle_startup_arg_uri_ = GetLifecycleArgUri();

  // insert the program name at the beginning
  std::vector<std::string> arg_vector{std::string(lifecycle_info_->package_name)};
  for (auto &arg_uri : lifecycle_startup_arg_uri_) {
    std::vector<std::string> arg_uri_split = cli_args_parser::GetArgs(arg_uri.c_str());
    arg_vector.insert(arg_vector.end(), arg_uri_split.begin(), arg_uri_split.end());
  }

  cli_args_parser::ParseGflags(arg_vector);
#else
  cli_args_parser::ParseGflags(argc, argv);
#endif  // #ifdef ML_LUMIN
}

void Application::VisitNode(std::shared_ptr<Node> node) {
  renderer_->Visit(node);
}

void Application::RunApp() {
  using namespace ml::app_framework;
  // Set the app status to running.
  // lifecycle_status_ gets updated by the Lifecycle callback function and
  // app_status_ gets synced to lifecycle_status_ in Application::Update().
  app_status_ = ApplicationStatus::Running;
  lifecycle_status_ = ApplicationStatus::Running;

  Initialize();
  OnStart();
  ML_LOG(Verbose, "Start loop.");
  prev_update_time_ = ApplicationClock::now();
  while (true) {
    SyncAppStatus();
    if (app_status_ == ApplicationStatus::Stopped) {
      OnStop();
      break;
    }
    if (app_status_ == ApplicationStatus::Paused) {
      Sleep();
      continue;
    }

    Update();
    if (use_gfx_) {
      Render();
    }
  }
  ML_LOG(Verbose, "End loop. Total dropped frames: %zu", dropped_frames_);

  Terminate();
}

void Application::StopApp() {
  exit_signal_ = true;
  WakeUp();
}

void Application::InternalRenderCamCallback(std::shared_ptr<CameraComponent> cam) {
  OnRenderCamera(cam);
}

void Application::UpdateMLCamera(const MLGraphicsFrameInfo &frame_info) {
  using namespace ml::app_framework;

  for (uint32_t camera_index = 0; camera_index < frame_info.num_virtual_cameras; ++camera_index) {
    std::shared_ptr<CameraComponent> cam = camera_nodes_[camera_index]->GetComponent<CameraComponent>();

    const MLRectf &viewport = frame_info.viewport;
    cam->SetRenderTarget(ml_render_target_cache_[std::make_pair(frame_info.color_id, camera_index)]);

#ifndef ML_LUMIN
    if (FLAGS_mirror_window && camera_index == 1) {
      // add a blit target, only for one eye
      auto dims = graphics_context_->GetFramebufferDimensions();
      if (!default_render_target_) {
        default_render_target_ = std::make_shared<RenderTarget>(dims.first, dims.second);
      }
      default_render_target_->SetWidth(dims.first);
      default_render_target_->SetHeight(dims.second);
      cam->SetBlitTarget(default_render_target_);
    }
#endif  // #ifndef ML_LUMIN

    cam->SetViewport(glm::vec4((GLint)viewport.x, (GLint)viewport.y, (GLsizei)viewport.w, (GLsizei)viewport.h));

    auto &current_camera = frame_info.virtual_cameras[camera_index];
    auto proj = glm::make_mat4(current_camera.projection.matrix_colmajor);
    cam->SetProjectionMatrix(proj);
    camera_nodes_[camera_index]->SetWorldRotation(glm::make_quat(current_camera.transform.rotation.values));
    camera_nodes_[camera_index]->SetWorldTranslation(glm::make_vec3(current_camera.transform.position.values));
    ml_sync_objs_[camera_index] = current_camera.sync_object;

    if (camera_index == 1) {
      // Light will follow one eye
      light_node_->SetWorldTranslation(glm::make_vec3(current_camera.transform.position.values));
    }
  }
}

void Application::UpdateFrameParams(const MLGraphicsFrameParamsEx &frame_params) {
  frame_params_ = frame_params;
}

}  // namespace app_framework
}  // namespace ml
