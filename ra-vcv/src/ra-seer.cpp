#include "ra-widgets.hpp"
#include <atomic>
#include <algorithm>

using namespace rack;

extern Plugin *pluginInstance;

struct RaSeerModule : Module {
    enum ParamIds {
        SMOOTH_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CH1_INPUT,
        CH2_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        CH1_OUTPUT,
        CH2_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    float sum1 = 0.f, sum2 = 0.f;
    float sumSq1 = 0.f, sumSq2 = 0.f;
    float blockPeak1 = 0.f, blockPeak2 = 0.f;
    int blockCount = 0;

    std::atomic<float> peak1{0.f}, peak2{0.f};
    std::atomic<float> rms1{0.f}, rms2{0.f};
    std::atomic<float> dc1{0.f}, dc2{0.f};
    std::atomic<float> real1{0.f}, real2{0.f};

    RaSeerModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(SMOOTH_PARAM, 0.f, 1.f, 0.5f, "Smoothing", " ms", 10000.f, 5.f, 0.f);
        configInput(CH1_INPUT, "Channel 1");
        configInput(CH2_INPUT, "Channel 2");
        configOutput(CH1_OUTPUT, "Channel 1");
        configOutput(CH2_OUTPUT, "Channel 2");
    }

    void process(const ProcessArgs &args) override {
        float v1 = inputs[CH1_INPUT].getVoltage();
        float v2 = inputs[CH2_INPUT].getVoltage();

        sum1 += v1;
        sum2 += v2;
        sumSq1 += v1 * v1;
        sumSq2 += v2 * v2;
        blockPeak1 = fmaxf(fabsf(v1), blockPeak1);
        blockPeak2 = fmaxf(fabsf(v2), blockPeak2);
        blockCount++;

        float t = params[SMOOTH_PARAM].getValue();
        float timeMs = 5.f * powf(10000.f, t);
        int blockSize = (int)(timeMs * args.sampleRate / 1000.f + 0.5f);
        blockSize = clamp(blockSize, 2, 2400000);

        if (blockCount >= blockSize) {
            float inv = 1.f / blockCount;
            dc1.store(sum1 * inv, std::memory_order_relaxed);
            dc2.store(sum2 * inv, std::memory_order_relaxed);
            rms1.store(sqrtf(sumSq1 * inv), std::memory_order_relaxed);
            rms2.store(sqrtf(sumSq2 * inv), std::memory_order_relaxed);
            peak1.store(blockPeak1, std::memory_order_relaxed);
            peak2.store(blockPeak2, std::memory_order_relaxed);
            sum1 = sum2 = 0.f;
            sumSq1 = sumSq2 = 0.f;
            blockPeak1 = blockPeak2 = 0.f;
            blockCount = 0;
        }

        real1.store(v1, std::memory_order_relaxed);
        real2.store(v2, std::memory_order_relaxed);

        outputs[CH1_OUTPUT].setVoltage(v1);
        outputs[CH2_OUTPUT].setVoltage(v2);
    }
};

struct SeerDisplay : Widget {
    RaSeerModule *module;
    PortWidget *ch1Input = nullptr;
    PortWidget *ch2Input = nullptr;
    std::shared_ptr<Font> font;

    SeerDisplay() {
        font = APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
    }

    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2);
        nvgFillColor(args.vg, nvgRGB(0x10, 0x10, 0x10));
        nvgFill(args.vg);

        if (!module || !font) return;

        nvgFontFaceId(args.vg, font->handle);

        NVGcolor col1 = nvgRGB(0xff, 0xcc, 0x00);
        NVGcolor col2 = nvgRGB(0x00, 0xcc, 0xff);
        if (ch1Input) {
            auto cables = APP->scene->rack->getCompleteCablesOnPort(ch1Input);
            if (!cables.empty())
                col1 = cables[0]->color;
        }
        if (ch2Input) {
            auto cables = APP->scene->rack->getCompleteCablesOnPort(ch2Input);
            if (!cables.empty())
                col2 = cables[0]->color;
        }

        struct Row { const char *label; float labelY; float val1Y; float val2Y; };
        Row rows[] = {
            {"PEAK", 14.f, 28.f, 40.f},
            {"RMS",  64.f, 78.f, 90.f},
            {"DC",  114.f, 128.f, 140.f},
            {"REAL", 164.f, 178.f, 190.f},
        };

        float peak1 = module->peak1.load(std::memory_order_relaxed);
        float peak2 = module->peak2.load(std::memory_order_relaxed);
        float rms1 = module->rms1.load(std::memory_order_relaxed);
        float rms2 = module->rms2.load(std::memory_order_relaxed);
        float dc1 = module->dc1.load(std::memory_order_relaxed);
        float dc2 = module->dc2.load(std::memory_order_relaxed);
        float real1 = module->real1.load(std::memory_order_relaxed);
        float real2 = module->real2.load(std::memory_order_relaxed);
        float vals[][2] = {{peak1, peak2}, {rms1, rms2}, {dc1, dc2}, {real1, real2}};
        NVGcolor chColors[] = {col1, col2};

        nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);

        for (int r = 0; r < 4; r++) {
            nvgFontSize(args.vg, 9);
            nvgFillColor(args.vg, nvgRGBA(0x88, 0x88, 0x88, 0xff));
            nvgText(args.vg, 5, rows[r].labelY, rows[r].label, NULL);

            for (int c = 0; c < 2; c++) {
                char buf[32];
                float v = vals[r][c];
                snprintf(buf, sizeof(buf), "%+.1fV", v);
                nvgFontSize(args.vg, 10);
                nvgFillColor(args.vg, chColors[c]);
                float y = (c == 0) ? rows[r].val1Y : rows[r].val2Y;
                nvgText(args.vg, 18, y, buf, NULL);
            }
        }
    }
};

struct RaSeerWidget : ModuleWidget {
    RaSeerWidget(RaSeerModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-seer.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        auto *display = new SeerDisplay();
        display->box.pos = Vec(6, 60);
        display->box.size = Vec(48, 270);
        display->module = module;
        addChild(display);

        display->ch1Input = createInputCentered<RaPort>(Vec(14, 26), module, RaSeerModule::CH1_INPUT);
        addChild(display->ch1Input);
        display->ch2Input = createInputCentered<RaPort>(Vec(46, 26), module, RaSeerModule::CH2_INPUT);
        addChild(display->ch2Input);

        addParam(createParamCentered<RaKnobTrim>(Vec(30, 46), module, RaSeerModule::SMOOTH_PARAM));

        addOutput(createOutputCentered<RaPort>(Vec(14, 358), module, RaSeerModule::CH1_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(46, 358), module, RaSeerModule::CH2_OUTPUT));
    }
};

Model *modelRaSeer = createModel<RaSeerModule, RaSeerWidget>("ra-seer");
