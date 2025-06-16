# Welcome to F Prime's Reference Repo for the AMSAT® CubeSat Simulator

<img width="300" alt="CubeSatSim v2" src="https://CubeSatSim.org/v2/cubesatsim%20v2%20complete.png">

This Git Repo contains the F' reference repository for the AMSAT® CubeSat Simulator.

## F' Framework Overview
F´ (F Prime) is an open-source, component-driven software framework developed by NASA’s Jet Propulsion Laboratory (JPL) for rapid development and deployment of embedded systems and spaceflight applications. It is designed to simplify the creation of flight-quality software, particularly for small-scale missions like CubeSats, SmallSats, instruments, and deployables, but it can be used for any embedded system.

## AMSAT® CubeSat Simulator Overview
The CubeSatSim(TM) is a low cost satellite emulator that runs on solar panels and batteries, transmits UHF radio telemetry, has a 3D printed frame, and can be extended by additional sensors and modules.  This project is sponsored by the not-for-profit [Radio Amateur Satellite Corporation, AMSAT®](https://amsat.org).

## AMSAT® CubeSat Simulator Hardware Block Diagram
![CubeSatSim Block Diagram](https://github.com/user-attachments/assets/a09086b9-2a05-4b4e-91a7-f8360718b6ce)

## Building and Running the F' Deployments for the AMSAT® CubeSat Simulator
There are two F' deployments for the AMSAT® CubeSat. 
1. CHDDeployment - Executes on the Raspberry Pi Zero that manages command and telemetry of the AMSAT® CubeSat. This board also manages the PI Camera.
2. MainDeployment - Executes on the Raspberry Pi Pico mounted on the AMSAT® CubeSat Main Board.

### Raspberry PI ARM Cross-Compiler
Below is a link to the setup tutorial to install the Raspberry PI ARM Cross-Compiler:
[F´ Cross-Compilation Setup Tutorial](https://fprime.jpl.nasa.gov/latest/docs/tutorials/cross-compilation/)

### Cloning the Repository  
1. git clone https://github.com/fprime-community/fprime-amsat-reference.git
2. git submodule update --init --recursive

### Start the Python Virtual Environment
The python virtual environment is required to build the F' deployments.
1. cd fprime-amsat-reference
2. . fprime-venv/bin/activate

### Building CHDDeployment
1. cd CHDDeployment
2. fprime-util generate
3. fprime-util build

### Building MainDeployment
1. cd MainDeployment
2. fprime-util generate
3. fprime-util build

## Installing F' FSW on the AMSAT® CubeSat Simulator on Raspberry Pi Zero and Pico Boards
TBD
