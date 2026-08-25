
## Build System

- Set `RACK_DIR` to the SDK path in the Makefile (defaults to `../Rack-SDK`)
- List all `.cpp` sources in `SOURCES`, including `dep/s7/s7.c` for the Scheme interpreter
- Add `FLAGS += -Idep/s7` to include the s7 headers
- Add resource files with `DISTRIBUTABLES += $(wildcard res/*)`
- `make` — builds `plugin.so`
- `make install` — packages into `.vcvplugin` and copies to `~/.local/share/Rack2/plugins-lin-x64/`
- Version in `plugin.json` must start with `2` for Rack v2 ABI compatibility

## plugin.json

- `slug` (plugin) — unique identifier, used as brand in module browser
- `slug` (module) — unique per-module, used in patch files, never change after release
- `version` — must be `2.x.x` for Rack v2
- `tags` — controls which category the module appears in
- Plugin slug is `"ReplicatAudio"`; contains 16 modules total

## Coordinate System

VCV Rack v2 uses an internal **rack‑unit** coordinate system.

| Constant               | Value | Meaning                    |
|------------------------|-------|----------------------------|
| `RACK_GRID_WIDTH`      | 15    | width of 1 hp in rack‑units |
| `RACK_GRID_HEIGHT`     | 380   | standard panel height       |

**Conversions:**

```
1 hp           = 5.08 mm        = 15 rack‑units
rack‑units → mm = value × 5.08 ÷ 15
mm → rack‑units = value × 15 ÷ 5.08
```

`SvgPanel` loads a panel SVG (authored in mm) and sets the widget's `box.size`
in rack‑units, scaling the SVG automatically.

For a 4 hp panel: `viewBox="0 0 20.32 128.5"` → `box.size = (60, 380)`

## Module Anatomy

Every module has three parts:

All source files include `"ra-components.hpp"` (which itself includes `"rack.hpp"`)
for widget type aliases instead of including `"rack.hpp"` directly.

### 1. Model (global variable)

```cpp
Model *modelRaVca = createModel<RaVcaModule, RaVcaWidget>("ra-vca");
```

The string `"ra-vca"` must match the module slug in plugin.json.

### 2. Module (DSP)

```cpp
struct RaVcaModule : Module {
    enum ParamIds { GAIN_PARAM, NUM_PARAMS };
    enum InputIds { AUDIO_INPUT, CV_INPUT, NUM_INPUTS };
    enum OutputIds { AUDIO_OUTPUT, NUM_OUTPUTS };
    enum LightIds { NUM_LIGHTS };

    RaVcaModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(GAIN_PARAM, 0.f, 2.f, 1.f, "Gain", " dB", 0.f, 20.f, 0.f);
        configInput(AUDIO_INPUT, "Audio");
        configInput(CV_INPUT, "CV");
        configOutput(AUDIO_OUTPUT, "Audio");
    }

    void process(const ProcessArgs &args) override {
        float gain = params[GAIN_PARAM].getValue() + inputs[CV_INPUT].getVoltage() / 10.f;
        gain = clamp(gain, 0.f, 2.f);
        outputs[AUDIO_OUTPUT].setVoltage(inputs[AUDIO_INPUT].getVoltage() * gain);
    }
};
```

### 3. ModuleWidget (UI)

```cpp
struct RaVcaWidget : ModuleWidget {
    RaVcaWidget(RaVcaModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-vca.svg")));
        // Use addChild for screws, createWidget for non-interactive elements
        // Use addParam/addInput/addOutput for params/ports

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RaKnob>(Vec(x, y), module, RaVcaModule::GAIN_PARAM));
        addInput(createInputCentered<RaPort>(Vec(x, y), module, RaVcaModule::AUDIO_INPUT));
        addOutput(createOutputCentered<RaPort>(Vec(x, y), module, RaVcaModule::AUDIO_OUTPUT));
    }
};
```

#### Screw Positioning

