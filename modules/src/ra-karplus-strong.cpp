#include "ra-components.hpp"
#include <vector>
#include <cmath>
#include <cstring>

using namespace rack;

extern Plugin *pluginInstance;

static constexpr int MAX_DELAY = 4096;
static constexpr float MIN_FREQ = 20.f;
static constexpr float MAX_FREQ = 4000.f;
static constexpr float REF_FREQ = 65.406f; // C2 at 0 V

struct RaKarplusStrongModule : Module {
    enum ParamIds {
        FREQ_PARAM,
        FM_ATTN_PARAM,
        DAMP_PARAM,
        FEEDBACK_PARAM,
        BRIGHTNESS_PARAM,
        PICK_POS_PARAM,
        STIFFNESS_PARAM,
        LEVEL_PARAM,
        EXCITE_MODE_PARAM,
        SYMP_COUNT_PARAM,
        SYMP_DETUNE_PARAM,
        SYMP_LEVEL_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        TRIG_INPUT,
        PITCH_INPUT,
        FM_INPUT,
        DAMP_CV_INPUT,
        FEEDBACK_CV_INPUT,
        BRIGHTNESS_CV_INPUT,
        PICK_POS_CV_INPUT,
        STIFFNESS_CV_INPUT,
        LEVEL_CV_INPUT,
        SYMP_DETUNE_CV_INPUT,
        SYMP_COUNT_CV_INPUT,
        EXCITE_MODE_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        AUDIO_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    // Excitation modes
    static constexpr int EXCITE_NOISE = 0;
    static constexpr int EXCITE_CHIRP = 1;
    static constexpr int EXCITE_SAW = 2;
    static constexpr int EXCITE_SQUARE = 3;
    static constexpr int EXCITE_SINE = 4;

    // Delay line
    float delayBuf[MAX_DELAY] = {};
    int writePos = 0;
    int delayLen = 100;

    // Filter state
    float prevDelayOut = 0.f;

    // Pick position comb filter state
    float combBuf[MAX_DELAY] = {};
    int combWritePos = 0;

    // Stiffness allpass state
    float stiffXPrev = 0.f;
    float stiffYPrev = 0.f;

    // Excitation buffer
    float exciteBuf[MAX_DELAY] = {};
    int exciteLen = 0;

    // Sympathetic strings — secondary KS resonators driven by the main string
    static constexpr int NUM_SYMP = 3;
    // Per-string detune spread in semitones at full detune setting
    static constexpr float SYMP_SPREAD_SEMIS[NUM_SYMP] = {0.35f, -0.28f, 0.5f};

    float sympBuf[NUM_SYMP][MAX_DELAY] = {};
    int sympWritePos[NUM_SYMP] = {};
    float sympPrevOut[NUM_SYMP] = {};

    dsp::SchmittTrigger trigger;

    RaKarplusStrongModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(FREQ_PARAM, 0.f, 1.f, 0.5f, "Frequency", " Hz");
        configParam(FM_ATTN_PARAM, 0.f, 1.f, 0.f, "FM attenuation", "%", 0.f, 100.f);
        configParam(DAMP_PARAM, 0.f, 1.f, 0.3f, "Damping", "%", 0.f, 100.f);
        configParam(FEEDBACK_PARAM, 0.f, 1.1f, 0.85f, "Feedback", "%", 0.f, 100.f);
        configParam(BRIGHTNESS_PARAM, 0.f, 1.f, 0.5f, "Brightness", "%", 0.f, 100.f);
        configParam(PICK_POS_PARAM, 0.f, 1.f, 0.3f, "Pick position", "%", 0.f, 100.f);
        configParam(STIFFNESS_PARAM, 0.f, 1.f, 0.f, "Stiffness", "%", 0.f, 100.f);
        configParam(LEVEL_PARAM, 0.f, 1.f, 0.8f, "Level", "%", 0.f, 100.f);
        configParam(EXCITE_MODE_PARAM, 0.f, 4.f, 0.f, "Excitation mode");

        configInput(TRIG_INPUT, "Trigger");
        configInput(PITCH_INPUT, "1V/Oct");
        configInput(FM_INPUT, "FM");
        configInput(DAMP_CV_INPUT, "Damping CV");
        configInput(FEEDBACK_CV_INPUT, "Feedback CV");
        configInput(BRIGHTNESS_CV_INPUT, "Brightness CV");
        configInput(PICK_POS_CV_INPUT, "Pick position CV");
        configInput(STIFFNESS_CV_INPUT, "Stiffness CV");
        configInput(LEVEL_CV_INPUT, "Level CV");
        configInput(SYMP_DETUNE_CV_INPUT, "Sympathetic detune CV");
        configInput(SYMP_COUNT_CV_INPUT, "Sympathetic strings CV");
        configInput(EXCITE_MODE_CV_INPUT, "Excitation mode CV");

