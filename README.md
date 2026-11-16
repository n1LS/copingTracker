# copingTracker

copingTracker is a fork of picoTracker ~2.3 (https://www.github.com/xiphonics/picoTracker). Maybe a spoon. Or a spork. Yes, probably a spork.

## Silly name. Why?
_copingTracker_ is essentially _picoTracker_ having an existential crisis - jumbled up a bit (_pico_ → _copi_), with some extra features thrown in (_n_ + _g_) and finally accepted its fate of _coping_ with having to run on an rp2040/2350.

## Features

* 8 song channels
* 256 chains
* 128 phrases
* up to 32 instruments consisting of 
    * Sample instruments
    * MIDI instruments
    * Stack instrument (multi-saw -> chord wavetable synthesizer)
    * internal 135 instrument GM Bank
    * Chiptune Drum
    * Chiptune Lead
* 8 or 16bit samples up to 44.1kHz, mono or stereo
* 16bit/44.1kHz/Stereo audio output

The picoTracker is powered by an RP2040 microcontroller and supports the following hardware:

* Headphone/Lineout
* TRS MIDI In & Out, USB MIDI Out
* 320x240 2.8in LCD display
* 16MB of Flash
* MicroSD cards upto 64 GB for project & sample library storage
* USB-C for MIDI, charging and simple drag&drop firmware upgrades

## Limitations

* The processor will probably struggle with 8 song channels playing SampleInstruments at the same time while 8x Chiptune instruments are fine. The number of used Effects also affects performance.
* Samples are copied to flash upon load and played from there. Since flash has to be shared with program code, only 8MB is available for it when using the typical 16M flash

## MANUAL

tbd

## mirrorUI

The [mirrorUI client is available here](https://ui.props-north.com/) and its [git repo is here](https://github.com/n1LS/mirrorUI).

## Get involved or chat?

Feel free to hop on the [copingTracker discord](https://props-north.com/discord). 

## Development

Head over to the [Developer Guide](docs/DEV.md)
