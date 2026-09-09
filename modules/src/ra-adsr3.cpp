// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: GATE_INPUT "Gate"
// fname: RETRIG_INPUT "Retrigge"
// fname: TRIGGER_INPUT "Trigger"
// fname: ATTACK_PARAM "Attack"
// fname: DECAY_PARAM "Decay"
// fname: SUSTAIN_PARAM "Sustain"
// fname: RELEASE_PARAM "Release"
// fname: ENVELOPE_OUTPUT "Envelope"
#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

// Envelope curve constants matching ra-adsr
static constexpr float ENV_TARGET = 1.2f;
static constexpr float MIN_TIME = 1e-3f;
static constexpr float MAX_TIME = 10.f;
static constexpr float LAMBDA_BASE = MAX_TIME / MIN_TIME;

struct RaAdsr3Module : Module {
    enum ParamIds {
        ATTACK_PARAM,
        DECAY_PARAM,
        SUSTAIN_PARAM,
        RELEASE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        GATE_INPUT,
        RETRIG_INPUT,
        TRIGGER_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        ENVELOPE_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    float env = 0.f;
    bool gate = false;
    bool attacking = false;
    bool triggerActive = false;
    dsp::SchmittTrigger retrigger;
    dsp::SchmittTrigger trigger;

    RaAdsr3Module() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(ATTACK_PARAM, 0.f, 1.f, 0.5f, "Attack", " ms", LAMBDA_BASE, MIN_TIME * 1000);
        configParam(DECAY_PARAM, 0.f, 1.f, 0.5f, "Decay", " ms", LAMBDA_BASE, MIN_TIME * 1000);
        configParam(SUSTAIN_PARAM, 0.f, 1.f, 0.5f, "Sustain", "%", 0, 100);
        configParam(RELEASE_PARAM, 0.f, 1.f, 0.5f, "Release", " ms", LAMBDA_BASE, MIN_TIME * 1000);

        configInput(GATE_INPUT, "Gate");
        configInput(RETRIG_INPUT, "Retrigger");
        configInput(TRIGGER_INPUT, "Trigger");
        configOutput(ENVELOPE_OUTPUT, "Envelope");
    }

    void process(const ProcessArgs &args) override {
        float attackParam = params[ATTACK_PARAM].getValue();
        float decayParam = params[DECAY_PARAM].getValue();
        float sustainParam = params[SUSTAIN_PARAM].getValue();
        float releaseParam = params[RELEASE_PARAM].getValue();

        float attackLambda = std::pow(LAMBDA_BASE, -attackParam) / MIN_TIME;
        float decayLambda = std::pow(LAMBDA_BASE, -decayParam) / MIN_TIME;
        float releaseLambda = std::pow(LAMBDA_BASE, -releaseParam) / MIN_TIME;

        bool oldGate = gate;

        // Trigger input: rising edge forces gate high and restarts attack
        bool trigEdge = trigger.process(inputs[TRIGGER_INPUT].getVoltage());
        if (trigEdge)
            triggerActive = true;

        gate = inputs[GATE_INPUT].getVoltage() >= 1.f;
        gate = gate || triggerActive;

        attacking = attacking || (gate && !oldGate);
        attacking = attacking || trigEdge;
        attacking = attacking || retrigger.process(inputs[RETRIG_INPUT].getVoltage());
        attacking = attacking && gate;

        float decayTarget = triggerActive ? 0.f : sustainParam;
        float target = attacking ? ENV_TARGET : (gate ? decayTarget : 0.f);
        float lambda = attacking ? attackLambda : (gate ? decayLambda : releaseLambda);

        env += (target - env) * lambda * args.sampleTime;
        if (env < 0.f)
            env = 0.f;

        // Attack ends once the envelope reaches 1
        attacking = attacking && (env < 1.f);

        // Trigger stays active while the trigger-latched gate is attacking or sounding
        triggerActive = triggerActive && (trigEdge || attacking || env > 0.01f);

        outputs[ENVELOPE_OUTPUT].setVoltage(10.f * env);
    }
};

struct RaAdsr3Widget : ModuleWidget {
    RaAdsr3Widget(RaAdsr3Module *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-adsr3.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float x = box.size.x / 2.f;
        float y[] = {35.f, 75.f, 115.f, 155.f, 195.f, 235.f, 275.f, 315.f};

        addInput(createInputCentered<RaPort>(Vec(x, y[0]), module, RaAdsr3Module::GATE_INPUT));
        addInput(createInputCentered<RaPort>(Vec(x, y[1]), module, RaAdsr3Module::RETRIG_INPUT));
        addInput(createInputCentered<RaPort>(Vec(x, y[2]), module, RaAdsr3Module::TRIGGER_INPUT));
        addParam(createParamCentered<RaKnobTrim>(Vec(x, y[3]), module, RaAdsr3Module::ATTACK_PARAM));
        addParam(createParamCentered<RaKnobTrim>(Vec(x, y[4]), module, RaAdsr3Module::DECAY_PARAM));
        addParam(createParamCentered<RaKnobTrim>(Vec(x, y[5]), module, RaAdsr3Module::SUSTAIN_PARAM));
        addParam(createParamCentered<RaKnobTrim>(Vec(x, y[6]), module, RaAdsr3Module::RELEASE_PARAM));
        addOutput(createOutputCentered<RaPort>(Vec(x, y[7]), module, RaAdsr3Module::ENVELOPE_OUTPUT));
    }
};

Model *modelRaAdsr3 = createModel<RaAdsr3Module, RaAdsr3Widget>("ra-adsr3");