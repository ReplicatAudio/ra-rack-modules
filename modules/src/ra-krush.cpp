#include "ra-components.hpp"
#include <cmath>
#include <cstring>

using namespace rack;

extern Plugin *pluginInstance;

// Port of the x-bitcrusher (KingKrusher) krush DSP chain:
// crush mapping -> anti-alias biquad -> downsampler -> quantizer -> slew,
// with an optional TPT-SVF frequency split (crush the low band, pass the
// high band clean).
//
// Polyphony: full per-channel processing (one DSP state set per voice), or
// sum-to-mono via the context menu (Valley Plateau style).

static float juceRandom() {
    return random::uniform();
}

static int juceRandomInt(int n) {
    return (int)(random::uniform() * n);
}

// Biquad lowpass matching juce::dsp::IIR::Coefficients::makeLowPass(sr, fc, Q)
struct BiQuadLP {
    float b0 = 1.f, b1 = 0.f, b2 = 0.f, a1 = 0.f, a2 = 0.f;
    float x1 = 0.f, x2 = 0.f, y1 = 0.f, y2 = 0.f;

    void setCutoff(float sr, float fc, float q) {
        fc = clamp(fc, 10.f, sr * 0.499f);
        float n = 1.f / std::tan(M_PI * fc / sr);
        float ns = n * n;
        float invQ = 1.f / q;
        float c1 = 1.f / (1.f + invQ * n + ns);
        b0 = c1;
        b1 = c1 * 2.f;
        b2 = c1;
        a1 = c1 * 2.f * (1.f - ns);
        a2 = c1 * (1.f - invQ * n + ns);
    }

    float process(float x) {
        float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = x;
        y2 = y1; y1 = y;
        return y;
    }

    void reset() {
        x1 = x2 = y1 = y2 = 0.f;
    }
};

// Trapezoidal state variable filter matching juce::dsp::StateVariableTPTFilter
struct TptSvf {
    float g = 0.1f, r2 = M_SQRT2, h = 0.5f;
    float s1 = 0.f, s2 = 0.f;

    void update(float sr, float fc, float res) {
        fc = clamp(fc, 10.f, sr * 0.49f);
        g = std::tan(M_PI * fc / sr);
        r2 = 1.f / res;
        h = 1.f / (1.f + r2 * g + g * g);
    }

    float lowpass(float x) {
        float yHP = h * (x - s1 * (g + r2) - s2);
        float yBP = yHP * g + s1;
        s1 = yHP * g + yBP;
        float yLP = yBP * g + s2;
        s2 = yBP * g + yLP;
        return yLP;
    }

    void reset() {
        s1 = s2 = 0.f;
    }
};

// Port of DownsampleProcessor (single voice)
struct Downsampler {
    enum Interp { ZOH, CUBIC, RANDOM_HOLD, JITTER };

    float phase = 0.f;
    float lastValue = 0.f, prevValue = 0.f, prevPrevValue = 0.f, prevPrevPrevValue = 0.f;
    static constexpr int rndBufferSize = 4;
    float rndBuffer[rndBufferSize] = {};
    int rndBufferIdx = 0;

    void reset() {
        lastValue = prevValue = prevPrevValue = prevPrevPrevValue = 0.f;
        phase = 0.f;
        for (auto &v : rndBuffer)
            v = 0.f;
        rndBufferIdx = 0;
    }

