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

static constexpr uint64_t FNVBasis = 0xcbf29ce484222325;
static constexpr uint64_t FNVPrime = 0x00000100000001b3;

// Recursive computation due the restriction from costexpr
static inline constexpr uint64_t _HashStrWithFNVInternal(uint64_t hash, const char* str,
                                                         int32_t iteration, uint64_t size) {
  return iteration < size
             ? _HashStrWithFNVInternal(hash ^ str[iteration] * FNVPrime, str, iteration + 1, size)
             : hash;
}

static inline constexpr uint64_t _HashStrWithFNV(const char* str, uint64_t size) {
  // FNV-1 algorithm
  return _HashStrWithFNVInternal(FNVBasis, str, 0, size);
}

typedef uint64_t RUNTIME_TYPE_ID;

class RuntimeType {
public:
  RuntimeType() = default;
  virtual ~RuntimeType() = default;

  virtual RUNTIME_TYPE_ID GetRuntimeType() const = 0;
};

#define RUNTIME_TYPE_REGISTER(type)                       \
public:                                                  \
  static const constexpr RUNTIME_TYPE_ID RuntimeTypeId = \
      _HashStrWithFNV(#type, sizeof(#type) - 1);         \
  static RUNTIME_TYPE_ID GetClassRuntimeType() {         \
    return RuntimeTypeId;                                \
  }                                                      \
  RUNTIME_TYPE_ID GetRuntimeType() const override {      \
    return GetClassRuntimeType();                        \
  }
