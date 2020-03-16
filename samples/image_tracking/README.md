# Image Tracking Sample

This sample demonstrates the use of image recognition and tracking of two-dimensional, planar images.

## Prerequisites
- Print the images(DeepSeaExploration and DeepSpaceExploration) to Letter or A4-size paper.
- Have sufficient ambient lighting of at least a standard living room.

## Gui
 - Use the controller's bumper to show and hide the GUI.

## Launch from Cmd Line

Host: ./image_tracking
Device: mldb launch com.magicleap.capi.sample.image_tracking

## What to Expect

 - Loads all images under 'data/' and then the tracker will be looking
   for those images in the real world.  If the image is tracked there
   will be a green plane rendered on top of the image. Red is untracked
   and is rendered when the tracker loses tracking after it successfully
   had it before.

 - When running against MLRemote make sure the 'data' directory is in
   the same directory you run the app from.

## Privileges

- CameraCapture
