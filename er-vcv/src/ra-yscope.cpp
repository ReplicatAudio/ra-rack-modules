#include "rack.hpp"
#include <atomic>

using namespace rack;

extern Plugin *pluginInstance;

struct RaYscopeModule : Module {
    enum ParamIds {
        TIME_PARAM,
        RANGE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        SIG_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        SIG_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    static constexpr int MAX_HISTORY = 2400000;
    float history[MAX_HISTORY] = {};
    std::atomic<int> head{0};
    std::atomic<int> displayLen{24000};
    std::atomic<int> range{0};

    RaYscopeModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(TIME_PARAM, 0.f, 1.f, 0.5f, "Time", " ms", 10000.f, 5.f, 0.f);
        configSwitch(RANGE_PARAM, 0.f, 1.f, 0.f, "Range", {"\u00B15V", "0\u201310V"});
        configInput(SIG_INPUT, "Signal");
        configOutput(SIG_OUTPUT, "Signal");
    }

    void process(const ProcessArgs &args) override {
        float v = inputs[SIG_INPUT].getVoltage();
        int h = head.load(std::memory_order_relaxed);
        history[h] = v;
        head.store((h + 1) % MAX_HISTORY, std::memory_order_release);
        outputs[SIG_OUTPUT].setVoltage(v);

        float t = params[TIME_PARAM].getValue();
        float timeMs = 5.f * powf(10000.f, t);
        int len = (int)(timeMs * args.sampleRate / 1000.f + 0.5f);
        displayLen.store(clamp(len, 2, MAX_HISTORY - 1), std::memory_order_relaxed);

        range.store((int)(params[RANGE_PARAM].getValue() + 0.5f), std::memory_order_relaxed);
    }
};

struct YscopeDisplay : Widget {
    RaYscopeModule *module;
    float local[RaYscopeModule::MAX_HISTORY];

    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
        nvgFillColor(args.vg, nvgRGB(0x10, 0x10, 0x10));
        nvgFill(args.vg);

        if (!module) return;

        int head = module->head.load(std::memory_order_acquire);
        int len = module->displayLen.load(std::memory_order_relaxed);
        if (len < 2) return;
        len = std::min(len, RaYscopeModule::MAX_HISTORY - 1);

        for (int i = 0; i < len; i++) {
            int idx = (head - 1 - i + RaYscopeModule::MAX_HISTORY) % RaYscopeModule::MAX_HISTORY;
            local[i] = module->history[idx];
        }

        float w = box.size.x;
        float h = box.size.y;
        float cx = w / 2.f;

        int rng = module->range.load(std::memory_order_relaxed);
        float hScale, offset;
        if (rng == 0) {
            offset = cx;
            hScale = cx / 5.f;
        } else {
            offset = 0.f;
            hScale = w / 10.f;
        }

        nvgBeginPath(args.vg);
        nvgStrokeWidth(args.vg, 0.5f);
        nvgStrokeColor(args.vg, nvgRGB(0x33, 0x33, 0x33));
        nvgMoveTo(args.vg, cx, 0);
        nvgLineTo(args.vg, cx, h);
        nvgStroke(args.vg);

        int rows = (int)h;
        double samplesPerRow = (double)(len - 1) / std::max(rows - 1, 1);

        nvgBeginPath(args.vg);
        nvgStrokeWidth(args.vg, 1.f);
        nvgStrokeColor(args.vg, nvgRGB(0xaa, 0xdd, 0x88));

        for (int r = 0; r < rows; r++) {
            int si = (int)(r * samplesPerRow);
            if (si >= len) si = len - 1;
            float v = local[si];
            float x = offset + v * hScale;
            float y = (float)r / (float)(rows - 1) * h;
            if (r == 0)
                nvgMoveTo(args.vg, x, y);
            else
                nvgLineTo(args.vg, x, y);
        }
        nvgStroke(args.vg);
    }
};

struct RaYscopeWidget : ModuleWidget {
    RaYscopeWidget(RaYscopeModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-yscope.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        auto *display = new YscopeDisplay();
        display->box.pos = Vec(6, 60);
        display->box.size = Vec(48, 270);
        display->module = module;
        addChild(display);

        addInput(createInputCentered<PJ301MPort>(Vec(30, 26), module, RaYscopeModule::SIG_INPUT));
        addParam(createParamCentered<Trimpot>(Vec(30, 46), module, RaYscopeModule::TIME_PARAM));
        addParam(createParamCentered<CKSS>(Vec(10, 48), module, RaYscopeModule::RANGE_PARAM));
        addOutput(createOutputCentered<PJ301MPort>(Vec(30, 358), module, RaYscopeModule::SIG_OUTPUT));
    }
};

Model *modelRaYscope = createModel<RaYscopeModule, RaYscopeWidget>("ra-yscope");
