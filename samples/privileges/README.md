# Privileges Sample

This sample shows how to request privileges from the system.  You can
make a blocking call where sensitive privileges will pop a dialog box.
You can also call asynchronously and use the
MLPrivilegesRequestPrivilegeTryGet to determine whether access has
been granted.

## Prerequisites
  - None

## Gui
  - No gui

## Launch from Cmd Line

Host: ./privileges --help
Device: mldb launch com.magicleap.capi.sample.privileges

## What to Expect

  - Camera Capture privilege is requested first and blocks until the
    user grants or denies access.  After that other privileges are
    requested asynchronously and checked in the update loop.  When
    those privileges are checked the app logs a message and exits.

  - Also note, that when coming back from a pause, privileges should
    be checked and requested again if need be since the user can
    switch some off in the settings app during the pause of the app.
