#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <string>

#include <napi.h>
#include <AudioUnit/AudioUnit.h>

#include "../include/Signal.h"
#include "../include/Utils.h"

class NodeAudioOutput {
public:
	AudioUnit audioUnit;
	Napi::ThreadSafeFunction threadSafeCallbackWrapper = Napi::ThreadSafeFunction();

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
		auto requestedBufferFrameCount = static_cast<int>((bufferDuration / 1000.0) * float(sampleRate));

		trace("Sample rate: %d Hz\n", sampleRate);
		trace("Channel count: %d\n", channelCount);
		trace("Requested buffer duration: %f milliseconds\n", bufferDuration);
		trace("Requested buffer frame count: %d\n", requestedBufferFrameCount);

		// Initialize Audio Unit device for playback
		OSErr err;

		AudioComponentDescription audioComponentDescription = {
			.componentType = kAudioUnitType_Output,
			.componentSubType = kAudioUnitSubType_DefaultOutput,
			.componentManufacturer = kAudioUnitManufacturer_Apple,
		};

		AudioComponent audioOutputComponent = AudioComponentFindNext(NULL, &audioComponentDescription);

		if (!audioOutputComponent) {
			initializationPromiseDeferred.Reject(Napi::Error::New(env, "Couldn't find a default output").Value());

			return initializationPromise;
		}

		err = AudioComponentInstanceNew(audioOutputComponent, &this->audioUnit);

		if (err != 0) {
			std::stringstream errorString;
    		errorString << "Error creating unit: " << err;

			initializationPromiseDeferred.Reject(Napi::Error::New(env, errorString.str()).Value());

			this->Dispose();

			return initializationPromise;
		}

		// Set callback function
		AURenderCallbackStruct callbackStruct = {
			.inputProc = audioUnitCallback,
			.inputProcRefCon = this
		};

		err = AudioUnitSetProperty(
				audioUnit,
				kAudioUnitProperty_SetRenderCallback,
				kAudioUnitScope_Output,
				0,
				&callbackStruct,
				sizeof(callbackStruct));

		if (err != 0) {
			std::stringstream errorString;
    		errorString << "Error setting callback: " << err;

			initializationPromiseDeferred.Reject(Napi::Error::New(env, errorString.str()).Value());

			this->Dispose();

			return initializationPromise;
		}

		// Set stream format
		AudioStreamBasicDescription audioStreamBasicDescription = {
			.mFormatID = kAudioFormatLinearPCM,
			.mFormatFlags = 0
				| kAudioFormatFlagIsSignedInteger
				| kAudioFormatFlagIsPacked
				| kAudioFormatFlagIsNonInterleaved,
			.mSampleRate = static_cast<float>(sampleRate),
			.mBitsPerChannel = 16,
			.mChannelsPerFrame = channelCount,
			.mFramesPerPacket = 1,
			.mBytesPerFrame = 2,
			.mBytesPerPacket = 2,
		};

		err = AudioUnitSetProperty(
				audioUnit,
				kAudioUnitProperty_StreamFormat,
				kAudioUnitScope_Input,
				0,
				&audioStreamBasicDescription,
				sizeof(AudioStreamBasicDescription));

		if (err != 0) {
			std::stringstream errorString;
    		errorString << "Error setting stream format: " << err;

			initializationPromiseDeferred.Reject(Napi::Error::New(env, errorString.str()).Value());

			this->Dispose();

			return initializationPromise;
		}

		// Set maximum frames per slice
		{
			UInt32 maxFramesPerSlice = requestedBufferFrameCount;

			err = AudioUnitSetProperty(
					audioUnit,
					kAudioUnitProperty_MaximumFramesPerSlice,
					kAudioUnitScope_Output,
					0,
					&maxFramesPerSlice,
					sizeof(maxFramesPerSlice));

			if (err != 0) {
				std::stringstream errorString;
				errorString << "Error setting maximum frames per slice: " << err;

				initializationPromiseDeferred.Reject(Napi::Error::New(env, errorString.str()).Value());

				this->Dispose();

				return initializationPromise;
			}
		}

		// Set render quality
		if (false) {
			UInt32 renderQuality = 127;

			err = AudioUnitSetProperty(
					audioUnit,
					kAudioUnitProperty_RenderQuality,
					kAudioUnitScope_Global,
					0,
					&renderQuality,
					sizeof(renderQuality));

			if (err != 0) {
				std::stringstream errorString;
				errorString << "Error setting render quality: " << err;

				initializationPromiseDeferred.Reject(Napi::Error::New(env, errorString.str()).Value());

				this->Dispose();

				return initializationPromise;
			}
		}

