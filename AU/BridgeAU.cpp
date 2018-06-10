
#include "MusicDeviceBase.h"


#include "../common/client.cpp"


struct BridgeAU : public MusicDeviceBase {
	BridgeClient *client;

	BridgeAU(AudioUnit component) : MusicDeviceBase(component, BRIDGE_INPUTS, BRIDGE_OUTPUTS, 1) {
		CreateElements();
		Globals()->UseIndexedParameters(1 + BRIDGE_NUM_PARAMS);
		client = new BridgeClient();
	}

	~BridgeAU() {
		delete client;
	}

	bool CanScheduleParameters() const override {
		return false;
	}

	bool StreamFormatWritable(AudioUnitScope scope, AudioUnitElement element) override {
		return !IsInitialized();
	}

	Float64 GetTailTime() override {
		// 1 year, essentially infinite
		return (60.0 * 60.0 * 24.0 * 365.0);
	}

	bool SupportsTail() override {
		return true;
	}

	ComponentResult GetParameterValueStrings(AudioUnitScope inScope, AudioUnitParameterID inParameterID, CFArrayRef *outStrings) override {
		if (!outStrings)
			return noErr;

		if (inScope == kAudioUnitScope_Global) {
			if (inParameterID == 0) {
				CFStringRef strings[BRIDGE_NUM_PORTS];
				for (int i = 0; i < BRIDGE_NUM_PORTS; i++) {
					strings[i] = CFStringCreateWithFormat(NULL, NULL, CFSTR("%d"), i + 1);
				}

				*outStrings = CFArrayCreate(NULL, (const void**) strings, 16, NULL);
				return noErr;
			}
		}
		return kAudioUnitErr_InvalidParameter;
	}

	ComponentResult GetParameterInfo(AudioUnitScope inScope, AudioUnitParameterID inParameterID, AudioUnitParameterInfo &outParameterInfo) override {
		outParameterInfo.flags = kAudioUnitParameterFlag_IsWritable | kAudioUnitParameterFlag_IsReadable;

		if (inScope == kAudioUnitScope_Global) {
			if (inParameterID == 0) {
				FillInParameterName(outParameterInfo, CFSTR("Port"), false);
				outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
				outParameterInfo.minValue = 0.0;
				outParameterInfo.maxValue = (float) (BRIDGE_NUM_PARAMS - 1);
				outParameterInfo.defaultValue = 0.0;
				return noErr;
			}
			else if (1 <= inParameterID && inParameterID < BRIDGE_NUM_PARAMS + 1) {
				CFStringRef name = CFStringCreateWithFormat(NULL, NULL, CFSTR("CC %d"), (int) (inParameterID - 1));
				FillInParameterName(outParameterInfo, name, false);
				outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
				outParameterInfo.minValue = 0.0;
				outParameterInfo.maxValue = 10.0;
				outParameterInfo.defaultValue = 0.0;
				return noErr;
			}
		}
		return kAudioUnitErr_InvalidParameter;
	}

	/*
		OSStatus ProcessBufferLists(AudioUnitRenderActionFlags &ioActionFlags, const AudioBufferList &inBuffer, AudioBufferList &outBuffer, UInt32 inFramesToProcess) override {
			// Push parameters
			int port = (int) Globals()->GetParameter(0);
			client->setPort(port);
			for (int i = 0; i < BRIDGE_NUM_PARAMS; i++) {
				float param = Globals()->GetParameter(i + 1) / 10.f;
				client->setParam(i, param);
			}
			client->setSampleRate((int) GetOutput(0)->GetStreamFormat().mSampleRate);

			// TODO Check that the stream is Float32, add better error handling.
			float input[BRIDGE_INPUTS * inFramesToProcess];
			float output[BRIDGE_OUTPUTS * inFramesToProcess];
			memset(input, 0, sizeof(input));
			memset(output, 0, sizeof(output));
			// Interleave input
			for (int c = 0; c < (int) inBuffer.mNumberBuffers; c++) {
				const float *buffer = (const float*) inBuffer.mBuffers[c].mData;
				for (int i = 0; i < (int) inFramesToProcess; i++) {
					input[BRIDGE_INPUTS * i + c] = buffer[i];
				}
			}
			// Process audio
			client->processStream(input, output, inFramesToProcess);
			// Deinterleave output
			for (int c = 0; c < (int) outBuffer.mNumberBuffers; c++) {
				float *buffer = (float*) outBuffer.mBuffers[c].mData;
				for (int i = 0; i < (int) inFramesToProcess; i++) {
					buffer[i] = output[BRIDGE_OUTPUTS * i + c];
				}
			}
			return noErr;
		}
	*/

	OSStatus Render(AudioUnitRenderActionFlags &ioActionFlags, const AudioTimeStamp& inTimeStamp, UInt32 nFrames) override {
		// printf(".");
		// AUInputElement *inputBus = GetInput(0);
		// AudioBufferList &inputBufferList = inputBus->GetBufferList();
		// AudioBuffer &inputBuffer = inputBufferList.mBuffers[0];
		// float *input = (float*) inputBuffer.mData;
		// printf("input buffers %d\n", inputBufferList.mNumberBuffers);
		double beat;
		double tempo;
		if (!CallHostBeatAndTempo(&beat, &tempo)) {
			printf("%f %f\n", beat, tempo);
		}

		AUOutputElement *outputBus = GetOutput(0);
		outputBus->PrepareBuffer(nFrames);
		AudioBufferList &outputBufferList = outputBus->GetBufferList();
		printf("output buffers %d\n", outputBufferList.mNumberBuffers);

		for (int i = 0; i < outputBufferList.mNumberBuffers; i++) {
			AudioBuffer &outputBuffer = outputBufferList.mBuffers[i];
			float *output = (float*) outputBuffer.mData;
			printf("\t%d channels %d\n", i, outputBuffer.mNumberChannels);
			for (int j = 0; j < nFrames; j++) {
				float r = (float) rand() / RAND_MAX;
				for (int c = 0; c < outputBuffer.mNumberChannels; c++) {
					output[j * outputBuffer.mNumberChannels + c] = (2.f * r - 1.f) * 0.01f;
				}
			}
		}
		return noErr;
	}

	OSStatus MIDIEvent(UInt32 inStatus, UInt32 inData1, UInt32 inData2, UInt32 inOffsetSampleFrame) override {
		printf("MIDIEvent: %02x %02x %02x at %d\n", inStatus, inData1, inData2, inOffsetSampleFrame);
		return noErr;
	}
};


AUDIOCOMPONENT_ENTRY(AUMusicDeviceFactory, BridgeAU)
