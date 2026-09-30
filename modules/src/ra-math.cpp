// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: MODE1_PARAM "M1"
// fname: KNOB1A_PARAM "A1"
// fname: KNOB1B_PARAM "B1"
// fname: IN1A_INPUT "In1A"
// fname: IN1B_INPUT "In1B"
// fname: OUT1_OUTPUT "Out1"
// fname: MODE2_PARAM "M2"
// fname: KNOB2A_PARAM "A2"
// fname: KNOB2B_PARAM "B2"
// fname: IN2A_INPUT "In2A"
// fname: IN2B_INPUT "In2B"
// fname: OUT2_OUTPUT "Out2"
// fname: MODE3_PARAM "M3"
// fname: KNOB3A_PARAM "A3"
// fname: KNOB3B_PARAM "B3"
// fname: IN3A_INPUT "In3A"
// fname: IN3B_INPUT "In3B"
// fname: OUT3_OUTPUT "Out3"
// fname: MODE4_PARAM "M4"
// fname: KNOB4A_PARAM "A4"
// fname: KNOB4B_PARAM "B4"
// fname: IN4A_INPUT "In4A"
// fname: IN4B_INPUT "In4B"
// fname: OUT4_OUTPUT "Out4"
#include "ra-components.hpp"

#include <cmath>
#include <cstring>

using namespace rack;

extern Plugin *pluginInstance;

static constexpr int NUM_CHANNELS = 4;
static constexpr int NUM_MODES = 16;

static const char *MODE_NAMES[NUM_MODES] = {
    "ADD", "SUB", "MULT", "DIV", "POW", "MOD", "MAX", "MIN",
    "AVG", "ABS", "SQRT", "FLOOR", "CEIL", "ROUND", "SIN", "COS"
};

struct RaMathModule : Module {
    enum ParamIds {
        MODE1_PARAM,
        MODE2_PARAM,
        MODE3_PARAM,
        MODE4_PARAM,
        KNOB1A_PARAM,
        KNOB1B_PARAM,
        KNOB2A_PARAM,
        KNOB2B_PARAM,
        KNOB3A_PARAM,
        KNOB3B_PARAM,
        KNOB4A_PARAM,
        KNOB4B_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        IN1A_INPUT,
        IN1B_INPUT,
        IN2A_INPUT,
        IN2B_INPUT,
        IN3A_INPUT,
        IN3B_INPUT,
        IN4A_INPUT,
        IN4B_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT1_OUTPUT,
        OUT2_OUTPUT,
        OUT3_OUTPUT,
        OUT4_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    int modes[NUM_CHANNELS] = {0, 1, 2, 3};
    dsp::SchmittTrigger modeTriggers[NUM_CHANNELS];

    RaMathModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < NUM_CHANNELS; i++) {
            configButton(MODE1_PARAM + i, string::f("Mode %d", i + 1));
            configParam(KNOB1A_PARAM + i * 2, -10.f, 10.f, 0.f, string::f("A %d", i + 1), " V");
            configParam(KNOB1B_PARAM + i * 2, -10.f, 10.f, 0.f, string::f("B %d", i + 1), " V");
            configInput(IN1A_INPUT + i * 2, string::f("In %d A", i + 1));
            configInput(IN1B_INPUT + i * 2, string::f("In %d B", i + 1));
            configOutput(OUT1_OUTPUT + i, string::f("Out %d", i + 1));
        }
    }

    void process(const ProcessArgs &args) override {
        for (int i = 0; i < NUM_CHANNELS; i++) {
            if (modeTriggers[i].process(params[MODE1_PARAM + i].getValue()))
                modes[i] = (modes[i] + 1) % NUM_MODES;

            float a = inputs[IN1A_INPUT + i * 2].isConnected()
                ? inputs[IN1A_INPUT + i * 2].getVoltage()
                : params[KNOB1A_PARAM + i * 2].getValue();
            float b = inputs[IN1B_INPUT + i * 2].isConnected()
                ? inputs[IN1B_INPUT + i * 2].getVoltage()
                : params[KNOB1B_PARAM + i * 2].getValue();

            float result;
            switch (modes[i]) {
                case 0: result = a + b; break;
                case 1: result = a - b; break;
                case 2: result = a * b; break;
                case 3: result = (b != 0.f) ? a / b : 0.f; break;
                case 4: result = std::pow(a, b); break;
                case 5: result = (b != 0.f) ? std::fmod(a, b) : 0.f; break;
                case 6: result = std::max(a, b); break;
                case 7: result = std::min(a, b); break;
                case 8: result = (a + b) / 2.f; break;
                case 9: result = std::abs(a + b); break;
                case 10: result = std::sqrt(std::abs(a + b)); break;
                case 11: result = std::floor(a + b); break;
                case 12: result = std::ceil(a + b); break;
                case 13: result = std::round(a + b); break;
                case 14: result = std::sin(a + b); break;
                case 15: result = std::cos(a + b); break;
                default: result = 0.f; break;
            }

            outputs[OUT1_OUTPUT + i].setVoltage(result);
        }
    }
};

struct MathDisplay : LedDisplay {
    RaMathModule *module;
    int channel = 0;

    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, -3, -3, box.size.x + 6, box.size.y + 6, 4);
        nvgFillColor(args.vg, nvgRGB(0x0a, 0x0a, 0x0a));
        nvgFill(args.vg);
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStrokeColor(args.vg, nvgRGB(0x4a, 0x40, 0x66));
        nvgStroke(args.vg);

        if (!module) return;

        const char *name = MODE_NAMES[module->modes[channel]];

        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgFontSize(args.vg, 10);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(args.vg, nvgRGB(0x99, 0x6d, 0xd2));
        nvgText(args.vg, box.size.x / 2, box.size.y / 2, name, NULL);
    }
};

struct RaMathWidget : ModuleWidget {
    RaMathWidget(RaMathModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-math.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float colX[4] = {22.f, 60.f, 98.f, 136.f};
        float rowY = 40.f;

        for (int i = 0; i < 4; i++) {
            float x = colX[i];

            MathDisplay *display = createWidget<MathDisplay>(Vec(x - 15, rowY));
            display->box.size = Vec(30, 20);
            display->module = module;
            display->channel = i;
            addChild(display);

            addParam(createParamCentered<RaButton>(Vec(x, rowY + 25), module, RaMathModule::MODE1_PARAM + i));

            addParam(createParamCentered<RaKnobTrim>(Vec(x, rowY + 55), module, RaMathModule::KNOB1A_PARAM + i * 2));
            addInput(createInputCentered<RaPort>(Vec(x, rowY + 85), module, RaMathModule::IN1A_INPUT + i * 2));

            addParam(createParamCentered<RaKnobTrim>(Vec(x, rowY + 115), module, RaMathModule::KNOB1B_PARAM + i * 2));
            addInput(createInputCentered<RaPort>(Vec(x, rowY + 145), module, RaMathModule::IN1B_INPUT + i * 2));

            addOutput(createOutputCentered<RaPort>(Vec(x, rowY + 180), module, RaMathModule::OUT1_OUTPUT + i));
        }
    }
};

Model *modelRaMath = createModel<RaMathModule, RaMathWidget>("ra-math");
