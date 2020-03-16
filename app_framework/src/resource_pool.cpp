//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#include <app_framework/node.h>
#include <app_framework/preset_resource.h>
#include <app_framework/resource_pool.h>
#include <app_framework/components/renderable_component.h>
#include <app_framework/material/pbr_material.h>
#include <app_framework/material/textured_material.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/texture.h>

#include <stb_image.h>

namespace ml {
namespace app_framework {

void ResourcePool::InitializePresetResources() {
  PresetResource preset_resource;
  for (const auto &mesh : preset_resource.meshes) {
    mesh_cache_[std::to_string(mesh->GetRuntimeType())] = mesh;
  }
}

std::shared_ptr<Node> ResourcePool::LoadNodeHierarchy(const std::string &path, const aiScene *ai_scene,
                                                      const aiNode *ai_node) {
  ML_LOG(Info, "Loading node %s", ai_node->mName.C_Str());
  auto node = std::make_shared<Node>();
  aiVector3D position{};
  aiQuaternion rotation{};
  aiVector3D scaling{};
  ai_node->mTransformation.Decompose(scaling, rotation, position);
  node->SetLocalTranslation(glm::vec3{position.x, position.y, position.z});
  node->SetLocalRotation(glm::quat{rotation.w, rotation.x, rotation.y, rotation.z});
  node->SetLocalScale(glm::vec3{scaling.x, scaling.y, scaling.z});

  for (size_t mesh_index = 0; mesh_index < ai_node->mNumMeshes; ++mesh_index) {
    auto model = LoadModel(path, ai_scene, ai_node->mMeshes[mesh_index]);
    auto renderable_pbr = std::make_shared<RenderableComponent>(model.mesh, model.material);

    auto model_node = std::make_shared<Node>();
    model_node->AddComponent(renderable_pbr);
    node->AddChild(model_node);
  }

  for (size_t child_index = 0; child_index < ai_node->mNumChildren; ++child_index) {
    node->AddChild(LoadNodeHierarchy(path, ai_scene, ai_node->mChildren[child_index]));
  }

  return node;
}

std::shared_ptr<Node> ResourcePool::LoadAsset(const std::string &path) {
  Assimp::Importer importer;
  const aiScene *ai_scene = importer.ReadFile(path.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                                                                aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
  if (ai_scene == nullptr || ai_scene->mNumMeshes <= 0) {
    ML_LOG(Error, "Unable to load model for file %s, %s", path.c_str(), importer.GetErrorString());
    return nullptr;
  }

  return LoadNodeHierarchy(path, ai_scene, ai_scene->mRootNode);
}

Model ResourcePool::LoadModel(const std::string &path, const aiScene *ai_scene, size_t mesh_index) {
  const aiMesh *ai_mesh = ai_scene->mMeshes[mesh_index];
  ML_LOG(Info, "Loading mesh %s", ai_mesh->mName.C_Str());

  Model model;
  std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(Buffer::Category::Static, GL_UNSIGNED_INT);
  std::shared_ptr<PBRMaterial> mat = std::make_shared<PBRMaterial>();

  const glm::vec3 *vertices = nullptr;
  const glm::vec3 *normals = nullptr;
  std::vector<uint32_t> indices;
  size_t num_vertices = 0;

  if (ai_mesh->HasPositions()) {
    vertices = (const glm::vec3 *)ai_mesh->mVertices;
    num_vertices = ai_mesh->mNumVertices;
  } else {
    ML_LOG(Error, "No vert data");
    return model;
  }

  if (ai_mesh->HasNormals()) {
    mat->SetHasNormals(true);
    normals = (const glm::vec3 *)ai_mesh->mNormals;
  }

  if (ai_mesh->HasFaces()) {
    indices.resize(ai_mesh->mNumFaces * 3);
    int indices_index = 0;
    for (int i = 0; i < ai_mesh->mNumFaces; ++i) {
      const aiFace ai_face = ai_mesh->mFaces[i];
      if (ai_face.mNumIndices != 3) {
        ML_LOG(Error, "Unexpected index cnt %d", ai_face.mNumIndices);
        return model;
      }
      indices[indices_index++] = ai_face.mIndices[0];
      indices[indices_index++] = ai_face.mIndices[1];
      indices[indices_index++] = ai_face.mIndices[2];
    }
  }

  ML_LOG(Debug, "Inited model vert:%d indices:%u", ai_mesh->mNumVertices, (uint32_t)indices.size());
  mesh->UpdateMesh(vertices, normals, ai_mesh->mNumVertices, indices.data(), indices.size());
  if (ai_mesh->HasTextureCoords(0)) {
    std::vector<glm::vec2> tex_coords(ai_mesh->mNumVertices);
    for (uint32_t i = 0; i < ai_mesh->mNumVertices; ++i) {
      tex_coords[i].x = ai_mesh->mTextureCoords[0][i].x;
      tex_coords[i].y = ai_mesh->mTextureCoords[0][i].y;
    }
    mesh->UpdateTexCoordsBuffer((glm::vec2 const *)tex_coords.data());
  }

  mesh_cache_.insert(std::make_pair(path, mesh));
  model.mesh = mesh;

  // Load material
  const aiMaterial *ai_mat = ai_scene->mMaterials[ai_mesh->mMaterialIndex];

  for (uint32_t i = 0; i < ai_mat->mNumProperties; ++i) {
    ML_LOG(Debug, "Property name: %s, sematic %x data size %u", ai_mat->mProperties[i]->mKey.data,
           ai_mat->mProperties[i]->mSemantic, ai_mat->mProperties[i]->mDataLength);
  }

  aiString name;
  ai_mat->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), name);
  ML_LOG(Debug, "property ...%s", name.data);

