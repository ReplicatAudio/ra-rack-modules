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

    float phase = 0.f;
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
        configOutput(AUDIO_OUTPUT, "Audio");

        for (auto &a : displayAmp)
            a.store(0.f, std::memory_order_relaxed);
    }

    void process(const ProcessArgs &args) override {
        float freq = MIN_FREQ * powf(MAX_FREQ / MIN_FREQ, params[FREQ_PARAM].getValue());
        float pitch = inputs[PITCH_INPUT].getVoltage()
            + inputs[FM_INPUT].getVoltage() * params[FM_ATTN_PARAM].getValue();
        freq *= powf(2.f, pitch);
        freq = clamp(freq, 0.1f, 20000.f);

        phase += freq * args.sampleTime;
        if (phase >= 1.f)
            phase -= 1.f;

        float output = 0.f;
        float totalAmp = 0.f;

        float theta = 2.f * M_PI * phase;
        float cosTheta = cosf(theta);
        float s0 = 0.f;
        float s1 = sinf(theta);

        for (int n = 1; n <= NUM_HARMS; n++) {
            float sn;
            if (n == 1) {
                sn = s1;
            } else {
                sn = 2.f * cosTheta * s1 - s0;
                s0 = s1;
                s1 = sn;
            }

            float knob = params[HARM1_PARAM + n - 1].getValue();
            float amp = knob;
            if (inputs[HARM1_CV_INPUT + n - 1].isConnected())
                amp = knob * clamp(inputs[HARM1_CV_INPUT + n - 1].getVoltage() / 10.f, 0.f, 1.f);

            totalAmp += amp;
            output += amp * sn;
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
        const float numW = 11.f;
        const float barX = numW + 1.f;
        const float barW = box.size.x - barX - 2.f;
        const float cellH = (box.size.y - padTop - padBottom) / n;
        const float barH = cellH * 0.78f;

        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFontSize(args.vg, 8);

        for (int i = 0; i < n; i++) {
            float slotY = padTop + i * cellH;
            float midY = slotY + cellH / 2.f;

            // Channel number
            char num[4];
            snprintf(num, sizeof(num), "%d", i + 1);
            nvgFillColor(args.vg, nvgRGB(0x66, 0x66, 0x66));
            nvgText(args.vg, 2.f, midY, num, NULL);

            // Slot
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, barX, midY - barH / 2.f, barW, barH, 1);
            nvgFillColor(args.vg, nvgRGB(0x16, 0x16, 0x16));
            nvgFill(args.vg);

            // Bar — level of the harmonic (knob value, or knob × CV when a CV is patched)
            float v = clamp(module->displayAmp[i].load(std::memory_order_relaxed), 0.f, 1.f);
            if (v > 0.005f) {
                nvgBeginPath(args.vg);
                nvgRoundedRect(args.vg, barX, midY - barH / 2.f, barW * v, barH, 1);
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
        addParam(createParamCentered<RaKnobLarge>(Vec(44, 55), module, RaAddModule::FREQ_PARAM));
        addInput(createInputCentered<RaPort>(Vec(76, 55), module, RaAddModule::PITCH_INPUT));
        addParam(createParamCentered<RaKnobSmall>(Vec(164, 55), module, RaAddModule::FM_ATTN_PARAM));
        addInput(createInputCentered<RaPort>(Vec(196, 55), module, RaAddModule::FM_INPUT));

        // Harmonic rows — harmonics 1-8 in the left block, 9-16 in the right block.
        // Each harmonic has a level knob with a CV input below it; the knob acts as
        // an attenuator when CV is patched.
        for (int row = 0; row < 4; row++) {
            int y = 100 + row * 64;
            int leftIdx = row * 2;
            int rightIdx = 8 + row * 2;
            addParam(createParamCentered<RaKnobSmall>(Vec(44, y), module, RaAddModule::HARM1_PARAM + leftIdx));
            addInput(createInputCentered<RaPort>(Vec(44, y + 28), module, RaAddModule::HARM1_CV_INPUT + leftIdx));
            addParam(createParamCentered<RaKnobSmall>(Vec(76, y), module, RaAddModule::HARM1_PARAM + leftIdx + 1));
            addInput(createInputCentered<RaPort>(Vec(76, y + 28), module, RaAddModule::HARM1_CV_INPUT + leftIdx + 1));
            addParam(createParamCentered<RaKnobSmall>(Vec(164, y), module, RaAddModule::HARM1_PARAM + rightIdx));
            addInput(createInputCentered<RaPort>(Vec(164, y + 28), module, RaAddModule::HARM1_CV_INPUT + rightIdx));
            addParam(createParamCentered<RaKnobSmall>(Vec(196, y), module, RaAddModule::HARM1_PARAM + rightIdx + 1));
            addInput(createInputCentered<RaPort>(Vec(196, y + 28), module, RaAddModule::HARM1_CV_INPUT + rightIdx + 1));
        }

        // Bar graph display — all 16 harmonic levels, 1 at the top, 16 at the bottom
        HarmBarDisplay* display = createWidget<HarmBarDisplay>(Vec(104, 20));
        display->box.size = Vec(32, 320);
        display->module = module;
        addChild(display);

        addOutput(createOutputCentered<RaPort>(Vec(120, 356), module, RaAddModule::AUDIO_OUTPUT));
    }
};

Model *modelRaAdd = createModel<RaAddModule, RaAddWidget>("ra-add");