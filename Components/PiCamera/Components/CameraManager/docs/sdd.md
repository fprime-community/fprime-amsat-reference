# PiCamera::CameraManager

Component capturing still images from a Raspberry Pi camera. It runs the Raspberry Pi camera applications (`rpicam-still` and `rpicam-hello`, from the `rpicam-apps` package) as child processes, the same way the CubeSatSim software does, so no camera libraries are needed at build time. Images are written as JPEG files to an image directory, where they can be downlinked with `fileDownlink.SendFile`.

CameraManager is an active component. A capture blocks the component's thread until `rpicam-still` exits (about `CAPTURE_DELAY_MS` plus a fraction of a second), without blocking the command dispatcher or rate groups.

## Requirements

| Name | Description | Validation |
|---|---|---|
| PICAM-001 | The CameraManager shall capture a JPEG still image to a new file in the image directory on the TAKE_PICTURE command | Unit-Test, Integration |
| PICAM-002 | The CameraManager shall capture images at the width and height set by the IMAGE_WIDTH and IMAGE_HEIGHT parameters | Unit-Test |
| PICAM-003 | The CameraManager shall report a failed capture when the capture command exits with an error or produces no image | Unit-Test |
| PICAM-004 | The CameraManager shall report whether a camera is connected on the CHECK_CAMERA command | Unit-Test, Integration |
| PICAM-005 | The CameraManager shall report when a camera command cannot be started | Unit-Test |
| PICAM-006 | The CameraManager shall respond to health pings | Unit-Test |

## Configuration

The deployment calls `configure` during topology setup:

```cpp
camera.configure("images");  // image directory, created if it does not exist
```

`configure` optionally takes the capture and camera-listing commands, which default to `rpicam-still` and `rpicam-hello` and are found on the `PATH`. Raspberry Pi OS Bullseye names them `libcamera-still` and `libcamera-hello`:

```cpp
camera.configure("images", "libcamera-still", "libcamera-hello");
```

The camera must be enabled on the Pi. Run `rpicam-hello --list-cameras` on the Pi to confirm that it is detected.

## Port Descriptions

| Name | Description |
|---|---|
| pingIn | Health ping input, answered on pingOut |
| pingOut | Health ping output |
| timeCaller | Port for requesting current time, used for telemetry, events, and image file names |
| tlmOut | Port for sending telemetry channels to downlink |
| CmdDisp | Command receive port |
| CmdReg | Command registration port |
| CmdStatus | Command response port |
| Log | Event port |
| LogText | Text event port |
| PrmGet | Parameter get port |
| PrmSet | Parameter set port |

## Commands

| Name | Description |
|---|---|
| TAKE_PICTURE | Runs `rpicam-still -n -o <file> --width <IMAGE_WIDTH> --height <IMAGE_HEIGHT> -t <CAPTURE_DELAY_MS>`. Images are named `img_<seconds>_<count>.jpg`, where `<seconds>` is the capture time and `<count>` is the number of earlier capture attempts since startup. |
| CHECK_CAMERA | Runs `rpicam-hello --list-cameras` and reports whether a camera is listed. |

## Parameters

| Name | Description |
|---|---|
| IMAGE_WIDTH | Image width in pixels. Default: 320 |
| IMAGE_HEIGHT | Image height in pixels. Default: 256 |
| CAPTURE_DELAY_MS | Time in milliseconds `rpicam-still` runs before capturing, letting exposure settle. Default: 1000 |

The 320 × 256 default matches the image size the CubeSatSim software uses for SSTV.

## Events

| Name | Description |
|---|---|
| PictureTaken | An image was captured. Reports the file path and size. |
| CaptureFailed | `rpicam-still` exited with an error or wrote no image. Reports the path and exit status (-1 if it did not exit normally). Throttled to 5. |
| LaunchFailed | A camera command could not be started, for example because it is not installed. Reports the command and error number. Throttled to 5. |
| ImageDirectoryError | The image directory could not be created during `configure`. |
| CameraDetected | CHECK_CAMERA found a camera. |
| CameraNotDetected | CHECK_CAMERA found no camera, or `rpicam-hello` exited with an error. |

## Telemetry

| Name | Description |
|---|---|
| PicturesTaken | Number of images captured since startup |
| CaptureFailures | Number of failed capture attempts since startup |
| LastImageSize | Size in bytes of the most recent image |

## Unit Tests

The unit tests replace `rpicam-still` and `rpicam-hello` with shell scripts written to a scratch directory, so they run on a development computer without a camera.

| Name | Description | Requirements |
|---|---|---|
| ConfigureCreatesDirectory | `configure` creates the image directory | PICAM-001 |
| TakePicture | A capture passes the default parameters, reports the image, and gives consecutive images distinct names | PICAM-001 |
| TakePictureUsesParameters | Updated parameters are passed to `rpicam-still` | PICAM-002 |
| TakePictureCommandFails | A non-zero exit from `rpicam-still` is reported as a failed capture | PICAM-003 |
| TakePictureNoImage | A clean exit with no image written is reported as a failed capture | PICAM-003 |
| TakePictureLaunchFails | A missing `rpicam-still` is reported as a launch failure | PICAM-005 |
| CheckCameraDetected | A listed camera is reported as detected | PICAM-004 |
| CheckCameraNotDetected | "No cameras available!" is reported as no camera; a missing `rpicam-hello` is a launch failure | PICAM-004, PICAM-005 |
| Ping | Health pings are answered | PICAM-006 |
| ParameterCommands | Parameter set commands change the arguments passed to `rpicam-still` | PICAM-002 |
| CheckCameraLargeOutput | 100 KiB of `rpicam-hello` output is read without blocking, and the camera is still detected | PICAM-004 |
| ConfigureDirectoryError | An image directory that cannot be created is reported | PICAM-001 |
| TakePictureKilledBySignal | `rpicam-still` killed by a signal is reported as a failed capture with exit status -1 | PICAM-003 |
| CaptureFailedThrottle | `CaptureFailed` events stop after 5 while `CaptureFailures` telemetry counts every failure | PICAM-003 |
| CheckCameraExitError | `rpicam-hello` listing a camera but exiting with an error is reported as no camera | PICAM-004 |

Run them from this directory:

```sh
fprime-util generate --ut
fprime-util check
```
