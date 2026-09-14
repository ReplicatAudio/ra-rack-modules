# ra-freeberd — Freeverb Reverb

Classic Freeverb (Jezar) stereo reverb — 8 comb filters and 4 allpass filters per channel, with smoothed room size, damping, and dry/wet control.

## Controls
- **Room size**: reverb room size / feedback amount (0–100%).
- **Damping**: high-frequency damping of the reverb tail (0–100%).
- **Dry/wet**: balance between the dry signal and the reverb (0–100%).

## Inputs
- **Left / Right**: stereo signal to process. If Right is unpatched, the Left input is used for both channels.
- **Room size CV**: CV added to the room size knob (0–10 V spans the full range).
- **Damping CV**: CV added to the damping knob (0–10 V spans the full range).

## Outputs
- **Left / Right**: the processed stereo output, mixing the dry and wet signals per the dry/wet knob.