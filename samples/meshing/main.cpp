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
#include <app_framework/components/magicleap_mesh_component.h>
#include <app_framework/material/magicleap_mesh_visualization_material.h>

#include <imgui.h>

#include <gflags/gflags.h>

#include <glad/glad.h>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/gtx/quaternion.hpp>

#include <ml_graphics.h>
#include <ml_head_tracking.h>
#include <ml_input.h>
#include <ml_lifecycle.h>
#include <ml_meshing2.h>
#include <ml_perception.h>

#include <cinttypes>
#include <cstdlib>
#include <unordered_map>
#include <vector>

DEFINE_bool(PointCloud, false, "If set, will return a point cloud instead of a triangle mesh.");
DEFINE_bool(ComputeNormals, false, "If set, the system will compute the normals for the triangle vertices.");
DEFINE_bool(ComputeConfidence, true, "If set, the system will compute the confidence values.");
DEFINE_bool(Planarize, false,
            "If set, the system will planarize the returned mesh (planar regions will be smoothed out).");
DEFINE_bool(RemoveMeshSkirt, false,
            "If set, the mesh skirt (overlapping area between two mesh blocks) will be removed.");
DEFINE_bool(IndexOrderCCW, false,
            "If set, winding order of indices will be be changed from clockwise to counter "
            "clockwise. This could be useful for face culling process in different engines.");

DEFINE_int32(MLMeshingLOD, 1,
             "Level of detail of the block mesh.\n"
             "0:Minimum, 1: Medium, 2: Maximum");

DEFINE_double(fill_hole_length, 3.0, "Perimeter (in meters) of holes you wish to have filled.");
DEFINE_double(disconnected_component_area, 0.5,
              "Any component that is disconnected from the main mesh and which has an area (in "
              "meters^2) less than this size will be removed.");

DEFINE_bool(BoundsFollowUser, false,
            "The query bounds will stay centered at the user's pose, "
            "instead of at the world origin.");

DEFINE_bool(DrawBlockBounds, false,
            "Draw the block boundaries.  It will be colored according to the block status."
            "new = green"
            "updated = orange"
            "unchanged = violet");

namespace std {

template <>
struct hash<MLCoordinateFrameUID> {
  size_t operator()(const MLCoordinateFrameUID &id) const {
    std::size_t h1 = std::hash<uint64_t>{}(id.data[0]);
    std::size_t h2 = std::hash<uint64_t>{}(id.data[1]);
    return h1 ^ (h2 << 1);
  }
};

}  // namespace std

bool operator==(const MLCoordinateFrameUID lhs, const MLCoordinateFrameUID rhs) {
  return lhs.data[0] == rhs.data[0] && lhs.data[1] == rhs.data[1];
}

class MeshingApp : public ml::app_framework::Application {
public:
  MeshingApp(int argc = 0, char **argv = nullptr) : ml::app_framework::Application(argc, argv) {
    request_extents_.center = {0, 0, 0};
    request_extents_.rotation = {0, 0, 0, 1};
    request_extents_.extents = {10, 10, 10};
  }

  void OnStart() override {
    UNWRAP_MLRESULT(MLMeshingInitSettings(&meshing_settings_));
    UNWRAP_MLRESULT(MLHeadTrackingCreate(&head_tracker_));
    UNWRAP_MLRESULT(MLHeadTrackingGetStaticData(head_tracker_, &head_static_data_));

    bounds_follow_user_ = FLAGS_BoundsFollowUser;
    draw_block_bounds_ = FLAGS_DrawBlockBounds;
    meshing_settings_.fill_hole_length = FLAGS_fill_hole_length;
    meshing_settings_.disconnected_component_area = FLAGS_disconnected_component_area;
    meshing_settings_.flags = (FLAGS_PointCloud ? MLMeshingFlags_PointCloud : 0) |
                              (FLAGS_ComputeNormals ? MLMeshingFlags_ComputeNormals : 0) |
                              (FLAGS_ComputeConfidence ? MLMeshingFlags_ComputeConfidence : 0) |
                              (FLAGS_Planarize ? MLMeshingFlags_Planarize : 0) |
                              (FLAGS_RemoveMeshSkirt ? MLMeshingFlags_RemoveMeshSkirt : 0) |
                              (FLAGS_IndexOrderCCW ? MLMeshingFlags_IndexOrderCCW : 0);
    UNWRAP_MLRESULT(MLMeshingCreateClient(&meshing_client_, &meshing_settings_));
    meshing_lod_ = static_cast<MLMeshingLOD>(FLAGS_MLMeshingLOD);
    mesh_mat_ = std::make_shared<ml::app_framework::MagicLeapMeshVisualizationMaterial>();
    geom_shader_ = mesh_mat_->GetGeometryProgram();

    extents_node_ = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Cube);
    auto extents_renderable = extents_node_->GetComponent<ml::app_framework::RenderableComponent>();
    auto extents_material =
        std::static_pointer_cast<ml::app_framework::FlatMaterial>(extents_renderable->GetMaterial());
    extents_material->SetOverrideVertexColor(true);
    extents_material->SetColor(blue_);
    GetRoot()->AddChild(extents_node_);

