# ra-tom — 808 Style Tom

Classic 808-style tom made from a pitch-swept sine body with an optional harmonic brightness and a short beater snap, plus the standard drive/accent/level controls.

## Controls
- **1V/Oct**: pitch knob — a semitone knob (±24 st from G2) that sets the base frequency.
- **FM attn**: attenuverter (bipolar) scaling the dedicated FM input.
- **Tone**: harmonic brightness of the body (mixes in a 2nd harmonic).
- **Body**: level of the tonal body.
- **Pitch drop**: how far the body sweeps down on each hit (up to 3×).
- **Pitch drop time**: how fast that sweep happens (2–150 ms).
- **Decay**: length of the body envelope (20 ms–1 s).
- **Snap**: amount of the short beater transient at the start of the hit.
- **Drive**: tanh saturation/punch amount.
- **Accent**: extra level boost applied to hits that arrive while the Accent input is high (up to +6 dB).
- **Level**: output level (0–100%).

## Inputs
- **Trigger**: fires a hit on each rising edge.
- **1V/Oct**: dedicated standard 1V/oct pitch CV.
- **FM**: dedicated frequency-modulation CV, scaled by the FM attn knob.
- **Tone / Body / Pitch drop / Pitch drop time / Decay / Snap / Drive / Level CV**: bipolar CV (±10 V = full range) added to the matching knob.
- **Accent**: gate that boosts the level of triggers that occur while high.

## Outputs
- **Audio**: the tom output.