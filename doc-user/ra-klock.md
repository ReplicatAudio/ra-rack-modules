# ra-klock — Master Clock

Master clock generator with BPM, swing, run/reset and multiple multiplication/division clock outputs.

## Controls
- **BPM**: clock tempo (0–333 BPM, V/oct controllable).
- **Swing**: shuffle amount.
- **Run**: start/stop button.
- **Reset**: reset button.

## Inputs
- **v/oct**: exponential tempo CV (1 V doubles the BPM).
- **Swing CV**: modulates swing.
- **Run**: gate toggles running state.
- **Reset**: trigger resets the clock.

## Outputs
- **CLK**: quarter-note clock pulse.
- **Run**: gate high while running.
- **Reset**: pulse on reset.
- **x2 / x4 / x8 / x16**: sub-beat clock multiples.
- **/2 / /4 / /8 / /16 / /32**: beat divisions.
