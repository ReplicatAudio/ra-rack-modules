# ra-lerper — Target Lerper

Lerps the CV output toward one of four target values (A, B, C, D) at a rate set by T. A square screen in the center shows a dot representing the current output position between the four corner targets; the active target's corner is highlighted.

## Controls
- **A / B / C / D**: target values (−10 to +10 V).
- **T**: time to reach a target (0–10 s).
- **A tr / B tr / C tr / D tr**: buttons that begin lerping the output toward that target.

## Inputs
- **A CV / B CV / C CV / D CV**: CV inputs that override the corresponding target knobs when connected.
- **T CV**: CV input that overrides the T knob when connected (0–10 s).
- **A tr / B tr / C tr / D tr (CV)**: trigger inputs that begin lerping toward that target.

## Outputs
- **CV**: the lerped output voltage.

## Screen
- The square screen has a point at each corner representing the four targets (A top-left, B top-right, C bottom-left, D bottom-right).
- A dot shows the current output position, gliding toward the active target's corner at rate T.
- The active target's corner point and label are highlighted, and its trigger button lights up.