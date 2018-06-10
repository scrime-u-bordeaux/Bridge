
#include "AUMIDIEffectBase.h"


#include "../common/client.cpp"


struct BridgeAU : public AUMIDIEffectBase {
	BridgeClient *client;

	BridgeAU(AudioUnit component) : AUMIDIEffectBase(component) {
		CreateElements();
		Globals()->UseIndexedParameters(1 + BRIDGE_NUM_PARAMS);
		client = new BridgeClient();
	}

	~BridgeAU() {
		delete client;
	}

	bool SupportsTail() override {
		return true;
	}

	Float64 GetTailTime() override {
		// 1 year, essentially infinite
		return (60.0 * 60.0 * 24.0 * 365.0);
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
				outParameterInfo.minValue = 0.f;
				outParameterInfo.maxValue = (float) (BRIDGE_NUM_PARAMS - 1);
				outParameterInfo.defaultValue = 0.f;
				return noErr;
			}
			else if (1 <= inParameterID && inParameterID < BRIDGE_NUM_PARAMS + 1) {
				CFStringRef name = CFStringCreateWithFormat(NULL, NULL, CFSTR("CC %d"), (int) (inParameterID - 1));
				FillInParameterName(outParameterInfo, name, false);
				outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
				outParameterInfo.minValue = 0.f;
				outParameterInfo.maxValue = 10.f;
				outParameterInfo.defaultValue = 0.f;
				return noErr;
			}
		}
		return kAudioUnitErr_InvalidParameter;
	}

	OSStatus GetParameter( AudioUnitParameterID inID, AudioUnitScope inScope, AudioUnitElement inElement, AudioUnitParameterValue &outValue) override {
		if (inScope == kAudioUnitScope_Global) {
			if (inID == 0) {
				outValue = (float) client->getPort();
				return noErr;
			}
			else if (1 <= inID && inID < BRIDGE_NUM_PARAMS + 1) {
				outValue = client->getParam(inID - 1);
				return noErr;
			}
		}
		return kAudioUnitErr_InvalidParameter;
	}

	OSStatus SetParameter(AudioUnitParameterID inID, AudioUnitScope inScope, AudioUnitElement inElement, AudioUnitParameterValue inValue, UInt32 inBufferOffsetInFrames) override {
		if (inScope == kAudioUnitScope_Global) {
			if (inID == 0) {
				client->setPort((int) roundf(inValue));
				return noErr;
			}
			else if (1 <= inID && inID < BRIDGE_NUM_PARAMS + 1) {
				client->setParam(inID - 1, inValue);
				return noErr;
			}
		}
		return kAudioUnitErr_InvalidParameter;
	}

	OSStatus ProcessBufferLists(AudioUnitRenderActionFlags &ioActionFlags, const AudioBufferList &inBuffer, AudioBufferList &outBuffer, UInt32 inFramesToProcess) override {
		double beat;
		double tempo;
		if (!CallHostBeatAndTempo(&beat, &tempo)) {
			printf("%f %f\n", beat, tempo);
		}

		// Set sample rate
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
				float r = (float) rand() / RAND_MAX;
				buffer[i] = output[BRIDGE_OUTPUTS * i + c] + (r * 2.f - 1.f);
			}
		}
		printf("%d\n", inFramesToProcess);
		return noErr;
	}

	OSStatus MIDIEvent(UInt32 inStatus, UInt32 inData1, UInt32 inData2, UInt32 inOffsetSampleFrame) override {
		MidiMessage msg;
		msg.cmd = inStatus;
		msg.data1 = inData1;
		msg.data2 = inData2;
		client->pushMidi(msg);
		// log("%02x %02x %02x", msg.cmd, msg.data1, msg.data2);		
		return noErr;
	}
};


AUDIOCOMPONENT_ENTRY(AUMIDIEffectFactory, BridgeAU)
