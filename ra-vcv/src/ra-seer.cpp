#include "ra-widgets.hpp"
#include <atomic>
#include <algorithm>
#include <cmath>

using namespace rack;

extern Plugin *pluginInstance;

struct RaSeerModule : Module {
    enum ParamIds {
        PARAM,
        MODE_PARAM,
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

    static constexpr int MAX_HISTORY = 2400000;
    static constexpr int FFT_SIZE = 256;
    static constexpr int NUM_BINS = 64;

    // Scope
    float history[MAX_HISTORY] = {};
    float history2[MAX_HISTORY] = {};
    std::atomic<int> head{0};
    std::atomic<int> displayLen{24000};

    // Utility
    float sum1 = 0.f, sum2 = 0.f;
    float sumSq1 = 0.f, sumSq2 = 0.f;
    float blockPeak1 = 0.f, blockPeak2 = 0.f;
    int blockCount = 0;
    std::atomic<float> peak1{0.f}, peak2{0.f};
    std::atomic<float> rms1{0.f}, rms2{0.f};
    std::atomic<float> dc1{0.f}, dc2{0.f};
    std::atomic<float> real1{0.f}, real2{0.f};

    // Spectrum
    float fftBuf1[FFT_SIZE] = {};
    float fftBuf2[FFT_SIZE] = {};
    int fftIdx = 0;
    std::atomic<float> spec1[NUM_BINS]{};
    std::atomic<float> spec2[NUM_BINS]{};
    float smooth1[NUM_BINS] = {};
    float smooth2[NUM_BINS] = {};

    std::atomic<int> mode{0};

    int bitRev[FFT_SIZE];
    float cosTbl[FFT_SIZE];
    float sinTbl[FFT_SIZE];
    float window[FFT_SIZE];
    bool tablesReady = false;

    RaSeerModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(PARAM, 0.f, 1.f, 0.5f, "Time/Smoothing", " ms", 10000.f, 5.f, 0.f);
        configSwitch(MODE_PARAM, 0.f, 2.f, 0.f, "Mode", {"Scope", "Spectrum", "Utility"});
        configInput(CH1_INPUT, "Channel 1");
        configInput(CH2_INPUT, "Channel 2");
        configOutput(CH1_OUTPUT, "Channel 1");
        configOutput(CH2_OUTPUT, "Channel 2");
        initFFT();
    }

    void initFFT() {
        if (tablesReady) return;
        tablesReady = true;
        for (int i = 0; i < FFT_SIZE; i++) {
            int rev = 0, n = FFT_SIZE, j = i;
            while (n >>= 1) { rev = (rev << 1) | (j & 1); j >>= 1; }
            bitRev[i] = rev;
            cosTbl[i] = cosf(2.f * M_PI * i / FFT_SIZE);
            sinTbl[i] = sinf(2.f * M_PI * i / FFT_SIZE);
            window[i] = 0.5f * (1.f - cosf(2.f * M_PI * i / (FFT_SIZE - 1)));
        }
    }

    void runFFT(const float *in, float *out) {
        float real[FFT_SIZE], imag[FFT_SIZE];
        for (int i = 0; i < FFT_SIZE; i++) {
            float w = in[i] * window[i];
            real[bitRev[i]] = w;
            imag[bitRev[i]] = 0.f;
        }
        int stages = 0, tmp = FFT_SIZE;
        while (tmp >>= 1) stages++;
        for (int s = 0; s < stages; s++) {
            int m = 1 << (s + 1), m2 = m >> 1;
            for (int k = 0; k < FFT_SIZE; k += m) {
                for (int j = 0; j < m2; j++) {
                    int step = FFT_SIZE / m;
                    float c = cosTbl[j * step];
                    float si = sinTbl[j * step];
                    float tr = c * real[k + j + m2] - si * imag[k + j + m2];
                    float ti = si * real[k + j + m2] + c * imag[k + j + m2];
                    real[k + j + m2] = real[k + j] - tr;
                    imag[k + j + m2] = imag[k + j] - ti;
                    real[k + j] += tr;
                    imag[k + j] += ti;
                }
            }
        }
        int binsPerBand = (FFT_SIZE / 2) / NUM_BINS;
        for (int b = 0; b < NUM_BINS; b++) {
            float sum = 0.f;
            int cnt = 0;
            for (int i = 0; i < binsPerBand; i++) {
                int idx = b * binsPerBand + i;
                if (idx >= FFT_SIZE / 2) break;
                sum += sqrtf(real[idx] * real[idx] + imag[idx] * imag[idx]);
                cnt++;
            }
            float avg = cnt > 0 ? sum / cnt : 0.f;
            float db = 20.f * log10f(avg + 1e-6f);
            out[b] = clamp((db + 60.f) / 60.f, 0.f, 1.f);
        }
    }

