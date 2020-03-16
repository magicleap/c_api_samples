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

#include <app_framework/cli_args_parser.h>

#include <gflags/gflags.h>
#include <ml_lifecycle.h>
#include <ml_logging.h>

#include <sstream>
#include <string>
#include <cctype>

namespace ml {
namespace app_framework {
namespace cli_args_parser {

static bool IsMatch(char val1, char val2) {
  return val1 == val2 || (std::isspace(val1) && std::isspace(val2));
}

// This function finds the first argument from the given string
// and returns the pointer to the next argument. The arguments are
// separated by a whitespace, but the whitespaces can be in the quotes
// such as ' or ". Consider the following case :
// - a'b " c''fg' "h'i" jk  ==> {"ab \" cfg", "h'i", "jk"}
static const char *FindNextArg(const char *args, std::string &value) {
  const std::string quotes("'\"");

  // remove leading whitespaces
  for (; std::isspace(*args) && *args != '\0'; args++) continue;

  // Find the last character of the argument string. Depending on the start character, the
  // argument string should end with either a whitespace, a quote, or a NULL character
  std::stringstream value_stream;
  char start_char = ' ';
  if (quotes.find(*args) != std::string::npos) {
    start_char = *args++;
  }

  // concatenate until the last character is found
  bool has_contiguous = false;
  for (; *args != '\0'; args++) {
    if (IsMatch(start_char, *args)) {
      // the last character is found, but not followed by a space
      if (!std::isspace(start_char) && !std::isspace(*++args)) {
        has_contiguous = true;
      }
      break;
    }

    // a quote is found, but the string does not start with a quote
    if (std::isspace(start_char) && (quotes.find(*args) != std::string::npos)) {
      has_contiguous = true;
      break;
    }
    value_stream << *args;
  }
  if (has_contiguous && *args != '\0') {
    // concatenate the contiguous argument string
    std::string to_append = "";
    args = FindNextArg(args, to_append);
    value_stream << to_append;
  }

  // remove trailing whitespaces
  for (; std::isspace(*args) && *args != '\0'; args++) continue;

  value = value_stream.str();
  return args;
}

void ParseGflags(int argc, char **argv) {
  if (argc > 0) {
    // Ignore unknown flags
    gflags::AllowCommandLineReparsing();
    gflags::ParseCommandLineFlags(&argc, &argv, true);
  }
}

void ParseGflags(const std::vector<std::string> &argv) {
  // convert vector<string> to char** so that it can be passed onto gflags
  std::vector<const char *> argv_char;
  argv_char.reserve(argv.size());
  for (const std::string &str : argv) {
    argv_char.push_back(str.c_str());
  }
  int argc_new = static_cast<int>(argv_char.size());
  char **argv_new = const_cast<char **>(argv_char.data());
  ParseGflags(argc_new, argv_new);
}

std::vector<std::string> GetArgs(const char *args) {
  std::vector<std::string> arg_list;
  if (!args) {
    return arg_list;
  }

  std::string arg;
  while (*args != '\0') {
    args = FindNextArg(args, arg);
    if (arg.size() > 0) {
      arg_list.push_back(arg);
    }
  }
  return arg_list;
}

}  // cli_args_parser
}  // namespace app_framework
}  // namespace ml
