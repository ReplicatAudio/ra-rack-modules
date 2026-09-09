# ra-ar — 3hp Dual Attack/Release Envelope

3hp attack/release envelope generator with two independent channels, each with its own trigger input, attack/release knob set, and envelope output. Channel 1 IO is on top, channel 2 on the bottom.

## Controls
- **Attack 1 / Release 1**: channel 1 attack/release times (1 ms–10 s, logarithmic).
- **Attack 2 / Release 2**: channel 2 attack/release times (1 ms–10 s, logarithmic).

## Inputs
- **Trigger 1**: rising edge restarts channel 1's attack.
- **Trigger 2**: rising edge restarts channel 2's attack.
- Each channel is a full attack → release cycle, no sustain.

## Outputs
- **Envelope 1**: 0–10 V envelope for channel 1.
- **Envelope 2**: 0–10 V envelope for channel 2.