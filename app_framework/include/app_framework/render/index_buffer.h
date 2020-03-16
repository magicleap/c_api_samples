//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once
#include <app_framework/common.h>
#include "buffer.h"

namespace ml {
namespace app_framework {

class IndexBuffer final : public Buffer {
public:
  IndexBuffer(Buffer::Category category, GLint type)
      : Buffer(category, GL_ELEMENT_ARRAY_BUFFER), type_(type), index_cnt_(0) {
    switch (type) {
      case GL_UNSIGNED_BYTE: index_size_ = 1; break;
      case GL_UNSIGNED_SHORT: index_size_ = 2; break;
      case GL_UNSIGNED_INT: index_size_ = 4; break;
      default: ML_LOG(Fatal, "unhandled type (%x)", type);
    }
  }
  ~IndexBuffer() = default;

  void UpdateBuffer(const char *data, uint64_t size) override {
    Buffer::UpdateBuffer(data, size);
    index_cnt_ = size / index_size_;
  }

  GLint GetIndexType() const {
    return type_;
  }

  uint64_t GetIndexSize() const {
    return index_size_;
  }

  uint64_t GetIndexCount() const {
    return index_cnt_;
  }

private:
  GLint type_;
  uint64_t index_size_;
  uint64_t index_cnt_;
};
}
}
