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

#include <stb_image.h>

#include <ml_head_tracking.h>
#include <ml_privileges.h>

#include <image_tracking.h>

ImageTracking::ImageTracking(int argc, char **argv)
    : ml::app_framework::Application(argc, argv), max_targets_(targets_to_add_.size()) {}

void ImageTracking::OnStart() {
  RequestPrivileges();
  InitializeHeadTracker();
  InitializeImageTracker();
  AddImageTargets();
  InitializeGui();
  ML_LOG(Info, "Started.");
}

void ImageTracking::OnStop() {
  TerminateGui();
  RemoveImageTargets();  // Not necessary, but to make sure that it can successfully remove the image targets.
  TerminateImageTracker();
  TerminateHeadTracker();
  UNWRAP_MLRESULT(MLPrivilegesStartup());
  ML_LOG(Info, "Stopped.");
}

void ImageTracking::OnPause() {
  ML_LOG(Info, "Paused.");
  PauseTracker();
}

void ImageTracking::OnResume() {
  ML_LOG(Info, "Resumed.");
  UpdateTrackerSettings();  // Restore the settings back to how it was set before being paused.
}

void ImageTracking::OnUpdate(float) {
  ml::app_framework::Gui::GetInstance().BeginUpdate();
  UpdateTargetResults();
  UpdateGui();
  ml::app_framework::Gui::GetInstance().EndUpdate();
}

void ImageTracking::RequestPrivileges() {
  UNWRAP_MLRESULT(MLPrivilegesStartup());

  MLResult result = MLPrivilegesRequestPrivilege(MLPrivilegeID_CameraCapture);
  switch (result) {
    case MLPrivilegesResult_Granted: ML_LOG(Info, "Successfully acquired the Camera Privilege"); break;
    case MLResult_UnspecifiedFailure: ML_LOG(Fatal, "Error requesting privileges, exiting the app."); break;
    case MLPrivilegesResult_Denied:
      ML_LOG(Fatal, "Camere privilege denied, exiting the app. Error code: %s.", MLPrivilegesGetResultString(result));
      break;
    default: ML_LOG(Fatal, "Unknown privilege error, exiting the app."); break;
  }
}

void ImageTracking::InitializeHeadTracker() {
  MLHeadTrackingCreate(&head_tracker_handle_);
}

void ImageTracking::TerminateHeadTracker() {
  MLHeadTrackingDestroy(head_tracker_handle_);
}

void ImageTracking::InitializeImageTracker() {
  MLImageTrackerInitSettings(&image_tracker_setting_);
  image_tracker_setting_.enable_image_tracking = true;
  image_tracker_setting_.max_simultaneous_targets = max_targets_;

  UNWRAP_MLRESULT(MLImageTrackerCreate(&image_tracker_setting_, &image_tracker_handle_));
  LogTrackerSettings("Tracker settings", image_tracker_setting_);
}

void ImageTracking::TerminateImageTracker() {
  MLImageTrackerDestroy(image_tracker_handle_);
}

void ImageTracking::AddImageTargets() {
  for (auto &target : targets_to_add_) {
    const std::string &file_path = target.first;
    ImageTargetProperties file_properties = target.second;

    // Add a target from image file
    MLHandle target_handle = AddTarget(file_path.c_str(), file_properties);
    if (!MLHandleIsValid(target_handle)) {
      continue;
    }

    // Get information of the target
    MLImageTrackerTargetStaticData target_data;
    UNWRAP_MLRESULT(MLImageTrackerGetTargetStaticData(image_tracker_handle_, target_handle, &target_data));

    // Add target settings/information to the target list
    image_targets_[target_handle] = {
        {file_properties.settings, file_properties.width, file_properties.height}, target_data, {}};
    LogTargetSettings("Target added", file_properties.settings);

    // Add a plane to the target
    AddPlaneToTarget(target_handle);
  }
}

