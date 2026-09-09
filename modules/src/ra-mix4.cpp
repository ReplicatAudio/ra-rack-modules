// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: GAIN1_PARAM "Gain 1"
// fname: GAIN2_PARAM "Gain 2"
// fname: GAIN3_PARAM "Gain 3"
// fname: GAIN4_PARAM "Gain 4"
// fname: MASTER_PARAM "Master"
// fname: PAN1_PARAM "Pan 1"
// fname: PAN2_PARAM "Pan 2"
// fname: PAN3_PARAM "Pan 3"
// fname: PAN4_PARAM "Pan 4"
// fname: MASTER_PAN_PARAM "Pan"
// fname: CV_GAIN1_INPUT "CV G1"
// fname: CV_GAIN2_INPUT "CV G2"
// fname: CV_GAIN3_INPUT "CV G3"
// fname: CV_GAIN4_INPUT "CV G4"
// fname: CV_GAIN_MASTER_INPUT "CV GM"
// fname: CV_PAN1_INPUT "CV P1"
// fname: CV_PAN2_INPUT "CV P2"
// fname: CV_PAN3_INPUT "CV P3"
// fname: CV_PAN4_INPUT "CV P4"
// fname: CV_PAN_MASTER_INPUT "CV PM"
// fname: CH1_INPUT "In 1"
// fname: CH2_INPUT "In 2"
// fname: CH3_INPUT "In 3"
// fname: CH4_INPUT "In 4"
// fname: OUT_L "Left"
// fname: OUT_R "Right"
#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaMix4Module : Module {
    enum ParamIds {
        GAIN1_PARAM,
        GAIN2_PARAM,
        GAIN3_PARAM,
        GAIN4_PARAM,
        MASTER_PARAM,
        PAN1_PARAM,
        PAN2_PARAM,
        PAN3_PARAM,
        PAN4_PARAM,
        MASTER_PAN_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CH1_INPUT,
        CH2_INPUT,
        CH3_INPUT,
        CH4_INPUT,
        CV_GAIN1_INPUT,
        CV_GAIN2_INPUT,
        CV_GAIN3_INPUT,
        CV_GAIN4_INPUT,
        CV_GAIN_MASTER_INPUT,
        CV_PAN1_INPUT,
        CV_PAN2_INPUT,
        CV_PAN3_INPUT,
        CV_PAN4_INPUT,
        CV_PAN_MASTER_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT_L,
        OUT_R,
        NUM_OUTPUTS
    };
    static constexpr int VU_SEGMENTS = 10;
    enum LightIds {
        VU1_BASE,
        VU2_BASE = VU1_BASE + VU_SEGMENTS * 3,
        VU3_BASE = VU2_BASE + VU_SEGMENTS * 3,
        VU4_BASE = VU3_BASE + VU_SEGMENTS * 3,
        VU_OUT_L_BASE,
        VU_OUT_R_BASE = VU_OUT_L_BASE + VU_SEGMENTS * 3,
        NUM_LIGHTS = VU_OUT_R_BASE + VU_SEGMENTS * 3
    };

    RaMix4Module() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(GAIN1_PARAM, 0.f, 1.5f, 1.f, "Gain 1");
        configParam(GAIN2_PARAM, 0.f, 1.5f, 1.f, "Gain 2");
        configParam(GAIN3_PARAM, 0.f, 1.5f, 1.f, "Gain 3");
        configParam(GAIN4_PARAM, 0.f, 1.5f, 1.f, "Gain 4");
        configParam(MASTER_PARAM, 0.f, 1.f, 1.f, "Master");
        configParam(PAN1_PARAM, -1.f, 1.f, 0.f, "Pan 1");
        configParam(PAN2_PARAM, -1.f, 1.f, 0.f, "Pan 2");
        configParam(PAN3_PARAM, -1.f, 1.f, 0.f, "Pan 3");
        configParam(PAN4_PARAM, -1.f, 1.f, 0.f, "Pan 4");
        configParam(MASTER_PAN_PARAM, -1.f, 1.f, 0.f, "Pan");
        configInput(CH1_INPUT, "In 1");
        configInput(CH2_INPUT, "In 2");
        configInput(CH3_INPUT, "In 3");
        configInput(CH4_INPUT, "In 4");
        configInput(CV_GAIN1_INPUT, "CV G1");
        configInput(CV_GAIN2_INPUT, "CV G2");
        configInput(CV_GAIN3_INPUT, "CV G3");
        configInput(CV_GAIN4_INPUT, "CV G4");
        configInput(CV_GAIN_MASTER_INPUT, "CV GM");
        configInput(CV_PAN1_INPUT, "CV P1");
        configInput(CV_PAN2_INPUT, "CV P2");
        configInput(CV_PAN3_INPUT, "CV P3");
        configInput(CV_PAN4_INPUT, "CV P4");
        configInput(CV_PAN_MASTER_INPUT, "CV PM");
        configOutput(OUT_L, "Left");
        configOutput(OUT_R, "Right");
        for (int c = 0; c < 4; c++) {
            int vuBase = VU1_BASE + c * VU_SEGMENTS * 3;
            for (int i = 0; i < VU_SEGMENTS; i++) {
                configLight(vuBase + i * 3, "In " + std::to_string(c + 1) + " VU LED " + std::to_string(i + 1));
                configLight(vuBase + i * 3 + 1, "In " + std::to_string(c + 1) + " VU LED " + std::to_string(i + 1));
                configLight(vuBase + i * 3 + 2, "In " + std::to_string(c + 1) + " VU LED " + std::to_string(i + 1));
            }
        }
        for (int i = 0; i < VU_SEGMENTS; i++) {
            configLight(VU_OUT_L_BASE + i * 3, "L VU LED " + std::to_string(i + 1));
            configLight(VU_OUT_L_BASE + i * 3 + 1, "L VU LED " + std::to_string(i + 1));
            configLight(VU_OUT_L_BASE + i * 3 + 2, "L VU LED " + std::to_string(i + 1));
            configLight(VU_OUT_R_BASE + i * 3, "R VU LED " + std::to_string(i + 1));
            configLight(VU_OUT_R_BASE + i * 3 + 1, "R VU LED " + std::to_string(i + 1));
            configLight(VU_OUT_R_BASE + i * 3 + 2, "R VU LED " + std::to_string(i + 1));
        }
    }

    void processVu(float level, int base) {
        // level is linear full-scale (0..1, 1 = 10 V). Convert to a VU-style
        // dB scale — 0 dB = full scale, meter dead below -40 dB — so typical
        // ±5 V program material lights most of the bar instead of the bottom
        // tenth. (-40 dB ≈ 0.1 V, -20 dB ≈ 1 V, -12 dB ≈ 2.5 V, -6 dB ≈ 5 V)
        float db = level > 0.f ? 20.f * std::log10(level) : -60.f;
        float norm = clamp((db + 40.f) / 40.f, 0.f, 1.f);
        for (int i = 0; i < VU_SEGMENTS; i++) {
            float brightness = clamp((norm - (float)i / VU_SEGMENTS) * VU_SEGMENTS, 0.f, 1.f);
            float t = (float)i / (VU_SEGMENTS - 1);
            float r = brightness * (0.3f + 0.7f * t);
            float g = brightness * (0.4f * t);
            float b = brightness * (0.3f + 0.5f * t);
            lights[base + i * 3].setBrightness(r);
            lights[base + i * 3 + 1].setBrightness(g);
            lights[base + i * 3 + 2].setBrightness(b);
        }
    }

    void process(const ProcessArgs &args) override {
        float left = 0.f;
        float right = 0.f;
        float mono = 0.f;

        for (int c = 0; c < 4; c++) {
            float in = inputs[CH1_INPUT + c].getVoltage();
            float cvGain = inputs[CV_GAIN1_INPUT + c].getVoltage();
            float gain;
            if (inputs[CV_GAIN1_INPUT + c].isConnected())
                // With CV in, the slider acts as a 0-100% attenuator of the CV gain.
                gain = clamp(params[GAIN1_PARAM + c].getValue() * cvGain / 10.f, 0.f, 1.5f);
            else
                gain = clamp(params[GAIN1_PARAM + c].getValue(), 0.f, 1.5f);
            float pan = clamp(params[PAN1_PARAM + c].getValue() + inputs[CV_PAN1_INPUT + c].getVoltage(), -1.f, 1.f);
            processVu(clamp(fabsf(in * gain) / 10.f, 0.f, 1.f), VU1_BASE + c * VU_SEGMENTS * 3);
            mono += in * gain;
            float a = cosf((pan + 1.f) * M_PI / 4.f);
            float b = sinf((pan + 1.f) * M_PI / 4.f);
            left += in * gain * a;
            right += in * gain * b;
        }

        float masterGain;
        if (inputs[CV_GAIN_MASTER_INPUT].isConnected())
            // With CV in, the slider acts as a 0-100% attenuator of the CV gain.
            masterGain = clamp(params[MASTER_PARAM].getValue() * inputs[CV_GAIN_MASTER_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        else
            masterGain = clamp(params[MASTER_PARAM].getValue(), 0.f, 1.f);
        float masterPan = clamp(params[MASTER_PAN_PARAM].getValue() + inputs[CV_PAN_MASTER_INPUT].getVoltage(), -1.f, 1.f);
        float ma = cosf((masterPan + 1.f) * M_PI / 4.f);
        float mb = sinf((masterPan + 1.f) * M_PI / 4.f);

        float outL, outR;
        if (outputs[OUT_L].isConnected() && !outputs[OUT_R].isConnected()) {
            outL = clamp(mono * masterGain, -10.f, 10.f);
            outR = 0.f;
        } else {
            outL = clamp((left * ma + right * mb) * masterGain, -10.f, 10.f);
            outR = clamp((left * mb + right * ma) * masterGain, -10.f, 10.f);
        }
        outputs[OUT_L].setVoltage(outL);
        outputs[OUT_R].setVoltage(outR);

        processVu(clamp(fabsf(outL) / 10.f, 0.f, 1.f), VU_OUT_L_BASE);
        processVu(clamp(fabsf(outR) / 10.f, 0.f, 1.f), VU_OUT_R_BASE);
    }
};

struct RaMix4Widget : ModuleWidget {
    RaMix4Widget(RaMix4Module *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-mix4.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        // Channel gain sliders
        addParam(createParamCentered<VCVSlider>(Vec(45, 55), module, RaMix4Module::GAIN1_PARAM));
        addParam(createParamCentered<VCVSlider>(Vec(75, 55), module, RaMix4Module::GAIN2_PARAM));
        addParam(createParamCentered<VCVSlider>(Vec(105, 55), module, RaMix4Module::GAIN3_PARAM));
        addParam(createParamCentered<VCVSlider>(Vec(135, 55), module, RaMix4Module::GAIN4_PARAM));
        addParam(createParamCentered<VCVSlider>(Vec(165, 55), module, RaMix4Module::MASTER_PARAM));

        // Channel CV gain inputs (under sliders)
        addInput(createInputCentered<RaPort>(Vec(45, 105), module, RaMix4Module::CV_GAIN1_INPUT));
        addInput(createInputCentered<RaPort>(Vec(75, 105), module, RaMix4Module::CV_GAIN2_INPUT));
        addInput(createInputCentered<RaPort>(Vec(105, 105), module, RaMix4Module::CV_GAIN3_INPUT));
        addInput(createInputCentered<RaPort>(Vec(135, 105), module, RaMix4Module::CV_GAIN4_INPUT));
        addInput(createInputCentered<RaPort>(Vec(165, 105), module, RaMix4Module::CV_GAIN_MASTER_INPUT));

        // Channel pan knobs
        addParam(createParamCentered<RaKnobSmall>(Vec(45, 155), module, RaMix4Module::PAN1_PARAM));
        addParam(createParamCentered<RaKnobSmall>(Vec(75, 155), module, RaMix4Module::PAN2_PARAM));
        addParam(createParamCentered<RaKnobSmall>(Vec(105, 155), module, RaMix4Module::PAN3_PARAM));
        addParam(createParamCentered<RaKnobSmall>(Vec(135, 155), module, RaMix4Module::PAN4_PARAM));
        addParam(createParamCentered<RaKnobSmall>(Vec(165, 155), module, RaMix4Module::MASTER_PAN_PARAM));

        // Channel CV pan inputs (under knobs)
        addInput(createInputCentered<RaPort>(Vec(45, 205), module, RaMix4Module::CV_PAN1_INPUT));
        addInput(createInputCentered<RaPort>(Vec(75, 205), module, RaMix4Module::CV_PAN2_INPUT));
        addInput(createInputCentered<RaPort>(Vec(105, 205), module, RaMix4Module::CV_PAN3_INPUT));
        addInput(createInputCentered<RaPort>(Vec(135, 205), module, RaMix4Module::CV_PAN4_INPUT));
        addInput(createInputCentered<RaPort>(Vec(165, 205), module, RaMix4Module::CV_PAN_MASTER_INPUT));

        // Signal inputs and stereo output, spread out
        addInput(createInputCentered<RaPort>(Vec(45, 255), module, RaMix4Module::CH1_INPUT));
        addInput(createInputCentered<RaPort>(Vec(75, 255), module, RaMix4Module::CH2_INPUT));
        addInput(createInputCentered<RaPort>(Vec(105, 255), module, RaMix4Module::CH3_INPUT));
        addInput(createInputCentered<RaPort>(Vec(135, 255), module, RaMix4Module::CH4_INPUT));

        // Channel input VU meters (below each channel input jack)
        for (int i = 0; i < 10; i++) {
            addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(Vec(45, 371 - i * 10), module, RaMix4Module::VU1_BASE + i * 3));
            addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(Vec(75, 371 - i * 10), module, RaMix4Module::VU2_BASE + i * 3));
            addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(Vec(105, 371 - i * 10), module, RaMix4Module::VU3_BASE + i * 3));
            addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(Vec(135, 371 - i * 10), module, RaMix4Module::VU4_BASE + i * 3));
        }

        // Master output VU meters — L on the left edge, R on the right edge
        for (int i = 0; i < 10; i++) {
            addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(Vec(20, 228 - i * 12), module, RaMix4Module::VU_OUT_L_BASE + i * 3));
            addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(Vec(190, 228 - i * 12), module, RaMix4Module::VU_OUT_R_BASE + i * 3));
        }

        addOutput(createOutputCentered<RaPort>(Vec(165, 255), module, RaMix4Module::OUT_L));
        addOutput(createOutputCentered<RaPort>(Vec(165, 300), module, RaMix4Module::OUT_R));
    }
};

Model *modelRaMix4 = createModel<RaMix4Module, RaMix4Widget>("ra-mix4");