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

#pragma once

#include <string>
#include <vector>

namespace ml {
namespace app_framework {
namespace cli_args_parser {

/*!
  \brief Parses the command line arguments using gflags.
  \param argc Number of arguments.
  \param argv Argument vector.
*/
void ParseGflags(int argc, char **argv);

/*!
  \brief Parses the command line arguments using gflags.
  \param argv Argument vector.
*/
void ParseGflags(const std::vector<std::string> &argv);

/*!
  \brief Get a list of arguments separated by whitespaces, unless it is quoted.
  \param arg_str Null-terminated string of arguments.
  \return A list of arguments(empty if arg_str is nullptr).
*/
std::vector<std::string> GetArgs(const char *arg_str);

}  // cli_args_parser
}  // namespace app_framework
}  // namespace ml
