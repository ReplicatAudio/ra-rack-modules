// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: MODE1_PARAM "M1"
// fname: MODE2_PARAM "M2"
// fname: MODE3_PARAM "M3"
// fname: MODE4_PARAM "M4"
// fname: MODE5_PARAM "M5"
// fname: MODE6_PARAM "M6"
// fname: MODE7_PARAM "M7"
// fname: MODE8_PARAM "M8"
// fname: A1_INPUT "A1"
// fname: B1_INPUT "B1"
// fname: A2_INPUT "A2"
// fname: B2_INPUT "B2"
// fname: A3_INPUT "A3"
// fname: B3_INPUT "B3"
// fname: A4_INPUT "A4"
// fname: B4_INPUT "B4"
// fname: A5_INPUT "A5"
// fname: B5_INPUT "B5"
// fname: A6_INPUT "A6"
// fname: B6_INPUT "B6"
// fname: A7_INPUT "A7"
// fname: B7_INPUT "B7"
// fname: A8_INPUT "A8"
// fname: B8_INPUT "B8"
// fname: OUT1_OUTPUT "O1"
// fname: OUT2_OUTPUT "O2"
// fname: OUT3_OUTPUT "O3"
// fname: OUT4_OUTPUT "O4"
// fname: OUT5_OUTPUT "O5"
// fname: OUT6_OUTPUT "O6"
// fname: OUT7_OUTPUT "O7"
// fname: OUT8_OUTPUT "O8"
#include "ra-components.hpp"

#include <cstring>

using namespace rack;

extern Plugin *pluginInstance;

static constexpr int NUM_GATES = 8;
static constexpr float HIGH_THRESHOLD = 1.f; // inputs above 1 V count as high
static constexpr float HIGH_VOLTS = 10.f;    // output voltage when high

// Gate modes — order defines the cycle of the mode button
enum GateMode {
    AND_MODE,
    OR_MODE,
    XOR_MODE,
    NAND_MODE,
    NOR_MODE,
    XNOR_MODE,
    NOT_A_MODE,
    NOT_B_MODE,
    NUM_MODES
};

// Code shown on the per-gate screen for each mode
static const char *GATE_CODES[NUM_MODES] = {
    "AND", "OR", "XOR", "NAND", "NOR", "XNOR", "NOTA", "NOTB"
};

struct RaLogicModule : Module {
    enum ParamIds {
        MODE1_PARAM,
        MODE2_PARAM,
        MODE3_PARAM,
        MODE4_PARAM,
        MODE5_PARAM,
        MODE6_PARAM,
        MODE7_PARAM,
        MODE8_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        A1_INPUT, B1_INPUT,
        A2_INPUT, B2_INPUT,
        A3_INPUT, B3_INPUT,
        A4_INPUT, B4_INPUT,
        A5_INPUT, B5_INPUT,
        A6_INPUT, B6_INPUT,
        A7_INPUT, B7_INPUT,
        A8_INPUT, B8_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT1_OUTPUT,
        OUT2_OUTPUT,
        OUT3_OUTPUT,
        OUT4_OUTPUT,
        OUT5_OUTPUT,
        OUT6_OUTPUT,
        OUT7_OUTPUT,
        OUT8_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    // Selected mode per gate
    int gateModes[NUM_GATES];
    dsp::SchmittTrigger modeTriggers[NUM_GATES];

    RaLogicModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < NUM_GATES; i++) {
            configButton(MODE1_PARAM + i, string::f("Gate %d mode", i + 1));
            configInput(A1_INPUT + 2 * i, string::f("Gate %d A", i + 1));
            configInput(B1_INPUT + 2 * i, string::f("Gate %d B", i + 1));
            configOutput(OUT1_OUTPUT + i, string::f("Gate %d out", i + 1));
        }
        onReset();
    }

    void onReset() override {
        // Default each gate to a different mode so all 8 are visible on load
        for (int i = 0; i < NUM_GATES; i++) {
            gateModes[i] = i % NUM_MODES;
        }
    }

    void process(const ProcessArgs &args) override {
        for (int i = 0; i < NUM_GATES; i++) {
            // Mode button — advance one mode per press
            if (modeTriggers[i].process(params[MODE1_PARAM + i].getValue()))
                gateModes[i] = (gateModes[i] + 1) % NUM_MODES;

            bool a = inputs[A1_INPUT + 2 * i].getVoltage() > HIGH_THRESHOLD;
            bool b = inputs[B1_INPUT + 2 * i].getVoltage() > HIGH_THRESHOLD;

            bool out;
            switch (gateModes[i]) {
                case AND_MODE: out = a && b; break;
                case OR_MODE: out = a || b; break;
                case XOR_MODE: out = a != b; break;
                case NAND_MODE: out = !(a && b); break;
                case NOR_MODE: out = !(a || b); break;
                case XNOR_MODE: out = a == b; break;
                case NOT_A_MODE: out = !a; break;
                case NOT_B_MODE: out = !b; break;
                default: out = false; break;
            }

            outputs[OUT1_OUTPUT + i].setVoltage(out ? HIGH_VOLTS : 0.f);
        }
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        json_t *modesJ = json_array();
        for (int i = 0; i < NUM_GATES; i++) {
            json_array_insert_new(modesJ, i, json_integer(gateModes[i]));
        }
        json_object_set_new(rootJ, "gateModes", modesJ);
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *modesJ = json_object_get(rootJ, "gateModes");
        if (modesJ) {
            for (int i = 0; i < NUM_GATES; i++) {
                json_t *modeJ = json_array_get(modesJ, i);
                if (modeJ)
                    gateModes[i] = clamp(json_integer_value(modeJ), 0, NUM_MODES - 1);
            }
        }
    }
};

