#include <thread>
#include <chrono>

#include <windows.h>
#include <mmsystem.h>

#include <napi.h>

#include "../include/Signal.h"
#include "../include/Utils.h"

HWAVEOUT createWaveOutHandle(int64_t sampleRate, int64_t channelCount) {
	int bytesPerSample = sizeof(int16_t);
	int bitsPerSample = bytesPerSample * 8;

	// Set up the wave format structure
	WAVEFORMATEX wfx;

	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = channelCount;
	wfx.nSamplesPerSec = sampleRate;
	wfx.wBitsPerSample = bitsPerSample;
	wfx.nBlockAlign = channelCount * bytesPerSample;
	wfx.nAvgBytesPerSec = sampleRate * wfx.nBlockAlign;
	wfx.cbSize = 0;

	HWAVEOUT waveOutHandle;

	if (waveOutOpen(&waveOutHandle, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
		return 0;
	}

	return waveOutHandle;
}

void disposeWaveOutHandle(HWAVEOUT waveOutHandle) {
	waveOutClose(waveOutHandle);
}

int initializeWaveHeader(HWAVEOUT waveOutHandle, WAVEHDR* waveHeader, int16_t* pcmSamples, uint32_t sampleCount, uint32_t bytesPerSample) {
	(*waveHeader).lpData = (LPSTR)pcmSamples;
	(*waveHeader).dwBufferLength = sampleCount * bytesPerSample;
	(*waveHeader).dwFlags = 0;

	// Prepare
	return waveOutPrepareHeader(waveOutHandle, waveHeader, sizeof(WAVEHDR));
}

void releaseWaveHeader(HWAVEOUT waveOutHandle, WAVEHDR* waveHeader) {
	waveOutUnprepareHeader(waveOutHandle, waveHeader, sizeof(WAVEHDR));
}

int isBufferDone(const WAVEHDR* waveHeader) {
	return ((*waveHeader).dwFlags & WHDR_DONE) == 1;
}

void waitUntilBufferIsDone(const WAVEHDR* waveHeader) {
	while (!isBufferDone(waveHeader)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

MMRESULT writeSamples(HWAVEOUT waveOutHandle, WAVEHDR* waveHeader) {
	return waveOutWrite(waveOutHandle, waveHeader, sizeof(WAVEHDR));
}

int getSamplePosition(HWAVEOUT waveOutHandle) {
	MMTIME timeData;
	timeData.wType = TIME_SAMPLES;

	waveOutGetPosition(waveOutHandle, &timeData, sizeof(timeData));

	return timeData.u.sample;
}

class NodeAudioOutput {
private:
	Napi::ThreadSafeFunction threadSafeCallbackWrapper = Napi::ThreadSafeFunction();
	bool disposeRequested = false;

	std::vector<Napi::Reference<Napi::Int16Array>> outputBuffers;
	std::vector<WAVEHDR> bufferHeaders;

public:
	Napi::Promise Initialize(const Napi::CallbackInfo& info) {
		auto env = info.Env();

		// Create initialization promise
		auto initializationPromiseDeferred = Napi::Promise::Deferred::New(info.Env());
		auto initializationPromise = initializationPromiseDeferred.Promise();

		// NOTE:
		// This method assumes that all arguments are 100% valid!
		// for simplicity of implementaiton, all arguments must be thoroughly pre-validated in JavaScript
		// by a wrapper method, before this one is called!
		Napi::Object configObject = info[0].As<Napi::Object>();

		auto sampleRate = configObject.Get("sampleRate").As<Napi::Number>().Uint32Value();
		auto channelCount = configObject.Get("channelCount").As<Napi::Number>().Uint32Value();
		auto bufferDuration = configObject.Get("bufferDuration").As<Napi::Number>().FloatValue();
		auto userCallback = info[1].As<Napi::Function>();

		// Compute buffer sample count
		auto bufferFrameCount = static_cast<int>((bufferDuration / 1000.0) * float(sampleRate));
		auto bufferSampleCount = bufferFrameCount * channelCount;

		trace("Sample rate: %d Hz\n", sampleRate);
		trace("Channel count: %d\n", channelCount);
		trace("Requested buffer duration: %f milliseconds\n", bufferDuration);
		trace("Requested buffer frame count: %d\n", bufferFrameCount);

		// Initialize wave out handle
		HWAVEOUT waveOutHandle = createWaveOutHandle(sampleRate, channelCount);

		if (waveOutHandle == 0) {
			initializationPromiseDeferred.Reject(Napi::Error::New(env, "Failed to create wave out handle").Value());

			return initializationPromise;
		}

		for (int i = 0; i < 2; i++) {
			auto napiBuffer = Napi::Int16Array::New(env, bufferSampleCount);
			auto napiBufferReference = Napi::Persistent(napiBuffer);

			outputBuffers.push_back(std::move(napiBufferReference));
			bufferHeaders.push_back(WAVEHDR());
		}

		// Initialize headers for buffers
		for (int i = 0; i < 2; i++) {
			// Initialize header
			auto status = initializeWaveHeader(waveOutHandle, &bufferHeaders[i], outputBuffers[i].Value().Data(), 0, sizeof(int16_t));

			if (status != 0) {
				initializationPromiseDeferred.Reject(Napi::Error::New(env, "Failed to initialize wave header").Value());

				// Note: should preferably also release any wave headers that succeeded initialization,
				// although it's highly unlikely that one header succeeded and the other didn't.

				disposeWaveOutHandle(waveOutHandle);

				return initializationPromise;
			}

			// Set it to done
			bufferHeaders[i].dwFlags |= WHDR_DONE;
		}

		// Initialize JavaScript callback wrapper
		this->threadSafeCallbackWrapper = Napi::ThreadSafeFunction::New(env, userCallback, "threadSafeCallbackWrapper", 1, 1);

		// Start a new thread for the output
		std::thread([=]() {
			Signal signal;

			// Start the loop
			auto currentBufferIndex = 0;

			while (!this->disposeRequested) {
				// Wait until the current buffer is done
				trace("Waiting until current MME buffer is done playing..\n");

				waitUntilBufferIsDone(&bufferHeaders[currentBufferIndex]);

				trace("Iteration start\n");

				// Call back into JavaScript to let the user write to the buffer
				auto blockingCallStatus = this->threadSafeCallbackWrapper.BlockingCall([&](Napi::Env env, Napi::Function jsCallback) {
					trace("Blocking call start\n");

					// Get current buffer
					auto currentNapiBuffer = outputBuffers[currentBufferIndex].Value();

					// Set current buffer to all 0s (silence)
					std::memset((void*)currentNapiBuffer.Data(), 0, currentNapiBuffer.ByteLength());

					// Get header for current buffer
					auto currentBufferHeader = &bufferHeaders[currentBufferIndex];

					// Reinitialize the header with current buffer
					releaseWaveHeader(waveOutHandle, currentBufferHeader);

					trace("Before JavaScript callback\n");

					// Call back to JavaScript to have the buffer filled with samples
					jsCallback.Call({ currentNapiBuffer });

					trace("After JavaScript callback\n");

					// Initialize wave header
					auto initResult = initializeWaveHeader(waveOutHandle, currentBufferHeader, currentNapiBuffer.Data(), bufferSampleCount, sizeof(int16_t));

					if (initResult != 0) {
						Napi::Error::New(env, "Failed to initialize wave header").ThrowAsJavaScriptException();

						this->RequestDispose();
						signal.send();

						return;
					}

					// Write buffer to audio output
					auto writeResult = writeSamples(waveOutHandle, currentBufferHeader);

					if (writeResult != 0) {
						Napi::Error::New(env, "Failed to write buffer to wave out").ThrowAsJavaScriptException();

						this->RequestDispose();
						signal.send();

						return;
					}

					// Switch to other buffer
					currentBufferIndex = currentBufferIndex == 0 ? 1 : 0;

					signal.send();
				});

				signal.wait();

				trace("Iteration end\n");
			}

			// Wait for buffers to complete playback and release them
			for (int i = 0; i < 2; i++) {
				waitUntilBufferIsDone(&bufferHeaders[i]);
				releaseWaveHeader(waveOutHandle, &bufferHeaders[i]);

				// Decrement reference count to allow the garbage collector to free
				// the Int16Array and its underlying memory
				outputBuffers[i].Unref();
			}

			// Dispose wave out handle
			disposeWaveOutHandle(waveOutHandle);

			// Release callback wrapper
			this->threadSafeCallbackWrapper.Release();

			// Delete NodeAudioOutput object
			delete this;

			trace("MME output disposed\n");
		}).detach();

		// Build result object
		auto resultObject = Napi::Object().New(env);

		auto disposeMethod = [this](const Napi::CallbackInfo& info) {
			this->RequestDispose();
		};

		resultObject.Set(Napi::String::New(env, "dispose"), Napi::Function::New(env, disposeMethod));

		// Resolve initialization promise with the result object
		initializationPromiseDeferred.Resolve(resultObject);

		return initializationPromise;
	}

	void RequestDispose() {
		trace("Dispose requested..\n");

		this->disposeRequested = true;
	}
};

Napi::Promise createAudioOutput(const Napi::CallbackInfo& info) {
	auto env = info.Env();

	auto output = new NodeAudioOutput();

	return output->Initialize(info);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
	exports.Set(Napi::String::New(env, "createAudioOutput"), Napi::Function::New(env, createAudioOutput));

	return exports;
}

NODE_API_MODULE(addon, Init)
