#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaChordModule : Module {
    enum ParamIds {
        OFFSET1_PARAM,
        OFFSET2_PARAM,
        OFFSET3_PARAM,
        OFFSET4_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        IN1_INPUT,
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

    RaChordModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < 4; i++) {
            configParam(OFFSET1_PARAM + i, -16.f, 16.f, 0.f, string::f("Ch %d Offset", i + 1), " st");
            paramQuantities[OFFSET1_PARAM + i]->snapEnabled = true;
            configOutput(OUT1_OUTPUT + i, string::f("Output %d", i + 1));
        }
        configInput(IN1_INPUT, "Input");
    }

    void process(const ProcessArgs &args) override {
        float in = inputs[IN1_INPUT].getVoltage();
        for (int i = 0; i < 4; i++) {
            float offset = params[OFFSET1_PARAM + i].getValue();
            outputs[OUT1_OUTPUT + i].setVoltage(in + offset / 12.f);
        }
    }
};

struct RaChordWidget : ModuleWidget {
    RaChordWidget(RaChordModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-chord.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float x = box.size.x / 2.f;
        float y[] = {35.f, 75.f, 115.f, 155.f, 195.f, 235.f, 275.f, 315.f, 355.f};

        addInput(createInputCentered<RaPort>(Vec(x, y[0]), module, RaChordModule::IN1_INPUT));
        for (int i = 0; i < 4; i++) {
            addParam(createParamCentered<RaKnobTrim>(Vec(x, y[i * 2 + 1]), module, RaChordModule::OFFSET1_PARAM + i));
            addOutput(createOutputCentered<RaPort>(Vec(x, y[i * 2 + 2]), module, RaChordModule::OUT1_OUTPUT + i));
        }
    }
};

Model *modelRaChord = createModel<RaChordModule, RaChordWidget>("ra-chord");
