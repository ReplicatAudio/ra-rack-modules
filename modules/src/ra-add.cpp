#include "ra-components.hpp"
#include <array>
#include <atomic>
#include <cstdio>

using namespace rack;

extern Plugin *pluginInstance;

struct RaAddModule : Module {
    enum ParamIds {
        FREQ_PARAM,
        FM_ATTN_PARAM,
        HARM1_PARAM,
        HARM2_PARAM,
        HARM3_PARAM,
        HARM4_PARAM,
        HARM5_PARAM,
        HARM6_PARAM,
        HARM7_PARAM,
        HARM8_PARAM,
        HARM9_PARAM,
        HARM10_PARAM,
        HARM11_PARAM,
        HARM12_PARAM,
        HARM13_PARAM,
        HARM14_PARAM,
        HARM15_PARAM,
        HARM16_PARAM,
        FM1_PARAM,
        FM2_PARAM,
        FM3_PARAM,
        FM4_PARAM,
        FM5_PARAM,
        FM6_PARAM,
        FM7_PARAM,
        FM8_PARAM,
        FM9_PARAM,
        FM10_PARAM,
        FM11_PARAM,
        FM12_PARAM,
        FM13_PARAM,
        FM14_PARAM,
        FM15_PARAM,
        FM16_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        PITCH_INPUT,
        FM_INPUT,
        HARM1_CV_INPUT,
        HARM2_CV_INPUT,
        HARM3_CV_INPUT,
        HARM4_CV_INPUT,
        HARM5_CV_INPUT,
        HARM6_CV_INPUT,
        HARM7_CV_INPUT,
        HARM8_CV_INPUT,
        HARM9_CV_INPUT,
        HARM10_CV_INPUT,
        HARM11_CV_INPUT,
        HARM12_CV_INPUT,
        HARM13_CV_INPUT,
        HARM14_CV_INPUT,
        HARM15_CV_INPUT,
        HARM16_CV_INPUT,
        FM1_CV_INPUT,
        FM2_CV_INPUT,
        FM3_CV_INPUT,
        FM4_CV_INPUT,
        FM5_CV_INPUT,
        FM6_CV_INPUT,
        FM7_CV_INPUT,
        FM8_CV_INPUT,
        FM9_CV_INPUT,
        FM10_CV_INPUT,
        FM11_CV_INPUT,
        FM12_CV_INPUT,
        FM13_CV_INPUT,
        FM14_CV_INPUT,
        FM15_CV_INPUT,
        FM16_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        AUDIO_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    static constexpr int NUM_HARMS = 16;

    std::array<float, NUM_HARMS> phase;
    std::array<std::atomic<float>, NUM_HARMS> displayAmp;

    static constexpr float MIN_FREQ = 2.f;
    static constexpr float MAX_FREQ = 8000.f;

    RaAddModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(FREQ_PARAM, 0.f, 1.f, 0.588f, "Frequency", " Hz");
        configParam(FM_ATTN_PARAM, 0.f, 1.f, 0.f, "FM attenuation", "%", 0.f, 100.f);
        configParam(HARM1_PARAM, 0.f, 1.f, 1.f, "Harmonic 1", "%", 0.f, 100.f);
        configParam(HARM2_PARAM, 0.f, 1.f, 0.f, "Harmonic 2", "%", 0.f, 100.f);
        configParam(HARM3_PARAM, 0.f, 1.f, 0.f, "Harmonic 3", "%", 0.f, 100.f);
        configParam(HARM4_PARAM, 0.f, 1.f, 0.f, "Harmonic 4", "%", 0.f, 100.f);
        configParam(HARM5_PARAM, 0.f, 1.f, 0.f, "Harmonic 5", "%", 0.f, 100.f);
        configParam(HARM6_PARAM, 0.f, 1.f, 0.f, "Harmonic 6", "%", 0.f, 100.f);
        configParam(HARM7_PARAM, 0.f, 1.f, 0.f, "Harmonic 7", "%", 0.f, 100.f);
        configParam(HARM8_PARAM, 0.f, 1.f, 0.f, "Harmonic 8", "%", 0.f, 100.f);
        configParam(HARM9_PARAM, 0.f, 1.f, 0.f, "Harmonic 9", "%", 0.f, 100.f);
        configParam(HARM10_PARAM, 0.f, 1.f, 0.f, "Harmonic 10", "%", 0.f, 100.f);
        configParam(HARM11_PARAM, 0.f, 1.f, 0.f, "Harmonic 11", "%", 0.f, 100.f);
        configParam(HARM12_PARAM, 0.f, 1.f, 0.f, "Harmonic 12", "%", 0.f, 100.f);
        configParam(HARM13_PARAM, 0.f, 1.f, 0.f, "Harmonic 13", "%", 0.f, 100.f);
        configParam(HARM14_PARAM, 0.f, 1.f, 0.f, "Harmonic 14", "%", 0.f, 100.f);
        configParam(HARM15_PARAM, 0.f, 1.f, 0.f, "Harmonic 15", "%", 0.f, 100.f);
        configParam(HARM16_PARAM, 0.f, 1.f, 0.f, "Harmonic 16", "%", 0.f, 100.f);
        configInput(PITCH_INPUT, "1V/Oct");
        configInput(FM_INPUT, "FM");
        configInput(HARM1_CV_INPUT, "Harmonic 1 CV");
        configInput(HARM2_CV_INPUT, "Harmonic 2 CV");
        configInput(HARM3_CV_INPUT, "Harmonic 3 CV");
        configInput(HARM4_CV_INPUT, "Harmonic 4 CV");
        configInput(HARM5_CV_INPUT, "Harmonic 5 CV");
        configInput(HARM6_CV_INPUT, "Harmonic 6 CV");
        configInput(HARM7_CV_INPUT, "Harmonic 7 CV");
        configInput(HARM8_CV_INPUT, "Harmonic 8 CV");
        configInput(HARM9_CV_INPUT, "Harmonic 9 CV");
        configInput(HARM10_CV_INPUT, "Harmonic 10 CV");
        configInput(HARM11_CV_INPUT, "Harmonic 11 CV");
        configInput(HARM12_CV_INPUT, "Harmonic 12 CV");
        configInput(HARM13_CV_INPUT, "Harmonic 13 CV");
        configInput(HARM14_CV_INPUT, "Harmonic 14 CV");
        configInput(HARM15_CV_INPUT, "Harmonic 15 CV");
        configInput(HARM16_CV_INPUT, "Harmonic 16 CV");
        configParam(FM1_PARAM, 0.f, 1.f, 0.f, "FM 1", "%", 0.f, 100.f);
        configParam(FM2_PARAM, 0.f, 1.f, 0.f, "FM 2", "%", 0.f, 100.f);
        configParam(FM3_PARAM, 0.f, 1.f, 0.f, "FM 3", "%", 0.f, 100.f);
        configParam(FM4_PARAM, 0.f, 1.f, 0.f, "FM 4", "%", 0.f, 100.f);
        configParam(FM5_PARAM, 0.f, 1.f, 0.f, "FM 5", "%", 0.f, 100.f);
        configParam(FM6_PARAM, 0.f, 1.f, 0.f, "FM 6", "%", 0.f, 100.f);
        configParam(FM7_PARAM, 0.f, 1.f, 0.f, "FM 7", "%", 0.f, 100.f);
        configParam(FM8_PARAM, 0.f, 1.f, 0.f, "FM 8", "%", 0.f, 100.f);
        configParam(FM9_PARAM, 0.f, 1.f, 0.f, "FM 9", "%", 0.f, 100.f);
        configParam(FM10_PARAM, 0.f, 1.f, 0.f, "FM 10", "%", 0.f, 100.f);
        configParam(FM11_PARAM, 0.f, 1.f, 0.f, "FM 11", "%", 0.f, 100.f);
        configParam(FM12_PARAM, 0.f, 1.f, 0.f, "FM 12", "%", 0.f, 100.f);
        configParam(FM13_PARAM, 0.f, 1.f, 0.f, "FM 13", "%", 0.f, 100.f);
        configParam(FM14_PARAM, 0.f, 1.f, 0.f, "FM 14", "%", 0.f, 100.f);
        configParam(FM15_PARAM, 0.f, 1.f, 0.f, "FM 15", "%", 0.f, 100.f);
        configParam(FM16_PARAM, 0.f, 1.f, 0.f, "FM 16", "%", 0.f, 100.f);
        configInput(FM1_CV_INPUT, "FM 1 CV");
        configInput(FM2_CV_INPUT, "FM 2 CV");
        configInput(FM3_CV_INPUT, "FM 3 CV");
        configInput(FM4_CV_INPUT, "FM 4 CV");
        configInput(FM5_CV_INPUT, "FM 5 CV");
        configInput(FM6_CV_INPUT, "FM 6 CV");
        configInput(FM7_CV_INPUT, "FM 7 CV");
        configInput(FM8_CV_INPUT, "FM 8 CV");
        configInput(FM9_CV_INPUT, "FM 9 CV");
        configInput(FM10_CV_INPUT, "FM 10 CV");
        configInput(FM11_CV_INPUT, "FM 11 CV");
        configInput(FM12_CV_INPUT, "FM 12 CV");
        configInput(FM13_CV_INPUT, "FM 13 CV");
        configInput(FM14_CV_INPUT, "FM 14 CV");
        configInput(FM15_CV_INPUT, "FM 15 CV");
        configInput(FM16_CV_INPUT, "FM 16 CV");
        configOutput(AUDIO_OUTPUT, "Audio");

