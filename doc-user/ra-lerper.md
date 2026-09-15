# ra-lerper — Target Lerper

Lerps the CV output toward one of four target values (A, B, C, D) at a rate set by T, or toward a position on an X/Y interpolation matrix. A square screen in the center shows a dot representing the current output position between the four corner targets; the active target is highlighted.

## Controls
- **A / B / C / D**: target values (−10 to +10 V).
- **T**: time to reach a target (0–10 s).
- **A tr / B tr / C tr / D tr**: buttons that begin lerping the output toward that target.
- **MODE** (switch, left of the output): selects the lerp behavior:
  - **target**: lerps the output value toward the target value.
  - **mix**: lerps toward a target mix weight.

## Inputs
- **A CV / B CV / C CV / D CV**: CV inputs that override the corresponding target knobs when connected.
- **T CV**: CV input that overrides the T knob when connected (0–10 s).
- **X / Y**: CV inputs (0–10 V) that define a position on the A–D interpolation matrix. When either is patched, that position becomes the active lerp target and the output is the bilinear interpolation of the four corners at that point.
- **A tr / B tr / C tr / D tr (CV)**: trigger inputs that begin lerping toward that target (used when no X/Y target is active).

## Outputs
- **CV**: the lerped output voltage.
- **OUT** (switch, right of the output): selects the output voltage range:
  - **0–10V**: internal ±10 V mapped onto 0–10 V.
  - **±5V**: internal ±10 V mapped onto −5…+5 V.
  - **0–1V**: internal ±10 V mapped onto 0–1 V.

The A/B/C/D knob hover tooltips show their value scaled to the currently selected output mode, matching the CV output.

## Screen
- The square screen has a point at each corner representing the four targets (A top-left, B top-right, C bottom-left, D bottom-right).
- A dot shows the current output position, gliding toward the active target at rate T.
- When X or Y is patched, a **lighter dot** marks the interpolation target and drives the output; that target becomes active, no corner is highlighted, and its trigger lights stay off.
- The active corner's point and label are highlighted, and its trigger button lights up.