#include "ra-widgets.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaTycheModule : Module {
    enum ParamIds {
        BIAS_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        BIAS_CV_INPUT,
        TRIG_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT_1_2,
        OUT_1_4,
        OUT_1_8,
        OUT_1_16,
        OUT_1_32,
        OUT_1_64,
        OUT_1_128,
        OUT_1_256,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    dsp::SchmittTrigger trigTrigger;
    dsp::PulseGenerator pulseGens[NUM_OUTPUTS];

    RaTycheModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(BIAS_PARAM, -5.f, 5.f, 0.f, "Bias", " V");
        configInput(BIAS_CV_INPUT, "Bias CV");
        configInput(TRIG_INPUT, "Trigger");
        configOutput(OUT_1_2, "1/2");
        configOutput(OUT_1_4, "1/4");
        configOutput(OUT_1_8, "1/8");
        configOutput(OUT_1_16, "1/16");
        configOutput(OUT_1_32, "1/32");
        configOutput(OUT_1_64, "1/64");
        configOutput(OUT_1_128, "1/128");
        configOutput(OUT_1_256, "1/256");
    }

    void process(const ProcessArgs &args) override {
        float bias = params[BIAS_PARAM].getValue();
        if (inputs[BIAS_CV_INPUT].isConnected())
            bias += inputs[BIAS_CV_INPUT].getVoltage() / 2.f;
        bias = clamp(bias, -5.f, 5.f);

        static const float nominals[NUM_OUTPUTS] = {
            1.f / 2.f,
            1.f / 4.f,
            1.f / 8.f,
            1.f / 16.f,
            1.f / 32.f,
            1.f / 64.f,
            1.f / 128.f,
            1.f / 256.f
        };

        if (trigTrigger.process(inputs[TRIG_INPUT].getVoltage())) {
            for (int i = 0; i < NUM_OUTPUTS; i++) {
                float prob = clamp(nominals[i] + bias * 0.1f, 0.f, 1.f);
                if (random::uniform() < prob)
                    pulseGens[i].trigger(1e-3f);
            }
        }

        for (int i = 0; i < NUM_OUTPUTS; i++)
            outputs[OUT_1_2 + i].setVoltage(pulseGens[i].process(args.sampleTime) ? 10.f : 0.f);
    }
};

struct RaTycheWidget : ModuleWidget {
    RaTycheWidget(RaTycheModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-tyche.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float x = box.size.x / 2;

        addParam(createParamCentered<RaKnobTrim>(Vec(x, 35), module, RaTycheModule::BIAS_PARAM));
        addInput(createInputCentered<RaPort>(Vec(x, 68), module, RaTycheModule::BIAS_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(x, 101), module, RaTycheModule::TRIG_INPUT));

        static const int outIds[] = {
            RaTycheModule::OUT_1_2,
            RaTycheModule::OUT_1_4,
            RaTycheModule::OUT_1_8,
            RaTycheModule::OUT_1_16,
            RaTycheModule::OUT_1_32,
            RaTycheModule::OUT_1_64,
            RaTycheModule::OUT_1_128,
            RaTycheModule::OUT_1_256
        };

        float y = 138;
        for (int i = 0; i < 8; i++) {
            addOutput(createOutputCentered<RaPort>(Vec(x, y), module, outIds[i]));
            y += 28;
        }
    }
};

Model *modelRaTyche = createModel<RaTycheModule, RaTycheWidget>("ra-tyche");
