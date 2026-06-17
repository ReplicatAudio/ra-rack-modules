# VCV Rack Plugin Development Notes

## Project Structure

```
plugin-name/
├── Makefile              # Build config: RACK_DIR, SOURCES, DISTRIBUTABLES
├── plugin.json           # Manifest: slug, version, modules[]
├── make.sh               # Convenience build script
├── src/
│   ├── plugin.cpp        # Entry point: init(), pluginInstance
│   ├── ra-vca.cpp        # Module: Module + Widget structs, Model definition
│   └── ra-knob.cpp       # Another module
└── res/
    ├── ra-vca.svg        # Panel SVG for each module
    └── ra-knob.svg
```

## make.sh

```bash
#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
RACK_DIR="${RACK_DIR:-../Rack-SDK}"
make clean 2>/dev/null || true
make RACK_DIR="$RACK_DIR"
make install RACK_DIR="$RACK_DIR"
```

Override `RACK_DIR` at runtime if the SDK lives elsewhere:
```bash
RACK_DIR=/path/to/Rack-SDK ./make.sh
```

## Build System

- Set `RACK_DIR` to the SDK path in the Makefile
- List all `.cpp` sources in `SOURCES`
- Add resource files with `DISTRIBUTABLES += $(wildcard res/*)`
- `make` — builds `plugin.so`
- `make install` — packages into `.vcvplugin` and copies to `~/.local/share/Rack2/plugins-lin-x64/`
- Version in `plugin.json` must start with `2` for Rack v2 ABI compatibility

## plugin.json

```json
{
  "slug": "ER",
  "version": "2.0.0",
  "name": "ER Modules",
  "author": "er",
  "license": "CC0-1.0",
  "modules": [
    {
      "slug": "ra-vca",
      "name": "ra-vca",
      "description": "Voltage-controlled amplifier",
      "tags": ["VCA", "Utility"]
    },
    {
      "slug": "ra-knob",
      "name": "ra-knob",
      "description": "4-channel macro CV source with RGB indicators",
      "tags": ["CV source", "Utility"]
    }
  ]
}
```

- `slug` (plugin) — unique identifier, used as brand in module browser
- `slug` (module) — unique per-module, used in patch files, never change after release
- `version` — must be `2.x.x` for Rack v2
- `tags` — controls which category the module appears in

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

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RoundBlackKnob>(Vec(x, y), module, RaVcaModule::GAIN_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, RaVcaModule::AUDIO_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, RaVcaModule::AUDIO_OUTPUT));
    }
};
```

#### Screw Positioning

Screws should be positioned with `RACK_GRID_WIDTH` offset from edges so they
stay fully visible (widget rendering clips to `box.size`):

| Position      | Code                                              |
|---------------|---------------------------------------------------|
| Top‑left      | `Vec(RACK_GRID_WIDTH, 0)`                         |
| Top‑right     | `Vec(box.size.x - RACK_GRID_WIDTH, 0)`            |
| Bottom‑left   | `Vec(RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)` |
| Bottom‑right  | `Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)` |

Setting a screw at `(0, 0)` puts its centre at the panel corner — the left and
top halves are clipped off. Use `(0, 0)` only when you intentionally want
the screw flush with the left/top edges.

## plugin.cpp

```cpp
#include "rack.hpp"

using namespace rack;

Plugin *pluginInstance;
extern Model *modelRaVca;
extern Model *modelRaKnob;

void init(Plugin *p) {
    pluginInstance = p;
    p->addModel(modelRaVca);
    p->addModel(modelRaKnob);
}
```

- `pluginInstance` — global pointer set in `init()`, used by `asset::plugin()` to resolve resource paths
- `extern Model *modelXxx` — forward-declare models defined in other `.cpp` files
- `init()` must have C linkage (declared `extern "C"` in `plugin/callbacks.hpp`), so define it as a plain function

## Parameters

### Param types

| Creator function                | Widget            | Usage                    |
|--------------------------------|-------------------|--------------------------|
| `createParamCentered`          | `RoundBlackKnob`  | Standard macro knob      |
| `createParamCentered`          | `RoundSmallBlackKnob` | Small trim knob      |
| `createParamCentered`          | `CKSS`            | 2‑position toggle switch |
| `createParamCentered`          | `CKSSThree`       | 3‑position toggle switch |

### configParam

```cpp
configParam(paramId, minValue, maxValue, defaultValue, name, unit, displayMultiplier, displayOffset, displayBase);
```

- `name` — label shown in context menu and tooltip
- `unit` — appended after the value (e.g. `" dB"`, `" %"`, `" V"`)
- `displayMultiplier` / `displayOffset` — transform the raw value for display:
  `displayed = raw × displayMultiplier + displayOffset`
- `displayBase` — if non‑zero, display uses `displayMultiplier × base^raw + displayOffset`

Example — gain shown in dB with 0 dB at unity:
```cpp
configParam(GAIN_PARAM, 0.f, 2.f, 1.f, "Gain", " dB", 0.f, 20.f, 0.f);
// raw 0→0.0 dB, raw 1→20*log10(1)=0 dB, raw 2→6.0 dB
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
| `RoundBlackKnob` | Standard knob |
| `RoundSmallBlackKnob` | Small knob |
| `PJ301MPort` | Audio/CV jack (input or output) |
| `CKSS` | 2‑position toggle switch |
| `CKSSThree` | 3‑position toggle switch |
| `ScrewSilver` | Panel screw |
| `TinyLight<RedGreenBlueLight>` | 3 mm RGB LED |
| `MediumLight<RedGreenBlueLight>` | 5 mm RGB LED |
| `LargeLight<RedGreenBlueLight>` | 5 mm RGB LED |

## Port Ordering

Rack renders module params/inputs/outputs in enum order, not addition order.
Keep `NUM_PARAMS` / `NUM_INPUTS` / `NUM_OUTPUTS` / `NUM_LIGHTS` last in
each enum.

## Build Automation

- `make.sh` — cleans, builds, and installs in one step
- Override `RACK_DIR` at invocation: `RACK_DIR=../Rack-SDK ./make.sh`
- After install, restart Rack to pick up the updated `.vcvplugin`

## Common Pitfalls

| Symptom | Cause |
|---------|-------|
| `"Failed to load plugin"` | `version` in `plugin.json` doesn't start with `2` |
| Undefined symbol `init` | Missing `extern "C"` linkage on `init()` |
| Undefined symbol `pluginInstance` | Missing `Plugin *pluginInstance;` in `plugin.cpp` |
| SVGs missing from package | Missing `DISTRIBUTABLES += $(wildcard res/*)` |
| Screws half off panel | Position at edge with `0` instead of `RACK_GRID_WIDTH` offset |
| Lights not showing | `NUM_LIGHTS` too small — count each RGB channel separately |
| Module not in browser | Missing `p->addModel(modelXxx)` in `init()` |
| Module doesn't save state | `plugin.json` slug mismatch with `createModel` string |
