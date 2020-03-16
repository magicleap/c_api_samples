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

#include <algorithm>
#include <app_framework/ml_macros.h>
#include <app_framework/node.h>
#include <glm/ext.hpp>
#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/gtx/transform.hpp>

namespace ml {
namespace app_framework {

Node::Node() : Node(glm::vec3(0)) {}

Node::Node(const glm::vec3 &translation_vector)
    : local_dirty_(true),
      world_dirty_(true),
      local_transform_(glm::mat4(1)),
      local_translation_(translation_vector),
      local_rotation_(glm::mat4(1)),
      local_scale_(glm::vec3(1, 1, 1)),
      world_transform_(glm::mat4(1)),
      parent_(),
      name_("") {}

bool Node::AddChild(std::shared_ptr<ml::app_framework::Node> new_child) {
  auto shared_this = shared_from_this();

  // make sure adding child does not create a cycle in the hierarchy
  std::vector<std::shared_ptr<ml::app_framework::Node>> dfs_stack;
  dfs_stack.push_back(new_child);
  while (!dfs_stack.empty()) {
    auto back = dfs_stack.back();
    if (back == shared_this) {
      ML_LOG(Error, "Adding node \"%s\" as a child of node \"%s\" would create a cycle", new_child->name_.c_str(),
             name_.c_str());
      return false;
    }
    dfs_stack.pop_back();
    // Note: since back is a shared_ptr, I can still dereference it even after the pop_back.
    for (auto &child : back->child_list_) {
      dfs_stack.push_back(child);
    }
  }

  if (auto locked_parent = new_child->parent_.lock()) {
    locked_parent->RemoveChild(new_child);
  }

  new_child->parent_ = shared_this;
  new_child->SetDirty();
  child_list_.push_back(new_child);
  return true;
}

bool Node::RemoveChild(std::shared_ptr<ml::app_framework::Node> child) {
  if (child) {
    child_list_.erase(std::remove_if(child_list_.begin(), child_list_.end(),
                                     [child](std::shared_ptr<ml::app_framework::Node> &node) { return node == child; }),
                      child_list_.end());
    child->parent_.reset();
  } else {
    ML_LOG(Error, "Can not remove null pointer");
    return false;
  }
  return true;
}

const std::vector<std::shared_ptr<ml::app_framework::Node>> &Node::GetChildren() const {
  return child_list_;
}

const glm::mat4 Node::GetParentWorldTransform() const {
  glm::mat4 parentWorldTransform;
  if (auto locked_parent = parent_.lock()) {
    parentWorldTransform = locked_parent->GetWorldTransform();
  } else {
    parentWorldTransform = glm::mat4(1);
  }
  return parentWorldTransform;
}

void Node::SetName(std::string name) {
  name_ = name;
}

const bool Node::IsDirty() const {
  return local_dirty_ || world_dirty_;
}

void Node::SetDirty() {
  local_dirty_ = true;
  world_dirty_ = true;
  for (auto &child : child_list_) {
    child->SetDirty();
  }
}

void Node::SetLocalTranslation(const glm::vec3 &translation) {
  SetDirty();
  local_translation_ = translation;
}

void Node::SetWorldTranslation(const glm::vec3 &translation) {
  SetDirty();
  local_translation_ = glm::vec3(glm::inverse(GetParentWorldTransform()) * glm::vec4(translation, 1));
}

void Node::SetLocalRotation(const glm::quat &quaternion) {
  SetDirty();
  local_rotation_ = quaternion;
}

void Node::SetWorldRotation(const glm::quat &quaternion) {
  SetDirty();
  local_rotation_ = glm::quat_cast(glm::inverse(GetParentWorldTransform())) * quaternion;
}

void Node::SetLocalScale(const glm::vec3 &scale_vector) {
  SetDirty();
  local_scale_ = scale_vector;
}

void Node::AddComponent(std::shared_ptr<app_framework::Component> component) {
  auto it = components_by_type_.find(component->GetRuntimeType());
  if (components_by_type_.end() != it) {
    components_.erase(std::find(components_.begin(), components_.end(), it->second));
  }

  components_.push_back(component);
  components_by_type_[component->GetRuntimeType()] = component;
  component->SetNode(shared_from_this());
}

const glm::mat4 &Node::GetLocalTransform() const {
  if (local_dirty_) {
    local_transform_ = glm::translate(local_translation_) * glm::mat4_cast(local_rotation_) * glm::scale(local_scale_);
    local_dirty_ = false;
  }
  return local_transform_;
}

const glm::mat4 &Node::GetWorldTransform() const {
  if (world_dirty_) {
    world_transform_ = GetParentWorldTransform() * GetLocalTransform();
    world_dirty_ = false;
  }
  return world_transform_;
}

const glm::vec3 &Node::GetLocalTranslation() const {
  return local_translation_;
}

const glm::vec3 Node::GetWorldTranslation() const {
  return glm::vec3(GetWorldTransform()[3]);
}

const glm::quat &Node::GetLocalRotation() const {
  return local_rotation_;
}

const glm::quat Node::GetWorldRotation() const {
  return glm::toQuat(GetWorldTransform());
}

const glm::vec3 &Node::GetLocalScale() const {
  return local_scale_;
}

}  // namespace app_framework
}  // namespace ml
