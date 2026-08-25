#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaKickModule : Module {
    enum ParamIds {
        PITCH_PARAM,
        FM_ATTN_PARAM,
        DECAY_PARAM,
        CLICK_PARAM,
        LEVEL_PARAM,
        PITCH_DROP_PARAM,
        DRIVE_PARAM,
        PITCH_TIME_PARAM,
        CLICK_TONE_PARAM,
        SUB_PARAM,
        ACCENT_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        TRIG_INPUT,
        TONE_CV_INPUT,
        FM_INPUT,
        DECAY_CV_INPUT,
        CLICK_CV_INPUT,
        LEVEL_CV_INPUT,
        PITCH_CV_INPUT,
        DRIVE_CV_INPUT,
        PITCH_TIME_CV_INPUT,
        CLICK_TONE_CV_INPUT,
        SUB_CV_INPUT,
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

    static constexpr float REF_FREQ = 65.406f;    // C2 at 0 V / 0 semitones
    static constexpr float TONE_RANGE = 24.f;      // Tone knob spans ±24 semitones
    static constexpr float MAX_SWEEP_START = 8.f;  // pitch drop up to 8x the tone frequency
    static constexpr float MIN_SWEEP_TIME = 0.002f; // 2 ms
    static constexpr float MAX_SWEEP_TIME = 0.15f;  // 150 ms
    static constexpr float MIN_CLICK_FREQ = 200.f;
    static constexpr float MAX_CLICK_FREQ = 2000.f;
    static constexpr float SUB_OCT = 0.5f;           // sub sits one octave below the tone
    static constexpr float CLICK_TIME = 0.002f;      // 2 ms
    static constexpr float MIN_DECAY = 0.02f;        // 20 ms
    static constexpr float MAX_DECAY = 1.0f;         // 1000 ms

    dsp::SchmittTrigger trigger;
    float phase = 0.f;
    float subPhase = 0.f;
    float env = 0.f;
    float pitchEnv = 0.f;
    float clickPhase = 0.f;
    float clickEnv = 0.f;
    bool accentActive = false;

    RaKickModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(PITCH_PARAM, 0.f, 1.f, 0.5f, "1V/Oct", " st", 0.f, 48.f, -24.f);
        configParam(FM_ATTN_PARAM, -1.f, 1.f, 0.f, "FM attn", "%", 0, 100);
        configParam(DECAY_PARAM, 0.f, 1.f, 0.4f, "Decay", "%", 0, 100);
        configParam(CLICK_PARAM, 0.f, 1.f, 0.25f, "Click", "%", 0, 100);
        configParam(LEVEL_PARAM, 0.f, 1.f, 1.f, "Level", "%", 0, 100);
        configParam(PITCH_DROP_PARAM, 0.f, 1.f, 0.5f, "Pitch drop", "%", 0, 100);
        configParam(DRIVE_PARAM, 0.f, 1.f, 0.3f, "Drive", "%", 0, 100);
        configParam(PITCH_TIME_PARAM, 0.f, 1.f, 0.25f, "Pitch drop time", " ms", 0.f, 148.f, 2.f);
        configParam(CLICK_TONE_PARAM, 0.f, 1.f, 0.4f, "Click tone", " Hz", 10.f, MIN_CLICK_FREQ, 0.f);
        configParam(SUB_PARAM, 0.f, 1.f, 0.5f, "Sub level", "%", 0, 100);
        configParam(ACCENT_PARAM, 0.f, 1.f, 0.5f, "Accent", "%", 0, 100);

        configInput(TRIG_INPUT, "Trigger");
        configInput(TONE_CV_INPUT, "1V/Oct");
        configInput(FM_INPUT, "FM");
        configInput(DECAY_CV_INPUT, "Decay CV");
        configInput(CLICK_CV_INPUT, "Click CV");
        configInput(LEVEL_CV_INPUT, "Level CV");
        configInput(PITCH_CV_INPUT, "Pitch drop CV");
        configInput(DRIVE_CV_INPUT, "Drive CV");
        configInput(PITCH_TIME_CV_INPUT, "Pitch drop time CV");
        configInput(CLICK_TONE_CV_INPUT, "Click tone CV");
        configInput(SUB_CV_INPUT, "Sub level CV");
        configInput(ACCENT_INPUT, "Accent");
        configOutput(AUDIO_OUTPUT, "Audio");
    }

    void process(const ProcessArgs& args) override {
        float toneSemis = params[PITCH_PARAM].getValue() * 48.f - 24.f;
        float decay = clamp(params[DECAY_PARAM].getValue() + inputs[DECAY_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float clickAmt = clamp(params[CLICK_PARAM].getValue() + inputs[CLICK_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float level = clamp(params[LEVEL_PARAM].getValue() + inputs[LEVEL_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float pitchDrop = clamp(params[PITCH_DROP_PARAM].getValue() + inputs[PITCH_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float drive = clamp(params[DRIVE_PARAM].getValue() + inputs[DRIVE_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float pitchTime = clamp(params[PITCH_TIME_PARAM].getValue() + inputs[PITCH_TIME_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float clickTone = clamp(params[CLICK_TONE_PARAM].getValue() + inputs[CLICK_TONE_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float subAmt = clamp(params[SUB_PARAM].getValue() + inputs[SUB_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float accentDepth = clamp(params[ACCENT_PARAM].getValue(), 0.f, 1.f);
        bool accent = inputs[ACCENT_INPUT].getVoltage() >= 1.f;

        // 1V/Oct knob and CV set pitch; FM is a dedicated pitch-modulation CV
        float baseFreq = REF_FREQ * powf(2.f, toneSemis / 12.f
            + inputs[TONE_CV_INPUT].getVoltage()
            + inputs[FM_INPUT].getVoltage() * params[FM_ATTN_PARAM].getValue());
        baseFreq = clamp(baseFreq, 16.f, 2000.f);
        // Pitch drop sets how far the pitch sweeps down from (relative to the tone)
        float sweepStart = 1.f + (MAX_SWEEP_START - 1.f) * pitchDrop;
        // Pitch drop time sets how fast that sweep happens (independent of decay)
        float sweepTime = MIN_SWEEP_TIME + (MAX_SWEEP_TIME - MIN_SWEEP_TIME) * pitchTime;
        // Click tone sets the frequency of the transient tick
        float clickFreq = MIN_CLICK_FREQ * powf(MAX_CLICK_FREQ / MIN_CLICK_FREQ, clickTone);
        // Drive sets the tanh saturation amount
        float driveGain = 1.f + 6.f * drive;
        // Decay sets the body envelope time
        float decayTime = MIN_DECAY * powf(MAX_DECAY / MIN_DECAY, decay);

        if (trigger.process(inputs[TRIG_INPUT].getVoltage())) {
            phase = 0.f;
            subPhase = 0.f;
            env = 1.f;
            pitchEnv = 1.f;
            clickPhase = 0.f;
            clickEnv = 1.f;
            accentActive = accent;
        }

        float out = 0.f;

        // Body: pitch-swept sine gated by a decaying envelope, plus a sub sine an octave below
        if (env > 1e-4f) {
            float freq = baseFreq * (1.f + (sweepStart - 1.f) * pitchEnv);
            phase += freq * args.sampleTime;
            if (phase >= 1.f)
                phase -= 1.f;
            float body = std::sin(2.f * M_PI * phase);
            out += body * env;

            float sub = std::sin(2.f * M_PI * subPhase);
            subPhase += (baseFreq * SUB_OCT) * args.sampleTime;
            out += sub * env * subAmt * 0.9f;

            env *= std::exp(-args.sampleTime / decayTime);
            pitchEnv *= std::exp(-args.sampleTime / sweepTime);
        }

        // Click: short decaying square-wave transient at the start
        if (clickEnv > 1e-4f) {
            float click = (std::sin(2.f * M_PI * clickPhase) >= 0.f ? 1.f : -1.f) * clickEnv;
            clickPhase += clickFreq * args.sampleTime;
            out += click * clickAmt * 0.4f;
            clickEnv *= std::exp(-args.sampleTime / CLICK_TIME);
        }

        // Drive-controlled saturation for punch
        out = 5.f * std::tanh(out * driveGain);

        // Accent: extra level boost latched on triggers that arrive while the accent gate is high
        float accentGain = 1.f + accentDepth * (accentActive ? 1.f : 0.f);
        outputs[AUDIO_OUTPUT].setVoltage(out * level * accentGain);
    }
};

struct RaKickWidget : ModuleWidget {
    RaKickWidget(RaKickModule* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-kick.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float cx = box.size.x / 2;

        addInput(createInputCentered<RaPort>(Vec(cx, 25), module, RaKickModule::TRIG_INPUT));

        // Pitch section (left column): 1V/Oct knob + CV, FM attn + input, aligned to grid IO rows
        addParam(createParamCentered<RaKnob>(Vec(20, 75), module, RaKickModule::PITCH_PARAM));
        addInput(createInputCentered<RaPort>(Vec(20, 112), module, RaKickModule::TONE_CV_INPUT));
        addParam(createParamCentered<RaKnob>(Vec(20, 160), module, RaKickModule::FM_ATTN_PARAM));
        addInput(createInputCentered<RaPort>(Vec(20, 197), module, RaKickModule::FM_INPUT));

        // Row 1
        addParam(createParamCentered<RaKnob>(Vec(57, 75), module, RaKickModule::CLICK_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(93, 75), module, RaKickModule::SUB_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(130, 75), module, RaKickModule::PITCH_DROP_PARAM));
        addInput(createInputCentered<RaPort>(Vec(57, 112), module, RaKickModule::CLICK_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(93, 112), module, RaKickModule::SUB_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(130, 112), module, RaKickModule::PITCH_CV_INPUT));

        // Row 2
        addParam(createParamCentered<RaKnob>(Vec(57, 160), module, RaKickModule::DECAY_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(93, 160), module, RaKickModule::CLICK_TONE_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(130, 160), module, RaKickModule::ACCENT_PARAM));
        addInput(createInputCentered<RaPort>(Vec(57, 197), module, RaKickModule::DECAY_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(93, 197), module, RaKickModule::CLICK_TONE_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(130, 197), module, RaKickModule::ACCENT_INPUT));

        // Row 3 — Level is the last knob (bottom right)
        addParam(createParamCentered<RaKnob>(Vec(57, 245), module, RaKickModule::PITCH_TIME_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(93, 245), module, RaKickModule::DRIVE_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(130, 245), module, RaKickModule::LEVEL_PARAM));
        addInput(createInputCentered<RaPort>(Vec(57, 282), module, RaKickModule::PITCH_TIME_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(93, 282), module, RaKickModule::DRIVE_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(130, 282), module, RaKickModule::LEVEL_CV_INPUT));

        addOutput(createOutputCentered<RaPort>(Vec(cx, 330), module, RaKickModule::AUDIO_OUTPUT));
    }
};

Model* modelRaKick = createModel<RaKickModule, RaKickWidget>("ra-kick");
