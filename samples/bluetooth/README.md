# Bluetooth Sample

This sample uses the Bluetooth API to scan for BLE (Bluetooth Low
Energy) devices.  It shows these devices in an imgui you can access by
clicking the bumper.  It shows the name, address and rssi of a BLE
device.  Highlighting a BLE device and clicking connect will bond to
the device and print out it's Gatt services and characteristics.

## Prerequisites
  - None

## Gui
  - Show the gui by pressing the bumper button once on the
    controller.  Press it again to place the gui in the world.

## Launch from Cmd Line

Host: ./bluetooth --help
Device: mldb launch com.magicleap.capi.sample.bluetooth

## What to Expect

 - By default the app starts out scanning for Bluetooth Low Energy
   devices and when found adds them to the list in the gui.
