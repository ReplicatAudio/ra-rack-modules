# ra-resampler — Polyphonic Sample Player

A polyphonic sample player that loads a single WAV sample and pitch-shifts it 1 V/oct. Up to 16 voices.

## Controls
- **Load** button: opens the file browser to load a WAV sample.
- **Speed** knob + **Spd CV**: playback speed multiplier (0.1–10×, default 1×). CV adds up to 9×.
- **Position** knob + **Pos CV**: start position within the sample (0–100%, default 0%).
- **Level** knob + **Lvl CV**: output level (0–100%, default 100%).
- **Loop** button: toggles between one-shot and looped playback.

## Inputs
- **1V/Oct** (polyphonic): pitch CV. Shifts playback rate by 2^(volts) — 0 V = original pitch, 1 V = one octave up, −1 V = one octave down.
- **Gate** (polyphonic): triggers playback on rising edge. In loop mode, playback continues while gate is high.

## Outputs
- **Audio** (polyphonic): one channel per voice.

## Behavior
- **One-shot mode** (loop off): gate high triggers playback from the position, sample plays to end.
- **Loop mode** (loop on): sample loops while gate is high, stops when gate goes low.
- Fractional interpolation for smooth pitch shifting at non-integer rates.

## Notes
- The sample is loaded from a 16-bit PCM WAV file (mono or stereo, any sample rate).
- All voices share the same sample but have independent playback positions.
- The output is polyphonic — connect a polyphonic gate or pitch CV to play multiple voices.
