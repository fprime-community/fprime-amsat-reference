#!/bin/bash

# USBSoundCard Integration Test Runner
# This script compiles and runs the integration test separately

set -e

echo "=== USBSoundCard Integration Test ==="

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMPONENT_DIR="$(dirname "$SCRIPT_DIR")"

echo "Component directory: $COMPONENT_DIR"

# Change to component directory
cd "$COMPONENT_DIR"

# Compile the integration test
echo "Compiling integration test..."
g++ -std=c++11 \
    -I. \
    -I../../lib/fprime \
    -I../../lib/fprime/Fw/Types \
    -I../../lib/fprime/Fw/Buffer \
    -I../../lib/fprime/Fw/Obj \
    -I../../lib/fprime/Fw/Comp \
    -I../../lib/fprime/Fw/Log \
    -I../../lib/fprime/Fw/Time \
    -I../../lib/fprime/Fw/Cmd \
    -I../../lib/fprime/Fw/Tlm \
    -I../../lib/fprime/Fw/Com \
    -I../../lib/fprime/Fw/Prm \
    -I../../lib/fprime/Fw/Port \
    -I../../lib/fprime/Fw/Interfaces \
    -I../../lib/fprime/Os \
    -I../../lib/fprime/Svc \
    -I../../lib/fprime/Drv \
    -I../../lib/fprime/Utils \
    -I../../lib/fprime/STest \
    -I../../lib/fprime/googletest/googletest/include \
    -I../../build-fprime-automatic-native-ut \
    -I../../build-fprime-automatic-native-ut/F-Prime \
    -I../../build-fprime-automatic-native-ut/Components/USBSoundCard \
    -DASSERT_FILE_ID=0x12345678 \
    -DPROTECTED=public \
    -DPRIVATE=public \
    -DSTATIC= \
    -DBUILD_UT \
    -o integration_test \
    test/ut/integration/IntegrationTest.cpp \
    test/ut/PacketReceiver.cpp \
    -lm

# Run the integration test
echo "Running integration test..."
./integration_test

# Clean up
rm -f integration_test

echo "Integration test completed!" 