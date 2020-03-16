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
#include <app_framework/application.h>
#include <app_framework/ml_macros.h>
#include <app_framework/toolset.h>
#include <gflags/gflags.h>

#include <ml_connections.h>
#include <ml_privileges.h>

#include <future>
#include <thread>

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

DEFINE_bool(Sender, false, "If true, instance will be sender otherwise receiver of invite.");
DEFINE_string(ServerIP, "", "Server IP address");
DEFINE_int32(ServerPort, 8080, "Server Port");

class ConnectionsApp : public ml::app_framework::Application {
public:
  ConnectionsApp(int argc = 0, char **argv = nullptr) : ml::app_framework::Application(argc, argv) {}

  void OnStart() override {
    using namespace ml::app_framework;

    RequestPrivileges();
    UNWRAP_MLRESULT(MLConnectionsRegistrationStartup(&connections_));
    UNWRAP_MLRESULT(MLConnectionsStartup());

    MLConnectionsInviteCallbacks callbacks = {};
    callbacks.on_registration_complete = ConnectionsApp::OnRegistration;
    callbacks.on_invitation = ConnectionsApp::OnInvitation;
    UNWRAP_MLRESULT(MLConnectionsRegisterForInvite(connections_, callbacks, this));
    ML_LOG(Info, "Registering for invites.");

    if (FLAGS_Sender) {
      server_ = std::async([&]{
          ServerMain();
        });

      MLConnectionsInviteArgs args = {};
      args.invitee_count = 1;
      args.invite_user_prompt = "CAPI Test: Invite Others?";
      args.invite_payload = std::string(FLAGS_ServerIP).c_str();
      ML_LOG(Info, "invite_payload: %s", args.invite_payload);
      args.default_invitee_filter = MLConnectionsInviteeFilter::MLConnectionsInviteeFilter_Followers;

      // if successful this will block, if not it returns fail
      UNWRAP_MLRESULT(MLConnectionsRequestInvite(&args, &invite_request_));
    }
  }

  void OnStop() override {
    if (FLAGS_Sender) {
      if (server_.valid()) {
        server_.wait();
      }
    } else {
      if (client_.valid()) {
        client_.wait();
      }
    }
    MLConnectionsShutdown();
    MLConnectionsRegistrationShutdown(connections_);
    MLConnectionsReleaseRequestResources(invite_request_);
    MLPrivilegesShutdown();
  }

  void OnUpdate(float delta_time_scale) override {
    if (FLAGS_Sender) {
      if (invite_request_ != ML_INVALID_HANDLE) {
        MLConnectionsInviteStatus status;
        UNWRAP_MLRESULT(MLConnectionsTryGetInviteStatus(invite_request_, &status));
        switch (status) {
          case MLConnectionsInviteStatus_SubmittingRequest:
            ML_LOG(Debug, "invite status: SubmittingRequest");
            break;
          case MLConnectionsInviteStatus_Pending:
            ML_LOG(Debug, "invite status: Pending");
            break;
          case MLConnectionsInviteStatus_Cancelled:
            ML_LOG(Debug, "invite status: Canceled");
          case MLConnectionsInviteStatus_DispatchFailed:
            ML_LOG(Debug, "invite status: DispatchFailed");
          case MLConnectionsInviteStatus_InvalidHandle:
            ML_LOG(Debug, "invite status: InvalidHandle");
          case MLConnectionsInviteStatus_Dispatched:
            ML_LOG(Debug, "invite status: Dispatched");
            UNWRAP_MLRESULT(MLConnectionsReleaseRequestResources(invite_request_));
            invite_request_ = ML_INVALID_HANDLE;
            break;
          default:
            ML_LOG(Error, "invite status not handled");
            break;
        }
      }
    } else { // receiver
      // everything is handled via callbacks
    }
  }

  void RequestPrivileges() {
    // Request all the needed privileges
    UNWRAP_MLRESULT(MLPrivilegesStartup());

    MLResult result = MLPrivilegesRequestPrivilege(MLPrivilegeID_LocalAreaNetwork);
    switch (result) {
      case MLPrivilegesResult_Granted: ML_LOG(Info, "Successfully acquired the LocalAreaNetwork Privilege"); break;
      case MLResult_UnspecifiedFailure: ML_LOG(Fatal, "Error requesting privileges, exiting the app."); break;
      case MLPrivilegesResult_Denied:
        ML_LOG(Fatal, "LocalAreaNetwork privilege denied, exiting the app. Error code: %s", MLPrivilegesGetResultString(result));
        break;
      default: ML_LOG(Fatal, "Unknown privilege error, exiting the app."); break;
    }
  }

private:
  static void OnRegistration(MLResult status, void *context) {
    UNWRAP_MLRESULT(status);
    ML_LOG(Info, "OnRegistration complete.");
  }

  static void OnInvitation(bool user_accepted_from_icon_grid, const char* payload, void *context) {
    auto app = reinterpret_cast<ConnectionsApp*>(context);
    ML_LOG(Info, "Received Invitation: user_accepted_from_icon_grid: %d, payload: %s", user_accepted_from_icon_grid, payload);
    std::string ip_payload(payload);

    if (user_accepted_from_icon_grid) {
      // flow where a user does not have the app open before getting the invite
    } else {
      // flow where a user has the app open and receives the invite
    }

    app->client_ = std::async([=]{
        // given the ip address connect to the server
        app->ClientMain(ip_payload);
      });
  }

  void ServerMain() {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    std::string hello("Hello from server");

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      ML_LOG(Info, "socket failed");
      StopApp();
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt)) < 0) {
      ML_LOG(Info, "setsockopt failed");
      StopApp();
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(FLAGS_ServerPort);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
      ML_LOG(Info, "bind failed");
      StopApp();
    }

    if (listen(server_fd, 3) < 0) {
      ML_LOG(Info, "listen failed");
      StopApp();
    }

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t*)&addrlen)) < 0) {
      ML_LOG(Info, "accept failed");
      StopApp();
    }

    valread = read(new_socket, buffer, 1024);
    ML_LOG(Info, "%s", buffer);
    send(new_socket, hello.data(), hello.size() + 1 , 0);
    ML_LOG(Info, "Hello message sent.");
    StopApp();
  }

  void ClientMain(const std::string server_ip) {
    struct sockaddr_in address;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    std::string hello("Hello from client");
    char buffer[1024] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
      ML_LOG(Info, "socket creation failed");
      StopApp();
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(FLAGS_ServerPort);

    // Convert IPv4 and IPv6 addresses from text to binary form
    int e = inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr);
    if (e <= 0) {
      ML_LOG(Info, "Invalid address: %s/ Address not supported error: %d", server_ip.c_str(), e);
      StopApp();
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
      ML_LOG(Info, "Connection Failed");
      StopApp();
    }
    send(sock , hello.data() , hello.size() + 1, 0);
    ML_LOG(Info, "Hello message sent.");
    valread = read(sock, buffer, 1024);
    ML_LOG(Info, "%s", buffer);
    StopApp();
  }

  MLHandle connections_;
  MLHandle invite_request_;
  std::future<void> server_, client_;
};

int main(int argc, char **argv) {
  ConnectionsApp app(argc, argv);
  app.SetUseGfx(true);
  app.RunApp();
  return 0;
}
