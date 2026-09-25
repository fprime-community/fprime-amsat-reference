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
fprime-gds --framing-selection fprime
```

> [!NOTE]
> CDHDeployment uses F' framing. Newer versions of the F' GDS default to CCSDS framing, so pass `--framing-selection fprime` on every `fprime-gds` command.

To run the ground system without starting the application:

```
cd CDHDeployment
fprime-gds --no-app --framing-selection fprime
```

The application binary may then be run independently from the `bin` directory, starting from the project root:

```
cd build-artifacts/<platform>/CDHDeployment/bin/
./CDHDeployment -a 127.0.0.1 -p 50000
```
