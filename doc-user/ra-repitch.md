# ra-repitch — Phase Vocoder Pitch Shifter

A phase vocoder pitch shifter that repitches incoming audio in real time.

## Controls
- **Oct** knob + **Oct CV**: octave shift (-4 to +4, default 0).
- **Semi** knob + **Semi CV**: semitone shift (-12 to +12, default 0).
- **Cent** knob + **Cent CV**: cent shift (-100 to +100, default 0).
- **Mix** knob + **Mix CV**: dry/wet mix (0–100%, default 100%).

## Inputs
- **In**: audio input to pitch shift.

## Outputs
- **Out**: pitch-shifted audio output.

## Notes
- The total pitch shift is calculated as 2^(octave + semitone/12 + cent/1200).
- The mix control blends between the dry (original) and wet (pitch-shifted) signal.
- Uses a phase vocoder algorithm with 2048-sample FFT and 75% overlap for high-quality pitch shifting.
- All CV inputs are added to their corresponding knob values.
