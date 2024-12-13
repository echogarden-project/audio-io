import { AudioOutput } from './AudioIO.js'
import { getSineWave } from './AudioUtilities.js'
import { OpenPromise } from './OpenPromise.js'
import { decodeWaveToFloat32Channels, float32ChannelsToBuffer } from '@echogarden/wave-codec'

export async function playTestTone(userOptions?: TestToneOptions, positionCallback?: PositionCallback) {
	const options = { ...defaultTestToneOptions, ...userOptions } as Required<TestToneOptions>

	const testToneLeft = getSineWave(options.frequencyLeft, options.sampleRate * options.totalDuration, options.sampleRate)
	const testToneRight = getSineWave(options.frequencyRight, options.sampleRate * options.totalDuration, options.sampleRate)

	const testTone = [testToneLeft, testToneRight]

	await playFloat32Channels(testTone, options.sampleRate, { bufferDuration: options.bufferDuration }, positionCallback)
}

export async function playWaveData(
	waveData: Uint8Array,
	options?: PlaybackOptions,
	positionCallback?: PositionCallback): Promise<void> {

	const { audioChannels, sampleRate } = decodeWaveToFloat32Channels(waveData)

	return playFloat32Channels(audioChannels, sampleRate, options, positionCallback)
}

export async function playFloat32Channels(
	float32Channels: Float32Array[],
	sampleRate: number,
	options?: PlaybackOptions,
	positionCallback?: PositionCallback): Promise<void> {

	const channelCount = float32Channels.length

	const sampleBuffer = float32ChannelsToBuffer(float32Channels, 16)
	const int16Samples = new Int16Array(sampleBuffer.buffer, sampleBuffer.byteOffset, sampleBuffer.length / 2)

	return playInt16Samples(int16Samples, sampleRate, channelCount, options, positionCallback)
}

export async function playInt16Samples(
	int16Samples: Int16Array,
	sampleRate: number,
	channelCount: number,
	options?: PlaybackOptions,
	positionCallback?: PositionCallback): Promise<void> {

	options = { ...defaultPlaybackOptions, ...options }

	const openPromise = new OpenPromise()

	if (!int16Samples || !(int16Samples instanceof Int16Array)) {
		openPromise.reject(`pcmSamples were not provided or not an Int16Array`)

		return openPromise.promise
	}

	if (typeof sampleRate !== 'number') {
		openPromise.reject(`sampleRate was not provided or not a number`)

		return openPromise.promise
	}

	if (typeof channelCount !== 'number') {
		openPromise.reject(`channelCount was not provided or not a number`)

		return openPromise.promise
	}

	let ended = false
	let offset = 0

	let audioOutput: AudioOutput | undefined

	function audioOutputHandler(outputBuffer: Int16Array) {
		if (!audioOutput) {
			openPromise.reject(`No audio output object set`)

			return
		}

		if (ended) {
			return
		}

		const sampleCount = outputBuffer.length
		const samplesToOutput = int16Samples.subarray(offset, offset + sampleCount)

		outputBuffer.set(samplesToOutput)

		if (positionCallback) {
			positionCallback({
				sampleOffset: audioOutput.sampleOffset,
				timePosition: audioOutput.timePosition
			})
		}

		if (samplesToOutput.length < sampleCount) {
			ended = true

			audioOutput.dispose()
			openPromise.resolve()
		}

		offset += sampleCount
	}

	const AudioIO = await import('./AudioIO.js')

	if (!AudioIO.isPlatformSupported()) {
		openPromise.reject(`Platform is not supported`)
	}

	try {
		audioOutput = await AudioIO.createAudioOutput({
			sampleRate,
			channelCount,
			bufferDuration: options!.bufferDuration,
		}, audioOutputHandler)
	} catch (e) {
		openPromise.reject(e)
	}

	return openPromise.promise
}

export type PositionCallback = (playbackData: PositionCallbackData) => void

export type PositionCallbackData = {
	sampleOffset: number
	timePosition: number
}

const defaultBufferDuration = 100

export interface PlaybackOptions {
	bufferDuration?: number
}

const defaultPlaybackOptions: PlaybackOptions = {
	bufferDuration: defaultBufferDuration,
}

export interface TestToneOptions {
	totalDuration?: number
	sampleRate?: number
	bufferDuration?: number
	frequencyLeft?: number
	frequencyRight?: number
}

const defaultTestToneOptions: TestToneOptions = {
	totalDuration: 1,
	sampleRate: 48000,
	bufferDuration: defaultBufferDuration,
	frequencyLeft: 440,
	frequencyRight: 280,
}
