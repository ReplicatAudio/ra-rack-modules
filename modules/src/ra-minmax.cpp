// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: A_INPUT "A"
// fname: B_INPUT "B"
// fname: MODE_PARAM "Mode"
// fname: OUTPUT "Out"
// fname: LED_A "A"
// fname: LED_B "B"
#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaMinmaxModule : Module {
    enum ParamIds {
        MODE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        A_INPUT,
        B_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        LED_A,
        LED_B,
        NUM_LIGHTS
    };

    RaMinmaxModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configSwitch(MODE_PARAM, 0.f, 1.f, 0.f, "Mode", {"Min", "Max"});
        configInput(A_INPUT, "A");
        configInput(B_INPUT, "B");
        configOutput(OUTPUT, "Out");
        configLight(LED_A, "A active");
        configLight(LED_B, "B active");
    }

    void process(const ProcessArgs &args) override {
        float a = inputs[A_INPUT].getVoltage();
        float b = inputs[B_INPUT].getVoltage();
        int mode = (int)std::round(params[MODE_PARAM].getValue());

        float out;
        if (mode == 0) {
            out = std::min(a, b);
            lights[LED_A].setBrightness(a <= b ? 1.f : 0.f);
            lights[LED_B].setBrightness(b < a ? 1.f : 0.f);
        } else {
            out = std::max(a, b);
            lights[LED_A].setBrightness(a >= b ? 1.f : 0.f);
            lights[LED_B].setBrightness(b > a ? 1.f : 0.f);
        }

        outputs[OUTPUT].setVoltage(out);
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
        float rowY = 50.f;

        addInput(createInputCentered<RaPort>(Vec(colX[0], rowY), module, RaMinmaxModule::A_INPUT));
        addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(Vec(colX[0], rowY + 30.f), module, RaMinmaxModule::LED_A));

        addParam(createParamCentered<RaSwitch2>(Vec(colX[1], rowY), module, RaMinmaxModule::MODE_PARAM));

        addInput(createInputCentered<RaPort>(Vec(colX[2], rowY), module, RaMinmaxModule::B_INPUT));
        addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(Vec(colX[2], rowY + 30.f), module, RaMinmaxModule::LED_B));

        addOutput(createOutputCentered<RaPort>(Vec(colX[1], 120.f), module, RaMinmaxModule::OUTPUT));
    }
};

Model *modelRaMinmax = createModel<RaMinmaxModule, RaMinmaxWidget>("ra-minmax");