        configParam(SYMP_COUNT_PARAM, 0.f, 3.f, 2.f, "Sympathetic strings", " strings", 0.f, 1.f, 0.f);
        configParam(SYMP_DETUNE_PARAM, 0.f, 1.f, 0.4f, "Sympathetic detune", "%", 0.f, 100.f);
        configParam(SYMP_LEVEL_PARAM, 0.f, 1.f, 0.3f, "Sympathetic level", "%", 0.f, 100.f);
        configOutput(AUDIO_OUTPUT, "Audio");

        resetDelay();
    }

    void resetDelay() {
        std::memset(delayBuf, 0, sizeof(delayBuf));
        std::memset(combBuf, 0, sizeof(combBuf));
        std::memset(sympBuf, 0, sizeof(sympBuf));
        writePos = 0;
        combWritePos = 0;
        for (int i = 0; i < NUM_SYMP; i++) {
            sympWritePos[i] = 0;
            sympPrevOut[i] = 0.f;
        }
        prevDelayOut = 0.f;
        stiffXPrev = 0.f;
        stiffYPrev = 0.f;
    }

    void onReset() override {
        resetDelay();
    }

    void generateExcitation(int mode, int len, float brightness) {
        exciteLen = len;

        // Brightness controls a simple one-pole lowpass on the excitation
        // Higher brightness = more high-frequency content (1.0 = unfiltered)
        float lpCoeff = clamp(brightness, 0.02f, 1.f);
        float lpState = 0.f;

        for (int i = 0; i < len; i++) {
            float t = (float)i / (float)len;
            float sample = 0.f;

            switch (mode) {
                case EXCITE_NOISE: {
                    // Band-limited noise burst
                    sample = random::uniform() * 2.f - 1.f;
                    break;
                }
                case EXCITE_CHIRP: {
                    // Rapid sine sweep from high to low frequency
                    float chirpPhase = t * (10.f - 8.f * t);
                    sample = std::sin(2.f * M_PI * chirpPhase);
                    break;
                }
                case EXCITE_SAW: {
                    // Single cycle sawtooth
                    sample = 2.f * t - 1.f;
                    break;
                }
                case EXCITE_SQUARE: {
                    // Single cycle square
                    sample = (t < 0.5f) ? 1.f : -1.f;
                    break;
                }
                case EXCITE_SINE: {
                    // Single cycle sine
                    sample = std::sin(2.f * M_PI * t);
                    break;
                }
            }

            // Lowpass the excitation (brightness)
            lpState += lpCoeff * (sample - lpState);

            // Mild decay envelope across the burst
            float env = std::exp(-t * (4.f - 3.f * brightness));
            exciteBuf[i] = lpState * env;
        }
    }

