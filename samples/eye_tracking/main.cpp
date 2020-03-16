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

#include <gflags/gflags.h>

#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

#include <ml_eye_tracking.h>
#include <ml_head_tracking.h>
#include <ml_logging.h>
#include <ml_perception.h>

DEFINE_bool(draw_fixation_point, true, "Draw the fixation point.");
DEFINE_bool(draw_eye_centers, true, "Draw the eye centers.");
DEFINE_bool(draw_gaze_left, false, "Draw left gaze vector.");
DEFINE_bool(draw_gaze_right, false, "Draw right gaze vector.");
DEFINE_bool(eye_centers_origin, true,
            "If true, eye centers are drawn relative to the origin; otherwise, the world space position is used.");

const glm::vec3 kCubeScale = glm::vec3(0.01f, 0.01f, 0.01f);
const glm::vec3 kEyeCenterScale = glm::vec3(0.02f, 0.02f, 0.02f);

class EyeTrackingApp : public ml::app_framework::Application {
public:
  EyeTrackingApp(int argc = 0, char **argv = nullptr) : ml::app_framework::Application(argc, argv) {}

  void OnStart() override {
    UNWRAP_MLRESULT(MLHeadTrackingCreate(&head_tracker_));
    UNWRAP_MLRESULT(MLHeadTrackingGetStaticData(head_tracker_, &head_static_data_));

    UNWRAP_MLRESULT(MLEyeTrackingCreate(&eye_tracker_));
    UNWRAP_MLRESULT_FATAL(MLEyeTrackingGetStaticData(eye_tracker_, &eye_static_data_));

    fixation_node_ = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Cube);
    left_center_node_ = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Cube);
    right_center_node_ = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Cube);
    left_gaze_node_ = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Line);
    right_gaze_node_ = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Line);
    left_gaze_point_node_ = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Cube);
    right_gaze_point_node_ = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Cube);

    GetRoot()->AddChild(fixation_node_);
    GetRoot()->AddChild(left_center_node_);
    GetRoot()->AddChild(right_center_node_);
    GetRoot()->AddChild(left_gaze_node_);
    GetRoot()->AddChild(right_gaze_node_);
    GetRoot()->AddChild(left_gaze_point_node_);
    GetRoot()->AddChild(right_gaze_point_node_);
    fixation_node_->SetLocalScale(kCubeScale);
    left_center_node_->SetLocalScale(kEyeCenterScale);
    right_center_node_->SetLocalScale(kEyeCenterScale);
    left_gaze_point_node_->SetLocalScale(kEyeCenterScale);
    right_gaze_point_node_->SetLocalScale(kEyeCenterScale);
  }

  void OnStop() override {
    UNWRAP_MLRESULT(MLHeadTrackingDestroy(head_tracker_));
    UNWRAP_MLRESULT(MLEyeTrackingDestroy(eye_tracker_));
  }

  void OnUpdate(float) override {
    MLSnapshot *snapshot = nullptr;
    UNWRAP_MLRESULT(MLPerceptionGetSnapshot(&snapshot));

    MLTransform fixation = {};
    MLTransform left_eye_center = {};
    MLTransform right_eye_center = {};
    MLTransform head = {};
    UNWRAP_MLRESULT(MLSnapshotGetTransform(snapshot, &head_static_data_.coord_frame_head, &head));
    UNWRAP_MLRESULT(MLSnapshotGetTransform(snapshot, &eye_static_data_.fixation, &fixation));
    UNWRAP_MLRESULT(MLSnapshotGetTransform(snapshot, &eye_static_data_.left_center, &left_eye_center));
    UNWRAP_MLRESULT(MLSnapshotGetTransform(snapshot, &eye_static_data_.right_center, &right_eye_center));
    UNWRAP_MLRESULT(MLPerceptionReleaseSnapshot(snapshot));

    if (FLAGS_draw_fixation_point) {
      fixation_node_->SetWorldRotation(ml::app_framework::to_glm(fixation.rotation));
      fixation_node_->SetWorldTranslation(ml::app_framework::to_glm(fixation.position));
    }

    if (FLAGS_eye_centers_origin) {
      //
      // This case draws the eye centers relative to the
      // origin. When running on device this would allow you to move
      // back from the origin and see the eye centers versus them
      // being within your head and not able to see them.
      //
      left_center_node_->SetWorldRotation(ml::app_framework::to_glm(left_eye_center.rotation));
      left_center_node_->SetWorldTranslation(ml::app_framework::to_glm(left_eye_center.position) - ml::app_framework::to_glm(head.position));
      right_center_node_->SetWorldRotation(ml::app_framework::to_glm(right_eye_center.rotation));
      right_center_node_->SetWorldTranslation(ml::app_framework::to_glm(right_eye_center.position) - ml::app_framework::to_glm(head.position));
    } else {
      //
      // This case is more useful on MLRemote where you could change
      // the world space position of the eye centers independently
      // of the headpose.  This would also be useful on device if
      // you wanted to log these positions to make sure that the eye
      // center values are correct.
      //
      left_center_node_->SetWorldRotation(ml::app_framework::to_glm(left_eye_center.rotation));
      left_center_node_->SetWorldTranslation(ml::app_framework::to_glm(left_eye_center.position));
      right_center_node_->SetWorldRotation(ml::app_framework::to_glm(right_eye_center.rotation));
      right_center_node_->SetWorldTranslation(ml::app_framework::to_glm(right_eye_center.position));
    }

    if (FLAGS_draw_gaze_left) {
      auto left_origin = ml::app_framework::to_glm(left_eye_center.position);
      auto left_direction = ml::app_framework::to_glm(left_eye_center.rotation) * glm::vec3(0, 0, -1);
      glm::vec3 left_gaze_line[] = {
        left_origin,
        left_origin + left_direction
      };
      auto left_line_renderable = left_gaze_node_->GetComponent<ml::app_framework::RenderableComponent>();
      left_line_renderable->GetMesh()->SetPrimitiveType(GL_LINES);
      left_line_renderable->GetMesh()->UpdateMesh(left_gaze_line, nullptr, sizeof(left_gaze_line)/sizeof(glm::vec3), nullptr, 0);
      auto left_line_material = std::static_pointer_cast<ml::app_framework::FlatMaterial>(left_line_renderable->GetMaterial());
      left_line_material->SetColor(glm::vec4(.0f, .0f, 1.0f, 1.0f));

      left_gaze_point_node_->SetWorldTranslation(left_origin + left_direction);
      left_gaze_point_node_->SetWorldRotation(ml::app_framework::to_glm(left_eye_center.rotation));
    }

    if (FLAGS_draw_gaze_right) {
      auto right_origin = ml::app_framework::to_glm(right_eye_center.position);
      auto right_direction = ml::app_framework::to_glm(right_eye_center.rotation) * glm::vec3(0, 0, -1);
      glm::vec3 right_gaze_line[] = {
        right_origin,
        right_origin + right_direction
      };
      auto right_line_renderable = right_gaze_node_->GetComponent<ml::app_framework::RenderableComponent>();
      right_line_renderable->GetMesh()->SetPrimitiveType(GL_LINES);
      right_line_renderable->GetMesh()->UpdateMesh(right_gaze_line, nullptr, sizeof(right_gaze_line)/sizeof(glm::vec3), nullptr, 0);
      auto right_line_material = std::static_pointer_cast<ml::app_framework::FlatMaterial>(right_line_renderable->GetMaterial());
      right_line_material->SetColor(glm::vec4(.0f, .0f, 1.0f, 1.0f));

      right_gaze_point_node_->SetWorldTranslation(right_origin + right_direction);
      right_gaze_point_node_->SetWorldRotation(ml::app_framework::to_glm(right_eye_center.rotation));
    }
  }

private:
  std::shared_ptr<ml::app_framework::Node> fixation_node_;
  std::shared_ptr<ml::app_framework::Node> left_center_node_;
  std::shared_ptr<ml::app_framework::Node> right_center_node_;
  std::shared_ptr<ml::app_framework::Node> left_gaze_node_;
  std::shared_ptr<ml::app_framework::Node> right_gaze_node_;
  std::shared_ptr<ml::app_framework::Node> left_gaze_point_node_;
  std::shared_ptr<ml::app_framework::Node> right_gaze_point_node_;

  MLHandle head_tracker_ = ML_INVALID_HANDLE;
  MLHeadTrackingStaticData head_static_data_ = {};
  MLHandle eye_tracker_ = ML_INVALID_HANDLE;
  MLEyeTrackingStaticData eye_static_data_ = {};
};

int main(int argc, char **argv) {
  EyeTrackingApp app(argc, argv);
  app.RunApp();
  return 0;
}
