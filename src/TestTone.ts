import { float32ToInt16Pcm, getSineWave, interleaveChannels } from './AudioUtilities.js'
import { OpenPromise } from './OpenPromise.js'

export async function playTestTone(totalDuration = 1, bufferDuration = 100, sampleRate = 48000, trace = false) {
	const openPromise = new OpenPromise()

	let log = (arg: any) => {}

	if (trace) {
		log = console.log
	}

	const channelCount = 2 as number

	const testToneLeft = getSineWave(440, sampleRate * totalDuration, sampleRate)
	const testToneRight = getSineWave(280, sampleRate * totalDuration, sampleRate)
	const interleavedChannels = channelCount === 1 ? testToneLeft : interleaveChannels([testToneLeft, testToneRight])
	const pcmSamples = float32ToInt16Pcm(interleavedChannels)

	let offset = 0
	let ended = false

	let disposeAudioOutput: Function

	function audioOutputCallback(outputBuffer: Int16Array) {
		if (ended) {
			return
		}

		const sampleCount = outputBuffer.length
		const samplesToOutput = pcmSamples.subarray(offset, offset + sampleCount)

		outputBuffer.set(samplesToOutput)

		if (samplesToOutput.length < sampleCount) {
			ended = true

			log(`JavaScript: output ended`)

			if (!disposeAudioOutput) {
				throw new Error(`No dispose method set`);
			}

			disposeAudioOutput()
			openPromise.resolve()
		}

		offset += sampleCount
	}

	log("JavaScript: initializing")
	const NodeAudioOutput = await import('./Exports.js')

	if (!NodeAudioOutput.isPlatformSupported()) {
		throw new Error(`Platform is not supported`)
	}

	try {
		const initResult = await NodeAudioOutput.createAudioOutput({
			sampleRate,
			channelCount,
			bufferDuration,
		}, audioOutputCallback)

		log("JavaScript: initialization promise resolved")
		disposeAudioOutput = initResult.dispose
	} catch (e) {
		openPromise.reject(e)
	}

	return openPromise.promise
}
