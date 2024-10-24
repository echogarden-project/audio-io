import { float32ToInt16Pcm, getSineWave, interleaveChannels } from './AudioUtils.js'

const log = console.log

async function start() {
	const sampleRate = 48000
	const channelCount = 2 as number
	const sineWaveDuration = 1

	const bufferDuration = 100

	const testToneLeft = getSineWave(440, sampleRate * sineWaveDuration, sampleRate)
	const testToneRight = getSineWave(280, sampleRate * sineWaveDuration, sampleRate)
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
		}

		offset += sampleCount
	}

	const NodeAudioOutput = await import('./Exports.js')

	if (!NodeAudioOutput.isPlatformSupported()) {
		throw new Error(`Platform is not supported`)
	}

	const initResult = await NodeAudioOutput.initAudioOutput({
		sampleRate,
		channelCount,
		bufferDuration,
	}, audioOutputCallback)

	log("JavaScript: initialization promise resolved")
	disposeAudioOutput = initResult.dispose
}

start()