    void process(const ProcessArgs &args) override {
        float v1 = inputs[CH1_INPUT].getVoltage();
        float v2 = inputs[CH2_INPUT].getVoltage();
        outputs[CH1_OUTPUT].setVoltage(v1);
        outputs[CH2_OUTPUT].setVoltage(v2);

        int m = int(params[MODE_PARAM].getValue() + 0.5f);
        mode.store(m, std::memory_order_relaxed);
        float t = params[PARAM].getValue();

        if (m == 0) {
            int h = head.load(std::memory_order_relaxed);
            history[h] = v1;
            history2[h] = v2;
            head.store((h + 1) % MAX_HISTORY, std::memory_order_release);
            float timeMs = 5.f * powf(10000.f, t);
            int len = int(timeMs * args.sampleRate / 1000.f + 0.5f);
            displayLen.store(clamp(len, 2, MAX_HISTORY - 1), std::memory_order_relaxed);
        } else if (m == 1) {
            fftBuf1[fftIdx] = v1;
            fftBuf2[fftIdx] = v2;
            fftIdx++;
            if (fftIdx >= FFT_SIZE) {
                fftIdx = 0;
                float mag1[NUM_BINS], mag2[NUM_BINS];
                runFFT(fftBuf1, mag1);
                runFFT(fftBuf2, mag2);
                float decay = 0.5f + t * 0.49f;
                for (int i = 0; i < NUM_BINS; i++) {
                    smooth1[i] = smooth1[i] * decay + mag1[i] * (1.f - decay);
                    smooth2[i] = smooth2[i] * decay + mag2[i] * (1.f - decay);
                    spec1[i].store(smooth1[i], std::memory_order_relaxed);
                    spec2[i].store(smooth2[i], std::memory_order_relaxed);
                }
            }
        } else {
            sum1 += v1;
            sum2 += v2;
            sumSq1 += v1 * v1;
            sumSq2 += v2 * v2;
            blockPeak1 = fmaxf(fabsf(v1), blockPeak1);
            blockPeak2 = fmaxf(fabsf(v2), blockPeak2);
            blockCount++;
            float timeMs = 5.f * powf(10000.f, t);
            int blockSize = int(timeMs * args.sampleRate / 1000.f + 0.5f);
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
        }
    }
};

struct SeerDisplay : Widget {
    RaSeerModule *module;
    PortWidget *ch1Input = nullptr;
    PortWidget *ch2Input = nullptr;
    std::shared_ptr<Font> font;

    float local[RaSeerModule::MAX_HISTORY];
    float local2[RaSeerModule::MAX_HISTORY];

    SeerDisplay() {
        font = APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
    }

    void drawScope(const DrawArgs &args) {
        int head = module->head.load(std::memory_order_acquire);
        int len = module->displayLen.load(std::memory_order_relaxed);
        if (len < 2) return;
        len = std::min(len, RaSeerModule::MAX_HISTORY - 1);

        for (int i = 0; i < len; i++) {
            int idx = (head - 1 - i + RaSeerModule::MAX_HISTORY) % RaSeerModule::MAX_HISTORY;
            local[i] = module->history[idx];
            local2[i] = module->history2[idx];
        }

        float w = box.size.x, h = box.size.y;
        float cx = w / 2.f;

        NVGcolor col1 = nvgRGB(0xff, 0xcc, 0x00);
        NVGcolor col2 = nvgRGB(0x00, 0xcc, 0xff);
        if (ch1Input) {
            auto cables = APP->scene->rack->getCompleteCablesOnPort(ch1Input);
            if (!cables.empty()) col1 = cables[0]->color;
        }
        if (ch2Input) {
            auto cables = APP->scene->rack->getCompleteCablesOnPort(ch2Input);
            if (!cables.empty()) col2 = cables[0]->color;
        }

        nvgBeginPath(args.vg);
        nvgStrokeWidth(args.vg, 0.5f);
        nvgStrokeColor(args.vg, nvgRGB(0x33, 0x33, 0x33));
        nvgMoveTo(args.vg, cx, 0);
        nvgLineTo(args.vg, cx, h);
        nvgStroke(args.vg);

        int rows = int(h);
        double spr = double(len - 1) / std::max(rows - 1, 1);

        nvgBeginPath(args.vg);
        nvgStrokeWidth(args.vg, 1.f);
        nvgStrokeColor(args.vg, col2);
        for (int r = 0; r < rows; r++) {
            int si = int(r * spr);
            if (si >= len) si = len - 1;
            float x = cx + local2[si] * (cx / 5.f);
            float y = h - float(r) / float(rows - 1) * h;
            if (r == 0) nvgMoveTo(args.vg, x, y);
            else nvgLineTo(args.vg, x, y);
        }
        nvgStroke(args.vg);

        nvgBeginPath(args.vg);
        nvgStrokeWidth(args.vg, 1.f);
        nvgStrokeColor(args.vg, col1);
        for (int r = 0; r < rows; r++) {
            int si = int(r * spr);
            if (si >= len) si = len - 1;
            float x = cx + local[si] * (cx / 5.f);
            float y = h - float(r) / float(rows - 1) * h;
            if (r == 0) nvgMoveTo(args.vg, x, y);
            else nvgLineTo(args.vg, x, y);
        }
        nvgStroke(args.vg);
    }