  // Test textures
  for (int32_t i = (int32_t)aiTextureType_NONE; i <= aiTextureType_UNKNOWN; ++i) {
    uint32_t tex_cnt = ai_mat->GetTextureCount((aiTextureType)i);
    if (tex_cnt == 0) {
      continue;
    }
    ML_LOG(Debug, "Texture cnt %u for %x", tex_cnt, i);
    for (uint32_t j = 0; j < tex_cnt; ++j) {
      aiString texture_path;
      auto res = ai_mat->GetTexture((aiTextureType)i, j, &texture_path);
      if (res != AI_SUCCESS) {
        ML_LOG(Error, "Failed for for %x.%u", i, j);
        continue;
      }
      std::shared_ptr<Texture> tex;
      const aiTexture *ai_tex = nullptr;
      std::string texture_full_name;
      bool embedded_texture = false;

      ML_LOG(Debug, "Texture name %s for %x.%u", texture_path.data, i, j);
      if (texture_path.data[0] == '*') {
        embedded_texture = true;
        int32_t ai_tex_index = atoi(&texture_path.data[1]);
        ai_tex = ai_scene->mTextures[ai_tex_index];
      } else {
        std::string base_filename = path.substr(path.find_last_of("/\\") + 1);
        std::string dir_name = path.substr(0, path.find(base_filename));
        texture_full_name = dir_name;
        texture_full_name.append((char *)texture_path.data);
        ML_LOG(Debug, "Loading texture name %s", texture_full_name.c_str());
      }

      switch (i) {
        case aiTextureType_DIFFUSE:
          tex = embedded_texture ? LoadAssimpEmbeddedTexture(ai_tex, GL_SRGB8_ALPHA8)
                                 : LoadTexture(texture_full_name, GL_SRGB8_ALPHA8);
          mat->SetAlbedo(tex);
          mat->SetHasAlbedo(true);
          ML_LOG(Debug, "Setting albedo...");
          break;
        case aiTextureType_SPECULAR:
          tex =
              embedded_texture ? LoadAssimpEmbeddedTexture(ai_tex, GL_RGBA8) : LoadTexture(texture_full_name, GL_RGBA8);
          mat->SetMetallicChannel(0);
          mat->SetMetallic(tex);
          mat->SetHasMetallic(true);
          ML_LOG(Debug, "Setting metallic...");
          break;
        case aiTextureType_LIGHTMAP:
        case aiTextureType_AMBIENT:
          tex =
              embedded_texture ? LoadAssimpEmbeddedTexture(ai_tex, GL_RGBA8) : LoadTexture(texture_full_name, GL_RGBA8);
          mat->SetAmbientOcclusion(tex);
          mat->SetHasAmbientOcclusion(true);
          ML_LOG(Debug, "Setting ao...");
          break;
        case aiTextureType_EMISSIVE:
          tex = embedded_texture ? LoadAssimpEmbeddedTexture(ai_tex, GL_SRGB8_ALPHA8)
                                 : LoadTexture(texture_full_name, GL_SRGB8_ALPHA8);
          mat->SetEmissive(tex);
          mat->SetHasEmissive(true);
          ML_LOG(Debug, "Setting emissive...");
          break;
        case aiTextureType_NORMALS:
        case aiTextureType_HEIGHT:
          tex =
              embedded_texture ? LoadAssimpEmbeddedTexture(ai_tex, GL_RGBA8) : LoadTexture(texture_full_name, GL_RGBA8);
          mat->SetNormals(tex);
          mat->SetHasNormalMap(true);
          ML_LOG(Debug, "Setting normal map...");
          break;
        case aiTextureType_SHININESS:
          tex =
              embedded_texture ? LoadAssimpEmbeddedTexture(ai_tex, GL_RGBA8) : LoadTexture(texture_full_name, GL_RGBA8);
          mat->SetRoughnessChannel(0);
          mat->SetRoughness(tex);
          mat->SetHasRoughness(true);
          ML_LOG(Debug, "Setting roughness...");
          break;
        case aiTextureType_UNKNOWN:
          tex =
              embedded_texture ? LoadAssimpEmbeddedTexture(ai_tex, GL_RGBA8) : LoadTexture(texture_full_name, GL_RGBA8);
          mat->SetMetallicChannel(2);
          mat->SetRoughnessChannel(1);
          mat->SetMetallic(tex);
          mat->SetRoughness(tex);
          mat->SetHasRoughness(true);
          mat->SetHasMetallic(true);
          ML_LOG(Debug, "Setting metallic and roughness...");
        default: break;
      }
    }
  }

