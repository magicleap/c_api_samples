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
#include <imgui.h>

#include <ml_audio.h>
#include <ml_logging.h>
#include <ml_privileges.h>

#include <algorithm>
#include <fstream>
#include <limits>

DEFINE_int32(sample_rate, 16000, "sample rate for the audio stream in hertz");
DEFINE_int32(channels, 1, "number of audio channels");
DEFINE_bool(use_callback, false, "use the audio callback instead of updating sounds in the update loop");

constexpr auto kKibibyte = 2 << 10;
constexpr auto kMaxBufferSize = 320 * kKibibyte;

#define UNWRAP_MLAudioResult(result) MLRESULT_EXPECT((result), MLResult_Ok, MLAudioGetResultString, Error)

class AudioPlaybackApp : public ml::app_framework::Application {
public:
  AudioPlaybackApp(int argc = 0, char **argv = nullptr) : ml::app_framework::Application(argc, argv) {}

  void OnStart() override {
    UNWRAP_MLRESULT(MLPrivilegesStartup());
    UNWRAP_MLPRIVILEGES_RESULT_FATAL(MLPrivilegesRequestPrivilege(MLPrivilegeID_AudioCaptureMic));

    uint32_t recommended_size;
    MLAudioBufferFormat buffer_format = {};

    UNWRAP_MLAudioResult(
        MLAudioGetBufferedInputDefaults(FLAGS_channels, FLAGS_sample_rate, &buffer_format, &recommended_size, nullptr));

    MLAudioBufferCallback input_callback = [](MLHandle handle, void *callback_context) {
      auto *self = reinterpret_cast<AudioPlaybackApp *>(callback_context);
      ML_LOG_IF(Fatal, self->audio_input_handle_ != handle,
                "The callback's handle and the sample's handle don't match.");
      self->FillInputBuffer();
    };

    UNWRAP_MLAudioResult(MLAudioCreateInputFromVoiceComm(&buffer_format, recommended_size,
                                                         FLAGS_use_callback ? input_callback : nullptr, (void *)this,
                                                         &audio_input_handle_));

    MLAudioBufferCallback output_callback = [](MLHandle handle, void *callback_context) {
      auto *self = reinterpret_cast<AudioPlaybackApp *>(callback_context);
      ML_LOG_IF(Fatal, self->audio_output_handle_ != handle,
                "The callback's handle and the sample's handle don't match.");
      self->FillOutputBuffer();
    };

    UNWRAP_MLAudioResult(MLAudioCreateSoundWithBufferedOutput(&buffer_format, recommended_size,
                                                              FLAGS_use_callback ? output_callback : nullptr,
                                                              (void *)this, &audio_output_handle_));

    ml::app_framework::Gui::GetInstance().Initialize(GetGraphicsContext());
    GetRoot()->AddChild(ml::app_framework::Gui::GetInstance().GetNode());
  }

  void OnStop() override {
    ml::app_framework::Gui::GetInstance().Cleanup();
    UNWRAP_MLAudioResult(MLAudioStopInput(audio_input_handle_));
    UNWRAP_MLAudioResult(MLAudioDestroyInput(audio_input_handle_));
    UNWRAP_MLAudioResult(MLAudioStopSound(audio_output_handle_));
    UNWRAP_MLAudioResult(MLAudioDestroySound(audio_output_handle_));
  }

  void OnUpdate(float) override {
    // Since more than one input or output buffer may be ready during a single update, we'll call FillInputBuffer and
    // FillOutputBuffer a few times each frame. The actual number of times we call them is arbitrary.
    static constexpr int kMaxAudioBuffersPerFrame = 3;

    if (!FLAGS_use_callback) {
      for (int i = 0; i < kMaxAudioBuffersPerFrame; ++i)
        if (!FillInputBuffer()) {
          break;
        }
      for (int i = 0; i < kMaxAudioBuffersPerFrame; ++i) {
        if (!FillOutputBuffer()) {
          break;
        }
      }
    }

    UpdateGui();
  }

private:
  bool FillInputBuffer() {
    MLAudioBuffer buffer = {};
    MLResult result{MLAudioGetInputBuffer(audio_input_handle_, &buffer)};
    if (MLAudioResult_BufferNotReady == result) {
      return false;
    }
    UNWRAP_MLAudioResult(result);

    std::lock_guard<std::mutex> lock{pcm_buffer_mtx_};

    if (pcm_buffer_.size() < kMaxBufferSize) {
      auto old_size = pcm_buffer_.size();
      pcm_buffer_.resize(pcm_buffer_.size() + buffer.size);
      std::copy_n(buffer.ptr, buffer.size, pcm_buffer_.data() + old_size);
    }

    UNWRAP_MLAudioResult(MLAudioReleaseInputBuffer(audio_input_handle_));
    return true;
  }

