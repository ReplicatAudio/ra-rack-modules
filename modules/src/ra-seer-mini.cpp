#include "ra-components.hpp"
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstring>

using namespace rack;

extern Plugin *pluginInstance;

struct RaSeerMiniModule : Module {
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
        MODE_LIGHT_R,
        MODE_LIGHT_G,
        MODE_LIGHT_B,
        NUM_LIGHTS
    };

    static constexpr int MAX_HISTORY = 2400000;
    static constexpr int FFT_SIZE = 256;
    static constexpr int NUM_BINS = 64;

    // Scope — history is shared between the channels of each port so total
    // memory stays constant: each channel gets MAX_HISTORY / channels samples.
    float history[MAX_HISTORY] = {};
    float history2[MAX_HISTORY] = {};
    int channels1 = 1;
    int channels2 = 1;
    int cap1 = MAX_HISTORY;
    int cap2 = MAX_HISTORY;
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
    dsp::SchmittTrigger modeCycleTrigger;

    int bitRev[FFT_SIZE];
    float cosTbl[FFT_SIZE];
    float sinTbl[FFT_SIZE];
    float window[FFT_SIZE];
    bool tablesReady = false;

    RaSeerMiniModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(PARAM, 0.f, 1.f, 0.5f, "Time/Smoothing", " ms", 10000.f, 5.f, 0.f);
        configSwitch(MODE_PARAM, 0.f, 1.f, 0.f, "Cycle mode", {"Cycle"});
        configInput(CH1_INPUT, "Channel 1");
        configInput(CH2_INPUT, "Channel 2");
        configOutput(CH1_OUTPUT, "Channel 1");
        configOutput(CH2_OUTPUT, "Channel 2");
        configLight(MODE_LIGHT_R, "Mode indicator");
        configLight(MODE_LIGHT_G, "");
        configLight(MODE_LIGHT_B, "");
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
        int ch1 = inputs[CH1_INPUT].getChannels();
        int ch2 = inputs[CH2_INPUT].getChannels();
        if (ch1 != channels1) {
            channels1 = ch1;
            cap1 = std::max(1, MAX_HISTORY / std::max(ch1, 1));
            memset(history, 0, sizeof(history));
        }
        if (ch2 != channels2) {
            channels2 = ch2;
            cap2 = std::max(1, MAX_HISTORY / std::max(ch2, 1));
            memset(history2, 0, sizeof(history2));
        }

        outputs[CH1_OUTPUT].setChannels(ch1);
        outputs[CH1_OUTPUT].writeVoltages(inputs[CH1_INPUT].getVoltages());
        outputs[CH2_OUTPUT].setChannels(ch2);
        outputs[CH2_OUTPUT].writeVoltages(inputs[CH2_INPUT].getVoltages());

        if (modeCycleTrigger.process(params[MODE_PARAM].getValue())) {
            mode = (mode.load() + 1) % 3;
        }
        int m = mode.load(std::memory_order_relaxed);

        float lr = 0.f, lg = 0.f, lb = 0.f;
        switch (m) {
            case 0: lr = lg = lb = 1.0f; break;
            case 1: lr = lg = 1.0f; break;
            case 2: lg = 1.0f; break;
        }
        lights[MODE_LIGHT_R].setBrightness(lr);
        lights[MODE_LIGHT_G].setBrightness(lg);
        lights[MODE_LIGHT_B].setBrightness(lb);
        float t = params[PARAM].getValue();

        if (m == 0) {
            int h = head.load(std::memory_order_relaxed);
            for (int c = 0; c < channels1; c++)
                history[c * cap1 + h % cap1] = inputs[CH1_INPUT].getVoltage(c);
            for (int c = 0; c < channels2; c++)
                history2[c * cap2 + h % cap2] = inputs[CH2_INPUT].getVoltage(c);
            head.store((h + 1) % MAX_HISTORY, std::memory_order_release);
            float timeMs = 5.f * powf(10000.f, t);
            int len = int(timeMs * args.sampleRate / 1000.f + 0.5f);
            displayLen.store(clamp(len, 2, MAX_HISTORY - 1), std::memory_order_relaxed);
        } else if (m == 1) {
            fftBuf1[fftIdx] = inputs[CH1_INPUT].getVoltageSum();
            fftBuf2[fftIdx] = inputs[CH2_INPUT].getVoltageSum();
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
            for (int c = 0; c < channels1; c++) {
                float v = inputs[CH1_INPUT].getVoltage(c);
                sum1 += v;
                sumSq1 += v * v;
                blockPeak1 = fmaxf(fabsf(v), blockPeak1);
            }
            for (int c = 0; c < channels2; c++) {
                float v = inputs[CH2_INPUT].getVoltage(c);
                sum2 += v;
                sumSq2 += v * v;
                blockPeak2 = fmaxf(fabsf(v), blockPeak2);
            }
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
            real1.store(inputs[CH1_INPUT].getVoltageSum(), std::memory_order_relaxed);
            real2.store(inputs[CH2_INPUT].getVoltageSum(), std::memory_order_relaxed);
        }
    }
};

