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
// fname: ATTN1_PARAM "Attn1"
// fname: CLAMP1_PARAM "C1"
// fname: MODE2_PARAM "M2"
// fname: KNOB2A_PARAM "A2"
// fname: KNOB2B_PARAM "B2"
// fname: IN2A_INPUT "In2A"
// fname: IN2B_INPUT "In2B"
// fname: OUT2_OUTPUT "Out2"
// fname: ATTN2_PARAM "Attn2"
// fname: CLAMP2_PARAM "C2"
// fname: MODE3_PARAM "M3"
// fname: KNOB3A_PARAM "A3"
// fname: KNOB3B_PARAM "B3"
// fname: IN3A_INPUT "In3A"
// fname: IN3B_INPUT "In3B"
// fname: OUT3_OUTPUT "Out3"
// fname: ATTN3_PARAM "Attn3"
// fname: CLAMP3_PARAM "C3"
// fname: MODE4_PARAM "M4"
// fname: KNOB4A_PARAM "A4"
// fname: KNOB4B_PARAM "B4"
// fname: IN4A_INPUT "In4A"
// fname: IN4B_INPUT "In4B"
// fname: OUT4_OUTPUT "Out4"
// fname: ATTN4_PARAM "Attn4"
// fname: CLAMP4_PARAM "C4"
#include "ra-components.hpp"

#include <cmath>
#include <cstring>
#include <cstdio>

using namespace rack;

extern Plugin *pluginInstance;

static constexpr int NUM_CHANNELS = 4;
static constexpr int NUM_MODES = 17;

static const char *MODE_NAMES[NUM_MODES] = {
    "ADD", "SUB", "MULT", "DIV", "POW", "MOD", "MAX", "MIN",
    "AVG", "ABS", "SQRT", "FLOOR", "CEIL", "ROUND", "SIN", "COS", "LOG"
};

struct PurpleLight : GrayModuleLightWidget {
    PurpleLight() {
        addBaseColor(nvgRGB(0x99, 0x6d, 0xd2));
    }
};

struct RaCalcModule : Module {
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
        ATTN1_PARAM,
        ATTN2_PARAM,
        ATTN3_PARAM,
        ATTN4_PARAM,
        CLAMP1_PARAM,
        CLAMP2_PARAM,
        CLAMP3_PARAM,
        CLAMP4_PARAM,
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
        CLAMP1_LIGHT,
        CLAMP2_LIGHT,
        CLAMP3_LIGHT,
        CLAMP4_LIGHT,
        NUM_LIGHTS
    };

    int modes[NUM_CHANNELS] = {0, 1, 2, 3};
    dsp::SchmittTrigger modeTriggers[NUM_CHANNELS];
    bool clampState[NUM_CHANNELS] = {};
    dsp::SchmittTrigger clampTriggers[NUM_CHANNELS];

    RaCalcModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < NUM_CHANNELS; i++) {
            configButton(MODE1_PARAM + i, string::f("Mode %d", i + 1));
            configParam(KNOB1A_PARAM + i * 2, -10.f, 10.f, 0.f, string::f("A %d", i + 1), " V");
            configParam(KNOB1B_PARAM + i * 2, -10.f, 10.f, 0.f, string::f("B %d", i + 1), " V");
            configInput(IN1A_INPUT + i * 2, string::f("In %d A", i + 1));
            configInput(IN1B_INPUT + i * 2, string::f("In %d B", i + 1));
            configOutput(OUT1_OUTPUT + i, string::f("Out %d", i + 1));
            configParam(ATTN1_PARAM + i, -1.f, 1.f, 1.f, string::f("Attn %d", i + 1), "x", 0.f, 1.f, 0.f);
            configParam(CLAMP1_PARAM + i, 0.f, 1.f, 0.f, string::f("Clamp %d", i + 1));
            configLight(CLAMP1_LIGHT + i, string::f("Clamp %d LED", i + 1));
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
                case 16: result = std::log(std::abs(a + b)); break;
                default: result = 0.f; break;
            }

            if (clampTriggers[i].process(params[CLAMP1_PARAM + i].getValue()))
                clampState[i] = !clampState[i];

            if (clampState[i]) {
                result = std::max(-10.f, std::min(result, 10.f));
            }

            float attn = params[ATTN1_PARAM + i].getValue();
            result = result * attn;

            lights[CLAMP1_LIGHT + i].setBrightness(clampState[i] ? 1.f : 0.f);

            outputs[OUT1_OUTPUT + i].setVoltage(result);
        }
    }
};

