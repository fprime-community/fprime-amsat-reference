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

## Loading Flight Software onto an Assembled CubeSatSim
[scripts/deploy-fsw.sh](scripts/deploy-fsw.sh) loads both deployments from your development computer after they are built:
- **CDHDeployment** onto the Pi Zero 2, over the network with SSH
- **MainSensorBoardDeployment** onto the Pico W, over a USB cable from your computer to the Pico W. The Pi Zero cannot program the Pico W, because only a UART connects them.

### First-time setup
1. Check out [fprime-amsat-main-board-reference](https://github.com/fprime-community/fprime-amsat-main-board-reference) next to this repo, and build it (see its README).
2. Give your computer SSH key access to the Pi. The Pi's user needs `sudo`, which the default `pi` user has:
   ```
   ssh-copy-id pi@cubesatsim.local
   ```
3. Build CDHDeployment for `aarch64-linux` with a cross-compiler whose glibc is no newer than the Pi's. The Arm GNU Toolchain 10.2 (`aarch64-none-linux-gnu`) works with every current Raspberry Pi OS. Newer distribution cross-compilers, such as Ubuntu 24.04's, produce binaries that Raspberry Pi OS Bookworm cannot run. The script checks this before copying.

### Loading
```
scripts/deploy-fsw.sh                      # load both boards
scripts/deploy-fsw.sh --build --verify     # build both, load both, then test both through the GDS
scripts/deploy-fsw.sh --only pi            # or --only pico
scripts/deploy-fsw.sh --dry-run            # run the checks and show what would change
scripts/deploy-fsw.sh --pi pi@192.168.1.50 --pico-port /dev/ttyACM1
```
The Pico W is found automatically by its USB name. If its firmware isn't running, the script asks you to hold BOOTSEL and reconnect it. Run `scripts/deploy-fsw.sh --help` for all options.

### What changes on the Pi
- Each load creates a release in `~/fprime/releases/`, and `~/fprime/current` points to the newest. The newest 3 releases are kept.
- The `fprime-cdh` systemd service runs `current/CDHDeployment -a 0.0.0.0 -p 50000` from `~/fprime/data`, where camera images and `PrmDb.dat` persist between releases.
- On the first load, the stock CubeSatSim services that use the UART, radio, audio, or camera (`cubesatsim`, `transmit`, `command`, `pacsatsim`, `frequency`) are stopped and masked. The push button and OliveTin web UI keep running. `scripts/deploy-fsw.sh --restore-stock` stops F' and brings the stock services back.
- To roll back, on the Pi: `cd ~/fprime && ln -sfn releases/<older release> current && sudo systemctl restart fprime-cdh`

### Connecting the GDS
```
fprime-gds -n --dictionary build-artifacts/aarch64-linux/CDHDeployment/dict/CDHDeploymentTopologyDictionary.json \
    --framing-selection fprime --ip-client --ip-address cubesatsim.local --ip-port 50000
```

> [!NOTE]
> `systemctl stop fprime-cdh` sends SIGTERM, and CDHDeployment exits within a few seconds whether or not a GDS is connected (see [AmsatDrv::TcpServer](Components/AmsatDrv/TcpServer/docs/sdd.md)). As a safeguard, the service sets `TimeoutStopSec=10`, so systemd kills it after 10 s instead of the default 90 s if it ever does not exit.
