#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

// Envelope curve constants matching ra-adsr
static constexpr float ATT_TARGET = 1.2f;
static constexpr float MIN_TIME = 1e-3f;
static constexpr float MAX_TIME = 10.f;
static constexpr float LAMBDA_BASE = MAX_TIME / MIN_TIME;

struct RaArModule : Module {
    enum ParamIds {
        ATTACK1_PARAM,
        RELEASE1_PARAM,
        ATTACK2_PARAM,
        RELEASE2_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        TRIGGER1_INPUT,
        TRIGGER2_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        ENVELOPE1_OUTPUT,
        ENVELOPE2_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    static constexpr int NUM_CHANNELS = 2;

    float env[NUM_CHANNELS] = {};
    bool attacking[NUM_CHANNELS] = {};
    dsp::SchmittTrigger trigger[NUM_CHANNELS];

    RaArModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < NUM_CHANNELS; i++) {
            configParam(ATTACK1_PARAM + 2 * i, 0.f, 1.f, 0.5f, string::f("Attack %d", i + 1), " ms", LAMBDA_BASE, MIN_TIME * 1000);
            configParam(RELEASE1_PARAM + 2 * i, 0.f, 1.f, 0.5f, string::f("Release %d", i + 1), " ms", LAMBDA_BASE, MIN_TIME * 1000);
        }

        configInput(TRIGGER1_INPUT, "Trigger 1");
        configInput(TRIGGER2_INPUT, "Trigger 2");
        configOutput(ENVELOPE1_OUTPUT, "Envelope 1");
        configOutput(ENVELOPE2_OUTPUT, "Envelope 2");
    }

    void process(const ProcessArgs &args) override {
        for (int i = 0; i < NUM_CHANNELS; i++) {
            float attackLambda = std::pow(LAMBDA_BASE, -params[ATTACK1_PARAM + 2 * i].getValue()) / MIN_TIME;
            float releaseLambda = std::pow(LAMBDA_BASE, -params[RELEASE1_PARAM + 2 * i].getValue()) / MIN_TIME;

            // Trigger input: rising edge restarts the attack
            if (trigger[i].process(inputs[TRIGGER1_INPUT + i].getVoltage()))
                attacking[i] = true;

            float target = attacking[i] ? ATT_TARGET : 0.f;
            float lambda = attacking[i] ? attackLambda : releaseLambda;

            env[i] += (target - env[i]) * lambda * args.sampleTime;
            if (env[i] < 0.f)
                env[i] = 0.f;

            // Attack ends once the envelope reaches 1
            attacking[i] = attacking[i] && (env[i] < 1.f);

            outputs[ENVELOPE1_OUTPUT + i].setVoltage(10.f * env[i]);
        }
    }
};

struct RaArWidget : ModuleWidget {
    RaArWidget(RaArModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-ar.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float x = box.size.x / 2.f;
        float y[] = {35.f, 75.f, 115.f, 155.f, 195.f, 235.f, 275.f, 315.f};

        addInput(createInputCentered<RaPort>(Vec(x, y[0]), module, RaArModule::TRIGGER1_INPUT));
        addParam(createParamCentered<RaKnobTrim>(Vec(x, y[1]), module, RaArModule::ATTACK1_PARAM));
        addParam(createParamCentered<RaKnobTrim>(Vec(x, y[2]), module, RaArModule::RELEASE1_PARAM));
        addOutput(createOutputCentered<RaPort>(Vec(x, y[3]), module, RaArModule::ENVELOPE1_OUTPUT));

        addInput(createInputCentered<RaPort>(Vec(x, y[4]), module, RaArModule::TRIGGER2_INPUT));
        addParam(createParamCentered<RaKnobTrim>(Vec(x, y[5]), module, RaArModule::ATTACK2_PARAM));
        addParam(createParamCentered<RaKnobTrim>(Vec(x, y[6]), module, RaArModule::RELEASE2_PARAM));
        addOutput(createOutputCentered<RaPort>(Vec(x, y[7]), module, RaArModule::ENVELOPE2_OUTPUT));
    }
};

Model *modelRaAr = createModel<RaArModule, RaArWidget>("ra-ar");