Left-side screws must be at **x = 0** (flush with panel edge).
Right-side screws must be at **x = box.size.x - RACK_GRID_WIDTH** (flush with
panel edge — the screw SVG is exactly RACK_GRID_WIDTH wide).

| Position      | Code                                              |
|---------------|---------------------------------------------------|
| Top‑left      | `Vec(0, 0)`                                       |
| Top‑right     | `Vec(box.size.x - RACK_GRID_WIDTH, 0)`            |
| Bottom‑left   | `Vec(0, box.size.y - RACK_GRID_WIDTH)`           |
| Bottom‑right  | `Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)` |

Do NOT use `RACK_GRID_WIDTH` for left-side x positions — that puts the screw
7.5 units in from the left edge, which is incorrect for this project.

## plugin.cpp

```cpp
#include "rack.hpp"

using namespace rack;

Plugin *pluginInstance;
extern Model *modelRaVca;
extern Model *modelRaGnawbz4x;
extern Model *modelRaGnawbz1x4;
extern Model *modelRaUlfo;
extern Model *modelRaScaler;
extern Model *modelRaReflectingPool;
extern Model *modelRaYscope;
extern Model *modelRaShapes;
extern Model *modelRaChance;
extern Model *modelRaVipberus;
extern Model *modelRaSeerMini;
extern Model *modelRaBlank;
extern Model *modelRaAdsr;
extern Model *modelRaKlock;
extern Model *modelRaButtons;
extern Model *modelRaScheme;

void init(Plugin *p) {
    pluginInstance = p;
    p->addModel(modelRaVca);
    p->addModel(modelRaGnawbz4x);
    p->addModel(modelRaGnawbz1x4);
    p->addModel(modelRaUlfo);
    p->addModel(modelRaScaler);
    p->addModel(modelRaReflectingPool);
    p->addModel(modelRaYscope);
    p->addModel(modelRaShapes);
    p->addModel(modelRaChance);
    p->addModel(modelRaVipberus);
    p->addModel(modelRaSeerMini);
    p->addModel(modelRaBlank);
    p->addModel(modelRaAdsr);
    p->addModel(modelRaKlock);
    p->addModel(modelRaButtons);
    p->addModel(modelRaScheme);
}
```

- `pluginInstance` — global pointer set in `init()`, used by `asset::plugin()` to resolve resource paths
- `extern Model *modelXxx` — forward-declare models defined in other `.cpp` files
- `init()` must have C linkage (declared `extern "C"` in `plugin/callbacks.hpp`), so define it as a plain function

## Parameters

### Param types

| Creator function                | Widget            | Usage                    |
|--------------------------------|-------------------|--------------------------|
| `createParamCentered`          | `RaKnob`          | Standard macro knob      |
| `createParamCentered`          | `RaKnobSmall`     | Small trim knob          |
| `createParamCentered`          | `RaSwitch2`       | 2‑position toggle switch |
| `createParamCentered`          | `RaSwitch3`       | 3‑position toggle switch |

### configParam

```cpp
configParam(paramId, minValue, maxValue, defaultValue, name, unit, displayBase, displayMultiplier, displayOffset);
```

- `name` — label shown in context menu and tooltip
- `unit` — appended after the value (e.g. `" dB"`, `" %"`, `" V"`)
- `displayMultiplier` / `displayOffset` — transform the raw value for display:
  `displayed = raw × displayMultiplier + displayOffset`
- `displayBase` — if non‑zero, display uses `displayMultiplier × base^raw + displayOffset`

Example — gain shown in dB with 0 dB at unity:
```cpp
configParam(GAIN_PARAM, 0.f, 2.f, 1.f, "Gain", " dB", 10.f, 20.f, 0.f);
// raw 0→20*10^0=20 dB, raw 0.5→20*sqrt(10)≈63 dB, raw 1→20*10=200 dB
```

Example — scale knob shown as percentage:
```cpp
configParam(SCALE_PARAM, 0.f, 1.f, 1.f, "Scale", "%", 0.f, 100.f);
```