  bool FillOutputBuffer() {
    MLAudioBuffer buffer = {};
    MLResult result{MLAudioGetOutputBuffer(audio_output_handle_, &buffer)};
    if (MLAudioResult_BufferNotReady == result) {
      return false;
    }
    UNWRAP_MLAudioResult(result);

    std::lock_guard<std::mutex> lock{pcm_buffer_mtx_};

    // ensure we do not read past the end of pcm_file_buffer_
    size_t bytes_to_copy{std::min(static_cast<size_t>(buffer.size), pcm_buffer_.size() - buffer_output_position_)};

    std::copy_n(pcm_buffer_.data() + buffer_output_position_, bytes_to_copy, buffer.ptr);
    // if there wasn't enough pcm data to fill the buffer, zero out the rest of it to avoid crackles
    std::fill_n(buffer.ptr + bytes_to_copy, buffer.size - bytes_to_copy, 0);
    buffer_output_position_ += bytes_to_copy;

    UNWRAP_MLAudioResult(MLAudioReleaseOutputBuffer(audio_output_handle_));
    return true;
  }

  void UpdateGui() {
    ml::app_framework::Gui::GetInstance().BeginUpdate();

    bool is_running{true};
    if (ImGui::Begin("Audio Input", &is_running, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
      if (ImGui::CollapsingHeader("Record", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Start##Input")) {
          UNWRAP_MLAudioResult(MLAudioStartInput(audio_input_handle_));
        }
        ImGui::SameLine();

        if (ImGui::Button("Stop##Input")) {
          UNWRAP_MLAudioResult(MLAudioStopInput(audio_input_handle_));
        }
        ImGui::SameLine();

        if (ImGui::Button("Clear##Input")) {
          std::lock_guard<std::mutex> lock{pcm_buffer_mtx_};
          pcm_buffer_.clear();
          buffer_output_position_ = 0;
        }

        float latency_in_msec{0};
        UNWRAP_MLAudioResult(MLAudioGetBufferedInputLatency(audio_input_handle_, &latency_in_msec));
        ImGui::Text("Input Latency: %.2f", latency_in_msec);

        bool is_muted = false;
        UNWRAP_MLAudioResult(MLAudioIsMicMuted(&is_muted));
        if (ImGui::Checkbox("Mic Mute##Input", &is_muted)) {
          UNWRAP_MLAudioResult(MLAudioSetMicMute(is_muted));
        }

        MLAudioState state;
        UNWRAP_MLAudioResult(MLAudioGetInputState(audio_input_handle_, &state));

        static constexpr std::array<const char *, 3> audio_state_strs{"Stopped", "Playing", "Paused"};
        ImGui::Text("SoundState: %s", audio_state_strs[state]);

        {
          std::lock_guard<std::mutex> lock{pcm_buffer_mtx_};
          ImGui::PlotLines("recorded audio",
                           [](void *data, int idx) {
                             return reinterpret_cast<int16_t *>(data)[idx] / float(std::numeric_limits<int16_t>::max());
                           },
                           pcm_buffer_.data(), pcm_buffer_.size() / 2, 0, nullptr, -1.0f, 1.0f, ImVec2(0, 80));
        }

        if (ImGui::Button("Save##Input")) {
          std::lock_guard<std::mutex> lock{pcm_buffer_mtx_};
          auto file_path = std::string{GetLifecycleInfo().writable_dir_path} + "sound.pcm";
          std::ofstream pcm_file{file_path.c_str(), std::ios::binary};
          pcm_file.write(reinterpret_cast<const char *>(pcm_buffer_.data()), pcm_buffer_.size());
          pcm_file.close();
          ML_LOG(Info, "Saved recording to %s", file_path.c_str());
        }
      }

      if (ImGui::CollapsingHeader("Playback", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Start##Output")) {
          std::lock_guard<std::mutex> lock{pcm_buffer_mtx_};
          buffer_output_position_ = 0;
          UNWRAP_MLAudioResult(MLAudioStartSound(audio_output_handle_));
        }
        ImGui::SameLine();

        if (ImGui::Button("Stop##Output")) {
          UNWRAP_MLAudioResult(MLAudioStopSound(audio_output_handle_));
        }
        ImGui::SameLine();

        if (ImGui::Button("Pause##Output")) {
          UNWRAP_MLAudioResult(MLAudioPauseSound(audio_output_handle_));
        }
        ImGui::SameLine();

        if (ImGui::Button("Resume##Output")) {
          UNWRAP_MLAudioResult(MLAudioResumeSound(audio_output_handle_));
        }

        MLAudioState state;
        UNWRAP_MLAudioResult(MLAudioGetSoundState(audio_output_handle_, &state));

        static constexpr std::array<const char *, 3> audio_state_strs{"Stopped", "Playing", "Paused"};
        ImGui::Text("SoundState: %s", audio_state_strs[state]);
      }
    }
    ImGui::End();
    ml::app_framework::Gui::GetInstance().EndUpdate();

    if (!is_running) {
      StopApp();
    }
  }

  MLHandle audio_input_handle_ = ML_INVALID_HANDLE;
  MLHandle audio_output_handle_ = ML_INVALID_HANDLE;
  size_t buffer_output_position_ = 0;
  std::vector<uint8_t> pcm_buffer_;
  std::mutex pcm_buffer_mtx_;
};

int main(int argc, char **argv) {
  AudioPlaybackApp app(argc, argv);
  app.SetUseGfx(true);
  app.RunApp();
  return 0;
}
