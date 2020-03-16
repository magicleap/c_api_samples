//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#include "renderer.h"
#include <app_framework/gui.h>

#include <algorithm>

namespace ml {
namespace app_framework {

Renderer::Renderer() {
  glGenBuffers(1, &camera_uniform_buffer_);
  glBindBuffer(GL_UNIFORM_BUFFER, camera_uniform_buffer_);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraUBO), nullptr, GL_DYNAMIC_DRAW);

  glGenBuffers(1, &light_uniform_buffer_);
  glBindBuffer(GL_UNIFORM_BUFFER, light_uniform_buffer_);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(LightsUBO), nullptr, GL_DYNAMIC_DRAW);

  glGenBuffers(1, &model_uniform_buffer_);
  glBindBuffer(GL_UNIFORM_BUFFER, model_uniform_buffer_);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
}

Renderer::~Renderer() {
  glDeleteBuffers(1, &camera_uniform_buffer_);
  glDeleteBuffers(1, &light_uniform_buffer_);
  glDeleteBuffers(1, &model_uniform_buffer_);
}

void Renderer::QueueCamera(std::shared_ptr<CameraComponent> camera) {
  queued_cameras_.push_back(camera);
}

void Renderer::QueueRendererable(std::shared_ptr<RenderableComponent> renderable) {
  if (renderable->GetVisible()) {
    auto material = renderable->GetMaterial();
    if (material->AlphaBlendingEnabled()) {
      queued_transparent_renderables_.push_back(renderable);
    } else {
      queued_opaque_renderables_[material].push_back(renderable);
    }
  }
}

void Renderer::QueueLight(std::shared_ptr<LightComponent> light) {
  queued_lights_.push_back(light);
}

void Renderer::Render() {
  // Simple single pass, per-object basis forward rendering
  if (pre_render_callback_) {
    pre_render_callback_();
  }

  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_FRAMEBUFFER_SRGB);

  for (const std::shared_ptr<CameraComponent> &cam : queued_cameras_) {
    current_cam_ = cam;
    if (pre_cam_callback_) {
      pre_cam_callback_(current_cam_);
    }

    // Bind the render target
    auto render_target = cam->GetRenderTarget();
    if (!render_target) {
      continue;
    }
    GLuint framebuffer = render_target->GetGLFramebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    auto viewport = current_cam_->GetViewport();
    glViewport((int)viewport.x, (int)viewport.y, (int)viewport.z, (int)viewport.w);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw all opaque objects first.
    glDisable(GL_BLEND);

    for (auto &material_and_renderables : queued_opaque_renderables_) {
      auto &material = *material_and_renderables.first;
      auto &renderables = material_and_renderables.second;

      UseMaterial(material);
      glBindBuffer(GL_UNIFORM_BUFFER, model_uniform_buffer_);
      for (const auto &renderable : renderables) {
        RenderRenderable(*renderable);
      }
    }

    // Sort transparent renderables back-to-front and then draw them
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    const auto camera_position = cam->GetNode()->GetWorldTranslation();
    std::sort(queued_transparent_renderables_.begin(), queued_transparent_renderables_.end(),
        [&camera_position](const std::shared_ptr<RenderableComponent> &renderable1,
            const std::shared_ptr<RenderableComponent> &renderable2) {
          const auto dist1 = glm::distance(camera_position, renderable1->GetNode()->GetWorldTranslation());
          const auto dist2 = glm::distance(camera_position, renderable2->GetNode()->GetWorldTranslation());
          return dist1 > dist2;
        });
    for (const auto &renderable : queued_transparent_renderables_) {
      UseMaterial(*renderable->GetMaterial());
      glBindBuffer(GL_UNIFORM_BUFFER, model_uniform_buffer_);
      RenderRenderable(*renderable);
    }

    // Finally unbind any vertex array after drawing, to make sure nothing else messes with it
    glBindVertexArray(0);

    // Reset the glPolygonMode to avoid interfering with imgui's rendering
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    auto blit_target = cam->GetBlitTarget();
    if (blit_target) {
      glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, blit_target->GetGLFramebuffer());
      glBlitFramebuffer((int)viewport.x, (int)viewport.y, (int)viewport.z, (int)viewport.w, 0, 0,
          blit_target->GetWidth(), blit_target->GetHeight(), GL_COLOR_BUFFER_BIT, GL_LINEAR);

      if (Gui::GetInstance().GetShowImgui()) {
        const auto &imgui_frame_buffer = Gui::GetInstance().GetFrameBuffer();
        const auto &imgui_texture = Gui::GetInstance().GetTexture();

        glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)imgui_frame_buffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, blit_target->GetGLFramebuffer());
        glBlitFramebuffer(0, 0, imgui_texture->GetWidth(), imgui_texture->GetHeight(), blit_target->GetWidth() / 2, 0,
            blit_target->GetWidth(), blit_target->GetHeight() / 2, GL_COLOR_BUFFER_BIT, GL_LINEAR);
      }
    }

    if (post_cam_callback_) {
      post_cam_callback_(current_cam_);
    }
  }

  if (post_render_callback_) {
    post_render_callback_();
  }
  ClearQueues();
}

void Renderer::ClearQueues() {
  queued_opaque_renderables_.clear();
  queued_transparent_renderables_.clear();
  queued_cameras_.clear();
  queued_lights_.clear();
}