        phase.fill(0.f);
        for (auto &a : displayAmp)
            a.store(0.f, std::memory_order_relaxed);
    }

    void process(const ProcessArgs &args) override {
        float freq = MIN_FREQ * powf(MAX_FREQ / MIN_FREQ, params[FREQ_PARAM].getValue());
        float pitch = inputs[PITCH_INPUT].getVoltage()
            + inputs[FM_INPUT].getVoltage() * params[FM_ATTN_PARAM].getValue();
        freq *= powf(2.f, pitch);
        freq = clamp(freq, 0.1f, 20000.f);

        float output = 0.f;
        float totalAmp = 0.f;

        for (int n = 1; n <= NUM_HARMS; n++) {
            float knob = params[HARM1_PARAM + n - 1].getValue();
            float amp = knob;
            if (inputs[HARM1_CV_INPUT + n - 1].isConnected())
                amp = knob * clamp(inputs[HARM1_CV_INPUT + n - 1].getVoltage() / 10.f, 0.f, 1.f);

            totalAmp += amp;

            // Per-harmonic FM — the knob is an attenuator for the FM CV.
            float fm = inputs[FM1_CV_INPUT + n - 1].getVoltage() * params[FM1_PARAM + n - 1].getValue();
            float harmFreq = n * freq * powf(2.f, fm);

            phase[n - 1] += harmFreq * args.sampleTime;
            phase[n - 1] -= floorf(phase[n - 1]);

            output += amp * sinf(2.f * M_PI * phase[n - 1]);
            displayAmp[n - 1].store(amp, std::memory_order_relaxed);
        }

        if (totalAmp > 0.f)
            output /= totalAmp;

        outputs[AUDIO_OUTPUT].setVoltage(output * 5.f);
    }
};

struct HarmBarDisplay : Widget {
    RaAddModule *module;

    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2);
        nvgFillColor(args.vg, nvgRGB(0x0a, 0x0a, 0x0a));
        nvgFill(args.vg);

        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2);
        nvgStrokeWidth(args.vg, 1.f);
        nvgStrokeColor(args.vg, nvgRGB(0x33, 0x33, 0x33));
        nvgStroke(args.vg);

        if (!module) return;

        const int n = RaAddModule::NUM_HARMS;
        const float padTop = 4.f;
        const float padBottom = 4.f;
        const float barX = 2.f;
        const float barW = box.size.x - barX - 2.f;
        const float cellH = (box.size.y - padTop - padBottom) / n;
        const float barH = cellH * 0.78f;

        for (int i = 0; i < n; i++) {
            float slotY = padTop + i * cellH;
            float midY = slotY + cellH / 2.f;

            // Slot
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, barX, midY - barH / 2.f, barW, barH, 1);
            nvgFillColor(args.vg, nvgRGB(0x16, 0x16, 0x16));
            nvgFill(args.vg);

            // Bar — level of the harmonic (knob value, or knob × CV when a CV is patched).
            // Grows outward from the horizontal center of the display.
            float v = clamp(module->displayAmp[i].load(std::memory_order_relaxed), 0.f, 1.f);
            if (v > 0.005f) {
                nvgBeginPath(args.vg);
                nvgRoundedRect(args.vg, barX + barW * (1.f - v) / 2.f, midY - barH / 2.f, barW * v, barH, 1);
                nvgFillColor(args.vg, nvgRGBA(0x99, 0x6d, 0xd2, 0xC8));
                nvgFill(args.vg);
            }
        }
    }
};

