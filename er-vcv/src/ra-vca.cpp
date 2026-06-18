#include "ra-widgets.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaVcaModule : Module {
    enum ParamIds {
        GAIN_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        AUDIO_INPUT,
        CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        AUDIO_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    RaVcaModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(GAIN_PARAM, 0.f, 2.f, 1.f, "Gain", " dB", 0.f, 20.f, 0.f);
        configInput(AUDIO_INPUT, "Audio");
        configInput(CV_INPUT, "CV");
        configOutput(AUDIO_OUTPUT, "Audio");
    }

    void process(const ProcessArgs &args) override {
        float gain = params[GAIN_PARAM].getValue() + inputs[CV_INPUT].getVoltage() / 10.f;
        gain = clamp(gain, 0.f, 2.f);
        outputs[AUDIO_OUTPUT].setVoltage(inputs[AUDIO_INPUT].getVoltage() * gain);
    }
};

struct RaVcaWidget : ModuleWidget {
    RaVcaWidget(RaVcaModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-vca.svg")));

        addChild(createWidget<RaScrew>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RaKnob>(Vec(box.size.x / 2, 30), module, RaVcaModule::GAIN_PARAM));
        addInput(createInputCentered<RaPort>(Vec(box.size.x / 2, 89), module, RaVcaModule::AUDIO_INPUT));
        addInput(createInputCentered<RaPort>(Vec(box.size.x / 2, 65), module, RaVcaModule::CV_INPUT));
        addOutput(createOutputCentered<RaPort>(Vec(box.size.x / 2, 113), module, RaVcaModule::AUDIO_OUTPUT));
    }
};

Model *modelRaVca = createModel<RaVcaModule, RaVcaWidget>("ra-vca");
