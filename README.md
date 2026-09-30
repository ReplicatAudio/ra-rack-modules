# Rackmods (RA-VCV)

A collection of free ReplicatAudio VCV-Rack modules. 

**Major work in progress. Many things are incomplete and will change. Do not use these in serious projects that you want to come back to later!** 

More docs and graphics coming soon. 

## Docs

[Module docs](./doc-user/)!

See [Dev docs](./doc-dev/) for development and building info. Specifially the [tips doc](./doc-dev/tips.md). 

## Modules

| Module | Description |
|--------|-------------|
| [ra-accumulator](./doc-user/ra-accumulator.md) | CV accumulator with origin, delta write, reset, and slew |
| [ra-adsr](./doc-user/ra-adsr.md) | Polyphonic ADSR envelope generator with CV control and visual display |
| [ra-adsr3](./doc-user/ra-adsr3.md) | Compact ADSR envelope generator with gate, retrigger, and trigger inputs |
| [ra-ar](./doc-user/ra-ar.md) | Dual attack/release envelope with per-channel controls |
| [ra-autotrig](./doc-user/ra-autotrig.md) | 4-channel CV delta trigger — fires when CV changes by a set amount |
| [ra-blank](./doc-user/ra-blank.md) | Resizable blank panel for filling empty space |
| [ra-bump](./doc-user/ra-bump.md) | Trigger sequencer with 8 toggleable steps |
| [ra-buttons](./doc-user/ra-buttons.md) | 4-channel trigger/gate source with LED buttons and multiple modes |
| [ra-calc](./doc-user/ra-calc.md) | 4-channel math module with 17 operations, attenuverter, and clamp |
| [ra-chord](./doc-user/ra-chord.md) | CV transposer with 4 outputs offset by semitones |
| [ra-clap](./doc-user/ra-clap.md) | 808-style clap drum with multi-tap noise bursts |
| [ra-control](./doc-user/ra-control.md) | Animated cables with signal visualization |
| [ra-countdown](./doc-user/ra-countdown.md) | Trigger countdown — fires after N triggers |
| [ra-cymbal](./doc-user/ra-cymbal.md) | 808-style crash cymbal with inharmonic partials |
| [ra-delay-cv](./doc-user/ra-delay-cv.md) | DC-coupled CV/trigger delay line with clock sync |
| [ra-freeberd](./doc-user/ra-freeberd.md) | Classic Freeverb stereo reverb |
| [ra-gear](./doc-user/ra-gear.md) | Visual gear counter with increment/decrement and cycle trigger |
| [ra-glitch](./doc-user/ra-glitch.md) | Random glitch processor with freeze, swap, and drop |
| [ra-gnawbz](./doc-user/ra-gnawbz.md) | 3-channel control voltage source |
| [ra-hat](./doc-user/ra-hat.md) | 808-style hi-hat with inharmonic metallic partials |
| [ra-just](./doc-user/ra-just.md) | Just intonation CV quantizer with 6 channels |
| [ra-karplus-strong](./doc-user/ra-karplus-strong.md) | Physical modelling plucked string synthesis |
| [ra-kick](./doc-user/ra-kick.md) | 808-style kick drum with pitch-swept sine |
| [ra-klock](./doc-user/ra-klock.md) | Master clock with BPM, swing, and multiple outputs |
| [ra-krush](./doc-user/ra-krush.md) | Bitcrusher/downsampler with 8 algorithms |
| [ra-lerper](./doc-user/ra-lerper.md) | Lerp toward A/B/C/D targets with visual display |
| [ra-lfo](./doc-user/ra-lfo.md) | Voltage-controlled LFO with clock sync |
| [ra-logic](./doc-user/ra-logic.md) | 8 configurable logic gates (AND, OR, XOR, etc.) |
| [ra-lsystem](./doc-user/ra-lsystem.md) | L-system drum/trigger module with visual matrix |
| [ra-m2](./doc-user/ra-m2.md) | Stereo master processor with gain, limiter, and mute |
| [ra-macrow](./doc-user/ra-macrow.md) | Single macro knob with 4 attenuverted outputs |
| [ra-magus](./doc-user/ra-magus.md) | Scheme expression evaluator with CV inputs |
| [ra-meteor](./doc-user/ra-meteor.md) | 808-style metallic percussion (hat, crash, ride) |
| [ra-minmax](./doc-user/ra-minmax.md) | 3-channel min/max math module with active input LEDs |
| [ra-mix4](./doc-user/ra-mix4.md) | 4-channel stereo mixer with gain and pan |
| [ra-mothership](./doc-user/ra-mothership.md) | 8-oscillator LFO/VCO with wave morphing |
| [ra-noyz](./doc-user/ra-noyz.md) | Noise source with multiple colors and filters |
| [ra-ntet](./doc-user/ra-ntet.md) | N-TET CV processor with chromatic stretch or quantize |
| [ra-quant](./doc-user/ra-quant.md) | 8-channel pitch quantizer with selectable scale |
| [ra-ranger](./doc-user/ra-ranger.md) | 4-channel signal scaler with clipping and wave folding |
| [ra-ranger2](./doc-user/ra-ranger2.md) | CV range rescaler |
| [ra-rec](./doc-user/ra-rec.md) | 4-track CV/audio recorder with scope display |
| [ra-reflectingpool](./doc-user/ra-reflectingpool.md) | OP-1 style endless sequencer |
| [ra-repeater](./doc-user/ra-repeater.md) | Trigger repeater with delay and chance per output |
| [ra-repitch](./doc-user/ra-repitch.md) | Phase vocoder pitch shifter |
| [ra-resampler](./doc-user/ra-resampler.md) | Polyphonic sample player with 1V/oct pitch shifting |
| [ra-reverbir](./doc-user/ra-reverbir.md) | Stereo convolution reverb |
| [ra-ride](./doc-user/ra-ride.md) | 808-style ride cymbal with bell ping |
| [ra-seer](./doc-user/ra-seer.md) | Dual oscilloscope with Lissajous and spectrum |
| [ra-seer-mini](./doc-user/ra-seer-mini.md) | Signal meter / oscilloscope / spectrum analyzer |
| [ra-shapes](./doc-user/ra-shapes.md) | VCO with sine, triangle, saw, and square outputs |
| [ra-slowrandom](./doc-user/ra-slowrandom.md) | Slow random LFO with Perlin, smooth, and brown noise |
| [ra-snare](./doc-user/ra-snare.md) | 808-style snare drum with noise rattle |
| [ra-think](./doc-user/ra-think.md) | Simple VCO with morphing waveform and filter |
| [ra-tom](./doc-user/ra-tom.md) | 808-style tom with pitch-swept body |
| [ra-tracker](./doc-user/ra-tracker.md) | 4-channel tracker-style sequencer with screen |
| [ra-trigcv](./doc-user/ra-trigcv.md) | 8 trigger-to-CV mapper |
| [ra-tuner](./doc-user/ra-tuner.md) | Tuner showing frequency and note |
| [ra-tyche](./doc-user/ra-tyche.md) | Probabilistic trigger divider with bias CV |
| [ra-ulfo](./doc-user/ra-ulfo.md) | Ultra low frequency LFO |
| [ra-vash](./doc-user/ra-vash.md) | 64-step drum sequencer with Game of Life |
| [ra-vca](./doc-user/ra-vca.md) | Voltage-controlled amplifier |
| [ra-vipberus](./doc-user/ra-vipberus.md) | Polyphonic additive synth with 16 harmonics |
| [ra-vnote](./doc-user/ra-vnote.md) | Text editor for patch notes |
| [ra-xyout](./doc-user/ra-xyout.md) | Dual CV source with XY pad |
| [ra-zeno](./doc-user/ra-zeno.md) | Slowdown/halftime effect |

## Usage

```bash
# Clone
git clone https://github.com/ReplicatAudio/ra-rack-modules

# Move to project root
cd ra-rack-modules

# Pull the vcv rack sdk
./util/pull-sdk.sh

# Build and install the plugin (all modules)
./util/make.sh
```

This will automatically install the modules as well (at least for linux).

## Contibutions

Contibutions are limited to people I know personally at this time. If you know me and want to make a PR feel free. PRs may be open to the public in the future. 

## License

All code is GPL3 or later unless otherwise specified. 

All art assets are copyright Mathieu Dombrock. No 3rd party artwork has been used. 

### Font

Internal svg font is `Bitstream Vera Sans Mono` (OFL)

URW GOTHIC

