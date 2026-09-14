# ra-mix4 — 4-Channel Stereo Mixer

Simple 4-channel stereo mixer with a gain slider and CV per channel, a pan knob and CV per channel, plus master gain and pan with a stereo output.

## Controls
- **Gain 1–4**: per-channel gain (0–150%, default 100%).
- **Pan 1–4**: per-channel pan (left to right).
- **Master**: master output gain (0–100%).
- **Pan**: master stereo pan.

## Inputs
- **In 1–4**: the four channel signals.
- **CV G1–G4**: CV for each channel's gain. When connected, the gain slider acts as a 0–100% attenuator of the CV gain (0–10 V = full gain).
- **CV GM**: CV for the master gain (slider acts as a 0–100% attenuator).
- **CV P1–P4**: CV added to each channel's pan.
- **CV PM**: CV added to the master pan.

## Outputs
- **Left / Right**: the stereo mix. If only the Left output is connected, the mono sum of the channels is sent instead.