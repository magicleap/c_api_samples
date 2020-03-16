// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#include <app_framework/convert.h>

namespace ml {
namespace app_framework {

std::string to_string(const MLCoordinateFrameUID &cfuid) {
  char buffer[37];
  static constexpr char hex_chars[] = "0123456789ABCDEF";

  static const auto to_hex = [&](char* two_chars, uint8_t data) {
    two_chars[0] = hex_chars[(data >> 4) & 0X0F];
    two_chars[1] = hex_chars[data        & 0x0F];
  };

  to_hex(buffer +  0,  cfuid.data[0]        & 0xFF);
  to_hex(buffer +  2, (cfuid.data[0] >>  8) & 0xFF);
  to_hex(buffer +  4, (cfuid.data[0] >> 16) & 0xFF);
  to_hex(buffer +  6, (cfuid.data[0] >> 24) & 0xFF);
  buffer[8] = '-';
  to_hex(buffer +  9, (cfuid.data[0] >> 32) & 0xFF);
  to_hex(buffer + 11, (cfuid.data[0] >> 40) & 0xFF);
  buffer[13] = '-';
  to_hex(buffer + 14, (cfuid.data[0] >> 48) & 0xFF);
  to_hex(buffer + 16, (cfuid.data[0] >> 56) & 0xFF);
  buffer[18] = '-';
  to_hex(buffer + 19,  cfuid.data[1]        & 0xFF);
  to_hex(buffer + 21, (cfuid.data[1] >>  8) & 0xFF);
  buffer[23] = '-';
  to_hex(buffer + 24, (cfuid.data[1] >> 16) & 0xFF);
  to_hex(buffer + 26, (cfuid.data[1] >> 24) & 0xFF);
  to_hex(buffer + 28, (cfuid.data[1] >> 32) & 0xFF);
  to_hex(buffer + 30, (cfuid.data[1] >> 40) & 0xFF);
  to_hex(buffer + 32, (cfuid.data[1] >> 48) & 0xFF);
  to_hex(buffer + 34, (cfuid.data[1] >> 56) & 0xFF);
  buffer[36] = '\0';

  return std::string(buffer);
}

}  // namespace app_framework
}  // namespace ml