    void drawSpectrum(const DrawArgs &args) {
        float w = box.size.x, h = box.size.y;

        NVGcolor col1 = nvgRGB(0xff, 0xcc, 0x00);
        NVGcolor col2 = nvgRGB(0x00, 0xcc, 0xff);
        if (ch1Input) {
            auto cables = APP->scene->rack->getCompleteCablesOnPort(ch1Input);
            if (!cables.empty()) col1 = cables[0]->color;
        }
        if (ch2Input) {
            auto cables = APP->scene->rack->getCompleteCablesOnPort(ch2Input);
            if (!cables.empty()) col2 = cables[0]->color;
        }

        int nb = RaSeerModule::NUM_BINS;
        float binH = h / nb;

        for (int i = 0; i < nb; i++) {
            float m1 = module->spec1[i].load(std::memory_order_relaxed);
            float m2 = module->spec2[i].load(std::memory_order_relaxed);
            float y = h - (i + 1) * binH;

            if (m2 > 0.01f) {
                nvgBeginPath(args.vg);
                nvgRect(args.vg, 0, y, m2 * w, binH);
                nvgFillColor(args.vg, col2);
                nvgFill(args.vg);
            }
            if (m1 > 0.01f) {
                nvgBeginPath(args.vg);
                nvgRect(args.vg, 0, y, m1 * w, binH);
                nvgFillColor(args.vg, col1);
                nvgFill(args.vg);
            }
        }
    }

    void drawUtility(const DrawArgs &args) {
        if (!font) return;
        nvgFontFaceId(args.vg, font->handle);

        NVGcolor col1 = nvgRGB(0xff, 0xcc, 0x00);
        NVGcolor col2 = nvgRGB(0x00, 0xcc, 0xff);
        if (ch1Input) {
            auto cables = APP->scene->rack->getCompleteCablesOnPort(ch1Input);
            if (!cables.empty()) col1 = cables[0]->color;
        }
        if (ch2Input) {
            auto cables = APP->scene->rack->getCompleteCablesOnPort(ch2Input);
            if (!cables.empty()) col2 = cables[0]->color;
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
                snprintf(buf, sizeof(buf), "%+.1fV", vals[r][c]);
                nvgFontSize(args.vg, 10);
                nvgFillColor(args.vg, chColors[c]);
                float y = (c == 0) ? rows[r].val1Y : rows[r].val2Y;
                nvgText(args.vg, 18, y, buf, NULL);
            }
        }
    }

    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2);
        nvgFillColor(args.vg, nvgRGB(0x10, 0x10, 0x10));
        nvgFill(args.vg);

        if (!module) return;
        int m = module->mode.load(std::memory_order_relaxed);
        switch (m) {
            case 0: drawScope(args); break;
            case 1: drawSpectrum(args); break;
            default: drawUtility(args); break;
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
        display->box.pos = Vec(6, 70);
        display->box.size = Vec(48, 260);
        display->module = module;
        addChild(display);

        display->ch1Input = createInputCentered<RaPort>(Vec(14, 26), module, RaSeerModule::CH1_INPUT);
        addChild(display->ch1Input);
        display->ch2Input = createInputCentered<RaPort>(Vec(46, 26), module, RaSeerModule::CH2_INPUT);
        addChild(display->ch2Input);

        addParam(createParamCentered<RaKnobTrim>(Vec(30, 46), module, RaSeerModule::PARAM));
        addParam(createParamCentered<RaSwitch3>(Vec(10, 48), module, RaSeerModule::MODE_PARAM));

        addOutput(createOutputCentered<RaPort>(Vec(14, 358), module, RaSeerModule::CH1_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(46, 358), module, RaSeerModule::CH2_OUTPUT));
    }
};

Model *modelRaSeer = createModel<RaSeerModule, RaSeerWidget>("ra-seer");
