#include "ra-widgets.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaNtetModule : Module {
    enum ParamIds {
        SCALE_PARAM,
        SMASH_PARAM,
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

    bool lastSmash = false;

    RaNtetModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configSwitch(SCALE_PARAM, 1.f, 10.f, 1.f, "Scale", {"12-TET", "24-TET", "36-TET", "48-TET", "60-TET", "72-TET", "84-TET", "96-TET", "108-TET", "120-TET"});
        configSwitch(SMASH_PARAM, 0.f, 1.f, 0.f, "Smash", {"Off", "On"});
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
        float s = params[SCALE_PARAM].getValue();
        bool smash = params[SMASH_PARAM].getValue() > 0.f;
        bool quant = params[MODE_PARAM].getValue() > 0.f;

        if (smash != lastSmash) {
            lastSmash = smash;
            auto* sq = static_cast<SwitchQuantity*>(paramQuantities[SCALE_PARAM]);
            if (smash)
                sq->labels = {"12-TET", "10-TET", "9-TET", "8-TET", "7-TET", "6-TET", "5-TET", "4-TET", "3-TET", "2-TET"};
            else
                sq->labels = {"12-TET", "24-TET", "36-TET", "48-TET", "60-TET", "72-TET", "84-TET", "96-TET", "108-TET", "120-TET"};
        }

        float divisions = smash ? (s == 1.f ? 12.f : 12.f - s) : s * 12.f;

        for (int i = 0; i < 8; i++) {
            float in = inputs[IN1_INPUT + i].getVoltage();
            float out;
            if (quant) {
                float quanta = 1.f / divisions;
                out = std::round(in / quanta) * quanta;
            } else {
                float factor = smash ? (12.f / divisions) : (1.f / divisions);
                out = in * factor;
            }
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

        addParam(createParamCentered<RaKnob>(Vec(box.size.x / 2, 25), module, RaNtetModule::SCALE_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(box.size.x / 2, 50), module, RaNtetModule::SMASH_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(box.size.x / 2, 72), module, RaNtetModule::MODE_PARAM));

        addInput(createInputCentered<RaPort>(Vec(16, 98), module, RaNtetModule::IN1_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 124), module, RaNtetModule::IN2_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 150), module, RaNtetModule::IN3_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 176), module, RaNtetModule::IN4_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 202), module, RaNtetModule::IN5_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 228), module, RaNtetModule::IN6_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 254), module, RaNtetModule::IN7_INPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 280), module, RaNtetModule::IN8_INPUT));

        addOutput(createOutputCentered<RaPort>(Vec(44, 98), module, RaNtetModule::OUT1_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 124), module, RaNtetModule::OUT2_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 150), module, RaNtetModule::OUT3_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 176), module, RaNtetModule::OUT4_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 202), module, RaNtetModule::OUT5_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 228), module, RaNtetModule::OUT6_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 254), module, RaNtetModule::OUT7_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 280), module, RaNtetModule::OUT8_OUTPUT));
    }
};

Model *modelRaNtet = createModel<RaNtetModule, RaNtetWidget>("ra-ntet");
