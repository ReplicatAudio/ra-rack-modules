# ra-kick — 808 Style Kick Drum

Classic 808-style kick drum synthesised from a pitch-swept sine wave with an exponential decay envelope, plus a tunable transient click and sub oscillator, with CV control over most parameters.

## Controls
- **1V/Oct**: pitch knob — a semitone knob (±24 st from C2) that sets the base frequency the kick settles on.
- **FM attn**: attenuverter (bipolar) scaling the dedicated FM input.
- **Pitch drop**: how far the pitch sweeps down on each hit (1×–8× the tone frequency).
- **Pitch drop time**: how fast that sweep happens (2–150 ms), independent of the body decay.
- **Decay**: length of the body envelope (20 ms–1 s).
- **Sub level**: amount of a pure sine one octave below the tone, mixed under the body.
- **Click**: amount of the short transient "tick" at the start of the hit.
- **Click tone**: frequency of the click transient (200–2000 Hz).
- **Drive**: tanh saturation/punch amount (clean to hard-clipped).
- **Accent**: extra level boost applied to hits that arrive while the Accent input is high (up to +6 dB).
- **Level**: output level (0–100%).

## Inputs
- **Trigger**: fires a hit on each rising edge.
- **1V/Oct**: dedicated standard 1V/oct pitch CV for the 1V/Oct knob.
- **FM**: dedicated frequency-modulation CV, scaled by the FM attn knob (modulates the 1V/Oct pitch).
- **Pitch drop / Pitch drop time / Decay / Sub level / Click / Click tone / Drive / Level CV**: bipolar CV (±10 V = full range) added to the matching knob.
- **Accent**: gate that boosts the level of triggers that occur while high.

## Outputs
- **Audio**: the kick drum output.
