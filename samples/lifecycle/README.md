# Lifecycle Sample

This sample demonstrates the lifecycle API.  This API has information
such as where an app's sandbox location is in the filesystem and a way
to grab any command line args that are passed into the app. Some of
the API is also demonstrated in simple_gl_app including the callbacks
for when the app is paused, resumed, etc.

## Prerequisites
  - None

## Gui
  - None

## Launch from Cmd Line

Host: ./lifecycle --help
Device: mldb launch com.magicleap.capi.sample.lifecycle

## What to Expect

 - The app will print out the lifecycle info to the mldb log and every
   two seconds write out the headpose position to a file
   (headpose.txt) to the app's sandbox location in the filesystem.
