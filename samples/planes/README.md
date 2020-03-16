# Planes Sample

This sample renders planes that are semantically tagged as floors.  It
shows a pointer coming from the controller.  When you point at a plane
and pull the trigger a sphere will be placed at the middle of the
plane.

## Prerequisites
  - None

## Gui
  - None

## Launch from Cmd Line

Host: ./planes --help
Device: mldb launch com.magicleap.capi.sample.planes

Turn rendering of boundaries on like so.  This will show the boundaries of the planes.
Device: mldb launch -i "-RenderBoundaries=1" com.magicleap.capi.sample.planes

## What to Expect

 - By default the app shows planes that are semantically tagged as
   floors.  If the area is sufficiently mapped, you should see floors
   colored green.  With the controller you can pull the trigger
   placing a sphere in the middle of the plane being pointed at by the
   controller.