  static_material_cache_.insert(std::make_pair(path, mat));
  model.material = mat;
  return model;
}

std::shared_ptr<Texture> ResourcePool::LoadAssimpEmbeddedTexture(const aiTexture *ai_tex, GLint gl_internal_format) {
  int32_t channels = 0;
  GLuint gl_texture = 0;
  int32_t width = 0;
  int32_t height = 0;
  void *buffer = nullptr;
  glGenTextures(1, &gl_texture);
  glBindTexture(GL_TEXTURE_2D, gl_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  std::shared_ptr<Texture> texture = std::make_shared<Texture>(GL_TEXTURE_2D, gl_texture, width, height, true);

  if (ai_tex->mHeight == 0) {
    ML_LOG(Debug, "Compressed texture %u %u, image format %s", ai_tex->mWidth, ai_tex->mHeight, ai_tex->achFormatHint);
    buffer = stbi_load_from_memory((unsigned char *)ai_tex->pcData, ai_tex->mWidth, &width, &height, &channels,
                                   STBI_rgb_alpha);
  } else {
    width = ai_tex->mWidth;
    height = ai_tex->mHeight;
    buffer = ai_tex->pcData;
  }

  glTexImage2D(GL_TEXTURE_2D, 0, gl_internal_format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
  texture->SetWidth(width);
  texture->SetHeight(height);

  if (ai_tex->mHeight == 0) {
    stbi_image_free(buffer);
  }

  glBindTexture(GL_TEXTURE_2D, 0);
  return texture;
}

std::shared_ptr<Texture> ResourcePool::LoadTexture(const std::string &path, GLint gl_internal_format) {
  auto texture = GetCacheElement<Texture>(texture_cache_, path);
  if (texture) {
    return texture;
  }

  int32_t channels = 0;
  GLuint gl_texture = 0;
  int32_t width = 0;
  int32_t height = 0;

  unsigned char *buffer = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
  if (buffer == nullptr) {
    ML_LOG(Error, "Unable to load texture %s", path.c_str());
    return nullptr;
  }
  ML_LOG(Debug, "Number of channels for image %s is %d", path.c_str(), channels);
  glGenTextures(1, &gl_texture);
  glBindTexture(GL_TEXTURE_2D, gl_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, gl_internal_format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
  glBindTexture(GL_TEXTURE_2D, 0);
  stbi_image_free(buffer);

  texture = std::make_shared<Texture>(GL_TEXTURE_2D, gl_texture, width, height, true);
  texture_cache_.insert(std::make_pair(path, texture));
  return texture;
}

}  // namespace app_framework
}  // namespace ml
