//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once

namespace ml {
namespace app_framework {

static const char *kPBRFragmentShader = R"GLSL(
  #version 410 core
  // BRDF functinos refers the same one used in https://github.com/KhronosGroup/glTF-WebGL-PBR/

  uniform sampler2D Albedo;
  uniform sampler2D Metallic;
  uniform sampler2D Roughness;
  uniform sampler2D Normals;
  uniform sampler2D AmbientOcclusion;
  uniform sampler2D Emissive;

  const vec3 Fc = vec3(0.04, 0.04, 0.04);
  const float PI = 3.14159265359;

  layout (location = 0) in vec3 in_world_position;
  layout (location = 1) in vec3 in_normal;
  layout (location = 2) in vec2 in_tex_coords;

  layout (location = 0) out vec4 out_color;

  layout(std140) uniform Camera {
    mat4 view_proj;
    vec4 world_position;
  } camera;

  layout(std140) uniform Material {
    int MetallicChannel;
    int RoughnessChannel;
    bool HasNormals;
    bool HasAlbedo;
    bool HasNormalMap;
    bool HasMetallic;
    bool HasRoughness;
    bool HasAmbientOcclusion;
    bool HasEmissive;
  } material;

  struct Light {
    vec3 light_position;
    float light_strength;
    vec3 light_direction;
    int light_type;
    vec3 light_color;
    float pad;
  };

  layout(std140) uniform Lights {
    Light lights_array[32];
    int number_of_lights;
  } lights;

  vec3 SpecularReflection(float cos_theta, vec3 F0) {
    return F0 + (1 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
  }

  float GeometricOcclusion(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotL = clamp(dot(N, L), 0.001, 1.0);
    float NdotV = clamp(abs(dot(N, V)), 0.001, 1.0);
    float r = roughness;

    float attenuation_L = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
    float attenuation_V = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));
    return attenuation_L * attenuation_V;
  }

  float MicrofacetDistribution(vec3 N, vec3 H, float roughness) {
    float NdotH = clamp(dot(N, H), 0.0, 1.0);
    float roughness_Sq = roughness * roughness;
    float f = (NdotH * roughness_Sq - NdotH) * NdotH + 1.0;
    return roughness_Sq / (PI * f * f);
  }

  // Convert the tangent space normal to world space
  vec3 GetWorldNormal() {
    vec3 dPx = dFdx(in_world_position);
    vec3 dPy = dFdy(in_world_position);
    vec3 N = normalize(cross(dPx, dPy));
    vec2 dUVx = dFdx(in_tex_coords);
    vec2 dUVy = dFdy(in_tex_coords);

    if (material.HasNormals) {
      N = normalize(in_normal);
    }

    if (!material.HasNormalMap) {
      // Just return surface normal
      return N;
    }

    vec3 tangent_normal = texture(Normals, in_tex_coords).xyz * 2.0 - 1.0;
    vec3 T = (dUVy.t * dPx - dUVx.t * dPy) / (dUVx.s * dUVy.t - dUVy.s * dUVx.t);
    T = normalize(T - N * dot(N, T));
    vec3 B = normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * tangent_normal);
  }

  void main() {
    float roughness = 0.5f;
    float metallic = 0.0f;

    if (material.HasRoughness) {
      vec4 metallic_sample = texture(Metallic, in_tex_coords);
      if (material.MetallicChannel == 0) {
        metallic = metallic_sample.r;
      } else if (material.MetallicChannel == 1) {
        metallic = metallic_sample.g;
      } else if (material.MetallicChannel == 2) {
        metallic = metallic_sample.b;
      } else {
        metallic = metallic_sample.a;
      }
    }

    if (material.HasMetallic) {
      vec4 roughness_sample = texture(Roughness, in_tex_coords);
      if (material.RoughnessChannel == 0) {
        roughness = roughness_sample.r;
      } else if (material.RoughnessChannel == 1) {
        roughness = roughness_sample.g;
      } else if (material.RoughnessChannel == 2) {
        roughness = roughness_sample.b;
      } else {
        roughness = roughness_sample.a;
      }
    }

    vec4 albedo = vec4(1.0f);
    if (material.HasAlbedo) {
      albedo = texture(Albedo, in_tex_coords);
    }

    vec3 F0 = mix(Fc, albedo.rgb, metallic);
    vec3 V = normalize(camera.world_position.xyz - in_world_position);
    vec3 N = GetWorldNormal();
    vec3 color = vec3(0.0);

    for (int i = 0; i < lights.number_of_lights; ++i) {
      float attenuation = 1.0f;
      vec3 L;
      if (lights.lights_array[i].light_type == 0) {
        // Point light
        L = normalize(lights.lights_array[i].light_position - in_world_position);
        float distance    = length(lights.lights_array[i].light_position - in_world_position);
        attenuation = 1.0 / (distance * distance);
      } else {
        // Directional light
        L = -normalize(lights.lights_array[i].light_direction);
      }
      vec3 H = normalize(V + L);
      vec3 F = SpecularReflection(clamp(dot(H, V), 0.0, 1.0), F0);
      float G = GeometricOcclusion(N, V, L, roughness);
      float D = MicrofacetDistribution(N, H, roughness);

      float NdotL = clamp(dot(N, L), 0.001, 1.0);
      float NdotV = clamp(abs(dot(N, V)), 0.001, 1.0);

      vec3 diffuse_part = (albedo.rgb * (vec3(1.0) - F)) / PI;
      diffuse_part *= 1 - metallic;
      vec3 spectacular_part = F * G * D / max(4.0 * NdotL * NdotV, 0.0001);

      color += NdotL * lights.lights_array[i].light_color * lights.lights_array[i].light_strength * attenuation * (diffuse_part + spectacular_part);
    }

    if (material.HasAmbientOcclusion) {
      float ao = texture(AmbientOcclusion, in_tex_coords).r;
      color = color * ao;
    }

    if (material.HasEmissive) {
      vec3 emissive = texture(Emissive, in_tex_coords).rgb;
      color += emissive;
    }

    color = color / (color + vec3(1.0));
    out_color = vec4(color, 1.0f);
  }
)GLSL";

}
}
