# ra-lfo — Voltage-Controlled LFO

Low-frequency oscillator producing sine, triangle, sawtooth, and square outputs with linear FM and pulse-width modulation. Can be clock-synced from an external clock, and outputs are polyphonic (frequency follows the FM input's channel count).

## Controls
- **Freq**: base frequency, −3 octaves to +3⅓ octaves relative to 1 Hz, e.g. 125 mHz–10.08 Hz.
- **Pulse**: pulse width of the square output (1–99%).
- **Phase**: phase offset (0–100% of a full cycle), applied to all outputs. When a Phase CV input is connected it becomes a 0–1 attenuator for that CV.
- **FM**: amount of linear frequency modulation from the FM input.
- **Invert**: inverts all outputs (button).
- **Offset**: bipolar (±5 V) or unipolar (0–10 V) output (button).
- **PWM**: amount of pulse-width modulation from the PWM input.

## Inputs
- **FM**: linear frequency modulation (1 V/oct).
- **Clock**: external clock — the LFO runs at the clock's frequency. When unpatched it free-runs at the Freq knob.
- **Reset**: resets all phases to 0.
- **PWM**: pulse-width modulation CV.
- **Phase CV**: phase-offset CV (0–10 V maps to a full cycle); the Phase knob attenuates it when connected.
- **Inv G**: while the gate is high, inverts the Invert button's current state (off↔on); reverts when low.
- **Off G**: while the gate is high, inverts the Offset button's current state (off↔on); reverts when low.

## Outputs
- **Sine**, **Tri**, **Saw**, **Sqr**: the four waveforms, ±5 V bipolar or 0–10 V unipolar depending on the Offset button.

A screen at the top of the panel shows the current phase position as a small purple dot.</｜DSML｜parameter