    // Returns the held/downsampled value for this input sample.
    // antiAliasIn: input already filtered by the anti-alias stage.
    float process(float in, float downsample, Interp interp) {
        float out = lastValue;

        switch (interp) {
            case ZOH: {
                if (phase >= downsample) {
                    phase -= downsample;
                    lastValue = in;
                }
                out = lastValue;
                break;
            }
            case RANDOM_HOLD: {
                float period = downsample / 4; // RND IS VERY HEAVY
                if (phase >= period) {
                    phase -= period;
                    rndBuffer[rndBufferIdx] = in;
                    rndBufferIdx = (rndBufferIdx + 1) % rndBufferSize;
                    lastValue = rndBuffer[juceRandomInt(rndBufferSize)];
                }
                out = lastValue;
                break;
            }
            case JITTER: {
                float jitterAmount = downsample * 0.15f;
                float jitteredPeriod =
                    downsample + (juceRandom() * 2.f - 1.f) * jitterAmount;
                jitteredPeriod = std::max(1.0f, jitteredPeriod);
                if (phase >= jitteredPeriod) {
                    phase -= jitteredPeriod;
                    lastValue = in;
                }
                out = lastValue;
                break;
            }
            case CUBIC: {
                float prevPhase = phase;
                if (phase >= downsample) {
                    phase -= downsample;
                    prevPrevPrevValue = prevPrevValue;
                    prevPrevValue = prevValue;
                    prevValue = lastValue;
                    lastValue = in;
                }
                float t = prevPhase / downsample;
                float p0 = prevPrevPrevValue;
                float p1 = prevPrevValue;
                float p2 = prevValue;
                float p3 = lastValue;
                out = 0.5f * ((2.0f * p1) + (-p0 + p2) * t +
                              (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t * t +
                              (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t * t * t);
                break;
            }
        }

        phase += 1.0f;
        return out;
    }
};

// Port of Quantizer (single voice)
struct Quantizer {
    enum Algo { STANDARD, DITHER, NOISE_SHAPED, CORRUPT };

    float quantError = 0.f;
    int glitchHold = 0;
    float glitchValue = 0.f;

    void reset() {
        quantError = 0.f;
        glitchHold = 0;
        glitchValue = 0.f;
    }

    float process(float val, float bitDepth, float crush, Algo algo) {
        switch (algo) {
            case STANDARD:
                return quantize(val, bitDepth);
            case DITHER:
                return quantizeDither(val, bitDepth);
            case NOISE_SHAPED:
                return quantizeNoiseShaped(val, bitDepth);
            case CORRUPT:
                return bitMask(quantize(val, bitDepth), bitDepth, crush);
        }
        return val;
    }

private:
    float quantize(float val, float bitDepth) {
        float maxVal = std::pow(2.0f, bitDepth) - 1.0f;
        float scaled = (val + 1.0f) * 0.5f * maxVal;
        float quantized = std::floor(scaled + 0.5f) / maxVal;
        float out = quantized * 2.0f - 1.0f;
        return clamp(out, -1.0f, 1.0f);
    }

    float quantizeDither(float val, float bitDepth) {
        float maxVal = std::pow(2.0f, bitDepth) - 1.0f;
        float dither = (juceRandom() * 2.0f - 1.0f) * (1.0f / maxVal);
        float scaled = (val + 1.0f) * 0.5f * maxVal + dither;
        float quantized = std::floor(scaled + 0.5f) / maxVal;
        float out = quantized * 2.0f - 1.0f;
        return clamp(out, -1.0f, 1.0f);
    }

    float quantizeNoiseShaped(float val, float bitDepth) {
        float maxVal = std::pow(2.0f, bitDepth) - 1.0f;
        float v = val + quantError;
        float scaled = (v + 1.0f) * 0.5f * maxVal;
        float quantized = std::floor(scaled + 0.5f) / maxVal;
        float output = quantized * 2.0f - 1.0f;
        quantError = v - output;
        return clamp(output, -1.0f, 1.0f);
    }

    float bitMask(float sample, float bitDepth, float crush) {
        if (bitDepth >= 16.0f || bitDepth <= 0.0f)
            return sample;

        int maxInt = (int)std::pow(2.0f, bitDepth) - 1;
        float glitchChance = crush * 0.005f;

        if (glitchHold > 0) {
            glitchHold--;
            return glitchValue;
        }

        if (juceRandom() >= glitchChance)
            return sample;

        float normalized = (sample + 1.0f) * 0.5f;
        int intVal = (int)(normalized * maxInt);

        int type = juceRandomInt(3);
        if (type == 0) {
            intVal = maxInt - intVal;
        } else if (type == 1) {
            intVal = 0;
        } else {
            int upperBits = 1;
            int mask = maxInt & ~((1 << ((int)bitDepth - upperBits)) - 1);
            intVal = intVal ^ mask;
        }
        intVal &= maxInt;

        glitchValue = ((float)intVal / (float)maxInt) * 2.0f - 1.0f;
        glitchHold = 1 + juceRandomInt(3);
        return std::tanh(glitchValue);
    }
};

// Per-voice DSP state
struct KrushVoice {
    BiQuadLP antiAliasFilter;
    Downsampler downsampler;
    Quantizer quantizer;
    TptSvf lpSplit[2];
    TptSvf splitPostLp[2];
    float slewPrev = 0.f;

    void reset() {
        antiAliasFilter.reset();
        downsampler.reset();
        quantizer.reset();
        for (auto &f : lpSplit)
            f.reset();
        for (auto &f : splitPostLp)
            f.reset();
        slewPrev = 0.f;
    }
};

struct RaKrushModule : Module {
    enum ParamIds {
        CRUSH_PARAM,
        ALGO_PARAM,
        SLEW_PARAM,
        CUT_PARAM,
        RES_PARAM,
        SPLIT_PARAM,
        FILTER_PARAM,
        ANTIALIAS_PARAM,
        QUANTIZE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        IN_INPUT,
        CRUSH_CV_INPUT,
        CUT_CV_INPUT,
        SLEW_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT_OUTPUT,
        LOW_OUTPUT,
        HIGH_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    static constexpr int NUM_VOICES = 16;

    // Krush algo indices (match plugin BitcrushAlgo order)
    static constexpr int ALGO_STD = 0;
    static constexpr int ALGO_JIT = 1;
    static constexpr int ALGO_RND = 2;
    static constexpr int ALGO_CUB = 3;
    static constexpr int ALGO_BTK = 4;
    static constexpr int ALGO_DWN = 5;
    static constexpr int ALGO_COR = 6;
    static constexpr int ALGO_NSH = 7;

    KrushVoice voices[NUM_VOICES];
    // Anti-alias coefficients are shared across voices (same cutoff); only
    // recomputed when the cutoff changes.
    float lastAntiAliasCutoff = -1.0f;

    // true = sum all input channels into one voice (Plateau style),
    // false = one independent voice per poly channel (default).
    bool sumMode = false;

    RaKrushModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(CRUSH_PARAM, 0.f, 1.f, 0.f, "Krush", "%", 0.f, 100.f);
        configParam(ALGO_PARAM, 0.f, 7.f, 0.f, "Krush algorithm");
        paramQuantities[ALGO_PARAM]->snapEnabled = true;
        configParam(SLEW_PARAM, 0.f, 1.f, 0.f, "Slew", "%", 0.f, 100.f);
        configParam(CUT_PARAM, 0.f, 1.f, 1.f, "Split cutoff", " Hz",
            20000.f / 20.f, 20.f); // log 20..20k
        configParam(RES_PARAM, 0.1f, 2.0f, 0.7f, "Split resonance");

        configSwitch(SPLIT_PARAM, 0.f, 1.f, 0.f, "Split mode", {"OFF", "ON"});
        configSwitch(FILTER_PARAM, 0.f, 1.f, 0.f, "Post-crush filter", {"OFF", "ON"});
        configSwitch(ANTIALIAS_PARAM, 0.f, 1.f, 1.f, "Anti-alias", {"OFF", "ON"});
        configSwitch(QUANTIZE_PARAM, 0.f, 1.f, 0.f, "Quantize", {"OFF", "ON"});

        configInput(IN_INPUT, "Audio");
        configInput(CRUSH_CV_INPUT, "Krush CV");
        configInput(CUT_CV_INPUT, "Split cutoff CV (1V/Oct)");
        configInput(SLEW_CV_INPUT, "Slew CV");

        configOutput(OUT_OUTPUT, "Audio");
        configOutput(LOW_OUTPUT, "Crushed low band");
        configOutput(HIGH_OUTPUT, "Clean high band");

        onReset();
    }

    void onReset() override {
        for (auto &v : voices)
            v.reset();
        lastAntiAliasCutoff = -1.0f;
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        json_object_set_new(rootJ, "sumMode", json_boolean(sumMode));
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *sumModeJ = json_object_get(rootJ, "sumMode");
        if (sumModeJ)
            sumMode = json_boolean_value(sumModeJ);
    }

    // Port of processBitcrush: returns crushed sample from an unfiltered input
    float processBitcrush(KrushVoice &v, float in, float bitDepth,
                          float downsample, int algo, float crush,
                          float sampleRate, bool antiAlias) {
        bool doDownsample = downsample > 1.5f;
        Downsampler::Interp interp = Downsampler::ZOH;
        bool runDownsampler = false;
        Quantizer::Algo quantAlgo = Quantizer::STANDARD;
        bool runQuantizer = true;

        switch (algo) {
            case ALGO_STD:
            case ALGO_NSH:
            case ALGO_DWN:
                interp = Downsampler::ZOH;
                runDownsampler = doDownsample;
                break;
            case ALGO_JIT:
                interp = Downsampler::JITTER;
                runDownsampler = doDownsample;
                break;
            case ALGO_RND:
                interp = Downsampler::RANDOM_HOLD;
                runDownsampler = doDownsample;
                break;
            case ALGO_CUB:
                interp = Downsampler::CUBIC;
                runDownsampler = doDownsample;
                break;
            case ALGO_COR:
                interp = Downsampler::ZOH;
                runDownsampler = doDownsample;
                break;
            case ALGO_BTK:
                runDownsampler = false;
                break;
        }

        switch (algo) {
            case ALGO_STD:
            case ALGO_JIT:
            case ALGO_RND:
            case ALGO_CUB:
            case ALGO_BTK:
                quantAlgo = Quantizer::STANDARD;
                break;
            case ALGO_COR:
                quantAlgo = Quantizer::CORRUPT;
                break;
            case ALGO_NSH:
                quantAlgo = Quantizer::NOISE_SHAPED;
                break;
            case ALGO_DWN:
                runQuantizer = false;
                break;
        }

        float x = in;

        // --- Anti-alias pre-filter ---
        if (runDownsampler && antiAlias) {
            float targetNyquist = (sampleRate / downsample) * 0.45f;
            if (targetNyquist != lastAntiAliasCutoff) {
                v.antiAliasFilter.setCutoff(sampleRate, targetNyquist, 0.70710678f);
                lastAntiAliasCutoff = targetNyquist;
            }
            x = v.antiAliasFilter.process(x);
        }

        // --- Downsampling ---
        if (runDownsampler)
            x = v.downsampler.process(x, downsample, interp);

        // --- Quantization ---
        if (runQuantizer)
            x = v.quantizer.process(x, bitDepth, crush, quantAlgo);

        return x;
    }

    // Port of processSlew
    float processSlew(KrushVoice &v, float dry, float slew, float sampleRate) {
        constexpr float kRefSampleRate = 48000.0f;
        float srRatio = sampleRate / kRefSampleRate;
        float maxSlew = 0.01f / srRatio;

        float delta = dry - v.slewPrev;
        delta = clamp(delta, -maxSlew, maxSlew);
        v.slewPrev += delta;
        return dry * (1.0f - slew) + v.slewPrev * slew;
    }

    void process(const ProcessArgs &args) override {
        float crush = clamp(
            params[CRUSH_PARAM].getValue() + inputs[CRUSH_CV_INPUT].getVoltage() / 10.f,
            0.f, 1.f);
        int algo = clamp((int)std::round(params[ALGO_PARAM].getValue()), 0, 7);
        float slewAmt = clamp(
            params[SLEW_PARAM].getValue() + inputs[SLEW_CV_INPUT].getVoltage() / 10.f,
            0.f, 1.f);
        float cutNorm = params[CUT_PARAM].getValue();
        float cutHz = 20.f * std::pow(20000.f / 20.f, cutNorm)
            * powf(2.f, inputs[CUT_CV_INPUT].getVoltage());
        cutHz = clamp(cutHz, 10.f, args.sampleRate * 0.45f);
        float res = params[RES_PARAM].getValue();

        bool split = params[SPLIT_PARAM].getValue() > 0.5f;
        bool splitFilter = params[FILTER_PARAM].getValue() > 0.5f;
        bool antiAlias = params[ANTIALIAS_PARAM].getValue() > 0.5f;
        bool quantMode = params[QUANTIZE_PARAM].getValue() > 0.5f;

        float bitDepth = 16.0f - std::pow(crush, 0.35f) * 13.0f;
        float downsample = 1.0f + std::pow(crush, 1.5f) * 31.0f;
        if (quantMode) {
            bitDepth = std::round(bitDepth);
            downsample = std::round(downsample);
        }

        float sr = args.sampleRate;
        int channels;
        if (sumMode) {
            channels = 1;
        } else {
            channels = std::max(1, inputs[IN_INPUT].getChannels());
        }

        for (int c = 0; c < channels; c++) {
            KrushVoice &v = voices[c];
            float in = sumMode ? inputs[IN_INPUT].getVoltageSum()
                               : inputs[IN_INPUT].getPolyVoltage(c);
            in /= 5.f;

            // --- Split band separation ---
            float lowBand, highBand;
            if (split) {
                v.lpSplit[0].update(sr, cutHz, res);
                v.lpSplit[1].update(sr, cutHz, res);
                lowBand = v.lpSplit[1].lowpass(v.lpSplit[0].lowpass(in));
                highBand = in - lowBand;
            } else {
                lowBand = in;
                highBand = 0.f;
            }

            // --- Crush the low band ---
            lowBand = processBitcrush(v, lowBand, bitDepth, downsample, algo,
                crush, sr, antiAlias);

            // --- Optional post-crush LP on the low band (split mode only) ---
            if (split && splitFilter) {
                v.splitPostLp[0].update(sr, cutHz, res);
                v.splitPostLp[1].update(sr, cutHz, res);
                lowBand = v.splitPostLp[1].lowpass(
                    v.splitPostLp[0].lowpass(lowBand));
            }

            // --- Slew (low band only in split mode, full band otherwise) ---
            if (slewAmt > 0.f)
                lowBand = processSlew(v, lowBand, slewAmt, sr);

            // --- Outputs ---
            outputs[OUT_OUTPUT].setVoltage((lowBand + highBand) * 5.f, c);
            outputs[LOW_OUTPUT].setVoltage(lowBand * 5.f, c);
            outputs[HIGH_OUTPUT].setVoltage(split ? highBand * 5.f : 0.f, c);
        }

        outputs[OUT_OUTPUT].setChannels(channels);
        outputs[LOW_OUTPUT].setChannels(channels);
        outputs[HIGH_OUTPUT].setChannels(channels);
    }
};

struct RaKrushWidget : ModuleWidget {
    RaKrushWidget(RaKrushModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-krush.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float cx = box.size.x / 2;

        // Input at top center
        addInput(createInputCentered<RaPort>(Vec(cx, 28), module, RaKrushModule::IN_INPUT));

        // Row 1: Crush | Algo
        addParam(createParamCentered<RaKnob>(Vec(54, 68), module, RaKrushModule::CRUSH_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(126, 68), module, RaKrushModule::ALGO_PARAM));

        // Row 2: Crush CV | Slew trim
        addInput(createInputCentered<RaPort>(Vec(54, 105), module, RaKrushModule::CRUSH_CV_INPUT));
        addParam(createParamCentered<RaKnobTrim>(Vec(126, 105), module, RaKrushModule::SLEW_PARAM));

        // Row 3: Split cutoff | Split resonance
        addParam(createParamCentered<RaKnob>(Vec(54, 148), module, RaKrushModule::CUT_PARAM));
        addParam(createParamCentered<RaKnobTrim>(Vec(126, 148), module, RaKrushModule::RES_PARAM));

        // Row 4: Cutoff CV | Slew CV
        addInput(createInputCentered<RaPort>(Vec(54, 184), module, RaKrushModule::CUT_CV_INPUT));
        addInput(createInputCentered<RaPort>(Vec(126, 184), module, RaKrushModule::SLEW_CV_INPUT));

        // Switch row: SPLIT, FILTER, ANTI-ALIAS, QUANTIZE
        addParam(createParamCentered<RaSwitch2>(Vec(27, 225), module, RaKrushModule::SPLIT_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(69, 225), module, RaKrushModule::FILTER_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(111, 225), module, RaKrushModule::ANTIALIAS_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(153, 225), module, RaKrushModule::QUANTIZE_PARAM));

        // Bottom row: LOW, HIGH, OUT
        addOutput(createOutputCentered<RaPort>(Vec(30, 290), module, RaKrushModule::LOW_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(cx, 290), module, RaKrushModule::HIGH_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(150, 290), module, RaKrushModule::OUT_OUTPUT));
    }

    void appendContextMenu(Menu *menu) override {
        RaKrushModule *module = getModule<RaKrushModule>();
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Polyphony"));
        menu->addChild(createMenuItem("Full poly (per voice)", module->sumMode ? "" : "\u2714",
            [=]() { module->sumMode = false; }));
        menu->addChild(createMenuItem("Sum channels (mono mix)", module->sumMode ? "\u2714" : "",
            [=]() { module->sumMode = true; }));
    }
};

Model *modelRaKrush = createModel<RaKrushModule, RaKrushWidget>("ra-krush");
