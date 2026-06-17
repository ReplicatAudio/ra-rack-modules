# ra-knob — Technical Reference

## Overview

ra-knob is a 4‑channel CV source module for VCV Rack v2.  
Each channel produces a DC voltage controlled by three parameters (Macro, Range, Scale) and
indicates the output level with an RGB LED.

Channel layout is a single vertical column in a 4 hp panel (20.32 mm × 128.5 mm).
The four channels are evenly spaced at y (rack‑unit) positions 32, 120, 208, 296,
with 88 units between macro centers and 7‑units of visible gap between the bottom
of one channel’s output jack and the top of the next channel’s macro knob.

| y (rack‑units) | Control        | Type                 |
|----------------|----------------|----------------------|
| y[i]           | Macro knob     | `RoundBlackKnob`     |
| y[i] + 18      | Range switch   | `CKSS`               |
| y[i] + 30      | Scale knob     | `RoundSmallBlackKnob`|
| y[i] + 45      | RGB LED        | `MediumLight<RedGreenBlueLight>` |
| y[i] + 54      | Output jack    | `PJ301MPort`         |

Horizontal layout (4 hp, `box.size.x` = 60 rack‑units):

| Control        | x (rack‑units) | x (SVG mm) |
|----------------|----------------|------------|
| Macro knob     | 30             | 10.16      |
| Range switch   | 52             | 17.61      |
| Scale knob     | 30             | 10.16      |
| RGB LED        | 8              | 2.71       |
| Output jack    | 30             | 10.16      |

Screws: top–left at (0, 0), top–right at `(box.size.x − RACK_GRID_WIDTH, 0)`,
bottom–left at `(0, box.size.y − RACK_GRID_WIDTH)`, bottom–right at
`(box.size.x − RACK_GRID_WIDTH, box.size.y − RACK_GRID_WIDTH)`.

---

## Signal Flow

```
Macro (0‑1)
  │
  ├─ Range=Unipolar:  × 10    →  0 … 10 V
  └─ Range=Bipolar:  × 10 − 5  →  −5 … +5 V
  │
  └─ × Scale (0‑1)   →  final voltage
       │
       ├─ Output jack
       └─ RGB LED hue ← voltage mapped to [0, 1)
```

## Process

For each of the four channels the `process()` method:

1.  Reads `MACRO1_PARAM` (range 0‑1, default 0.5).
2.  Applies the range switch:
    - **Unipolar** (up): `v = macro × 10` → output range 0 … +10 V.
    - **Bipolar** (down): `v = macro × 10 − 5` → output range −5 … +5 V.
3.  Applies the scale knob: `v ×= SCALE1_PARAM` (range 0‑1, default 1).
4.  Writes `v` to the output port via `outputs[OUTPUT1].setVoltage(v)`.
5.  Normalises `v` to a hue value in `[0, 1)`:
    - **Unipolar**: `hue = v / 10`  — 0 V → red, 10 V → red (full cycle).
    - **Bipolar**: `hue = (v + 5) / 10` — −5 V → red, +5 V → red (full cycle).
6.  Converts hue to RGB with a standard HSV‑to‑RGB algorithm (hue ∈ [0, 1),
    full saturation and value). Splits into six‑sector piecewise linear
    interpolation to avoid trigonometric functions.
7.  Sets the three `RedGreenBlueLight` channels:
    ```
    lights[LIGHTn_R].setBrightness(r)
    lights[LIGHTn_G].setBrightness(g)
    lights[LIGHTn_B].setBrightness(b)
    ```

At scale = 0 the output is always 0 V, so the hue collapses to a fixed colour
(0.0 for unipolar, 0.5 for bipolar) — the LED stops cycling.

---

## Parameters

| ID            | Type        | Range | Default | Display                 |
|---------------|-------------|-------|---------|-------------------------|
| MACROn_PARAM  | knob        | 0‑1   | 0.5     | —                       |
| RANGEn_PARAM  | toggle      | 0‑1   | 0       | **±5 V** / **0–10 V**  |
| SCALEn_PARAM  | knob        | 0‑1   | 1       | `%` (0–100, 0 dB gain)  |

## Outputs

| ID        | Description            |
|-----------|------------------------|
| OUTPUTn   | DC voltage (‑5 … +10 V) |

## Lights

| ID       | Channels | Widget                            |
|----------|----------|-----------------------------------|
| LIGHTn_R | 0        | —                                 |
| LIGHTn_G | 1        | —                                 |
| LIGHTn_B | 2        | `MediumLight<RedGreenBlueLight>`  |

Each `RedGreenBlueLight` consumes three consecutive light indices (R, G, B).
Total light count: 4 channels × 3 colours = 12.

---

## Coordinate System

VCV Rack v2 uses an internal coordinate system where 1 hp = 15 rack‑units.
Panel SVGs are authored in millimeters; the `SvgPanel` scales them so that
a 20.32 mm‑wide viewBox (4 hp) produces `box.size.x = 60` rack‑units.

```
mm → rack‑units:  multiply by  15 / 5.08   (≈ 2.953)
rack‑units → mm:  multiply by  5.08 / 15    (≈ 0.339)
```

Widget positions throughout this file are given in rack‑units to match the
VCV coordinate system. SVG elements use millimeters.

---

## Dependencies

- VCV Rack v2 SDK
- `componentlibrary.hpp`: `MediumLight`, `RedGreenBlueLight`, `CKSS`,
  `RoundBlackKnob`, `RoundSmallBlackKnob`, `PJ301MPort`, `ScrewSilver`
- SVG panel at `res/ra-knob.svg` (viewBox `0 0 20.32 128.5`)
