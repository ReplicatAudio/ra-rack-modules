// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: GATE_INPUT "Gate"
// fname: COLOR_PARAM "Color"
// fname: LP_CUT_PARAM "LP cut"
// fname: LP_CUT_CV_INPUT "LP cut C"
// fname: LP_RES_PARAM "LP res"
// fname: LP_RES_CV_INPUT "LP res C"
// fname: HP_CUT_PARAM "HP cut"
// fname: HP_CUT_CV_INPUT "HP cut C"
// fname: HP_RES_PARAM "HP res"
// fname: HP_RES_CV_INPUT "HP res C"
// fname: AMP_PARAM "Amplitud"
// fname: AMP_CV_INPUT "Amplitud"
// fname: AUDIO_OUTPUT "Audio"
#include "ra-components.hpp"
#include <cmath>
#include <cstring>

using namespace rack;

extern Plugin *pluginInstance;

struct RaNoyzFreqQuantity : ParamQuantity {
    float getDisplayValue() override {
        float v = getValue();
        return 20.f * powf(1000.f, v);
    }
};

// Pink noise — Voss algorithm (same as VCV Rack's stock Noise module,
// http://www.firstpr.com.au/dsp/pink-noise/)
template <int QUALITY = 8>
struct PinkNoiseGenerator {
    int frame = -1;
    float values[QUALITY] = {};

    float process() {
        int lastFrame = frame;
        frame++;
        if (frame >= (1 << QUALITY))
            frame = 0;
        int diff = lastFrame ^ frame;

        float sum = 0.f;
        for (int i = 0; i < QUALITY; i++) {
            if (diff & (1 << i))
                values[i] = random::uniform() - 0.5f;
            sum += values[i];
        }
        return sum;
    }
};

// Gray noise — inverse A-weighting FFT filter (same as VCV Rack's stock Noise module)
struct InverseAWeightingFFTFilter {
    static constexpr int BUFFER_LEN = 1024;

    float* inputBuffer;
    float* outputBuffer;
    float* freqBuffer;
    int frame = 0;
    dsp::RealFFT fft;

    InverseAWeightingFFTFilter() : fft(BUFFER_LEN) {
        inputBuffer = (float*)pffft_aligned_malloc(BUFFER_LEN * sizeof(float));
        outputBuffer = (float*)pffft_aligned_malloc(BUFFER_LEN * sizeof(float));
        freqBuffer = (float*)pffft_aligned_malloc(2 * BUFFER_LEN * sizeof(float));
        std::memset(inputBuffer, 0, BUFFER_LEN * sizeof(float));
        std::memset(outputBuffer, 0, BUFFER_LEN * sizeof(float));
        std::memset(freqBuffer, 0, 2 * BUFFER_LEN * sizeof(float));
    }

    ~InverseAWeightingFFTFilter() {
        pffft_aligned_free(inputBuffer);
        pffft_aligned_free(outputBuffer);
        pffft_aligned_free(freqBuffer);
    }

    float process(float deltaTime, float x) {
        inputBuffer[frame] = x;
        if (++frame >= BUFFER_LEN) {
            frame = 0;
            fft.rfft(inputBuffer, freqBuffer);

            for (int i = 0; i < BUFFER_LEN; i++) {
                float f = 1.f / deltaTime / 2.f / BUFFER_LEN * i;
                float amp = 0.f;
                if (80.f <= f && f <= 20000.f) {
                    float f2 = f * f;
                    // Inverse A-weighted curve
                    amp = ((424.36f + f2) * std::sqrt((11599.3f + f2) * (544496.f + f2)) * (148693636.f + f2)) / (148693636.f * f2 * f2);
                }
                freqBuffer[2 * i + 0] *= amp / BUFFER_LEN;
                freqBuffer[2 * i + 1] *= amp / BUFFER_LEN;
            }

            fft.irfft(freqBuffer, outputBuffer);
        }
        return outputBuffer[frame];
    }
};

// Trapezoidal state variable filter (matches juce::dsp::StateVariableTPTFilter);
// exposes both the lowpass and highpass outputs from the same state.
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

    float highpass(float x) {
        float yHP = h * (x - s1 * (g + r2) - s2);
        float yBP = yHP * g + s1;
        s1 = yHP * g + yBP;
        float yLP = yBP * g + s2;
        s2 = yBP * g + yLP;
        return yHP;
    }

    void reset() {
        s1 = s2 = 0.f;
    }
};

