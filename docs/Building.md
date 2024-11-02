# Building the N-API addons

The library uses pre-build addons only, no recompilation should be needed.

If you still want to compile yourself, for a modification or a fork, use these instructions.

## Windows x64

* Install Visual Studio 2022 build tools
* Use `x64 Native Tools Command Prompt for VS 2022` to ensure x64 VS2022 build tools are available in path
* In `addons`, run `npm install` and then `npm run build-windows-x64`

## macOS x64 and arm64

* In `addons`, run `npm install`, then: `npm run build-macos-x64` (x64) or `npm run build-macos-arm64` (arm64)

## Linux x64

* Ensure you have the ALSA header files installed globally. On Ubuntu you can use `sudo apt install libasound2-dev`.
* In `addons`, run `npm install` and then `npm run build-linux-x64`

## Linux arm64 (assuming cross-compiling from x64)

To successfully cross-compile for arm64:
* Install `g++-aarch64-linux-gnu` package
* Manually download a `libasound2-dev` package targeting arm64 ([example](https://launchpad.net/ubuntu/noble/arm64/libasound2-dev/1.2.11-1build2)) and extract the package locally to `~/arm64-libs` (that's the default location used in the build script - you'll need to edit the build script to change it)
* In `addons`, run `npm install` and then `npm run build-linux-arm64`
