//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once
#include <functional>
#include <string>
#include <unordered_map>

#include <app_framework/common.h>

#include "buffer.h"
#include "texture.h"

namespace ml {
namespace app_framework {

template <typename ValueType>
struct VariableAccessor {
  ValueType operator()(const void *) {
    ML_LOG(Fatal, "No impl");
    return ValueType();
  }
  void operator()(void *, const ValueType &value) {
    ML_LOG(Fatal, "No impl");
  }
};

class Variable : public RuntimeType {
  RUNTIME_TYPE_REGISTER(Variable)
public:
  Variable(const std::string &name) : name_(name) {}
  virtual ~Variable() = default;
  const std::string &GetName() const {
    return name_;
  }
  void SetName(const std::string &name) {
    name_ = name;
  }

  virtual void CopyValue(std::shared_ptr<Variable> rhs) {
    ML_LOG(Fatal, "No impl");
  }

  template <typename ValueType>
  void SetValue(const ValueType &value) {
    VariableAccessor<ValueType> accessor;
    accessor.operator()((void *)this, value);
  }

  template <typename ValueType>
  ValueType GetValue() const {
    VariableAccessor<ValueType> accessor;
    return accessor.operator()((const void *)this);
  }

  virtual void *GetMemoryPtr() const {
    ML_LOG(Fatal, "No impl");
    return nullptr;
  }
  virtual uint64_t GetSize() const {
    ML_LOG(Fatal, "No impl");
    return 0;
  }

private:
  std::string name_;
};

#ifndef VARIABLE_CREATOR_EXTERN_INSTANCE
#define VARIABLE_CREATOR_EXTERN_INSTANCE extern
#endif

VARIABLE_CREATOR_EXTERN_INSTANCE
std::unordered_map<GLenum, std::function<std::shared_ptr<Variable>(const std::string &)>> sCreators;

inline std::shared_ptr<Variable> MakeVariableFromGLType(const std::string &name, GLenum gl_type) {
  return sCreators[gl_type](name);
}

#define VARIABLE_TYPE_DECLARE_CREATOR(handler_type, gl_type)                                             \
  struct _##handler_type##ValueTypeCreatorDummy {                                                        \
    _##handler_type##ValueTypeCreatorDummy() {                                                           \
      sCreators[gl_type] = [](const std::string &name) { return std::make_shared<handler_type>(name); }; \
    }                                                                                                    \
  };                                                                                                     \
                                                                                                         \
  VARIABLE_CREATOR_EXTERN_INSTANCE _##handler_type##ValueTypeCreatorDummy                                \
      _##handler_type##ValueTypeCreatorDummyInstance;

#define VARIABLE_TYPE_DECLARE_ACCESSOR(type, handler_type)                              \
  template <>                                                                           \
  struct VariableAccessor<type> {                                                       \
    void operator()(void *ptr, const type &value) {                                     \
      if (((Variable *)ptr)->GetRuntimeType() != handler_type::GetClassRuntimeType()) { \
        ML_LOG(Fatal, "Illegal casting");                                               \
      }                                                                                 \
      handler_type *child = (handler_type *)ptr;                                        \
      child->SetValue(value);                                                           \
    }                                                                                   \
    type operator()(const void *ptr) {                                                  \
      if (((Variable *)ptr)->GetRuntimeType() != handler_type::GetClassRuntimeType()) { \
        ML_LOG(Fatal, "Illegal casting");                                               \
      }                                                                                 \
      handler_type *child = (handler_type *)ptr;                                        \
      return child->GetValue();                                                         \
    }                                                                                   \
  };

#define VARIABLE_TYPE_DECLARE_VALUE(type, handler_type, gl_type) \
  class handler_type : public Variable {                         \
    RUNTIME_TYPE_REGISTER(handler_type)                           \
  public:                                                        \
    handler_type(const std::string &name) : Variable(name) {     \
      memset(&value_, 0, sizeof(type));                          \
    }                                                            \
    ~handler_type() = default;                                   \
    type GetValue() const {                                      \
      return value_;                                             \
    }                                                            \
    void CopyValue(std::shared_ptr<Variable> rhs) override {     \
      memcpy(GetMemoryPtr(),                                     \
        rhs->GetMemoryPtr(), rhs->GetSize());                    \
    }                                                            \
    void SetValue(const type &value) {                           \
      value_ = value;                                            \
    }                                                            \
    void *GetMemoryPtr() const override {                        \
      return (void *)&value_;                                    \
    }                                                            \
    uint64_t GetSize() const override {                          \
      return sizeof(type);                                       \
    }                                                            \
                                                                 \
  private:                                                       \
    type value_;                                                 \
  };                                                             \
  VARIABLE_TYPE_DECLARE_ACCESSOR(type, handler_type)             \
  VARIABLE_TYPE_DECLARE_CREATOR(handler_type, gl_type)

#define VARIABLE_TYPE_DECLARE_REFERENCE(type, handler_type, gl_type)  \
  class handler_type : public Variable {                              \
    RUNTIME_TYPE_REGISTER(handler_type)                                \
  public:                                                             \
    handler_type(const std::string &name) : Variable(name) {}         \
    ~handler_type() = default;                                        \
    std::shared_ptr<type> GetValue() const {                          \
      return value_;                                                  \
    }                                                                 \
    void SetValue(const std::shared_ptr<type> &value) {               \
      value_ = value;                                                 \
    }                                                                 \
    void CopyValue(std::shared_ptr<Variable> rhs) override {          \
      auto rhs_ptr = std::static_pointer_cast<handler_type>(rhs);     \
      value_ = rhs_ptr->value_;                                       \
    }                                                                 \
  private:                                                            \
    std::shared_ptr<type> value_;                                     \
  };                                                                  \
  VARIABLE_TYPE_DECLARE_ACCESSOR(std::shared_ptr<type>, handler_type) \
  VARIABLE_TYPE_DECLARE_CREATOR(handler_type, gl_type)

VARIABLE_TYPE_DECLARE_VALUE(bool, BoolVariable, GL_BOOL);
VARIABLE_TYPE_DECLARE_VALUE(float, FloatVariable, GL_FLOAT);
VARIABLE_TYPE_DECLARE_VALUE(int32_t, IntegerVariable, GL_INT);
VARIABLE_TYPE_DECLARE_VALUE(glm::vec4, Vec4Variable, GL_FLOAT_VEC4);
VARIABLE_TYPE_DECLARE_VALUE(glm::mat4, Mat4Variable, GL_FLOAT_MAT4);
VARIABLE_TYPE_DECLARE_VALUE(glm::vec3, Vec3Variable, GL_FLOAT_VEC3);
VARIABLE_TYPE_DECLARE_VALUE(glm::vec2, Vec2Variable, GL_FLOAT_VEC2);
VARIABLE_TYPE_DECLARE_REFERENCE(Texture, TextureVariable, GL_SAMPLER_2D);

}
}
