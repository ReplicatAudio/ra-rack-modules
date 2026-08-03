#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaAudio2Module : Module {
    enum ParamIds {
        LEVEL_PARAM,
        MUTE_PARAM,
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
        L_RED_LIGHT,
        L_YELLOW_LIGHT,
        L_GREEN4_LIGHT,
        L_GREEN3_LIGHT,
        L_GREEN2_LIGHT,
        L_GREEN1_LIGHT,
        R_RED_LIGHT,
        R_YELLOW_LIGHT,
        R_GREEN4_LIGHT,
        R_GREEN3_LIGHT,
        R_GREEN2_LIGHT,
        R_GREEN1_LIGHT,
        MUTE_LIGHT,
        NUM_LIGHTS
    };

    bool muted = false;
    bool prevMutePress = false;
    float sampleRate = 44100.f;
    float lastL = 0.f;
    float lastR = 0.f;

    dsp::VuMeter2 vuMeterL;
    dsp::VuMeter2 vuMeterR;

    RaAudio2Module() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(LEVEL_PARAM, 0.f, 1.f, 1.f, "Level", "%", 0.f, 100.f);
        configParam(MUTE_PARAM, 0.f, 1.f, 0.f, "Mute");
        configInput(L_INPUT, "Left");
        configInput(R_INPUT, "Right");
        configOutput(L_OUTPUT, "Left");
        configOutput(R_OUTPUT, "Right");

        const char *vuLabels[] = {"Red", "Yellow", "Green 4", "Green 3", "Green 2", "Green 1"};
        for (int ch = 0; ch < 2; ch++) {
            for (int i = 0; i < 6; i++) {
                configLight(L_RED_LIGHT + ch * 6 + i, string::f("VU %s %s", ch == 0 ? "L" : "R", vuLabels[i]));
            }
        }
        configLight(MUTE_LIGHT, "Mute light");

        vuMeterL.lambda = 30.f;
        vuMeterR.lambda = 30.f;
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        json_object_set_new(rootJ, "muted", json_boolean(muted));
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *mutedJ = json_object_get(rootJ, "muted");
        if (mutedJ)
            muted = json_boolean_value(mutedJ);
    }

    void process(const ProcessArgs &args) override {
        sampleRate = args.sampleRate;

        bool mutePressed = params[MUTE_PARAM].getValue() > 0.5f;
        if (mutePressed && !prevMutePress)
            muted = !muted;
        prevMutePress = mutePressed;

        float level = params[LEVEL_PARAM].getValue();
        float l = inputs[L_INPUT].getVoltage();
        float r = inputs[R_INPUT].getVoltage();

        lastL = l;
        lastR = r;

        vuMeterL.process(args.sampleTime, l);
        vuMeterR.process(args.sampleTime, r);

        float thresholds[6] = {-24.f, -18.f, -12.f, -6.f, -3.f, 0.f};
        for (int i = 0; i < 6; i++) {
            float prev = (i == 0) ? -INFINITY : thresholds[i - 1];
            lights[L_GREEN1_LIGHT + i].setBrightness(vuMeterL.getBrightness(prev, thresholds[i]));
            lights[R_GREEN1_LIGHT + i].setBrightness(vuMeterR.getBrightness(prev, thresholds[i]));
        }

        if (muted) {
            outputs[L_OUTPUT].setVoltage(0.f);
            outputs[R_OUTPUT].setVoltage(0.f);
        } else {
            outputs[L_OUTPUT].setVoltage(l * level);
            outputs[R_OUTPUT].setVoltage(r * level);
        }

        lights[MUTE_LIGHT].setBrightness(muted ? 1.f : 0.f);
    }
};


struct InfoText : LedDisplayChoice {
    void onButton(const ButtonEvent& e) override {}
};


struct RaAudio2Widget : ModuleWidget {
    InfoText* srChoice;
    InfoText* statusChoice;