void Renderer::BindProgram(GLuint vert, GLuint geom, GLuint frag) {
  std::tuple<GLuint, GLuint, GLuint> key = std::make_tuple(vert, geom, frag);
  auto it = shader_program_cache_.find(key);
  if (it == shader_program_cache_.end()) {
    glGenProgramPipelines(1, &program_pipeline_);
    glUseProgramStages(program_pipeline_, GL_VERTEX_SHADER_BIT, vert);
    if (geom != 0) {
      glUseProgramStages(program_pipeline_, GL_GEOMETRY_SHADER_BIT, geom);
    }
    glUseProgramStages(program_pipeline_, GL_FRAGMENT_SHADER_BIT, frag);
    shader_program_cache_[key] = program_pipeline_;
  } else {
    program_pipeline_ = it->second;
  }

  glBindProgramPipeline(program_pipeline_);
}

void Renderer::UseMaterial(Material &material) {
  auto view = glm::inverse(current_cam_->GetNode()->GetWorldTransform());
  auto view_proj = current_cam_->GetProjectionMatrix() * view;

  // Get the camera data, mvp, update the uniform
  camera_uniform_buffer_dirty_ = true;
  CameraUBO transforms_ubo(view_proj, current_cam_->GetNode()->GetWorldTranslation());

  if (current_vertex_program_) {
    BindCameraUniform(*current_vertex_program_, transforms_ubo);
    BindModelUniform(*current_vertex_program_);
  }
  if (current_frag_program_) {
    BindCameraUniform(*current_frag_program_, transforms_ubo);
    BindModelUniform(*current_frag_program_);
  }
  if (current_geom_program_) {
    BindCameraUniform(*current_geom_program_, transforms_ubo);
    BindModelUniform(*current_geom_program_);
  }

  glPolygonMode(GL_FRONT_AND_BACK, material.GetPolygonMode());

  current_vertex_program_ = material.GetVertexProgram();
  current_frag_program_ = material.GetFragmentProgram();
  current_geom_program_ = material.GetGeometryProgram();

  bool bind_gs = static_cast<bool>(current_geom_program_);

  BindProgram(current_vertex_program_->GetGLProgram(), bind_gs ? current_geom_program_->GetGLProgram() : 0,
      current_frag_program_->GetGLProgram());

  const auto &fragment_ubo_blk_list = current_frag_program_->GetUniformBlocks();
  lights_.clear();
  for (auto &light : queued_lights_) {
    Light l(light->GetNode()->GetWorldTranslation(), light->GetLightColor(), light->GetDirection(),
        light->GetLightType(), light->GetLightStrength());
    lights_.push_back(l);
  }
  LightsUBO lights_ubo(lights_);
  auto fragment_ubo_light_it = fragment_ubo_blk_list.find(UniformName::kLight);
  if (fragment_ubo_light_it != fragment_ubo_blk_list.end()) {
    const auto &des = fragment_ubo_light_it->second;
    glBindBufferBase(GL_UNIFORM_BUFFER, des.binding, light_uniform_buffer_);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lights_ubo), &lights_ubo);
  }

  material.UpdateMaterialUniforms();
  auto fragment_ubo_it = fragment_ubo_blk_list.find(UniformName::kMaterial);
  if (fragment_ubo_it != fragment_ubo_blk_list.end()) {
    const auto &des = fragment_ubo_it->second;
    material.UpdateMaterialUniformBuffer();
    glBindBufferBase(GL_UNIFORM_BUFFER, des.binding, material.GetGLUniformBuffer());
  }
}

void Renderer::RenderRenderable(const RenderableComponent &renderable) {
  auto model_transform = renderable.GetNode()->GetWorldTransform();
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(model_transform), glm::value_ptr(model_transform));

  auto mesh = renderable.GetMesh();

  if (mesh->GetPrimitiveType() == GL_POINTS) {
    glPointSize(mesh->GetPointSize());
  }

  glBindVertexArray(mesh->GetVertexArrayObject());

  if (mesh->UsesIndexedRendering()) {
    glDrawArrays(mesh->GetPrimitiveType(), 0, mesh->GetNumVertices());
  } else {
    glDrawElements(mesh->GetPrimitiveType(), mesh->GetNumIndices(), mesh->GetIndexType(), nullptr);
  }
}

void Renderer::BindCameraUniform(Program &program, const CameraUBO &camera_ubo) {
  const auto &vertex_ubo_list = program.GetUniformBlocks();
  // Bind the transform UBO
  auto vertex_ubo_it = vertex_ubo_list.find(UniformName::kCamera);
  if (vertex_ubo_it != vertex_ubo_list.end()) {
    const auto &blk_des = vertex_ubo_it->second;
    glBindBufferBase(GL_UNIFORM_BUFFER, blk_des.binding, camera_uniform_buffer_);
    if (camera_uniform_buffer_dirty_) {
      glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(camera_ubo), &camera_ubo);
      camera_uniform_buffer_dirty_ = false;
    }
  }
}

// This only binds the model uniform buffer, it doesn't set it since we want to set it once per node.
// TODO: assign constant binding points to uniform blocks, so that it won't be necessary to look them up.
void Renderer::BindModelUniform(Program &program) {
  const auto &vertex_ubo_list = program.GetUniformBlocks();
  // Bind the transform UBO
  auto vertex_ubo_it = vertex_ubo_list.find(UniformName::kModel);
  if (vertex_ubo_it != vertex_ubo_list.end()) {
    const auto &blk_des = vertex_ubo_it->second;
    glBindBufferBase(GL_UNIFORM_BUFFER, blk_des.binding, model_uniform_buffer_);
  }
}

}  // namespace app_framework
}  // namespace ml
