#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaVcaModule : Module {
    enum ParamIds {
        GAIN1_PARAM,
        MODE_PARAM,
        SOFTCLIP_PARAM,
        SUM_PARAM,
        GAIN2_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        AUDIO1_INPUT,
        CV1_INPUT,
        AUDIO2_INPUT,
        CV2_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        AUDIO1_OUTPUT,
        AUDIO2_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        VU1_BASE,
        VU1_END = VU1_BASE + 8 * 3,
        VU2_BASE = VU1_END,
        NUM_LIGHTS = VU2_BASE + 8 * 3
    };

    RaVcaModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(GAIN1_PARAM, 0.f, 1.f, 1.f, "Gain 1", "%", 0.f, 100.f, 0.f);
        configSwitch(MODE_PARAM, 0.f, 1.f, 0.f, "Mode", {"Linear", "Exp"});
        configSwitch(SOFTCLIP_PARAM, 0.f, 1.f, 0.f, "Soft Clip", {"Off", "On"});
        configSwitch(SUM_PARAM, 0.f, 1.f, 0.f, "Sum", {"Indiv", "Sum"});
        configParam(GAIN2_PARAM, 0.f, 1.f, 1.f, "Gain 2", "%", 0.f, 100.f, 0.f);
        configInput(AUDIO1_INPUT, "Audio 1");
        configInput(CV1_INPUT, "CV 1");
        configInput(AUDIO2_INPUT, "Audio 2");
        configInput(CV2_INPUT, "CV 2");
        configOutput(AUDIO1_OUTPUT, "Audio 1");
        configOutput(AUDIO2_OUTPUT, "Audio 2");
        for (int i = 0; i < 16; i++)
            for (int c = 0; c < 3; c++)
                configLight(VU1_BASE + i * 3 + c, "VU LED " + std::to_string(i + 1));
    }

    void processVu(float level, int base) {
        for (int i = 0; i < 8; i++) {
            float brightness = clamp((level - (float)i / 8.f) * 8.f, 0.f, 1.f);

            float t = (float)i / 7.f;
            float r = brightness * (0.3f + 0.7f * t);
            float g = brightness * (0.4f * t);
            float b = brightness * (0.3f + 0.5f * t);
            lights[base + i * 3].setBrightness(r);
            lights[base + i * 3 + 1].setBrightness(g);
            lights[base + i * 3 + 2].setBrightness(b);
        }
    }

    void process(const ProcessArgs &args) override {
        float gain1, gain2;

        if (params[SUM_PARAM].getValue() > 0.f) {
            float summedGain = params[GAIN1_PARAM].getValue() + params[GAIN2_PARAM].getValue();
            bool anyCv = inputs[CV1_INPUT].isConnected() || inputs[CV2_INPUT].isConnected();
            if (anyCv) {
                float summed = inputs[CV1_INPUT].getVoltage() + inputs[CV2_INPUT].getVoltage();
                summed = clamp(summed, 0.f, 10.f);
                gain1 = summedGain * summed / 10.f;
                gain2 = summedGain * summed / 10.f;
            } else {
                gain1 = summedGain;
                gain2 = summedGain;
            }
        } else {
            if (inputs[CV1_INPUT].isConnected())
                gain1 = params[GAIN1_PARAM].getValue() * inputs[CV1_INPUT].getVoltage() / 10.f;
            else
                gain1 = params[GAIN1_PARAM].getValue();

            if (inputs[CV2_INPUT].isConnected())
                gain2 = params[GAIN2_PARAM].getValue() * inputs[CV2_INPUT].getVoltage() / 10.f;
            else
                gain2 = params[GAIN2_PARAM].getValue();
        }

        gain1 = clamp(gain1, 0.f, 1.f);
        gain2 = clamp(gain2, 0.f, 1.f);

        if (params[MODE_PARAM].getValue() > 0.f) {
            gain1 = gain1 * gain1;
            gain2 = gain2 * gain2;
        }

        float out1 = inputs[AUDIO1_INPUT].getVoltage() * gain1;
        float out2 = inputs[AUDIO2_INPUT].getVoltage() * gain2;

        if (params[SOFTCLIP_PARAM].getValue() > 0.f) {
            out1 = 10.f * tanhf(out1 / 10.f);
            out2 = 10.f * tanhf(out2 / 10.f);
        }

        outputs[AUDIO1_OUTPUT].setVoltage(out1);
        outputs[AUDIO2_OUTPUT].setVoltage(out2);

        processVu(gain1, VU1_BASE);
        processVu(gain2, VU2_BASE);
    }
};

struct RaVcaWidget : ModuleWidget {
    RaVcaWidget(RaVcaModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-vca.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float cx = 19;
        addParam(createParamCentered<RaKnob>(Vec(cx, 56), module, RaVcaModule::GAIN1_PARAM));
        addInput(createInputCentered<RaPort>(Vec(cx, 96), module, RaVcaModule::CV1_INPUT));
        addInput(createInputCentered<RaPort>(Vec(cx, 126), module, RaVcaModule::AUDIO1_INPUT));
        addOutput(createOutputCentered<RaPort>(Vec(cx, 156), module, RaVcaModule::AUDIO1_OUTPUT));

        addParam(createParamCentered<RaSwitch2>(Vec(15, 191), module, RaVcaModule::MODE_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(30, 191), module, RaVcaModule::SOFTCLIP_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(45, 191), module, RaVcaModule::SUM_PARAM));

        addParam(createParamCentered<RaKnob>(Vec(cx, 226), module, RaVcaModule::GAIN2_PARAM));
        addInput(createInputCentered<RaPort>(Vec(cx, 266), module, RaVcaModule::CV2_INPUT));
        addInput(createInputCentered<RaPort>(Vec(cx, 296), module, RaVcaModule::AUDIO2_INPUT));
        addOutput(createOutputCentered<RaPort>(Vec(cx, 326), module, RaVcaModule::AUDIO2_OUTPUT));

        for (int i = 0; i < 8; i++) {
            addChild(createLightCentered<TinyLight<RedGreenBlueLight>>(Vec(50, 56 + i * 15), module, RaVcaModule::VU1_BASE + i * 3));
            addChild(createLightCentered<TinyLight<RedGreenBlueLight>>(Vec(50, 226 + i * 15), module, RaVcaModule::VU2_BASE + i * 3));
        }
    }
};

Model *modelRaVca = createModel<RaVcaModule, RaVcaWidget>("ra-vca");
