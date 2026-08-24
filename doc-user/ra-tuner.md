# ra-tuner — Tuner

Shows the frequency and western note notation of the input signal.

## Controls
- **Mode**: switch between `1V/oct` and `Audio` interpretation of the input.

## Inputs
- **Input**: signal to measure. In `1V/oct` mode it is read as pitch CV (0 V = C4); in `Audio` mode the fundamental frequency is detected from the waveform via zero-crossing measurement.

The display shows the note name (e.g. `A4`, `C#3`), the measured frequency, and the active mode. When no valid pitch is available it shows `--` with `NO SIGNAL` (audio) or `NO CABLE` (1V/oct). Only the first channel of the input is measured.