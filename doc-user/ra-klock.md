# ra-klock — Master Clock

Master clock generator with BPM, swing, run/reset, a metronome-style grandfather clock screen, one main CLK output, and 6 knob-controlled ratio outputs.

## Display
Metronome-style grandfather clock animation:
- **Dial**: hands sweep with elapsed musical time (second hand once per beat, minute hand per 60 beats, hour hand per 720 beats).
- **Pendulum**: swings back and forth once per beat while running (beat at the extremes, tick-tock style).
- **Readout**: `BPM` (left) and `SWG` (right) with the tempo and swing values. While dragging a ratio knob, the left readout switches to `OUT n` + the chosen ratio (e.g. `x4` or `/3`).
- While the BPM, Swing, or a ratio knob is being dragged, its value is shown at full brightness with no beat pulsing; when released it returns to the normal beat-pulsed glow.

## Controls
- **BPM**: clock tempo (0–333 BPM, V/oct controllable). Knob is continuously adjustable.
- **Swing**: shuffle amount — delays the offbeat (odd) sub-beat pulses of **multiplier outputs only** (x2–x32). x1 and all divisions are unaffected.
- **Run**: start/stop button.
- **Reset**: reset button.

## Inputs
- **v/oct**: exponential tempo CV (1 V doubles the BPM).
- **Swing CV**: modulates swing (applies to multipliers as above).
- **Run**: gate toggles running state.
- **Reset**: trigger resets the clock (and all ratio outputs).

## Outputs
- **CLK**: main quarter-note clock pulse.
- **Out 1–6**: six ratio outputs, each set by its knob across `/32 /16 /12 /8 /6 /5 /4 /3 /2 x1 x2 x3 x4 x5 x6 x8 x12 x16 x24 x32`.
  - Knobs snap discretely between the 20 ratios; the hover tooltip shows the snapped ratio.
  - Multipliers (x2–x32) fire on the swung sub-beat grid — the odd pulse of each pair is delayed by the swing amount.
  - Divisions (/2–/32) fire straight, once per N beats.
  - x1 passes the beat through straight, in phase with CLK.