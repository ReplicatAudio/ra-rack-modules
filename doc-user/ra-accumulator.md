# ra-accumulator — CV Accumulator

Accumulates a delta value onto an origin whenever a write trigger fires, producing a CV output with optional slew.

## Controls
- **Origin**: base voltage (−10 to +10 V).
- **Delta**: amount added on each write.
- **Slew**: portamento time applied to the output (0–10 s).
- **Write**: button to add the delta.
- **Reset**: button to return the accumulated value to zero.

## Inputs
- **Origin CV / Delta CV / Slew CV**: CV inputs that override the corresponding knobs when connected.
- **Write trig / Reset trig**: trigger inputs for write and reset.

## Outputs
- **CV**: `origin + accumulated delta`, optionally slewed.
