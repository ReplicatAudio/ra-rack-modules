#include "ra-widgets.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaM2Module : Module {
    enum ParamIds {
        GAIN_PARAM,
        LIMITER_PARAM,
        SOFTCLIP_PARAM,
        MUTE_PARAM,
        STEREO_MONO_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        L_INPUT,
        R_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        L_OUTPUT,
        R_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        MUTE_LIGHT,
        NUM_LIGHTS
    };

    bool muteState = false;
    bool prevMuteButton = false;

    RaM2Module() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(GAIN_PARAM, 0.f, 2.f, 1.f, "Master gain", "%", 0.f, 100.f);
        configSwitch(LIMITER_PARAM, 0.f, 1.f, 0.f, "Limiter (-6dB)", {"Off", "On"});
        configSwitch(SOFTCLIP_PARAM, 0.f, 1.f, 0.f, "Soft clip", {"Off", "On"});
        configParam(STEREO_MONO_PARAM, 0.f, 1.f, 0.f, "Stereo\u2192mono", "%", 0.f, 100.f);
        configParam(MUTE_PARAM, 0.f, 1.f, 0.f, "Mute");
        configInput(L_INPUT, "Left");
        configInput(R_INPUT, "Right");
        configOutput(L_OUTPUT, "Left");
        configOutput(R_OUTPUT, "Right");
        configLight(MUTE_LIGHT, "Mute");
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        json_object_set_new(rootJ, "mute", json_boolean(muteState));
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *muteJ = json_object_get(rootJ, "mute");
        if (muteJ) muteState = json_boolean_value(muteJ);
    }

    float softClip(float x) {
        float threshold = 5.f;
        float absX = fabs(x);
        if (absX <= threshold) {
            float n = x / threshold;
            return threshold * (1.5f * n - 0.5f * n * n * n);
        }
        return x > 0.f ? threshold : -threshold;
    }

    void process(const ProcessArgs &args) override {
        bool buttonPressed = params[MUTE_PARAM].getValue() > 0.5f;
        if (buttonPressed && !prevMuteButton)
            muteState = !muteState;
        prevMuteButton = buttonPressed;

        lights[MUTE_LIGHT].setBrightnessSmooth(muteState ? 1.f : 0.f, args.sampleTime);

        if (muteState) {
            outputs[L_OUTPUT].setVoltage(0.f);
            outputs[R_OUTPUT].setVoltage(0.f);
            return;
        }

        float lIn = inputs[L_INPUT].getVoltage();
        float rIn = inputs[R_INPUT].getVoltage();

        bool limiterOn = params[LIMITER_PARAM].getValue() > 0.5f;
        bool softClipOn = params[SOFTCLIP_PARAM].getValue() > 0.5f;
        float gain = params[GAIN_PARAM].getValue();
        float stereoMono = params[STEREO_MONO_PARAM].getValue();

        float mono = (lIn + rIn) / 2.f;
        float l = crossfade(lIn, mono, stereoMono);
        float r = crossfade(rIn, mono, stereoMono);

        l *= gain;
        r *= gain;

        if (limiterOn) {
            l = clamp(l, -5.f, 5.f);
            r = clamp(r, -5.f, 5.f);
        }

        if (softClipOn) {
            l = softClip(l);
            r = softClip(r);
        }

        outputs[L_OUTPUT].setVoltage(l);
        outputs[R_OUTPUT].setVoltage(r);
    }
};

struct RaM2Widget : ModuleWidget {
    RaM2Widget(RaM2Module *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-m2.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float cx = box.size.x / 2;
        float lx = 25;
        float rx = 65;

        addInput(createInputCentered<RaPort>(Vec(lx, 30), module, RaM2Module::L_INPUT));
        addInput(createInputCentered<RaPort>(Vec(rx, 30), module, RaM2Module::R_INPUT));

        addParam(createParamCentered<RaKnob>(Vec(cx, 85), module, RaM2Module::GAIN_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(cx, 145), module, RaM2Module::STEREO_MONO_PARAM));

        addParam(createParamCentered<RaSwitch2>(Vec(35, 200), module, RaM2Module::LIMITER_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(55, 200), module, RaM2Module::SOFTCLIP_PARAM));

        addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(cx, 255), module, RaM2Module::MUTE_PARAM, RaM2Module::MUTE_LIGHT));

        addOutput(createOutputCentered<RaPort>(Vec(lx, 320), module, RaM2Module::L_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(rx, 320), module, RaM2Module::R_OUTPUT));
    }
};

Model *modelRaM2 = createModel<RaM2Module, RaM2Widget>("ra-m2");
