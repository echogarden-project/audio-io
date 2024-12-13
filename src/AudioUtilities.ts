export function getSineWave(frequency: number, sampleCount: number, sampleRate: number) {
	const audioSamples = new Float32Array(sampleCount)

	const audioGainScaler = 0.1

	for (let i = 0; i < sampleCount; i++) {
		audioSamples[i] = Math.sin(2.0 * Math.PI * frequency * i / sampleRate) * audioGainScaler;
	}

	return audioSamples
}
