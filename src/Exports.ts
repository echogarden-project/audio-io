import { createRequire } from 'node:module'

export { playAudioSamples, playTestTone } from './Player.js'

let audioOutputModule: AudioOutputAddon | undefined

export async function createAudioOutput(config: AudioOutputConfig, handler: AudioOutputHandler) {
	if (typeof config !== 'object') {
		throw new Error(`No valid configuration object provided`)
	}

	if (typeof handler !== 'function') {
		throw new Error(`No valid handler function provided`)
	}

	config = { ...config }

	const module = await getAudioOutputModuleForCurrentPlatform()

	const sampleRate = config.sampleRate

	if (typeof sampleRate !== 'number' || Math.floor(sampleRate) !== sampleRate || sampleRate < 1) {
		throw new Error(`Sample rate ${sampleRate} is invalid. It must be a positive integer greater than 0`)
	}

	const channelCount = config.channelCount

	if (typeof channelCount !== 'number' || Math.floor(channelCount) !== channelCount || channelCount < 1) {
		throw new Error(`Channel count of ${channelCount} is invalid. It must be a positive integer greater than 0`)
	}

	const defaultBufferDuration = 100

	if (config.bufferDuration == null) {
		config.bufferDuration = defaultBufferDuration
	}

	let bufferDuration = config.bufferDuration

	if (bufferDuration == null) {
		config.bufferDuration = defaultBufferDuration
	} else if (typeof bufferDuration !== 'number' || bufferDuration <= 0) {
		throw new Error(`Buffer duration of ${bufferDuration} is invalid. It must be a floating point value greater than 0 (representing milliseconds)`)
	}

	if (typeof handler !== 'function') {
		throw new Error(`Handler is not a function`)
	}

	let wrappedHandler: AudioOutputHandler

	{
		let sampleOffset = 0

	 	wrappedHandler = (audioBuffer: Int16Array) => {
			const timeOffset = sampleOffset / sampleRate / channelCount

			handler(audioBuffer)

			sampleOffset += audioBuffer.length
		}
	}

	const nativeResult = await module.createAudioOutput(config, wrappedHandler)

	const nativeDisposeMethod = nativeResult.dispose

	let isDisposed = false

	const wrappedDisposeMethod = () => {
		return new Promise<void>((resolve, reject) => {
			if (isDisposed) {
				resolve()

				return
			}

			process.nextTick(() => {
				try {
					nativeDisposeMethod()
					resolve()
				} catch(e) {
					reject(e)
				}

				isDisposed = true
			})
		})
	}

	const wrappedResult: AudioOutput = {
		dispose: wrappedDisposeMethod
	}

	return wrappedResult
}

async function getAudioOutputModuleForCurrentPlatform() {
	if (audioOutputModule) {
		return audioOutputModule
	}

	const platform = process.platform
	const arch = process.arch

	if (platform === 'win32' && arch === 'x64') {
		audioOutputModule = createRequire(import.meta.url)('../addons/bin/windows-x64-mme-output.node')
	} else if (platform === 'darwin' && arch === 'x64') {
		audioOutputModule = createRequire(import.meta.url)('../addons/bin/macos-x64-coreaudio-output.node')
	} else if (platform === 'darwin' && arch === 'arm64') {
		audioOutputModule = createRequire(import.meta.url)('../addons/bin/macos-arm64-coreaudio-output.node')
	} else if (platform === 'linux' && arch === 'x64') {
		audioOutputModule = createRequire(import.meta.url)('../addons/bin/linux-x64-alsa-output.node')
	} else if (platform === 'linux' && arch === 'arm64') {
		audioOutputModule = createRequire(import.meta.url)('../addons/bin/linux-arm64-alsa-output.node')
	} else {
		throw new Error(`audio-io initialization error: platform ${platform}, ${arch} is not supported`);
	}

	return audioOutputModule!
}

export function isPlatformSupported() {
	const platform = process.platform
	const arch = process.arch

	if (platform === 'win32' && arch === 'x64') {
		return true
	}

	if (platform === 'darwin' && (arch === 'x64' || arch === 'arm64')) {
		return true
	}

	if (platform === 'linux' && (arch === 'x64' || arch === 'arm64')) {
		return true
	}

	return false
}

export interface AudioOutput {
	dispose(): Promise<void>
}

export type AudioOutputHandler = (outputBuffer: Int16Array) => void

export interface AudioOutputConfig {
	sampleRate: number
	channelCount: number
	bufferDuration?: number
}

interface AudioOutputAddon {
	createAudioOutput(config: AudioOutputConfig, handler: AudioOutputHandler): Promise<NativeAudioOutput>
}

interface NativeAudioOutput {
	dispose(): void
}
