#!/bin/bash

# Define the input and output file names
INPUT_FILE="macos-coreaudio-output.cpp"
OUTPUT_FILE_X86_64="macos-x64-coreaudio-output.node"
OUTPUT_FILE_ARM64="macos-arm64-coreaudio-output.node"

# Define the common compiler flags
COMMON_FLAGS="-I../include/napi -shared -O0 -framework AudioUnit -fPIC -undefined dynamic_lookup -std=c++17"

# Check for optional TRACE flag
if [ "$1" == "enable-trace" ]; then
  COMMON_FLAGS="$COMMON_FLAGS -D TRACE"
fi

# Compile for x86_64 architecture
echo "Compiling for x86_64 architecture..."
clang++ $INPUT_FILE -o $OUTPUT_FILE_X86_64 $COMMON_FLAGS -arch x86_64

# Compile for arm64 architecture
echo "Compiling for arm64 architecture..."
clang++ $INPUT_FILE -o $OUTPUT_FILE_ARM64 $COMMON_FLAGS -arch arm64

echo "Compilation complete."
