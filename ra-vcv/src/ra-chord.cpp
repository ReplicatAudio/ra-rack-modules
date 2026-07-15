#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaChordModule : Module {
    enum ParamIds {
        OFFSET1_PARAM,
        OFFSET2_PARAM,
        OFFSET3_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        IN1_INPUT,
        IN2_INPUT,
        IN3_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT1_OUTPUT,
        OUT2_OUTPUT,
        OUT3_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    RaChordModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < 3; i++) {
            configParam(OFFSET1_PARAM + i, -16.f, 16.f, 0.f, string::f("Ch %d Offset", i + 1), " st");
            paramQuantities[OFFSET1_PARAM + i]->snapEnabled = true;
            configInput(IN1_INPUT + i, string::f("Input %d", i + 1));
            configOutput(OUT1_OUTPUT + i, string::f("Output %d", i + 1));
        }
    }

    void process(const ProcessArgs &args) override {
        for (int i = 0; i < 3; i++) {
            float in = inputs[IN1_INPUT + i].getVoltage();
            float offset = params[OFFSET1_PARAM + i].getValue();
            float out = in + offset / 12.f;
            outputs[OUT1_OUTPUT + i].setVoltage(out);
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

        for (int i = 0; i < 3; i++) {
            addInput(createInputCentered<RaPort>(Vec(x, y[i * 3]), module, RaChordModule::IN1_INPUT + i));
            addParam(createParamCentered<RaKnobTrim>(Vec(x, y[i * 3 + 1]), module, RaChordModule::OFFSET1_PARAM + i));
            addOutput(createOutputCentered<RaPort>(Vec(x, y[i * 3 + 2]), module, RaChordModule::OUT1_OUTPUT + i));
        }
    }
};

Model *modelRaChord = createModel<RaChordModule, RaChordWidget>("ra-chord");
