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
#include <app_framework/cli_args_parser.h>
#include <app_framework/graphics_context.h>
#include <app_framework/ml_macros.h>

#include <gflags/gflags.h>
#include <glad/glad.h>

#include <ml_graphics.h>
#include <ml_head_tracking.h>
#include <ml_lifecycle.h>
#include <ml_logging.h>
#include <ml_perception.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

DEFINE_bool(TestBool, true, "gflags test bool");
DEFINE_int32(TestInt32, 1, "gflags test int32");

enum class AppStatus { Stopped, Paused, Running };

// Callbacks
static void OnStop(void *application_context) {
  auto status = reinterpret_cast<std::atomic<AppStatus> *>(application_context);
  *status = AppStatus::Stopped;
  ML_LOG(Info, "On stop called.");
}

static void OnPause(void *application_context) {
  auto status = reinterpret_cast<std::atomic<AppStatus> *>(application_context);
  *status = AppStatus::Paused;
  ML_LOG(Info, "On pause called.");
}

static void OnResume(void *application_context) {
  auto status = reinterpret_cast<std::atomic<AppStatus> *>(application_context);
  *status = AppStatus::Running;
  ML_LOG(Info, "On resume called.");
}

const std::vector<std::string> GetLifecycleArgUri() {
  std::vector<std::string> lifecycle_arg_uri;
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
            lifecycle_arg_uri.push_back(std::string(uri));
          }
        }
      }
    }
    MLLifecycleFreeInitArgList(&list);
  }
  return lifecycle_arg_uri;
}

void ParseArgs(int argc, char **argv, const char *package_name) {
#ifdef ML_LUMIN
  (void)argc;
  (void)argv;

  // get the list of args uri via lifecycle
  auto &lifecycle_startup_arg_uri = GetLifecycleArgUri();

  // insert the program name at the beginning
  std::vector<std::string> arg_vector{std::string(package_name)};
  for (auto &arg_uri : lifecycle_startup_arg_uri) {
    std::vector<std::string> arg_uri_split = ml::app_framework::cli_args_parser::GetArgs(arg_uri.c_str());
    arg_vector.insert(arg_vector.end(), arg_uri_split.begin(), arg_uri_split.end());
  }

  ml::app_framework::cli_args_parser::ParseGflags(arg_vector);
#else
  ml::app_framework::cli_args_parser::ParseGflags(argc, argv);
#endif  // #ifdef ML_LUMIN
}

int main(int argc, char **argv) {
  MLLifecycleCallbacksEx lifecycle_callbacks = {};
  MLLifecycleCallbacksExInit(&lifecycle_callbacks);
  lifecycle_callbacks.on_stop = OnStop;
  lifecycle_callbacks.on_pause = OnPause;
  lifecycle_callbacks.on_resume = OnResume;

  std::atomic<AppStatus> status(AppStatus::Running);
  MLLifecycleSelfInfo *self_info;

  UNWRAP_MLRESULT(MLLifecycleInitEx(&lifecycle_callbacks, (void *)&status));
  UNWRAP_MLRESULT(MLLifecycleGetSelfInfo(&self_info));

  ParseArgs(argc, argv, self_info->package_name);
  ML_LOG(Info, "gflags: TestBool: %d TestInt32: %d", FLAGS_TestBool, FLAGS_TestInt32);

  ML_LOG(Info, "-- Lifecycle Info --");
  ML_LOG(Info, "writable_dir_path: %s", self_info->writable_dir_path);
  ML_LOG(Info, "package_dir_path: %s", self_info->package_dir_path);
  ML_LOG(Info, "package_name: %s", self_info->package_name);
  ML_LOG(Info, "component_name: %s", self_info->component_name);
  ML_LOG(Info, "tmp_dir_path: %s", self_info->tmp_dir_path);
  ML_LOG(Info, "visible_name: %s", self_info->visible_name);
  ML_LOG(Info, "writable_dir_path_locked_and_unlocked: %s", self_info->writable_dir_path_locked_and_unlocked);

  // initialize perception system
  MLPerceptionSettings perception_settings;
  UNWRAP_MLRESULT(MLPerceptionInitSettings(&perception_settings));
  UNWRAP_MLRESULT(MLPerceptionStartup(&perception_settings));

  // Now that graphics is connected, the app is ready to go
  UNWRAP_MLRESULT(MLLifecycleSetReadyIndication());

  MLHandle head_tracker;
  UNWRAP_MLRESULT(MLHeadTrackingCreate(&head_tracker));
  MLHeadTrackingStaticData head_static_data;
  UNWRAP_MLRESULT(MLHeadTrackingGetStaticData(head_tracker, &head_static_data));

  ML_LOG(Info, "Start loop.");
  std::ofstream headpose_file;
  std::string path = std::string(self_info->writable_dir_path) + std::string("headpose.txt");
  ML_LOG(Info, "path: %s", path.c_str());
  headpose_file.open(path);

  auto start = std::chrono::steady_clock::now();
  MLSnapshot *snapshot = nullptr;
  MLTransform head_transform = {};

  while (AppStatus::Stopped != status) {
    if (AppStatus::Paused == status) {
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    auto msRuntime =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

    if (msRuntime > 2000) {
      UNWRAP_MLRESULT(MLPerceptionGetSnapshot(&snapshot));
      UNWRAP_MLRESULT(MLSnapshotGetTransform(snapshot, &head_static_data.coord_frame_head, &head_transform));
      ML_LOG(Info, "Writing headpose position (%f,%f,%f) to file.",
             head_transform.position.x,
             head_transform.position.y,
             head_transform.position.z);

      headpose_file << "(" << head_transform.position.x << ","
                    << head_transform.position.y << ","
                    << head_transform.position.z << ")" << std::endl;

      UNWRAP_MLRESULT(MLPerceptionReleaseSnapshot(snapshot));
      start = std::chrono::steady_clock::now();
    }
  }

  headpose_file.close();

  // clean up system
  UNWRAP_MLRESULT(MLPerceptionShutdown());
  UNWRAP_MLRESULT(MLLifecycleFreeSelfInfo(&self_info));

  return 0;
}
