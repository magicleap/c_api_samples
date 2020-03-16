# Image Capture Sample

This sample captures image data from the RGB camera.  The command line
flags switch between a still raw image capture and a raw video capture.  Raw
video capture supports yuv format and raw image capture supports jpeg
and yuv formats.

## Prerequisites

## Gui
 - No gui

## Launch from Cmd Line

Host: ./image_capture --help
Device: mldb launch com.magicleap.capi.sample.image_capture

## What to Expect

 - By default this captures one frame in JPEG format.
 - Use -i "-video=1" to capture video frames in YUV format, default is 3 seconds of video.
 - Use -i "-video=0 -jpeg=0" to capture a still raw image in YUV format.
 - Get the files like so 'mldb pull -p <package_name_here> documents/C2/*'
