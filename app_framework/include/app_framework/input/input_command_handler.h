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

#include <app_framework/input/input_handler.h>

#include <ml_input.h>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ml {
namespace app_framework {

class InputCommandHandler {
public:
  typedef std::function<void(std::vector<std::string>)> ExecuteFn;
  struct Attributes {
    ExecuteFn fn;
    std::string description;
  };

  InputCommandHandler();
  ~InputCommandHandler();

  /*!
    \brief Adds an input handler.

    \param[in] input InputHandler object which needs to implement key event functions.
  */
  void AddInputHandler(std::shared_ptr<InputHandler> input);

  /*!
    \brief Adds a command handler.

    \param[in] command Command to register.
    \param[in] description Description of the command.
    \param[in] fn Function to execute when the command is received.
  */
  void Add(const std::string &command, const std::string &description, const ExecuteFn &fn);

  /*!
    Adds a error handler function to be called when unknown command is received.

    \param[in] fn Function to execute when unknonwn command is received.
  */
  void SetErrorHandler(const ExecuteFn &fn);

  /*!
    \brief Starts the input command handler.

    Use the GetCommand() and/or ProcessCommand() to query and execute the user input.
  */
  void Start();

  /*!
    \brief Stops the input command handler.

    This function sets the keyboard callback functions to nullptr.
  */
  void Stop();

  /*!
    Pushes a command line into a queue so that it can be processed in the order it was received.

    Useful in case the the command line is read in a custom way(non-InputHandler).

    \param[in] command Command line to push into the queue.
  */
  void PushCommand(const std::string &command);

  /*!
    Queries all command lines in queue received by user input interface.

    \return Command lines received by user input interface. Empty if not exist.
  */
  std::queue<std::string> GetCommands();

  /*!
    Queries a command line received by user input interface(FIFO).

    \return Command line received by user input interface. Empty if not exist.
  */
  std::string GetCommand();

  /*!
    Gets Attributes of the command which contains the handler function and description.

    \return Command(without arguments) received by user input interface. Empty if not exist.
  */
  InputCommandHandler GetCommandAttributes();

  /*!
    Queries and executes a command line received by user input interface(FIFO).
  */
  void ProcessCommand();

  /*!
    Queries and executes all command lines received by user input interface(FIFO).
  */
  void ProcessCommands();

  /*!
    Executes a command.

    \param[in] command Commnad line to execute(retrieved via GetCommand() and GetCommands()).
  */
  void ProcessCommand(const std::string &command);

  /*!
    Returns all commands available(registered via the Add function.

    \return List of all available command. Each element is a pair - command and description.
  */
  std::vector<std::pair<std::string, std::string>> GetAllAvailableCommands() const;

private:
  void OnChar(InputHandler::EventArgs key_event_args);
  void OnKeyUp(InputHandler::EventArgs key_event_args);

  Attributes GetCommandAttributes(const std::string &cmd) const;
  void DefaultErrorHandler(const std::vector<std::string> &commands);

  // commands
  std::unordered_map<std::string, Attributes> all_commands_;
  std::vector<std::pair<std::string, std::string>> commands_with_description_;
  Attributes error_handler_;

  // for key event handlers
  std::vector<std::shared_ptr<InputHandler>> input_handlers_;
  std::mutex input_mutex_;
  std::string command_buffer_;
  std::queue<std::string> command_queue_;
};

}  // namespace app_framework
}  // namespace ml
