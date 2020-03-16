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
#include <app_framework/convert.h>
#include <app_framework/ml_macros.h>
#include <app_framework/toolset.h>

#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>

#include <ml_head_tracking.h>
#include <ml_logging.h>
#include <ml_perception.h>
#include <ml_persistent_coordinate_frames.h>
#include <ml_privileges.h>

#include <array>
#include <cinttypes>
#include <iomanip>
#include <sstream>

static constexpr size_t kMaxPCFCount = 100;

class PcfApp : public ml::app_framework::Application {
public:
  void OnStart() override {
    RequestPrivileges();
    for (auto &pcf : pcfs_) {
      pcf.cube = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Cube);
      pcf.text = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Text);
      GetRoot()->AddChild(pcf.cube);
      GetRoot()->AddChild(pcf.text);
    }

    // The closest PCF will be outlined with a blue cube.
    closest_pcf_cube_ = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Cube);
    auto closest_pcf_renderable = closest_pcf_cube_->GetComponent<ml::app_framework::RenderableComponent>();
    auto closest_pcf_material = std::static_pointer_cast<ml::app_framework::FlatMaterial>(closest_pcf_renderable->GetMaterial());
    closest_pcf_material->SetOverrideVertexColor(true);
    closest_pcf_material->SetColor(glm::vec4(.0f, .0f, 1.0f, 1.0f));
    closest_pcf_cube_->SetLocalScale(glm::vec3(0.3f, 0.3f, 0.3f));
    GetRoot()->AddChild(closest_pcf_cube_);

    UNWRAP_MLRESULT(MLHeadTrackingCreate(&head_tracker_));
    UNWRAP_MLRESULT(MLHeadTrackingGetStaticData(head_tracker_, &head_static_data_));
    UNWRAP_MLPASSABLE_WORLD_RESULT(MLPersistentCoordinateFrameTrackerCreate(&pcf_tracker_));
  }

  void OnStop() override {
    UNWRAP_MLRESULT(MLHeadTrackingDestroy(head_tracker_));
    UNWRAP_MLPASSABLE_WORLD_RESULT(MLPersistentCoordinateFrameTrackerDestroy(pcf_tracker_));
    MLPrivilegesShutdown();
  }

  void OnUpdate(float delta_time_scale) override {
    ResetPcfs();

    MLSnapshot *snapshot = nullptr;
    UNWRAP_MLRESULT(MLPerceptionGetSnapshot(&snapshot));

    uint32_t pcf_count = 0;
    MLResult result = MLPersistentCoordinateFrameGetCount(pcf_tracker_, &pcf_count);
    pcf_count = std::min(pcf_count, static_cast<uint32_t>(kMaxPCFCount));

    if (result == MLResult_Ok) {
      MLTransform head_transform = {};
      UNWRAP_MLRESULT(MLSnapshotGetTransform(snapshot, &head_static_data_.coord_frame_head, &head_transform));

      MLCoordinateFrameUID closest_pcf_cfuid = {};

      UNWRAP_MLPASSABLE_WORLD_RESULT(
          MLPersistentCoordinateFrameGetClosest(pcf_tracker_, &head_transform.position, &closest_pcf_cfuid));

      MLTransform closest_pcf_transform = {};
      UNWRAP_MLRESULT(MLSnapshotGetTransform(snapshot, &closest_pcf_cfuid, &closest_pcf_transform));
      closest_pcf_cube_->GetComponent<ml::app_framework::RenderableComponent>()->SetVisible(true);
      closest_pcf_cube_->SetWorldTranslation(ml::app_framework::to_glm(closest_pcf_transform.position));
      closest_pcf_cube_->SetWorldRotation(ml::app_framework::to_glm(closest_pcf_transform.rotation));

      static std::array<MLCoordinateFrameUID, kMaxPCFCount> pcf_uids;
      UNWRAP_MLPASSABLE_WORLD_RESULT(MLPersistentCoordinateFrameGetAllEx(pcf_tracker_, pcf_count, pcf_uids.data()));

      for (size_t i = 0; i < pcf_count; ++i) {
        MLTransform pcf_transform = {};
        UNWRAP_MLRESULT(MLSnapshotGetTransform(snapshot, &pcf_uids[i], &pcf_transform));

        auto pcf_position = ml::app_framework::to_glm(pcf_transform.position);
        pcfs_[i].cube->GetComponent<ml::app_framework::RenderableComponent>()->SetVisible(true);
        pcfs_[i].cube->SetWorldTranslation(pcf_position);
        pcfs_[i].cube->SetWorldRotation(ml::app_framework::to_glm(pcf_transform.rotation));
        pcfs_[i].cube->SetLocalScale(glm::vec3(0.2f, 0.2f, 0.2f));

        pcfs_[i].text->SetWorldTranslation(pcf_position);
        pcfs_[i].text->GetComponent<ml::app_framework::RenderableComponent>()->SetVisible(true);

        std::stringstream ss;
        ss << "pcf #" << i + 1 << "/" << pcf_count << std::endl;
        ss << "cfuid = " << ml::app_framework::to_string(pcf_uids[i]);
        pcfs_[i].text->GetComponent<ml::app_framework::TextComponent>()->SetText(ss.str().c_str(), 0.5f, 0.5f);
      }

      UNWRAP_MLRESULT(MLPerceptionReleaseSnapshot(snapshot));
    } else {
      ML_LOG(Warning, "(%X)%s", result, MLPersistentCoordinateFrameGetResultString(result));
    }
  }

  void ResetPcfs() {
    closest_pcf_cube_->GetComponent<ml::app_framework::RenderableComponent>()->SetVisible(false);
    for (auto &pcf : pcfs_) {
      pcf.cube->GetComponent<ml::app_framework::RenderableComponent>()->SetVisible(false);
      pcf.text->GetComponent<ml::app_framework::RenderableComponent>()->SetVisible(false);
    }
  }

private:
  void RequestPrivileges() {
    UNWRAP_MLRESULT(MLPrivilegesStartup());

    MLResult result = MLPrivilegesRequestPrivilege(MLPrivilegeID_PwFoundObjRead);
    switch (result) {
      case MLPrivilegesResult_Granted: ML_LOG(Info, "Successfully acquired the PwFoundObjRead Privilege"); break;
      case MLResult_UnspecifiedFailure: ML_LOG(Fatal, "Error requesting privileges, exiting the app."); break;
      case MLPrivilegesResult_Denied:
        ML_LOG(Fatal, "PwFoundObjRead privilege denied, exiting the app. Error code: %s.", MLPrivilegesGetResultString(result));
        break;
      default: ML_LOG(Fatal, "Unknown privilege error, exiting the app."); break;
    }
  }

  struct PCFVisuals {
    std::shared_ptr<ml::app_framework::Node> cube;
    std::shared_ptr<ml::app_framework::Node> text;
  };
  std::array<PCFVisuals, kMaxPCFCount> pcfs_;
  std::shared_ptr<ml::app_framework::Node> closest_pcf_cube_;

  MLHandle pcf_tracker_ = ML_INVALID_HANDLE;
  MLHandle head_tracker_ = ML_INVALID_HANDLE;
  MLHeadTrackingStaticData head_static_data_ = {};
};

int main() {
  PcfApp app;
  app.RunApp();
  return 0;
}