    void process(const ProcessArgs &args) override {
        float freq = MIN_FREQ * powf(MAX_FREQ / MIN_FREQ, params[FREQ_PARAM].getValue());
        float pitch = inputs[PITCH_INPUT].getVoltage()
            + inputs[FM_INPUT].getVoltage() * params[FM_ATTN_PARAM].getValue();
        freq *= powf(2.f, pitch);
        freq = clamp(freq, MIN_FREQ, MAX_FREQ);

        float damping = clamp(
            params[DAMP_PARAM].getValue() + inputs[DAMP_CV_INPUT].getVoltage() / 10.f,
            0.f, 1.f);
        float feedback = clamp(
            params[FEEDBACK_PARAM].getValue() + inputs[FEEDBACK_CV_INPUT].getVoltage() / 10.f,
            0.f, 1.1f);
        float brightness = clamp(
            params[BRIGHTNESS_PARAM].getValue() + inputs[BRIGHTNESS_CV_INPUT].getVoltage() / 10.f,
            0.f, 1.f);
        float pickPos = clamp(
            params[PICK_POS_PARAM].getValue() + inputs[PICK_POS_CV_INPUT].getVoltage() / 10.f,
            0.f, 1.f);
        float stiffness = clamp(
            params[STIFFNESS_PARAM].getValue() + inputs[STIFFNESS_CV_INPUT].getVoltage() / 10.f,
            0.f, 1.f);
        float level = clamp(
            params[LEVEL_PARAM].getValue() + inputs[LEVEL_CV_INPUT].getVoltage() / 10.f,
            0.f, 1.f);

        // Calculate delay line length from frequency
        // Account for filter group delay: ~0.5 samples for the averaging filter
        float idealDelay = args.sampleRate / freq;
        delayLen = std::max(2, std::min(MAX_DELAY - 1, (int)std::round(idealDelay)));

        // Pick position comb filter delay (0 = no comb, full = center of string)
        // The comb creates notches at harmonics based on pick position
        int combDelay = std::max(0, std::min(delayLen - 1, (int)(pickPos * delayLen)));

        // Stiffness allpass coefficient
        // Higher stiffness = more inharmonicity (like a stiff string)
        float stiffCoeff = stiffness * 0.4f;

        // --- Trigger detection ---
        if (trigger.process(inputs[TRIG_INPUT].getVoltage())) {
            // Generate new excitation
            int burstLen = delayLen;
            generateExcitation(
                clamp((int)std::round(params[EXCITE_MODE_PARAM].getValue()
                    + inputs[EXCITE_MODE_CV_INPUT].getVoltage()), 0, 4),
                burstLen, brightness);

            // Reset delay line with excitation
            resetDelay();
            for (int i = 0; i < burstLen; i++) {
                delayBuf[i % delayLen] = exciteBuf[i];
            }
            writePos = burstLen % delayLen;
        }

        // --- Per-sample processing ---
        float out = 0.f;

        // Read from delay line (linear interpolation for fractional delay)
        float readPos = (float)writePos - (float)delayLen;
        if (readPos < 0.f) readPos += (float)delayLen;
        int idx0 = (int)readPos;
        int idx1 = idx0 + 1;
        if (idx1 >= delayLen) idx1 -= delayLen;
        float frac = readPos - (float)idx0;
        float delayOut = delayBuf[idx0] * (1.f - frac) + delayBuf[idx1] * frac;

        // --- Damping filter (simple 1-pole lowpass) ---
        // damping = 0 -> bright, damping = 1 -> very damped
        // The averaging filter: out = (prev + current) / 2
        // More damping: single-pole lowpass with variable cutoff
        float dampCoeff = 0.1f + 0.9f * (1.f - damping); // higher damping = lower cutoff
        float filtered = prevDelayOut + dampCoeff * (delayOut - prevDelayOut);
        prevDelayOut = filtered;

        // --- Pick position comb filter ---
        // H_beta(z) = 1 - z^{-beta*N} creates notches
        if (combDelay > 0) {
            int combIdx = writePos - combDelay;
            if (combIdx < 0) combIdx += delayLen;
            float combSample = combBuf[combIdx % delayLen];
            // Normalized comb: y = (x - a*x_delayed) / (1 + a)
            // Keeps peak loop gain at unity so Feedback alone controls sustain
            filtered = (filtered - pickPos * 0.7f * combSample) / (1.f + pickPos * 0.7f);
        }

        // Store into comb buffer
        combBuf[combWritePos] = filtered;
        combWritePos++;
        if (combWritePos >= delayLen) combWritePos = 0;

        // --- Stiffness allpass ---
        // First-order allpass y = a*x + x[n-1] - a*y[n-1]
        // Adds inharmonic stretch of upper partials (stiff string behavior)
        if (stiffCoeff > 0.001f) {
            float apOut = stiffCoeff * filtered + stiffXPrev - stiffCoeff * stiffYPrev;
            stiffXPrev = filtered;
            stiffYPrev = apOut;
            filtered = apOut;
        }

        // --- Feedback with gain ---
        float feedbackGain = feedback;
        filtered *= feedbackGain;

        // --- Write back to delay line ---
        delayBuf[writePos] = filtered;
        writePos++;
        if (writePos >= delayLen) writePos = 0;

        // --- Output ---
        out = filtered;

        // --- Sympathetic strings ---
        // Count is quantized: 1V per string, clamped to 0..3
        int sympCount = clamp((int)std::round(params[SYMP_COUNT_PARAM].getValue()
            + inputs[SYMP_COUNT_CV_INPUT].getVoltage()), 0, NUM_SYMP);
        if (sympCount > 0) {
            float sympDetune = clamp(
                params[SYMP_DETUNE_PARAM].getValue() + inputs[SYMP_DETUNE_CV_INPUT].getVoltage() / 10.f,
                0.f, 1.f);
            float sympLevel = params[SYMP_LEVEL_PARAM].getValue();

            float drive = filtered * 0.5f;
            float sympSum = 0.f;
            for (int i = 0; i < sympCount; i++) {
                float sympFreq = freq * powf(2.f, SYMP_SPREAD_SEMIS[i] * sympDetune / 12.f);
                float sympIdealDelay = args.sampleRate / sympFreq;
                int sympDelayLen = std::max(2, std::min(MAX_DELAY - 1, (int)std::round(sympIdealDelay)));

                // Read head one delay length behind write position
                int readIdx = sympWritePos[i] - sympDelayLen;
                if (readIdx < 0) readIdx += MAX_DELAY;
                float sympDelayOut = sympBuf[i][readIdx];

                // Same damping filter as the main loop
                float sympFiltered = sympPrevOut[i] + dampCoeff * (sympDelayOut - sympPrevOut[i]);
                sympPrevOut[i] = sympFiltered;

                // Slightly lower feedback so sympathetic strings decay a bit faster
                sympFiltered = drive + sympFiltered * feedbackGain * 0.985f;
                sympBuf[i][sympWritePos[i]] = sympFiltered;
                if (++sympWritePos[i] >= MAX_DELAY) sympWritePos[i] = 0;

                sympSum += sympFiltered - drive;
            }
            out += sympLevel * (sympSum / (float)sympCount);
        }

        // Soft clip to prevent blowup
        out = std::tanh(out * 2.f);

        outputs[AUDIO_OUTPUT].setVoltage(out * level * 5.f);
    }
};