struct RaAddWidget : ModuleWidget {
    RaAddWidget(RaAddModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-add.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        // Voice controls across the top: left = freq knob + freq CV, right = FM knob + FM CV
        addParam(createParamCentered<RaKnobLarge>(Vec(30, 55), module, RaAddModule::FREQ_PARAM));
        addInput(createInputCentered<RaPort>(Vec(70, 55), module, RaAddModule::PITCH_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(210, 55), module, RaAddModule::FM_ATTN_PARAM));
        addInput(createInputCentered<RaPort>(Vec(240, 55), module, RaAddModule::FM_INPUT));

        // Harmonic rows — harmonics 1-8 in the left block, 9-16 in the right block.
        // Each harmonic has a level knob with a CV input below it, followed by an
        // FM knob (attenuator) with an FM CV input below it.
        for (int row = 0; row < 4; row++) {
            int y = 100 + row * 64;
            int leftIdx = row * 2;
            int rightIdx = 8 + row * 2;
            addParam(createParamCentered<RaKnobSmall>(Vec(30, y), module, RaAddModule::HARM1_PARAM + leftIdx));
            addInput(createInputCentered<RaPort>(Vec(30, y + 28), module, RaAddModule::HARM1_CV_INPUT + leftIdx));
            addParam(createParamCentered<RaKnobSmall>(Vec(60, y), module, RaAddModule::FM1_PARAM + leftIdx));
            addInput(createInputCentered<RaPort>(Vec(60, y + 28), module, RaAddModule::FM1_CV_INPUT + leftIdx));
            addParam(createParamCentered<RaKnobSmall>(Vec(90, y), module, RaAddModule::HARM1_PARAM + leftIdx + 1));
            addInput(createInputCentered<RaPort>(Vec(90, y + 28), module, RaAddModule::HARM1_CV_INPUT + leftIdx + 1));
            addParam(createParamCentered<RaKnobSmall>(Vec(120, y), module, RaAddModule::FM1_PARAM + leftIdx + 1));
            addInput(createInputCentered<RaPort>(Vec(120, y + 28), module, RaAddModule::FM1_CV_INPUT + leftIdx + 1));
            addParam(createParamCentered<RaKnobSmall>(Vec(210, y), module, RaAddModule::HARM1_PARAM + rightIdx));
            addInput(createInputCentered<RaPort>(Vec(210, y + 28), module, RaAddModule::HARM1_CV_INPUT + rightIdx));
            addParam(createParamCentered<RaKnobSmall>(Vec(240, y), module, RaAddModule::FM1_PARAM + rightIdx));
            addInput(createInputCentered<RaPort>(Vec(240, y + 28), module, RaAddModule::FM1_CV_INPUT + rightIdx));
            addParam(createParamCentered<RaKnobSmall>(Vec(270, y), module, RaAddModule::HARM1_PARAM + rightIdx + 1));
            addInput(createInputCentered<RaPort>(Vec(270, y + 28), module, RaAddModule::HARM1_CV_INPUT + rightIdx + 1));
            addParam(createParamCentered<RaKnobSmall>(Vec(300, y), module, RaAddModule::FM1_PARAM + rightIdx + 1));
            addInput(createInputCentered<RaPort>(Vec(300, y + 28), module, RaAddModule::FM1_CV_INPUT + rightIdx + 1));
        }

        // Bar graph display — all 16 harmonic levels, 1 at the top, 16 at the bottom
        HarmBarDisplay* display = createWidget<HarmBarDisplay>(Vec(149, 20));
        display->box.size = Vec(32, 320);
        display->module = module;
        addChild(display);

        addOutput(createOutputCentered<RaPort>(Vec(165, 356), module, RaAddModule::AUDIO_OUTPUT));
    }
};

Model *modelRaAdd = createModel<RaAddModule, RaAddWidget>("ra-add");