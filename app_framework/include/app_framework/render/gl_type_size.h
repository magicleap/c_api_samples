//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once
#include <cinttypes>

#include <glad/glad.h>

#include <app_framework/ml_macros.h>

namespace ml {
namespace app_framework {

static inline uint64_t GetGLTypeSize(GLint type) {
  uint64_t size = 0;
  switch (type) {
    case GL_BYTE:
    case GL_BOOL:
    case GL_UNSIGNED_BYTE: size = 1; break;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT: size = 2; break;
    case GL_INT:
    case GL_FLOAT:
    case GL_UNSIGNED_INT: size = 4; break;
    case GL_FLOAT_VEC2: size = 8; break;
    case GL_FLOAT_VEC3: size = 12; break;
    case GL_FLOAT_VEC4: size = 16; break;
    case GL_FLOAT_MAT4: size = 64; break;
    case GL_SAMPLER_2D: size = 0; break;
#ifdef ML_LUMIN
    case GL_SAMPLER_EXTERNAL_OES: size = 0; break;
#endif
    default: ML_LOG(Error, "unhandled type (%x)", type);
  }
  return size;
}
}
}
