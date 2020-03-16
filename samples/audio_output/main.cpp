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
#include <ml_input.h>
#include <ml_logging.h>

#include <algorithm>

constexpr float kDragSpeed = 0.1f;

DEFINE_string(filename, "data/ML_AUD_Example.raw", "Raw PCM file to open");
DEFINE_int32(sample_rate, 48000, "sample rate for the audio stream in hertz");
DEFINE_int32(channels, 2, "number of audio channels");
DEFINE_bool(use_callback, false, "use the audio callback instead of updating sounds in the update loop");
DEFINE_bool(spatial_sound_enable, false, "Enables 3D audio processing for a sound output");

#define UNWRAP_MLAudioResult(result) MLRESULT_EXPECT((result), MLResult_Ok, MLAudioGetResultString, Error)

class AudioPlaybackApp : public ml::app_framework::Application {
public:
  AudioPlaybackApp(int argc = 0, char **argv = nullptr) : ml::app_framework::Application(argc, argv) {}

  void OnStart() override {
    MLAudioBufferFormat format;
    uint32_t recommended_size;
    constexpr float max_pitch = 1.0;

    UNWRAP_MLAudioResult(MLAudioGetBufferedOutputDefaults(FLAGS_channels, FLAGS_sample_rate, max_pitch, &format,
                                                          &recommended_size, nullptr));

    // load the entire sound file to a buffer:
    std::ifstream stream(FLAGS_filename, std::ios::in | std::ios::binary);
    ML_LOG_IF(Fatal, !stream.good(), "file \"%s\" not found", FLAGS_filename.c_str());
    pcm_file_buffer_ = std::vector<uint8_t>((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    ML_LOG_IF(Fatal, pcm_file_buffer_.size() < recommended_size,
              "File \"%s\" was too small! The pcm file should be at least %" PRIu32 " bytes!", FLAGS_filename.c_str(),
              recommended_size);

    MLAudioBufferCallback callback = [](MLHandle handle, void *callback_context) {
      auto *self = reinterpret_cast<AudioPlaybackApp *>(callback_context);
      self->FillBuffer();
    };
    UNWRAP_MLAudioResult(MLAudioCreateSoundWithBufferedOutput(
        &format, recommended_size, FLAGS_use_callback ? callback : nullptr, (void *)this, &audio_handle_));

    UNWRAP_MLAudioResult(MLAudioSetSpatialSoundEnable(audio_handle_, FLAGS_spatial_sound_enable));

    UNWRAP_MLAudioResult(MLAudioStartSound(audio_handle_));

    for (auto &node : sound_nodes_) {
      node = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Axis);
      node->SetLocalScale(glm::vec3(0.2f, 0.2f, 0.2f));
      GetRoot()->AddChild(node);
    }

    MLInputConfiguration input_config = {};
    input_config.dof[0] = MLInputControllerDof_6;
    input_config.dof[1] = MLInputControllerDof_6;
    UNWRAP_MLRESULT(MLInputCreate(&input_config, &input_tracker_));

    ml::app_framework::Gui::GetInstance().Initialize(GetGraphicsContext());
    GetRoot()->AddChild(ml::app_framework::Gui::GetInstance().GetNode());
  }

  void OnStop() override {
    ml::app_framework::Gui::GetInstance().Cleanup();
    UNWRAP_MLRESULT(MLInputDestroy(input_tracker_));
    UNWRAP_MLAudioResult(MLAudioStopSound(audio_handle_));

    UNWRAP_MLAudioResult(MLAudioDestroySound(audio_handle_));
  }

  void OnUpdate(float) override {
    /* Since more than one buffer may be ready during a single update, we'll call FillBuffer a few times each frame
     * until we can't fill any more buffers. The actual number of times we call FillBuffer is arbitrary. */
    static constexpr int kMaxAudioBuffersPerFrame = 3;

    if (!FLAGS_use_callback) {
      for (int i = 0; i < kMaxAudioBuffersPerFrame; ++i) {
        if (!FillBuffer()) {
          break;
        }
      }
    }

    MLInputControllerState input_state[MLInput_MaxControllers];
    UNWRAP_MLRESULT(MLInputGetControllerState(input_tracker_, input_state));

    for (uint32_t i{0}; i < sound_nodes_.size(); ++i) {
      sound_nodes_[i]->GetComponent<ml::app_framework::RenderableComponent>()->SetVisible(i < FLAGS_channels);
      if (channel_tracks_controller_[i] && !(i > 0 && copy_channel_transforms_)) {
        sound_nodes_[i]->SetWorldTranslation(ml::app_framework::to_glm(input_state[i].position));
        sound_nodes_[i]->SetWorldRotation(ml::app_framework::to_glm(input_state[i].orientation));

        UNWRAP_MLAudioResult(MLAudioSetSpatialSoundPosition(audio_handle_, i, &input_state[i].position));
        UNWRAP_MLAudioResult(MLAudioSetSpatialSoundDirection(audio_handle_, i, &input_state[i].orientation));
      } else {
        MLVec3f position{};
        MLQuaternionf orientation{};
        UNWRAP_MLAudioResult(MLAudioGetSpatialSoundPosition(audio_handle_, i, &position));
        UNWRAP_MLAudioResult(MLAudioGetSpatialSoundDirection(audio_handle_, i, &orientation));

        sound_nodes_[i]->SetWorldTranslation(ml::app_framework::to_glm(position));
        sound_nodes_[i]->SetWorldRotation(ml::app_framework::to_glm(orientation));
      }
    }

    UpdateGui();
  }

private:
  bool FillBuffer() {
    MLAudioBuffer buffer = {};
    MLResult result;
    result = MLAudioGetOutputBuffer(audio_handle_, &buffer);
    if (MLAudioResult_BufferNotReady == result) {
      return false;
    }
    UNWRAP_MLAudioResult(result);

    // ensure we do not read past the end of pcm_file_buffer_
    size_t bytes_to_copy = std::min(static_cast<size_t>(buffer.size), pcm_file_buffer_.size() - buffer_position_);

    std::copy_n(pcm_file_buffer_.data() + buffer_position_, bytes_to_copy, buffer.ptr);
    buffer_position_ += buffer.size;
    buffer_position_ %= pcm_file_buffer_.size();
    UNWRAP_MLAudioResult(MLAudioReleaseOutputBuffer(audio_handle_));
    return true;
  }

  void UpdateGui() {
    ml::app_framework::Gui::GetInstance().BeginUpdate();

    bool is_running{true};
    if (ImGui::Begin("Audio", &is_running, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
      if (ImGui::CollapsingHeader("CONTROL", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Start")) {
          UNWRAP_MLAudioResult(MLAudioStartSound(audio_handle_));
        }
        ImGui::SameLine();

        if (ImGui::Button("Stop")) {
          UNWRAP_MLAudioResult(MLAudioStopSound(audio_handle_));
        }
        ImGui::SameLine();

        if (ImGui::Button("Pause")) {
          UNWRAP_MLAudioResult(MLAudioPauseSound(audio_handle_));
        }
        ImGui::SameLine();

        if (ImGui::Button("Resume")) {
          UNWRAP_MLAudioResult(MLAudioResumeSound(audio_handle_));
        }

        MLAudioState state;
        UNWRAP_MLAudioResult(MLAudioGetSoundState(audio_handle_, &state));

        static constexpr std::array<const char *, 3> audio_state_strs = {"Stopped", "Playing", "Paused"};
        ImGui::Text("SoundState: %s", audio_state_strs[state]);

        MLAudioBufferFormat format = {};
        UNWRAP_MLAudioResult(MLAudioGetSoundFormat(audio_handle_, &format));
        if (ImGui::TreeNodeEx("BufferFormat")) {
          ImGui::Value("channel_count", format.channel_count);
          ImGui::Value("samples_per_second", format.samples_per_second);
          ImGui::Value("bits_per_sample", format.bits_per_sample);
          ImGui::Value("valid_bits_per_sample", format.valid_bits_per_sample);
          static constexpr std::array<const char *, 2> sample_format_strs = {"Float", "Int"};
          ImGui::Value("sample_format", sample_format_strs[format.sample_format]);
          ImGui::Value("reserved", format.reserved);
          ImGui::TreePop();
        }
      }

      if (ImGui::CollapsingHeader("PARAMETERS", ImGuiTreeNodeFlags_DefaultOpen)) {
        float volume = 0;
        UNWRAP_MLAudioResult(MLAudioGetSoundVolumeLinear(audio_handle_, &volume));
        if (ImGui::SliderFloat("SoundVolumeLinear", &volume, 0.f, 8.f)) {
          UNWRAP_MLAudioResult(MLAudioSetSoundVolumeLinear(audio_handle_, volume));
        }

        UNWRAP_MLAudioResult(MLAudioGetSoundVolumeDb(audio_handle_, &volume));
        if (ImGui::SliderFloat("SoundVolumeDb", &volume, -100.f, 18.f)) {
          UNWRAP_MLAudioResult(MLAudioSetSoundVolumeDb(audio_handle_, volume));
        }

        float pitch = 0;
        UNWRAP_MLAudioResult(MLAudioGetSoundPitch(audio_handle_, &pitch));
        if (ImGui::SliderFloat("SoundPitch", &pitch, 0.5f, 2.f)) {
          UNWRAP_MLAudioResult(MLAudioSetSoundPitch(audio_handle_, pitch));
        }

        bool is_muted = false;
        UNWRAP_MLAudioResult(MLAudioIsSoundMuted(audio_handle_, &is_muted));
        if (ImGui::Checkbox("SoundMuted", &is_muted)) {
          UNWRAP_MLAudioResult(MLAudioSetSoundMute(audio_handle_, is_muted));
        }

        MLAudioOutputDevice current_device = MLAudioOutputDevice_Wearable;
        UNWRAP_MLAudioResult(MLAudioGetOutputDevice(&current_device));
        constexpr std::array<const char *, 3> output_device_strs = {"Wearable", "AnalogJack"};
        ImGui::Text("OutputDevice: %s", output_device_strs[current_device]);
      }

      if (ImGui::CollapsingHeader("BUFFERING", ImGuiTreeNodeFlags_DefaultOpen)) {
        float latency_in_msec = 0;
        UNWRAP_MLAudioResult(MLAudioGetBufferedOutputLatency(audio_handle_, &latency_in_msec));
        ImGui::Value("OutputStreamLatency(ms)", latency_in_msec);

        uint64_t frames_played = 0;
        UNWRAP_MLAudioResult(MLAudioGetBufferedOutputFramesPlayed(audio_handle_, &frames_played));
        ImGui::Text("OutputStreamFramesPlayed: %" PRIu64, frames_played);
      }

      if (ImGui::CollapsingHeader("SPATIAL SOUND", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool is_enabled = false;
        UNWRAP_MLAudioResult(MLAudioGetSpatialSoundEnable(audio_handle_, &is_enabled));
        if (ImGui::Checkbox("SetSpatialSoundEnable", &is_enabled)) {
          UNWRAP_MLAudioResult(MLAudioSetSpatialSoundEnable(audio_handle_, is_enabled));
        }

        bool is_head_relative = false;
        UNWRAP_MLAudioResult(MLAudioIsSpatialSoundHeadRelative(audio_handle_, &is_head_relative));
        if (ImGui::Checkbox("SpatialSoundHeadRelative", &is_head_relative)) {
          UNWRAP_MLAudioResult(MLAudioSetSpatialSoundHeadRelative(audio_handle_, is_head_relative));
        }

        std::array<const char *, 2> channel_names = {"Left", "Right"};
        for (int channel = 0; channel < FLAGS_channels; ++channel) {
          if (channel > 0) {
            ImGui::SameLine();
          }
          ImGui::BeginGroup();
          if (FLAGS_channels == 2) {
            // We only need to say which of these panels is "Left" or "Right" if there are 2 channels
            ImGui::Text("%s", channel_names[channel]);
          }
          ImVec2 child_size = {ImGui::GetWindowContentRegionWidth() / float(FLAGS_channels), 0.f};
          if (ImGui::BeginChild(channel_names[channel], child_size, true)) {
            ImGui::PushItemWidth(ImGui::GetWindowWidth() / float(FLAGS_channels));

            if (channel > 0) {
              ImGui::Checkbox("Copy Other Channel's Transform", &copy_channel_transforms_);
            }

            if (channel > 0 && copy_channel_transforms_) {
              MLVec3f position = {};
              UNWRAP_MLAudioResult(MLAudioGetSpatialSoundPosition(audio_handle_, 0, &position));
              UNWRAP_MLAudioResult(MLAudioSetSpatialSoundPosition(audio_handle_, channel, &position));

              MLQuaternionf direction = {};
              UNWRAP_MLAudioResult(MLAudioGetSpatialSoundDirection(audio_handle_, 0, &direction));
              UNWRAP_MLAudioResult(MLAudioSetSpatialSoundDirection(audio_handle_, channel, &direction));

            } else {
              ImGui::Checkbox("Follow Controller", &channel_tracks_controller_[channel]);

              if (!channel_tracks_controller_[channel]) {
                MLVec3f position = {};
                UNWRAP_MLAudioResult(MLAudioGetSpatialSoundPosition(audio_handle_, channel, &position));
                if (ImGui::DragFloat3("Position", &position.x, kDragSpeed)) {
                  UNWRAP_MLAudioResult(MLAudioSetSpatialSoundPosition(audio_handle_, channel, &position));
                }

                MLQuaternionf direction = {};
                UNWRAP_MLAudioResult(MLAudioGetSpatialSoundDirection(audio_handle_, channel, &direction));
                auto angles = glm::degrees(glm::eulerAngles(ml::app_framework::to_glm(direction)));
                if (ImGui::SliderFloat3("Direction", glm::value_ptr(angles), -180.f, 180.f)) {
                  direction = ml::app_framework::to_ml(glm::quat(glm::radians(angles)));
                  UNWRAP_MLAudioResult(MLAudioSetSpatialSoundDirection(audio_handle_, channel, &direction));
                }
              }
            }
            if (ImGui::CollapsingHeader("DistanceProperties")) {
              MLAudioSpatialSoundDistanceProperties distance_props = {};
              UNWRAP_MLAudioResult(MLAudioGetSpatialSoundDistanceProperties(audio_handle_, channel, &distance_props));
              bool update = ImGui::DragFloat("min_distance", &distance_props.min_distance, kDragSpeed);
              update |= ImGui::DragFloat("max_distance", &distance_props.max_distance, kDragSpeed);
              update |= ImGui::DragFloat("rolloff_factor", &distance_props.rolloff_factor, kDragSpeed);
              if (update) {
                UNWRAP_MLAudioResult(MLAudioSetSpatialSoundDistanceProperties(audio_handle_, channel, &distance_props));
              }
            }

            if (ImGui::CollapsingHeader("RadiationProperties")) {
              MLAudioSpatialSoundRadiationProperties radiation_props = {};
              UNWRAP_MLAudioResult(MLAudioGetSpatialSoundRadiationProperties(audio_handle_, channel, &radiation_props));
              bool update = ImGui::SliderFloat("inner_angle", &radiation_props.inner_angle, 0.f, 360.f);
              update |= ImGui::SliderFloat("outer_angle", &radiation_props.outer_angle, 0.f, 360.f);
              update |= ImGui::SliderFloat("outer_gain", &radiation_props.outer_gain, 0.f, 1.f);
              update |= ImGui::SliderFloat("outer_gain_lf", &radiation_props.outer_gain_lf, 0.f, 1.f);
              update |= ImGui::SliderFloat("outer_gain_mf", &radiation_props.outer_gain_mf, 0.f, 1.f);
              update |= ImGui::SliderFloat("outer_gain_hf", &radiation_props.outer_gain_hf, 0.f, 1.f);
              if (update) {
                UNWRAP_MLAudioResult(
                    MLAudioSetSpatialSoundRadiationProperties(audio_handle_, channel, &radiation_props));
              }
            }

            if (ImGui::CollapsingHeader("DirectSendLevels")) {
              MLAudioSpatialSoundSendLevels direct_send_levels = {};
              UNWRAP_MLAudioResult(MLAudioGetSpatialSoundDirectSendLevels(audio_handle_, channel, &direct_send_levels));
              bool update = ImGui::SliderFloat("gain", &direct_send_levels.gain, 0.f, 1.f);
              update |= ImGui::SliderFloat("gain_lf", &direct_send_levels.gain_lf, 0.f, 1.f);
              update |= ImGui::SliderFloat("gain_mf", &direct_send_levels.gain_mf, 0.f, 1.f);
              update |= ImGui::SliderFloat("gain_hf", &direct_send_levels.gain_hf, 0.f, 1.f);
              if (update) {
                UNWRAP_MLAudioResult(
                    MLAudioSetSpatialSoundDirectSendLevels(audio_handle_, channel, &direct_send_levels));
              }
            }

            if (ImGui::CollapsingHeader("RoomSendLevels")) {
              MLAudioSpatialSoundSendLevels room_send_levels = {};
              UNWRAP_MLAudioResult(MLAudioGetSpatialSoundRoomSendLevels(audio_handle_, channel, &room_send_levels));
              bool update = ImGui::SliderFloat("gain", &room_send_levels.gain, 0.f, 1.f);
              update |= ImGui::SliderFloat("gain_lf", &room_send_levels.gain_lf, 0.f, 1.f);
              update |= ImGui::SliderFloat("gain_mf", &room_send_levels.gain_mf, 0.f, 1.f);
              update |= ImGui::SliderFloat("gain_hf", &room_send_levels.gain_hf, 0.f, 1.f);
              if (update) {
                UNWRAP_MLAudioResult(MLAudioSetSpatialSoundRoomSendLevels(audio_handle_, channel, &room_send_levels));
              }
            }
            ImGui::PopItemWidth();
          }
          ImGui::EndChild();
          ImGui::EndGroup();
        }

        if (ImGui::TreeNodeEx("RoomProperties")) {
          MLAudioSpatialSoundRoomProperties room_props = {};
          UNWRAP_MLAudioResult(MLAudioGetSpatialSoundRoomProperties(&room_props));
          bool update = ImGui::SliderFloat("gain", &room_props.gain, 0.f, 1.f);
          update |= ImGui::DragFloat("reflections_delay", &room_props.reflections_delay, kDragSpeed);
          update |= ImGui::SliderFloat("reflections_gain", &room_props.reflections_gain, 0.f, 8.f);
          update |= ImGui::DragFloat("reverb_delay", &room_props.reverb_delay, kDragSpeed);
          update |= ImGui::SliderFloat("reverb_gain", &room_props.reverb_gain, 0.f, 8.f);
          update |= ImGui::DragFloat("reverb_decay_time", &room_props.reverb_decay_time, kDragSpeed);
          update |= ImGui::DragFloat("reverb_decay_time_lf_ratio", &room_props.reverb_decay_time_lf_ratio, kDragSpeed);
          update |= ImGui::DragFloat("reverb_decay_time_hf_ratio", &room_props.reverb_decay_time_hf_ratio, kDragSpeed);
          if (update) {
            UNWRAP_MLAudioResult(MLAudioSetSpatialSoundRoomProperties(&room_props));
          }
          ImGui::TreePop();
        }
      }

      if (ImGui::CollapsingHeader("MASTER VOLUME", ImGuiTreeNodeFlags_DefaultOpen)) {
        float volume = 0;
        UNWRAP_MLAudioResult(MLAudioGetMasterVolume(&volume));
        ImGui::Value("MasterVolume", volume);
      }
    }
    ImGui::End();
    ml::app_framework::Gui::GetInstance().EndUpdate();

    if (!is_running) {
      StopApp();
    }
  }

  MLHandle audio_handle_ = ML_INVALID_HANDLE;
  MLHandle input_tracker_ = ML_INVALID_HANDLE;
  size_t buffer_position_ = 0;
  std::vector<uint8_t> pcm_file_buffer_;
  std::array<std::shared_ptr<ml::app_framework::Node>, 2> sound_nodes_;
  std::array<bool, 2> channel_tracks_controller_ = {true, true};
  bool copy_channel_transforms_ = true;
};

int main(int argc, char **argv) {
  AudioPlaybackApp app(argc, argv);
  app.SetUseGfx(true);
  app.RunApp();
  return 0;
}
