# Audio Playback
## Prerequisites
In order to play sounds other than the default sine wave, you must upload them to this test's package using `mldb push -p com.magicleap.capi.test.audio_playback filename path/in/the/package/`.

`path/in/the/package/` can be any path that mldb will let you push files to.

## Gui
 - There are 5 sections in the Gui:
    - CONTROL: provides controls for playing, pausing, and stopping audio
    - PARAMETERS: provides controls for audio pitch, volume, and other parameters
    - BUFFERING: provides feedback about how much of a buffer has been played and buffer latency
    - SPATIAL SOUND: provides controls for enabling and changing spatial sound parameters
    - MASTER VOLUME: provides feedback about the hardware's master volume

## Launch from Cmd Line

- Host: `./audio_playback`
- Device: `mldb launch -i "--filename=/documents/C2/sound.pcm" com.magicleap.capi.test.audio_playback`

## What to Expect

 - If `--filename` is not specified, a sine wave will be played by default.
 - Widgets in the GUI are nearly 1:1 with functions in the API, so interacting with each widget should do what the corresponding function's documentation says.
 - Use the `--help` argument for more details about what other command line arguments do
