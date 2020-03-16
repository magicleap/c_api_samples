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
#include <app_framework/input/input_command_handler.h>
#include <app_framework/ml_macros.h>
#include <app_framework/cli_args_parser.h>

#include <iterator>
#include <sstream>

namespace ml {
namespace app_framework {

InputCommandHandler::InputCommandHandler() {
}

InputCommandHandler::~InputCommandHandler() {
  Stop();
}

void InputCommandHandler::AddInputHandler(std::shared_ptr<InputHandler> input) {
  input_handlers_.push_back(input);
}

void InputCommandHandler::Add(const std::string &command, const std::string &description, const ExecuteFn &handler_fn) {
  // Overwrite if the key exists.
  if (handler_fn && command.size() > 0) {
    all_commands_[command] = {handler_fn, description};
    commands_with_description_.push_back(std::make_pair(command, description));
  }
}

void InputCommandHandler::SetErrorHandler(const ExecuteFn &handler_fn) {
  if (handler_fn) {
    error_handler_ = {handler_fn, std::string()};
  }
}

void InputCommandHandler::Start() {
  if (input_handlers_.size() == 0) {
    return;
  }

  for (const auto &input_handler : input_handlers_) {
    auto on_char = [this](InputHandler::EventArgs key_event_args) {
      OnChar(key_event_args);
    };

    auto on_key_up = [this](InputHandler::EventArgs key_event_args) {
      OnKeyUp(key_event_args);
    };

    input_handler->SetEventHandlers(nullptr, on_char, on_key_up);
  }
}

void InputCommandHandler::Stop() {
  if (input_handlers_.size() == 0) {
    return;
  }

  for (const auto &input_handler : input_handlers_) {
    input_handler->SetEventHandlers(nullptr, nullptr, nullptr);
  }
}

std::vector<std::pair<std::string, std::string>> InputCommandHandler::GetAllAvailableCommands() const {
  return commands_with_description_;
}

void InputCommandHandler::OnChar(InputHandler::EventArgs key_event_args) {
  if (key_event_args.key_char >= 0x20 && key_event_args.key_char <= 0x7E) {
    std::lock_guard<std::mutex> locker(input_mutex_);
    command_buffer_.push_back(static_cast<char>(key_event_args.key_char));
  }
}

void InputCommandHandler::OnKeyUp(InputHandler::EventArgs key_event_args) {
  std::lock_guard<std::mutex> locker(input_mutex_);
  if (command_buffer_.length() == 0 || key_event_args.key_code >= MLKEYCODE_COUNT) {
    return;
  }

  MLKeyCode key_code = static_cast<MLKeyCode>(key_event_args.key_code);
  if (key_code == MLKEYCODE_ENTER) {
    command_queue_.push(command_buffer_);
    command_buffer_ = "";
  } else if (key_code == MLKEYCODE_DEL) {
    command_buffer_.pop_back();
  }
}

void InputCommandHandler::PushCommand(const std::string &command) {
  if (command.empty()) {
    return;
  }
  std::lock_guard<std::mutex> locker(input_mutex_);
  command_queue_.push(command);
}

std::queue<std::string> InputCommandHandler::GetCommands() {
  // read and clear the command queue
  std::queue<std::string> command_queue_read_;

  std::lock_guard<std::mutex> locker(input_mutex_);
  if (!command_queue_.empty()) {
    std::swap(command_queue_, command_queue_read_);
  }
  return command_queue_read_;
}

std::string InputCommandHandler::GetCommand() {
  std::string command;
  std::lock_guard<std::mutex> locker(input_mutex_);
  if (!command_queue_.empty()) {
    command = command_queue_.front();
    command_queue_.pop();
  }
  return command;
}

void InputCommandHandler::ProcessCommands() {
  std::queue<std::string> command_queue = GetCommands();
  while (!command_queue.empty()) {
    ProcessCommand(command_queue.front());
    command_queue.pop();
  }
}

void InputCommandHandler::ProcessCommand() {
  std::string command = GetCommand();
  if (!command.empty()) {
    ProcessCommand(command);
  }
}

void InputCommandHandler::ProcessCommand(const std::string &command) {
  ML_LOG(Verbose, "Processing the command \"%s\".", command.c_str());

  // Split the command line string by whitespace.
  std::vector<std::string> args = cli_args_parser::GetArgs(command.c_str());
  if (args.size() == 0) {
    return;
  }

  InputCommandHandler::Attributes attributes = GetCommandAttributes(args[0]);
  if (attributes.fn) {
    // Invoke handler function.
    attributes.fn(args);
  } else {
    // Command not found, invoke the error handler.
    ML_LOG(Info, "Command not found, invoking the error handler.");
    if (error_handler_.fn) {
      error_handler_.fn(std::vector<std::string>());
    } else {
      DefaultErrorHandler(args);
    }
  }
}

InputCommandHandler::Attributes InputCommandHandler::GetCommandAttributes(const std::string &cmd) const {
  auto found_entry = all_commands_.find(cmd.c_str());
  if (found_entry != all_commands_.end()) {
    return found_entry->second;
  } else {
    return {nullptr, std::string()};
  }
}

void InputCommandHandler::DefaultErrorHandler(const std::vector<std::string> &commands) {
  // List all available commands with their description.
  ML_LOG(Info, "Unknown command \"%s\". Available Commands :", commands[0].c_str());
  for (auto &entry : commands_with_description_) {
    ML_LOG(Info, "  %s\t\t%s", entry.first.c_str(), entry.second.c_str());
  }
}

}  // namespace app_framework
}  // namespace ml