    UpdateMaterial();

    ml::app_framework::Gui::GetInstance().Initialize(GetGraphicsContext());
    GetRoot()->AddChild(ml::app_framework::Gui::GetInstance().GetNode());

    UNWRAP_MLRESULT(MLInputCreate(nullptr, &input_tracker_));

    MLInputControllerCallbacksEx controller_callbacks = {};
    controller_callbacks.on_touchpad_gesture_end =
        [](uint8_t controller_id, const MLInputControllerTouchpadGesture *touchpad_gesture, void *data) {
          MeshingApp *app = static_cast<MeshingApp *>(data);
          if (app) {
            ML_LOG(Debug, "on_touchpad_gesture_end(controller_id:%d, data:0x%zX)", controller_id, (size_t)data);
            if (touchpad_gesture->type == MLInputControllerTouchpadGestureType_Swipe) {
              app->meshing_lod_ = static_cast<MLMeshingLOD>((app->meshing_lod_ + 1) % 3);
              ML_LOG(Debug, "swipe up performed, change LOD, meshing_lod_: %d", app->meshing_lod_);
            }
          }
        };

    controller_callbacks.on_button_up = [](uint8_t controller_id, MLInputControllerButton button, void *data) {
      MeshingApp *app = static_cast<MeshingApp *>(data);
      if (app) {
        ML_LOG(Debug, "on_button_up(controller_id:%d, button: %d, data:0x%zX)", controller_id, button, (size_t)data);
        if (button == MLInputControllerButton_HomeTap) {
          app->trigger_state_ = (app->trigger_state_ + 1) % 3;
          ML_LOG(Debug, "home tap pressed, change bounding box, trigger_state_: %s",
                 app->trigger_states_[app->trigger_state_].c_str());
          app->bounds_follow_user_ = false;
          app->request_extents_.center = {0, 0, 0};
          app->request_extents_.rotation = {0, 0, 0, 1};

          if (app->trigger_state_ == 0) {  // follow user
            app->bounds_follow_user_ = true;
          } else if (app->trigger_state_ == 1) {  // 3 meters
            app->request_extents_.extents = {3, 3, 3};
          } else {  // 10 meters
            app->request_extents_.extents = {10, 10, 10};
          }
        }
      }
    };

