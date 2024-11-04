# Echogarden Audio I/O

A Node.js package that provides low-level audio outputs (**audio inputs are not implemented yet**) for common audio APIs, on various platforms:

* [MME (Multimedia Extensions)](https://en.wikipedia.org/wiki/Windows_legacy_audio_components) (Wave Out) on Windows x64 and arm64
* [Core Audio](https://en.wikipedia.org/wiki/Core_Audio) (via low-latency [Audio Units](https://en.wikipedia.org/wiki/Audio_Units)) on macOS for both Intel x64 and Apple Silicon (arm64)
* [ALSA (Advanced Linux Sound Architecture)](https://en.wikipedia.org/wiki/Advanced_Linux_Sound_Architecture) on Linux for both x64 and arm64

The code is very minimalistic and doesn't rely on any external libraries, only direct system calls.

For each platform, there's a single, independent `cpp` source file, which uses the Node Addon API (C++) to produce a Node.js addon that integrates with the Node.js runtime for the given platform.

The addons are distributed as **precompiled binaries only**, which means the package doesn't require any build-time postprocessing to install.

## Installation

```sh
npm install @echogarden/audio-io
```

## Usage example

Notes:
* Only audio outputs are supported at this time. Audio inputs will be added in the future
* Only 16-bit, signed integer, little-endian, interleaved buffers are currently supported. Ensure the audio data is converted to this format before writing it to the handler's buffer

```ts
// Import module
import { createAudioOutput } from '@echogarden/audio-io'

// Define an audio output handler function
function audioOutputHandler(outputBuffer: Int16Array) {
    // Write 16-bit signed integer little-endian samples to `outputBuffer`.
    // If there are multiple channels, interleave them, like `LRLRLRLR..` for stereo.
}

// Create audio output, passing a configuration object and the handler
const audioOutput = await createAudioOutput({
    sampleRate: 44100, // Sample rate in Hz, should be an integer like 44100, 22050, 8000
    channelCount: 2, // Channel count, likely 1 (mono), or 2 (stereo)
    bufferDuration: 100.0, // Target buffer duration, in milliseconds. Defaults to 100.0
}, audioOutputHandler)

// ...

// When done, call audioOutput.dispose(),
// which will close the audio output and free any associated memory or handles.
await audioOutput.dispose()
```

**Notes on `bufferDuration`**:
* On MME (Windows) and ALSA (Linux) `bufferDuration` will be used to directly compute the buffer size
* On Core Audio (macOS), it will be used to set the maximum buffer size, but the actual buffer size selected by the driver may be significantly smaller

## Utility methods

Play signed, 16-bit interleaved, PCM audio samples, given as an `Int16Array`:

```ts
// Import module
import { playAudioSamples } from '@echogarden/audio-io'

const pcmAudioSamples = //... retrieve audio samples

await playAudioSamples(pcmAudioSamples, 44100, 2, 150) // 44100 Hz, 2 channels, 150ms buffer duration,
```

Play a stereo test tone (sine wave), to test the audio output:

```ts
// Import module
import { playTestTone } from '@echogarden/audio-io'

await playTestTone(1, 48000, 100) // 1 second, 48000 Hz, 100ms buffer duration.
```

## Building the addons

Pre-built addons are bundled for all supported platforms.

To rebuild them yourself, see [guide for building the addons](docs/Building.md).

## Still experimental, feedback is needed

All code was written from scratch, meaning it hasn't been tested on a large variety of systems yet:

* Windows addon has only been tested on Windows 11
* Linux addon has only been tested in WSL2 (Ubuntu 24.04) and an Ubuntu 23.10 VM in VirtualBox (x64 only)
* macOS addon has only been tested in a macOS 13 VM in VMWare (x64 only)

If you encounter any crashes, unexpected errors, or audio problems, like distortions or artifacts, it's likely they can be solved relatively easily. Just open an issue and let me know about it.

## Future

* **Audio inputs** would be implemented once audio outputs are sufficiently stabilized and tested
* Add option to select an audio output device other than the default one
* Add optional lower latency I/O on Windows via the [WASAPI](https://en.wikipedia.org/wiki/Technical_features_new_to_Windows_Vista#Audio_stack_architecture) API (supported on Windows Vista or newer)

## License

MIT
