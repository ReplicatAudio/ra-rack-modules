# ra-hat — 808 Style Hi-Hat

Classic 808-style hi-hat made from six inharmonic square-wave partials with per-partial decays and a brightness highpass. The decay range spans closed to open hits.

## Controls
- **1V/Oct**: pitch knob — a semitone knob (±24 st from A4) that tunes the metallic partials.
- **FM attn**: attenuverter (bipolar) scaling the dedicated FM input.
- **Tone**: ratio multiplier of the metallic partials.
- **Metallic**: how many of the 6 inharmonic partials are active (2–6).
- **Decay**: length of the ring (20 ms–600 ms, closed to open).
- **Snap**: how much faster the higher partials decay (attack shimmer).
- **Body**: level of the metallic signal.
- **Bright**: highpass cutoff (dark to bright).
- **Drive**: tanh saturation amount.
- **Accent**: extra level boost applied to hits that arrive while the Accent input is high (up to +6 dB).
- **Level**: output level (0–100%).

## Inputs
- **Trigger**: fires a hit on each rising edge.
- **1V/Oct**: dedicated standard 1V/oct pitch CV.
- **FM**: dedicated frequency-modulation CV, scaled by the FM attn knob.
- **Tone / Metallic / Decay / Snap / Body / Bright / Drive / Level CV**: bipolar CV (±10 V = full range) added to the matching knob.
- **Accent**: gate that boosts the level of triggers that occur while high.

## Outputs
- **Audio**: the hi-hat output.