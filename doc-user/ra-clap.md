# ra-clap — 808 Style Clap

Classic 808-style handclap made from white-noise bursts through a resonant bandpass with multiple retrigger taps and a low thump underneath, plus the standard drive/accent/level controls.

## Controls
- **1V/Oct**: pitch knob — a semitone knob (±24 st from C3) that tunes the noise bandpass and thump.
- **FM attn**: attenuverter (bipolar) scaling the dedicated FM input.
- **Q**: bandpass resonance (bandwidth) of the noise filter.
- **Taps**: number of retrigger bursts per hit (1–4, ~12 ms apart).
- **Pitch drop**: how far the thump sweeps down on each hit.
- **Pitch drop time**: how fast that sweep happens (2–150 ms).
- **Thump**: length of the thump tail (20 ms–1 s).
- **Snap**: decay time of each noise burst (10–300 ms).
- **Drive**: tanh saturation amount.
- **Accent**: extra level boost applied to hits that arrive while the Accent input is high (up to +6 dB).
- **Level**: output level (0–100%).

## Inputs
- **Trigger**: fires a hit on each rising edge.
- **1V/Oct**: dedicated standard 1V/oct pitch CV.
- **FM**: dedicated frequency-modulation CV, scaled by the FM attn knob.
- **Q / Taps / Pitch drop / Pitch drop time / Thump / Snap / Drive / Level CV**: bipolar CV (±10 V = full range) added to the matching knob.
- **Accent**: gate that boosts the level of triggers that occur while high.

## Outputs
- **Audio**: the clap output.