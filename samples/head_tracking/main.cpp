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
#include <app_framework/ml_macros.h>
#include <app_framework/toolset.h>
#include <gflags/gflags.h>

#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/gtx/transform.hpp>

#include <cstdlib>
#include <ml_head_tracking.h>
#include <ml_perception.h>

DEFINE_bool(origin, true, "Draw the origin");
DEFINE_double(cubedist, 1.5f, "Cube distance from the origin, in meters.");

class HeadTrackingApp : public ml::app_framework::Application {
public:
  HeadTrackingApp(int argc = 0, char **argv = nullptr) : ml::app_framework::Application(argc, argv) {}

  void OnStart() override {
    auto cube = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Cube);
    GetRoot()->AddChild(cube);
    cube->SetWorldTranslation(glm::vec3(0.f, 0.f, (float)-FLAGS_cubedist));

    if (FLAGS_origin) {
      auto origin_axis = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Axis);
      GetRoot()->AddChild(origin_axis);
      origin_axis->SetLocalScale(glm::vec3(.15f, .15f, .15f));
      origin_axis->SetWorldTranslation(glm::vec3(.0f, .0f, .0f));
    }

    UNWRAP_MLRESULT(MLHeadTrackingCreate(&head_tracker_));
    UNWRAP_MLRESULT(MLHeadTrackingGetStaticData(head_tracker_, &head_static_data_));
  }

  void OnUpdate(float) override {
    // Update Head tracking state
    MLHeadTrackingState cur_state;
    uint64_t cur_map_events;

    UNWRAP_MLRESULT(MLHeadTrackingGetState(head_tracker_, &cur_state));
    UNWRAP_MLRESULT(MLHeadTrackingGetMapEvents(head_tracker_, &cur_map_events));

    auto headStateModeString = [](MLHeadTrackingMode mode) -> const char * {
      switch (mode) {
        case MLHeadTrackingMode_6DOF: return "MLHeadTrackingMode_6DOF";
        case MLHeadTrackingMode_Unavailable: return "MLHeadTrackingMode_Unavailable";
        default: return "Undefined";
      }
    };
    auto headStateErrorString = [](MLHeadTrackingError error) -> const char * {
      switch (error) {
        case MLHeadTrackingError_None: return "MLHeadTrackingError_None";
        case MLHeadTrackingError_NotEnoughFeatures: return "MLHeadTrackingError_NotEnoughFeatures";
        case MLHeadTrackingError_LowLight: return "MLHeadTrackingError_LowLight";
        case MLHeadTrackingError_Unknown: return "MLHeadTrackingError_Unknown";
        default: return "Undefined";
      }
    };

    // Log only if something has changed significantly
    if (!initial_state_logged_ || prev_head_state_.mode != cur_state.mode ||
        prev_head_state_.error != cur_state.error ||
        std::fabs(prev_head_state_.confidence - cur_state.confidence) > 0.3f) {
      ML_LOG(Info, "Head tracking state: mode [ %s ] error [ %s ] confidence [ %f ]",
             headStateModeString(cur_state.mode), headStateErrorString(cur_state.error), cur_state.confidence);
      initial_state_logged_ = true;
    }
    prev_head_state_ = cur_state;

    // Log if there is a map event
    if (prev_map_events_ != cur_map_events) {
      ML_LOG(Info, "Head tracking state: map_events [ %llu ]", static_cast<unsigned long long>(cur_map_events));
    }
    prev_map_events_ = cur_map_events;

    // Exercising more of the Head Tracking API.  Grabbing the headpose transform.
    MLSnapshot *snapshot = nullptr;
    UNWRAP_MLRESULT(MLPerceptionGetSnapshot(&snapshot));

    MLTransform head_transform = {};
    UNWRAP_MLRESULT(MLSnapshotGetTransform(snapshot, &head_static_data_.coord_frame_head, &head_transform));
    UNWRAP_MLRESULT(MLPerceptionReleaseSnapshot(snapshot));
  }

  void OnStop() override {
    UNWRAP_MLRESULT(MLHeadTrackingDestroy(head_tracker_));
  }

private:
  MLHandle head_tracker_;
  MLHeadTrackingStaticData head_static_data_;

  bool initial_state_logged_ = false;
  MLHeadTrackingState prev_head_state_{};
  uint64_t prev_map_events_ = 0;
};

int main(int argc, char **argv) {
  HeadTrackingApp app(argc, argv);
  app.RunApp();
  return 0;
}
