#include "ra-components.hpp"

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
    static constexpr int VU_SEGMENTS = 10;

    enum LightIds {
        MUTE_LIGHT,
        VU_L_BASE,
        VU_R_BASE = VU_L_BASE + VU_SEGMENTS * 3,
        NUM_LIGHTS = VU_R_BASE + VU_SEGMENTS * 3
    };

    bool muteState = false;
    bool prevMuteButton = false;
    float pulsePhase = 0.f;

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
        for (int i = 0; i < VU_SEGMENTS; i++) {
            configLight(VU_L_BASE + i * 3, "L VU LED " + std::to_string(i + 1));
            configLight(VU_L_BASE + i * 3 + 1, "L VU LED " + std::to_string(i + 1));
            configLight(VU_L_BASE + i * 3 + 2, "L VU LED " + std::to_string(i + 1));
            configLight(VU_R_BASE + i * 3, "R VU LED " + std::to_string(i + 1));
            configLight(VU_R_BASE + i * 3 + 1, "R VU LED " + std::to_string(i + 1));
            configLight(VU_R_BASE + i * 3 + 2, "R VU LED " + std::to_string(i + 1));
        }
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
        bool buttonPressed = params[MUTE_PARAM].getValue() > 0.5f;
        if (buttonPressed && !prevMuteButton)
            muteState = !muteState;
        prevMuteButton = buttonPressed;

        pulsePhase += args.sampleTime;
        float pulse = muteState ? 0.5f * (1.f + sinf(2.f * M_PI * 2.f * pulsePhase)) : 0.f;
        lights[MUTE_LIGHT].setBrightnessSmooth(pulse, args.sampleTime);

        if (muteState) {
            outputs[L_OUTPUT].setVoltage(0.f);
            outputs[R_OUTPUT].setVoltage(0.f);
            processVu(0.f, VU_L_BASE);
            processVu(0.f, VU_R_BASE);
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

        processVu(clamp(fabsf(l) / 10.f, 0.f, 1.f), VU_L_BASE);
        processVu(clamp(fabsf(r) / 10.f, 0.f, 1.f), VU_R_BASE);
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

        addInput(createInputCentered<RaPort>(Vec(lx, 85), module, RaM2Module::L_INPUT));
        addInput(createInputCentered<RaPort>(Vec(rx, 85), module, RaM2Module::R_INPUT));

        addParam(createParamCentered<RaKnob>(Vec(cx, 125), module, RaM2Module::GAIN_PARAM));
        addParam(createParamCentered<RaKnob>(Vec(cx, 170), module, RaM2Module::STEREO_MONO_PARAM));

        addParam(createParamCentered<RaSwitch2>(Vec(20, 250), module, RaM2Module::LIMITER_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(70, 250), module, RaM2Module::SOFTCLIP_PARAM));

        addParam(createLightParamCentered<VCVLightBezel<RedLight>>(Vec(cx, 250), module, RaM2Module::MUTE_PARAM, RaM2Module::MUTE_LIGHT));

        // VU meters — 5 LEDs each, above the limiter/softclip switches
        for (int i = 0; i < 10; i++) {
            addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(Vec(20, 228 - i * 12), module, RaM2Module::VU_L_BASE + i * 3));
            addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(Vec(70, 228 - i * 12), module, RaM2Module::VU_R_BASE + i * 3));
        }

        addOutput(createOutputCentered<RaPort>(Vec(lx, 295), module, RaM2Module::L_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(rx, 295), module, RaM2Module::R_OUTPUT));
    }

    void onSelectKey(const SelectKeyEvent &e) override {
        if (e.action == GLFW_PRESS && e.key == GLFW_KEY_SPACE) {
            if (RaM2Module *m = dynamic_cast<RaM2Module *>(module)) {
                m->muteState = !m->muteState;
            }
            e.consume(this);
            return;
        }
        ModuleWidget::onSelectKey(e);
    }
};

Model *modelRaM2 = createModel<RaM2Module, RaM2Widget>("ra-m2");
