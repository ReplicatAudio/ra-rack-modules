// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: A1_INPUT "A1"
// fname: B1_INPUT "B1"
// fname: MODE1_PARAM "Mode1"
// fname: OUT1_OUTPUT "Out1"
// fname: LED1_A "A1"
// fname: LED1_B "B1"
// fname: A2_INPUT "A2"
// fname: B2_INPUT "B2"
// fname: MODE2_PARAM "Mode2"
// fname: OUT2_OUTPUT "Out2"
// fname: LED2_A "A2"
// fname: LED2_B "B2"
// fname: A3_INPUT "A3"
// fname: B3_INPUT "B3"
// fname: MODE3_PARAM "Mode3"
// fname: OUT3_OUTPUT "Out3"
// fname: LED3_A "A3"
// fname: LED3_B "B3"
#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

static constexpr int NUM_CHANNELS = 3;

struct RaMinmaxModule : Module {
    enum ParamIds {
        MODE1_PARAM,
        MODE2_PARAM,
        MODE3_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        A1_INPUT,
        B1_INPUT,
        A2_INPUT,
        B2_INPUT,
        A3_INPUT,
        B3_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT1_OUTPUT,
        OUT2_OUTPUT,
        OUT3_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        LED1_A,
        LED1_B,
        LED2_A,
        LED2_B,
        LED3_A,
        LED3_B,
        NUM_LIGHTS
    };

    RaMinmaxModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < NUM_CHANNELS; i++) {
            configSwitch(MODE1_PARAM + i, 0.f, 1.f, 0.f, string::f("Mode %d", i + 1), {"Min", "Max"});
            configInput(A1_INPUT + i * 2, string::f("A %d", i + 1));
            configInput(B1_INPUT + i * 2, string::f("B %d", i + 1));
            configOutput(OUT1_OUTPUT + i, string::f("Out %d", i + 1));
            configLight(LED1_A + i * 2, string::f("A %d active", i + 1));
            configLight(LED1_B + i * 2, string::f("B %d active", i + 1));
        }
    }

    void process(const ProcessArgs &args) override {
        for (int i = 0; i < NUM_CHANNELS; i++) {
            float a = inputs[A1_INPUT + i * 2].getVoltage();
            float b = inputs[B1_INPUT + i * 2].getVoltage();
            int mode = (int)std::round(params[MODE1_PARAM + i].getValue());

            float out;
            if (mode == 0) {
                out = std::min(a, b);
                lights[LED1_A + i * 2].setBrightness(a <= b ? 1.f : 0.f);
                lights[LED1_B + i * 2].setBrightness(b < a ? 1.f : 0.f);
            } else {
                out = std::max(a, b);
                lights[LED1_A + i * 2].setBrightness(a >= b ? 1.f : 0.f);
                lights[LED1_B + i * 2].setBrightness(b > a ? 1.f : 0.f);
            }

            outputs[OUT1_OUTPUT + i].setVoltage(out);
        }
    }
};

struct RaMinmaxWidget : ModuleWidget {
    RaMinmaxWidget(RaMinmaxModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-minmax.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float colX[3] = {20.f, 45.f, 70.f};
        float rowY[3] = {40.f, 130.f, 220.f};

        for (int i = 0; i < 3; i++) {
            float y = rowY[i];

            addInput(createInputCentered<RaPort>(Vec(colX[0], y), module, RaMinmaxModule::A1_INPUT + i * 2));
            addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(Vec(colX[0], y + 25.f), module, RaMinmaxModule::LED1_A + i * 2));

            addParam(createParamCentered<RaSwitch2>(Vec(colX[1], y), module, RaMinmaxModule::MODE1_PARAM + i));

            addInput(createInputCentered<RaPort>(Vec(colX[2], y), module, RaMinmaxModule::B1_INPUT + i * 2));
            addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(Vec(colX[2], y + 25.f), module, RaMinmaxModule::LED1_B + i * 2));

            addOutput(createOutputCentered<RaPort>(Vec(colX[1], y + 50.f), module, RaMinmaxModule::OUT1_OUTPUT + i));
        }
    }
};

Model *modelRaMinmax = createModel<RaMinmaxModule, RaMinmaxWidget>("ra-minmax");
