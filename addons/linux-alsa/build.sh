#!/bin/bash

# Define the input and output file names
INPUT_FILE="linux-alsa-output.cpp"
OUTPUT_FILE_X86_64="linux-x64-alsa-output.node"
OUTPUT_FILE_ARM64="linux-arm64-alsa-output.node"

# Define the common compiler flags
COMMON_FLAGS="-I../include/napi -shared -fPIC -O0 -lm -std=c++17"

# Check for optional TRACE flag
if [ "$1" == "enable-trace" ]; then
  COMMON_FLAGS="$COMMON_FLAGS -D TRACE"
fi

# Compile for x86_64 architecture
echo "Compiling for x86_64 architecture..."
g++ $INPUT_FILE -o $OUTPUT_FILE_X86_64 $COMMON_FLAGS -lasound

# Compile for ARM64 architecture
echo "Compiling for ARM64 architecture..."
aarch64-linux-gnu-g++ $INPUT_FILE -o $OUTPUT_FILE_ARM64 -I ~/arm64-libs/usr/include -L ~/arm64-libs/usr/lib/aarch64-linux-gnu $COMMON_FLAGS

echo "Compilation complete."
