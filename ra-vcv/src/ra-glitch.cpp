#include "ra-widgets.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaGlitchModule : Module {
    enum ParamIds {
        FREQ_PARAM,
        LENGTH_PARAM,
        MODE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        AUDIO_INPUT,
        REPLACE_INPUT,
        FREQ_CV_INPUT,
        LENGTH_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        AUDIO_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    static const int FREEZE_BUFFER_SIZE = 44100;
    float freezeBuffer[FREEZE_BUFFER_SIZE] = {};
    int freezeWritePos = 0;
    int freezeReadPos = 0;

    bool inGlitch = false;
    float glitchTimer = 0.f;

    RaGlitchModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(FREQ_PARAM, 0.f, 1.f, 0.2f, "Frequency", "%", 0.f, 100.f);
        configParam(LENGTH_PARAM, 0.f, 1.f, 0.3f, "Length", "%", 0.f, 100.f);
        configSwitch(MODE_PARAM, 0.f, 2.f, 0.f, "Mode", {"Freeze", "Replace", "Drop"});
        configInput(AUDIO_INPUT, "Audio");
        configInput(REPLACE_INPUT, "Replace");
        configInput(FREQ_CV_INPUT, "Frequency CV");
        configInput(LENGTH_CV_INPUT, "Length CV");
        configOutput(AUDIO_OUTPUT, "Audio");
    }

    void process(const ProcessArgs &args) override {
        float freq = clamp(params[FREQ_PARAM].getValue() + inputs[FREQ_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float maxLen = clamp(params[LENGTH_PARAM].getValue() + inputs[LENGTH_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        int mode = (int)std::round(params[MODE_PARAM].getValue());

        float in = inputs[AUDIO_INPUT].getVoltage();
        float out;

        if (inGlitch) {
            glitchTimer -= args.sampleTime;

            if (freezeWritePos < FREEZE_BUFFER_SIZE) {
                freezeBuffer[freezeWritePos] = in;
                freezeWritePos++;
            }

            switch (mode) {
                case 0: {
                    if (freezeWritePos > 0) {
                        out = freezeBuffer[freezeReadPos % freezeWritePos];
                        freezeReadPos++;
                    } else {
                        out = 0.f;
                    }
                    break;
                }
                case 1:
                    out = inputs[REPLACE_INPUT].getVoltage();
                    break;
                default:
                    out = 0.f;
                    break;
            }

            if (glitchTimer <= 0.f) {
                inGlitch = false;
            }
        } else {
            out = in;
        }

        if (!inGlitch) {
            float glitchRate = freq * 10.f;
            if (random::uniform() < glitchRate * args.sampleTime) {
                inGlitch = true;
                float MAX_GLITCH_SEC = 1.f;
                glitchTimer = random::uniform() * maxLen * MAX_GLITCH_SEC;
                freezeWritePos = 0;
                freezeReadPos = 0;
            }
        }

        outputs[AUDIO_OUTPUT].setVoltage(out);
    }
};

struct RaGlitchWidget : ModuleWidget {
    RaGlitchWidget(RaGlitchModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-glitch.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float cx = box.size.x / 2;
        float lx = 22;
        float rx = 68;

        addInput(createInputCentered<RaPort>(Vec(lx, 25), module, RaGlitchModule::AUDIO_INPUT));
        addInput(createInputCentered<RaPort>(Vec(rx, 25), module, RaGlitchModule::REPLACE_INPUT));

        addParam(createParamCentered<RaKnob>(Vec(cx, 75), module, RaGlitchModule::FREQ_PARAM));
        addInput(createInputCentered<RaPort>(Vec(cx, 110), module, RaGlitchModule::FREQ_CV_INPUT));

        addParam(createParamCentered<RaKnob>(Vec(cx, 155), module, RaGlitchModule::LENGTH_PARAM));
        addInput(createInputCentered<RaPort>(Vec(cx, 190), module, RaGlitchModule::LENGTH_CV_INPUT));

        addParam(createParamCentered<RaSwitch3>(Vec(cx, 245), module, RaGlitchModule::MODE_PARAM));

        addOutput(createOutputCentered<RaPort>(Vec(cx, 315), module, RaGlitchModule::AUDIO_OUTPUT));
    }
};

Model *modelRaGlitch = createModel<RaGlitchModule, RaGlitchWidget>("ra-glitch");
