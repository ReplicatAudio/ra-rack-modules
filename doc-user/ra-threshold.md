# ra-threshold — 8-Channel CV Threshold Comparator

Eight independent threshold comparators. Each channel watches a 1 V/oct CV input and outputs a trigger when the CV crosses above a note threshold, plus a CV passthrough of the input.

## Controls
- **T n** knob: sets the threshold voltage for channel n (0–10 V, default 5 V). On the 1 V/oct scale, 0 V = C0, 1 V = C1, 2 V = C2, and so on.

## Inputs
- **CV n**: 1 V/oct CV input for channel n.

## Outputs
- **TR n**: 10 V gate output. Goes high when the CV input crosses above the threshold, and low when it drops below. A small hysteresis (10 mV) prevents chatter near the threshold.
- **P n**: CV passthrough — mirrors the input voltage.

## Notes
- All eight channels are independent.
- The trigger output is a gate (not a one-shot pulse): it stays high as long as the CV is above the threshold.
- The passthrough output is always active, regardless of trigger state.
