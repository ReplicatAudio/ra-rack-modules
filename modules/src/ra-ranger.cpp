#include "ra-components.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaRangerModule : Module {
    enum ParamIds {
        SCALE_PARAM,
        CLIP_PARAM,
        POWER_PARAM,
        CLIP_MODE_PARAM,
        RANGE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        SCALE_INPUT,
        CLIP_INPUT,
        INPUT1,
        INPUT2,
        INPUT3,
        INPUT4,
        NUM_INPUTS
    };
    enum OutputIds {
        OUTPUT1,
        OUTPUT2,
        OUTPUT3,
        OUTPUT4,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    int lastRange = -1;

    RaRangerModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(SCALE_PARAM, 0.f, 1.f, 1.f, "Scale amount", "", 0.f, 10.f);
        configParam(CLIP_PARAM, 0.f, 1.f, 1.f, "Clip", "%", 0.f, 100.f);
        configSwitch(POWER_PARAM, 0.f, 1.f, 0.f, "Power scale", {"Off", "On"});
        configSwitch(CLIP_MODE_PARAM, 0.f, 2.f, 0.f, "Clip mode", {"Hard", "Soft", "Fold"});
        configSwitch(RANGE_PARAM, 0.f, 2.f, 0.f, "Range", {"0\u201310", "\u00B15", "0\u20131"});
        configInput(SCALE_INPUT, "Scale amount");
        configInput(CLIP_INPUT, "Clip");
        for (int i = 0; i < 4; i++) {
            configInput(INPUT1 + i, string::f("Input %d", i + 1));
            configOutput(OUTPUT1 + i, string::f("Output %d", i + 1));
        }
    }

    void process(const ProcessArgs &args) override {
        int range = (int)std::round(params[RANGE_PARAM].getValue());
        float fullScale;
        float scaleDisplayMul = 1.f;
        float scaleDisplayOff = 0.f;
        switch (range) {
            default:
            case 0: fullScale = 10.f; scaleDisplayMul = 10.f; scaleDisplayOff = 0.f; break;
            case 1: fullScale = 5.f; scaleDisplayMul = 10.f; scaleDisplayOff = -5.f; break;
            case 2: fullScale = 1.f; scaleDisplayMul = 1.f; scaleDisplayOff = 0.f; break;
        }

        if (range != lastRange) {
            lastRange = range;
            if (paramQuantities[SCALE_PARAM]) {
                paramQuantities[SCALE_PARAM]->displayBase = 0.f;
                paramQuantities[SCALE_PARAM]->displayMultiplier = scaleDisplayMul;
                paramQuantities[SCALE_PARAM]->displayOffset = scaleDisplayOff;
            }
        }

        float scale = clamp(params[SCALE_PARAM].getValue() + inputs[SCALE_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float scaleFactor = scale * scaleDisplayMul + scaleDisplayOff;
        float clip = clamp(params[CLIP_PARAM].getValue() + inputs[CLIP_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        float threshold = clip * fullScale;
        int clipMode = (int)std::round(params[CLIP_MODE_PARAM].getValue());
        bool power = params[POWER_PARAM].getValue() > 0.5f;

        for (int i = 0; i < 4; i++) {
            float in = inputs[INPUT1 + i].getVoltage();

            float scaled;
            if (power) {
                float exponent = scale * 10.f;
                float sign = in >= 0.f ? 1.f : -1.f;
                scaled = sign * powf(fabs(in) + 1e-10f, exponent);
            } else {
                scaled = in * scaleFactor;
            }

            float out;
            switch (clipMode) {
                case 0: {
                    out = clamp(scaled, -threshold, threshold);
                    break;
                }
                case 1: {
                    if (threshold <= 0.001f) {
                        out = 0.f;
                    } else {
                        float ratio = clamp(scaled / threshold, -1.f, 1.f);
                        out = threshold * (1.5f * ratio - 0.5f * ratio * ratio * ratio);
                    }
                    break;
                }
                case 2: {
                    out = waveFold(scaled, threshold);
                    break;
                }
                default: {
                    out = scaled;
                    break;
                }
            }

            outputs[OUTPUT1 + i].setVoltage(out);
        }
    }

    float waveFold(float x, float t) {
        if (t <= 0.0001f) return 0.f;
        if (x > t) {
            float excess = x - t;
            float folds = floorf(excess / t);
            float rem = fmodf(excess, t);
            if (fmodf(folds, 2.f) < 0.5f)
                return t - rem;
            else
                return -t + rem;
        } else if (x < -t) {
            float excess = -x - t;
            float folds = floorf(excess / t);
            float rem = fmodf(excess, t);
            if (fmodf(folds, 2.f) < 0.5f)
                return -t + rem;
            else
                return t - rem;
        }
        return x;
    }
};

struct RaRangerWidget : ModuleWidget {
    RaRangerWidget(RaRangerModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-ranger.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float cx = box.size.x / 2;

        addParam(createParamCentered<RaKnob>(Vec(cx, 74.5), module, RaRangerModule::SCALE_PARAM));
        addParam(createParamCentered<RaKnobSmall>(Vec(cx, 108.5), module, RaRangerModule::CLIP_PARAM));
        addInput(createInputCentered<RaPort>(Vec(16, 136.5), module, RaRangerModule::SCALE_INPUT));
        addInput(createInputCentered<RaPort>(Vec(44, 136.5), module, RaRangerModule::CLIP_INPUT));
        addParam(createParamCentered<RaSwitch3>(Vec(16, 170.5), module, RaRangerModule::RANGE_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(30, 170.5), module, RaRangerModule::POWER_PARAM));
        addParam(createParamCentered<RaSwitch3>(Vec(44, 170.5), module, RaRangerModule::CLIP_MODE_PARAM));

        float rows[] = {225, 253, 281, 309};
        for (int i = 0; i < 4; i++) {
            addInput(createInputCentered<RaPort>(Vec(16, rows[i]), module, RaRangerModule::INPUT1 + i));
            addOutput(createOutputCentered<RaPort>(Vec(44, rows[i]), module, RaRangerModule::OUTPUT1 + i));
        }
    }
};

Model *modelRaRanger = createModel<RaRangerModule, RaRangerWidget>("ra-ranger");
