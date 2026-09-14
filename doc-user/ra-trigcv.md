# ra-trigcv — Trigger to CV Mapper

Eight trigger inputs select among eight stored values, each set by a knob or CV, and output the currently selected value as CV with optional slew. The active group is the highest-numbered trigger that is currently high; it is held until another trigger fires.

## Controls
- **Knob 1–8**: per-group value (0–1) as a proportion of the output range.
- **Slew**: slew limiting (exponential smoothing) applied to the output; 0 snaps instantly.
- **Mode**: output range — 0–10 V, ±5 V, or 0–1 V.

## Inputs
- **Trigger 1–8**: when high (over 1 V), selects that group as the active output.
- **CV 1–8**: per-group CV value. When connected, the CV controls the value and the knob acts as a 0–1 attenuator (0–10 V = full value).

## Outputs
- **Output**: the active group's value, slewed, mapped to the mode's output range. If no trigger is high, the last output is held.