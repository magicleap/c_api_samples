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
#include <app_framework/gui.h>
#include <app_framework/ml_macros.h>
#include <app_framework/toolset.h>

#include <imgui.h>

#include <cstdlib>
#include <ml_location.h>
#include <ml_privileges.h>

class LocationApp : public ml::app_framework::Application {
public:
  LocationApp(int argc = 0, char **argv = nullptr) : ml::app_framework::Application(argc, argv) {}

  void OnStart() override {
    RequestPrivileges();
    MLLocationDataInit(&location_data_);
    MLLocationDataInit(&fine_location_data_);

    ml::app_framework::Gui::GetInstance().Initialize(GetGraphicsContext());
    GetRoot()->AddChild(ml::app_framework::Gui::GetInstance().GetNode());
  }

  void UpdateGui() {
    ml::app_framework::Gui::GetInstance().BeginUpdate();
    ImGui::Text("Location");
    ImGui::Indent();

    if (ImGui::Button("Get last known location")) {
      UNWRAP_MLRESULT(MLLocationGetLastCoarseLocation(&location_data_));
      ML_LOG(Info, "Lat: %.2f, Long: %.2f", location_data_.latitude, location_data_.longitude);
      if (location_data_.location_mask & MLLocationDataMask_HasPostalCode ) {
          ML_LOG(Info, "Postal Code : %s", location_data_.postal_code);
      }
    }

    ImGui::Text("Lat: %.2f, Long: %.2f", location_data_.latitude, location_data_.longitude);
    if (location_data_.location_mask & MLLocationDataMask_HasPostalCode ) {
      ImGui::Text("Postal Code : %s", location_data_.postal_code);
    }

    if (ImGui::Button("Get last known fine location")) {
      UNWRAP_MLRESULT(MLLocationGetLastFineLocation(&fine_location_data_));
      ML_LOG(Info, "Lat: %.5f, Long: %.5f", fine_location_data_.latitude, fine_location_data_.longitude);
      ML_LOG(Info, "Last timestamp ms = %" PRIu64, fine_location_data_.timestamp_ms);

      if (fine_location_data_.location_mask & MLLocationDataMask_HasAccuracy) {
        ML_LOG(Info, "Accuracy : %.5f", fine_location_data_.accuracy);
      }
      if (fine_location_data_.location_mask & MLLocationDataMask_HasPostalCode) {
        ML_LOG(Info, "Postal Code : %s", fine_location_data_.postal_code);
      }
    }

    ImGui::Text("Lat: %.5f, Long: %.5f", fine_location_data_.latitude, fine_location_data_.longitude);
    ImGui::Text("Last timestamp ms = %" PRIu64, fine_location_data_.timestamp_ms);
    if (fine_location_data_.location_mask & MLLocationDataMask_HasAccuracy) {
      ImGui::Text("Accuracy : %.5f", fine_location_data_.accuracy);
    }
    if (fine_location_data_.location_mask & MLLocationDataMask_HasPostalCode) {
      ImGui::Text("Postal Code : %s", fine_location_data_.postal_code);
    }

    ml::app_framework::Gui::GetInstance().EndUpdate();
  }

  void OnUpdate(float) override {
    UpdateGui();
  }

  void OnStop() override {
    ml::app_framework::Gui::GetInstance().Cleanup();
    UNWRAP_MLRESULT(MLPrivilegesShutdown());
  }

private:

  void RequestPrivileges() {
    UNWRAP_MLRESULT(MLPrivilegesStartup());

    MLResult result = MLPrivilegesRequestPrivilege(MLPrivilegeID_CoarseLocation);
    switch (result) {
      case MLPrivilegesResult_Granted: ML_LOG(Info, "Successfully acquired the Location Privilege"); break;
      case MLResult_UnspecifiedFailure: ML_LOG(Error, "Error requesting privileges, but the app will continue."); break;
      case MLPrivilegesResult_Denied:
        ML_LOG(Error, "Location privilege denied, but the app will continue. Error code: %s", MLPrivilegesGetResultString(result));
        break;
      default: ML_LOG(Fatal, "Unknown privilege error, exiting the app."); break;
    }
    result = MLPrivilegesRequestPrivilege(MLPrivilegeID_FineLocation);
    switch (result) {
      case MLPrivilegesResult_Granted: ML_LOG(Info, "Successfully acquired the FineLocation Privilege"); break;
      case MLResult_UnspecifiedFailure: ML_LOG(Error, "Error requesting FineLocation privilege, but the app will continue."); break;
      case MLPrivilegesResult_Denied:
        ML_LOG(Error, "FineLocation privilege denied, but the app will continue. Error code: %s", MLPrivilegesGetResultString(result));
        break;
      default: ML_LOG(Fatal, "Unknown FineLocation privilege error, exiting the app."); break;
    }
  }

  bool GetLastCoarseLocation() {
    MLResult result = MLLocationGetLastCoarseLocation(&location_data_);
    if (MLResult_Ok != result) {
      ML_LOG(Error, "Failed to fetch location data, error: %s", MLLocationGetResultString(result));
      return false;
    }
    return true;
  }

  MLLocationData location_data_ = {};
  MLLocationData fine_location_data_ = {};
  MLResult most_recent_query_result_ = MLResult_UnspecifiedFailure;

};

int main(int argc, char **argv) {
  LocationApp app(argc, argv);
  app.SetUseGfx(true);
  app.RunApp();
  return 0;
}
