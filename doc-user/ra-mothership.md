# ra-mothership — 8-Voice LFO/VCO

An 8-oscillator LFO/VCO. Each of the eight voices sits on its own row with shape, phase, filter, FM, and detune control, plus an inverted-polarity switch. Global controls set the shared frequency, FM, and phase.

- **LFO** mode: 0.01 Hz – 10 Hz
- **VCO** mode: 20 Hz – 20 kHz

## Controls (global)
- **Mode**: LFO / VCO switch.
- **Frequency**: base frequency for all voices. When a CV is connected, the knob acts as an attenuator for the frequency CV.
- **Frequency CV attenuator**: scales the frequency CV (also voltage-controllable).
- **FM attenuation**: amount of global FM applied.
- **Phase**: global phase offset applied to all voices.

## Controls (per voice)
- **Shape**: morphs the waveform from saw up to square.
- **Phase**: voice phase offset.
- **Invert**: inverts the voice output.
- **Filter cutoff**: one-pole lowpass filter cutoff.
- **FM attenuation**: per-voice FM amount (when its CV is connected, the knob acts as an attenuator).
- **Detune**: per-voice detune, ±2 semitones. When its CV is connected, the knob acts as an attenuator.

## Inputs (global)
- **Frequency CV**: exponential frequency modulation.
- **Frequency CV attenuator**: modulates the frequency CV attenuation.
- **FM**: global exponential frequency modulation.
- **Phase CV**: modulates the global phase.

## Inputs (per voice)
- **Shape CV**: morphs the waveform (the knob acts as an attenuator when connected).
- **Phase CV**: modulates the voice phase.
- **Filter CV**: controls the filter cutoff (the knob acts as an attenuator when connected).
- **FM CV**: per-voice exponential frequency modulation.
- **Detune CV**: controls detune (the knob acts as an attenuator when connected).

## Outputs
- **8× Oscillator**: one output per voice, ±5 V.