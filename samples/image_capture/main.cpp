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

#include <ml_camera.h>
#include <ml_lifecycle.h>
#include <ml_logging.h>
#include <ml_privileges.h>

#include <gflags/gflags.h>
#include <imgui.h>

#include <chrono>
#include <cinttypes>

using namespace std::chrono;

DEFINE_bool(jpeg, true, "jpeg is the default; set to false for yuv format");
DEFINE_bool(video, false, "video or image capture");
DEFINE_int32(videoLength, 3000, "length of video in milliseconds");

#define CHECK_PRIV(priv)                                                \
  if (MLPrivilegesResult_Granted != MLPrivilegesCheckPrivilege(priv)) { \
    ML_LOG(Info, "***WARNING*** privilege revoked");                    \
    return;                                                             \
  }

class ImageCaptureApp : public ml::app_framework::Application {
public:
  ImageCaptureApp(int argc = 0, char **argv = nullptr) : ml::app_framework::Application(argc, argv) {}
  ~ImageCaptureApp() {}

  void OnStart() override {
    if (FLAGS_video) {
      if (FLAGS_videoLength <= 0) {
        ML_LOG(Info, "No video length set, exiting.");
        StopApp();
      }
      end_ = system_clock::now() + std::chrono::milliseconds(FLAGS_videoLength);
      ML_LOG(Info, "Starting Video capture.");
    } else {
      ML_LOG(Info, "Starting Image capture.");
    }

    if (!SetUpCamera()) {
      ML_LOG(Info, "Failed to setup the camera, exiting.");
      StopApp();
    }
  }

  void OnStop() override {
    UNWRAP_MLRESULT(MLCameraDisconnect());
    UNWRAP_MLRESULT(MLPrivilegesShutdown());
  }

  void OnUpdate(float) override {
    if (FLAGS_video) {
      if (std::chrono::system_clock::now() >= end_) {
        ML_LOG(Info, "Time's Up Stopping Video Recording");
        MLCameraCaptureVideoStop();
        camera_handle_ = ML_INVALID_HANDLE;
        StopApp();
        return;
      }
    }

    if (camera_handle_ == ML_INVALID_HANDLE) {
      UNWRAP_MLRESULT(MLCameraSetOutputFormat(
          FLAGS_jpeg ? MLCameraOutputFormat_JPEG : MLCameraOutputFormat_YUV_420_888));  // only effects image capture
      UNWRAP_MLRESULT(MLCameraPrepareCapture(
          (FLAGS_video ? MLCameraCaptureType_VideoRaw : MLCameraCaptureType_ImageRaw), &camera_handle_));
      if (FLAGS_video) {
        UNWRAP_MLRESULT(MLCameraCaptureRawVideoStart());
      } else {
        UNWRAP_MLRESULT(MLCameraCaptureImageRaw());
      }
    } else {
      CHECK_PRIV(MLPrivilegeID_CameraCapture);
      CHECK_PRIV(MLPrivilegeID_AudioCaptureMic);

      MLCameraResultExtras *extras = nullptr;
      uint32_t capture_status;
      UNWRAP_MLRESULT(MLCameraGetCaptureResultExtras(&extras));
      UNWRAP_MLRESULT(MLCameraGetCaptureStatus(&capture_status));

      if (capture_status & MLCameraCaptureStatusFlag_Completed) {
        ML_LOG(Info, "Frame #: %" PRIu64 " Timestamp %" PRIu64, extras->frame_number, extras->vcam_timestamp_us);
        MLCameraOutput *data = nullptr;
        MLResult ret = MLCameraGetImageStream(&data);
        if (MLCameraGetImageStream(&data) == MLResult_Ok) {
          save_path_ = std::string(GetLifecycleInfo().writable_dir_path) + "img_" + CurrentDateTime() + "_" +
                       std::to_string(extras->frame_number);
          if (FLAGS_video) {
            save_path_ += ".yuv";
          } else {
            if (FLAGS_jpeg) {
              save_path_ += ".jpg";
            } else {
              save_path_ += ".yuv";
            }
          }

          WriteCameraOutput(data, save_path_.c_str());
          if (!FLAGS_video) {
            camera_handle_ = ML_INVALID_HANDLE;
            StopApp();
          }
        }
      }
    }
  }

private:
  bool SetUpCamera() {
    UNWRAP_MLRESULT(MLPrivilegesStartup());
    MLResult result = MLPrivilegesRequestPrivilege(MLPrivilegeID_CameraCapture);
    if (MLPrivilegesResult_Granted != result) {
      ML_LOG(Error, "ImageCapture: Error requesting MLPrivilegeID_CameraCapture privilege, error: %s",
             MLPrivilegesGetResultString(result));
      return false;
    }
    result = MLPrivilegesRequestPrivilege(MLPrivilegeID_AudioCaptureMic);
    if (MLPrivilegesResult_Granted != result) {
      ML_LOG(Error, "ImageCapture: Error requesting MLPrivilegeID_AudioCaptureMic privilege, error: %s",
             MLPrivilegesGetResultString(result));
      return false;
    }

    UNWRAP_MLRESULT_FATAL(MLCameraConnect());
    return true;
  }

  const std::string CurrentDateTime() {
    time_t now = time(0);
    tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", &tstruct);
    return buf;
  }

  void WriteCameraOutput(const MLCameraOutput *image, const char *filepath) {
    if (image == NULL) {
      ML_LOG(Error, "Invalid camera output");
      return;
    }

    FILE *fp = fopen(filepath, "wb+");
    if (fp == NULL) {
      ML_LOG(Error, "Can't open file: %s", filepath);
      return;
    }

    for (int i = 0; i < image->plane_count; i++) {
      uint32_t length = image->planes[i].width * image->planes[i].bytes_per_pixel;
      uint8_t *src = image->planes[i].data;
      if (length < image->planes[i].stride) {
        for (uint32_t y = 0; y < image->planes[i].height; y++) {
          fwrite(src, 1, length, fp);
          src += image->planes[i].stride;
        }
      } else {
        fwrite(src, 1, image->planes[i].size, fp);
      }
    }
    fclose(fp);
    ML_LOG(Info, "Camera ouput written to \"%s\"", filepath);
    return;
  }

  MLHandle camera_handle_ = ML_INVALID_HANDLE;
  std::string save_path_;
  std::chrono::time_point<system_clock> end_;
};

int main(int argc, char **argv) {
  ImageCaptureApp app(argc, argv);
  app.SetUseGfx(true);
  app.RunApp();
  return 0;
}