struct SeerDisplay : Widget {
    RaSeerMiniModule *module;
    PortWidget *ch1Input = nullptr;
    PortWidget *ch2Input = nullptr;
    std::shared_ptr<Font> font;

    float local[RaSeerMiniModule::MAX_HISTORY];

    SeerDisplay() {
        font = APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
    }

    void drawScope(const DrawArgs &args) {
        int head = module->head.load(std::memory_order_acquire);
        int len = module->displayLen.load(std::memory_order_relaxed);
        if (len < 2) return;

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

        // One overlaid trace per poly channel, in the port's cable color
        auto strokeTrace = [&](const float *buf, int cap, NVGcolor col) {
            int l = std::min(len, cap);
            if (l < 2) return;
            for (int i = 0; i < l; i++) {
                int m = (head - 1 - i) % RaSeerMiniModule::MAX_HISTORY;
                if (m < 0) m += RaSeerMiniModule::MAX_HISTORY;
                local[i] = buf[m % cap];
            }
            nvgBeginPath(args.vg);
            nvgStrokeWidth(args.vg, 1.f);
            nvgStrokeColor(args.vg, col);
            for (int r = 0; r < rows; r++) {
                int si = int(r * spr);
                if (si >= l) si = l - 1;
                float x = cx + local[si] * (cx / 5.f);
                float y = h - float(r) / float(rows - 1) * h;
                if (r == 0) nvgMoveTo(args.vg, x, y);
                else nvgLineTo(args.vg, x, y);
            }
            nvgStroke(args.vg);
        };

        for (int c = 0; c < module->channels2; c++)
            strokeTrace(&module->history2[c * module->cap2], module->cap2, col2);
        for (int c = 0; c < module->channels1; c++)
            strokeTrace(&module->history[c * module->cap1], module->cap1, col1);
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

        int nb = RaSeerMiniModule::NUM_BINS;
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
        // Screen backdrop — painted slightly larger than the box to cover the
        // SVG bezel outline, recolored with a muted purple border to match the accent
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, -3, -3, box.size.x + 6, box.size.y + 6, 4);
        nvgFillColor(args.vg, nvgRGB(0x10, 0x10, 0x10));
        nvgFill(args.vg);
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStrokeColor(args.vg, nvgRGB(0x4a, 0x40, 0x66));
        nvgStroke(args.vg);

        if (!module) return;
        int m = module->mode.load(std::memory_order_relaxed);
        switch (m) {
            case 0: drawScope(args); break;
            case 1: drawSpectrum(args); break;
            default: drawUtility(args); break;
        }
    }
};

struct RaSeerMiniWidget : ModuleWidget {
    RaSeerMiniWidget(RaSeerMiniModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-seer-mini.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        auto *display = new SeerDisplay();
        display->box.pos = Vec(6, 70);
        display->box.size = Vec(48, 235);
        display->module = module;
        addChild(display);

        display->ch1Input = createInputCentered<RaPort>(Vec(14, 26), module, RaSeerMiniModule::CH1_INPUT);
        addChild(display->ch1Input);
        display->ch2Input = createInputCentered<RaPort>(Vec(46, 26), module, RaSeerMiniModule::CH2_INPUT);
        addChild(display->ch2Input);

        addParam(createParamCentered<RaKnobTrim>(Vec(30, 46), module, RaSeerMiniModule::PARAM));

        addParam(createLightParamCentered<VCVLightBezel<RedGreenBlueLight>>(Vec(30, 322), module, RaSeerMiniModule::MODE_PARAM, RaSeerMiniModule::MODE_LIGHT_R));

        addOutput(createOutputCentered<RaPort>(Vec(14, 358), module, RaSeerMiniModule::CH1_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(46, 358), module, RaSeerMiniModule::CH2_OUTPUT));
    }
};

Model *modelRaSeerMini = createModel<RaSeerMiniModule, RaSeerMiniWidget>("ra-seer-mini");
