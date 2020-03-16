# Connections Sample

This sample uses the MLConnections API to send an invite to another ML
user running the same app.  This can be used to setup the
communications for a multi-user experience.  The app is run as a
sender on one device and receiver of an invite on the other.  The
sender sends an invite to the other device and starts a server thread
waiting for a socket connection.  The receiver of the invite accepts
and starts a client thread, creating a socket and connecting to the
server, in this case the other device.  Who a user can send an invite
to is managed by the Social App where you can add/remove followers via
email or ML ID.

## Prerequisites
  - Add another user in the social app so you can send an invite to them.

## Gui
  - None

## Launch from Cmd Line

Host: ./connections --help

Device 1 (Sender of Invite): mldb launch -i "-Sender=1 -ServerIP="<ip address of Device 1> -ServerPort=8080" com.magicleap.capi.sample.connections

Device 2 (Receiver of Invite): mldb launch -i "-Sender=0" com.magicleap.capi.sample.connections

## What to Expect

 - Sender should see a social friend picker dialog where you pick the
   person to invite and click ok.  Once that happens, the app will wait
   for the connections from the invitee, print out a hello world
   message, and then exit.

 - Note the server is not coded for multiple invites.  It will accept
   the first connection, respond back and exit.  A different network
   diagram would have to be coded for a 1 to many server/client setup.