    UNWRAP_MLRESULT(MLInputSetControllerCallbacksEx(input_tracker_, &controller_callbacks, this));
  }

  void OnStop() override {
    ml::app_framework::Gui::GetInstance().Cleanup();
    mesh_block_nodes_.clear();
    UNWRAP_MLRESULT(MLMeshingDestroyClient(&meshing_client_));
    UNWRAP_MLRESULT(MLInputDestroy(input_tracker_));
  }

  void OnUpdate(float) override {
    UpdateGui();

    if (bounds_follow_user_) {
      MLSnapshot *snapshot = nullptr;
      MLTransform head_transform = {};
      UNWRAP_MLRESULT(MLPerceptionGetSnapshot(&snapshot));
      UNWRAP_MLRESULT(MLSnapshotGetTransform(snapshot, &head_static_data_.coord_frame_head, &head_transform));
      UNWRAP_MLRESULT(MLPerceptionReleaseSnapshot(snapshot));

      request_extents_.center = head_transform.position;
    }

    extents_node_->SetLocalTranslation(ml::app_framework::to_glm(request_extents_.center));
    extents_node_->SetLocalRotation(ml::app_framework::to_glm(request_extents_.rotation));
    extents_node_->SetLocalScale(ml::app_framework::to_glm(request_extents_.extents));

    // Request mesh info
    if (!MLHandleIsValid(current_mesh_info_request_)) {
      UNWRAP_MLRESULT(MLMeshingRequestMeshInfo(meshing_client_, &request_extents_, &current_mesh_info_request_));
    }

    // Request meshes
    if (!MLHandleIsValid(current_mesh_request_) && !block_requests_.empty()) {
      MLMeshingMeshRequest mesh_request = {};
      mesh_request.request_count = int(block_requests_.size());
      mesh_request.data = block_requests_.data();
      UNWRAP_MLRESULT(MLMeshingRequestMesh(meshing_client_, &mesh_request, &current_mesh_request_));
    }

    // Poll mesh info result
    if (MLHandleIsValid(current_mesh_info_request_)) {
      MLMeshingMeshInfo mesh_info = {};
      MLResult result = MLMeshingGetMeshInfoResult(meshing_client_, current_mesh_info_request_, &mesh_info);

      if (MLResult_Ok == result) {
        UpdateBlockRequests(mesh_info);
        MLMeshingFreeResource(meshing_client_, &current_mesh_info_request_);
        current_mesh_info_request_ = ML_INVALID_HANDLE;
      } else if (MLResult_Pending != result) {
        UNWRAP_MLRESULT(result);
        current_mesh_info_request_ = ML_INVALID_HANDLE;
      }
    }

    // Poll mesh result
    if (MLHandleIsValid(current_mesh_request_)) {
      MLMeshingMesh mesh = {};
      MLResult result = MLMeshingGetMeshResult(meshing_client_, current_mesh_request_, &mesh);

      if (MLResult_Ok == result) {
        UpdateBlocks(mesh);
        MLMeshingFreeResource(meshing_client_, &current_mesh_request_);
        current_mesh_request_ = ML_INVALID_HANDLE;
      } else if (MLResult_Pending != result) {
        UNWRAP_MLRESULT(result);
        current_mesh_request_ = ML_INVALID_HANDLE;
      }
    }

    // Set visibiliy of the block boundaries cubes
    if (draw_block_bounds_dirty_) {
      for (auto &block : mesh_block_nodes_) {
        if (block.second.bounds) {
          auto bounds_renderable = block.second.bounds->GetComponent<ml::app_framework::RenderableComponent>();
          bounds_renderable->SetVisible((draw_block_bounds_) ? true : false);
        }
      }
      draw_block_bounds_dirty_ = false;
    }
  };

