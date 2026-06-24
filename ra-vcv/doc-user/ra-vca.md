# ra-vca — Dual Voltage-Controlled Amplifier

4 HP dual VCA with 8‑segment RGB VU metering per channel, linear/exponential
response, CV sum mode, and switchable soft clipping.

## Panel Layout

```
┌──────────────────────┐
│[]                    │
│                      │
│          ●           │  y=56  Gain 1
│                      │
│          ○           │  y=96  CV 1
│                      │
│          ○           │  y=126 Audio In 1
│                      │
│          ○           │  y=156 Audio Out 1
│                      │
│ ┌──┐ ┌──┐ ┌──┐      │  y=191 Mode | Soft Clip | Sum
│                      │
│          ●           │  y=226 Gain 2
│                      │
│          ○           │  y=266 CV 2
│                      │
│          ○           │  y=296 Audio In 2
│                      │
│          ○           │  y=326 Audio Out 2
│[]                    │
└──────────────────────┘
    15   30   45
    19         50
```

● = knob, ○ = port, ┌──┐ = switch, [] = screw

A vertical column of 8 RGB LEDs sits at x=50 alongside each channel (y=56–161
for ch1, y=226–331 for ch2).

## Controls

### Gain Knob (per channel)

- Range: 0–100%
- **Without CV patched**: knob sets the fixed gain directly.
- **With CV patched**: knob attenuates the CV signal (multiplies the CV by the
  knob value).

### Mode Switch

- **Linear** (up): gain follows the amplitude linearly.
- **Exp** (down): gain is squared (`gain²`) for an exponential response curve.
  The VU meter tracks the post‑squared value.

### Soft Clip Switch

- **Off** (up): audio output passes through linearly, no clamping.
- **On** (down): `tanh` soft clipping on the audio output for rounded
  saturation near ±10 V.

### Sum Switch

- **Indiv** (up): each channel uses only its own CV input.
- **Sum** (down): CV 1 and CV 2 are summed together and hard‑clamped to
  0–10 V. Both channels receive the same summed CV, each attenuated by its own
  gain knob. When neither CV is patched, the knobs behave as fixed gain.

## Signal Flow

```
                          ┌───────────┐
CV 1 ───────────────────> │           │
                          │  Multiply  │──> gain 1
Gain Knob 1 ────────────> │           │
                          └───────────┘
                                │
                          ┌───────────┐
                          │  Clamp    │──> gain ∈ [0, 1]
                          │  0–1      │
                          └───────────┘
                                │
                          ┌───────────┐     ┌───────────┐
                          │  Mode?    │────>│  Square   │──> gain¹ or gain²
                          └───────────┘     └───────────┘
                                │
                          ┌───────────┐
Audio In ───────────────> │  Multiply │
final gain ─────────────> │           │
                          └───────────┘
                                │
                          ┌───────────┐
                          │ Soft Clip?│────> tanh or pass
                          └───────────┘
                                │
                           Audio Out

In Sum mode CV 1 and CV 2 are added and clamped 0–10 V before splitting:

  CV 1 ──┐
          ├──> (+) ──> clamp(0,10) ──> gain knob × cv/10
  CV 2 ──┘
```

## VU Meter

Each channel has 8 RGB LEDs (green → yellow → red) showing the applied gain:

| LEDs lit | Gain range |
|----------|------------|
| 0        | 0%         |
| 1–2      | 12–25%     |
| 3–4      | 37–50%     |
| 5–6      | 62–75%     |
| 7–8      | 87–100%    |

The VU displays the gain value after mode (linear/exp) processing — what is
actually applied to the audio.

## Patch Examples

### Dual envelope‑controlled VCA

```
Envelope 1 → CV 1    | Audio source 1 → Audio In 1
Envelope 2 → CV 2    | Audio source 2 → Audio In 2
Gain knobs at 100%   | Mode = Linear, Sum = Indiv
```

Each channel follows its own envelope at full strength.

### Single modulation source driving both

```
LFO/envelope → CV 1  | (CV 2 unpatched)
Sum = Sum             | Gain knobs set level per channel
```

The single CV source feeds both VCAs. The knob on ch2 controls its level
independently. Hard‑clamp ensures the summed CV never exceeds 10 V even with
2 patched sources at 10 V each.

### Saturated drum bus

```
Drum mix → Audio In 1 | Drum mix → Audio In 2
Gain knob at 100%     | Soft Clip = On, Sum = Indiv
```

Push the level into the soft clipper for warm saturation on the summed drum bus.

### Exponential ducking

```
Sidechain trigger → CV 1
Audio → Audio In 1
Gain knob tuned     | Mode = Exp
```

The squared curve gives a more pronounced ducking feel at mid‑CV levels.

### Fixed‑level stereo amplifier

```
(CV inputs unpatched)
Gain knobs at 75%   | Sum = Indiv, Mode = Linear
Left → Audio In 1   | Right → Audio In 2
Audio Out 1 → Left   | Audio Out 2 → Right
```

Both channels act as independent fixed‑gain amps with no patching required.
