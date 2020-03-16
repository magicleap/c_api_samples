//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once
#include <istream>
#include <fstream>
#include <algorithm>
#include <unordered_map>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/texture.h>

#include "common.h"

#include "registry.h"
#include "render/buffer.h"
#include "render/mesh.h"
#include "render/texture.h"

namespace ml {
namespace app_framework {

template <typename MeshType>
std::shared_ptr<MeshType> ResourcePool::GetMesh() const {
  std::shared_ptr<MeshType> element = std::static_pointer_cast<MeshType>(
    GetCacheElement<Mesh>(mesh_cache_, std::to_string(MeshType::GetClassRuntimeType()))
  );
  return element;
}

template <typename ProgramType>
std::shared_ptr<ProgramType> ResourcePool::LoadShaderFromFile(const std::string &path) {
  std::ifstream file_stream(path);
  std::string code((std::istreambuf_iterator<char>(file_stream)),
                    std::istreambuf_iterator<char>());
  return LoadShaderFromCode<ProgramType>(code, path);
}

template <typename ProgramType>
std::shared_ptr<ProgramType> ResourcePool::LoadShaderFromCode(const std::string &code, const std::string& identifier) {
  return LoadShaderFromCode<ProgramType>(code.c_str(), identifier);
}

template <typename ProgramType>
std::shared_ptr<ProgramType> ResourcePool::LoadShaderFromCode(const char* code, const std::string& identifier) {
  std::string key;
  if (identifier.empty()) {
    key = code;
  } else {
    key = identifier;
  }
  return GetOrCreateElement<ProgramType, Program>(program_cache_, key, code);
}

template <typename Type, typename ParentType, typename... Args>
std::shared_ptr<Type> ResourcePool::GetOrCreateElement(std::unordered_map<std::string, std::shared_ptr<ParentType>> &cache,
                                                       const std::string &key, Args &&... args) {
  std::shared_ptr<Type> element = std::static_pointer_cast<Type>(GetCacheElement<ParentType>(cache, key));
  if (element) {
    return element;
  }

  return CreateCacheElement<Type, ParentType, Args &&...>(cache, key, std::forward<Args>(args)...);
}

template <typename Type, typename ParentType, typename... Args>
std::shared_ptr<Type> ResourcePool::CreateCacheElement(std::unordered_map<std::string, std::shared_ptr<ParentType>> &cache,
                                                       const std::string &key, Args &&... args) {
  std::shared_ptr<Type> element = std::make_shared<Type>(std::forward<Args>(args)...);
  cache.insert(std::make_pair(key, element));
  return element;
}

template <typename ParentType>
std::shared_ptr<ParentType> ResourcePool::GetCacheElement(const std::unordered_map<std::string, std::shared_ptr<ParentType>> &cache,
                                                          const std::string &key) const {
  auto it = cache.find(key);
  if (it != cache.end()) {
    return it->second;
  }
  return nullptr;
}

}
}
