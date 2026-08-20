# ra-ranger — Signal Scaler / Waveshaper

Four-channel processor that scales a signal and applies clipping or wave folding.

## Controls
- **Scale amount**: input gain/attenuation.
- **Clip**: clip threshold (0–100% of full scale).
- **Power scale**: when on, applies an exponential power curve instead of linear scaling.
- **Clip mode**: Hard (clamp), Soft (tanh-like), Fold (wave folding).
- **Range**: output range 0–10 V, ±5 V, or 0–1 V.

## Inputs
- **Scale amount**: CV modulating the scale amount.
- **Clip**: CV modulating the clip threshold.
- **Input 1–4**: signals to process.

## Outputs
- **Output 1–4**: processed signals.
