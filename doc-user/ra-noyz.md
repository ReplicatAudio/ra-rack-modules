# ra-noyz — Noise Source

White, pink, brown/red, violet, blue, gray, or black noise through a resonant lowpass → highpass filter pair, with amplitude and gate control.

The seven noise colors are the same sources as the stock VCV Rack "Noise" module, all calibrated to the same RMS levels:

- **White** (0 dB/oct): Gaussian (normal) white noise, flat power density.
- **Pink** (−3 dB/oct): Voss-array pink noise.
- **Brown/Red** (−6 dB/oct): Brownian noise — 20 Hz Butterworth lowpassed white noise.
- **Violet** (+6 dB/oct): Differentiated white noise.
- **Blue** (+3 dB/oct): Differentiated pink noise.
- **Gray** (psychoacoustic equal loudness): White noise filtered with an inverse A-weighting curve (FFT, 1024 bins).
- **Black** (uniform): Uniform random numbers, −5 V to +5 V (not RMS-calibrated).

## Controls
- **Color**: 7-position knob selecting the noise color (White, Pink, Brown/Red, Violet, Blue, Gray, Black).
- **LP cut**: lowpass filter cutoff (20 Hz–20 kHz, logarithmic).
- **LP res**: lowpass filter resonance (0–100%).
- **HP cut**: highpass filter cutoff (20 Hz–20 kHz, logarithmic).
- **HP res**: highpass filter resonance (0–100%).
- **Amplitude**: output level (0–100%).

## Inputs
- **Gate**: when patched, sound only comes out while the gate is high (≥ 1 V); when unpatched, sound always comes out.
- **LP cut / LP res / HP cut / HP res / Amplitude CV**: when connected, the matching knob acts as a unipolar attenuator (0–100%) scaling the CV. Cutoff CVs are 1 V/oct above the 20 Hz minimum (0 V = 20 Hz; +10 V ≈ 20 kHz when the knob is at 100%); res and amplitude scale as knob × CV (±10 V = full range when the knob is at 100%). Knob at 0 = no CV effect.

## Outputs
- **Audio**: filtered noise output.