    RaAudio2Widget(RaAudio2Module *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-audio2.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float screenTop = 38.5f;

        // Screen background
        auto* screen = new LedDisplay;
        screen->box.pos = Vec(0, screenTop);
        screen->box.size = Vec(75, 140.9f);
        addChild(screen);

        // Title
        auto* title = new InfoText;
        title->box.pos = Vec(0, screenTop);
        title->box.size = Vec(75, 14.f);
        title->text = "RA AUDIO 2";
        title->color = nvgRGBA(0xff, 0xd0, 0x00, 0xff);
        addChild(title);

        // Separator
        auto* sep = new LedDisplaySeparator;
        sep->box.pos = Vec(5, screenTop + 14.f);
        sep->box.size = Vec(65, 1.f);
        addChild(sep);

        // Sample rate
        srChoice = new InfoText;
        srChoice->box.pos = Vec(0, screenTop + 16.f);
        srChoice->box.size = Vec(75, 14.f);
        srChoice->text = "SR: -- Hz";
        srChoice->color = nvgRGBA(0xff, 0xd0, 0x00, 0xff);
        addChild(srChoice);

        // Status (levels or mute)
        statusChoice = new InfoText;
        statusChoice->box.pos = Vec(0, screenTop + 28.f);
        statusChoice->box.size = Vec(75, 14.f);
        statusChoice->text = "";
        statusChoice->color = nvgRGBA(0xff, 0xd0, 0x00, 0xff);
        addChild(statusChoice);

        // VU meter lights (left channel)
        float vuLX = 19.8f;
        float vuRX = 55.2f;
        float vuYs[6] = {85.3f, 101.0f, 116.6f, 132.3f, 148.0f, 163.5f};

        int leftVULights[6] = {
            RaAudio2Module::L_RED_LIGHT,
            RaAudio2Module::L_YELLOW_LIGHT,
            RaAudio2Module::L_GREEN4_LIGHT,
            RaAudio2Module::L_GREEN3_LIGHT,
            RaAudio2Module::L_GREEN2_LIGHT,
            RaAudio2Module::L_GREEN1_LIGHT,
        };
        for (int i = 0; i < 6; i++)
            addChild(createLightCentered<TinyLight<WhiteLight>>(Vec(vuLX, vuYs[i]), module, leftVULights[i]));

        int rightVULights[6] = {
            RaAudio2Module::R_RED_LIGHT,
            RaAudio2Module::R_YELLOW_LIGHT,
            RaAudio2Module::R_GREEN4_LIGHT,
            RaAudio2Module::R_GREEN3_LIGHT,
            RaAudio2Module::R_GREEN2_LIGHT,
            RaAudio2Module::R_GREEN1_LIGHT,
        };
        for (int i = 0; i < 6; i++)
            addChild(createLightCentered<TinyLight<WhiteLight>>(Vec(vuRX, vuYs[i]), module, rightVULights[i]));

        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(38.f, 228.4f), module, RaAudio2Module::LEVEL_PARAM));

        addParam(createLightParamCentered<VCVLightBezel<RedLight>>(Vec(63.f, 205.f), module, RaAudio2Module::MUTE_PARAM, RaAudio2Module::MUTE_LIGHT));

        addInput(createInputCentered<RaPort>(Vec(21.5f, 286.f), module, RaAudio2Module::L_INPUT));
        addInput(createInputCentered<RaPort>(Vec(53.5f, 286.f), module, RaAudio2Module::R_INPUT));

        addOutput(createOutputCentered<RaPort>(Vec(21.5f, 334.f), module, RaAudio2Module::L_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(53.5f, 334.f), module, RaAudio2Module::R_OUTPUT));
    }

    void step() override {
        auto* m = dynamic_cast<RaAudio2Module*>(module);
        if (m) {
            srChoice->text = string::f("SR: %.0f Hz", m->sampleRate);
            if (m->muted) {
                statusChoice->text = "MUTED";
                statusChoice->color = nvgRGBA(0xff, 0x20, 0x20, 0xff);
            } else {
                statusChoice->text = string::f("L:%+.1f  R:%+.1f", m->lastL, m->lastR);
                statusChoice->color = nvgRGBA(0xff, 0xd0, 0x00, 0xff);
            }
        }
        Widget::step();
    }
};

Model *modelRaAudio2 = createModel<RaAudio2Module, RaAudio2Widget>("ra-audio2");
