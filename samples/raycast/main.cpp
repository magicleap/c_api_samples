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

#include <imgui.h>

#include <gflags/gflags.h>

#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>

#include <ml_controller.h>
#include <ml_head_tracking.h>
#include <ml_logging.h>
#include <ml_perception.h>
#include <ml_raycast.h>

DEFINE_int32(width, 1, "The number of horizontal rays. For single point raycast, set this to 1.");
DEFINE_int32(height, 1, "The number of vertical rays. For single point raycast, set this to 1.");
DEFINE_double(horizontal_fov_degrees, 45.f, "The horizontal field of view, in degrees.");
DEFINE_bool(collide_with_unobserved, true,
            "If true, a ray will terminate when encountering an unobserved area and return a "
            "surface or the ray will continue until it ends or hits a observed surface.");

class RaycastApp : public ml::app_framework::Application {
public:
  RaycastApp(int argc = 0, char **argv = nullptr) : ml::app_framework::Application(argc, argv) {}

  void OnStart() override {
    cube_ = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Cube);
    GetRoot()->AddChild(cube_);

    raycast_query_.position = MLVec3f{0.f, 0.f, 0.f};
    raycast_query_.direction = MLVec3f{0.f, 0.f, 0.f};
    raycast_query_.up_vector = MLVec3f{0.f, 1.f, 0.f};
    raycast_query_.width = FLAGS_width;
    raycast_query_.height = FLAGS_height;
    raycast_query_.horizontal_fov_degrees = FLAGS_horizontal_fov_degrees;
    raycast_query_.collide_with_unobserved = FLAGS_collide_with_unobserved;

    UNWRAP_MLRESULT(MLHeadTrackingCreate(&head_tracker_));
    UNWRAP_MLRESULT(MLHeadTrackingGetStaticData(head_tracker_, &head_static_data_));
    UNWRAP_MLRESULT(MLRaycastCreate(&raycast_tracker_));

    ml::app_framework::Gui::GetInstance().Initialize(GetGraphicsContext());
    GetRoot()->AddChild(ml::app_framework::Gui::GetInstance().GetNode());

    MLControllerConfiguration controller_config = {};
    controller_config.enable_imu3dof = false;
    controller_config.enable_em6dof = false;
    controller_config.enable_fused6dof = true;
    UNWRAP_MLRESULT(MLControllerCreateEx(&controller_config, &controller_tracker_));

    pointer_ = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Line);
    auto line_renderable = pointer_->GetComponent<ml::app_framework::RenderableComponent>();
    line_renderable->GetMesh()->SetPrimitiveType(GL_LINES);
    auto line_material = std::static_pointer_cast<ml::app_framework::FlatMaterial>(line_renderable->GetMaterial());
    line_material->SetColor(glm::vec4(.0f, .0f, 1.0f, 1.0f));

    GetRoot()->AddChild(pointer_);
  }

  void OnStop() override {
    ml::app_framework::Gui::GetInstance().Cleanup();
    UNWRAP_MLRESULT(MLControllerDestroy(controller_tracker_));
    UNWRAP_MLRESULT(MLRaycastDestroy(raycast_tracker_));
    UNWRAP_MLRESULT(MLHeadTrackingDestroy(head_tracker_));
  }

  void OnUpdate(float) override {
    MLSnapshot *snapshot = nullptr;
    UNWRAP_MLRESULT(MLPerceptionGetSnapshot(&snapshot));
    MLTransform head_transform = {};
    UNWRAP_MLRESULT(MLSnapshotGetTransform(snapshot, &head_static_data_.coord_frame_head, &head_transform));

    MLControllerSystemState system_state = {};
    UNWRAP_MLRESULT(MLControllerGetState(controller_tracker_, &system_state));
    auto &controller = system_state.controller_state[0]; //controller 0
    auto &stream = controller.stream[2];  //fused 6dof stream
    MLTransform controller_transform = {};
    UNWRAP_MLRESULT(MLSnapshotGetTransform(snapshot, &stream.coord_frame_controller, &controller_transform));

    bool should_request = auto_request_;
    UpdateGui(should_request);
    if (MLHandleIsValid(raycast_request_)) {
      ReceiveRayRequest(controller_transform);
    } else if (should_request) {
      RequestRay(controller_transform.position,
                 ml::app_framework::to_ml(ml::app_framework::to_glm(controller_transform.rotation) * glm::vec3(0, 0, -1)));
    }

    UNWRAP_MLRESULT(MLPerceptionReleaseSnapshot(snapshot));
  }

