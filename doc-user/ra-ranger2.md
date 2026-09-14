# ra-ranger2 — CV Range Rescaler

Rescales a CV input from an input range (floor to ceil) onto an output range (floor to ceil).

## Controls
- **Input floor**: floor of the input range (−5 to +10 V).
- **Input ceil**: ceil of the input range (−5 to +10 V).
- **Output floor**: floor of the output range (−5 to +10 V).
- **Output ceil**: ceil of the output range (−5 to +10 V).

## Inputs
- **Input**: the CV signal to be rescaled.
- **Input floor CV / Input ceil CV**: CV inputs that override the input range knobs when connected.
- **Output floor CV / Output ceil CV**: CV inputs that override the output range knobs when connected.

## Outputs
- **Output**: the input signal rescaled from the input range to the output range, clamped so it stays within the output floor/ceil even when the input goes outside the input range.