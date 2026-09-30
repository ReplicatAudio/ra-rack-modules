# ra-gear — Gear Module

A visual gear counter that increments or decrements through a configurable number of teeth, with a trigger output on cycle complete and a CV position output.

## Controls
- **Teeth** knob: sets the number of gear teeth (3–32, default 8). This is the number of ticks needed to complete one full rotation.
- **Inc** button + **Inc trig** input: moves the gear forward one tooth.
- **Dec** button + **Dec trig** input: moves the gear backward one tooth.
- **Range** switch: selects the CV output voltage range.
  - **0-1V** — unipolar, 0 to 1 V.
  - **0-10V** — unipolar, 0 to 10 V.
  - **±5V** — bipolar, -5 to 5 V.

## Outputs
- **Trig**: 10 V trigger pulse when the gear completes a full cycle (wraps past the last tooth or before the first).
- **CV**: position output as a percentage of the full rotation. 0 teeth = 0 V (or -5 V in ±5 V mode), full rotation = maximum of the selected range.

## Display
A screen at the top shows the gear with the correct number of teeth and its current orientation. The gear rotates as you increment or decrement.

## Notes
- The gear wraps around: incrementing past the last tooth returns to the first, and decrementing before the first tooth wraps to the last.
- The CV output is continuous — it reflects the exact position within the current tooth step.
- The trigger output is a single-sample pulse on cycle completion.
