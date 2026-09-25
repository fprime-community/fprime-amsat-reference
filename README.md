# Welcome to F Prime's Reference Repo for the AMSAT® CubeSat Simulator

<img width="300" alt="CubeSatSim v2" src="https://CubeSatSim.org/v2/cubesatsim%20v2%20complete.png">

This Git Repo contains the F' reference repository for the Command and Data Handling (CDH) Raspberry Pi Zero 2 board of the AMSAT® CubeSat Simulator.

## F' Framework Overview
F´ (F Prime) is an open-source, component-driven software framework developed by NASA’s Jet Propulsion Laboratory (JPL) for rapid development and deployment of embedded systems and spaceflight applications. It is designed to simplify the creation of flight-quality software, particularly for small-scale missions like CubeSats, SmallSats, instruments, and deployables, but it can be used for any embedded system.

## AMSAT® CubeSat Simulator Overview
The CubeSatSim(TM) is a low cost satellite emulator that runs on solar panels and batteries, transmits UHF radio telemetry, has a 3D printed frame, and can be extended by additional sensors and modules.  This project is sponsored by the not-for-profit [Radio Amateur Satellite Corporation, AMSAT®](https://amsat.org).

The [CubeSatSim kit](https://github.com/alanbjohnston/CubeSatSim/wiki/Kit) contains two processors:
- A **Raspberry Pi Zero 2** running Linux, with the Pi Camera attached.
- A **Raspberry Pi Pico W** mounted on the Main (STEM Payload) Board.

## AMSAT® CubeSat Simulator Hardware Block Diagram
![CubeSatSim Block Diagram](https://github.com/user-attachments/assets/a09086b9-2a05-4b4e-91a7-f8360718b6ce)

## AMSAT® CubeSat Simulator Deployments
There are two F' deployments for the AMSAT® CubeSat, each in its own Git Repo:

| Deployment | Processor | Repo |
|---|---|---|
| CDHDeployment | Raspberry Pi Zero 2 (Linux). Manages command and telemetry of the CubeSat and the Pi Camera. | This repo |
| MainSensorBoardDeployment | Raspberry Pi Pico W (baremetal) on the Main Board. | [fprime-amsat-main-board-reference](https://github.com/fprime-community/fprime-amsat-main-board-reference) |

This Git Repo contains the source code, CMake build files, and configuration files for the Raspberry Pi Zero 2 CDHDeployment only.

## Install F'
Below are the steps to install the F' Framework and clone this repo:
1. Install the F' [system requirements](https://fprime.jpl.nasa.gov/latest/docs/getting-started/installing-fprime/#system-requirements).
2. Install fprime-bootstrap: `pip install fprime-bootstrap`
3. Clone the project: `fprime-bootstrap clone https://github.com/fprime-community/fprime-amsat-reference.git`
4. `cd fprime-amsat-reference`
5. Activate the virtual environment: `. fprime-venv/bin/activate`

If you cloned with plain `git clone` instead of `fprime-bootstrap`, run `git submodule update --init --recursive` to fetch the F' framework into `lib/fprime`.

## Raspberry Pi ARM Cross-Compiler
The CDHDeployment is cross-compiled on a development computer for the Raspberry Pi Zero 2. Follow the [F´ Cross-Compilation Setup Tutorial](https://fprime.jpl.nasa.gov/latest/docs/tutorials/cross-compilation/) to install the ARM cross-compiler.

The toolchain must match the operating system installed on the Pi Zero 2. Run `getconf LONG_BIT` on the Pi:
- `64` — 64-bit Raspberry Pi OS: use the `aarch64-linux` toolchain.
- `32` — 32-bit Raspberry Pi OS: use the `arm-hf-linux` toolchain.

## Building the CDHDeployment
From the project virtual environment:
1. `cd CDHDeployment`
2. `fprime-util generate aarch64-linux -f` (`-f` deletes any previous `aarch64-linux` build directory)
3. `fprime-util build aarch64-linux`

Substitute `arm-hf-linux` for `aarch64-linux` if the Pi runs a 32-bit OS. The executable is written to `build-artifacts/aarch64-linux/CDHDeployment/bin/CDHDeployment` at the project root.

To build and run on the development computer instead (no Pi required), see [CDHDeployment/README.md](CDHDeployment/README.md).

## Loading and Running the CDHDeployment
TBD