void ImageTracking::RemoveImageTargets() {
  for (auto &target : image_targets_) {
    MLHandle target_handle = target.first;
    ImageTargetProperties &file_properties = target.second.properties;

    MLResult result = MLImageTrackerRemoveTarget(image_tracker_handle_, target_handle);
    if (result != MLResult_Ok) {
      ML_LOG(Error, "Error removing target[%s] - %s.", file_properties.settings.name, MLGetResultString(result));
    }
  }
}

void ImageTracking::InitializeGui() {
  ml::app_framework::Gui::GetInstance().Initialize(GetGraphicsContext());
  GetRoot()->AddChild(ml::app_framework::Gui::GetInstance().GetNode());
}

void ImageTracking::TerminateGui() {
  ml::app_framework::Gui::GetInstance().Cleanup();
}

MLHandle ImageTracking::AddTarget(const std::string &path, const ImageTargetProperties &target_properties) {
  MLHandle target_handle = ML_INVALID_HANDLE;
  MLResult result = MLResult_UnspecifiedFailure;

  if (target_properties.load_data) {
    // Read the content of the image file.
    int width, height, n;
    unsigned char *image_data = stbi_load(path.c_str(), &width, &height, &n, STBI_rgb_alpha);
    if (!image_data) {
      ML_LOG(Error, "Error loading a target[%s].", path.c_str());
      return target_handle;
    }

    result = MLImageTrackerAddTargetFromArray(image_tracker_handle_, &target_properties.settings, image_data, width,
                                              height, MLImageTrackerImageFormat_RGBA, &target_handle);
    stbi_image_free(image_data);
  } else {
    result = MLImageTrackerAddTargetFromImageFile(image_tracker_handle_, &target_properties.settings, path.c_str(),
                                                  &target_handle);
  }

  if (result != MLResult_Ok) {
    ML_LOG(Error, "Error adding a tracker target[%s] - %s.", path.c_str(), MLGetResultString(result));
  }
  return target_handle;
}

void ImageTracking::RemoveTarget(MLHandle target_handle) {
  MLResult result = MLImageTrackerRemoveTarget(image_tracker_handle_, target_handle);
  if (result != MLResult_Ok) {
    ML_LOG(Error, "Error removing a tracker target. Error code: %s.", MLGetResultString(result));
    return;
  }
  image_targets_.erase(target_handle);
}

void ImageTracking::UpdateTrackerSettings() {
  MLResult result = MLImageTrackerUpdateSettings(image_tracker_handle_, &image_tracker_setting_);
  if (result != MLResult_Ok) {
    ML_LOG(Error, "Error updating tracker settings - %s.", MLGetResultString(result));
  }
  LogTrackerSettings("Tracker settings updated", image_tracker_setting_);
}

void ImageTracking::UpdateTargetSettings(MLHandle target_handle) {
  MLImageTrackerTargetSettings &target_settings = image_targets_[target_handle].properties.settings;
  MLResult result = MLImageTrackerUpdateTargetSettings(image_tracker_handle_, target_handle, &target_settings);
  if (result != MLResult_Ok) {
    ML_LOG(Error, "Error updating target settings[%s] - %s.", target_settings.name, MLGetResultString(result));
  }
  LogTargetSettings("Target settings updated", target_settings);
}

void ImageTracking::PauseTracker() {
  // Using a copy of the current image tracker setting, disable the image tracking
  MLImageTrackerSettings image_tracker_setting_copy = image_tracker_setting_;
  image_tracker_setting_copy.enable_image_tracking = false;

  MLImageTrackerUpdateSettings(image_tracker_handle_, &image_tracker_setting_copy);
  LogTrackerSettings("Tracker settings updated", image_tracker_setting_copy);
}

void ImageTracking::UpdateTargetResults() {
  for (auto &target : image_targets_) {
    MLHandle target_handle = target.first;
    ImageTargetDataInfo &target_info = target.second;

    target_info.last_result = GetTargetResult(target_handle);
    UpdatePlane(planes_[target_handle], target_info);
  }
}

