# Hand Tracking Sample

This sample renders cubes at the tracked hand keypoints.  Once the
hand is recognized by the sensors the keypoints will be rendered.
Text is displayed at the hand center showing the current pose.

## Prerequisites

## Gui
 - Click bumper to show the gui

## Launch from Cmd Line

Host: ./hand_tracking --help
Device: mldb launch com.magicleap.capi.sample.hand_tracking hand_tracking

## What to Expect

### GUI Component

* Draws a cube at each of the keypoints when a pose is detected.

### Command Line Component

* Displays text describing hand keyposes and keypoints.

