import { playTestTone, playWaveData } from './Playback.js'

const log = console.log

async function testTone() {
	await playTestTone()
}

async function testWaveFile() {
	const { readFile }  = await import('fs/promises')

	const waveData = await readFile('test-audio/M1F1-uint8-AFsp.wav')
	await playWaveData(waveData)
}

async function testAllWaveFiles() {
	const { readFile, readdir } = await import('fs/promises')

	const fileNames = await readdir('test-audio')

	for (const fileName of fileNames) {
		log(`Playing ${fileName}`)

		const waveData = await readFile(`test-audio/${fileName}`)

		try {
			await playWaveData(waveData)
		} catch(e) {
			log(`Failed to play file:\n${e}`)
		}

		log('')
	}
}

testAllWaveFiles()
