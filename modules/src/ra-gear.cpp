// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: TEETH_PARAM "Teeth"
// fname: RANGE_PARAM "Range"
// fname: INCREMENT_PARAM "Inc"
// fname: INCREMENT_INPUT "Inc trig"
// fname: DECREMENT_PARAM "Dec"
// fname: DECREMENT_INPUT "Dec trig"
// fname: TRIG_OUTPUT "Trig"
// fname: CV_OUTPUT "CV"
#include "ra-components.hpp"

#include <cmath>
#include <algorithm>

using namespace rack;

extern Plugin *pluginInstance;

struct RaGearModule : Module {
    enum ParamIds {
        TEETH_PARAM,
        RANGE_PARAM,
        INCREMENT_PARAM,
        DECREMENT_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        INCREMENT_INPUT,
        DECREMENT_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        TRIG_OUTPUT,
        CV_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    int teeth = 8;
    int position = 0;
    dsp::SchmittTrigger incrementTrigger;
    dsp::SchmittTrigger decrementTrigger;
    dsp::SchmittTrigger incrementBtnTrigger;
    dsp::SchmittTrigger decrementBtnTrigger;

    RaGearModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(TEETH_PARAM, 3.f, 32.f, 8.f, "Teeth", " teeth", 0.f, 1.f, 0.f);
        configSwitch(RANGE_PARAM, 0.f, 2.f, 1.f, "Range", {"0-1V", "0-10V", "±5V"});
        configButton(INCREMENT_PARAM, "Increment");
        configButton(DECREMENT_PARAM, "Decrement");
        configInput(INCREMENT_INPUT, "Increment trig");
        configInput(DECREMENT_INPUT, "Decrement trig");
        configOutput(TRIG_OUTPUT, "Trig");
        configOutput(CV_OUTPUT, "CV");
    }

    void process(const ProcessArgs &args) override {
        teeth = (int)std::round(params[TEETH_PARAM].getValue());
        int range = (int)std::round(params[RANGE_PARAM].getValue());

        bool cycleComplete = false;

        bool increment = incrementTrigger.process(inputs[INCREMENT_INPUT].getVoltage());
        increment |= incrementBtnTrigger.process(params[INCREMENT_PARAM].getValue());
        if (increment) {
            position++;
            if (position >= teeth) {
                position = 0;
                cycleComplete = true;
            }
        }

        bool decrement = decrementTrigger.process(inputs[DECREMENT_INPUT].getVoltage());
        decrement |= decrementBtnTrigger.process(params[DECREMENT_PARAM].getValue());
        if (decrement) {
            position--;
            if (position < 0) {
                position = teeth - 1;
                cycleComplete = true;
            }
        }

        if (cycleComplete) {
            outputs[TRIG_OUTPUT].setVoltage(10.f);
        } else {
            outputs[TRIG_OUTPUT].setVoltage(0.f);
        }

        float cv = (float)position / (float)teeth;
        switch (range) {
            case 0: cv *= 1.f; break;
            case 1: cv *= 10.f; break;
            case 2: cv = cv * 10.f - 5.f; break;
        }
        outputs[CV_OUTPUT].setVoltage(cv);
    }
};

struct GearDisplay : LedDisplay {
    RaGearModule *module;

    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, -3, -3, box.size.x + 6, box.size.y + 6, 4);
        nvgFillColor(args.vg, nvgRGB(0x10, 0x10, 0x10));
        nvgFill(args.vg);
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStrokeColor(args.vg, nvgRGB(0x4a, 0x40, 0x66));
        nvgStroke(args.vg);

        if (!module) return;

        float cx = box.size.x / 2;
        float cy = box.size.y / 2;
        float radius = std::min(box.size.x, box.size.y) / 2.f - 4.f;
        float angle = (float)module->position / (float)module->teeth * 2.f * M_PI;

        nvgBeginPath(args.vg);
        int teeth = module->teeth;
        float toothAngle = 2.f * M_PI / (float)teeth;
        for (int i = 0; i < teeth; i++) {
            float a = angle + i * toothAngle;
            float r1 = radius * 0.7f;
            float r2 = radius;
            float x1 = cx + r1 * cosf(a);
            float y1 = cy + r1 * sinf(a);
            float x2 = cx + r2 * cosf(a);
            float y2 = cy + r2 * sinf(a);
            if (i == 0) {
                nvgMoveTo(args.vg, x1, y1);
            } else {
                nvgLineTo(args.vg, x1, y1);
            }
            nvgLineTo(args.vg, x2, y2);
        }
        nvgClosePath(args.vg);
        nvgFillColor(args.vg, nvgRGB(0x99, 0x6d, 0xd2));
        nvgFill(args.vg);

        nvgBeginPath(args.vg);
        nvgCircle(args.vg, cx, cy, radius * 0.3f);
        nvgFillColor(args.vg, nvgRGB(0x10, 0x10, 0x10));
        nvgFill(args.vg);
    }
};

struct RaGearWidget : ModuleWidget {
    RaGearWidget(RaGearModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-gear.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float colX[2] = {30.f, 75.f};
        float displayY = 20.f;
        float displayH = 60.f;
        float rowY[4] = {100.f, 160.f, 220.f, 280.f};

        auto *display = new GearDisplay();
        display->box.pos = Vec(15, displayY);
        display->box.size = Vec(60, displayH);
        display->module = module;
        addChild(display);

        addParam(createParamCentered<RaKnobTrim>(Vec(colX[0], rowY[0]), module, RaGearModule::TEETH_PARAM));
        addParam(createParamCentered<RaSwitch3>(Vec(colX[1], rowY[0]), module, RaGearModule::RANGE_PARAM));

        addParam(createParamCentered<RaButton>(Vec(colX[0], rowY[1]), module, RaGearModule::INCREMENT_PARAM));
        addParam(createParamCentered<RaButton>(Vec(colX[1], rowY[1]), module, RaGearModule::DECREMENT_PARAM));

        addInput(createInputCentered<RaPort>(Vec(colX[0], rowY[2]), module, RaGearModule::INCREMENT_INPUT));
        addInput(createInputCentered<RaPort>(Vec(colX[1], rowY[2]), module, RaGearModule::DECREMENT_INPUT));

        addOutput(createOutputCentered<RaPort>(Vec(colX[0], rowY[3]), module, RaGearModule::TRIG_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(colX[1], rowY[3]), module, RaGearModule::CV_OUTPUT));
    }
};

Model *modelRaGear = createModel<RaGearModule, RaGearWidget>("ra-gear");
