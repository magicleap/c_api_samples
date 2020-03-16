# Raycast Sample

This sample renders a cube at the hitpoint of a raycast originating
from the controller.  A line is rendered from the controller to the
hit point.

## Prerequisites
  - None

## Gui
  - Show the gui by pressing the bumper button once on the
    controller.  Press it again to place the gui in the world.

  - Shows the details of the MLRaycastQuery and you can specify the
    number of horizontal rays and vertical rays that are cast.  The
    default is one for a single ray to be cast.

  - Shows the details of the MLRaycastRequest allowing you to toggle
    Auto Request which continuously casts rays and receives results.

  - Shows the MLRaycastResult which is the result of the raycast.

## Launch from Cmd Line

Host: ./raycast --help
Device: mldb launch com.magicleap.capi.sample.raycast

## What to Expect

 - By default the app casts a single ray from the controller onto the
   mesh.  At the hitpoint of the mesh a cube is rendered and a line is
   rendered from the controller to the hit point.
