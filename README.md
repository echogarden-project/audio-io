# Echogarden Audio I/O

A Node.js package that provides low-level audio outputs (**audio inputs are not implemented yet**) for common audio APIs, on various platforms:

* [MME (Multimedia Extensions)](https://en.wikipedia.org/wiki/Windows_legacy_audio_components) (Wave Out) on Windows (x64, arm64)
* [Core Audio](https://en.wikipedia.org/wiki/Core_Audio) (via low-latency [Audio Units](https://en.wikipedia.org/wiki/Audio_Units)) on macOS (x64, arm64)
* [ALSA (Advanced Linux Sound Architecture)](https://en.wikipedia.org/wiki/Advanced_Linux_Sound_Architecture) on Linux (x64, arm64)

The code is very minimalistic and doesn't rely on any external libraries, only direct system calls.

For each platform, there's a single, independent `cpp` source file, which uses the Node Addon API (C++) to produce a Node.js addon that integrates with the Node.js runtime for the given platform.

The addons are distributed as **precompiled binaries only**, which means the package doesn't require any build-time postprocessing to install.

## Installation

```sh
npm install @echogarden/audio-io
```

## Low-level audio output

This example creates a low-level audio output using `createAudioOutput`, and uses a callback to directly write into the audio device output buffer. It is useful where direct, real-time control is needed or low-latency audio processing is done.

If you're more interested in playing an existing audio file, or audio buffer, you can skip this, and move to the high-level operations section below.

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
    bufferDuration: 100.0, // Target audio output buffer duration, in milliseconds. Defaults to 100.0
}, audioOutputHandler)

// ...

// When done, call audioOutput.dispose(),
// which will close the audio output and free any associated memory or handles.
await audioOutput.dispose()
```
**Notes**:
* Only 16-bit, signed integer, little-endian, interleaved buffers are currently supported. Ensure the audio data is converted to this format before writing it to the handler's buffer

**Notes on `bufferDuration`**:
* On MME (Windows) and ALSA (Linux) `bufferDuration` will be used to directly compute the output buffer size
* On Core Audio (macOS), it will be used to set the maximum buffer size, but the actual buffer size selected by the driver may be significantly smaller

## High-level playback methods

These methods wrap around `createAudioOutput` and will internally create a new audio output, play the given audio data, and then dispose the audio output.

#### `playWaveData(waveData: Uint8Array, options?: PlaybackOptions, positionCallback?: PositionCallback)`

Play WAVE format bytes, given as a `Uint8Array`:

```ts
// Import modules
import { readFile } from 'fs/promises`'
import { playWaveData } from '@echogarden/audio-io'

const waveData: Uint8Array = await readFile('test.wav')

await playWaveData(waveData, { bufferDuration: 125 }) // 125ms buffer duration
```

`positionCallback` parameter accepts a callback that will be repeatedly called while the audio is being played, with sample-accurate playback position data. For example:

```ts
await playWaveData(waveData, {}, (callbackData) => {
    console.log(`Current audio time position: ${callbackData.timePosition}`)
    console.log(`Current sample offset: ${callbackData.sampleOffset}`)
})
```

`positionCallback` is also accepted by all following playback methods.

If you want to use the underlying WAVE encoder/decoder independently, you can import the `@echogarden/wave-codec` package directly into your project, see [its documentation](https://github.com/echogarden-project/wave-codec) for more details.

#### `playFloat32Channels(float32Channels: Float32Array[], sampleRate: number, options?: PlaybackOptions, positionCallback?: PositionCallback)`

Play floating point samples, given as an array of `Float32Array`, where each `Float32Array` is a separate channel:

```ts
// Import module
import { playFloat32Channels } from '@echogarden/audio-io'

const float32Channels: Float32Array[] = //... retrieve audio samples

await playFloat32Channels(float32Channels, 44100) // 44100 Hz, default buffer duration
```

#### `playInt16Samples(int16Samples: Int16Array, sampleRate: number, channelCount: number, options?: PlaybackOptions, positionCallback?: PositionCallback)`

Play signed, 16-bit interleaved, PCM audio samples, given as an `Int16Array`:

```ts
// Import module
import { playInt16Samples } from '@echogarden/audio-io'

const int16Samples: Int16Array = //... retrieve audio samples

await playInt16Samples(int16Samples, 44100, 2) // 44100 Hz, 2 channels, default buffer duration
```

#### `playTestTone(options?: TestToneOptions, positionCallback?: PositionCallback)`

Play a stereo test tone (sine wave), to test if the audio output is working correctly.

```ts
import { playTestTone } from '@echogarden/audio-io'

await playTestTone({
    totalDuration: 1,
	sampleRate: 48000,
	bufferDuration: 100,
	frequencyLeft: 440,
	frequencyRight: 280,
})
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
* Option to list and select audio output devices, and to use a playback device other than the default one
* Better handling of multichannel audio (more than 2 channels) by detecting output device properties. Currently it will error if default output device doesn't support the given number of channels
* Add optional lower latency I/O on Windows via the [WASAPI](https://en.wikipedia.org/wiki/Technical_features_new_to_Windows_Vista#Audio_stack_architecture) API (supported on Windows Vista or newer)

## License

MIT
