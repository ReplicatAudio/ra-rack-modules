#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

static const float JI_CENTS[12] = {
    0.f, 111.73f, 203.91f, 315.64f, 386.31f, 498.04f,
    590.22f, 701.96f, 813.69f, 884.36f, 996.09f, 1088.27f
};

static float quantizeJI(float semitones) {
    float cents = semitones * 100.f;
    float norm = fmodf(cents + 600.f, 1200.f) - 600.f;
    int best = 0;
    float bestDist = fabsf(norm);
    for (int i = 1; i < 12; i++) {
        float d = fabsf(norm - JI_CENTS[i]);
        if (d < bestDist) {
            bestDist = d;
            best = i;
        }
    }
    return JI_CENTS[best] / 1200.f;
}

struct RaJustModule : Module {
    enum ParamIds {
        ROOT_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        IN1_INPUT,
        IN2_INPUT,
        IN3_INPUT,
        IN4_INPUT,
        IN5_INPUT,
        IN6_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT1_OUTPUT,
        OUT2_OUTPUT,
        OUT3_OUTPUT,
        OUT4_OUTPUT,
        OUT5_OUTPUT,
        OUT6_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        ENABLE_LED_R,
        ENABLE_LED_G,
        ENABLE_LED_B,
        NUM_LIGHTS
    };

    RaJustModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(ROOT_PARAM, -1.f, 11.f, -1.f, "Root", " st");
        paramQuantities[ROOT_PARAM]->snapEnabled = true;
        for (int c = 0; c < 3; c++)
            configLight(ENABLE_LED_R + c, "Enable");
        configInput(IN1_INPUT, "Input 1");
        configInput(IN2_INPUT, "Input 2");
        configInput(IN3_INPUT, "Input 3");
        configInput(IN4_INPUT, "Input 4");
        configInput(IN5_INPUT, "Input 5");
        configInput(IN6_INPUT, "Input 6");
        configOutput(OUT1_OUTPUT, "Output 1");
        configOutput(OUT2_OUTPUT, "Output 2");
        configOutput(OUT3_OUTPUT, "Output 3");
        configOutput(OUT4_OUTPUT, "Output 4");
        configOutput(OUT5_OUTPUT, "Output 5");
        configOutput(OUT6_OUTPUT, "Output 6");
    }

    void process(const ProcessArgs &args) override {
        float root = params[ROOT_PARAM].getValue();
        bool enabled = root >= 0.f;

        lights[ENABLE_LED_R + 0].setBrightness(enabled ? 0.6f : 1.f);
        lights[ENABLE_LED_R + 1].setBrightness(0.f);
        lights[ENABLE_LED_R + 2].setBrightness(enabled ? 1.f : 0.f);

        for (int i = 0; i < 6; i++) {
            int in = IN1_INPUT + i;
            int out = OUT1_OUTPUT + i;
            if (!inputs[in].isConnected()) {
                outputs[out].setVoltage(0.f);
                continue;
            }
            int chans = inputs[in].getChannels();
            outputs[out].setChannels(chans);
            for (int c = 0; c < chans; c++) {
                float v = inputs[in].getPolyVoltage(c);
                if (!enabled) {
                    outputs[out].setVoltage(v, c);
                } else {
                    float st = v * 12.f + root;
                    outputs[out].setVoltage(quantizeJI(st), c);
                }
            }
        }
    }
};

struct RaJustWidget : ModuleWidget {
    RaJustWidget(RaJustModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-just.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        addChild(createLightCentered<RaRGBLight>(Vec(35, 18), module, RaJustModule::ENABLE_LED_R));

        addParam(createParamCentered<RaKnob>(Vec(box.size.x / 2, 49), module, RaJustModule::ROOT_PARAM));

        addInput(createInputCentered<RaPort>(Vec(16, 97), module, RaJustModule::IN1_INPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 97), module, RaJustModule::OUT1_OUTPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 141), module, RaJustModule::IN2_INPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 141), module, RaJustModule::OUT2_OUTPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 185), module, RaJustModule::IN3_INPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 185), module, RaJustModule::OUT3_OUTPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 229), module, RaJustModule::IN4_INPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 229), module, RaJustModule::OUT4_OUTPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 273), module, RaJustModule::IN5_INPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 273), module, RaJustModule::OUT5_OUTPUT));
        addInput(createInputCentered<RaPort>(Vec(16, 317), module, RaJustModule::IN6_INPUT));
        addOutput(createOutputCentered<RaPort>(Vec(44, 317), module, RaJustModule::OUT6_OUTPUT));
    }
};

Model *modelRaJust = createModel<RaJustModule, RaJustWidget>("ra-just");
