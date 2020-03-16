# Controller Sample

This sample demonstrates how to get the state of the Controllers.
This includes getting the position and orientation of the controller
through the MLSnapshot system as well as the state of the controllers
buttons.

See the ml_input.h header for the MLInputControllerCallbacks, which
can also be used to get events on controller input.

## Prerequisites

## Gui
 - No gui

## Launch from Cmd Line

Host: ./controller --help
Device: mldb launch com.magicleap.capi.sample.controller

## What to Expect

 - A cube is rendered at the 6dof position of the controller. Also the
   controller information is shown as text next to the controller.

 - This sample is using the MLInput API to get the controller information.
