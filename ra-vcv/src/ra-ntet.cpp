#include "ra-widgets.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaNtetModule : Module {
    enum ParamIds {
        TET_PARAM,
        MODE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        IN1_INPUT,
        IN2_INPUT,
        IN3_INPUT,
        IN4_INPUT,
        IN5_INPUT,
        IN6_INPUT,
        IN7_INPUT,
        IN8_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT1_OUTPUT,
        OUT2_OUTPUT,
        OUT3_OUTPUT,
        OUT4_OUTPUT,
        OUT5_OUTPUT,
        OUT6_OUTPUT,
        OUT7_OUTPUT,
        OUT8_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    RaNtetModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(TET_PARAM, 2.f, 48.f, 12.f, "TET");
        paramQuantities[TET_PARAM]->snapEnabled = true;
        configSwitch(MODE_PARAM, 0.f, 1.f, 0.f, "Mode", {"Chromatic", "Quant"});
        configInput(IN1_INPUT, "Input 1");
        configInput(IN2_INPUT, "Input 2");
        configInput(IN3_INPUT, "Input 3");
        configInput(IN4_INPUT, "Input 4");
        configInput(IN5_INPUT, "Input 5");
        configInput(IN6_INPUT, "Input 6");
        configInput(IN7_INPUT, "Input 7");
        configInput(IN8_INPUT, "Input 8");
        configOutput(OUT1_OUTPUT, "Output 1");
        configOutput(OUT2_OUTPUT, "Output 2");
        configOutput(OUT3_OUTPUT, "Output 3");
        configOutput(OUT4_OUTPUT, "Output 4");
        configOutput(OUT5_OUTPUT, "Output 5");
        configOutput(OUT6_OUTPUT, "Output 6");
        configOutput(OUT7_OUTPUT, "Output 7");
        configOutput(OUT8_OUTPUT, "Output 8");
    }

    void process(const ProcessArgs &args) override {
        float tet = params[TET_PARAM].getValue();
        bool quant = params[MODE_PARAM].getValue() > 0.f;

        for (int i = 0; i < 8; i++) {
            float in = inputs[IN1_INPUT + i].getVoltage();
            float out;
            if (quant)
                out = std::round(in * tet) / tet;
            else
                out = in * 12.f / tet;
            outputs[OUT1_OUTPUT + i].setVoltage(out);
        }
    }
};

struct RaNtetWidget : ModuleWidget {
    RaNtetWidget(RaNtetModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-ntet.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RaKnob>(Vec(box.size.x / 2, 25), module, RaNtetModule::TET_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(box.size.x / 2, 55), module, RaNtetModule::MODE_PARAM));

        addInput(createInputCentered<RaPort>(Vec(16, 92), module, RaNtetModule::IN1_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 124), module, RaNtetModule::IN2_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 156), module, RaNtetModule::IN3_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 188), module, RaNtetModule::IN4_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 220), module, RaNtetModule::IN5_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 252), module, RaNtetModule::IN6_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 284), module, RaNtetModule::IN7_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 316), module, RaNtetModule::IN8_INPUT));

        addOutput(createOutputCentered<RaPort>(Vec(44, 92), module, RaNtetModule::OUT1_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 124), module, RaNtetModule::OUT2_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 156), module, RaNtetModule::OUT3_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 188), module, RaNtetModule::OUT4_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 220), module, RaNtetModule::OUT5_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 252), module, RaNtetModule::OUT6_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 284), module, RaNtetModule::OUT7_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 316), module, RaNtetModule::OUT8_OUTPUT));
    }
};

Model *modelRaNtet = createModel<RaNtetModule, RaNtetWidget>("ra-ntet");
