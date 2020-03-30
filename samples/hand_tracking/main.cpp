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
#include <app_framework/gui.h>
#include <app_framework/ml_macros.h>
#include <app_framework/toolset.h>

#include <gflags/gflags.h>

#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui.h>

#include <ml_controller.h>
#include <ml_hand_tracking.h>
#include <ml_head_tracking.h>
#include <ml_perception.h>

#include <array>
#include <iomanip>
#include <sstream>
#include <string>

DEFINE_bool(handtracking_pipeline_enabled, true,
            "Enables or disables hand tracking. Disabling this causes no hand recognition to take place.");
DEFINE_int32(keypoint_filter_level, 2,
             "\n0 - Raw\n1 - Smoothing at the cost of latency\n2 - Predictive smoothing at the cost of latency");
DEFINE_int32(
    pose_filter_level, 2,
    "\n0 - Raw\n1 - Robustness to flicker at the cost of latency\n2 - More robust to flicker at the cost of latency");
DEFINE_bool(finger, true, "Enable/Disable hand pose - finger");
DEFINE_bool(fist, true, "Enable/Disable hand pose - fist");
DEFINE_bool(pinch, true, "Enable/Disable hand pose - pinch");
DEFINE_bool(thumb, true, "Enable/Disable hand pose - thumb");
DEFINE_bool(l, true, "Enable/Disable hand pose - l");
DEFINE_bool(openhand, true, "Enable/Disable hand pose - openhand");
DEFINE_bool(ok, true, "Enable/Disable hand pose - ok");
DEFINE_bool(c, true, "Enable/Disable hand pose - c");
DEFINE_bool(nopose, true, "Enable/Disable hand pose - nopose");

const glm::vec3 kKeypointCubeScale = glm::vec3(.01f, .01f, .01f);

class HandTrackingApp : public ml::app_framework::Application {
public:
  HandTrackingApp(int argc = 0, char **argv = nullptr) : ml::app_framework::Application(argc, argv) {}

