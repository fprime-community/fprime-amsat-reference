#!/bin/bash

# Simple test runner for USBSoundCard transmission functionality
# This script compiles and runs a standalone test that verifies the packet transmission
# functionality without requiring the full FPrime build system.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Running USBSoundCard transmission tests from: $SCRIPT_DIR"

# Compile the test
echo "Compiling simple test..."
g++ -std=c++11 -o simple_test simple_test.cpp -lm

# Run the test
echo "Running simple test..."
./simple_test

echo "Test completed successfully!" 