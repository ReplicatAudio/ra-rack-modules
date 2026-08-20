# ra-glitch — Random Glitch Processor

Randomly disrupts an audio signal with CV-controllable frequency and length.

## Controls
- **Frequency**: how often glitches occur (0–100%).
- **Length**: duration of each glitch.
- **Mode**: Freeze (loop a short captured segment), Swap (swap the audio/swap inputs), Drop (silence).

## Inputs
- **Audio**: signal to process.
- **Swap**: second signal used in swap mode.
- **Frequency CV**: modulates glitch frequency.
- **Length CV**: modulates glitch length.

## Outputs
- **Audio**: processed output.
- **Swap**: second processed output.
