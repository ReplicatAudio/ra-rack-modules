# ra-ntet — N-TET CV Processor

Eight-channel utility for working with arbitrary equal-temperament tunings. Two modes handle pre-quantized chromatic CV and unquantized CV.

## Controls
- **Scale**: selects the target equal temperament (10 snapped positions, e.g. 12, 24, 36…‑TET).
- **Smash**: switches to a sub-12 scale series (12, 10, 9, 8…2‑TET).
- **Mode**: Chromatic (scales pre-quantized 12‑TET semitones to the target N‑TET) or Quant (snaps CV to the nearest N‑TET step).

## Inputs
- **Input 1–8**: pitch CV (1 V/oct). Unpatched inputs read 0 V.

## Outputs
- **Output 1–8**: processed pitch CV.
