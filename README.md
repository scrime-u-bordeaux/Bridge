# VCV Bridge

Documentation in the Bridge section of https://vcvrack.com/manual/Core.html.

## Building

Clone this repository in the root directory of the [Rack](https://github.com/VCVRack/Rack) repository. Or, clone this repository anywhere and set the `RACK_DIR` environment variable.

For the VST plugin, obtain `VST2_SDK/` from Steinberg's [VST 3 SDK](https://www.steinberg.net/en/company/developers.html), and place it in the `vst/` directory.

For the AU plugin, obtain `AudioUnitExamplesAudioUnitEffectGeneratorInstrumentMIDIProcessorandOffline/` from Apple's [Core Audio SDK](https://developer.apple.com/library/content/documentation/MusicAudio/Conceptual/AudioUnitProgrammingGuide/AQuickTouroftheCoreAudioSDK/AQuickTouroftheCoreAudioSDK.html), and place it in the `au/` directory.

Run `make dist` in each desired directory. The plugins are placed in `dist/`.

## Notes

### Linux

The VST plugin has been successfully tested on Ubuntu 16.04 with several DAWs: Ardour (5.12), Bitwig Studio (2.3), Qtractor (0.8.4).
However it might behave poorly if the host does not use an audio buffer with a fixed size (i.e. Renoise 3.1.0, Radium, 5.4.9).
In Carla (2.0-beta6), "Fixed-size buffer" has to be enabled in the plugin's options.

## License

Source code licensed under [BSD-3-Clause](LICENSE.txt) by [Andrew Belt](https://andrewbelt.name/)