// Out-of-class definition (required for odr-used constexpr array member in C++11)
constexpr float RaKarplusStrongModule::SYMP_SPREAD_SEMIS[];

struct RaKarplusStrongWidget : ModuleWidget {
    RaKarplusStrongWidget(RaKarplusStrongModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-karplus-strong.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float cx = box.size.x / 2;

        // Trigger at top center
        addInput(createInputCentered<RaPort>(Vec(cx, 25), module, RaKarplusStrongModule::TRIG_INPUT));

        // Row 1: Frequency knob + V/Oct + FM | Damping + Feedback
        addParam(createParamCentered<RaKnob>(Vec(24, 68), module, RaKarplusStrongModule::FREQ_PARAM));
        addInput(createInputCentered<RaPort>(Vec(24, 105), module, RaKarplusStrongModule::PITCH_INPUT));
        addParam(createParamCentered<RaKnob>(Vec(68, 68), module, RaKarplusStrongModule::FM_ATTN_PARAM));
        addInput(createInputCentered<RaPort>(Vec(68, 105), module, RaKarplusStrongModule::FM_INPUT));
        addParam(createParamCentered<RaKnob>(Vec(112, 68), module, RaKarplusStrongModule::DAMP_PARAM));
        addInput(createInputCentered<RaPort>(Vec(112, 105), module, RaKarplusStrongModule::DAMP_CV_INPUT));
        addParam(createParamCentered<RaKnob>(Vec(156, 68), module, RaKarplusStrongModule::FEEDBACK_PARAM));
        addInput(createInputCentered<RaPort>(Vec(156, 105), module, RaKarplusStrongModule::FEEDBACK_CV_INPUT));

        // Row 2: Brightness, Pick position, Stiffness, Excitation mode (+CV)
        addParam(createParamCentered<RaKnob>(Vec(24, 152), module, RaKarplusStrongModule::BRIGHTNESS_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(68, 152), module, RaKarplusStrongModule::PICK_POS_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(112, 152), module, RaKarplusStrongModule::STIFFNESS_PARAM));
        addInput(createInputCentered<RaPort>(Vec(24, 189), module, RaKarplusStrongModule::BRIGHTNESS_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(68, 189), module, RaKarplusStrongModule::PICK_POS_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(112, 189), module, RaKarplusStrongModule::STIFFNESS_CV_INPUT));
        addParam(createParamCentered<RaKnob>(Vec(156, 152), module, RaKarplusStrongModule::EXCITE_MODE_PARAM));
        addInput(createInputCentered<RaPort>(Vec(156, 189), module, RaKarplusStrongModule::EXCITE_MODE_CV_INPUT));

        // Row 3: Level | Sympathetic strings (+CV), detune (+CV)
        addParam(createParamCentered<RaKnob>(Vec(24, 236), module, RaKarplusStrongModule::LEVEL_PARAM));
        addInput(createInputCentered<RaPort>(Vec(24, 273), module, RaKarplusStrongModule::LEVEL_CV_INPUT));
        addParam(createParamCentered<RaKnob>(Vec(112, 236), module, RaKarplusStrongModule::SYMP_COUNT_PARAM));
        addInput(createInputCentered<RaPort>(Vec(112, 273), module, RaKarplusStrongModule::SYMP_COUNT_CV_INPUT));
        addParam(createParamCentered<RaKnob>(Vec(156, 236), module, RaKarplusStrongModule::SYMP_DETUNE_PARAM));
        addInput(createInputCentered<RaPort>(Vec(156, 273), module, RaKarplusStrongModule::SYMP_DETUNE_CV_INPUT));

        // Bottom row: sympathetic level trim + main output
        addParam(createParamCentered<RaKnob>(Vec(24, 330), module, RaKarplusStrongModule::SYMP_LEVEL_PARAM));
        addOutput(createOutputCentered<RaPort>(Vec(cx, 330), module, RaKarplusStrongModule::AUDIO_OUTPUT));
    }
};

Model *modelRaKarplusStrong = createModel<RaKarplusStrongModule, RaKarplusStrongWidget>("ra-karplus-strong");