### configSwitch

```cpp
configSwitch(paramId, minValue, maxValue, defaultValue, name, labels);
```

`labels` is an `std::vector<std::string>` where each element is the label for
that position. The raw value snaps to integer positions.

```cpp
configSwitch(RANGE_PARAM, 0.f, 1.f, 0.f, "Range", {"\u00B15V", "0\u201310V"});
```

Use `\u00B5` for µ, `\u00B1` for ±, `\u2013` for en‑dash, `\u2014` for em‑dash.

## Inputs & Outputs

```cpp
configInput(PORT_ID, "Name");
configOutput(PORT_ID, "Name");

// In process:
float v = inputs[PORT_ID].getVoltage();
outputs[PORT_ID].setVoltage(v);
```

Inputs can be left unpatched — `getVoltage()` returns `0.f`.

## Lights

### Light types

| Widget class                     | Size  | Colour channels              |
|----------------------------------|-------|------------------------------|
| `TinyLight<RedGreenBlueLight>`   | 3 mm  | 3 (R, G, B via base colors) |
| `MediumLight<RedGreenBlueLight>` | 5 mm  | 3                            |
| `LargeLight<RedGreenBlueLight>`  | 5 mm  | 3                            |
| `TinyLight<GreenRedBlueLight>`   | 3 mm  | 3 (G, R, B order)           |
| `TinyLight<WhiteLight>`          | 3 mm  | 1                            |

### Light IDs

Multi‑colour lights consume **multiple consecutive light indices**.  
`RedGreenBlueLight` adds 3 base colors → uses 3 indices per widget.

```cpp
enum LightIds {
    LIGHT1_R,   // index 0 — red
    LIGHT1_G,   // index 1 — green
    LIGHT1_B,   // index 2 — blue
    LIGHT2_R,   // index 3 — red
    // ...
    NUM_LIGHTS  // = 12 for 4 RGB widgets
};
```

In the constructor:
```cpp
for (int c = 0; c < 3; c++)
    configLight(LIGHT1_R + i * 3 + c, "Light");
```

### Adding a light to the widget

```cpp
addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(
    Vec(x, y), module, LIGHT1_R + i * 3));
```

The `lightId` argument points to the **first** of the 3 consecutive indices.

### Setting colours in process

```cpp
void process(const ProcessArgs &args) override {
    // ... compute output voltage v ...

    float hue = clamp((v + 5.f) / 10.f, 0.f, 1.f);  // bipolar normalisation

    // HSV → RGB (manual, avoids trig)
    int hi = (int)(hue * 6.f);
    float f = hue * 6.f - hi;
    float p = 1.f - f;
    float q = 1.f - (1.f - f);
    float r, g, b;
    switch (hi % 6) {
        case 0: r = 1.f; g = q; b = p; break;
        case 1: r = p; g = 1.f; b = p; break;
        // ... etc
    }
    lights[LIGHTn_R].setBrightness(r);
    lights[LIGHTn_G].setBrightness(g);
    lights[LIGHTn_B].setBrightness(b);
}
```

`lights[id].setBrightness(float)` sets a single light channel. The 3 channels
must be set individually for an RGB widget.

## SVG Panels

- Units are millimeters. 1hp = 5.08mm.
- `viewBox` dimensions determine module size: `viewBox="0 0 20.32 128.5"` = 4hp × standard height
- Must be included in the package via `DISTRIBUTABLES` in Makefile
- Labels, artwork, and LED‑hole indicators are drawn as regular SVG elements

To swap a panel, change the path in `setPanel(createPanel(asset::plugin(...)))`.

## Key Widget Types

