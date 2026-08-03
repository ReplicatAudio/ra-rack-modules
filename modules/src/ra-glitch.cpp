#include "ra-components.hpp"

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
        SWAP_INPUT,
        FREQ_CV_INPUT,
        LENGTH_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        AUDIO_OUTPUT,
        SWAP_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    static const int FREEZE_BUFFER_SIZE = 48000;
    float freezeBuffer[FREEZE_BUFFER_SIZE] = {};
    int freezeWritePos = 0;
    int freezeReadPos = 0;
    int freezeCaptureLen = 0;
    bool freezeCaptured = false;

    bool inGlitch = false;
    float glitchTimer = 0.f;

    RaGlitchModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(FREQ_PARAM, 0.f, 1.f, 0.2f, "Frequency", "%", 0.f, 100.f);
        configParam(LENGTH_PARAM, 0.f, 1.f, 0.1f, "Length", " ms", 0.f, 499.f, 1.f);
        configSwitch(MODE_PARAM, 0.f, 2.f, 0.f, "Mode", {"Freeze", "Swap", "Drop"});
        configInput(AUDIO_INPUT, "Audio");
        configInput(SWAP_INPUT, "Swap");
        configInput(FREQ_CV_INPUT, "Frequency CV");
        configInput(LENGTH_CV_INPUT, "Length CV");
        configOutput(AUDIO_OUTPUT, "Audio");
        configOutput(SWAP_OUTPUT, "Swap");
    }

    void process(const ProcessArgs &args) override {
        float freq = clamp(params[FREQ_PARAM].getValue() + inputs[FREQ_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float maxLenMs = 1.f + clamp(params[LENGTH_PARAM].getValue() + inputs[LENGTH_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f) * 499.f;
        int mode = (int)std::round(params[MODE_PARAM].getValue());

        float in1 = inputs[AUDIO_INPUT].getVoltage();
        float in2 = inputs[SWAP_INPUT].getVoltage();
        float out1, out2;

        if (inGlitch) {
            glitchTimer -= args.sampleTime;

            if (mode == 0 && !freezeCaptured) {
                freezeBuffer[freezeWritePos] = in1;
                freezeWritePos++;
                if (freezeWritePos >= freezeCaptureLen) {
                    freezeCaptured = true;
                }
            }

            switch (mode) {
                case 0: {
                    if (freezeCaptured && freezeCaptureLen > 0) {
                        out1 = freezeBuffer[freezeReadPos % freezeCaptureLen];
                        freezeReadPos++;
                    } else {
                        out1 = in1;
                    }
                    out2 = in2;
                    break;
                }
                case 1:
                    out1 = in2;
                    out2 = in1;
                    break;
                default:
                    out1 = 0.f;
                    out2 = 0.f;
                    break;
            }

            if (glitchTimer <= 0.f && freq < 1.f) {
                inGlitch = false;
            }
        } else {
            out1 = in1;
            out2 = in2;
        }

        if (!inGlitch) {
            if (freq >= 1.f || random::uniform() < freq * 10.f * args.sampleTime) {
                inGlitch = true;
                glitchTimer = random::uniform() * maxLenMs / 1000.f;
                freezeWritePos = 0;
                freezeReadPos = 0;
                freezeCaptured = false;
                int glitchSamples = (int)(glitchTimer * args.sampleRate);
                freezeCaptureLen = std::max(16, std::min(glitchSamples / 4, FREEZE_BUFFER_SIZE));
            }
        }

        outputs[AUDIO_OUTPUT].setVoltage(out1);
        outputs[SWAP_OUTPUT].setVoltage(out2);
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
        addInput(createInputCentered<RaPort>(Vec(rx, 25), module, RaGlitchModule::SWAP_INPUT));

        addParam(createParamCentered<RaKnob>(Vec(cx, 75), module, RaGlitchModule::FREQ_PARAM));
        addInput(createInputCentered<RaPort>(Vec(cx, 110), module, RaGlitchModule::FREQ_CV_INPUT));

        addParam(createParamCentered<RaKnob>(Vec(cx, 155), module, RaGlitchModule::LENGTH_PARAM));
        addInput(createInputCentered<RaPort>(Vec(cx, 190), module, RaGlitchModule::LENGTH_CV_INPUT));

        addParam(createParamCentered<RaSwitch3>(Vec(cx, 245), module, RaGlitchModule::MODE_PARAM));

        addOutput(createOutputCentered<RaPort>(Vec(lx, 315), module, RaGlitchModule::AUDIO_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(rx, 315), module, RaGlitchModule::SWAP_OUTPUT));
    }
};

Model *modelRaGlitch = createModel<RaGlitchModule, RaGlitchWidget>("ra-glitch");
