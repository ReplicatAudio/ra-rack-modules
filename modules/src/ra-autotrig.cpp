// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: CV1_INPUT "CV1"
// fname: KNOB1_PARAM "D1"
// fname: DELTA1_CV_INPUT "D1 CV"
// fname: MODE1_PARAM "M1"
// fname: TRIG1_OUTPUT "TR1"
// fname: PASSTHRU1_OUTPUT "P1"
// fname: LED1 "L1"
// fname: CV2_INPUT "CV2"
// fname: KNOB2_PARAM "D2"
// fname: DELTA2_CV_INPUT "D2 CV"
// fname: MODE2_PARAM "M2"
// fname: TRIG2_OUTPUT "TR2"
// fname: PASSTHRU2_OUTPUT "P2"
// fname: LED2 "L2"
// fname: CV3_INPUT "CV3"
// fname: KNOB3_PARAM "D3"
// fname: DELTA3_CV_INPUT "D3 CV"
// fname: MODE3_PARAM "M3"
// fname: TRIG3_OUTPUT "TR3"
// fname: PASSTHRU3_OUTPUT "P3"
// fname: LED3 "L3"
// fname: CV4_INPUT "CV4"
// fname: KNOB4_PARAM "D4"
// fname: DELTA4_CV_INPUT "D4 CV"
// fname: MODE4_PARAM "M4"
// fname: TRIG4_OUTPUT "TR4"
// fname: PASSTHRU4_OUTPUT "P4"
// fname: LED4 "L4"
#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

static constexpr int NUM_CHANNELS = 4;
static constexpr float SEMITONE = 1.f / 12.f;

struct RaAutotrigModule : Module {
    enum ParamIds {
        KNOB1_PARAM,
        KNOB2_PARAM,
        KNOB3_PARAM,
        KNOB4_PARAM,
        MODE1_PARAM,
        MODE2_PARAM,
        MODE3_PARAM,
        MODE4_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CV1_INPUT,
        CV2_INPUT,
        CV3_INPUT,
        CV4_INPUT,
        DELTA1_CV_INPUT,
        DELTA2_CV_INPUT,
        DELTA3_CV_INPUT,
        DELTA4_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        TRIG1_OUTPUT,
        TRIG2_OUTPUT,
        TRIG3_OUTPUT,
        TRIG4_OUTPUT,
        PASSTHRU1_OUTPUT,
        PASSTHRU2_OUTPUT,
        PASSTHRU3_OUTPUT,
        PASSTHRU4_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        LED1,
        LED2,
        LED3,
        LED4,
        NUM_LIGHTS
    };

    float lastValue[NUM_CHANNELS] = {};
    float ledTimer[NUM_CHANNELS] = {};

    RaAutotrigModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < NUM_CHANNELS; i++) {
            configParam(KNOB1_PARAM + i, 0.01f, 10.f, 1.f, string::f("Delta %d", i + 1), " V");
            configSwitch(MODE1_PARAM + i, 0.f, 1.f, 0.f, string::f("Mode %d", i + 1), {"Delta", "1V/Oct"});
            configInput(CV1_INPUT + i, string::f("CV %d", i + 1));
            configInput(DELTA1_CV_INPUT + i, string::f("Delta %d CV", i + 1));
            configOutput(TRIG1_OUTPUT + i, string::f("Trigger %d", i + 1));
            configOutput(PASSTHRU1_OUTPUT + i, string::f("Passthrough %d", i + 1));
        }
    }

    void process(const ProcessArgs &args) override {
        for (int i = 0; i < NUM_CHANNELS; i++) {
            float cv = inputs[CV1_INPUT + i].getVoltage();
            int mode = (int)std::round(params[MODE1_PARAM + i].getValue());

            float delta;
            if (mode == 1) {
                delta = SEMITONE;
            } else {
                delta = params[KNOB1_PARAM + i].getValue() + inputs[DELTA1_CV_INPUT + i].getVoltage();
                delta = std::max(0.01f, delta);
            }

            float diff = cv - lastValue[i];
            if (std::abs(diff) >= delta) {
                outputs[TRIG1_OUTPUT + i].setVoltage(10.f);
                lastValue[i] = cv;
                ledTimer[i] = 0.25f;
            } else {
                outputs[TRIG1_OUTPUT + i].setVoltage(0.f);
            }

            outputs[PASSTHRU1_OUTPUT + i].setVoltage(cv);

            if (ledTimer[i] > 0.f) {
                ledTimer[i] -= args.sampleTime;
                if (ledTimer[i] < 0.f)
                    ledTimer[i] = 0.f;
            }
            lights[LED1 + i].setBrightness(ledTimer[i] > 0.f ? 1.f : 0.f);
        }
    }
};

struct PurpleLight : GrayModuleLightWidget {
    PurpleLight() {
        addBaseColor(nvgRGB(0x99, 0x6d, 0xd2));
    }
};

struct RaAutotrigWidget : ModuleWidget {
    RaAutotrigWidget(RaAutotrigModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-autotrig.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float colX[4] = {22.5f, 57.5f, 92.5f, 127.5f};
        float rowY[6] = {40.f, 75.f, 110.f, 145.f, 180.f, 215.f};

        for (int col = 0; col < 4; col++) {
            float x = colX[col];

            addInput(createInputCentered<RaPort>(Vec(x, rowY[0]), module, RaAutotrigModule::CV1_INPUT + col));
            addParam(createParamCentered<RaKnobTrim>(Vec(x, rowY[1]), module, RaAutotrigModule::KNOB1_PARAM + col));
            addInput(createInputCentered<RaPort>(Vec(x, rowY[2]), module, RaAutotrigModule::DELTA1_CV_INPUT + col));
            addParam(createParamCentered<RaSwitch2>(Vec(x, rowY[3]), module, RaAutotrigModule::MODE1_PARAM + col));
            addChild(createLightCentered<MediumLight<PurpleLight>>(Vec(x, rowY[4]), module, RaAutotrigModule::LED1 + col));
            addOutput(createOutputCentered<RaPort>(Vec(x, rowY[5]), module, RaAutotrigModule::TRIG1_OUTPUT + col));
            addOutput(createOutputCentered<RaPort>(Vec(x, rowY[5] + 30.f), module, RaAutotrigModule::PASSTHRU1_OUTPUT + col));
        }
    }
};

Model *modelRaAutotrig = createModel<RaAutotrigModule, RaAutotrigWidget>("ra-autotrig");
