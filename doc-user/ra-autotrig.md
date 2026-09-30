# ra-autotrig — 8-Channel CV Delta Trigger

Eight independent delta triggers. Each channel watches a CV input and outputs a trigger whenever the CV changes by a set amount, plus a CV passthrough of the input.

## Controls
- **D n** knob: sets the delta threshold for channel n (0.01–10 V, default 1 V). A trigger is output each time the CV changes by this amount.

## Inputs
- **CV n**: CV input for channel n.

## Outputs
- **TR n**: 10 V trigger output. Fires a one-sample pulse each time the input CV changes by the delta amount (in either direction).
- **P n**: CV passthrough — mirrors the input voltage.

## Notes
- All eight channels are independent.
- The trigger fires on both rising and falling changes — any change of at least the delta amount produces a trigger.
- The passthrough output is always active, regardless of trigger state.
- This effectively quantizes the input signal to the delta step size and outputs a trigger on each step.
