# Eye Tracking Sample

## Prerequisites

## Gui
 - no gui

## Launch from Cmd Line

Host: ./eye_tracking --help
Device: mldb launch com.magicleap.capi.sample.eye_tracking

## What to Expect

 - Draws a cube at the fixation point (where you are looking).

 - Draws cubes for the eye centers. Note this is in world space and
   will be at the center of your eyes when wearing the headset so you
   won't be able to see these.  These can be seen in MLRemote's eye
   view.  Search for "eye_centers_origin" in the code to get an
   explanation of this flag.

