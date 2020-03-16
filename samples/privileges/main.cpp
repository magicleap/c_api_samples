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

#include <ml_lifecycle.h>
#include <ml_logging.h>
#include <ml_privileges.h>

#include <gflags/gflags.h>
#include <imgui.h>

#include <chrono>
#include <cinttypes>

using namespace std::chrono;

class PrivilegesApp : public ml::app_framework::Application {
public:
  PrivilegesApp(int argc = 0, char **argv = nullptr) : ml::app_framework::Application(argc, argv) {}
  ~PrivilegesApp() {}

  void OnStart() override {
    UNWRAP_MLRESULT(MLPrivilegesStartup());

    // Request a privilege blocking the user with a popup
    MLResult result = MLPrivilegesRequestPrivilege(MLPrivilegeID_CameraCapture);
    switch (result) {
      case MLPrivilegesResult_Granted: ML_LOG(Info, "Successfully acquired the Camera Privilege"); break;
      default:
        ML_LOG(Error, "Error requesting privileges, exiting the app.");
        StopApp();
        return;
    }

    // Request a privilege asyncronously
    // Check for granted or denied in the Update loop via TryGet
    MLPrivilegesRequestPrivilegeAsync(MLPrivilegeID_AudioCaptureMic, &audio_cap_mic_request_);
    MLPrivilegesRequestPrivilegeAsync(MLPrivilegeID_SocialConnectionsInvitesAccess, &social_connect_request_);
  }

  void OnStop() override {
    UNWRAP_MLRESULT(MLPrivilegesShutdown());
  }

  void OnResume() override {
    // Check to see if we still have the privileges
    // Users are able to turn off some subsystems in the Settings app
    CheckPrivilege(MLPrivilegeID_CameraCapture);
    CheckPrivilege(MLPrivilegeID_AudioCaptureMic);
    CheckPrivilege(MLPrivilegeID_SocialConnectionsInvitesAccess);
  }

  void OnUpdate(float) override {
    // Check for privileges
    if (check_audio_cap_mic_) {
      CheckPrivilegeStatus(MLPrivilegeID_AudioCaptureMic, audio_cap_mic_request_);
    }
    if (check_social_connect_) {
      CheckPrivilegeStatus(MLPrivilegeID_SocialConnectionsInvitesAccess, social_connect_request_);
    }

    if (!check_audio_cap_mic_ &&
        !check_social_connect_) {
      ML_LOG(Info, "Done checking for privileges, exiting");
      StopApp();
    }
  }

private:

  void CheckPrivilege(MLPrivilegeID priv) {
    if (MLPrivilegesResult_Granted != MLPrivilegesCheckPrivilege(priv)) {
      ML_LOG(Info, "***WARNING*** privilege revoked");
    }
  }

  void CheckPrivilegeStatus(MLPrivilegeID privilege_id, MLPrivilegesAsyncRequest* request) {
    MLResult result = MLPrivilegesRequestPrivilegeTryGet(request);

    switch (result) {
      case MLResult_Pending: ML_LOG(Info, "Pending. ID: %d", privilege_id); break;
      default:
        ML_LOG(Error, "ID: %d Error code: %s.", privilege_id, MLPrivilegesGetResultString(result));
        if (MLPrivilegeID_AudioCaptureMic == privilege_id) {
          check_audio_cap_mic_ = false;
        }

        if (MLPrivilegeID_SocialConnectionsInvitesAccess == privilege_id) {
          check_social_connect_ = false;
        }
        break;
    }
  }

  MLPrivilegesAsyncRequest *audio_cap_mic_request_;
  bool check_audio_cap_mic_ = true;
  MLPrivilegesAsyncRequest *social_connect_request_;
  bool check_social_connect_ = true;
};

int main(int argc, char **argv) {
  PrivilegesApp app(argc, argv);
  app.SetUseGfx(true);
  app.RunApp();
  return 0;
}
