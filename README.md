# copingTracker

copingTracker is a fork of picoTracker (https://www.github.com/xiphonics/picoTracker)

## Features

* 8 song channels
* 256 chains
* 128 phrases
* up to 32 instruments consisting of 
    * Sample instruments
    * MIDI instruments
    * SID instruments
    * OPAL instruments
    * Chiptune instruments
* 8 or 16bit samples up to 44.1kHz, mono or stereo
* 16bit/44.1kHz/Stereo audio output

The picoTracker is powered by an RP2040 microcontroller and supports the following hardware:

* Headphone/Lineout
* TRS MIDI In & Out, USB MIDI Out
* 320x240 2.8in LCD display
* 16MB of Flash
* MicroSD cards upto 32GB for project & sample library storage
* USB-C for MIDI, charging and simple drag&drop firmware upgrades

## Limitations

* The pico will probably struggle with 8 song channels playing at the same time in most cases (chiptune instruments are fine, sample, SID and OPAL are not)
* Samples are copied to flash upon load and played from there. Since flash has to be shared with program code, only 8MB is available for it when using the typical 16M flash

## Experimental Features

The CSID and OPAL synth instruments are expermental and may change in significantly in functionality or even be removed in future releases.

## MANUAL

tbd

## Remote UI

The original picoTracker [remote UI is available here](https://ui.xiphonics.com/) and its [git repo is here](https://github.com/xiphonics/picotracker_client).

## Get involved or chat?

Feel free to hop on the [copingTracker discord](https://props-north.com/discord). 

## Development

Head over to the [Developer Guide](docs/DEV.md)
