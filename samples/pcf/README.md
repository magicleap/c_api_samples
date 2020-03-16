# PCF Sample

This sample demonstrates how to use Persistence Coordinate Frames
(PCF) which can be used to persist content in the world between device
reboots.

A Persistent Coordinate Frame (PCF) is a local coordinate
frame that allows you to place content in the physical world and have
it stay in the same place without any drift across multiple user
sessions.

## Prerequisites

- Scan your area when promted to do so at the home screen so the
  device can recognize your area.

## Gui

- No gui

## Launch from Cmd Line

Host: ./pcf --help

Device: mldb launch com.magicleap.capi.sample.pcf

## What to Expect

- As long as the ML1 recognizes the space as being seen before, PCFs
  will show up as cubes with their UUID identifiers printed at the
  center.

- The closest PCF will be outlined with a blue cube.

## Privileges

- PwFoundObjRead

## More Info

[Persistent Coordinate Frames Guide](https://creator.magicleap.com/learn/guides/persistent-coordinate-frames)
