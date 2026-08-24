#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaSnareModule : Module {
    enum ParamIds {
        PITCH_PARAM,        // 1V/Oct pitch knob
        FM_ATTN_PARAM,
        TONE_PARAM,         // tonal body level
        NOISE_PARAM,        // noise/rattle level
        DECAY_PARAM,
        SNAP_PARAM,         // noise decay time
        PITCH_DROP_PARAM,
        PITCH_TIME_PARAM,
        DRIVE_PARAM,
        ACCENT_PARAM,
        LEVEL_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        TRIG_INPUT,
        PITCH_CV_INPUT,     // 1V/Oct
        FM_INPUT,
        TONE_CV_INPUT,
        NOISE_CV_INPUT,
        DECAY_CV_INPUT,
        SNAP_CV_INPUT,
        PITCH_DROP_CV_INPUT,
        PITCH_TIME_CV_INPUT,
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

    static constexpr float REF_FREQ = 130.8f;    // C3 at 0 V / 0 semitones
    static constexpr float MAX_SWEEP_START = 3.f; // tone pitch drops up to 3x
    static constexpr float MIN_SWEEP_TIME = 0.002f; // 2 ms
    static constexpr float MAX_SWEEP_TIME = 0.15f;  // 150 ms
    static constexpr float MIN_DECAY = 0.02f;     // 20 ms
    static constexpr float MAX_DECAY = 1.0f;      // 1000 ms
    static constexpr float MIN_SNAP = 0.005f;     // 5 ms
    static constexpr float MAX_SNAP = 0.3f;       // 300 ms
    static constexpr float NOISE_HP_FREQ = 2000.f; // fixed one-pole highpass on the rattle (~2 kHz)

    dsp::SchmittTrigger trigger;
    float phase = 0.f;
    float env = 0.f;
    float pitchEnv = 0.f;
    float noiseEnv = 0.f;
    float noiseLp = 0.f;                         // one-pole lowpass state for the noise highpass
    bool accentActive = false;

    RaSnareModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(PITCH_PARAM, 0.f, 1.f, 0.5f, "1V/Oct", " st", 0.f, 48.f, -24.f);
        configParam(FM_ATTN_PARAM, -1.f, 1.f, 0.f, "FM attn", "%", 0, 100);
        configParam(TONE_PARAM, 0.f, 1.f, 0.6f, "Body", "%", 0, 100);
        configParam(NOISE_PARAM, 0.f, 1.f, 0.8f, "Noise", "%", 0, 100);
        configParam(DECAY_PARAM, 0.f, 1.f, 0.3f, "Decay", "%", 0, 100);
        configParam(SNAP_PARAM, 0.f, 1.f, 0.5f, "Snap", "%", 0, 100);
        configParam(PITCH_DROP_PARAM, 0.f, 1.f, 0.5f, "Pitch drop", "%", 0, 100);
        configParam(PITCH_TIME_PARAM, 0.f, 1.f, 0.25f, "Pitch drop time", " ms", 0.f, 148.f, 2.f);
        configParam(DRIVE_PARAM, 0.f, 1.f, 0.3f, "Drive", "%", 0, 100);
        configParam(ACCENT_PARAM, 0.f, 1.f, 0.5f, "Accent", "%", 0, 100);
        configParam(LEVEL_PARAM, 0.f, 1.f, 1.f, "Level", "%", 0, 100);

        configInput(TRIG_INPUT, "Trigger");
        configInput(PITCH_CV_INPUT, "1V/Oct");
        configInput(FM_INPUT, "FM");
        configInput(TONE_CV_INPUT, "Tone CV");
        configInput(NOISE_CV_INPUT, "Noise CV");
        configInput(DECAY_CV_INPUT, "Decay CV");
        configInput(SNAP_CV_INPUT, "Snap CV");
        configInput(PITCH_DROP_CV_INPUT, "Pitch drop CV");
        configInput(PITCH_TIME_CV_INPUT, "Pitch drop time CV");
        configInput(DRIVE_CV_INPUT, "Drive CV");
        configInput(LEVEL_CV_INPUT, "Level CV");
        configInput(ACCENT_INPUT, "Accent");
        configOutput(AUDIO_OUTPUT, "Audio");
    }

    void process(const ProcessArgs& args) override {
        // Pitch: 1V/Oct knob and CV, plus FM as a dedicated modulation CV
        float pitchSemis = params[PITCH_PARAM].getValue() * 48.f - 24.f;
        float baseFreq = REF_FREQ * powf(2.f, pitchSemis / 12.f
            + inputs[PITCH_CV_INPUT].getVoltage()
            + inputs[FM_INPUT].getVoltage() * params[FM_ATTN_PARAM].getValue());
        baseFreq = clamp(baseFreq, 16.f, 4000.f);

        float tone = clamp(params[TONE_PARAM].getValue() + inputs[TONE_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float noise = clamp(params[NOISE_PARAM].getValue() + inputs[NOISE_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float decay = clamp(params[DECAY_PARAM].getValue() + inputs[DECAY_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float snap = clamp(params[SNAP_PARAM].getValue() + inputs[SNAP_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float drop = clamp(params[PITCH_DROP_PARAM].getValue() + inputs[PITCH_DROP_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float ptime = clamp(params[PITCH_TIME_PARAM].getValue() + inputs[PITCH_TIME_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float drive = clamp(params[DRIVE_PARAM].getValue() + inputs[DRIVE_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float level = clamp(params[LEVEL_PARAM].getValue() + inputs[LEVEL_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float accentDepth = clamp(params[ACCENT_PARAM].getValue(), 0.f, 1.f);
        bool accent = inputs[ACCENT_INPUT].getVoltage() >= 1.f;

        float sweepStart = 1.f + (MAX_SWEEP_START - 1.f) * drop;
        float sweepTime = MIN_SWEEP_TIME + (MAX_SWEEP_TIME - MIN_SWEEP_TIME) * ptime;
        float decayTime = MIN_DECAY * powf(MAX_DECAY / MIN_DECAY, decay);
        float snapTime = MIN_SNAP * powf(MAX_SNAP / MIN_SNAP, snap);
        float driveGain = 1.f + 6.f * drive;
        float noiseAlpha = 1.f - std::exp(-2.f * M_PI * NOISE_HP_FREQ * args.sampleTime);

        if (trigger.process(inputs[TRIG_INPUT].getVoltage())) {
            phase = 0.f;
            env = 1.f;
            pitchEnv = 1.f;
            noiseEnv = 1.f;
            accentActive = accent;
        }

        float out = 0.f;

        // Tonal body: pitch-swept triangle gated by a decaying envelope
        if (env > 1e-4f) {
            float freq = baseFreq * (1.f + (sweepStart - 1.f) * pitchEnv);
            phase += freq * args.sampleTime;
            if (phase >= 1.f)
                phase -= 1.f;
            float p = fmodf(phase, 1.f);
            float body = 1.f - 4.f * fabsf(p - 0.5f);
            out += body * env * tone;
            env *= std::exp(-args.sampleTime / decayTime);
            pitchEnv *= std::exp(-args.sampleTime / sweepTime);
        }

        // Noise: highpassed white-noise rattle (fixed ~2 kHz) with its own (snappier) decay
        if (noiseEnv > 1e-4f) {
            float noise = random::uniform() * 2.f - 1.f;
            noiseLp += noiseAlpha * (noise - noiseLp);
            out += (noise - noiseLp) * noiseEnv * noise;
            noiseEnv *= std::exp(-args.sampleTime / snapTime);
        }

        // Drive-controlled saturation for punch
        out = 5.f * std::tanh(out * driveGain);

        // Accent: extra level boost latched on triggers that arrive while the accent gate is high
        float accentGain = 1.f + accentDepth * (accentActive ? 1.f : 0.f);
        outputs[AUDIO_OUTPUT].setVoltage(out * level * accentGain);
    }
};

struct RaSnareWidget : ModuleWidget {
    RaSnareWidget(RaSnareModule* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-snare.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float cx = box.size.x / 2;

        addInput(createInputCentered<RaPort>(Vec(cx, 25), module, RaSnareModule::TRIG_INPUT));

        // Pitch section (left column): 1V/Oct knob + CV, FM attn + input, aligned to grid IO rows
        addParam(createParamCentered<RaKnob>(Vec(20, 75), module, RaSnareModule::PITCH_PARAM));
        addInput(createInputCentered<RaPort>(Vec(20, 112), module, RaSnareModule::PITCH_CV_INPUT));
        addParam(createParamCentered<RaKnob>(Vec(20, 160), module, RaSnareModule::FM_ATTN_PARAM));
        addInput(createInputCentered<RaPort>(Vec(20, 197), module, RaSnareModule::FM_INPUT));

        // Row 1
        addParam(createParamCentered<RaKnob>(Vec(57, 75), module, RaSnareModule::TONE_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(93, 75), module, RaSnareModule::NOISE_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(130, 75), module, RaSnareModule::PITCH_DROP_PARAM));
        addInput(createInputCentered<RaPort>(Vec(57, 112), module, RaSnareModule::TONE_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(93, 112), module, RaSnareModule::NOISE_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(130, 112), module, RaSnareModule::PITCH_DROP_CV_INPUT));

        // Row 2
        addParam(createParamCentered<RaKnob>(Vec(57, 160), module, RaSnareModule::DECAY_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(93, 160), module, RaSnareModule::SNAP_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(130, 160), module, RaSnareModule::ACCENT_PARAM));
        addInput(createInputCentered<RaPort>(Vec(57, 197), module, RaSnareModule::DECAY_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(93, 197), module, RaSnareModule::SNAP_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(130, 197), module, RaSnareModule::ACCENT_INPUT));

        // Row 3 — Level is the last knob (bottom right)
        addParam(createParamCentered<RaKnob>(Vec(57, 245), module, RaSnareModule::PITCH_TIME_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(93, 245), module, RaSnareModule::DRIVE_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(130, 245), module, RaSnareModule::LEVEL_PARAM));
        addInput(createInputCentered<RaPort>(Vec(57, 282), module, RaSnareModule::PITCH_TIME_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(93, 282), module, RaSnareModule::DRIVE_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(130, 282), module, RaSnareModule::LEVEL_CV_INPUT));

        addOutput(createOutputCentered<RaPort>(Vec(cx, 330), module, RaSnareModule::AUDIO_OUTPUT));
    }
};

Model* modelRaSnare = createModel<RaSnareModule, RaSnareWidget>("ra-snare");
