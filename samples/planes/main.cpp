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
#include <glm/gtx/transform.hpp>

#include <ml_head_tracking.h>
#include <ml_input.h>
#include <ml_logging.h>
#include <ml_perception.h>
#include <ml_planes.h>
#include <ml_raycast.h>

#include <array>

DEFINE_bool(RenderBoundaries, false, "Include planes boundaries in the results.");

DEFINE_double(min_hole_length, 0.5,
              "If #MLPlanesQueryFlag_IgnoreHoles is set to false, holes with a perimeter (in meters) smaller "
              "than this value will be ignored, and can be part of the plane. This value cannot be lower "
              "than 0 (lower values will be capped to this minimum). A good default value is 0.5.");
DEFINE_double(min_plane_area, 0.25,
              "The minimum area (in squared meters) of planes to be returned. This value cannot be lower "
              "than 0.04 (lower values will be capped to this minimum). A good default value is 0.25.");

static constexpr uint32_t MAX_PLANES = 25;
static constexpr uint32_t MAX_LINES = 25;
static constexpr uint32_t MAX_HOLES = 25;

class PlanesApp : public ml::app_framework::Application {
public:
  PlanesApp(int argc = 0, char **argv = nullptr) : ml::app_framework::Application(argc, argv) {}

  void OnStart() override {
    for (auto &plane : planes_) {
      plane = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Quad);
      GetRoot()->AddChild(plane);
    }
    for (auto &line : lines_) {
      line = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Line);
      GetRoot()->AddChild(line);
    }
    for (auto &hole : holes_) {
      hole = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Line);
      GetRoot()->AddChild(hole);
    }
    UNWRAP_MLRESULT(MLHeadTrackingCreate(&head_tracker_));
    UNWRAP_MLRESULT(MLHeadTrackingGetStaticData(head_tracker_, &head_static_data_));
    UNWRAP_MLRESULT(MLPlanesCreate(&planes_tracker_));
    UNWRAP_MLRESULT(MLInputCreate(nullptr, &input_tracker_));
    UNWRAP_MLRESULT(MLRaycastCreate(&raycast_tracker_));

    content_node_ = std::make_shared<ml::app_framework::Node>();
    GetRoot()->AddChild(content_node_);

    auto model = ml::app_framework::Registry::GetInstance()->GetResourcePool()->LoadAsset("data/sphere.obj");
    model->SetLocalRotation(glm::quat{glm::vec3{0.f, glm::pi<float>(), 0.f}});
    content_node_->AddChild(model);

    pointer_ = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Line);
    auto line_renderable = pointer_->GetComponent<ml::app_framework::RenderableComponent>();
    line_renderable->GetMesh()->SetPrimitiveType(GL_LINES);
    auto line_material = std::static_pointer_cast<ml::app_framework::FlatMaterial>(line_renderable->GetMaterial());
    line_material->SetColor(glm::vec4(.0f, .0f, 1.0f, 1.0f));

    GetRoot()->AddChild(pointer_);
  }

  void OnStop() override {
    UNWRAP_MLRESULT(MLInputDestroy(input_tracker_));
    UNWRAP_MLRESULT(MLPlanesDestroy(planes_tracker_));
    UNWRAP_MLRESULT(MLRaycastDestroy(raycast_tracker_));
    UNWRAP_MLRESULT(MLHeadTrackingDestroy(head_tracker_));
  }

  void OnUpdate(float) override {
    MLSnapshot *snapshot = nullptr;
    UNWRAP_MLRESULT(MLPerceptionGetSnapshot(&snapshot));
    if (MLHandleIsValid(planes_request_)) {
      if (FLAGS_RenderBoundaries) {
        ML_LOG(Verbose, "Trying to query for planes boundaries.");
        ReceivePlanesBoundariesRequest();
      } else {
        ML_LOG(Verbose, "Trying to query for normal planes.");
        ReceivePlanesRequest();
      }
    } else {
      MLTransform head_transform = {};
      UNWRAP_MLRESULT(MLSnapshotGetTransform(snapshot, &head_static_data_.coord_frame_head, &head_transform));
      RequestPlanes(head_transform);
    }

    HandleInput();

    UNWRAP_MLRESULT(MLPerceptionReleaseSnapshot(snapshot));
  }

