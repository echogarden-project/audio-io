import { float32ToInt16Pcm, getSineWave, interleaveChannels } from './AudioUtilities.js'
import { OpenPromise } from './OpenPromise.js'

export async function playTestTone(totalDuration = 1, sampleRate = 48000, bufferDuration = 100) {
	const channelCount = 2 as number

	const testToneLeft = getSineWave(440, sampleRate * totalDuration, sampleRate)
	const testToneRight = getSineWave(280, sampleRate * totalDuration, sampleRate)
	const interleavedChannels = channelCount === 1 ? testToneLeft : interleaveChannels([testToneLeft, testToneRight])
	const pcmSamples = float32ToInt16Pcm(interleavedChannels)

	await playAudioSamples(pcmSamples, sampleRate, channelCount, bufferDuration)
}

export async function playAudioSamples(pcmSamples: Int16Array, sampleRate: number, channelCount: number, bufferDuration = 100) {
	const openPromise = new OpenPromise()

	if (!pcmSamples || !(pcmSamples instanceof Int16Array)) {
		throw new Error(`pcmSamples were not provided or not an Int16Array`)
	}

	if (typeof sampleRate !== 'number') {
		throw new Error(`sampleRate was not provided or not a number`)
	}

	if (typeof channelCount !== 'number') {
		throw new Error(`channelCount was not provided or not a number`)
	}

	let ended = false
	let offset = 0

	let disposeAudioOutput: Function

	function audioOutputHandler(outputBuffer: Int16Array) {
		if (ended) {
			return
		}

		const sampleCount = outputBuffer.length
		const samplesToOutput = pcmSamples.subarray(offset, offset + sampleCount)

		outputBuffer.set(samplesToOutput)

		if (samplesToOutput.length < sampleCount) {
			ended = true

			if (!disposeAudioOutput) {
				throw new Error(`No dispose method set`);
			}

			disposeAudioOutput()
			openPromise.resolve()
		}

		offset += sampleCount
	}

	const NodeAudioOutput = await import('./Exports.js')

	if (!NodeAudioOutput.isPlatformSupported()) {
		throw new Error(`Platform is not supported`)
	}

	try {
		const initResult = await NodeAudioOutput.createAudioOutput({
			sampleRate,
			channelCount,
			bufferDuration,
		}, audioOutputHandler)

		disposeAudioOutput = initResult.dispose
	} catch (e) {
		openPromise.reject(e)
	}

	return openPromise.promise
}