private:
  void UpdateBlockRequests(const MLMeshingMeshInfo &mesh_info) {
    block_requests_.clear();
    for (size_t i = 0; i < mesh_info.data_count; ++i) {
      MLMeshingBlockRequest request = {};
      request.id = mesh_info.data[i].id;
      request.level = meshing_lod_;
      switch (mesh_info.data[i].state) {
        case MLMeshingMeshState_New: {
          block_requests_.push_back(request);
          std::shared_ptr<ml::app_framework::Node> new_block = std::make_shared<ml::app_framework::Node>();
          {
            using namespace ml::app_framework;
            std::shared_ptr<MagicLeapMeshComponent> mesh_comp = std::make_shared<MagicLeapMeshComponent>();
            std::shared_ptr<RenderableComponent> renderable =
                std::make_shared<RenderableComponent>(mesh_comp->GetMesh(), mesh_mat_);
            new_block->AddComponent(renderable);
            new_block->AddComponent(mesh_comp);
            UpdateNodeRenderOption(new_block);
            GetRoot()->AddChild(new_block);

            auto bounds_node = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Cube);
            auto bounds_renderable = bounds_node->GetComponent<ml::app_framework::RenderableComponent>();
            bounds_renderable->SetVisible((draw_block_bounds_) ? true : false);

            auto bounds_material =
                std::static_pointer_cast<ml::app_framework::FlatMaterial>(bounds_renderable->GetMaterial());
            bounds_material->SetOverrideVertexColor(true);
            bounds_material->SetColor(green_);
            bounds_node->SetWorldTranslation(to_glm(mesh_info.data[i].extents.center));
            bounds_node->SetWorldRotation(to_glm(mesh_info.data[i].extents.rotation));
            bounds_node->SetLocalScale(glm::vec3(mesh_info.data[i].extents.extents.x,
                                                 mesh_info.data[i].extents.extents.y,
                                                 mesh_info.data[i].extents.extents.z));
            GetRoot()->AddChild(bounds_node);

            MeshBlockNodes nodes = {new_block, bounds_node};

            auto insert_result = mesh_block_nodes_.insert(std::make_pair(mesh_info.data[i].id, nodes));
            if (!insert_result.second) {
              ML_LOG(Verbose, "Insertion of block failed, already exists: %s",
                     ml::app_framework::to_string(mesh_info.data[i].id).c_str());
            }
          }

          break;
        }
        case MLMeshingMeshState_Updated: {
          block_requests_.push_back(request);
          auto mesh_block_iter = mesh_block_nodes_.find(mesh_info.data[i].id);
          if (mesh_block_iter == mesh_block_nodes_.end()) {
            ML_LOG(Error, "mesh_info state updated: nonexistant block %s", ml::app_framework::to_string(mesh_info.data[i].id).c_str());
          } else {
            auto &block = (*mesh_block_iter).second;
            auto bounds_renderable = block.bounds->GetComponent<ml::app_framework::RenderableComponent>();
            auto bounds_material =
                std::static_pointer_cast<ml::app_framework::FlatMaterial>(bounds_renderable->GetMaterial());
            bounds_material->SetColor(orange_);
          }
          break;
        }
        case MLMeshingMeshState_Deleted: {
          auto to_remove = mesh_block_nodes_.find(mesh_info.data[i].id);
          if (to_remove != mesh_block_nodes_.end()) {
            GetRoot()->RemoveChild(to_remove->second.contents);
            GetRoot()->RemoveChild(to_remove->second.bounds);
            mesh_block_nodes_.erase(to_remove);
          }
          break;
        }
        case MLMeshingMeshState_Unchanged: {
          auto mesh_block_iter = mesh_block_nodes_.find(mesh_info.data[i].id);
          if (mesh_block_iter == mesh_block_nodes_.end()) {
            ML_LOG(Error, "mesh_info state unchanged: nonexistant block %s", ml::app_framework::to_string(mesh_info.data[i].id).c_str());
          } else {
            auto &block = (*mesh_block_iter).second;
            auto bounds_renderable = block.bounds->GetComponent<ml::app_framework::RenderableComponent>();
            auto bounds_material =
                std::static_pointer_cast<ml::app_framework::FlatMaterial>(bounds_renderable->GetMaterial());
            bounds_material->SetColor(violet_);
          }
          break;
        }
        default: break;
      }
    }
  }

  void UpdateBlocks(const MLMeshingMesh &mesh) {
    for (size_t i = 0; i < mesh.data_count; ++i) {
      auto mesh_block_iter = mesh_block_nodes_.find(mesh.data[i].id);
      if (mesh_block_iter == mesh_block_nodes_.end()) {
        ML_LOG(Error, "Tried to Update nonexistant block %s", ml::app_framework::to_string(mesh.data[i].id).c_str());
        continue;
      }
      mesh_block_iter->second.contents->GetComponent<ml::app_framework::MagicLeapMeshComponent>()
          ->UpdateMeshWithConfidence(reinterpret_cast<glm::vec3 *>(mesh.data[i].vertex),
                                     reinterpret_cast<glm::vec3 *>(mesh.data[i].normal), mesh.data[i].confidence,
                                     mesh.data[i].vertex_count, mesh.data[i].index, mesh.data[i].index_count);
    }
  }

  void UpdateNodeRenderOption(std::shared_ptr<ml::app_framework::Node> node) {
    using namespace ml::app_framework;
    const auto &renderable = node->GetComponent<RenderableComponent>();
    if (!(meshing_settings_.flags & MLMeshingFlags_PointCloud)) {
      renderable->GetMaterial()->SetPolygonMode(GL_LINE);
      renderable->GetMesh()->SetPrimitiveType(GL_TRIANGLES);
    } else {
      renderable->GetMesh()->SetPrimitiveType(GL_POINTS);
      renderable->GetMesh()->SetPointSize(8);
    }
  }

  void UpdateMaterial() {
    if (meshing_settings_.flags & MLMeshingFlags_PointCloud ||
        !(meshing_settings_.flags & MLMeshingFlags_ComputeNormals)) {
      mesh_mat_->SetGeometryProgram(nullptr);
    } else {
      mesh_mat_->SetGeometryProgram(geom_shader_);
    }
  }

  void UpdateGui() {
    ml::app_framework::Gui::GetInstance().BeginUpdate();
    if (ImGui::Begin("Meshing")) {
      if (ImGui::CollapsingHeader("MLMeshingExtents", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &re = request_extents_;

        ImGui::Checkbox("BoundsFollowUser", &bounds_follow_user_);
        bool bb_prev = draw_block_bounds_;
        ImGui::Checkbox("DrawBlockBoundaries", &draw_block_bounds_);
        if (bb_prev != draw_block_bounds_) {
          draw_block_bounds_dirty_ = true;
        }
        if (!bounds_follow_user_) {
          ImGui::SliderFloat3("center", &re.center.x, -20.f, 20.f);
        }

        auto angles = glm::degrees(glm::eulerAngles(ml::app_framework::to_glm(re.rotation)));
        ImGui::SliderFloat3("rotation", glm::value_ptr(angles), -180.f, 180.f);
        re.rotation = ml::app_framework::to_ml(glm::quat(glm::radians(angles)));
        ImGui::SliderFloat3("extents", &re.extents.x, 0.f, 20.f);
      }

      if (ImGui::CollapsingHeader("MLMeshingMeshRequest", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char *meshing_lod_options[] = {"Minimum", "Medium", "Maximum"};
        auto *level = reinterpret_cast<int *>(&meshing_lod_);
        ImGui::Combo("MLMeshingLOD", level, meshing_lod_options, IM_ARRAYSIZE(meshing_lod_options));
      }

      if (ImGui::CollapsingHeader("MLMeshingUpdateSettings", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool update = false;
        update |= ImGui::SliderFloat("fill_hole_length", &meshing_settings_.fill_hole_length, 0.0f, 10.0f);
        update |= ImGui::SliderFloat("disconnected_component_area", &meshing_settings_.disconnected_component_area,
                                     0.0f, 10.0f);

        if (ImGui::TreeNodeEx("flags", ImGuiTreeNodeFlags_DefaultOpen)) {
          auto *flags = reinterpret_cast<unsigned int *>(&meshing_settings_.flags);
          update |= ImGui::CheckboxFlags("PointCloud", flags, MLMeshingFlags_PointCloud);
          update |= ImGui::CheckboxFlags("ComputeNormals", flags, MLMeshingFlags_ComputeNormals);
          update |= ImGui::CheckboxFlags("ComputeConfidence", flags, MLMeshingFlags_ComputeConfidence);
          update |= ImGui::CheckboxFlags("Planarize", flags, MLMeshingFlags_Planarize);
          update |= ImGui::CheckboxFlags("RemoveMeshSkirt", flags, MLMeshingFlags_RemoveMeshSkirt);
          update |= ImGui::CheckboxFlags("IndexOrderCCW", flags, MLMeshingFlags_IndexOrderCCW);
          ImGui::TreePop();
        }

        if (update) {
          UNWRAP_MLRESULT(MLMeshingUpdateSettings(meshing_client_, &meshing_settings_));
          UpdateMaterial();
          for (const auto &pair : mesh_block_nodes_) {
            UpdateNodeRenderOption(pair.second.contents);
          }
        }
      }
    }
    ImGui::End();
    ml::app_framework::Gui::GetInstance().EndUpdate();
  }

  MLHandle meshing_client_ = ML_INVALID_HANDLE;
  MLHandle current_mesh_info_request_ = ML_INVALID_HANDLE;
  MLHandle current_mesh_request_ = ML_INVALID_HANDLE;
  MLMeshingSettings meshing_settings_ = {};
  MLMeshingLOD meshing_lod_ = MLMeshingLOD_Medium;
  MLMeshingExtents request_extents_ = {};
  std::vector<MLMeshingBlockRequest> block_requests_;

  struct MeshBlockNodes {
    std::shared_ptr<ml::app_framework::Node> contents;
    std::shared_ptr<ml::app_framework::Node> bounds;
  };

  std::unordered_map<MLCoordinateFrameUID, MeshBlockNodes> mesh_block_nodes_;

  std::shared_ptr<ml::app_framework::GeometryProgram> geom_shader_;
  std::shared_ptr<ml::app_framework::MagicLeapMeshVisualizationMaterial> mesh_mat_;
  std::shared_ptr<ml::app_framework::Node> extents_node_;

  MLHandle head_tracker_ = ML_INVALID_HANDLE;
  MLHeadTrackingStaticData head_static_data_ = {};
  bool bounds_follow_user_;
  bool draw_block_bounds_;
  MLHandle input_tracker_ = ML_INVALID_HANDLE;

  std::array<std::string, 3> trigger_states_ = {{"follow user", "3 meters", "10 meters"}};
  int trigger_state_ = 0;
  bool draw_block_bounds_dirty_;
  const glm::vec4 green_ = glm::vec4(.0f, 1.0f, .0f, 1.0f);
  const glm::vec4 blue_ = glm::vec4(.0f, .0f, 1.0f, 1.0f);
  const glm::vec4 violet_ = glm::vec4(1.0f, .0f, .75f, 1.0f);
  const glm::vec4 orange_ = glm::vec4(1.0f, .5f, .0f, 1.0f);
};

int main(int argc, char **argv) {
  MeshingApp app(argc, argv);
  app.SetUseGfx(true);
  app.RunApp();
  return 0;
}
