// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: CV1_INPUT "CV1"
// fname: KNOB1_PARAM "T1"
// fname: TRIG1_OUTPUT "TR1"
// fname: PASSTHRU1_OUTPUT "P1"
// fname: CV2_INPUT "CV2"
// fname: KNOB2_PARAM "T2"
// fname: TRIG2_OUTPUT "TR2"
// fname: PASSTHRU2_OUTPUT "P2"
// fname: CV3_INPUT "CV3"
// fname: KNOB3_PARAM "T3"
// fname: TRIG3_OUTPUT "TR3"
// fname: PASSTHRU3_OUTPUT "P3"
// fname: CV4_INPUT "CV4"
// fname: KNOB4_PARAM "T4"
// fname: TRIG4_OUTPUT "TR4"
// fname: PASSTHRU4_OUTPUT "P4"
// fname: CV5_INPUT "CV5"
// fname: KNOB5_PARAM "T5"
// fname: TRIG5_OUTPUT "TR5"
// fname: PASSTHRU5_OUTPUT "P5"
// fname: CV6_INPUT "CV6"
// fname: KNOB6_PARAM "T6"
// fname: TRIG6_OUTPUT "TR6"
// fname: PASSTHRU6_OUTPUT "P6"
// fname: CV7_INPUT "CV7"
// fname: KNOB7_PARAM "T7"
// fname: TRIG7_OUTPUT "TR7"
// fname: PASSTHRU7_OUTPUT "P7"
// fname: CV8_INPUT "CV8"
// fname: KNOB8_PARAM "T8"
// fname: TRIG8_OUTPUT "TR8"
// fname: PASSTHRU8_OUTPUT "P8"
#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

static constexpr int NUM_CHANNELS = 8;
static constexpr float HYSTERESIS = 0.01f;

struct RaAutotrigModule : Module {
    enum ParamIds {
        KNOB1_PARAM,
        KNOB2_PARAM,
        KNOB3_PARAM,
        KNOB4_PARAM,
        KNOB5_PARAM,
        KNOB6_PARAM,
        KNOB7_PARAM,
        KNOB8_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CV1_INPUT,
        CV2_INPUT,
        CV3_INPUT,
        CV4_INPUT,
        CV5_INPUT,
        CV6_INPUT,
        CV7_INPUT,
        CV8_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        TRIG1_OUTPUT,
        TRIG2_OUTPUT,
        TRIG3_OUTPUT,
        TRIG4_OUTPUT,
        TRIG5_OUTPUT,
        TRIG6_OUTPUT,
        TRIG7_OUTPUT,
        TRIG8_OUTPUT,
        PASSTHRU1_OUTPUT,
        PASSTHRU2_OUTPUT,
        PASSTHRU3_OUTPUT,
        PASSTHRU4_OUTPUT,
        PASSTHRU5_OUTPUT,
        PASSTHRU6_OUTPUT,
        PASSTHRU7_OUTPUT,
        PASSTHRU8_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    bool trigState[NUM_CHANNELS] = {};

    RaAutotrigModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < NUM_CHANNELS; i++) {
            configParam(KNOB1_PARAM + i, 0.f, 10.f, 5.f, string::f("Threshold %d", i + 1));
            configInput(CV1_INPUT + i, string::f("CV %d", i + 1));
            configOutput(TRIG1_OUTPUT + i, string::f("Trigger %d", i + 1));
            configOutput(PASSTHRU1_OUTPUT + i, string::f("Passthrough %d", i + 1));
        }
    }

    void process(const ProcessArgs &args) override {
        for (int i = 0; i < NUM_CHANNELS; i++) {
            float cv = inputs[CV1_INPUT + i].getVoltage();
            float threshold = params[KNOB1_PARAM + i].getValue();

            if (trigState[i]) {
                if (cv < threshold - HYSTERESIS)
                    trigState[i] = false;
            } else {
                if (cv > threshold + HYSTERESIS)
                    trigState[i] = true;
            }

            outputs[TRIG1_OUTPUT + i].setVoltage(trigState[i] ? 10.f : 0.f);
            outputs[PASSTHRU1_OUTPUT + i].setVoltage(cv);
        }
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

        float colX[4] = {20.f, 55.f, 90.f, 125.f};
        float rowY[2] = {40.f, 200.f};

        for (int row = 0; row < 2; row++) {
            for (int col = 0; col < 4; col++) {
                int ch = row * 4 + col;
                float x = colX[col];
                float yBase = rowY[row];

                addInput(createInputCentered<RaPort>(Vec(x, yBase), module, RaAutotrigModule::CV1_INPUT + ch));
                addParam(createParamCentered<RaKnobTrim>(Vec(x, yBase + 35), module, RaAutotrigModule::KNOB1_PARAM + ch));
                addOutput(createOutputCentered<RaPort>(Vec(x, yBase + 70), module, RaAutotrigModule::TRIG1_OUTPUT + ch));
                addOutput(createOutputCentered<RaPort>(Vec(x, yBase + 105), module, RaAutotrigModule::PASSTHRU1_OUTPUT + ch));
            }
        }
    }
};

Model *modelRaAutotrig = createModel<RaAutotrigModule, RaAutotrigWidget>("ra-autotrig");