  void OnStart() override {
    UNWRAP_MLRESULT(MLHeadTrackingCreate(&head_tracker_));
    UNWRAP_MLRESULT(MLHeadTrackingGetStaticData(head_tracker_, &head_static_data_));
    UNWRAP_MLRESULT(MLHandTrackingCreate(&hand_tracker_));
    controller_configuration_ = {false, true, true};  // enable 6dof
    UNWRAP_MLRESULT(MLControllerCreateEx(&controller_configuration_, &controller_tracker_));

    memset(&config_, 0, sizeof(MLHandTrackingConfiguration));
    config_.handtracking_pipeline_enabled = FLAGS_handtracking_pipeline_enabled;
    config_.keypoints_filter_level = (MLKeypointFilterLevel)FLAGS_keypoint_filter_level;
    config_.pose_filter_level = (MLPoseFilterLevel)FLAGS_pose_filter_level;

    for (int i = 0; i < (MLHandTrackingKeyPose_Count - 1); ++i) {
      config_.keypose_config[i] = true;
    }

    config_.keypose_config[MLHandTrackingKeyPose_Finger] = FLAGS_finger;
    config_.keypose_config[MLHandTrackingKeyPose_Fist] = FLAGS_fist;
    config_.keypose_config[MLHandTrackingKeyPose_Pinch] = FLAGS_pinch;
    config_.keypose_config[MLHandTrackingKeyPose_Thumb] = FLAGS_thumb;
    config_.keypose_config[MLHandTrackingKeyPose_L] = FLAGS_l;
    config_.keypose_config[MLHandTrackingKeyPose_OpenHand] = FLAGS_openhand;
    config_.keypose_config[MLHandTrackingKeyPose_Ok] = FLAGS_ok;
    config_.keypose_config[MLHandTrackingKeyPose_C] = FLAGS_c;
    config_.keypose_config[MLHandTrackingKeyPose_NoPose] = FLAGS_nopose;

    UNWRAP_MLRESULT(MLHandTrackingSetConfiguration(hand_tracker_, &config_));
    UNWRAP_MLRESULT_FATAL(MLHandTrackingGetStaticData(hand_tracker_, &hand_static_data_));

    for (uint32_t i = 0; i < MLHandTrackingStaticData_MaxKeyPoints; ++i) {
      if (hand_static_data_.left_frame[i].is_valid) {
        left_keypoints_[i] = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Cube);
        GetRoot()->AddChild(left_keypoints_[i]);
      }
      if (hand_static_data_.right_frame[i].is_valid) {
        right_keypoints_[i] = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Cube);
        GetRoot()->AddChild(right_keypoints_[i]);
      }
    }

    l_pose_text_ = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Text);
    r_pose_text_ = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Text);
    GetRoot()->AddChild(l_pose_text_);
    GetRoot()->AddChild(r_pose_text_);

    ml::app_framework::Gui::GetInstance().Initialize(GetGraphicsContext());
    GetRoot()->AddChild(ml::app_framework::Gui::GetInstance().GetNode());
  }

  void OnStop() override {
    ml::app_framework::Gui::GetInstance().Cleanup();
    UNWRAP_MLRESULT(MLControllerDestroy(controller_tracker_));
    UNWRAP_MLRESULT(MLHandTrackingDestroy(hand_tracker_));
    UNWRAP_MLRESULT(MLHeadTrackingDestroy(head_tracker_));
  }

  void OnUpdate(float) override {
    MLHandTrackingDataEx data;
    MLHandTrackingDataExInit(&data);
    MLSnapshot *snapshot = nullptr;
    MLPerceptionGetSnapshot(&snapshot);

    MLTransform head_transform = {};
    MLSnapshotGetTransform(snapshot, &head_static_data_.coord_frame_head, &head_transform);

    if (config_.handtracking_pipeline_enabled) {
      ResetDrawFlags();
      MLResult d = MLHandTrackingGetDataEx(hand_tracker_, &data);
      if (d != MLResult_Ok) {
        return;
      }

      for (uint32_t i = 0; i < MLHandTrackingStaticData_MaxKeyPoints; ++i) {
        if (data.left_hand_state.keypose != MLHandTrackingKeyPose_NoHand) {
          if (hand_static_data_.left_frame[i].is_valid && data.left_hand_state.keypoints_mask[i]) {
            left_draw_[i] = true;
            left_keypoints_[i]->GetComponent<ml::app_framework::RenderableComponent>()->SetVisible(true);
          }
        }

        if (data.right_hand_state.keypose != MLHandTrackingKeyPose_NoHand) {
          if (hand_static_data_.right_frame[i].is_valid && data.right_hand_state.keypoints_mask[i]) {
            right_draw_[i] = true;
            right_keypoints_[i]->GetComponent<ml::app_framework::RenderableComponent>()->SetVisible(true);
          }
        }
      }

      // Get the transforms for valid keypoints
      for (int i = 0; i < MLHandTrackingStaticData_MaxKeyPoints; ++i) {
        MLTransform lt = {};
        MLTransform rt = {};
        if (left_draw_[i]) {
          MLSnapshotGetTransform(snapshot, &hand_static_data_.left_frame[i].frame_id, &lt);
          left_keypoints_[i]->SetWorldTranslation(ml::app_framework::to_glm(lt.position));
          left_keypoints_[i]->SetWorldRotation(ml::app_framework::to_glm(lt.rotation));
          left_keypoints_[i]->SetLocalScale(kKeypointCubeScale);
        }

        if (right_draw_[i]) {
          MLSnapshotGetTransform(snapshot, &hand_static_data_.right_frame[i].frame_id, &rt);
          right_keypoints_[i]->SetWorldTranslation(ml::app_framework::to_glm(rt.position));
          right_keypoints_[i]->SetWorldRotation(ml::app_framework::to_glm(rt.rotation));
          right_keypoints_[i]->SetLocalScale(kKeypointCubeScale);
        }
      }

      // Get the transforms for hand centers for positioning of the text
      MLTransform l_hand_center = {};
      MLTransform r_hand_center = {};
      MLSnapshotGetTransform(snapshot, &hand_static_data_.left.hand_center.frame_id, &l_hand_center);
      MLSnapshotGetTransform(snapshot, &hand_static_data_.right.hand_center.frame_id, &r_hand_center);
      MLPerceptionReleaseSnapshot(snapshot);

      std::stringstream label;

      // Set the text for the pose and confidence
      label << "Left:" << GetHandPoseLabel(data.left_hand_state.keypose) << std::endl
            << "Hold Control: " << (data.left_hand_state.is_holding_control ? "True" : "False") << std::endl
            << "Hand Confidence: " << std::fixed << std::setprecision(2) << data.left_hand_state.hand_confidence;
      l_pose_text_->GetComponent<ml::app_framework::TextComponent>()->SetText(label.str().c_str(), 0.5f, 0.5f);
      label.str("");

      label << "Right:" << GetHandPoseLabel(data.right_hand_state.keypose) << std::endl
            << "Hold Control: " << (data.right_hand_state.is_holding_control ? "True" : "False") << std::endl
            << "Hand Confidence: " << std::fixed << std::setprecision(2) << data.right_hand_state.hand_confidence;
      r_pose_text_->GetComponent<ml::app_framework::TextComponent>()->SetText(label.str().c_str(), 0.5f, 0.5f);
      label.str("");

      PlaceText(l_pose_text_, data.left_hand_state, l_hand_center, l_last_hand_center_, head_transform);

      PlaceText(r_pose_text_, data.right_hand_state, r_hand_center, r_last_hand_center_, head_transform);
    }

    UpdateGui(data);  // Store which keypoints to draw
  }

  void PlaceText(std::shared_ptr<ml::app_framework::Node> &pose_text, const MLHandTrackingHandStateEx &hand_state,
                 const MLTransform &hand_center, MLTransform &last_hand_center, const MLTransform &head_transform) {
    pose_text->SetLocalScale(glm::vec3(.002f, -.002f, .002f));
    if (hand_state.keypose == MLHandTrackingKeyPose_NoPose || hand_state.keypose == MLHandTrackingKeyPose_NoHand) {
      pose_text->SetWorldTranslation(ml::app_framework::to_glm(last_hand_center.position));
      glm::vec3 eyevec = ml::app_framework::to_glm(head_transform.position) - pose_text->GetWorldTranslation();
      pose_text->SetWorldRotation(glm::quat(glm::vec3(0, atan2(eyevec.x, eyevec.z), 0)));
    } else {
      pose_text->SetWorldTranslation(ml::app_framework::to_glm(hand_center.position));
      glm::vec3 eyevec = ml::app_framework::to_glm(head_transform.position) - pose_text->GetWorldTranslation();
      pose_text->SetWorldRotation(glm::quat(glm::vec3(0, atan2(eyevec.x, eyevec.z), 0)));
      last_hand_center.position = ml::app_framework::to_ml(pose_text->GetWorldTranslation());
      last_hand_center.rotation = ml::app_framework::to_ml(pose_text->GetWorldRotation());
    }
  }

  void OnPause() override {
    if (config_.handtracking_pipeline_enabled) {
      config_.handtracking_pipeline_enabled = false;
      UNWRAP_MLRESULT(MLHandTrackingSetConfiguration(hand_tracker_, &config_));
    }
  }

  void OnResume() override {
    if (!config_.handtracking_pipeline_enabled) {
      config_.handtracking_pipeline_enabled = true;
      UNWRAP_MLRESULT(MLHandTrackingSetConfiguration(hand_tracker_, &config_));
    }
  }

