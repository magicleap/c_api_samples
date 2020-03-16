# Location Sample

This sample shows the usage of the Location API to get the lat/long of
where the device is located.  The functionality is on the imgui window
so be sure to click the bumper to show the imgui.

## Prerequisites
  - None

## Gui
  - imgui has buttons for getting the fine and coarse location of the device

## Launch from Cmd Line

Host: ./location --help
Device: mldb launch com.magicleap.capi.sample.location

## What to Expect

 - By default the app does not render anything.  Click the bumper to
   open the imgui window.  There you will see the buttons to retrieve
   fine and coarse locations.
