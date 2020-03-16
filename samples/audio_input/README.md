# Audio Input Test

This sample provides a gui that will allow users to record their voice and play
it back.

## Prerequisites
  - None

## Gui
  - Show the gui by pressing the bumper button once on the controller. Press it
  again to place the gui in the world.

  - The Recording section has buttons to start, stop, and clear the recording,
  as well as a toggle to mute the mic.

  - The Playback section has buttons to start, stop, pause, and resume the
  recorded audio

## Launch from Cmd Line

Host: ./audio_input --help
Device: mldb launch com.magicleap.capi.sample.audio_input

## What to Expect

- A user should be able to use the gui to record their voice and play it back.
