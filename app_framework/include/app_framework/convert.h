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
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ml_types.h>

#include <string>

namespace ml {
namespace app_framework {

inline glm::vec2 to_glm(const MLVec2f &vec) {
  return {vec.x, vec.y};
}

inline MLVec2f to_ml(const glm::vec2 &vec) {
  return {vec.x, vec.y};
}

inline glm::vec3 to_glm(const MLVec3f &vec) {
  return {vec.x, vec.y, vec.z};
}

inline MLVec3f to_ml(const glm::vec3 &vec) {
  return {vec.x, vec.y, vec.z};
}

inline glm::quat to_glm(const MLQuaternionf &quat) {
  return {quat.w, quat.x, quat.y, quat.z};
}

inline MLQuaternionf to_ml(const glm::quat &quat) {
  return {quat.x, quat.y, quat.z, quat.w};
}

inline glm::mat4 to_glm(const MLMat4f &mat) {
  return {glm::make_mat4(mat.matrix_colmajor)};
}

std::string to_string(const MLCoordinateFrameUID &cfuid);

}  // namespace app_framework
}  // namespace ml
