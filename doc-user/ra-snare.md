# ra-snare — 808 Style Snare Drum

Classic 808-style snare drum made from a pitch-swept triangle-wave tone mixed with a highpassed white-noise rattle, each with independent decays, plus drive, accent, and level controls.

## Controls
- **1V/Oct**: pitch knob — a semitone knob (±24 st from C3) that sets the base frequency of the tonal body.
- **FM attn**: attenuverter (bipolar) scaling the dedicated FM input.
- **Body**: amount of the tonal (oscillator) body in the hit.
- **Noise**: amount of the white-noise rattle (fixed ~2 kHz highpass).
- **Decay**: length of the tonal body envelope (20 ms–1 s).
- **Snap**: decay time of the noise rattle (5–300 ms).
- **Pitch drop**: how far the tone sweeps down on each hit (up to 3×).
- **Pitch drop time**: how fast that sweep happens (2–150 ms).
- **Drive**: tanh saturation/punch amount.
- **Accent**: extra level boost applied to hits that arrive while the Accent input is high (up to +6 dB).
- **Level**: output level (0–100%).

## Inputs
- **Trigger**: fires a hit on each rising edge.
- **1V/Oct**: dedicated standard 1V/oct pitch CV for the pitch knob.
- **FM**: dedicated frequency-modulation CV, scaled by the FM attn knob.
- **Body / Noise / Decay / Snap / Pitch drop / Pitch drop time / Drive / Level CV**: bipolar CV (±10 V = full range) added to the matching knob.
- **Accent**: gate that boosts the level of triggers that occur while high.

## Outputs
- **Audio**: the snare drum output.
