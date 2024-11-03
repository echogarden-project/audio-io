export function getSineWave(frequency: number, sampleCount: number, sampleRate: number) {
	const audioSamples = new Float32Array(sampleCount)

	const audioGainScaler = 0.1

	for (let i = 0; i < sampleCount; i++) {
		audioSamples[i] = Math.sin(2.0 * Math.PI * frequency * i / sampleRate) * audioGainScaler;
	}

	return audioSamples
}

export function interleaveChannels(channels: Float32Array[]) {
	const channelCount = channels.length

	if (channelCount === 0) {
		throw new Error('Empty channel array received')
	}

	if (channelCount === 1) {
		return channels[0].slice()
	}

	const sampleCount = channels[0].length
	const result = new Float32Array(sampleCount * channelCount)

	let writeIndex = 0

	for (let i = 0; i < sampleCount; i++) {
		for (let c = 0; c < channelCount; c++) {
			result[writeIndex] = channels[c][i]
			writeIndex += 1
		}
	}

	return result
}

export function float32ToInt16Pcm(input: Float32Array) {
	const output = new Int16Array(input.length)

	for (let i = 0; i < input.length; i++) {
		const sample = clampFloatSample(input[i])
		output[i] = (sample < 0 ? sample * 32768 : sample * 32767) | 0
	}

	return output
}

export function clampFloatSample(floatSample: number) {
	if (floatSample < -1.0) {
		return -1.0
	} else if (floatSample > 1.0) {
		return 1.0
	} else {
		return floatSample
	}
}
