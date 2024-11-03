#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>
#include <sstream>
#include <thread>
#include <chrono>

#include <alsa/asoundlib.h>
#include <napi.h>

#include "../include/Signal.h"
#include "../include/Utils.h"

class NodeAudioOutput {
private:
	Napi::ThreadSafeFunction threadSafeCallbackWrapper = Napi::ThreadSafeFunction();
	bool disposeRequested = false;
	std::vector<Napi::Reference<Napi::Int16Array>> outputBuffers;

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

		auto sampleRate = configObject.Get("sampleRate").As<Napi::Number>().Int64Value();
		auto channelCount = configObject.Get("channelCount").As<Napi::Number>().Int64Value();
		auto bufferDuration = configObject.Get("bufferDuration").As<Napi::Number>().FloatValue();
		auto userCallback = info[1].As<Napi::Function>();

		// Compute buffer sample count
		auto bufferFrameCount = static_cast<int64_t>((bufferDuration / 1000.0) * float(sampleRate));
		auto bufferSampleCount = bufferFrameCount * channelCount;

		trace("Sample rate: %d Hz\n", sampleRate);
		trace("Channel count: %d\n", channelCount);
		trace("Buffer duration: %f milliseconds\n", bufferDuration);
		trace("Buffer frame count: %d\n", bufferFrameCount);

		// Open ALSA device for playback
		trace("Initializing ALSA output..\n");

		int err;
		snd_pcm_t* pcmHandle;

		err = snd_pcm_open(&pcmHandle, "default", SND_PCM_STREAM_PLAYBACK, 0);

		if (err < 0) {
			initializationPromiseDeferred.Reject(Napi::Error::New(env, "Failed to open audio device").Value());

			return initializationPromise;
		}

		// Allocate a hardware parameters object
		snd_pcm_hw_params_t* params;

		snd_pcm_hw_params_malloc(&params);
		snd_pcm_hw_params_any(pcmHandle, params);

		// Set sample rate
		auto targetSampleRate = static_cast<unsigned int>(sampleRate);
		snd_pcm_hw_params_set_rate_near(pcmHandle, params, &targetSampleRate, 0);

		// Set channel count
		auto targetChannelCount = static_cast<unsigned int>(channelCount);
		snd_pcm_hw_params_set_channels(pcmHandle, params, targetChannelCount);

		// Set format
		snd_pcm_hw_params_set_format(pcmHandle, params, SND_PCM_FORMAT_S16_LE);

