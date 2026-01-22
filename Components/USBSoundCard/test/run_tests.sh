#!/bin/bash

# USBSoundCard Test Runner Script
# This script builds and runs the USB sound card component tests

set -e

echo "=== USBSoundCard Component Tests ==="
echo "Building and running tests..."

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMPONENT_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_ROOT="$(cd "$COMPONENT_DIR/../.." && pwd)"

echo "Component directory: $COMPONENT_DIR"
echo "Project root: $PROJECT_ROOT"

# Change to project root
cd "$PROJECT_ROOT"

# Build the tests
echo "Building tests..."
fprime-util build --ut

# Run the unit tests
echo "Running unit tests..."
fprime-util check --ut

# Run the integration tests specifically
echo "Running integration tests..."
if [ -f "build-fprime-automatic-native-ut/bin/Components_USBSoundCard_IntegrationTest" ]; then
    echo "Running USBSoundCard Integration Test..."
    ./build-fprime-automatic-native-ut/bin/Components_USBSoundCard_IntegrationTest
else
    echo "Integration test executable not found. Building may have failed."
    exit 1
fi

echo "=== All tests completed! ===" 