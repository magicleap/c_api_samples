//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#include "texture.h"
#include "stb_image.h"

namespace ml {
namespace app_framework {

Texture::~Texture() {
  if (owned_) {
    glDeleteTextures(1, &texture_);
    texture_ = 0;
  }
}

}
}