// Small screen showing the selected gate's code
struct GateDisplay : LedDisplay {
    RaLogicModule *module;
    int gate = 0;

    void draw(const DrawArgs &args) override {
        // Screen backdrop — painted slightly larger than the box to cover the
        // SVG bezel outline, recolored with a muted purple border to match the accent
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, -3, -3, box.size.x + 6, box.size.y + 6, 4);
        nvgFillColor(args.vg, nvgRGB(0x0a, 0x0a, 0x0a));
        nvgFill(args.vg);
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStrokeColor(args.vg, nvgRGB(0x4a, 0x40, 0x66));
        nvgStroke(args.vg);

        const char *code = GATE_CODES[0];
        if (module)
            code = GATE_CODES[module->gateModes[gate]];

        // Shrink the font for longer codes so they fit the small screen
        int len = strlen(code);
        float size = len <= 3 ? 9.f : (len == 4 ? 8.f : 7.f);

        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgFontSize(args.vg, size);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(args.vg, nvgRGB(0x99, 0x6d, 0xd2));
        nvgText(args.vg, box.size.x / 2, box.size.y / 2, code, NULL);
    }
};

struct RaLogicWidget : ModuleWidget {
    RaLogicWidget(RaLogicModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-logic.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        // 2 rows of 4 gates
        float colX[4] = {18.75f, 56.25f, 93.75f, 131.25f};

        // Top row: gates 1-4
        float rowY = 28.f;
        for (int i = 0; i < 4; i++) {
            int col = i % 4;
            float x = colX[col];

            // Screen above the mode button
            GateDisplay *display = createWidget<GateDisplay>(Vec(x - 12, rowY + 8));
            display->box.size = Vec(24, 15);
            display->module = module;
            display->gate = i;
            addChild(display);

            addParam(createParamCentered<RaButton>(Vec(x, rowY + 36), module, RaLogicModule::MODE1_PARAM + i));
            addInput(createInputCentered<RaPort>(Vec(x, rowY + 70), module, RaLogicModule::A1_INPUT + 2 * i));
            addInput(createInputCentered<RaPort>(Vec(x, rowY + 104), module, RaLogicModule::B1_INPUT + 2 * i));
            addOutput(createOutputCentered<RaPort>(Vec(x, rowY + 138), module, RaLogicModule::OUT1_OUTPUT + i));
        }

        // Bottom row: gates 5-8
        rowY = 204.f;
        for (int i = 4; i < 8; i++) {
            int col = i % 4;
            float x = colX[col];

            // Screen above the mode button
            GateDisplay *display = createWidget<GateDisplay>(Vec(x - 12, rowY + 8));
            display->box.size = Vec(24, 15);
            display->module = module;
            display->gate = i;
            addChild(display);

            addParam(createParamCentered<RaButton>(Vec(x, rowY + 36), module, RaLogicModule::MODE1_PARAM + i));
            addInput(createInputCentered<RaPort>(Vec(x, rowY + 70), module, RaLogicModule::A1_INPUT + 2 * i));
            addInput(createInputCentered<RaPort>(Vec(x, rowY + 104), module, RaLogicModule::B1_INPUT + 2 * i));
            addOutput(createOutputCentered<RaPort>(Vec(x, rowY + 138), module, RaLogicModule::OUT1_OUTPUT + i));
        }
    }
};

Model *modelRaLogic = createModel<RaLogicModule, RaLogicWidget>("ra-logic");
