# ra-delay-cv — CV/Trigger Delay

DC-coupled delay line designed for gates, triggers, and control voltage. Nothing in the signal path filters or resamples the delayed signal: edges stay sharp, gate widths are preserved exactly, and sustained CV holds its level. Layout and controls follow the VCV Delay.

## Controls
- **Time**: delay time, 1 ms – 10 s (logarithmic).
- **Feedback**: amount of delayed signal fed back into the input.
- **Time CV / FB CV**: attenuverters for the corresponding CV inputs.

## Inputs
- **In**: signal to delay. Passes gates, triggers, and CV (including steady offsets) through unchanged.
- **Clock**: syncs delay time to a clock — Time becomes a ratio of the half clock period, as on VCV Delay.
- **Time**: 1 V/octave delay-time modulation when Time CV is 100%.
- **Feedback**: CV for the Feedback parameter.

## Outputs
- **Wet**: delayed signal only. Use this for triggers and gates.
- **Echo**: input plus delayed signal, both at full amplitude — a proper echo tap that produces two clean pulses from one input trigger.

## Notes
- The clock LED flashes at the current internal delay rate.
- Feedback is unfiltered; at high settings a repeated gate will sustain rather than decay away like in an audio delay.
