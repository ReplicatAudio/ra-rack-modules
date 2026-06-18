#include "ra-widgets.hpp"

using namespace rack;

extern Plugin *pluginInstance;

struct RaScalerModule : Module {
    enum ParamIds {
        SCALE_PARAM,
        CLIP_PARAM,
        POWER_PARAM,
        CLIP_MODE_PARAM,
        RANGE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    int lastRange = -1;

    RaScalerModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(SCALE_PARAM, 0.f, 1.f, 1.f, "Scale amount", "", 0.f, 10.f);
        configParam(CLIP_PARAM, 0.f, 1.f, 0.f, "Clip");
        configSwitch(POWER_PARAM, 0.f, 1.f, 0.f, "Power scale", {"Off", "On"});
        configSwitch(CLIP_MODE_PARAM, 0.f, 2.f, 0.f, "Clip mode", {"Hard", "Soft", "Fold"});
        configSwitch(RANGE_PARAM, 0.f, 2.f, 0.f, "Range", {"0\u201310", "\u00B15", "0\u20131"});
        configInput(CV_INPUT, "Signal");
        configOutput(OUTPUT, "Output");
    }

    void process(const ProcessArgs &args) override {
        float in = inputs[CV_INPUT].getVoltage();
        float scale = params[SCALE_PARAM].getValue();

        float scaled;
        if (params[POWER_PARAM].getValue() > 0.5f) {
            scaled = (in >= 0.f) ? powf(in, scale) : -powf(-in, scale);
        } else {
            scaled = in * scale;
        }

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

        float clip = params[CLIP_PARAM].getValue();
        float threshold = clip * fullScale;

        float out;
        switch ((int)std::round(params[CLIP_MODE_PARAM].getValue())) {
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

        outputs[OUTPUT].setVoltage(out);
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

struct RaScalerWidget : ModuleWidget {
    RaScalerWidget(RaScalerModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-scaler.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        float cx = box.size.x / 2;

        addInput(createInputCentered<RaPort>(Vec(cx, 22), module, RaScalerModule::CV_INPUT));
        addParam(createParamCentered<RaKnob>(Vec(cx, 56), module, RaScalerModule::SCALE_PARAM));
        addParam(createParamCentered<RaKnobSmall>(Vec(cx, 84), module, RaScalerModule::CLIP_PARAM));
        addParam(createParamCentered<RaSwitch3>(Vec(cx - 12, 118), module, RaScalerModule::RANGE_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(cx, 118), module, RaScalerModule::POWER_PARAM));
        addParam(createParamCentered<RaSwitch3>(Vec(cx + 12, 118), module, RaScalerModule::CLIP_MODE_PARAM));
        addOutput(createOutputCentered<RaPort>(Vec(cx, 158), module, RaScalerModule::OUTPUT));
    }
};

Model *modelRaScaler = createModel<RaScalerModule, RaScalerWidget>("ra-scaler");
