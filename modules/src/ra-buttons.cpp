#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaButtonsModule : Module {
    enum ParamIds {
        BUTTON1_PARAM,
        BUTTON2_PARAM,
        BUTTON3_PARAM,
        BUTTON4_PARAM,
        MODE1_PARAM,
        MODE2_PARAM,
        MODE3_PARAM,
        MODE4_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        OUTPUT1,
        OUTPUT2,
        OUTPUT3,
        OUTPUT4,
        NUM_OUTPUTS
    };
    enum LightIds {
        LIGHT1,
        LIGHT2,
        LIGHT3,
        LIGHT4,
        NUM_LIGHTS
    };

    bool gateState[4] = {};
    bool prevButtonState[4] = {};
    float pulseTimer[4] = {};

    RaButtonsModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < 4; i++) {
            configParam(BUTTON1_PARAM + i, 0.f, 1.f, 0.f, string::f("Button %d", i + 1));
            configSwitch(MODE1_PARAM + i, 0.f, 2.f, 0.f, string::f("Mode %d", i + 1), {"Momentary", "Toggle", "Hold"});
            configOutput(OUTPUT1 + i, string::f("Output %d", i + 1));
            configLight(LIGHT1 + i, string::f("Light %d", i + 1));
        }
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        json_t *statesJ = json_array();
        for (int i = 0; i < 4; i++) {
            json_array_append_new(statesJ, json_boolean(gateState[i]));
        }
        json_object_set_new(rootJ, "gateStates", statesJ);
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *statesJ = json_object_get(rootJ, "gateStates");
        if (statesJ) {
            for (int i = 0; i < 4; i++) {
                json_t *v = json_array_get(statesJ, i);
                if (v) gateState[i] = json_boolean_value(v);
            }
        }
    }

    void process(const ProcessArgs &args) override {
        for (int i = 0; i < 4; i++) {
            int mode = int(params[MODE1_PARAM + i].getValue() + 0.5f);
            bool buttonPressed = params[BUTTON1_PARAM + i].getValue() > 0.5f;

            switch (mode) {
                case 0: {
                    if (buttonPressed && !prevButtonState[i]) {
                        gateState[i] = true;
                        pulseTimer[i] = 0.005f;
                    }
                    if (pulseTimer[i] > 0.f) {
                        pulseTimer[i] -= args.sampleTime;
                        if (pulseTimer[i] <= 0.f) {
                            gateState[i] = false;
                            pulseTimer[i] = 0.f;
                        }
                    }
                    break;
                }
                case 1:
                    if (buttonPressed && !prevButtonState[i])
                        gateState[i] = !gateState[i];
                    break;
                case 2:
                    gateState[i] = buttonPressed;
                    break;
            }

            prevButtonState[i] = buttonPressed;
            outputs[OUTPUT1 + i].setVoltage(gateState[i] ? 10.f : 0.f);
            lights[LIGHT1 + i].setBrightnessSmooth(gateState[i] ? 1.f : 0.f, args.sampleTime);
        }
    }
};

struct RaButtonsWidget : ModuleWidget {
    RaButtonsWidget(RaButtonsModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-buttons.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float ys[] = {55, 148, 241, 334};
        float switchX = 20;
        float buttonX = 60;
        float outputX = 100;

        for (int i = 0; i < 4; i++) {
            addParam(createParamCentered<RaSwitch3>(Vec(switchX, ys[i]), module, RaButtonsModule::MODE1_PARAM + i));
            addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX, ys[i]), module, RaButtonsModule::BUTTON1_PARAM + i, RaButtonsModule::LIGHT1 + i));
            addOutput(createOutputCentered<RaPort>(Vec(outputX, ys[i]), module, RaButtonsModule::OUTPUT1 + i));
        }
    }
};

Model *modelRaButtons = createModel<RaButtonsModule, RaButtonsWidget>("ra-buttons");