private:
  std::shared_ptr<ml::app_framework::Node> l_pose_text_;
  std::shared_ptr<ml::app_framework::Node> r_pose_text_;
  MLTransform l_last_hand_center_ = {};
  MLTransform r_last_hand_center_ = {};

  std::array<std::shared_ptr<ml::app_framework::Node>, MLHandTrackingStaticData_MaxKeyPoints> left_keypoints_;
  std::array<std::shared_ptr<ml::app_framework::Node>, MLHandTrackingStaticData_MaxKeyPoints> right_keypoints_;

  std::string static_hand_poses_string[MLHandTrackingKeyPose_Count];

  MLHandle head_tracker_ = ML_INVALID_HANDLE;
  MLHandle hand_tracker_ = ML_INVALID_HANDLE;
  // The below controller tracker is instantiated in order to use and test the "hand holding controller" behavior.
  // That is, when the controller tracker is active, any hand which is holding it should have its keypose set as
  // "No Pose" and the binary flag "is_holding_control" set to true.  Without this tracker this will not work.
  // No other calls need to be made, other than creating and destroying the tracker.
  MLHandle controller_tracker_ = ML_INVALID_HANDLE;
  MLHeadTrackingStaticData head_static_data_ = {};
  MLHandTrackingStaticData hand_static_data_ = {};
  MLHandTrackingConfiguration config_;
  MLControllerConfiguration controller_configuration_;

  bool left_draw_[MLHandTrackingStaticData_MaxKeyPoints] = {false};
  bool right_draw_[MLHandTrackingStaticData_MaxKeyPoints] = {false};

  void ResetDrawFlags() {
    for (uint32_t i = 0; i < MLHandTrackingStaticData_MaxKeyPoints; ++i) {
      if (left_draw_[i]) {
        left_keypoints_[i]->GetComponent<ml::app_framework::RenderableComponent>()->SetVisible(false);
        left_draw_[i] = false;
      }

      if (right_draw_[i]) {
        right_keypoints_[i]->GetComponent<ml::app_framework::RenderableComponent>()->SetVisible(false);
        right_draw_[i] = false;
      }
    }
  }

  const char *GetHandPoseLabel(MLHandTrackingKeyPose key_pose) {
    switch (key_pose) {
      case MLHandTrackingKeyPose_Finger: return "Finger";
      case MLHandTrackingKeyPose_Fist: return "Fist";
      case MLHandTrackingKeyPose_Pinch: return "Pinch";
      case MLHandTrackingKeyPose_Thumb: return "Thumb";
      case MLHandTrackingKeyPose_L: return "L";
      case MLHandTrackingKeyPose_OpenHand: return "OpenHand";
      case MLHandTrackingKeyPose_Ok: return "Ok";
      case MLHandTrackingKeyPose_C: return "C";
      case MLHandTrackingKeyPose_NoPose: return "NoPose";
      case MLHandTrackingKeyPose_NoHand: return "NoHand";
      default: return "Invalid hand pose";
    }
  }

  void UpdateGui(const MLHandTrackingDataEx &data) {
    ml::app_framework::Gui::GetInstance().BeginUpdate();
    if (ImGui::Begin("HandTracking")) {
      if (ImGui::CollapsingHeader("MLHandTrackingConfiguration", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::TreeNodeEx("keypose_config", ImGuiTreeNodeFlags_DefaultOpen)) {
          for (int pose = 0; pose < MLHandTrackingKeyPose_Count; ++pose) {
            ImGui::Checkbox(GetHandPoseLabel((MLHandTrackingKeyPose)pose), &config_.keypose_config[pose]);
          }
          ImGui::TreePop();
        }
        ImGui::Checkbox("handtracking_pipeline_enabled", &config_.handtracking_pipeline_enabled);
        ImGui::SliderInt("keypoints_filter_level", reinterpret_cast<int *>(&config_.keypoints_filter_level), 0, 2);
        ImGui::SliderInt("pose_filter_level", reinterpret_cast<int *>(&config_.pose_filter_level), 0, 2);
      }
      if (ImGui::CollapsingHeader("MLHandTrackingData", ImGuiTreeNodeFlags_DefaultOpen)) {
        std::array<std::pair<const char *, const MLHandTrackingHandStateEx &>, 2> states = {
            {{"left_hand_state", data.left_hand_state}, {"right_hand_state", data.right_hand_state}}};
        for (const auto &pair : states) {
          const auto &state = pair.second;
          if (ImGui::BeginChild(pair.first, ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 0.f)) &&
              ImGui::CollapsingHeader(pair.first, ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("keypose: %s", GetHandPoseLabel((MLHandTrackingKeyPose)state.keypose));

            ImGui::Value("hand_confidence", state.hand_confidence, "%.2f");

            if (ImGui::TreeNodeEx("keypose_confidence")) {
              for (int pose = 0; pose < MLHandTrackingKeyPose_Count; ++pose) {
                ImGui::Value(GetHandPoseLabel((MLHandTrackingKeyPose)pose), state.keypose_confidence[pose], "%.2f");
              }
              ImGui::TreePop();
            }

            if (ImGui::TreeNodeEx("keypose_confidence_filtered")) {
              for (int pose = 0; pose < MLHandTrackingKeyPose_Count; ++pose) {
                ImGui::Value(GetHandPoseLabel((MLHandTrackingKeyPose)pose), state.keypose_confidence_filtered[pose],
                             "%.2f");
              }
              ImGui::TreePop();
            }

            if (ImGui::TreeNodeEx("keypoints_mask")) {
              for (int point = 0; point < MLHandTrackingStaticData_MaxKeyPoints; ++point) {
                ImGui::Text("Point %d: %s", point, state.keypoints_mask[point] ? "true" : "false");
              }
              ImGui::TreePop();
            }
          }
          ImGui::EndChild();
          ImGui::SameLine();
        }
      }
    }
    ImGui::End();
    ml::app_framework::Gui::GetInstance().EndUpdate();
  }
};

int main(int argc, char **argv) {
  HandTrackingApp app(argc, argv);
  app.SetUseGfx(true);
  app.RunApp();
  return 0;
}
