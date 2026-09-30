# ra-slowrandom — Slow Random Oscillator

A slow random LFO with multiple smooth noise sources and selectable output range.

## Controls
- **Rate** knob + **Rate CV**: controls how fast the noise evolves (0.01–10 Hz, default 1 Hz).
- **Source** switch: selects the noise algorithm.
  - **Perlin** — gradient noise, smooth and natural-sounding.
  - **Smooth** — interpolated random values, continuously drifting.
  - **Brown** — random walk, heavy low-frequency character.
- **Range** switch: selects the output voltage range.
  - **0-1V** — unipolar, 0 to 1 V.
  - **0-10V** — unipolar, 0 to 10 V.
  - **±5V** — bipolar, -5 to 5 V.
- **Scale** knob + **Scale CV**: scales the output amplitude from 0 to 100%.

## Inputs
- **Rate CV**: added to the Rate knob value.
- **Scale CV**: added to the Scale knob value (0–10 V = 0–100%).

## Outputs
- **Out**: the random signal, scaled to the selected range.

## Notes
- All noise sources output values in the 0–1 range before scaling.
- The scale control affects amplitude only; the range switch sets the voltage ceiling.
- At scale = 0 the output is 0 V (or -5 V in ±5 V mode).
