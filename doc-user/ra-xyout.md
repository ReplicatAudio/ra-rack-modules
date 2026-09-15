# ra-xyout — XY Pad Dual CV Output

Outputs two CV signals (X and Y) from a position on a square XY pad. Clicking (or dragging) on the pad sets the target position; the outputs glide toward it at a rate set by LERP. A square screen in the center shows a target marker and a dot tracking the lerped position.

## Controls
- **LERP**: lerp speed (0–10, default 1). Higher values slew the outputs toward the clicked position faster; zero jumps instantly.
- **RANGE** (switch, center): selects the output voltage range for both outputs:
  - **0–10V**
  - **±5V**
  - **0–1V**

## Inputs
- **LERP CV**: CV input that overrides the LERP knob when connected (0–10 V).

## Outputs
- **X**: the horizontal position of the pad, mapped to the selected range.
- **Y**: the vertical position of the pad, mapped to the selected range.

## Pad
- The square pad shows a faint grid, a **target marker** where you last clicked, and a **dot** that glides toward it at the LERP rate.
- Click (or click and drag) anywhere on the pad to place the target; X and Y follow.