void ImageTracking::UpdateGui() {
  if (ImGui::Begin("Image Tracking Sample App")) {
    // Update tracker settings
    ImGui::Text("Tracker settings");
    ImGui::Separator();
    if (ConfigureTrackingSettings()) {
      UpdateTrackerSettings();
    }
    ImGui::NewLine();

    ImGui::Text("Image target settings");
    ImGui::Separator();
    for (auto &target : image_targets_) {
      MLHandle target_handle = target.first;
      ImageTargetDataInfo &target_info = target.second;

      // Update target settings
      if (ConfigureTargetSettings(&target_info.properties.settings)) {
        UpdateTargetSettings(target_handle);
      }

      // Get target tracking result
      DisplayTargetResult(target_info.last_result, target_info.data);
    }
    ImGui::NewLine();

    // Exit button to close the app
    if (ImGui::Button("Exit")) {
      StopApp();
    }
  }
  ImGui::End();
}

bool ImageTracking::ConfigureTrackingSettings() {
  bool updated = false;

  // Tracking enabled/disabled
  int enabled_rb = static_cast<int>(image_tracker_setting_.enable_image_tracking);
  updated |= ImGui::RadioButton("Enabed##Tracker", &enabled_rb, 1);
  ImGui::SameLine();
  updated |= ImGui::RadioButton("Disabled##Tracker", &enabled_rb, 0);
  image_tracker_setting_.enable_image_tracking = enabled_rb;

  // Maximum number of targets to track
  int max_targets_slider_value = static_cast<int>(image_tracker_setting_.max_simultaneous_targets);
  updated |= ImGui::SliderInt("Maximum Number of Targets", &max_targets_slider_value, 1, max_targets_);
  image_tracker_setting_.max_simultaneous_targets = max_targets_slider_value;

  return updated;
}

bool ImageTracking::ConfigureTargetSettings(MLImageTrackerTargetSettings *target_settings) {
  bool updated = false;

  // Image target enabled/disabled
  int enabled_rb = static_cast<int>(target_settings->is_enabled);
  ImGui::Text("%s", target_settings->name);
  updated |= ImGui::RadioButton((std::string("Enabled##") + target_settings->name).c_str(), &enabled_rb, 1);
  ImGui::SameLine();
  updated |= ImGui::RadioButton((std::string("Disabled##") + target_settings->name).c_str(), &enabled_rb, 0);
  target_settings->is_enabled = static_cast<bool>(enabled_rb);

  return updated;
}

void ImageTracking::DisplayTargetResult(const MLImageTrackerTargetResult &target_result,
                                        const MLImageTrackerTargetStaticData &target_data) {
  switch (target_result.status) {
    case MLImageTrackerTargetStatus_Tracked:
      ImGui::TextColored(ImVec4(ImColor(0, 255, 0)), "Tracked");  // Green
      ImGui::SameLine();
      ImGui::Text("%s", ml::app_framework::to_string(target_data.coord_frame_target).c_str());
      break;
    case MLImageTrackerTargetStatus_Unreliable:
      ImGui::TextColored(ImVec4(ImColor(255, 255, 0)), "Unreliable");  // Yellow
      ImGui::SameLine();
      ImGui::Text("%s", ml::app_framework::to_string(target_data.coord_frame_target).c_str());
      break;
    case MLImageTrackerTargetStatus_NotTracked:
      ImGui::TextColored(ImVec4(ImColor(255, 0, 0)), "Not Tracked");  // Red
      break;
    default: break;
  }
}

MLImageTrackerTargetResult ImageTracking::GetTargetResult(MLHandle target_handle) {
  MLImageTrackerTargetResult target_result;
  MLResult result = MLImageTrackerGetTargetResult(image_tracker_handle_, target_handle, &target_result);
  if (result != MLResult_Ok) {
    ML_LOG(Error, "Error getting target result - %s.", MLGetResultString(result));
  }
  return target_result;
}

