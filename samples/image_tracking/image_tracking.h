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
#include <app_framework/application.h>

#include <ml_image_tracking.h>

class ImageTracking : public ml::app_framework::Application {
public:
  ImageTracking(int argc = 0, char **argv = nullptr);
  ~ImageTracking() = default;

  void OnStart() override;
  void OnStop() override;
  void OnPause() override;
  void OnResume() override;
  void OnUpdate(float) override;

private:
  // Contains the settings for the image target.
  struct ImageTargetProperties {
    MLImageTrackerTargetSettings settings;
    float width;   // meters
    float height;  // meters
    bool load_data;
  };

  // Contains the settings and data for the image target.
  struct ImageTargetDataInfo {
    ImageTargetProperties properties;
    MLImageTrackerTargetStaticData data;
    MLImageTrackerTargetResult last_result;
  };

  void RequestPrivileges();
  void InitializeHeadTracker();
  void TerminateHeadTracker();
  void InitializeImageTracker();
  void TerminateImageTracker();
  void AddImageTargets();
  void RemoveImageTargets();
  void InitializeGui();
  void TerminateGui();

  // To add and remove image targets
  MLHandle AddTarget(const std::string &path, const ImageTargetProperties &target_properties);
  void RemoveTarget(MLHandle target_handle);

  // To update image tracker/target settings
  void UpdateTrackerSettings();
  void UpdateTargetSettings(MLHandle target_handle);
  void PauseTracker();

  // To refresh GUI using the latest settings/results
  void UpdateTargetResults();
  void UpdateGui();

  // To display and configure tracker/target setttings via GUI.
  bool ConfigureTrackingSettings();
  bool ConfigureTargetSettings(MLImageTrackerTargetSettings *target_settings);
  MLImageTrackerTargetResult GetTargetResult(MLHandle target_handle);
  void DisplayTargetResult(const MLImageTrackerTargetResult &target_result,
                           const MLImageTrackerTargetStaticData &target_data);

  void LogTrackerSettings(const char *title, const MLImageTrackerSettings &target_settings);
  void LogTargetSettings(const char *title, const MLImageTrackerTargetSettings &target_settings);

  // To draw planes on image targets.
  void AddPlaneToTarget(MLHandle target_handle);
  void UpdatePlane(std::shared_ptr<ml::app_framework::Node> target_plane, ImageTargetDataInfo target_data);

  MLHandle image_tracker_handle_;
  MLHandle head_tracker_handle_;
  MLImageTrackerSettings image_tracker_setting_;
  std::unordered_map<std::string, MLHandle> available_images_;
  std::unordered_map<MLHandle, ImageTargetDataInfo> image_targets_;
  std::unordered_map<MLHandle, std::shared_ptr<ml::app_framework::Node>> planes_;

  const std::unordered_map<std::string, ImageTargetProperties> targets_to_add_{
      {std::string("./data/DeepSpaceExploration.png"), {{"DeepSpaceExploration", 0.265f, true, true}, 0.185f, 0.265f, true}},
      {std::string("./data/DeepSeaExploration.png"), {{"DeepSeaExploration", 0.265f, true, true}, 0.265f, 0.185f, false}}};
  const uint32_t max_targets_;
};
