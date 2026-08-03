# ra-klock — Master Clock Module

10 HP master clock generator with BPM display, swing, run/reset control,
and multiple simultaneous clock outputs.

## Panel Layout

```
┌──────────────────────────────────────┐
│[]    ●    ●  ○───────────────────○   │  y=40   v/oct | BPM knob | beat LED
│                                      │
│[]    ●                        ┌──┐   │  y=98   swing CV
│      ●                        │BP│   │  y=118  swing knob | BPM display (30×40)
│                               │M │   │
│                               └──┘   │
│[] ●  ●  ●    ○───────────────────○   │  y=174  run CV | run btn | reset btn | reset CV
│                                      │
│ ○    ○    ○                          │  y=232  CLK | RUN | RESET
│                                      │
│ ○    ○    ○                          │  y=277  x2 | x4 | x8
│                                      │
│ ○    ○    ○                          │  y=322  /2 | /4 | /8
└──────────────────────────────────────┘
    25   75   125
```

Note: ● = knob, ○ = port, [●] = screw, ┌──┐ = display

## Controls

### BPM Knob (large)
- Range: 0–333 BPM, set via knob + v/oct CV.
- CV scaling is 1 V/oct exponential — +1 V doubles the tempo.

### Swing Knob
- 0% – 100%. Divides each beat into two halves: the onbeat half
  (ticks 0–3) lengthens as swing increases, the offbeat half (ticks 4–7)
  shortens proportionally.
- Formula: `onbeatRatio = 0.5 + swing × 0.3`
- At 0%: both halves are equal (50/50, straight).
- At 100%: onbeat occupies 80% of the beat, offbeat 20% (extreme shuffle).
- **Swing does not affect** CLK, /2, /4, /8, RUN, or RESET outputs.
- **Swing only affects** ×2, ×4, and ×8 outputs.

### Run Button / Run CV Input
- Button toggles run state on each press.
- CV input (gate trigger) toggles run state on rising edge.
- When stopped, all clock outputs are 0 V.

### Reset Button / Reset CV Input
- Button fires a reset pulse immediately.
- CV input (gate trigger) fires a reset pulse on rising edge.
- Reset zeroes the phase and beat counter. All × outputs fire
  a 1 ms pulse on reset.

## Display

3:4 aspect ratio (30 × 40 rack units), positioned right of the swing knob.

- **Top line**: current BPM (integer).
- **Bottom line**: swing percentage (e.g. `S50`).
- Brightness pulses on the beat. Shows `---` when stopped.

## Outputs

All outputs are 10 V gate pulses.

### CLK — Clock (×1)
- Fires once per beat (tick 0). Unaffected by swing.

### RUN — Run Gate
- 10 V while running, 0 V when stopped.

### RESET — Reset Gate
- 1 ms pulse on reset trigger (button or CV).

### ×2 — Doubled Clock
- Fires at tick 0 and tick 4 (mid-beat). **Swing shifts tick 4**:
  at high swing values the second pulse arrives noticeably later.

### ×4 — Quadrupled Clock
- Fires at ticks 0, 2, 4, 6. **Swing shifts ticks 2, 4, 6** —
  feels like a shuffle.

### ×8 — Octupled Clock
- Fires on every tick transition (8 pulses per beat). **Swing
  slightly adjusts inter-pulse spacing** — subtle shuffle feel.

### /2 — Beat Divider
- Fires once every 2 beats. **Not affected by swing.**

### /4 — Beat Divider
- Fires once every 4 beats. **Not affected by swing.**

### /8 — Beat Divider
- Fires once every 8 beats. **Not affected by swing.**

## Clock Architecture

Each beat is divided into **8 ticks** (0–7) by a phase accumulator.
`tickFromPhase()` maps the continuous phase [0 … 1) to ticks using
the swing-adjusted onbeat/offbeat split:

```
             onbeat half           offbeat half
tick:     | 0 | 1 | 2 | 3 |     | 4 | 5 | 6 | 7 |
phase:    0              onbeatRatio          1
```

The onbeat half spans `[0, onbeatRatio)` and the offbeat half
spans `[onbeatRatio, 1)`. At 0% swing `onbeatRatio = 0.5` (equal).
At 100% swing `onbeatRatio = 0.8` (4:1 ratio).

Swing widens/narrows the 4-tick groups. Tick 0 is always at phase 0
(the beat boundary), and tick 4 is always at `onbeatRatio` (the
swing-offset middle point). This means **CLK** (fires at tick 0
only) and all **beat-counter-based outputs** (/2, /4, /8) are
unaffected by swing.

## Patch Examples

### Basic clock with division
```
CLK → sequencer clock input
/4  → sequencer reset
```

### Drum pattern with swing
```
×2 → hi-hat trigger (straight 8ths)
×4 → snare trigger (shuffled 16ths)
```

### Tempo-synced LFO
```
Run the clock at 16× the LFO rate by setting BPM accordingly,
or use ×8 output for double-speed modulation.
```
