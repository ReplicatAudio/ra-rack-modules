#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaRideModule : Module {
    enum ParamIds {
        PITCH_PARAM,        // 1V/Oct pitch knob
        FM_ATTN_PARAM,
        TONE_PARAM,         // partial ratio multiplier
        METALLIC_PARAM,     // number/strength of active partials
        DECAY_PARAM,
        SNAP_PARAM,         // higher-partial decay skew
        BODY_PARAM,         // metallic level
        BRIGHT_PARAM,       // highpass cutoff
        ACCENT_PARAM,
        DRIVE_PARAM,
        LEVEL_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        TRIG_INPUT,
        PITCH_CV_INPUT,     // 1V/Oct
        FM_INPUT,
        TONE_CV_INPUT,
        METALLIC_CV_INPUT,
        DECAY_CV_INPUT,
        SNAP_CV_INPUT,
        BODY_CV_INPUT,
        BRIGHT_CV_INPUT,
        DRIVE_CV_INPUT,
        LEVEL_CV_INPUT,
        ACCENT_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        AUDIO_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    static constexpr float REF_FREQ = 440.f;     // A4 at 0 V / 0 semitones
    static constexpr float RATIOS[6] = {2.000f, 3.000f, 4.160f, 5.430f, 6.790f, 8.210f};
    static constexpr float BELL_RATIO = 9.5f;    // ride "ping" partial
    static constexpr float BELL_GAIN = 0.5f;
    static constexpr float MIN_DECAY = 0.1f;     // 100 ms
    static constexpr float MAX_DECAY = 4.0f;     // 4000 ms

    dsp::SchmittTrigger trigger;
    float phase[6] = {};
    float env[6] = {};
    float bellPhase = 0.f;
    float bellEnv = 0.f;
    float hpPrevIn = 0.f;
    float hpPrevOut = 0.f;
    bool accentActive = false;

    RaRideModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(PITCH_PARAM, 0.f, 1.f, 0.5f, "1V/Oct", " st", 0.f, 48.f, -24.f);
        configParam(FM_ATTN_PARAM, -1.f, 1.f, 0.f, "FM attn", "%", 0, 100);
        configParam(TONE_PARAM, 0.f, 1.f, 0.5f, "Tone", "%", 0, 100);
        configParam(METALLIC_PARAM, 0.f, 1.f, 0.5f, "Metallic", "%", 0, 100);
        configParam(DECAY_PARAM, 0.f, 1.f, 0.7f, "Decay", "%", 0, 100);
        configParam(SNAP_PARAM, 0.f, 1.f, 0.2f, "Snap", "%", 0, 100);
        configParam(BODY_PARAM, 0.f, 1.f, 1.f, "Body", "%", 0, 100);
        configParam(BRIGHT_PARAM, 0.f, 1.f, 0.45f, "Bright", "%", 0, 100);
        configParam(ACCENT_PARAM, 0.f, 1.f, 0.5f, "Accent", "%", 0, 100);
        configParam(DRIVE_PARAM, 0.f, 1.f, 0.3f, "Drive", "%", 0, 100);
        configParam(LEVEL_PARAM, 0.f, 1.f, 1.f, "Level", "%", 0, 100);

        configInput(TRIG_INPUT, "Trigger");
        configInput(PITCH_CV_INPUT, "1V/Oct");
        configInput(FM_INPUT, "FM");
        configInput(TONE_CV_INPUT, "Tone CV");
        configInput(METALLIC_CV_INPUT, "Metallic CV");
        configInput(DECAY_CV_INPUT, "Decay CV");
        configInput(SNAP_CV_INPUT, "Snap CV");
        configInput(BODY_CV_INPUT, "Body CV");
        configInput(BRIGHT_CV_INPUT, "Bright CV");
        configInput(DRIVE_CV_INPUT, "Drive CV");
        configInput(LEVEL_CV_INPUT, "Level CV");
        configInput(ACCENT_INPUT, "Accent");
        configOutput(AUDIO_OUTPUT, "Audio");
    }

    void process(const ProcessArgs& args) override {
        float pitchSemis = params[PITCH_PARAM].getValue() * 48.f - 24.f;
        float baseFreq = REF_FREQ * powf(2.f, pitchSemis / 12.f
            + inputs[PITCH_CV_INPUT].getVoltage()
            + inputs[FM_INPUT].getVoltage() * params[FM_ATTN_PARAM].getValue());
        baseFreq = clamp(baseFreq, 16.f, 4000.f);

        float tone = clamp(params[TONE_PARAM].getValue() + inputs[TONE_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float metallic = clamp(params[METALLIC_PARAM].getValue() + inputs[METALLIC_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float decay = clamp(params[DECAY_PARAM].getValue() + inputs[DECAY_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float snap = clamp(params[SNAP_PARAM].getValue() + inputs[SNAP_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float body = clamp(params[BODY_PARAM].getValue() + inputs[BODY_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float bright = clamp(params[BRIGHT_PARAM].getValue() + inputs[BRIGHT_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float drive = clamp(params[DRIVE_PARAM].getValue() + inputs[DRIVE_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float level = clamp(params[LEVEL_PARAM].getValue() + inputs[LEVEL_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float accentDepth = clamp(params[ACCENT_PARAM].getValue(), 0.f, 1.f);
        bool accent = inputs[ACCENT_INPUT].getVoltage() >= 1.f;

        float ratioMult = 0.7f + 0.6f * tone;
        float nActive = 2.f + 4.f * metallic;
        float decayTime = MIN_DECAY * powf(MAX_DECAY / MIN_DECAY, decay);
        float driveGain = 1.f + 6.f * drive;
        float bellDecay = decayTime * 1.5f;

        if (trigger.process(inputs[TRIG_INPUT].getVoltage())) {
            for (int i = 0; i < 6; i++) {
                phase[i] = 0.f;
                env[i] = 1.f;
            }
            bellPhase = 0.f;
            bellEnv = 1.f;
            accentActive = accent;
        }

        float out = 0.f;
        for (int i = 0; i < 6; i++) {
            if (env[i] > 1e-4f) {
                phase[i] += baseFreq * ratioMult * RATIOS[i] * args.sampleTime;
                if (phase[i] >= 1.f)
                    phase[i] -= 1.f;
                float s = (std::sin(2.f * M_PI * phase[i]) >= 0.f) ? 1.f : -1.f;
                float amp = clamp(nActive - i, 0.f, 1.f);
                out += s * env[i] * amp;
                float partDecay = decayTime / (1.f + snap * 0.4f * i);
                env[i] *= std::exp(-args.sampleTime / partDecay);
            }
        }

        // Bell "ping": a pure sine that rings longer than the metallic partials
        if (bellEnv > 1e-4f) {
            bellPhase += baseFreq * ratioMult * BELL_RATIO * args.sampleTime;
            if (bellPhase >= 1.f)
                bellPhase -= 1.f;
            out += std::sin(2.f * M_PI * bellPhase) * bellEnv * BELL_GAIN;
            bellEnv *= std::exp(-args.sampleTime / bellDecay);
        }

        // Highpass (Bright) shapes the metallic tone
        float cutoff = 100.f * powf(100.f, bright);
        float hp = std::exp(-2.f * M_PI * cutoff * args.sampleTime);
        float hpIn = out;
        out = hp * (hpPrevOut + hpIn - hpPrevIn);
        hpPrevOut = out;
        hpPrevIn = hpIn;

        out *= body;

        out = 5.f * std::tanh(out * driveGain);

        float accentGain = 1.f + accentDepth * (accentActive ? 1.f : 0.f);
        outputs[AUDIO_OUTPUT].setVoltage(out * level * accentGain);
    }
};

// C++11 out-of-class definition for the static constexpr array (odr-use)
constexpr float RaRideModule::RATIOS[6];

struct RaRideWidget : ModuleWidget {
    RaRideWidget(RaRideModule* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-ride.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float cx = box.size.x / 2;

        addInput(createInputCentered<RaPort>(Vec(cx, 25), module, RaRideModule::TRIG_INPUT));

        // Pitch section (left column): 1V/Oct knob + CV, FM attn + input, aligned to grid IO rows
        addParam(createParamCentered<RaKnob>(Vec(20, 75), module, RaRideModule::PITCH_PARAM));
        addInput(createInputCentered<RaPort>(Vec(20, 112), module, RaRideModule::PITCH_CV_INPUT));
        addParam(createParamCentered<RaKnob>(Vec(20, 160), module, RaRideModule::FM_ATTN_PARAM));
        addInput(createInputCentered<RaPort>(Vec(20, 197), module, RaRideModule::FM_INPUT));

        // Row 1
        addParam(createParamCentered<RaKnob>(Vec(57, 75), module, RaRideModule::TONE_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(93, 75), module, RaRideModule::METALLIC_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(130, 75), module, RaRideModule::DECAY_PARAM));
        addInput(createInputCentered<RaPort>(Vec(57, 112), module, RaRideModule::TONE_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(93, 112), module, RaRideModule::METALLIC_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(130, 112), module, RaRideModule::DECAY_CV_INPUT));

        // Row 2
        addParam(createParamCentered<RaKnob>(Vec(57, 160), module, RaRideModule::SNAP_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(93, 160), module, RaRideModule::BODY_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(130, 160), module, RaRideModule::ACCENT_PARAM));
        addInput(createInputCentered<RaPort>(Vec(57, 197), module, RaRideModule::SNAP_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(93, 197), module, RaRideModule::BODY_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(130, 197), module, RaRideModule::ACCENT_INPUT));

        // Row 3 — Level is the last knob (bottom right)
        addParam(createParamCentered<RaKnob>(Vec(57, 245), module, RaRideModule::BRIGHT_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(93, 245), module, RaRideModule::DRIVE_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(130, 245), module, RaRideModule::LEVEL_PARAM));
        addInput(createInputCentered<RaPort>(Vec(57, 282), module, RaRideModule::BRIGHT_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(93, 282), module, RaRideModule::DRIVE_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(130, 282), module, RaRideModule::LEVEL_CV_INPUT));

        addOutput(createOutputCentered<RaPort>(Vec(cx, 330), module, RaRideModule::AUDIO_OUTPUT));
    }
};

Model* modelRaRide = createModel<RaRideModule, RaRideWidget>("ra-ride");