void ImageTracking::LogTrackerSettings(const char *title, const MLImageTrackerSettings &tracker_settings) {
  ML_LOG(Verbose, "%s", title);
  ML_LOG(Verbose, " - # of Max. Targets : %d", tracker_settings.max_simultaneous_targets);
  ML_LOG(Verbose, " - enabled           : %s", tracker_settings.enable_image_tracking ? "Yes" : "No");
}

void ImageTracking::LogTargetSettings(const char *title, const MLImageTrackerTargetSettings &target_settings) {
  ML_LOG(Verbose, "%s", title);
  ML_LOG(Verbose, " - name        : %s", target_settings.name);
  ML_LOG(Verbose, " - longer dim. : %f", target_settings.longer_dimension);
  ML_LOG(Verbose, " - stationary  : %s", target_settings.is_stationary ? "Yes" : "No");
  ML_LOG(Verbose, " - enabled     : %s", target_settings.is_enabled ? "Yes" : "No");
}

void ImageTracking::AddPlaneToTarget(MLHandle target_handle) {
  planes_[target_handle] = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Quad);
  planes_[target_handle]->GetComponent<ml::app_framework::RenderableComponent>()->SetVisible(false);
  GetRoot()->AddChild(planes_[target_handle]);
}

void ImageTracking::UpdatePlane(std::shared_ptr<ml::app_framework::Node> target_plane,
                                ImageTargetDataInfo target_data) {
  if (target_data.last_result.status == MLImageTrackerTargetStatus_NotTracked ||
      !target_data.properties.settings.is_enabled || !image_tracker_setting_.enable_image_tracking) {
    // If the target is no longer tracked or is disabled, leave the plane at the last known location and change the color.
    auto renderable = target_plane->GetComponent<ml::app_framework::RenderableComponent>();
    if (renderable->GetVisible()) {
      auto material = std::static_pointer_cast<ml::app_framework::FlatMaterial>(renderable->GetMaterial());
      if (target_data.last_result.status == MLImageTrackerTargetStatus_NotTracked) {
        material->SetColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));  // Red
      } else {
        // Was being tracked, but the tracking is now disabled.
        material->SetColor(glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));  // Orange
      }
    }
    return;
  }

  // The target is being tracked, draw a plane on the target
  MLSnapshot *snapshot = nullptr;
  UNWRAP_MLRESULT(MLPerceptionGetSnapshot(&snapshot));
  MLTransform transform = {};
  MLResult result = MLSnapshotGetTransform(snapshot, &target_data.data.coord_frame_target, &transform);
  if (MLSnapshotGetTransform(snapshot, &target_data.data.coord_frame_target, &transform) != MLResult_Ok) {
    ML_LOG(Warning, "No Pose for target");
    return;
  }

  target_plane->SetWorldTranslation(ml::app_framework::to_glm(transform.position));
  target_plane->SetWorldRotation(ml::app_framework::to_glm(transform.rotation));
  target_plane->SetLocalScale(glm::vec3(target_data.properties.width, target_data.properties.height, 0.005f));
  target_plane->GetComponent<ml::app_framework::RenderableComponent>()->SetVisible(true);

  auto material = std::static_pointer_cast<ml::app_framework::FlatMaterial>(
      target_plane->GetComponent<ml::app_framework::RenderableComponent>()->GetMaterial());
  switch (target_data.last_result.status) {
    case MLImageTrackerTargetStatus_Tracked:
      material->SetColor(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));  // Green
      break;
    case MLImageTrackerTargetStatus_Unreliable:
      material->SetColor(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));  // Yellow
      break;
    default:
      material->SetColor(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));  // Blue
      break;
  }

  UNWRAP_MLRESULT(MLPerceptionReleaseSnapshot(snapshot));
}