| Widget | Usage |
|--------|-------|
| `RaKnob` | Standard knob (Davies1900hBlackKnob) |
| `RaKnobLarge` | Large knob (Davies1900hLargeBlackKnob) |
| `RaKnobSmall` | Small knob for tight spots (RoundSmallBlackKnob) |
| `RaKnobTrim` | Trim pot (Trimpot) |
| `RaPort` | Audio/CV jack — auto light/dark (ThemedPJ301MPort) |
| `RaSwitch2` | 2‑position toggle switch (CKSS) |
| `RaSwitch3` | 3‑position toggle switch (NKK) |
| `RaButton` | Momentary push button (VCVButton) |
| `RaLightButton` | Light-up push button (VCVLightButton<WhiteLight>) |
| `RaLightBezel` | Light-up bezel (VCVLightBezel<WhiteLight>) |
| `RaScrew` | Panel screw — auto light/dark (ThemedScrew) |
| `RaRGBLight` | 5 mm RGB LED (MediumLight<RedGreenBlueLight>) |

All type aliases are defined in `src/ra-components.hpp`. Include it instead of
`rack.hpp` in module source files for convenience.

## Port Ordering

Rack renders module params/inputs/outputs in enum order, not addition order.
Keep `NUM_PARAMS` / `NUM_INPUTS` / `NUM_OUTPUTS` / `NUM_LIGHTS` last in
each enum.

## Build Automation

- `util/make.sh` — cleans, builds, and installs in one step
- Override `RACK_DIR` at invocation: `RACK_DIR=../Rack-SDK ./util/make.sh`
- After install, restart Rack to pick up the updated `.vcvplugin`

## Common Pitfalls

| Symptom | Cause |
|---------|-------|
| `"Failed to load plugin"` | `version` in `plugin.json` doesn't start with `2` |
| Undefined symbol `init` | Missing `extern "C"` linkage on `init()` |
| Undefined symbol `pluginInstance` | Missing `Plugin *pluginInstance;` in `plugin.cpp` |
| SVGs missing from package | Missing `DISTRIBUTABLES += $(wildcard res/*)` |
| Screws half off panel | Left screw x position should be `0`, not `RACK_GRID_WIDTH` |
| Lights not showing | `NUM_LIGHTS` too small — count each RGB channel separately |
| Module not in browser | Missing `p->addModel(modelXxx)` in `init()` |
| Module doesn't save state | `plugin.json` slug mismatch with `createModel` string |
# VCV Rack Layout Reference

## Coordinate System

- **1 HP** = 5.08 mm = **15 Rack units** (also called px)
- **3U height** = **380 Rack units** (= 380 px at 100% zoom)
- **1 Rack unit** ≈ 5.08/15 = 0.339 mm ≈ 1 px at screen DPI
- All widget positions (`Vec(x, y)`) in `ModuleWidget` are in Rack units
- The module `box.size` is set by `setPanel()` — width is rounded to nearest HP, height stays at 380

### Constants

```cpp
RACK_GRID_WIDTH  = 15   // 1 HP
RACK_GRID_HEIGHT = 380  // 3U
RACK_GRID_SIZE   = Vec(15, 380)
RACK_OFFSET      = Vec(15 * 2000, 380 * 100)  // rack grid origin offset
```

### SVG ↔ Rack Units

