#!/bin/bash

# USBSoundCard Full Test Suite Runner
# This script builds and runs the complete test suite without baremetal dependencies
# Designed for Pi Zero (CDH deployment) testing

set -e

echo "=== USBSoundCard Full Test Suite ==="
echo "Building and running tests for Pi Zero deployment..."

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMPONENT_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_ROOT="$(cd "$COMPONENT_DIR/../.." && pwd)"

echo "Component directory: $COMPONENT_DIR"
echo "Project root: $PROJECT_ROOT"

# Change to project root
cd "$PROJECT_ROOT"

# Create a temporary project configuration for testing
echo "Creating test-specific project configuration..."
cp project-test.cmake project.cmake.backup

# Build the tests using the test configuration
echo "Building tests (excluding baremetal components)..."
fprime-util build --ut --jobs 1

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

# Run the simple test suite as well
echo "Running simple test suite..."
cd "$COMPONENT_DIR/test"
./run_simple_test.sh

# Restore original project configuration
cd "$PROJECT_ROOT"
mv project.cmake.backup project.cmake

echo "=== All tests completed! ===" 