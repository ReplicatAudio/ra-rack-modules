#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaClapModule : Module {
    enum ParamIds {
        PITCH_PARAM,        // 1V/Oct pitch knob (tunes the bandpass + thump)
        FM_ATTN_PARAM,
        TONE_PARAM,         // bandpass Q (bandwidth)
        DECAY_PARAM,
        SNAP_PARAM,         // per-burst noise decay
        TAPS_PARAM,         // number of retrigger bursts (1-4)
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
        DECAY_CV_INPUT,
        SNAP_CV_INPUT,
        TAPS_CV_INPUT,
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
    static constexpr float BP_MULT = 8.f;        // noise bandpass centre sits above the pitch
    static constexpr float MAX_SWEEP_START = 3.f; // thump pitch drops up to 3x
    static constexpr float MIN_SWEEP_TIME = 0.002f; // 2 ms
    static constexpr float MAX_SWEEP_TIME = 0.15f;  // 150 ms
    static constexpr float MIN_DECAY = 0.02f;    // 20 ms
    static constexpr float MAX_DECAY = 1.0f;     // 1000 ms
    static constexpr float MIN_SNAP = 0.01f;     // 10 ms
    static constexpr float MAX_SNAP = 0.3f;      // 300 ms
    static constexpr float TAP_INTERVAL = 0.012f; // 12 ms between clap bursts
    static constexpr float THUMP_GAIN = 0.25f;

    dsp::SchmittTrigger trigger;
    // SVF bandpass state
    float bpLow = 0.f;
    float bpBand = 0.f;
    // voices
    float noiseEnv = 0.f;
    float thumpEnv = 0.f;
    float thumpPhase = 0.f;
    float thumpPitchEnv = 0.f;
    int tapsLeft = 0;
    float tapTimer = 0.f;
    bool accentActive = false;

    RaClapModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(PITCH_PARAM, 0.f, 1.f, 0.5f, "1V/Oct", " st", 0.f, 48.f, -24.f);
        configParam(FM_ATTN_PARAM, -1.f, 1.f, 0.f, "FM attn", "%", 0, 100);
        configParam(TONE_PARAM, 0.f, 1.f, 0.5f, "Tone", "%", 0, 100);
        configParam(DECAY_PARAM, 0.f, 1.f, 0.35f, "Decay", "%", 0, 100);
        configParam(SNAP_PARAM, 0.f, 1.f, 0.3f, "Snap", "%", 0, 100);
        configParam(TAPS_PARAM, 0.f, 1.f, 0.66f, "Taps", "%", 0, 100);
        configParam(PITCH_DROP_PARAM, 0.f, 1.f, 0.3f, "Pitch drop", "%", 0, 100);
        configParam(PITCH_TIME_PARAM, 0.f, 1.f, 0.25f, "Pitch drop time", " ms", 0.f, 148.f, 2.f);
        configParam(DRIVE_PARAM, 0.f, 1.f, 0.3f, "Drive", "%", 0, 100);
        configParam(ACCENT_PARAM, 0.f, 1.f, 0.5f, "Accent", "%", 0, 100);
        configParam(LEVEL_PARAM, 0.f, 1.f, 1.f, "Level", "%", 0, 100);

        configInput(TRIG_INPUT, "Trigger");
        configInput(PITCH_CV_INPUT, "1V/Oct");
        configInput(FM_INPUT, "FM");
        configInput(TONE_CV_INPUT, "Tone CV");
        configInput(DECAY_CV_INPUT, "Decay CV");
        configInput(SNAP_CV_INPUT, "Snap CV");
        configInput(TAPS_CV_INPUT, "Taps CV");
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
        float decay = clamp(params[DECAY_PARAM].getValue() + inputs[DECAY_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float snap = clamp(params[SNAP_PARAM].getValue() + inputs[SNAP_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float taps = clamp(params[TAPS_PARAM].getValue() + inputs[TAPS_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float drop = clamp(params[PITCH_DROP_PARAM].getValue() + inputs[PITCH_DROP_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float ptime = clamp(params[PITCH_TIME_PARAM].getValue() + inputs[PITCH_TIME_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float drive = clamp(params[DRIVE_PARAM].getValue() + inputs[DRIVE_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float level = clamp(params[LEVEL_PARAM].getValue() + inputs[LEVEL_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float accentDepth = clamp(params[ACCENT_PARAM].getValue(), 0.f, 1.f);
        bool accent = inputs[ACCENT_INPUT].getVoltage() >= 1.f;

        // Bandpass centre tracks the pitch; Q narrows as Tone rises
        float bpFreq = clamp(baseFreq * BP_MULT, 100.f, 12000.f);
        float bpQ = 0.5f + 4.f * tone;
        // Pitch drop/time apply to the thump sweep
        float sweepStart = 1.f + (MAX_SWEEP_START - 1.f) * drop;
        float sweepTime = MIN_SWEEP_TIME + (MAX_SWEEP_TIME - MIN_SWEEP_TIME) * ptime;
        // Decay sets the thump tail length; Snap sets each noise burst's decay
        float decayTime = MIN_DECAY * powf(MAX_DECAY / MIN_DECAY, decay);
        float snapTime = MIN_SNAP * powf(MAX_SNAP / MIN_SNAP, snap);
        float driveGain = 1.f + 6.f * drive;
        int totalTaps = 1 + (int)std::round(taps * 3.f);

        if (trigger.process(inputs[TRIG_INPUT].getVoltage())) {
            noiseEnv = 1.f;
            thumpEnv = 1.f;
            thumpPhase = 0.f;
            thumpPitchEnv = 1.f;
            tapsLeft = totalTaps - 1;
            tapTimer = TAP_INTERVAL;
            accentActive = accent;
        }

        // Schedule the extra clap bursts
        if (tapsLeft > 0) {
            tapTimer -= args.sampleTime;
            if (tapTimer <= 0.f) {
                noiseEnv = 1.f;
                tapsLeft--;
                tapTimer = TAP_INTERVAL;
            }
        }

        float out = 0.f;

        // Noise bursts through a resonant bandpass (the clap body)
        if (noiseEnv > 1e-4f) {
            float x = random::uniform() * 2.f - 1.f;
            float g = tanf(M_PI * bpFreq * args.sampleTime);
            g = clamp(g, 0.f, 10.f);
            float R = 1.f / (2.f * bpQ);
            float S = 1.f / (1.f + g * (g + R));
            float hp = (x - bpLow - R * bpBand) * S;
            bpBand += g * hp;
            bpLow += g * bpBand;
            out += bpBand * noiseEnv;
            noiseEnv *= std::exp(-args.sampleTime / snapTime);
        }

        // Low thump under the clap, with the pitch-drop sweep applied
        if (thumpEnv > 1e-4f) {
            thumpPhase += baseFreq * (1.f + (sweepStart - 1.f) * thumpPitchEnv) * args.sampleTime;
            if (thumpPhase >= 1.f)
                thumpPhase -= 1.f;
            out += std::sin(2.f * M_PI * thumpPhase) * thumpEnv * THUMP_GAIN;
            thumpEnv *= std::exp(-args.sampleTime / decayTime);
            thumpPitchEnv *= std::exp(-args.sampleTime / sweepTime);
        }

        // Drive-controlled saturation
        out = 5.f * std::tanh(out * driveGain);

        // Accent: extra level boost latched on triggers that arrive while the accent gate is high
        float accentGain = 1.f + accentDepth * (accentActive ? 1.f : 0.f);
        outputs[AUDIO_OUTPUT].setVoltage(out * level * accentGain);
    }
};

struct RaClapWidget : ModuleWidget {
    RaClapWidget(RaClapModule* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-clap.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float cx = box.size.x / 2;

        addInput(createInputCentered<RaPort>(Vec(cx, 25), module, RaClapModule::TRIG_INPUT));

        // Pitch section (left column): 1V/Oct knob + CV, FM attn + input, aligned to grid IO rows
        addParam(createParamCentered<RaKnob>(Vec(20, 75), module, RaClapModule::PITCH_PARAM));
        addInput(createInputCentered<RaPort>(Vec(20, 112), module, RaClapModule::PITCH_CV_INPUT));
        addParam(createParamCentered<RaKnob>(Vec(20, 160), module, RaClapModule::FM_ATTN_PARAM));
        addInput(createInputCentered<RaPort>(Vec(20, 197), module, RaClapModule::FM_INPUT));

        // Row 1
        addParam(createParamCentered<RaKnob>(Vec(57, 75), module, RaClapModule::TONE_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(93, 75), module, RaClapModule::TAPS_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(130, 75), module, RaClapModule::PITCH_DROP_PARAM));
        addInput(createInputCentered<RaPort>(Vec(57, 112), module, RaClapModule::TONE_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(93, 112), module, RaClapModule::TAPS_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(130, 112), module, RaClapModule::PITCH_DROP_CV_INPUT));

        // Row 2
        addParam(createParamCentered<RaKnob>(Vec(57, 160), module, RaClapModule::DECAY_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(93, 160), module, RaClapModule::SNAP_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(130, 160), module, RaClapModule::ACCENT_PARAM));
        addInput(createInputCentered<RaPort>(Vec(57, 197), module, RaClapModule::DECAY_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(93, 197), module, RaClapModule::SNAP_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(130, 197), module, RaClapModule::ACCENT_INPUT));

        // Row 3 — Level is the last knob (bottom right)
        addParam(createParamCentered<RaKnob>(Vec(57, 245), module, RaClapModule::PITCH_TIME_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(93, 245), module, RaClapModule::DRIVE_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(130, 245), module, RaClapModule::LEVEL_PARAM));
        addInput(createInputCentered<RaPort>(Vec(57, 282), module, RaClapModule::PITCH_TIME_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(93, 282), module, RaClapModule::DRIVE_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(130, 282), module, RaClapModule::LEVEL_CV_INPUT));

        addOutput(createOutputCentered<RaPort>(Vec(cx, 330), module, RaClapModule::AUDIO_OUTPUT));
    }
};

Model* modelRaClap = createModel<RaClapModule, RaClapWidget>("ra-clap");