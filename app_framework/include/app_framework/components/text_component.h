//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once
#include <array>

#include <app_framework/common.h>
#include <app_framework/component.h>
#include <app_framework/node.h>
#include <app_framework/render/mesh.h>

#include <stb_easy_font.h>

namespace ml {
namespace app_framework {

// Text component
class TextComponent final : public Component {
  RUNTIME_TYPE_REGISTER(TextComponent)
public:
  TextComponent() {
    mesh_ = std::make_shared<Mesh>(Buffer::Category::Dynamic, GL_UNSIGNED_SHORT);
  }
  ~TextComponent() = default;

  std::shared_ptr<Mesh> GetMesh() const {
    return mesh_;
  }

  void SetText(const char *text, float horizontal_alignment = 0.f /* left */,
               float vertical_alignment = 0.f /* top */) {
    int width = stb_easy_font_width((char *)text);
    int height = stb_easy_font_height((char *)text);

    font_data_.resize(g_max_verts_per_char * std::strlen(text));
    int num_quads = stb_easy_font_print(-width * horizontal_alignment, -height * vertical_alignment, (char *)text,
                                        nullptr, font_data_.data(), sizeof(font_data_[0]) * font_data_.size());

    vert_data_.resize(num_quads * 4);
    for (size_t i = 0; i < vert_data_.size(); ++i) {
      vert_data_[i] = glm::vec3(font_data_[i]);
    }

    auto num_indices = 6 * num_quads;
    index_data_.resize(num_indices);
    for (size_t i = 0; 6 * i < index_data_.size(); ++i) {
      index_data_[6 * i + 0] = 4 * i + 0;
      index_data_[6 * i + 1] = 4 * i + 1;
      index_data_[6 * i + 2] = 4 * i + 2;
      index_data_[6 * i + 3] = 4 * i + 0;
      index_data_[6 * i + 4] = 4 * i + 2;
      index_data_[6 * i + 5] = 4 * i + 3;
    }

    mesh_->UpdateMesh(vert_data_.data(), nullptr, vert_data_.size(), index_data_.data(), index_data_.size());
  }

private:
  static const constexpr size_t g_max_verts_per_char = 4 * 11;

  std::shared_ptr<Mesh> mesh_;

  std::vector<glm::vec4> font_data_;
  std::vector<glm::vec3> vert_data_;
  std::vector<uint16_t> index_data_;
};
}
}