private:
  void UpdateLine(std::shared_ptr<ml::app_framework::Node> &node, const glm::vec3 *verts, int verts_size) {
    auto line_renderable = node->GetComponent<ml::app_framework::RenderableComponent>();
    line_renderable->GetMesh()->UpdateMesh(verts, nullptr, verts_size, nullptr, 0);
  }

  void RequestRay(const MLVec3f &position, const MLVec3f &direction) {
    MLRaycastQuery query = raycast_query_;
    query.position = position;
    query.direction = direction;
    query.up_vector = MLVec3f{0.f, 1.f, 0.f};
    UNWRAP_MLRESULT(MLRaycastRequest(raycast_tracker_, &query, &raycast_request_));
    ML_LOG(Verbose, "Requested raycast");
  }

  void ReceiveRayRequest(const MLTransform &controller_transform) {
    MLResult result = MLRaycastGetResult(raycast_tracker_, raycast_request_, &raycast_result_);
    if (MLResult_Ok == result) {
      if (MLRaycastResultState_HitObserved == raycast_result_.state ||
          MLRaycastResultState_HitUnobserved == raycast_result_.state) {
        glm::vec3 hitpoint = ml::app_framework::to_glm(raycast_result_.hitpoint);
        glm::vec3 normal = ml::app_framework::to_glm(raycast_result_.normal);

        cube_->SetWorldTranslation(hitpoint + 0.1f * normal);
        cube_->SetWorldRotation(glm::orientation(normal, {0.f, 1.f, 0.f}));
        cube_->SetLocalScale(glm::vec3{0.2f, 0.2f, 0.2f});

        glm::vec3 verts[] = {
          {controller_transform.position.x, controller_transform.position.y, controller_transform.position.z},
          {hitpoint.x, hitpoint.y, hitpoint.z}
        };
        UpdateLine(pointer_, verts, sizeof(verts)/sizeof(glm::vec3));
      }

      raycast_request_ = ML_INVALID_HANDLE;
    } else if (MLResult_Pending == result) {
      ML_LOG(Verbose, "PENDING!");
    } else {
      UNWRAP_MLRESULT(result);
    }
  }

  void UpdateGui(bool &should_request) {
    ml::app_framework::Gui::GetInstance().BeginUpdate();
    if (ImGui::Begin("Raycast")) {
      if (ImGui::CollapsingHeader("MLRaycastQuery", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderInt("width", reinterpret_cast<int *>(&raycast_query_.width), 1, 100);
        ImGui::SliderInt("height", reinterpret_cast<int *>(&raycast_query_.height), 1, 100);
        raycast_query_.horizontal_fov_degrees = glm::radians(raycast_query_.horizontal_fov_degrees);
        ImGui::SliderAngle("horizontal_fov_degrees", &raycast_query_.horizontal_fov_degrees, 0.f, 180.f);
        raycast_query_.horizontal_fov_degrees = glm::degrees(raycast_query_.horizontal_fov_degrees);
        ImGui::Checkbox("collide_with_unobserved", &raycast_query_.collide_with_unobserved);
      }
      if (ImGui::CollapsingHeader("MLRaycastRequest", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Auto Request", &auto_request_);
        if (!should_request) {
          should_request = ImGui::Button("Send Request");
        }
      }
      if (ImGui::CollapsingHeader("MLRaycastResult", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Value("confidence", raycast_result_.confidence, "%.2f");

        const char *state_str = "";
        switch (raycast_result_.state) {
          case MLRaycastResultState_RequestFailed: state_str = "RequestFailed"; break;
          case MLRaycastResultState_NoCollision: state_str = "NoCollision"; break;
          case MLRaycastResultState_HitUnobserved: state_str = "HitUnobserved"; break;
          case MLRaycastResultState_HitObserved: state_str = "HitObserved"; break;
          default: state_str = "INVALID"; break;
        };
        ImGui::LabelText("state", "%s", state_str);
      }
    }
    ImGui::End();
    ml::app_framework::Gui::GetInstance().EndUpdate();
  }

  std::shared_ptr<ml::app_framework::Node> cube_;
  MLHandle controller_tracker_ = ML_INVALID_HANDLE;
  MLHandle head_tracker_ = ML_INVALID_HANDLE;
  MLHeadTrackingStaticData head_static_data_ = {};
  MLHandle raycast_tracker_ = ML_INVALID_HANDLE;
  MLHandle raycast_request_ = ML_INVALID_HANDLE;
  MLRaycastQuery raycast_query_ = {};
  MLRaycastResult raycast_result_ = {};
  std::shared_ptr<ml::app_framework::Node> pointer_;

  bool auto_request_ = true;
};

int main(int argc, char **argv) {
  RaycastApp app(argc, argv);
  app.SetUseGfx(true);
  app.RunApp();
  return 0;
}
