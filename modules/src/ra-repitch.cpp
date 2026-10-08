// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: AUDIO_INPUT "In"
// fname: OCTAVE_PARAM "Oct"
// fname: OCTAVE_CV_INPUT "Oct CV"
// fname: SEMITONE_PARAM "Semi"
// fname: SEMITONE_CV_INPUT "Semi CV"
// fname: CENT_PARAM "Cent"
// fname: CENT_CV_INPUT "Cent CV"
// fname: MIX_PARAM "Mix"
// fname: MIX_CV_INPUT "Mix CV"
// fname: AUDIO_OUTPUT "Out"
#include "ra-components.hpp"

#include <vector>
#include <cmath>
#include <algorithm>

using namespace rack;

extern Plugin *pluginInstance;

static constexpr int FFT_SIZE = 2048;
static constexpr int HOP_SIZE = FFT_SIZE / 4;

struct RaRepitchModule : Module {
    enum ParamIds {
        OCTAVE_PARAM,
        SEMITONE_PARAM,
        CENT_PARAM,
        MIX_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        AUDIO_INPUT,
        OCTAVE_CV_INPUT,
        SEMITONE_CV_INPUT,
        CENT_CV_INPUT,
        MIX_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        AUDIO_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    dsp::RealFFT fft;

    std::vector<float> window;
    std::vector<float> inputBuffer;
    std::vector<float> outputBuffer;
    std::vector<float> lastPhase;
    std::vector<float> sumPhase;

    int pos = 0;
    int hopCount = HOP_SIZE;

    RaRepitchModule() : fft(FFT_SIZE) {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(OCTAVE_PARAM, -4.f, 4.f, 0.f, "Octave");
        configParam(SEMITONE_PARAM, -12.f, 12.f, 0.f, "Semitone");
        configParam(CENT_PARAM, -100.f, 100.f, 0.f, "Cent");
        configParam(MIX_PARAM, 0.f, 1.f, 1.f, "Mix", "%", 0.f, 100.f);
        configInput(AUDIO_INPUT, "Audio");
        configInput(OCTAVE_CV_INPUT, "Octave CV");
        configInput(SEMITONE_CV_INPUT, "Semitone CV");
        configInput(CENT_CV_INPUT, "Cent CV");
        configInput(MIX_CV_INPUT, "Mix CV");
        configOutput(AUDIO_OUTPUT, "Audio");

        window.resize(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE; i++)
            window[i] = 0.5f * (1.f - cosf(2.f * M_PI * i / (FFT_SIZE - 1)));

        inputBuffer.assign(FFT_SIZE, 0.f);
        outputBuffer.assign(FFT_SIZE, 0.f);
        lastPhase.assign(FFT_SIZE / 2 + 1, 0.f);
        sumPhase.assign(FFT_SIZE / 2 + 1, 0.f);
    }

