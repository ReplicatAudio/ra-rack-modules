#include "ra-components.hpp"
#include <atomic>

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

    std::atomic<float> displayIn{0.f};
    std::atomic<float> displayDiff{0.f};
    std::atomic<float> displayOut{0.f};

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

        float scaleFactor = scale * scaleDisplayMul + scaleDisplayOff;

        float scaled;
        if (params[POWER_PARAM].getValue() > 0.5f) {
            float exponent = scale * 10.f;
            float sign = in >= 0.f ? 1.f : -1.f;
            scaled = sign * powf(fabs(in) + 1e-10f, exponent);
        } else {
            scaled = in * scaleFactor;
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

        displayIn.store(in, std::memory_order_relaxed);
        displayDiff.store(out - in, std::memory_order_relaxed);
        displayOut.store(out, std::memory_order_relaxed);
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

struct ScalerDisplay : Widget {
    RaScalerModule *module;
    std::shared_ptr<Font> font;

    ScalerDisplay() {
        font = APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
    }

    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2);
        nvgFillColor(args.vg, nvgRGB(0x10, 0x10, 0x10));
        nvgFill(args.vg);

        if (!module || !font) return;

        nvgFontFaceId(args.vg, font->handle);

        float in = module->displayIn.load(std::memory_order_relaxed);
        float diff = module->displayDiff.load(std::memory_order_relaxed);
        float out = module->displayOut.load(std::memory_order_relaxed);

        struct Row { const char *label; float labelY; float valY; };
        Row rows[] = {
            {"IN",   10.f, 20.f},
            {"DIFF", 32.f, 42.f},
            {"OUT",  54.f, 64.f},
        };

        nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);

        for (int r = 0; r < 3; r++) {
            nvgFontSize(args.vg, 9);
            nvgFillColor(args.vg, nvgRGBA(0x88, 0x88, 0x88, 0xff));
            nvgText(args.vg, 5, rows[r].labelY, rows[r].label, NULL);

            char buf[32];
            float vals[] = {in, diff, out};
            snprintf(buf, sizeof(buf), "%+.2fV", vals[r]);
            nvgFontSize(args.vg, 10);
            nvgFillColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
            nvgText(args.vg, 10, rows[r].valY, buf, NULL);
        }
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

        addInput(createInputCentered<RaPort>(Vec(cx, 74.5), module, RaScalerModule::CV_INPUT));
        addParam(createParamCentered<RaKnob>(Vec(cx, 108.5), module, RaScalerModule::SCALE_PARAM));
        addParam(createParamCentered<RaKnobSmall>(Vec(cx, 136.5), module, RaScalerModule::CLIP_PARAM));
        addParam(createParamCentered<RaSwitch3>(Vec(16, 170.5), module, RaScalerModule::RANGE_PARAM));
        addParam(createParamCentered<RaSwitch2>(Vec(30, 170.5), module, RaScalerModule::POWER_PARAM));
        addParam(createParamCentered<RaSwitch3>(Vec(44, 170.5), module, RaScalerModule::CLIP_MODE_PARAM));
        addOutput(createOutputCentered<RaPort>(Vec(cx, 210.5), module, RaScalerModule::OUTPUT));

        auto *display = new ScalerDisplay();
        display->box.pos = Vec(6, 224.5);
        display->box.size = Vec(48, 80);
        display->module = module;
        addChild(display);
    }
};

Model *modelRaScaler = createModel<RaScalerModule, RaScalerWidget>("ra-scaler");
