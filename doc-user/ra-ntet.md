# ra-ntet — N‑TET CV Processor

> **Design note:** The module has no offset, transpose, or post‑gain control. In chromatic mode any offset is incorrect — dividing the input by *s* implicitly adds offset when you dial it away from zero, and a second offset would make 0 V in ≠ 0 V out. In quant mode there’s nothing to gain: pitch offsets are trivial to add with a mixer or precision adder if needed, and keeping the module pure avoids confusion about whether offset is pre‑ or post‑quantize.

4 HP, 8‑channel utility for working with arbitrary equal‑temperament tuning systems. Two modes cover both pre‑quantized chromatic CV and unquantized CV.

## Panel Layout

```
┌──────────────────────┐
│[]                    │
│                      │
│          ●           │  y=25   Scale knob
│                      │
│    ┌──┐    ┌──┐      │  y=55   Smash | Mode
│    │  │    │  │      │
│                      │
│ ○                ○   │  y=82   IN 1 / OUT 1
│ ○                ○   │  y=108  IN 2 / OUT 2
│ ○                ○   │  y=134  IN 3 / OUT 3
│ ○                ○   │  y=160  IN 4 / OUT 4
│ ○                ○   │  y=186  IN 5 / OUT 5
│ ○                ○   │  y=212  IN 6 / OUT 6
│ ○                ○   │  y=238  IN 7 / OUT 7
│ ○                ○   │  y=264  IN 8 / OUT 8
│[]                    │
└──────────────────────┘
    16   30   44
```

● = knob, ○ = port, ┌──┐ = toggle switch, [] = screw

## Controls

### Scale Knob

10 snap positions selecting the target equal‑temperament. The tooltip updates to show the current mapping:

| Position | Smash Off  | Smash On |
|----------|------------|----------|
| 1        | 12‑TET     | 12‑TET   |
| 2        | 24‑TET     | 10‑TET   |
| 3        | 36‑TET     | 9‑TET    |
| 4        | 48‑TET     | 8‑TET    |
| 5        | 60‑TET     | 7‑TET    |
| 6        | 72‑TET     | 6‑TET    |
| 7        | 84‑TET     | 5‑TET    |
| 8        | 96‑TET     | 4‑TET    |
| 9        | 108‑TET    | 3‑TET    |
| 10       | 120‑TET    | 2‑TET    |

### Smash Toggle

Switches the scale mapping from the standard multiplication table (12, 24, 36, …) to the sub‑12 series (12, 10, 9, 8, …, 2). See table above.

### Mode Toggle

Selects between two processing algorithms:

**Chromatic** — for CV that is already quantised to 12‑TET semitones (e.g. MIDI→CV, chromatic sequencer). The module linearly scales the voltage so each input semitone maps to one step of the target N‑TET.

- Smash Off: `out = in / s`
- Smash On: `out = in × (12 / div)`

| Example | CV step size | Behaviour |
|---------|-------------|-----------|
| Chromatic, s=2 (24‑TET) | 1 semitone = ¹⁄₁₂ V → ¹⁄₂₄ V | 12‑TET in → 24‑TET out |
| Chromatic, smash s=2 (10‑TET) | 1 semitone = ¹⁄₁₂ V → ¹⁄₁₀ V | 12‑TET in → 10‑TET out |

**Quant** — for unquantised CV (LFO, envelope, unquantised sequencer). The module snaps the voltage to the nearest step of the target N‑TET grid.

- Both smash states: `out = round(in / quanta) × quanta` where `quanta = 1 / divisions`

| Example | Behaviour |
|---------|-----------|
| Quant, s=1 (12‑TET) | Snaps to the nearest semitone |
| Quant, s=10 (120‑TET) | Snaps to the nearest ¹⁄₁₂₀ octave (10‑cent resolution) |
| Quant, smash s=3 (9‑TET) | Snaps to the nearest ¹⁄₉ octave step |

## Inputs / Outputs

8 inputs (left column) and 8 outputs (right column), 1 V/oct.

Unpatched inputs read 0 V.

## Patch Examples

### Map a chromatic sequencer to 24‑TET

```
Chromatic sequencer → IN 1
OUT 1 → 24‑TET oscillator pitch
Scale = 2 (24‑TET), Mode = Chromatic, Smash = Off
```

Each 12‑TET semitone from the sequencer becomes one 24‑TET microtone at the output.

### Quantise an LFO to 7‑TET

```
LFO → IN 1
OUT 1 → oscillator pitch
Scale = 5 (7‑TET), Mode = Quant, Smash = On
```

The LFO voltage is snapped to the nearest ¹⁄₇ octave step.

### Extreme microtonal stretch from a keyboard

```
MIDI→CV → IN 1–4
OUT 1–4 → four oscillators
Scale = 10 (120‑TET), Mode = Chromatic, Smash = Off
```

A 1‑octave keyboard span now covers 120 equal divisions. Each semitone key produces a ~10‑cent increment.