// Flat RGB LED. The stock MediumLight LED washes colors out two ways: its
// MediumLight.svg paints white glass glare over the color, and the SDK's
// screen blending of the three base colors pushes mixed colors (violet, pink)
// up toward white. This widget skips the glare overlay and mixes the RGB
// channels linearly, and skips the sqrt nonlinearity, so the brightness values
// set from process() are the LED color directly.
struct RaRgbLed : app::ModuleLightWidget {
    RaRgbLed() {
        addBaseColor(nvgRGB(0xff, 0x00, 0x00));
        addBaseColor(nvgRGB(0x00, 0xff, 0x00));
        addBaseColor(nvgRGB(0x00, 0x00, 0xff));
        box.size = mm2px(math::Vec(3, 3));
    }

    void step() override {
        std::vector<float> b(3, 1.f); // all on in editor preview (module == NULL)
        if (module) {
            if (module->isBypassed()
                || !(0 <= firstLightId && firstLightId + 3 <= (int)module->lights.size())) {
                b[0] = b[1] = b[2] = 0.f;
            }
            else {
                for (int i = 0; i < 3; i++) {
                    float v = module->lights[firstLightId + i].getBrightness();
                    b[i] = std::isfinite(v) ? math::clamp(v, 0.f, 1.f) : 0.f;
                }
            }
        }
        color = nvgRGBAf(b[0], b[1], b[2], 1.f);
        MultiLightWidget::step();
    }
};

struct RaNoyzModule : Module {
    enum ParamIds {
        COLOR_PARAM,
        LP_CUT_PARAM,
        LP_RES_PARAM,
        HP_CUT_PARAM,
        HP_RES_PARAM,
        AMP_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        GATE_INPUT,
        LP_CUT_CV_INPUT,
        LP_RES_CV_INPUT,
        HP_CUT_CV_INPUT,
        HP_RES_CV_INPUT,
        AMP_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        AUDIO_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        COLOR_LED_R,
        COLOR_LED_G,
        COLOR_LED_B,
        NUM_LIGHTS
    };

    static constexpr int COLOR_WHITE = 0;
    static constexpr int COLOR_PINK = 1;
    static constexpr int COLOR_RED = 2;
    static constexpr int COLOR_VIOLET = 3;
    static constexpr int COLOR_BLUE = 4;
    static constexpr int COLOR_GRAY = 5;
    static constexpr int COLOR_BLACK = 6;

    static constexpr float MIN_FILTER_FREQ = 20.f;
    static constexpr float MAX_FILTER_FREQ = 20000.f;

    PinkNoiseGenerator<8> pinkNoiseGenerator;
    dsp::IIRFilter<2, 2> redFilter;
    InverseAWeightingFFTFilter grayFilter;
    float lastWhite = 0.f;
    float lastPink = 0.f;

    TptSvf lpFilter;
    TptSvf hpFilter;

    RaNoyzModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configLight(COLOR_LED_R, "Color");
        configLight(COLOR_LED_G, "Color");
        configLight(COLOR_LED_B, "Color");
        configSwitch(COLOR_PARAM, 0.f, 6.f, 0.f, "Color", {"White", "Pink", "Brown/Red", "Violet", "Blue", "Gray", "Black"});
        configParam<RaNoyzFreqQuantity>(LP_CUT_PARAM, 0.f, 1.f, 1.f, "LP cut", " Hz");
        configParam(LP_RES_PARAM, 0.f, 1.f, 0.f, "LP res", "%", 0.f, 100.f);
        configParam<RaNoyzFreqQuantity>(HP_CUT_PARAM, 0.f, 1.f, 0.f, "HP cut", " Hz");
        configParam(HP_RES_PARAM, 0.f, 1.f, 0.f, "HP res", "%", 0.f, 100.f);
        configParam(AMP_PARAM, 0.f, 1.f, 1.f, "Amplitude", "%", 0.f, 100.f);

        // Hard-code coefficients for Butterworth lowpass with cutoff 20 Hz @ 44.1 kHz.
        const float b[] = {0.00425611, 0.00425611};
        const float a[] = {-0.99148778};
        redFilter.setCoefficients(b, a);

