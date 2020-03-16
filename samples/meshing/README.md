# Meshing Sample

This sample renders a wireframe mesh over the real world.  The blue
box is the bounding box, which is the area that will be meshed.
Anything outside that will not get mesh updates.

## Prerequisites
  - None

## Gui
  - Show the gui by pressing the bumper button once on the
    controller.  Press it again to place the gui in the world.

  - The bounding box, which is the area that will be meshed can be
    changed via the gui or by pressing the home button.  Pressing the
    home button cycles between following the user, a 3 meter box, and
    a 10 meter box.

  - Swiping up on the touchpad will change the LOD(Level of Detail) of
    the mesh returned.  The levels are Low, Medium, High.

  - Enabling Draw Block Bounds shows the bounding boxes of each mesh
    block.  The world is divided up into blocks that are then meshed
    based on the extents/bounding box that is given to the API.  Note
    that they overlap so as not to miss vertices and provide a more
    complete mesh.

## Launch from Cmd Line

Host: ./meshing --help
Device: mldb launch com.magicleap.capi.sample.meshing

## What to Expect

 - By default the app starts out rendering a mesh over the world in
   Medium level of detail.  The gui is hidden by default.
