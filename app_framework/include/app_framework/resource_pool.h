//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once

#include "app_framework/common.h"

#include <algorithm>
#include <unordered_map>
#include <string>

struct aiNode;
struct aiScene;
struct aiTexture;

namespace ml {
namespace app_framework {

class Node;
class Mesh;
class Material;
class Program;
class Texture;

struct Model {
  std::shared_ptr<Mesh> mesh;
  std::shared_ptr<Material> material;
};

class PBRMaterial;

// Load and cache the resource instance
class ResourcePool final {
public:
  ResourcePool() = default;
  ~ResourcePool() = default;

  void InitializePresetResources();

  // Get the pre-initialized static Mesh
  // Please use std::make_shared<Mesh> if you want to have a new Mesh instance and by pass the caching behavior
  template <typename MeshType>
  std::shared_ptr<MeshType> GetMesh() const;

  // Load a model from a 3D file and cache it, the returned material instance will always be a new one
  std::shared_ptr<Node> LoadAsset(const std::string &path);

  // Load a image as Texture and cache it
  std::shared_ptr<Texture> LoadTexture(const std::string &path, GLint gl_internal_format = GL_SRGB8_ALPHA8);

  // Load a GLSL as Program and cache it
  template <typename ProgramType>
  std::shared_ptr<ProgramType> LoadShaderFromFile(const std::string &path);

  // Load a GLSL as Code and cache it, identifier is the one used as the cache key
  template <typename ProgramType>
  std::shared_ptr<ProgramType> LoadShaderFromCode(const std::string &code, const std::string& identifier = std::string());
  template <typename ProgramType>
  std::shared_ptr<ProgramType> LoadShaderFromCode(const char* code, const std::string& identifier = std::string());

private:
  template <typename Type, typename ParentType, typename... Args>
  std::shared_ptr<Type> GetOrCreateElement(std::unordered_map<std::string, std::shared_ptr<ParentType>> &cache,
                                      const std::string &key, Args &&... args);
  template <typename Type, typename ParentType, typename... Args>
  std::shared_ptr<Type> CreateCacheElement(std::unordered_map<std::string, std::shared_ptr<ParentType>> &cache,
                                           const std::string &key, Args &&... args);
  template <typename ParentType>
  std::shared_ptr<ParentType> GetCacheElement(const std::unordered_map<std::string, std::shared_ptr<ParentType>> &cache,
                                              const std::string &key) const;

  // Load a model from a 3D file and cache it, the returned material instance will always be a new one
  Model LoadModel(const std::string &path, const aiScene *ai_scene, size_t mesh_index);
  std::shared_ptr<Node> LoadNodeHierarchy(const std::string &path, const aiScene *ai_scene, const aiNode *ai_node);

  std::shared_ptr<Texture> LoadAssimpEmbeddedTexture(const aiTexture *ai_tex, GLint gl_internal_format);

  std::unordered_map<std::string, std::shared_ptr<Program>> program_cache_;
  std::unordered_map<std::string, std::shared_ptr<Texture>> texture_cache_;
  std::unordered_map<std::string, std::shared_ptr<Mesh>> mesh_cache_;
  std::unordered_map<std::string, std::shared_ptr<PBRMaterial>> static_material_cache_;
};
}
}

#include "resource_pool.inl"
