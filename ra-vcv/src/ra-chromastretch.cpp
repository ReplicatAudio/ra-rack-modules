#include "ra-widgets.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaChromastretchModule : Module {
    enum ParamIds {
        SCALE_PARAM,
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

    RaChromastretchModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configSwitch(SCALE_PARAM, 1.f, 10.f, 1.f, "Scale", {"12-TET", "24-TET", "36-TET", "48-TET", "60-TET", "72-TET", "84-TET", "96-TET", "108-TET", "120-TET"});
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
        float scale = params[SCALE_PARAM].getValue() * 12.f;
        outputs[OUT1_OUTPUT].setVoltage(inputs[IN1_INPUT].getVoltage() / scale);
        outputs[OUT2_OUTPUT].setVoltage(inputs[IN2_INPUT].getVoltage() / scale);
        outputs[OUT3_OUTPUT].setVoltage(inputs[IN3_INPUT].getVoltage() / scale);
        outputs[OUT4_OUTPUT].setVoltage(inputs[IN4_INPUT].getVoltage() / scale);
        outputs[OUT5_OUTPUT].setVoltage(inputs[IN5_INPUT].getVoltage() / scale);
        outputs[OUT6_OUTPUT].setVoltage(inputs[IN6_INPUT].getVoltage() / scale);
        outputs[OUT7_OUTPUT].setVoltage(inputs[IN7_INPUT].getVoltage() / scale);
        outputs[OUT8_OUTPUT].setVoltage(inputs[IN8_INPUT].getVoltage() / scale);
    }
};

struct RaChromastretchWidget : ModuleWidget {
    RaChromastretchWidget(RaChromastretchModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-chromastretch.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RaKnob>(Vec(box.size.x / 2, 28), module, RaChromastretchModule::SCALE_PARAM));

        addInput(createInputCentered<RaPort>(Vec(16, 78), module, RaChromastretchModule::IN1_INPUT));
        addInput(createInputCentered<RaPort>(Vec(44, 78), module, RaChromastretchModule::IN2_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 106), module, RaChromastretchModule::IN3_INPUT));
        addInput(createInputCentered<RaPort>(Vec(44, 106), module, RaChromastretchModule::IN4_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 134), module, RaChromastretchModule::IN5_INPUT));
        addInput(createInputCentered<RaPort>(Vec(44, 134), module, RaChromastretchModule::IN6_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 162), module, RaChromastretchModule::IN7_INPUT));
        addInput(createInputCentered<RaPort>(Vec(44, 162), module, RaChromastretchModule::IN8_INPUT));

        addOutput(createOutputCentered<RaPort>(Vec(16, 200), module, RaChromastretchModule::OUT1_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 200), module, RaChromastretchModule::OUT2_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(16, 228), module, RaChromastretchModule::OUT3_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 228), module, RaChromastretchModule::OUT4_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(16, 256), module, RaChromastretchModule::OUT5_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 256), module, RaChromastretchModule::OUT6_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(16, 284), module, RaChromastretchModule::OUT7_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 284), module, RaChromastretchModule::OUT8_OUTPUT));
    }
};

Model *modelRaChromastretch = createModel<RaChromastretchModule, RaChromastretchWidget>("ra-chromastretch");
