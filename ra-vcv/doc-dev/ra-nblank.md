# ra-nblank — Animated Cable Visuals & On-Hover Scope

ra-nblank is a clone/port of the **Blank** module from [gibbonjoyeux/VCV-Biset](https://github.com/gibbonjoyeux/VCV-Biset) grafted onto the ra-blank resizable panel skeleton. It's a "blank" panel that is far from blank — it replaces VCV Rack's native cable rendering with animated signal-flow cables and provides a floating oscilloscope that appears when you hover over a cable port.

---

## What it does

1. **Resizable blank panel** — inherited from `ra-blank`. Tiles the SVG horizontally, lets you drag the edges to resize (snaps to HP increments), persists width in patch files.

2. **Animated cables** — hides VCV's default cable container and redraws every cable from scratch. Each cable is drawn as a quadratic Bezier curve whose sample points are displaced perpendicularly by historical voltage values, creating a ripple that visually represents the signal flowing through the cable.

3. **On-hover oscilloscope** — a floating scope widget (added to `APP->scene`, not the rack) that appears when you hover the mouse over any cable's input or output port. Shows the waveform from the cable's history buffer in the cable's color, with ±5V/±10V reference lines and a port-name label. Also works on unconnected output ports.

4. **Singleton gating** — only the last-instantiated ra-nblank in the patch is active. Others are dormant. When bypassed or deleted, default cable rendering is restored.

---

## Architecture (single file: `src/ra-nblank.cpp`)

```
RaNblankModule           — engine::Module, processes every 32 frames
├── cables[]             — BLANK_CABLES (256) + 1 extra for hovered unconnected port
├── buffer_i             — circular buffer write head
├── scope_index          — which cable is currently hovered (-1 if none)
│
BlankCablesWidget        — Widget added to APP->scene->rack
│   └── drawLayer(layer=1)  — draws all cables with animation
│
BlankScopeWidget         — Widget added to APP->scene
│   └── draw()              — draws floating oscilloscope
│
RaNblankPanel            — tiled SVG panel (same as RaBlankPanel)
│
RaNblankResizeHandle     — left/right drag handles (same as RaModuleResizeHandle)
│
RaNblankWidget           — ModuleWidget, assembles everything
    └── appendContextMenu  — check items for cable/scope options
```

### Buffer loop

```
                                 ┌─────────────────────┐
                                 │  RaNblankModule     │
                                 │  process()          │
                                 │  (every 32 frames)  │
                                 └──────┬──────────────┘
                                        │
                          ┌─────────────┴──────────────┐
                          │ Scan all CableWidgets from │
                          │ getCableContainer()        │
                          └─────────────┬──────────────┘
                                        │
                          ┌─────────────┴──────────────┐
                          │ Read output voltage via    │
                          │ cable->outputModule->      │
                          │   outputs[id].getVoltage() │
                          └─────────────┬──────────────┘
                                        │
                          ┌─────────────┴──────────────┐
                          │ Write to circular buffer   │
                          │ cables[i].buffer[buffer_i] │
                          │ buffer_i = (buffer_i+1)%2048│
                          └─────────────┬──────────────┘
                                        │
                          ┌─────────────┴──────────────┐
                          │ BlankCablesWidget          │
                          │ drawLayer(layer=1)         │
                          │ Read circular buffer,      │
                          │ displace Bezier by voltage │
                          └────────────────────────────┘
```

---

## Parameters (all context-menu only, no panel controls)

### Cable

| Param | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| Cable animation | switch | off/on | on | Hides native cables and draws animated ones |
| Cable LED | switch | off/on | on | Colored voltage indicators (green/red) inside port plugs |
| Cable brightness | switch | off/on | on | Apply `rackBrightness` to cable colors |
| Polyphonic thickness | switch | off/on | on | Thicker stroke for polyphonic cables |
| Polyphonic mode | index | 0-2 | 0 (1st ch) | Which voltage to animate: 1st channel, sum, or sum/channels |
| CPU fast | switch | off/on | off | Skip peak-detection in buffer read (reduces quality) |
| Cable slew | float | 0-1 | 0 | Low-pass filter on animation voltage (smooths jitter) |
| Cable scale | float | 0-2 | 1 | Amplitude multiplier for cable displacement |

### Scope

| Param | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| Scope | switch | off/on | on | Enable floating scope |
| Scope on Shift only | switch | off/on | off | Scope only appears while Shift is held |
| Scope mode | switch | circular/linear | linear | Circular = reads from buffer head backward; Linear = full buffer sweep |
| Scope position | index | 0-4 | 0 | Top-left, top-right, bottom-left, bottom-right, center |
| Scope scale | float | 0.02-1 | 0.2 | Size relative to rack window |
| Scope thickness | float | 1-10 | 2 | Waveform line width |
| Scope background alpha | float | 0-1 | 0.6 | Scope background opacity |
| Scope voltage alpha | float | 0-1 | 0.3 | Reference line opacity |
| Scope label alpha | float | 0-1 | 1 | Port name label opacity |
| Scope alpha | float | 0-1 | 1 | Overall scope opacity |

### Panel

| Param | Type | Range | Description |
|-------|------|-------|-------------|
| Panel | index | 0-3 | City pigeon, Wild pigeon, Pigeon gang, Pigeon Army |

---

## What was cloned from VCV-Biset Blank

The core logic was ported from [VCV-Biset](https://github.com/gibbonjoyeux/VCV-Biset) `src/Blank/`:

| Biset File | Lines | Ported To |
|------------|-------|-----------|
| `Blank.hpp` | Full header | Constants, enums, `BlankCable` struct, all param IDs |
| `Blank.cpp` | `process()` | `RaNblankModule::process()` — cable scanning, buffer writes, hover detection, incomplete cable |
| `BlankCables.cpp` | `drawLayer()` | `BlankCablesWidget::drawLayer()` — Bezier animation, LED plugs, halos, incomplete cable |
| `BlankScope.cpp` | `draw()` | `BlankScopeWidget::draw()` — scope background, voltage lines, waveform, label |
| `BlankWidget.cpp` | Constructor + `appendContextMenu()` + `set_panel()` | `RaNblankWidget` — display/scope widget creation, context menu, panel switching |
| (Biset `Blank.hpp`) | Singleton `g_blank` | `g_nblank` global singleton |

### Differences from the Biset original

1. **Single-file structure** — Biset uses 5 files (`.hpp` + 4 `.cpp`); we merged into one `.cpp` following the ra-vcv convention.
2. **Rack SDK 2.6.6 API** — uses `createCheckMenuItem` / `createSubmenuItem` from `helpers.hpp` instead of `MenuCheckItem` / `MenuSlider` (which don't exist in this SDK version).
3. **Resizable panel** — built on top of ra-blank's existing `RaNblankPanel` + `RaNblankResizeHandle` infrastructure (Biset's Blank is not resizable).
4. **No panel controls** — Biset's Blank has no physical controls either, but its SVG panels are images of pigeons. ra-nblank uses the same panel-switching mechanic with different SVG files.
5. **Continuous sliders omitted from menu** — slew, scale, and scope alpha/thickness/size params are defined and work via automation but don't have context-menu sliders (requires a `MenuSlider` class not present in this Rack SDK version).

---

## Porting notes

### The Bezier cable animation

Each cable is a quadratic Bezier:

```
P(t) = (1-t)²·P_out + 2·(1-t)·t·P_slump + t²·P_in
```

The control point `P_slump` is computed as the midpoint between the two ports, pushed downward by `(1 - cableTension) * (150 + distance)`, mimicking gravity on a loose cable.

At each of the 128 sample points along the Bezier, the signal voltage from the circular buffer is read (at a phase offset proportional to `t` and cable length), then applied perpendicular to the curve tangent:

```
displacement = cos(angle + π/2) * voltage * amp * scale * orientation
```

The `amp` factor tapers at the endpoints to keep the cable anchored at the ports.

### Scope buffer modes

- **Circular mode** — reads from `buffer_i` backward, showing the most recent samples. Uses peak-detection between sample points to avoid aliasing.
- **Linear mode** — reads from index 0 to `BLANK_BUFFER`, showing the full buffer as a snapshot.

### Scope positioning

The scope box size is a fraction of `APP->scene->box.size` (the rack window), scaled by the `PARAM_SCOPE_SCALE` parameter. Position is absolute with a 10-unit margin from the chosen edge.

### Singleton gating

```cpp
static RaNblankModule* g_nblank = NULL;

// In process():
if (g_nblank == NULL) g_nblank = this;
if (g_nblank != this) return;  // only the last-instanced runs

// In destructor:
if (this == g_nblank) {
    g_nblank = NULL;
    APP->scene->rack->getCableContainer()->show();
}
```

## SVG panel assets

The panel SVGs for the 4 variants are loaded from:
- `res/ra-nblank.svg` (default / City pigeon)
- `res/Blank-Wild.svg`
- `res/Blank-Gang.svg`
- `res/Blank-Army.svg`

The last three are Biset-original SVGs that need to be placed in `res/` from the Biset repository for the panel-switching feature to work.
