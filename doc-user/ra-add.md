# ra-add — Additive Oscillator

Additive synth VCO with 16 selectable harmonic amplitudes. Output is normalized by the sum of active harmonic levels. A vertical bar graph on the panel shows the effective level of all 16 channels.

## Controls
- **Frequency**: coarse pitch control.
- **FM attenuation**: amount of FM applied to the oscillator.
- **Harmonic 1–16**: level of each harmonic (0–100%). When a harmonic's CV input is patched, the knob acts as an attenuator for the CV signal.

## Inputs
- **Pitch**: 1 V/oct pitch CV.
- **FM**: frequency modulation input (scaled by FM attenuation).
- **Harmonic 1 CV–16 CV**: CV input for each harmonic level (0–10 V). When patched, the corresponding knob multiplies the CV to produce the effective harmonic level.

## Outputs
- **Audio**: mixed harmonic output, ±5 V.