		// Initialize audio unit
		err = AudioUnitInitialize(audioUnit);
		if (err != 0) {
			std::stringstream errorString;
			errorString << "Error initializing audio unit: " << err;

			initializationPromiseDeferred.Reject(Napi::Error::New(env, errorString.str()).Value());

			this->Dispose();

			return initializationPromise;
		}

		trace("Initialized Audio Unit\n");

		// Initialize JavaScript callback wrapper
		this->threadSafeCallbackWrapper = Napi::ThreadSafeFunction::New(env, userCallback, "threadSafeCallbackWrapper", 1, 1);

		// Start audio unit
		err = AudioOutputUnitStart(audioUnit);
		if (err != 0) {
			std::stringstream errorString;
			errorString << "Error starting audio unit: " << err;

			initializationPromiseDeferred.Reject(Napi::Error::New(env, errorString.str()).Value());

			this->Dispose();

			return initializationPromise;
		}

		trace("Started Audio Unit\n");

		// Build result object
		auto resultObject = Napi::Object().New(env);

		auto disposeMethod = [this](const Napi::CallbackInfo& info) {
			this->Dispose();
		};

		resultObject.Set(Napi::String::New(env, "dispose"), Napi::Function::New(env, disposeMethod));

		// Resolve initialization promise with the result object
		initializationPromiseDeferred.Resolve(resultObject);

		return initializationPromise;
	}

	static OSStatus audioUnitCallback(
			void* inRefCon,
			AudioUnitRenderActionFlags* ioActionFlags,
			const AudioTimeStamp* inTimeStamp,
			UInt32 inBusNumber,
			UInt32 inNumberOfFrames,
			AudioBufferList* ioData) {

		trace("Callback called. Bus number: %d, Buffer count: %d, Frame count: %d\n", inBusNumber, ioData->mNumberBuffers, inNumberOfFrames);

		auto instance = static_cast<NodeAudioOutput*>(inRefCon);

		Signal signal;

		auto blockingCallStatus = instance->threadSafeCallbackWrapper.BlockingCall([&](Napi::Env env, Napi::Function jsCallback) {
			auto frameCount = inNumberOfFrames;
			auto channelCount = ioData->mNumberBuffers;
			auto buffers = ioData->mBuffers;

			// Create interleaved buffer
			auto interleavedBufferLength = frameCount * channelCount;
			int16_t interleavedBuffer[interleavedBufferLength];
			std::memset(interleavedBuffer, 0, interleavedBufferLength * sizeof(int16_t));

			// Create a typed array that wraps the interleaved buffer
			auto arrayBuffer = Napi::ArrayBuffer::New(env, interleavedBuffer, interleavedBufferLength * sizeof(int16_t));
			auto typedArrayForBuffer = Napi::Int16Array::New(env, interleavedBufferLength, arrayBuffer, 0);

			// Call back to JavaScript to have the interleaved buffer filled with samples
			jsCallback.Call({ typedArrayForBuffer });

			// Deinterleave the updated interleaved buffer into the callback buffers
			auto readIndex = 0;

			for (auto frameIndex = 0; frameIndex < frameCount; frameIndex++) {
				for (auto channelIndex = 0; channelIndex < channelCount; channelIndex++) {
					auto buffer = static_cast<int16_t*>(buffers[channelIndex].mData);

					buffer[frameIndex] = interleavedBuffer[readIndex++];
				}
			}

			signal.send();
		});

		signal.wait();

		return noErr;
	}

	void Dispose() {
		trace("Stopping and disposing Audio Unit instance..\n");

		// Dispose Audio Unit
		AudioOutputUnitStop(this->audioUnit);
		AudioUnitUninitialize(this->audioUnit);
		AudioComponentInstanceDispose(this->audioUnit);

		// Release callback wrapper
		this->threadSafeCallbackWrapper.Release();

		trace("Audio Unit instance disposed\n");

		// Delete object
		delete this;
	}
};

Napi::Promise InitializeAudioOutput(const Napi::CallbackInfo& info) {
	auto env = info.Env();

	auto output = new NodeAudioOutput();

	return output->Initialize(info);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
	exports.Set(Napi::String::New(env, "initAudioOutput"), Napi::Function::New(env, InitializeAudioOutput));

	return exports;
}

NODE_API_MODULE(addon, Init)
