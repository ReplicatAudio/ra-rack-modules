# ra-meteor — 808 Style Metallic Percussion

One metallic percussion engine covering the hi-hat, crash, and ride cymbal range: inharmonic band-limited square partials mixed with a noise body, shaped by a highpass tone control, plus the ride's long-ringing bell ping. The Decay knob spans 20 ms (tight closed hat) to 4 s (long crash/ride wash); 2–6 shimmer partials de-synchronize on every hit so repeats aren't identical.

## Controls
- **1V/Oct**: pitch knob — a semitone knob (±24 st from A4) that scales the entire partial stack and the noise-body tone filter.
- **FM attn**: attenuverter (bipolar) scaling the dedicated FM input.
- **Tone**: partial-ratio multiplier for the metallic shimmer.
- **Metallic**: number/strength of the active shimmer partials (2–6).
- **Decay**: length of the wash (20 ms–4 s, covering closed hat through long ride).
- **Snap**: how much faster the higher partials decay than the fundamental (attack "snappiness").
- **Body**: crossfades between the pure noise body (1) and the metallic shimmer (0), 808-style.
- **Bright**: highpass cutoff of the tone filter.
- **Bell**: level of the ride-style bell "ping" — a pure sine at ~9.5× the pitch that rings 1.5× longer than the body (0 = off).
- **Drive**: tanh saturation/punch amount (clean to hard-clipped).
- **Accent**: extra level boost applied to hits that arrive while the Accent input is high (up to +6 dB).
- **Level**: output level (0–100%).

## Inputs
- **Trigger**: fires a hit on each rising edge.
- **1V/Oct**: dedicated standard 1V/oct pitch CV for the 1V/Oct knob.
- **FM**: dedicated frequency-modulation CV, scaled by the FM attn knob (modulates the 1V/Oct pitch).
- **Tone / Metallic / Decay / Snap / Body / Bright / Bell / Drive / Level CV**: bipolar CV (±10 V = full range) added to the matching knob.
- **Accent**: gate that boosts the level of triggers that occur while high.

## Outputs
- **Audio**: the metallic percussion output.