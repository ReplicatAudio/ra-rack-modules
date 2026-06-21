#include "ra-widgets.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaChromaquantModule : Module {
    enum ParamIds {
        SCALE_PARAM,
        SMASH_PARAM,
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

    RaChromaquantModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configSwitch(SCALE_PARAM, 1.f, 10.f, 1.f, "Scale", {"12-TET", "24-TET", "36-TET", "48-TET", "60-TET", "72-TET", "84-TET", "96-TET", "108-TET", "120-TET"});
        configSwitch(SMASH_PARAM, 0.f, 1.f, 0.f, "Smash", {"Off", "On"});
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

    bool lastSmash = false;

    void process(const ProcessArgs &args) override {
        float s = params[SCALE_PARAM].getValue();
        bool smash = params[SMASH_PARAM].getValue() > 0.f;
        if (smash != lastSmash) {
            lastSmash = smash;
            auto* sq = static_cast<SwitchQuantity*>(paramQuantities[SCALE_PARAM]);
            if (smash)
                sq->labels = {"12-TET", "10-TET", "9-TET", "8-TET", "7-TET", "6-TET", "5-TET", "4-TET", "3-TET", "2-TET"};
            else
                sq->labels = {"12-TET", "24-TET", "36-TET", "48-TET", "60-TET", "72-TET", "84-TET", "96-TET", "108-TET", "120-TET"};
        }
        float divisions = smash ? (s == 1.f ? 12.f : 12.f - s) : s * 12.f;
        float quanta = 1.f / divisions;
        outputs[OUT1_OUTPUT].setVoltage(std::round(inputs[IN1_INPUT].getVoltage() / quanta) * quanta);
        outputs[OUT2_OUTPUT].setVoltage(std::round(inputs[IN2_INPUT].getVoltage() / quanta) * quanta);
        outputs[OUT3_OUTPUT].setVoltage(std::round(inputs[IN3_INPUT].getVoltage() / quanta) * quanta);
        outputs[OUT4_OUTPUT].setVoltage(std::round(inputs[IN4_INPUT].getVoltage() / quanta) * quanta);
        outputs[OUT5_OUTPUT].setVoltage(std::round(inputs[IN5_INPUT].getVoltage() / quanta) * quanta);
        outputs[OUT6_OUTPUT].setVoltage(std::round(inputs[IN6_INPUT].getVoltage() / quanta) * quanta);
        outputs[OUT7_OUTPUT].setVoltage(std::round(inputs[IN7_INPUT].getVoltage() / quanta) * quanta);
        outputs[OUT8_OUTPUT].setVoltage(std::round(inputs[IN8_INPUT].getVoltage() / quanta) * quanta);
    }
};

struct RaChromaquantWidget : ModuleWidget {
    RaChromaquantWidget(RaChromaquantModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-chromaquant.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RaKnob>(Vec(box.size.x / 2, 28), module, RaChromaquantModule::SCALE_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(box.size.x / 2, 55), module, RaChromaquantModule::SMASH_PARAM));

        addInput(createInputCentered<RaPort>(Vec(16, 80), module, RaChromaquantModule::IN1_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 106), module, RaChromaquantModule::IN2_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 132), module, RaChromaquantModule::IN3_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 158), module, RaChromaquantModule::IN4_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 184), module, RaChromaquantModule::IN5_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 210), module, RaChromaquantModule::IN6_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 236), module, RaChromaquantModule::IN7_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 262), module, RaChromaquantModule::IN8_INPUT));

        addOutput(createOutputCentered<RaPort>(Vec(44, 80), module, RaChromaquantModule::OUT1_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 106), module, RaChromaquantModule::OUT2_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 132), module, RaChromaquantModule::OUT3_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 158), module, RaChromaquantModule::OUT4_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 184), module, RaChromaquantModule::OUT5_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 210), module, RaChromaquantModule::OUT6_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 236), module, RaChromaquantModule::OUT7_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 262), module, RaChromaquantModule::OUT8_OUTPUT));
    }
};

Model *modelRaChromaquant = createModel<RaChromaquantModule, RaChromaquantWidget>("ra-chromaquant");