    void process(const ProcessArgs &args) override {
        float octave = params[OCTAVE_PARAM].getValue() + inputs[OCTAVE_CV_INPUT].getVoltage();
        float semitone = params[SEMITONE_PARAM].getValue() + inputs[SEMITONE_CV_INPUT].getVoltage();
        float cent = params[CENT_PARAM].getValue() + inputs[CENT_CV_INPUT].getVoltage();
        float mix = clamp(params[MIX_PARAM].getValue() + inputs[MIX_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);

        float ratio = powf(2.f, octave + semitone / 12.f + cent / 1200.f);

        float in = inputs[AUDIO_INPUT].getVoltage();

        inputBuffer[pos] = in;

        // Hann-squared at 75% overlap sums to 1.5, so normalise by 2/3.
        float wet = outputBuffer[pos] * (2.f / 3.f);
        outputBuffer[pos] = 0.f;

        pos++;
        if (pos >= FFT_SIZE)
            pos = 0;

        if (--hopCount <= 0) {
            hopCount = HOP_SIZE;
            processFrame(ratio);
        }

        outputs[AUDIO_OUTPUT].setVoltage(in * (1.f - mix) + wet * mix);
    }

    void processFrame(float ratio) {
        std::vector<float> frame(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE; i++)
            frame[i] = inputBuffer[(pos + i) % FFT_SIZE] * window[i];

        std::vector<float> fftOut(FFT_SIZE);
        fft.rfft(frame.data(), fftOut.data());

        for (int k = 0; k <= FFT_SIZE / 2; k++) {
            float re, im;
            if (k == 0) {
                re = fftOut[0];
                im = 0.f;
            } else if (k == FFT_SIZE / 2) {
                re = fftOut[1];
                im = 0.f;
            } else {
                re = fftOut[2 * k];
                im = fftOut[2 * k + 1];
            }

            float mag = sqrtf(re * re + im * im);
            float phase = atan2f(im, re);

            float expected = 2.f * M_PI * (float)k * (float)HOP_SIZE / (float)FFT_SIZE;
            float phaseDiff = phase - lastPhase[k] - expected;
            lastPhase[k] = phase;

            while (phaseDiff > M_PI) phaseDiff -= 2.f * M_PI;
            while (phaseDiff < -M_PI) phaseDiff += 2.f * M_PI;

            // Instantaneous frequency scaled by the pitch ratio.
            sumPhase[k] += (expected + phaseDiff) * ratio;

            float outRe = mag * cosf(sumPhase[k]);
            float outIm = mag * sinf(sumPhase[k]);

            if (k == 0) {
                fftOut[0] = outRe;
            } else if (k == FFT_SIZE / 2) {
                fftOut[1] = outRe;
            } else {
                fftOut[2 * k] = outRe;
                fftOut[2 * k + 1] = outIm;
            }
        }

        fft.irfft(fftOut.data(), frame.data());
        fft.scale(frame.data());

        for (int i = 0; i < FFT_SIZE; i++)
            outputBuffer[(pos + i) % FFT_SIZE] += frame[i] * window[i];
    }
};

struct RaRepitchWidget : ModuleWidget {
    RaRepitchWidget(RaRepitchModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-repitch.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float colX[5] = {20.f, 55.f, 90.f, 125.f, 160.f};
        float rowY[2] = {50.f, 130.f};

        addInput(createInputCentered<RaPort>(Vec(colX[0], rowY[0]), module, RaRepitchModule::AUDIO_INPUT));

        addParam(createParamCentered<RaKnobTrim>(Vec(colX[1], rowY[0]), module, RaRepitchModule::OCTAVE_PARAM));
        addInput(createInputCentered<RaPort>(Vec(colX[1], rowY[1]), module, RaRepitchModule::OCTAVE_CV_INPUT));

        addParam(createParamCentered<RaKnobTrim>(Vec(colX[2], rowY[0]), module, RaRepitchModule::SEMITONE_PARAM));
        addInput(createInputCentered<RaPort>(Vec(colX[2], rowY[1]), module, RaRepitchModule::SEMITONE_CV_INPUT));

        addParam(createParamCentered<RaKnobTrim>(Vec(colX[3], rowY[0]), module, RaRepitchModule::CENT_PARAM));
        addInput(createInputCentered<RaPort>(Vec(colX[3], rowY[1]), module, RaRepitchModule::CENT_CV_INPUT));

        addParam(createParamCentered<RaKnobTrim>(Vec(colX[4], rowY[0]), module, RaRepitchModule::MIX_PARAM));
        addInput(createInputCentered<RaPort>(Vec(colX[4], rowY[1]), module, RaRepitchModule::MIX_CV_INPUT));

        addOutput(createOutputCentered<RaPort>(Vec(colX[2], 220.f), module, RaRepitchModule::AUDIO_OUTPUT));
    }
};

Model *modelRaRepitch = createModel<RaRepitchModule, RaRepitchWidget>("ra-repitch");
