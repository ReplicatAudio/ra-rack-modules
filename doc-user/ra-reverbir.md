# ra-reverbir — Stereo Convolution Reverb

A stereo convolution reverb that applies a user-loaded WAV impulse response to the input signal.

## Controls
- **Predelay**: delay before the reverb starts (0–500 ms).
- **Length**: length of the reverb tail (0–100%).
- **Damp**: damping factor for the reverb tail (0–100%).
- **Attack**: attack time of the reverb envelope (0–100%).
- **Decay**: decay time of the reverb envelope (0–100%).

## Inputs
- **In L**: left channel input.
- **In R**: right channel input.

## Outputs
- **Out L**: left channel output.
- **Out R**: right channel output.

## Notes
- Uses uniformly partitioned convolution for efficient real-time processing.
- The impulse response is loaded from a WAV file.
- The reverb tail can be adjusted for length and damping.
- The envelope controls shape the attack and decay of the reverb.
