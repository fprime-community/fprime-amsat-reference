# Welcome to F Prime's Reference Repo for the AMSAT® CubeSat Simulator

<img width="300" alt="CubeSatSim v2" src="https://CubeSatSim.org/v2/cubesatsim%20v2%20complete.png">

This Git Repo contains the F' reference repository for the AMSAT® CubeSat Simulator.

## F' Framework Overview
F´ (F Prime) is an open-source, component-driven software framework developed by NASA’s Jet Propulsion Laboratory (JPL) for rapid development and deployment of embedded systems and spaceflight applications. It is designed to simplify the creation of flight-quality software, particularly for small-scale missions like CubeSats, SmallSats, instruments, and deployables, but it can be used for any embedded system.

## AMSAT® CubeSat Simulator Overview
The CubeSatSim(TM) is a low cost satellite emulator that runs on solar panels and batteries, transmits UHF radio telemetry, has a 3D printed frame, and can be extended by additional sensors and modules.  This project is sponsored by the not-for-profit [Radio Amateur Satellite Corporation, AMSAT®](https://amsat.org).

## AMSAT® CubeSat Simulator Hardware Block Diagram
![CubeSatSim Block Diagram](https://github.com/user-attachments/assets/a09086b9-2a05-4b4e-91a7-f8360718b6ce)

## AMSAT® CubeSat Simulator Deployments
There are two F' deployments for the AMSAT® CubeSat. 
1. CDHDeployment - Executes on the Raspberry Pi Zero that manages command and telemetry of the AMSAT® CubeSat. This board also manages the PI Camera.
2. MainDeployment - Executes on the Raspberry Pi Pico mounted on the AMSAT® CubeSat Main Board.

## Raspberry PI ARM Cross-Compiler
Below is a link to the setup tutorial to install the Raspberry PI ARM Cross-Compiler:
[F´ Cross-Compilation Setup Tutorial](https://fprime.jpl.nasa.gov/latest/docs/tutorials/cross-compilation/)

## Getting Started  
Below are the steps to install the F' Framework and clone the F' AMSAT® CubeSat Repo:
1. Install the F' [system requirements](https://fprime.jpl.nasa.gov/latest/docs/getting-started/installing-fprime/#system-requirements).
2. Install fprime-bootstrap `pip install fprime-bootstrap`
3. Run `fprime-bootstrap clone https://github.com/fprime-community/fprime-amsat-reference.git`
4. cd fprime-amsat-reference
5. . fprime-venv/bin/activate

## Building the AMSAT® CubeSat F' Executables
The AMSAT® CubeSat constains two Raspberry PI's. A Raspberry PI Pico is mounted on the main processing board and interfaces with the Gyro's, Temperature, Pressure Sensors and the FM Transceiver Board. The Raspberry PI Zero interfaces with its own temperature sensor and processes the commands and telemetry and interfaces. The Raspberry PI camera is connected to the Raspberry PI Zero. 

The Raspberry PI Pico and the Raspberry PI Zero execute two different Operating Systems (OS) environments. Thus, the build steps for each Raspberry PI are slightly different and use different libraries.  

### Building CDHDeployment
The CDHDeployment executes on the Raspberry PI Zero executing Linux OS. In order to properly build, the Raspberry PI ARM Cross-Compiler must be installed on the development computer:
1. cd CDHDeployment
2. fprime-util generate aarch64-linux -f (Note: -f will delete any existing previous aarch64-linux build)
3. fprime-util build aarch64-linux 

### Building MainDeployment
The MainDeployment executes on the Raspberry PI Pico. The Raspberry Pi Pico doesn't run a full operating system like Raspberry Pi Zero. Instead, it's typically programmed directly with code, similar to an Arduino.

1. cd MainDeployment
2. fprime-util generate
3. fprime-util build


## Loading and running the AMSAT® CubeSat F' Executables
TBD