        configInput(GATE_INPUT, "Gate");
        configInput(LP_CUT_CV_INPUT, "LP cut CV");
        configInput(LP_RES_CV_INPUT, "LP res CV");
        configInput(HP_CUT_CV_INPUT, "HP cut CV");
        configInput(HP_RES_CV_INPUT, "HP res CV");
        configInput(AMP_CV_INPUT, "Amplitude CV");
        configOutput(AUDIO_OUTPUT, "Audio");
    }

    void process(const ProcessArgs &args) override {
        // Noise sources from VCV Rack's stock Noise module.
        // All noise except black is calibrated to 1 RMS, then scaled to match
        // the RMS of a sine wave with 5V amplitude.
        const float gain = 5.f / std::sqrt(2.f);

        float noise = 0.f;
        switch ((int)params[COLOR_PARAM].getValue()) {
            case COLOR_PINK: {
                // Pink noise: -3 dB/oct
                float pink = pinkNoiseGenerator.process() / 0.816f;
                noise = pink * gain;
                break;
            }
            case COLOR_RED: {
                // Red/brownian noise: -6 dB/oct
                float red = redFilter.process(random::normal()) / 0.0645f;
                noise = red * gain;
                break;
            }
            case COLOR_VIOLET: {
                // Violet noise: +6 dB/oct
                float white = random::normal();
                float violet = (white - lastWhite) / 1.41f;
                lastWhite = white;
                noise = violet * gain;
                break;
            }
            case COLOR_BLUE: {
                // Blue noise: +3 dB/oct
                float pink = pinkNoiseGenerator.process() / 0.816f;
                float blue = (pink - lastPink) / 0.705f;
                lastPink = pink;
                noise = blue * gain;
                break;
            }
            case COLOR_GRAY: {
                // Gray noise: psychoacoustic equal loudness
                float gray = grayFilter.process(args.sampleTime, random::normal()) / 1.67f;
                noise = gray * gain;
                break;
            }
            case COLOR_BLACK: {
                // Black noise: uniform random numbers
                noise = random::uniform() * 10.f - 5.f;
                break;
            }
            case COLOR_WHITE:
            default: {
                // White noise: 0 dB/oct
                noise = random::normal() * gain;
                break;
            }
        }

        // RGB LED showing the selected noise color
        switch ((int)params[COLOR_PARAM].getValue()) {
            case COLOR_PINK:
                lights[COLOR_LED_R].setBrightness(1.f);
                lights[COLOR_LED_G].setBrightness(0.55f);
                lights[COLOR_LED_B].setBrightness(0.7f);
                break;
            case COLOR_RED:
                lights[COLOR_LED_R].setBrightness(0.85f);
                lights[COLOR_LED_G].setBrightness(0.1f);
                lights[COLOR_LED_B].setBrightness(0.1f);
                break;
            case COLOR_VIOLET:
                lights[COLOR_LED_R].setBrightness(0.6f);
                lights[COLOR_LED_G].setBrightness(0.15f);
                lights[COLOR_LED_B].setBrightness(1.f);
                break;
            case COLOR_BLUE:
                lights[COLOR_LED_R].setBrightness(0.1f);
                lights[COLOR_LED_G].setBrightness(0.3f);
                lights[COLOR_LED_B].setBrightness(1.f);
                break;
            case COLOR_GRAY:
                lights[COLOR_LED_R].setBrightness(0.5f);
                lights[COLOR_LED_G].setBrightness(0.5f);
                lights[COLOR_LED_B].setBrightness(0.5f);
                break;
            case COLOR_BLACK:
                lights[COLOR_LED_R].setBrightness(0.f);
                lights[COLOR_LED_G].setBrightness(0.f);
                lights[COLOR_LED_B].setBrightness(0.f);
                break;
            case COLOR_WHITE:
            default:
                lights[COLOR_LED_R].setBrightness(1.f);
                lights[COLOR_LED_G].setBrightness(1.f);
                lights[COLOR_LED_B].setBrightness(1.f);
                break;
        }

        // Lowpass cutoff: when the CV is connected the knob becomes a unipolar
        // attenuator (0-100%) scaling the 1V/Oct CV above the 20 Hz minimum
        // (0 V = 20 Hz; +10 V = ~20 kHz at full knob)
        float lpKnob = params[LP_CUT_PARAM].getValue();
        float lpCutHz;
        if (inputs[LP_CUT_CV_INPUT].isConnected())
            lpCutHz = MIN_FILTER_FREQ * powf(2.f, inputs[LP_CUT_CV_INPUT].getVoltage() * lpKnob);
        else
            lpCutHz = MIN_FILTER_FREQ * powf(MAX_FILTER_FREQ / MIN_FILTER_FREQ, lpKnob);
        float lpRes = params[LP_RES_PARAM].getValue();
        if (inputs[LP_RES_CV_INPUT].isConnected())
            lpRes = clamp(lpRes * inputs[LP_RES_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);

        // Highpass cutoff: same attenuator behavior for the 1V/Oct CV
        float hpKnob = params[HP_CUT_PARAM].getValue();
        float hpCutHz;
        if (inputs[HP_CUT_CV_INPUT].isConnected())
            hpCutHz = MIN_FILTER_FREQ * powf(2.f, inputs[HP_CUT_CV_INPUT].getVoltage() * hpKnob);
        else
            hpCutHz = MIN_FILTER_FREQ * powf(MAX_FILTER_FREQ / MIN_FILTER_FREQ, hpKnob);
        float hpRes = params[HP_RES_PARAM].getValue();
        if (inputs[HP_RES_CV_INPUT].isConnected())
            hpRes = clamp(hpRes * inputs[HP_RES_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);

        // Resonance knob 0..1 -> Q 0.5 (flat) .. 20 (near self-oscillation)
        lpFilter.update(args.sampleRate, lpCutHz, 0.5f * powf(40.f, lpRes));
        hpFilter.update(args.sampleRate, hpCutHz, 0.5f * powf(40.f, hpRes));

        float out = lpFilter.lowpass(noise);
        out = hpFilter.highpass(out);

        float amp = params[AMP_PARAM].getValue();
        if (inputs[AMP_CV_INPUT].isConnected())
            amp = clamp(amp * inputs[AMP_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);

        // Gate: when patched, only sound while the gate is high; otherwise always on
        float gate = 1.f;
        if (inputs[GATE_INPUT].isConnected())
            gate = inputs[GATE_INPUT].getVoltage() >= 1.f ? 1.f : 0.f;

        outputs[AUDIO_OUTPUT].setVoltage(out * amp * gate);
    }
};

struct RaNoyzWidget : ModuleWidget {
    RaNoyzWidget(RaNoyzModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-noyz.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        addInput(createInputCentered<RaPort>(Vec(30, 24), module, RaNoyzModule::GATE_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(30, 64), module, RaNoyzModule::COLOR_PARAM));
        addChild(createLightCentered<RaRgbLed>(Vec(30, 295), module, RaNoyzModule::COLOR_LED_R));

        addParam(createParamCentered<RaKnobSmall>(Vec(14, 104), module, RaNoyzModule::LP_CUT_PARAM));
        addInput(createInputCentered<RaPort>(Vec(46, 104), module, RaNoyzModule::LP_CUT_CV_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(14, 142), module, RaNoyzModule::LP_RES_PARAM));
        addInput(createInputCentered<RaPort>(Vec(46, 142), module, RaNoyzModule::LP_RES_CV_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(14, 180), module, RaNoyzModule::HP_CUT_PARAM));
        addInput(createInputCentered<RaPort>(Vec(46, 180), module, RaNoyzModule::HP_CUT_CV_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(14, 218), module, RaNoyzModule::HP_RES_PARAM));
        addInput(createInputCentered<RaPort>(Vec(46, 218), module, RaNoyzModule::HP_RES_CV_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(14, 256), module, RaNoyzModule::AMP_PARAM));
        addInput(createInputCentered<RaPort>(Vec(46, 256), module, RaNoyzModule::AMP_CV_INPUT));

        addOutput(createOutputCentered<RaPort>(Vec(30, 320), module, RaNoyzModule::AUDIO_OUTPUT));
    }
};

Model *modelRaNoyz = createModel<RaNoyzModule, RaNoyzWidget>("ra-noyz");