private:
  class Ray
  {
  public:
    Ray(const glm::vec3 &orig, const glm::vec3 &dir) : orig(orig), dir(dir)
    {
      sign[0] = ((1/dir.x) < 0);
      sign[1] = ((1/dir.y) < 0);
      sign[2] = ((1/dir.z) < 0);
    }
    glm::vec3 orig, dir;
    int sign[3];
  };

  // Implementation based on following
  //   Amy Williams, Steve Barrus, R. Keith Morley, and Peter Shirley
  //   "An Efficient and Robust Ray-Box Intersection Algorithm"
  //   Journal of graphics tools, 10(1):49-54, 2005
  //
  bool intersect(const Ray &r, glm::vec3 *bounds) const {
    float tmin, tmax, tymin, tymax, tzmin, tzmax;

    tmin = (bounds[r.sign[0]].x - r.orig.x) * (1/r.dir.x);
    tmax = (bounds[1-r.sign[0]].x - r.orig.x) * (1/r.dir.x);
    tymin = (bounds[r.sign[1]].y - r.orig.y) * (1/r.dir.y);
    tymax = (bounds[1-r.sign[1]].y - r.orig.y) * (1/r.dir.y);

    if ((tmin > tymax) || (tymin > tmax))
      return false;
    if (tymin > tmin)
      tmin = tymin;
    if (tymax < tmax)
      tmax = tymax;

    tzmin = (bounds[r.sign[2]].z - r.orig.z) * (1/r.dir.z);
    tzmax = (bounds[1-r.sign[2]].z - r.orig.z) * (1/r.dir.z);

    if ((tmin > tzmax) || (tzmin > tmax))
      return false;
    if (tzmin > tmin)
      tmin = tzmin;
    if (tzmax < tmax)
      tmax = tzmax;

    return true;
  }

  void HandleInput() {
    UNWRAP_MLRESULT(MLInputGetControllerState(input_tracker_, input_state_));
    // Using the first connected contoller
    const auto &state = input_state_[0];
    MLTransform controller_transform{state.orientation, state.position};
    if (state.is_connected) {
      if (state.trigger_normalized > .75f && can_place_content_) {
        can_place_content_ = false;
        OnTriggerDown(controller_transform);
      }

      if (state.trigger_normalized <= .0f) {
        can_place_content_ = true;
      }

      if (MLHandleIsValid(raycast_request_)) {
        ReceiveRayRequest(controller_transform);
      } else {
        RequestRay(controller_transform.position,
                   ml::app_framework::to_ml(ml::app_framework::to_glm(controller_transform.rotation) * glm::vec3(0, 0, -1)));
      }
    }
  }

  void OnTriggerDown(const MLTransform &controller_transform) {
    for (size_t i = 0; i < num_planes_; ++i) {
      if (ml_planes_[i].flags & MLPlanesQueryFlag_Semantic_Floor) {
        // setup the bounding box for the plane
        glm::vec3 bounds[2];
        bounds[0].x = ml_planes_[i].position.x - (ml_planes_[i].width/2);
        bounds[0].y = ml_planes_[i].position.y;
        bounds[0].z = ml_planes_[i].position.z - (ml_planes_[i].height/2);

        bounds[1].x = ml_planes_[i].position.x + (ml_planes_[i].width/2);
        bounds[1].y = ml_planes_[i].position.y;
        bounds[1].z = ml_planes_[i].position.z + (ml_planes_[i].height/2);

        // perform intersection test on AABB of horizontal plane
        bool pointing_at_plane = intersect(
            Ray(ml::app_framework::to_glm(controller_transform.position),
                ml::app_framework::to_glm(controller_transform.rotation) * glm::vec3(0, 0, -1)),
            bounds);

        if (pointing_at_plane) {
          ML_LOG(Debug, "pointing_at_plane: %d plane_id: %llu", pointing_at_plane, static_cast<unsigned long long>(ml_planes_[i].id));
          // place content
          content_node_->SetWorldTranslation(ml::app_framework::to_glm(ml_planes_[i].position));
          break;
        }
      }
    }
  }

  void RequestPlanes(const MLTransform &bounds_transform) {
    MLPlanesQuery query = {};
    query.flags = MLPlanesQueryFlag_Horizontal | MLPlanesQueryFlag_Inner | MLPlanesQueryFlag_Semantic_Floor;
    if (FLAGS_RenderBoundaries) {
      query.flags |= MLPlanesQueryFlag_Polygons;
    }

    query.bounds_center = bounds_transform.position;
    query.bounds_rotation = bounds_transform.rotation;
    query.bounds_extents = MLVec3f{10.f, 10.f, 10.f};
    query.max_results = MAX_PLANES;
    query.min_hole_length = FLAGS_min_hole_length;
    query.min_plane_area = FLAGS_min_plane_area;
    UNWRAP_MLRESULT(MLPlanesQueryBegin(planes_tracker_, &query, &planes_request_));
  }

  void ProcessPlanes(std::shared_ptr<ml::app_framework::Node> &gfx_plane, MLPlane* ml_plane) {
        gfx_plane->SetWorldTranslation(ml::app_framework::to_glm(ml_plane->position));
        gfx_plane->SetWorldRotation(ml::app_framework::to_glm(ml_plane->rotation));
        gfx_plane->SetLocalScale(glm::vec3(ml_plane->width, ml_plane->height, 1.0f));
        gfx_plane->GetComponent<ml::app_framework::RenderableComponent>()->SetVisible(true);
        glm::vec4 plane_color = {
            (ml_plane->flags & MLPlanesQueryFlag_Semantic_Ceiling) ? 1.f : 0.f,
            (ml_plane->flags & MLPlanesQueryFlag_Semantic_Floor) ? 1.f : 0.f,
            (ml_plane->flags & MLPlanesQueryFlag_Semantic_Wall) ? 1.f : 0.f, 1.0f,
        };

        auto plane_material = std::static_pointer_cast<ml::app_framework::FlatMaterial>(gfx_plane->GetComponent<ml::app_framework::RenderableComponent>()->GetMaterial());
        plane_material->SetColor(plane_color);
  }

  void ReceivePlanesRequest() {
    MLResult result = MLPlanesQueryGetResultsWithBoundaries(planes_tracker_, planes_request_, ml_planes_.data(), &num_planes_, nullptr);
    if (MLResult_Ok == result) {
      ClearPlanes(planes_, num_planes_);
      for (size_t i = 0; i < num_planes_; ++i) {
        ProcessPlanes(planes_[i], &ml_planes_[i]);
      }

      planes_request_ = ML_INVALID_HANDLE;
    } else if (MLResult_Pending == result) {
      ML_LOG(Verbose, "PENDING!");
    } else {
      ML_LOG(Fatal, "ERROR %s!", MLGetResultString(result));
    }
  }

  void ReceivePlanesBoundariesRequest() {
    holes_count_ = 0;
    line_count_ = 0;
    MLPlaneBoundariesList boundaries_list;

    MLPlaneBoundariesListInit(&boundaries_list);
    MLResult plane_query_result = MLPlanesQueryGetResultsWithBoundaries(planes_tracker_,
      planes_request_, &ml_planes_.front(), &num_planes_, &boundaries_list);

    if (plane_query_result == MLResult_Ok ) {
      ClearPlanes(planes_, num_planes_);
      //Process the planes we recieved
      for (size_t i = 0; i < num_planes_; ++i) {
        ProcessPlanes(planes_[i], &ml_planes_[i]);
      }
      // render polygon boundaries
      for (uint32_t i = 0; i < boundaries_list.plane_boundaries_count; ++i) {
        MLPlaneBoundaries& plane_boundaries  = boundaries_list.plane_boundaries[i];
        // for each boundary region
        for (uint32_t j = 0; j < plane_boundaries.boundaries_count; ++j) {
          // boundary polygon
          MLPlaneBoundary& plane_boundary = plane_boundaries.boundaries[j];

          if (line_count_ < MAX_LINES) {
            auto boundary_renderable = lines_[line_count_]->GetComponent<ml::app_framework::RenderableComponent>();
            boundary_renderable->SetVisible(true);
            boundary_renderable->GetMesh()->SetPrimitiveType(GL_LINE_STRIP);
            boundary_renderable->GetMesh()->UpdateMesh(
                reinterpret_cast<glm::vec3 *>(plane_boundary.polygon->vertices), nullptr, plane_boundary.polygon->vertices_count, nullptr, 0);
            auto boundary_material = std::static_pointer_cast<ml::app_framework::FlatMaterial>(boundary_renderable->GetMaterial());
            boundary_material->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
            line_count_++;
          } else {
            ML_LOG(Info, "Reached max line count, %d", line_count_);
          }

          // polygon holes
          for (uint32_t k = 0; k < plane_boundary.holes_count; ++k) {
            if (holes_count_ < MAX_HOLES) {
              auto hole_renderable = holes_[holes_count_]->GetComponent<ml::app_framework::RenderableComponent>();
              hole_renderable->SetVisible(true);
              hole_renderable->GetMesh()->SetPrimitiveType(GL_LINE_LOOP);
              hole_renderable->GetMesh()->UpdateMesh(
                  reinterpret_cast<glm::vec3 *>(plane_boundary.holes[k].vertices), nullptr, plane_boundary.holes[k].vertices_count, nullptr, 0);
              auto hole_material = std::static_pointer_cast<ml::app_framework::FlatMaterial>(holes_[k]->GetComponent<ml::app_framework::RenderableComponent>()->GetMaterial());
              hole_material->SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
              holes_count_++;
            } else {
              ML_LOG(Info, "Reached max hole count, %d", line_count_);
            }
          }
        }
      }

      // Release memory for boundaries results
      if (FLAGS_RenderBoundaries && boundaries_list.plane_boundaries_count) {
        MLResult plane_release_result = MLPlanesReleaseBoundariesList(planes_tracker_, &boundaries_list);
        if (plane_release_result != MLResult_Ok) {
          ML_LOG(Error, "Releasing of boundaries list memory failed.");
        }
      }
      planes_request_ = ML_INVALID_HANDLE;

      for (uint32_t i = line_count_; i < MAX_LINES; ++i) {
        lines_[i]->GetComponent<ml::app_framework::RenderableComponent>()->SetVisible(false);
      }
      for (uint32_t i = holes_count_; i < MAX_HOLES; ++i) {
        holes_[i]->GetComponent<ml::app_framework::RenderableComponent>()->SetVisible(false);
      }
    } else if (MLResult_Pending == plane_query_result) {
      ML_LOG(Verbose, "PENDING!");
    } else {
      ML_LOG(Fatal, "ERROR %s!", MLGetResultString(plane_query_result));
    }
  }

  void ClearPlanes(std::array<std::shared_ptr<ml::app_framework::Node>, MAX_PLANES>& planes, uint32_t size) {
    if (size > MAX_PLANES) {
      ML_LOG(Fatal, "Size outside bounds of array. size > MAX_PLANES, exiting.");
    }

    for (uint32_t i = size; i < planes.size(); ++i) {
      planes[i]->GetComponent<ml::app_framework::RenderableComponent>()->SetVisible(false);
    }
  }

  void UpdateLine(std::shared_ptr<ml::app_framework::Node> &node, const glm::vec3 *verts, int verts_size) {
    auto line_renderable = node->GetComponent<ml::app_framework::RenderableComponent>();
    line_renderable->GetMesh()->UpdateMesh(verts, nullptr, verts_size, nullptr, 0);
  }

  void RequestRay(const MLVec3f &position, const MLVec3f &direction) {
    MLRaycastQuery query = raycast_query_;
    query.position = position;
    query.direction = direction;
    query.up_vector = MLVec3f{0.f, 1.f, 0.f};
    query.width = 1;
    query.height = 1;
    query.horizontal_fov_degrees = 45.0f;
    query.collide_with_unobserved = true;
    UNWRAP_MLRESULT(MLRaycastRequest(raycast_tracker_, &query, &raycast_request_));
  }

  void ReceiveRayRequest(const MLTransform &controller_transform) {
    MLResult result = MLRaycastGetResult(raycast_tracker_, raycast_request_, &raycast_result_);
    if (MLResult_Ok == result) {
      if (MLRaycastResultState_HitObserved == raycast_result_.state ||
          MLRaycastResultState_HitUnobserved == raycast_result_.state) {
        glm::vec3 hitpoint = ml::app_framework::to_glm(raycast_result_.hitpoint);
        glm::vec3 normal = ml::app_framework::to_glm(raycast_result_.normal);

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


  std::array<std::shared_ptr<ml::app_framework::Node>, MAX_PLANES> planes_;
  std::array<std::shared_ptr<ml::app_framework::Node>, MAX_LINES> lines_;
  std::array<std::shared_ptr<ml::app_framework::Node>, MAX_HOLES> holes_;
  uint32_t line_count_ = 0;
  uint32_t holes_count_ = 0;
  MLHandle head_tracker_ = ML_INVALID_HANDLE;
  MLHeadTrackingStaticData head_static_data_ = {};
  MLHandle planes_tracker_ = ML_INVALID_HANDLE;
  MLHandle planes_request_ = ML_INVALID_HANDLE;
  MLHandle input_tracker_ = ML_INVALID_HANDLE;
  std::array<MLPlane, MAX_PLANES> ml_planes_;
  uint32_t num_planes_ = 0;
  std::shared_ptr<ml::app_framework::Node> content_node_;
  bool can_place_content_ = true;
  bool content_placed_ = false;
  MLInputControllerState input_state_[MLInput_MaxControllers];
  MLHandle raycast_tracker_ = ML_INVALID_HANDLE;
  MLHandle raycast_request_ = ML_INVALID_HANDLE;
  MLRaycastQuery raycast_query_ = {};
  MLRaycastResult raycast_result_ = {};
  std::shared_ptr<ml::app_framework::Node> pointer_;
};

int main(int argc, char **argv) {
  PlanesApp app(argc, argv);
  app.SetUseGfx(true);
  app.RunApp();
  return 0;
}