struct MathDisplay : LedDisplay {
    RaCalcModule *module;
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

struct ValueDisplay : LedDisplay {
    RaCalcModule *module;
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

        float a = module->inputs[RaCalcModule::IN1A_INPUT + channel * 2].getVoltage();
        float b = module->inputs[RaCalcModule::IN1B_INPUT + channel * 2].getVoltage();
        float out = module->outputs[RaCalcModule::OUT1_OUTPUT + channel].getVoltage();

        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgFontSize(args.vg, 11);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

        char buf[16];
        float y = box.size.y / 4.f;

        const char *fmtA = (std::abs(a) >= 10.f) ? "%+.0f" : "%+.2f";
        const char *fmtB = (std::abs(b) >= 10.f) ? "%+.0f" : "%+.2f";
        const char *fmtOut = (std::abs(out) >= 10.f) ? "%+.0f" : "%+.2f";

        nvgFillColor(args.vg, nvgRGB(0x7c, 0xfc, 0x00));
        snprintf(buf, sizeof(buf), fmtA, a);
        if (std::isinf(a)) nvgText(args.vg, box.size.x / 2, y, "∞", NULL);
        else if (strlen(buf) > 6) nvgText(args.vg, box.size.x / 2, y, "...", NULL);
        else nvgText(args.vg, box.size.x / 2, y, buf, NULL);

        nvgFillColor(args.vg, nvgRGB(0x7c, 0xfc, 0x00));
        snprintf(buf, sizeof(buf), fmtB, b);
        if (std::isinf(b)) nvgText(args.vg, box.size.x / 2, y * 2.f, "∞", NULL);
        else if (strlen(buf) > 6) nvgText(args.vg, box.size.x / 2, y * 2.f, "...", NULL);
        else nvgText(args.vg, box.size.x / 2, y * 2.f, buf, NULL);

        nvgFillColor(args.vg, nvgRGB(0x99, 0x6d, 0xd2));
        snprintf(buf, sizeof(buf), fmtOut, out);
        if (std::isinf(out)) nvgText(args.vg, box.size.x / 2, y * 3.f, "∞", NULL);
        else if (strlen(buf) > 6) nvgText(args.vg, box.size.x / 2, y * 3.f, "...", NULL);
        else nvgText(args.vg, box.size.x / 2, y * 3.f, buf, NULL);
    }
};

struct RaCalcWidget : ModuleWidget {
    RaCalcWidget(RaCalcModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-math.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float colX[4] = {36.f, 72.f, 108.f, 144.f};
        float rowY = 45.f;

        for (int i = 0; i < 4; i++) {
            float x = colX[i];

            MathDisplay *display = createWidget<MathDisplay>(Vec(x - 15, rowY));
            display->box.size = Vec(30, 20);
            display->module = module;
            display->channel = i;
            addChild(display);

            addParam(createParamCentered<RaButton>(Vec(x, rowY + 25), module, RaCalcModule::MODE1_PARAM + i));

            addParam(createParamCentered<RaKnobTrim>(Vec(x, rowY + 55), module, RaCalcModule::KNOB1A_PARAM + i * 2));
            addInput(createInputCentered<RaPort>(Vec(x, rowY + 85), module, RaCalcModule::IN1A_INPUT + i * 2));

            addParam(createParamCentered<RaKnobTrim>(Vec(x, rowY + 115), module, RaCalcModule::KNOB1B_PARAM + i * 2));
            addInput(createInputCentered<RaPort>(Vec(x, rowY + 145), module, RaCalcModule::IN1B_INPUT + i * 2));

            addOutput(createOutputCentered<RaPort>(Vec(x, rowY + 175), module, RaCalcModule::OUT1_OUTPUT + i));

            addParam(createParamCentered<RaKnobTrim>(Vec(x, rowY + 205), module, RaCalcModule::ATTN1_PARAM + i));

            ValueDisplay *valueDisplay = createWidget<ValueDisplay>(Vec(x - 15, rowY + 225));
            valueDisplay->box.size = Vec(30, 60);
            valueDisplay->module = module;
            valueDisplay->channel = i;
            addChild(valueDisplay);

            addParam(createLightParamCentered<VCVLightBezel<PurpleLight>>(Vec(x, rowY + 305), module, RaCalcModule::CLAMP1_PARAM + i, RaCalcModule::CLAMP1_LIGHT + i));
        }
    }
};

Model *modelRaCalc = createModel<RaCalcModule, RaCalcWidget>("ra-calc");
