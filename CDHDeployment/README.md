# CDHDeployment Application

`CDHDeployment` is the F' deployment for the Raspberry Pi Zero 2 Command and Data Handling board of the AMSAT® CubeSat Simulator. To cross-compile it for the Pi Zero 2, see the [project README](../README.md#building-the-cdhdeployment).

The steps below build and run the deployment on a Linux or macOS development computer, which is useful for testing without the Pi.

## Building on the Development Computer

Generate a build directory, then build the deployment:

```
cd CDHDeployment
fprime-util generate
fprime-util build
```

## Running the Application and F' GDS

The following command starts the F' GDS, runs the application binary, and connects the two:

```
cd CDHDeployment
fprime-gds --framing-selection fprime --ip-client
```

> [!NOTE]
> Pass both options on every `fprime-gds` command:
> - `--framing-selection fprime`: CDHDeployment uses F' framing, and newer versions of the F' GDS default to CCSDS framing.
> - `--ip-client`: CDHDeployment listens for the GDS as a TCP server (`Drv.TcpServer`), so the GDS must connect as a client.

To run the ground system without starting the application:

```
cd CDHDeployment
fprime-gds --no-app --framing-selection fprime --ip-client
```

The application binary may then be run independently from the `bin` directory, starting from the project root:

```
cd build-artifacts/<platform>/CDHDeployment/bin/
./CDHDeployment -a 127.0.0.1 -p 50000
```

## Pi Camera

The `camera` instance (`PiCamera.CameraManager`, see [its design document](../Components/PiCamera/Components/CameraManager/docs/sdd.md)) captures still images by running `rpicam-still`. The camera must be enabled on the Pi; `rpicam-hello --list-cameras` should list it.

| Command | Description |
|---|---|
| `CDHDeployment.camera.CHECK_CAMERA` | Report whether a camera is connected |
| `CDHDeployment.camera.TAKE_PICTURE` | Capture a JPEG to `images/img_<seconds>_<count>.jpg`, relative to the application's working directory |

Image size and capture delay are set with the `IMAGE_WIDTH`, `IMAGE_HEIGHT` (default 320 × 256), and `CAPTURE_DELAY_MS` (default 1000) parameters. To downlink an image, send `CDHDeployment.fileDownlink.SendFile` with the path from the `PictureTaken` event. Command string arguments are limited to 40 characters (`FW_CMD_STRING_MAX_SIZE`), so use the relative path the event reports.

## Testing

Run these from the project root in the project virtual environment. [CI](../.github/workflows/ci.yml) runs all of them, plus an `aarch64-linux` cross-compile, on every push and pull request.

| Tests | What they cover | Command |
|---|---|---|
| Unit | `PiCamera::CameraManager` with fake camera commands | `cd Components/PiCamera/Components/CameraManager && fprime-util generate --ut && fprime-util check` |
| Host | Health ping table order, ID range, and rate group names, checked against the native build | `python -m pytest CDHDeployment/test/host` |
| Integration | The running deployment through the GDS: commands, each rate group, health pings, and the camera including image downlink | `CDHDeployment/test/int/run-integration-tests.sh` |

The host and integration tests need a native build first (`cd CDHDeployment && fprime-util generate && fprime-util build`). `run-integration-tests.sh` runs the deployment with the stand-in camera commands in [test/int/fake-camera](test/int/fake-camera), so no camera is needed. It takes about 2 minutes, and extra arguments are passed to pytest (for example `-v` or `-k camera`).
