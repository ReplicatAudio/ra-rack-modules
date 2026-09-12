# ra-think — Simple VCO with Filter

VCO with a morphed waveform and a resonant lowpass filter.

## Controls
- **Frequency**: coarse pitch.
- **Fine tune**: fine pitch (in semitones).
- **Shape**: morphs the waveform.
- **Pulse width**: duty cycle of the pulse portion.
- **Cutoff**: lowpass filter cutoff.
- **Resonance**: filter resonance.
- **DC correction**: removes DC offset from the output.
- **Gain**: output level (0–100%).

## Inputs
- **1V/Oct**: exponential pitch CV.
- **Shape / Pulse width**: CV inputs modulate the corresponding control around its set point.
- **Cutoff / Resonance / Gain**: CV inputs are attenuated by the corresponding control (0–100 attenuation). When no CV is connected, the knob sets the parameter directly.

## Outputs
- **Audio**: filtered oscillator output.
