# ra-krush — Krush / Downsampler Bitcrusher

Bitcrusher/downsampler ported from the x-bitcrusher DSP chain: crush mapping through an anti-alias biquad, downsampler, and quantizer, with slew limiting and an optional frequency split that crushes the low band while passing the high band clean. Fully polyphonic — one DSP state per voice, or sum-to-mono via the context menu.

## Controls
- **Krush**: crush amount (0–100%). Higher values reduce bit depth and sample rate.
- **Krush algorithm**: selects the algorithm — Standard, Jitter, Random, Cubic, Bitcrush, Downsample, Corrupt, or Noise-shaped.
- **Slew**: slew/glide limiting of the crushed output (0–100%).
- **Split cutoff**: frequency-split cutoff (20 Hz–20 kHz, logarithmic).
- **Split resonance**: resonance (Q) of the split filters (0.1–2.0).
- **Split mode**: ON crushes only the low band, passing the high band clean; OFF crushes the full signal.
- **Post-crush filter**: ON applies a lowpass to the crushed low band (split mode only).
- **Anti-alias**: ON applies the anti-alias pre-filter before downsampling.
- **Quantize**: ON forces integer bit-depth and downsample steps.

## Inputs
- **Audio**: the signal to crush.
- **Krush CV**: CV added to the crush knob (0–10 V spans the full range).
- **Split cutoff CV (1V/Oct)**: 1 V-per-octave cutoff tracking.
- **Slew CV**: CV added to the slew knob.

## Outputs
- **Audio**: the combined crushed output.
- **Crushed low band**: the crushed low band (or full crushed signal when split mode is off).
- **Clean high band**: the unprocessed high band (split mode only; 0 otherwise).