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
        LIMIT_PARAM,
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
        SYMP_LEVEL_CV_INPUT,
        LIMIT_CV_INPUT,
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

    static constexpr int NUM_VOICES = 16;

    // Per-voice delay line
    float delayBuf[NUM_VOICES][MAX_DELAY] = {};
    int writePos[NUM_VOICES] = {};
    int delayLen[NUM_VOICES] = {};

    // Per-voice filter state
    float prevDelayOut[NUM_VOICES] = {};

    // Per-voice pick position comb filter state
    float combBuf[NUM_VOICES][MAX_DELAY] = {};
    int combWritePos[NUM_VOICES] = {};

    // Per-voice stiffness allpass state
    float stiffXPrev[NUM_VOICES] = {};
    float stiffYPrev[NUM_VOICES] = {};
    float stiffXPrev2[NUM_VOICES] = {};
    float stiffYPrev2[NUM_VOICES] = {};

    // Per-voice classic KS brightness filter state (previous input sample)
    float brightPrev[NUM_VOICES] = {};

    // Per-voice DC blocker state (feedback path, 2 cascaded poles)
    float dcXPrev[NUM_VOICES] = {};
    float dcYPrev[NUM_VOICES] = {};
    float dc2XPrev[NUM_VOICES] = {};
    float dc2YPrev[NUM_VOICES] = {};

    // Excitation buffer (generated on trigger, consumed immediately)
    float exciteBuf[MAX_DELAY] = {};
    int exciteLen = 0;

    // Sympathetic strings — secondary KS resonators driven by the main string
    static constexpr int NUM_SYMP = 8;
    // Per-string detune spread in semitones at full detune setting
    static constexpr float SYMP_SPREAD_SEMIS[NUM_SYMP] = {0.35f, -0.28f, 0.5f, 0.22f, -0.45f, 0.65f, -0.15f, 0.8f};

    // Per-symp-string DC blocker state (recirculated path, 2 cascaded poles)
    float sympDcXPrev[NUM_VOICES][NUM_SYMP] = {};
    float sympDcYPrev[NUM_VOICES][NUM_SYMP] = {};
    float sympDc2XPrev[NUM_VOICES][NUM_SYMP] = {};
    float sympDc2YPrev[NUM_VOICES][NUM_SYMP] = {};

    float sympBuf[NUM_VOICES][NUM_SYMP][MAX_DELAY] = {};
    int sympWritePos[NUM_VOICES][NUM_SYMP] = {};
    float sympPrevOut[NUM_VOICES][NUM_SYMP] = {};

    dsp::SchmittTrigger trigger[NUM_VOICES];

    RaKarplusStrongModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(FREQ_PARAM, 0.f, 1.f, 0.5f, "Frequency", " Hz", 200.f, 20.f);
        configParam(FM_ATTN_PARAM, 0.f, 1.f, 0.f, "FM attenuation", "%", 0.f, 100.f);
        configParam(DAMP_PARAM, 0.f, 1.f, 0.3f, "Damping", "%", 0.f, 100.f);
        configParam(FEEDBACK_PARAM, 0.f, 2.f, 0.85f, "Feedback", "%", 0.f, 100.f);
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
        configInput(SYMP_LEVEL_CV_INPUT, "Sympathetic level CV");
        configInput(EXCITE_MODE_CV_INPUT, "Excitation mode CV");
        configInput(LIMIT_CV_INPUT, "Loop limit CV");

        configParam(SYMP_COUNT_PARAM, 0.f, 8.f, 2.f, "Sympathetic strings", " strings", 0.f, 1.f, 0.f);
        paramQuantities[EXCITE_MODE_PARAM]->snapEnabled = true;
        paramQuantities[SYMP_COUNT_PARAM]->snapEnabled = true;
        configParam(SYMP_DETUNE_PARAM, 0.f, 1.f, 0.4f, "Sympathetic detune", "%", 0.f, 100.f);
        configParam(SYMP_LEVEL_PARAM, 0.f, 1.f, 0.3f, "Sympathetic level", "%", 0.f, 100.f);
        configParam(LIMIT_PARAM, 1.f, 64.f, 8.f, "Loop limit");
        configOutput(AUDIO_OUTPUT, "Audio");

        resetDelay();
    }

    void resetDelay() {
        std::memset(delayBuf, 0, sizeof(delayBuf));
        std::memset(combBuf, 0, sizeof(combBuf));
        std::memset(sympBuf, 0, sizeof(sympBuf));
        for (int v = 0; v < NUM_VOICES; v++) {
            writePos[v] = 0;
            combWritePos[v] = 0;
            delayLen[v] = 100;
            prevDelayOut[v] = 0.f;
            stiffXPrev[v] = 0.f;
            stiffYPrev[v] = 0.f;
            stiffXPrev2[v] = 0.f;
            stiffYPrev2[v] = 0.f;
            brightPrev[v] = 0.f;
            dcXPrev[v] = 0.f;
            dcYPrev[v] = 0.f;
            dc2XPrev[v] = 0.f;
            dc2YPrev[v] = 0.f;
            for (int i = 0; i < NUM_SYMP; i++) {
                sympWritePos[v][i] = 0;
                sympPrevOut[v][i] = 0.f;
                sympDcXPrev[v][i] = 0.f;
                sympDcYPrev[v][i] = 0.f;
                sympDc2XPrev[v][i] = 0.f;
                sympDc2YPrev[v][i] = 0.f;
            }
        }
    }

    void onReset() override {
        resetDelay();
    }

    void generateExcitation(int mode, int len, float brightness) {
        exciteLen = len;

        // Classic KS: the excitation is generated raw (no filtering here);
        // brightness acts on the ringing loop instead (see process()).
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
                    // Rapid sine sweep from high (10 cycles per burst) down
                    // to a low positive rate (2 cycles/burst), never crossing
                    // 0 Hz (the old sweep went through zero and reversed)
                    float chirpPhase = t * (10.f - 4.f * t);
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

            // Mild decay envelope across the burst
            float env = std::exp(-t * (4.f - 3.f * brightness));
            exciteBuf[i] = sample * env;
        }
    }

    void process(const ProcessArgs &args) override {
        int channels = std::max(1, inputs[PITCH_INPUT].getChannels());

        // Shared parameters (read once per sample, applied to all voices)
        float baseFreq = MIN_FREQ * powf(MAX_FREQ / MIN_FREQ, params[FREQ_PARAM].getValue());
        float fmAtten = params[FM_ATTN_PARAM].getValue();
        float damping = clamp(
            params[DAMP_PARAM].getValue() + inputs[DAMP_CV_INPUT].getVoltage() / 10.f,
            0.f, 1.f);
        float feedback = clamp(
            params[FEEDBACK_PARAM].getValue() + inputs[FEEDBACK_CV_INPUT].getVoltage() / 10.f,
            0.f, 2.f);
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

        // Damping filter coefficient (shared)
        // damping = 0 -> bright, damping = 1 -> very damped
        float dampCoeff = 0.1f + 0.9f * (1.f - damping); // higher damping = lower cutoff

        // Stiffness allpass pole (shared)
        // Two cascaded first-order allpasses with pole near DC give real
        // inharmonic stretch (f_k ~ k*sqrt(1+B k^2)); a single allpass with
        // this sign convention produced ~0 cents at string partials.
        float stiffP = stiffness * 0.85f;

        // Classic KS brightness (loop filter blend; 0.5 = classic average)
        float brightMix = brightness;

        // DC blocker coefficient (~8 Hz highpass)
        float dcRCoeff = 1.f - 2.f * M_PI * 8.f / args.sampleRate;

        int exciteMode = clamp((int)std::round(params[EXCITE_MODE_PARAM].getValue()
            + inputs[EXCITE_MODE_CV_INPUT].getVoltage()), 0, 4);

        // Loop limit (in-loop saturation ceiling, 1..64)
        float loopLimit = clamp(
            params[LIMIT_PARAM].getValue() + inputs[LIMIT_CV_INPUT].getVoltage(),
            1.f, 64.f);

        // Sympathetic string settings (shared)
        // Count is quantized: 1V per string, clamped to 0..NUM_SYMP
        int sympCount = clamp((int)std::round(params[SYMP_COUNT_PARAM].getValue()
            + inputs[SYMP_COUNT_CV_INPUT].getVoltage()), 0, NUM_SYMP);
        float sympDetune = clamp(
            params[SYMP_DETUNE_PARAM].getValue() + inputs[SYMP_DETUNE_CV_INPUT].getVoltage() / 10.f,
            0.f, 1.f);
        float sympLevel = clamp(
            params[SYMP_LEVEL_PARAM].getValue() + inputs[SYMP_LEVEL_CV_INPUT].getVoltage() / 10.f,
            0.f, 1.f);

        for (int c = 0; c < channels; c++) {
            float freq = baseFreq
                * powf(2.f, inputs[PITCH_INPUT].getVoltage(c)
                    + inputs[FM_INPUT].getVoltage(c) * fmAtten);
            freq = clamp(freq, MIN_FREQ, MAX_FREQ);

            // Calculate delay line length from frequency. Subtract the DC
            // group delays of the in-loop filters so the fundamental stays
            // in tune: damping 1-pole (1-a)/a, classic KS brightness filter
            // 0.5*b, stiffness allpasses 2*(1+p)/(1-p) when enabled.
            float stiffGD = (stiffP > 0.001f)
                ? 2.f * (1.f + stiffP) / (1.f - stiffP) : 0.f;
            float idealDelay = args.sampleRate / freq
                - (1.f - dampCoeff) / dampCoeff
                - 0.5f * brightMix
                - stiffGD;
            delayLen[c] = std::max(2, std::min(MAX_DELAY - 1, (int)std::round(idealDelay)));

            // Pick position comb filter delay (0 = no comb, full = nearly the
            // full string length). H(z) = (1 - beta z^-C)/(1 + beta) creates
            // notches at harmonics k where k*pickPos is an integer.
            int combDelay = std::max(0, std::min(delayLen[c] - 1, (int)(pickPos * delayLen[c])));

            // --- Trigger detection (per voice) ---
            if (trigger[c].process(inputs[TRIG_INPUT].getVoltage(c))) {
                // Generate new excitation
                int burstLen = delayLen[c];
                generateExcitation(exciteMode, burstLen, brightness);

                // Reset this voice's delay line with excitation
                std::memset(delayBuf[c], 0, sizeof(delayBuf[c]));
                std::memset(combBuf[c], 0, sizeof(combBuf[c]));
                writePos[c] = 0;
                combWritePos[c] = 0;
                prevDelayOut[c] = 0.f;
                stiffXPrev[c] = 0.f;
                stiffYPrev[c] = 0.f;
                stiffXPrev2[c] = 0.f;
                stiffYPrev2[c] = 0.f;
                brightPrev[c] = 0.f;
                dcXPrev[c] = 0.f;
                dcYPrev[c] = 0.f;
                dc2XPrev[c] = 0.f;
                dc2YPrev[c] = 0.f;
                for (int i = 0; i < NUM_SYMP; i++) {
                    sympWritePos[c][i] = 0;
                    sympPrevOut[c][i] = 0.f;
                    sympDcXPrev[c][i] = 0.f;
                    sympDcYPrev[c][i] = 0.f;
                    sympDc2XPrev[c][i] = 0.f;
                    sympDc2YPrev[c][i] = 0.f;
                }
                for (int i = 0; i < burstLen; i++) {
                    delayBuf[c][i % delayLen[c]] = exciteBuf[i];
                }
                writePos[c] = burstLen % delayLen[c];
            }

            // --- Per-sample processing ---
            float out = 0.f;

            // Read from delay line with fractional-delay interpolation.
            // Integer rounding alone quantizes pitch (up to half a sample,
            // ~0.7 semitones at the top of the range, breaking 1V/oct
            // tracking). Fractional part blends toward the neighboring tap;
            // at frac == 0 the read is identical to before.
            float readPos = (float)writePos[c] - (float)delayLen[c];
            if (readPos < 0.f) readPos += (float)delayLen[c];
            int idx0 = (int)readPos;
            float frac = idealDelay - (float)delayLen[c];  // in [-0.5, 0.5)
            // Guard the degenerate case where clamped delayLen diverges from
            // idealDelay (extreme damping/stiffness at the top of the range).
            if (frac < -0.5f) frac = -0.5f;
            if (frac >= 0.5f) frac = 0.5f;
            float delayOut;
            if (frac >= 0.f) {
                int idxB = idx0 - 1;
                if (idxB < 0) idxB += delayLen[c];
                delayOut = delayBuf[c][idx0] * (1.f - frac) + delayBuf[c][idxB] * frac;
            } else {
                int idxB = idx0 + 1;
                if (idxB >= delayLen[c]) idxB -= delayLen[c];
                delayOut = delayBuf[c][idx0] * (1.f + frac) + delayBuf[c][idxB] * (-frac);
            }

            // --- Damping filter (simple 1-pole lowpass) ---
            // More damping: single-pole lowpass with variable cutoff
            float filtered = prevDelayOut[c] + dampCoeff * (delayOut - prevDelayOut[c]);
            prevDelayOut[c] = filtered;

            // --- Brightness (classic KS loop filter) ---
            // Classic KS: brightness sits in the loop, not on the excitation.
            // 0 = no filtering, 0.5 = classic (x + x[n-1])/2 average,
            // 1 = maximum filtering. Its 0.5*b sample group delay is
            // compensated in idealDelay.
            filtered = filtered + 0.5f * brightMix * (brightPrev[c] - filtered);
            brightPrev[c] = filtered;

            // --- Pick position comb filter ---
            // H_beta(z) = 1 - z^{-beta*N} creates notches
            if (combDelay > 0) {
                int combIdx = writePos[c] - combDelay;
                if (combIdx < 0) combIdx += delayLen[c];
                float combSample = combBuf[c][combIdx % delayLen[c]];
                // Normalized comb: y = (x - beta*x_delayed) / (1 + beta).
                // Normalization equalizes the comb's PEAK gain to unity only;
                // at the fundamental the loop gain is
                // |1 - beta*e^(-j*2*pi*pickPos)| / (1 + beta), so sustain
                // varies with pick position (e.g. ~0.18 at full). This is
                // inherent classic-KS comb behavior, documented not a bug.
                filtered = (filtered - pickPos * 0.7f * combSample) / (1.f + pickPos * 0.7f);
            }

            // Store into comb buffer
            combBuf[c][combWritePos[c]] = filtered;
            combWritePos[c]++;
            if (combWritePos[c] >= delayLen[c]) combWritePos[c] = 0;

            // --- Stiffness allpass (2 cascaded stages) ---
            // y = -p*x + x[n-1] + p*y[n-1], pole at +p bends phase near DC
            // so group delay decreases across the partial range: upper
            // partials stretch upward monotonically (stiff-string behavior).
            if (stiffP > 0.001f) {
                float apOut = -stiffP * filtered + stiffXPrev[c] + stiffP * stiffYPrev[c];
                stiffXPrev[c] = filtered;
                stiffYPrev[c] = apOut;
                filtered = apOut;

                apOut = -stiffP * filtered + stiffXPrev2[c] + stiffP * stiffYPrev2[c];
                stiffXPrev2[c] = filtered;
                stiffYPrev2[c] = apOut;
                filtered = apOut;
            }

            // --- DC blocker (2 cascaded one-pole highpasses) ---
            // The LP/allpass have unity DC gain, so any residual DC grows
            // unboundedly when feedback > 100% until floats hit inf/NaN.
            // y[n] = x[n] - x[n-1] + R*y[n-1]
            float dcOut = filtered - dcXPrev[c] + dcRCoeff * dcYPrev[c];
            dcXPrev[c] = filtered;
            dcYPrev[c] = dcOut;
            float dcOut2 = dcOut - dc2XPrev[c] + dcRCoeff * dc2YPrev[c];
            dc2XPrev[c] = dcOut;
            dc2YPrev[c] = dcOut2;
            filtered = dcOut2;

            // --- Feedback with gain ---
            float feedbackGain = feedback;

            // --- Output (tapped before feedback gain, so low settings
            // yield audible short plucks instead of silence) ---
            out = filtered;

            filtered *= feedbackGain;

            // Bound the recirculated signal so runaway modes saturate
            // instead of overflowing to inf/NaN. Scaled tanh is transparent
            // at normal levels, preserving extreme distortion when driven.
            filtered = loopLimit * std::tanh(filtered / loopLimit);

            // --- Write back to delay line ---
            delayBuf[c][writePos[c]] = filtered;
            writePos[c]++;
            if (writePos[c] >= delayLen[c]) writePos[c] = 0;

            // --- Sympathetic strings ---
            if (sympCount > 0) {
                float drive = filtered * 0.5f;
                float sympSum = 0.f;
                for (int i = 0; i < sympCount; i++) {
                    float sympFreq = freq * powf(2.f, SYMP_SPREAD_SEMIS[i] * sympDetune / 12.f);
                    float sympIdealDelay = args.sampleRate / sympFreq;
                    int sympDelayLen = std::max(2, std::min(MAX_DELAY - 1, (int)std::round(sympIdealDelay)));

                    // Read head one delay length behind write position
                    int readIdx = sympWritePos[c][i] - sympDelayLen;
                    if (readIdx < 0) readIdx += MAX_DELAY;
                    float sympDelayOut = sympBuf[c][i][readIdx];

                    // Same damping filter as the main loop
                    float sympFiltered = sympPrevOut[c][i] + dampCoeff * (sympDelayOut - sympPrevOut[c][i]);
                    sympPrevOut[c][i] = sympFiltered;

                    // Slightly lower feedback so sympathetic strings decay a bit faster
                    // DC-block the recirculated path so the symp loop can't build
                    // up DC when feedback * 0.985 exceeds 1
                    float sympRecirc = sympFiltered * feedbackGain * 0.985f;
                    float sympDc = sympRecirc - sympDcXPrev[c][i] + dcRCoeff * sympDcYPrev[c][i];
                    sympDcXPrev[c][i] = sympRecirc;
                    sympDcYPrev[c][i] = sympDc;
                    float sympDc2 = sympDc - sympDc2XPrev[c][i] + dcRCoeff * sympDc2YPrev[c][i];
                    sympDc2XPrev[c][i] = sympDc;
                    sympDc2YPrev[c][i] = sympDc2;

                    sympFiltered = drive + sympDc2;

                    // Bound the recirculated signal so runaway modes
                    // saturate instead of overflowing to inf/NaN
                    sympFiltered = loopLimit * std::tanh(sympFiltered / loopLimit);

                    sympBuf[c][i][sympWritePos[c][i]] = sympFiltered;
                    if (++sympWritePos[c][i] >= MAX_DELAY) sympWritePos[c][i] = 0;

                    sympSum += sympFiltered - drive;
                }
                out += sympLevel * (sympSum / (float)sympCount);
            }

            // Soft clip to prevent blowup
            out = std::tanh(out * 2.f);

            outputs[AUDIO_OUTPUT].setVoltage(out * level * 5.f, c);
        }

        outputs[AUDIO_OUTPUT].setChannels(channels);
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

        // Row 1: Frequency knob + V/Oct + FM | Damping + Loop limit
        addParam(createParamCentered<RaKnob>(Vec(24, 68), module, RaKarplusStrongModule::FREQ_PARAM));
        addInput(createInputCentered<RaPort>(Vec(24, 105), module, RaKarplusStrongModule::PITCH_INPUT));
        addParam(createParamCentered<RaKnob>(Vec(68, 68), module, RaKarplusStrongModule::FM_ATTN_PARAM));
        addInput(createInputCentered<RaPort>(Vec(68, 105), module, RaKarplusStrongModule::FM_INPUT));
        addParam(createParamCentered<RaKnob>(Vec(112, 68), module, RaKarplusStrongModule::DAMP_PARAM));
        addInput(createInputCentered<RaPort>(Vec(112, 105), module, RaKarplusStrongModule::DAMP_CV_INPUT));
        addParam(createParamCentered<RaKnob>(Vec(156, 68), module, RaKarplusStrongModule::LIMIT_PARAM));
        addInput(createInputCentered<RaPort>(Vec(156, 105), module, RaKarplusStrongModule::LIMIT_CV_INPUT));

        // Row 2: Brightness, Pick position, Stiffness (+CV), Feedback pushed down
        addParam(createParamCentered<RaKnob>(Vec(24, 152), module, RaKarplusStrongModule::BRIGHTNESS_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(68, 152), module, RaKarplusStrongModule::PICK_POS_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(112, 152), module, RaKarplusStrongModule::STIFFNESS_PARAM));
        addInput(createInputCentered<RaPort>(Vec(24, 189), module, RaKarplusStrongModule::BRIGHTNESS_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(68, 189), module, RaKarplusStrongModule::PICK_POS_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(112, 189), module, RaKarplusStrongModule::STIFFNESS_CV_INPUT));
        addParam(createParamCentered<RaKnob>(Vec(156, 152), module, RaKarplusStrongModule::FEEDBACK_PARAM));
        addInput(createInputCentered<RaPort>(Vec(156, 189), module, RaKarplusStrongModule::FEEDBACK_CV_INPUT));

        // Row 3: Level | Sympathetic level, count (+CV), Excitation mode pushed down
        addParam(createParamCentered<RaKnob>(Vec(24, 236), module, RaKarplusStrongModule::LEVEL_PARAM));
        addInput(createInputCentered<RaPort>(Vec(24, 273), module, RaKarplusStrongModule::LEVEL_CV_INPUT));
        addParam(createParamCentered<RaKnob>(Vec(68, 236), module, RaKarplusStrongModule::SYMP_LEVEL_PARAM));
        addInput(createInputCentered<RaPort>(Vec(68, 273), module, RaKarplusStrongModule::SYMP_LEVEL_CV_INPUT));
        addParam(createParamCentered<RaKnob>(Vec(112, 236), module, RaKarplusStrongModule::SYMP_COUNT_PARAM));
        addInput(createInputCentered<RaPort>(Vec(112, 273), module, RaKarplusStrongModule::SYMP_COUNT_CV_INPUT));
        addParam(createParamCentered<RaKnob>(Vec(156, 236), module, RaKarplusStrongModule::SYMP_DETUNE_PARAM));
        addInput(createInputCentered<RaPort>(Vec(156, 273), module, RaKarplusStrongModule::SYMP_DETUNE_CV_INPUT));

        // Row 4: Excitation mode (+CV)
        addParam(createParamCentered<RaKnob>(Vec(156, 320), module, RaKarplusStrongModule::EXCITE_MODE_PARAM));
        addInput(createInputCentered<RaPort>(Vec(156, 357), module, RaKarplusStrongModule::EXCITE_MODE_CV_INPUT));

        // Bottom row: main output
        addOutput(createOutputCentered<RaPort>(Vec(cx, 330), module, RaKarplusStrongModule::AUDIO_OUTPUT));
    }
};

Model *modelRaKarplusStrong = createModel<RaKarplusStrongModule, RaKarplusStrongWidget>("ra-karplus-strong");