		// Set PCM access type
		snd_pcm_hw_params_set_access(pcmHandle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

		// Set period time
		{
			unsigned int targetPeriodTime = 10 * 1000; // 10ms
			int targetPeriodTimeDirection = 0;
			snd_pcm_hw_params_set_period_time_near(pcmHandle, params, &targetPeriodTime, &targetPeriodTimeDirection);
		}

		// Set buffer time
		if (false) {
			unsigned int bufferDurationNanoseconds = (unsigned int)(bufferDuration * 1000);
			int bufferTimeDirection = 0;
			snd_pcm_hw_params_set_buffer_time_near(pcmHandle, params, &bufferDurationNanoseconds, &bufferTimeDirection);
		}

		// Initialize JavaScript callback wrapper
		this->threadSafeCallbackWrapper = Napi::ThreadSafeFunction::New(env, userCallback, "threadSafeCallbackWrapper", 1, 1);

		// Write the parameters to the driver
		err = snd_pcm_hw_params(pcmHandle, params);

		// If failed
		if (err < 0) {
			std::stringstream errorString;
			errorString << "Error " << err << " occurred while initializing ALSA output: " << snd_strerror(err);

			initializationPromiseDeferred.Reject(Napi::Error::New(env, errorString.str()).Value());

			// Dispose ALSA handle and parameters
			snd_pcm_close(pcmHandle);
			snd_pcm_hw_params_free(params);

			// Release callback wrapper
			this->threadSafeCallbackWrapper.Release();

			delete this;

			return initializationPromise;
		}

		trace("ALSA output initialized\n");

		// Get ALSA buffer size (frames) and period size (frames)
		snd_pcm_uframes_t alsaBufferFrameCount;
		snd_pcm_uframes_t alsaPeriodFrameCount;
		err = snd_pcm_get_params(pcmHandle, &alsaBufferFrameCount, &alsaPeriodFrameCount);

		if (err < 0) {
			std::stringstream errorString;
			errorString << "Error " << err << " occurred while reading ALSA parameters: " << snd_strerror(err);

			initializationPromiseDeferred.Reject(Napi::Error::New(env, errorString.str()).Value());

			// Dispose ALSA handle and parameters
			snd_pcm_close(pcmHandle);
			snd_pcm_hw_params_free(params);

			// Release callback wrapper
			this->threadSafeCallbackWrapper.Release();

			delete this;

			return initializationPromise;
		}

		trace("ALSA buffer frame count: %d, ALSA period frame count: %d\n", alsaBufferFrameCount, alsaPeriodFrameCount);

		// Initialize Int16Array buffers
		for (int i = 0; i < 2; i++) {
			auto napiBuffer = Napi::Int16Array::New(env, bufferSampleCount);
			auto napiBufferReference = Napi::Persistent(napiBuffer);

			outputBuffers.push_back(std::move(napiBufferReference));
		}

		// Start a new thread for the output loop
		std::thread([=]() {
			auto waitUntilALSABufferIsSufficientlyDrained = [&](int targetRemainingFrameCount) -> int {
				while (true) {
					// Get available frame count in ALSA buffer, andd ALSA I/O latency (in frames)
					snd_pcm_sframes_t availableFrameCount;
					snd_pcm_sframes_t delayInFrames;
					auto infoRequestErrorCode = snd_pcm_avail_delay(pcmHandle, &availableFrameCount, &delayInFrames);

					//trace("Available: %d, Delay: %d, Fill estimate: %d\n", availableFrames, delayInFrames, fillEstimate);

					// Handle underruns, if possible, or error
					if (infoRequestErrorCode == -EPIPE) {
						trace("Buffer underrun detected while waiting\n");

						auto recoverResult = snd_pcm_recover(pcmHandle, -EPIPE, 1);

						if (recoverResult < 0) {
							trace("Failed to recover from buffer underrun\n");

							//Napi::Error::New(env, "Failed to recover from buffer underrun").ThrowAsJavaScriptException();

							return recoverResult;
						}

						trace("Buffer underrun recovered\n");
					} else if (false && infoRequestErrorCode < 0) {
						trace("Unrecoverable error (%d) occurred while waiting: %s\n", infoRequestErrorCode, snd_strerror(infoRequestErrorCode));

						//Napi::Error::New(env, "Unrecoverable error occured while reading ALSA buffer information").ThrowAsJavaScriptException();

						return infoRequestErrorCode;
					}

					// Derive an estimate of how many frames remain in the buffer
					auto fillEstimate = availableFrameCount >= 0 ? alsaBufferFrameCount - availableFrameCount : 0;

					// If the number of remaining frames is smaller or equal to the target, break
					if (fillEstimate <= targetRemainingFrameCount) {
						return 0;
					}

					// Sleep for 1 millisecond
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
			};

			Signal signal;
			auto currentBufferIndex = 0;

			// Start the loop
			while (!this->disposeRequested) {
				trace("Waiting for ALSA buffer to become sufficently drained..\n");

				// Wait until the ALSA internal buffer is sufficiently drained
				auto waitResult = waitUntilALSABufferIsSufficientlyDrained(bufferFrameCount);

				if (waitResult < 0) {
					this->disposeRequested = true;

					break;
				}

				trace("Iteration start\n");

				// Call back into JavaScript to let the user write to the buffer
				auto status = this->threadSafeCallbackWrapper.BlockingCall([&](Napi::Env env, Napi::Function jsCallback) {
					// Get current buffer
					auto currentBuffer = outputBuffers[currentBufferIndex].Value();

					// Set current buffer to all 0s (silence)
					std::memset((void*)currentBuffer.Data(), 0, currentBuffer.ByteLength());

					// Call back to JavaScript to have the buffer filled with samples
					jsCallback.Call({ currentBuffer });

					// Write buffer to ALSA output
					auto writeResult = snd_pcm_writei(pcmHandle, currentBuffer.Data(), bufferFrameCount);

					// Detect buffer underruns and try to recover
					if (writeResult == -EPIPE) {
						trace("Buffer underrun detected\n");

						auto recoverResult = snd_pcm_recover(pcmHandle, writeResult, 1);

						if (recoverResult < 0) {
							Napi::Error::New(env, "Failed to recover from buffer underrun").ThrowAsJavaScriptException();

							this->RequestDispose();
							signal.send();

							return;
						}

						trace("Buffer underrun recovered\n");

						writeResult = snd_pcm_writei(pcmHandle, currentBuffer.Data(), bufferFrameCount);
					}

					if (writeResult < 0) {
						std::stringstream errorString;
						errorString << "Error " << writeResult << " occurred while writing ALSA output: " << snd_strerror(writeResult);

						Napi::Error::New(env, errorString.str()).ThrowAsJavaScriptException();

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

			trace("Disposing ALSA output..\n");

			if (false) {
				waitUntilALSABufferIsSufficientlyDrained(0);
			}

			// Wait for any remaining pending samples to play
			snd_pcm_drain(pcmHandle);

			// Dispose ALSA handle
			snd_pcm_close(pcmHandle);
			snd_pcm_hw_params_free(params);

			trace("ALSA output disposed\n");

			// Release callback wrapper
			this->threadSafeCallbackWrapper.Release();

			for (int i = 0; i < 2; i++) {
				outputBuffers[i].Unref();
			}

			// Delete NodeAudioOutput object
			delete this;
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
