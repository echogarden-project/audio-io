# Building the N-API addons

The library uses pre-build addons only, no recompilation should be needed.

If you still want to compile yourself, for a modification or a fork, use these instructions.

## Windows x64

* Install Visual Studio 2022 build tools
* Install [`msys2`](https://www.msys2.org/) and its x64 `binutils` package (`pacman -S mingw-w64-x86_64-binutils`)
* Ensure you have `C:\msys64\mingw64\bin` in path
* Use `x64 Native Tools Command Prompt for VS 2022` to ensure x64 VS2022 build tools are available in path
* In `addons/windows-mme`, run `build.cmd`. It will build a `.node` addon using `cl.exe` (MSVC), as well as its `node_api.lib` dependency using `dlltool`

## macOS x64 and ARM64

`addons/macos-coreaudio` directory includes a `build.sh` file that will build `.node` addons using `clang++` on macOS, for both `x86-64` and `arm64` targets.

## Linux x64 and ARM64

* Ensure you have the ALSA header files installed globally. On Ubuntu you can use `sudo apt install libasound2-dev`.
* In `addons/linux-alsa`, run `build.sh`. It will build `.node` addons (for `x64` and `arm64`) using `g++`

To successfully cross-compile for ARM64:
* Install `g++-aarch64-linux-gnu` package
* Manually download a `libasound2-dev` package targeting ARM64 ([example](https://launchpad.net/ubuntu/noble/arm64/libasound2-dev/1.2.11-1build2)) and extract the package locally to `~/arm64-libs` (that's the default location used in the build script - you'll need to edit the build script to change it)