Rack uses **72 DPI** for SVG rendering (not Inkscape's 96 DPI).

The module panel SVG viewBox should be in mm:
- Width = nHP × 5.08mm
- Height = 128.5mm (covers 380 Rack units)

SVG mm → Rack units: multiply by `15 / 5.08` (≈ 2.953)
Rack units → SVG mm: multiply by `5.08 / 15` (≈ 0.339)

## Component Sizes

All sizes in **Rack units (px)** at 100% zoom, determined from SVG width/height attributes.

### Ports & Jacks

| Component | Width | Height | Radius |
|-----------|-------|--------|--------|
| **PJ301MPort** | 23.7 | 23.7 | 11.85 |
| **PJ301MPort-dark** | 23.7 | 23.7 | 11.85 |
| **PJ3410** | 29.4 | 29.4 | 14.7 |
| **ADAT** | 29.5 | 30.2 | 14.75 |
| **MIDI_DIN** | 48.2 | 48.2 | 24.1 |
| **USB_B** | 30.9 | 34.6 | — |

### Buttons

| Component | Width | Height | Radius |
|-----------|-------|--------|--------|
| **TL1105** | 15.36 | 15.36 | 7.68 |
| **VCVButton** | 18 | 18 | 9 |
| **BefacoPush** | 28 | 28 | 14 |
| **PB61303** | 28.3 | 28.3 | 14.15 |

### Knobs

| Component | Width | Height | Radius |
|-----------|-------|--------|--------|
| **RoundHugeBlackKnob** | 53.9 | 53.9 | 26.95 |
| **RoundBigBlackKnob** | 45 | 45 | 22.5 |
| **RoundLargeBlackKnob** | 36 | 36 | 18 |
| **RoundBlackKnob** | 28.35 | 28.35 | 14.17 |
| **RoundSmallBlackKnob** | 22.68 | 22.68 | 11.34 |
| **Trimpot** | 17.86 | 17.86 | 8.93 |
| **BefacoTinyKnobWhite** | 25.5 | 25.5 | 12.75 |
| **BefacoBigKnob** | 73.7 | 73.7 | 36.85 |

### Switches

| Component | Width | Height | Radius |
|-----------|-------|--------|--------|
| **CKSS** (2 position) | 14 | 20.64 | 7×10.3 |
| **CKSSThree** (3 pos) | 13.46 | 28.35 | 6.73×14.17 |
| **CKSSThreeHorizontal** (3 pos) | 28.35 | 13.46 | 14.17×6.73 |
| **NKK** (3 pos) | 32 | 43.9 | 16×22 |

### Other

| Component | Width | Height | Radius |
|-----------|-------|--------|--------|
| **ScrewSilver** | 15 | 15 | 7.5 |
| **ScrewBlack** | 15 | 15 | 7.5 |
| **VCVBezel** | 21.3 | 21.3 | 10.65 |
| **CL1362** | 35 | 35 | 17.5 |

## Spacing Rules

### Minimum center-to-center distances

Two items should be spaced so the gap between their visible edges is ≥ 2-4 Rack units for tight layouts, ≥ 6-8 for comfortable layouts.

```
min_center_distance = r1 + r2 + desired_gap
```

| Pair | Radii sum | Tight (gap=2) | Comfortable (gap=6) |
|------|-----------|---------------|-------------------|
| Jack–Jack | 11.85+11.85=23.7 | **26** | **30** |
| Jack–TL1105 | 11.85+7.68=19.53 | **22** | **26** |
| TL1105–TL1105 | 7.68+7.68=15.36 | **18** | **22** |
| Jack–SmallKnob | 11.85+11.34=23.19 | **26** | **30** |
| Jack–RoundBlackKnob | 11.85+14.17=26.02 | **28** | **32** |

### Edge margins

Minimum distance from component edge to panel edge:
- Jack: 11.85 (touching edge), recommended ≥ 13 (small gap)
- TL1105: 7.68, recommended ≥ 10
- Knobs: depends on knob size

### Row spacing (vertical)

When stacking rows of controls:
```
min_row_spacing = max(r1_bottom, r2_top_in_next_row) + gap
```

Common ULFO reference:
- Sine output at y=106, Cosine output at y=106 (same row)
- Inv sine at y=128, Inv cosine at y=128 (next row)
- Row spacing = 128 - 106 = 22 units
- Jack radius = 11.85, so row 1 bottom = 106+11.85=117.85, row 2 top = 128-11.85=116.15
- Vertical overlap: 117.85 - 116.15 = 1.7 units (slight overlap accepted)
- With 24 unit row spacing: 106+11.85=117.85 vs 130-11.85=118.15 → gap of 0.3

For jacks: **24 units** between rows gives slight overlap (~2 units). For buttons: **22 units** is fine.
For clean separation with jacks: use **26-28 units** between row centers.

### Screw placement

Left-side screws go at x=0 to sit flush with the panel edge.
Right-side screws go at box.size.x - RACK_GRID_WIDTH to sit flush with the right edge.

```cpp
addChild(createWidget<RaScrew>(Vec(0, 0)));
addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));
```

## Reference Module: ULFO (4hp = 60 units wide)

```
x=30 (center)           — RoundBlackKnob (r=14.17) at y=24  → edges: 15.8–44.2
x=52 (right side)       — CKSS (r=7 wide) at y=46          → edges: 45–59
x=14 (left)             — PJ301M (r=11.85) at y=52         → edges: 2.15–25.85
x=30 (center)           — RoundSmallBlackKnob (r=11.34) at y=72 → edges: 18.66–41.34
x=16, 44 (L/R pair)    — PJ301M at y=106                  → edge margin ~4, gap between=4.3
x=16, 44 (L/R pair)    — PJ301M at y=128
x=30 (center)           — RoundSmallBlackKnob (r=11.34) at y=158
x=16, 44 (L/R pair)    — RoundSmallBlackKnob + PJ301M at y≈194/190
```

Key takeaways from ULFO:
- Left/right output jack pair (16 and 44) = **28 units between centers**
- Jack to panel edge margin: **~4 units** (very tight; 16 - 11.85 = 4.15)
- Output jack rows: **22 units** vertical spacing (slight overlap)

## CRITICAL: Follow Instructions Exactly — READ THIS FIRST

**The agent has a documented history of failing at basic instructions. You must follow these rules:**

1. **Do what the user says, exactly as they say it.** Take their words literally. Do not reinterpret, improve upon, or get creative with their instructions.
2. **If you don't understand or are unsure, stop and ask.** Do not guess. Do not try to solve ambiguous instructions with your own ideas.
3. **Your own ideas about layout, design, or architecture are not wanted.** The user is the architect. Execute precisely.
4. **If you find yourself thinking "but maybe they meant..." — stop. You are about to make a mistake. Ask.**

Violating these rules wastes the user's time and frustrates them. Don't do it.

### "vertical"
- "vertical row of LEDs" = one column, stacked top-to-bottom (same x, different y)
- "vertical space" = room along the Y axis (380 units tall)
- "stack vertically" = put things at different y positions
- NEVER interpret "vertical" as side-by-side columns

### "horizontal"
- "horizontal row of LEDs" = one row, left-to-right (same y, different x)
- "horizontal space" = room along the X axis (60 units for 4HP)

### Layout directions — literal meanings only

| User says | Means | Do this |
|-----------|-------|---------|
| "stack vertically" | chains at different **y** | same x, different y |
| "vertical row of LEDs" | LEDs in a **column** | same x, different y |
| "horizontal row" | LEDs in a **row** | same y, different x |
| "to the left/right" | move in x | change x, keep y |
| "move IO left" | shift ports to smaller x | change x, keep y |

### VU/LED meters — when user says "LEDs"

- Use `TinyLight<RedGreenBlueLight>` unless told otherwise
- Green (bottom/left) → Red (top/right) gradient for VU bars
- "8 in a row" = 8 LEDs in one horizontal row. "8 in a vertical row" = 8 LEDs stacked in one column.
- "per chain" = each signal path gets its own set
- "16 total" = 2 chains × 8 LEDs = 16

### If unsure: just ask. Do not guess and do not get creative.

## Layout Checklist

1. Place screws first: `Vec(0, 0)` (top-left), `Vec(box.size.x - RACK_GRID_WIDTH, 0)` (top-right)
2. Every `create*Centered()` positions the component's **center** at the given Vec.
3. All SVGs in `res/` are loaded with `createPanel()` and their viewBox width/height in mm set to nHP×5.08 × 128.5.
4. Label text in SVGs is placed at approximate positions (purely decorative — components are positioned by C++ code).
5. SVG coordinate ≈ Rack coordinate × 5.08/15.
6. Test visually: launch Rack, add module, check overlaps at 100% zoom.


## SVG Notes

The background SVG should just be an empty black image unless the user updates it. 

You never need to update SVGs. The user is an artist and they can handle that. 

