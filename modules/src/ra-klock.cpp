#include "ra-components.hpp"
#include <atomic>

using namespace rack;

extern Plugin *pluginInstance;

struct RatioEntry {
    const char* label;
    float mult; // multiplier value: N for xN, 1/N for /N
};

static const RatioEntry RATIOS[] = {
    {"/32", 1.f / 32.f},
    {"/16", 1.f / 16.f},
    {"/12", 1.f / 12.f},
    {"/8", 1.f / 8.f},
    {"/6", 1.f / 6.f},
    {"/5", 1.f / 5.f},
    {"/4", 1.f / 4.f},
    {"/3", 1.f / 3.f},
    {"/2", 1.f / 2.f},
    {"x1", 1.f},
    {"x2", 2.f},
    {"x3", 3.f},
    {"x4", 4.f},
    {"x5", 5.f},
    {"x6", 6.f},
    {"x8", 8.f},
    {"x12", 12.f},
    {"x16", 16.f},
    {"x24", 24.f},
    {"x32", 32.f},
};

static const int NUM_RATIOS = (int)(sizeof(RATIOS) / sizeof(RATIOS[0]));
// Index of the "x1" passthrough entry
static const float RATIO_DEFAULT = 9.f;

/** Ratio knob quantity — the knob snaps between discrete ratios and the hover
    tooltip shows the snapped ratio label. */
struct RatioParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        int idx = clamp((int)lroundf(getValue()), 0, NUM_RATIOS - 1);
        return RATIOS[idx].label;
    }
};

struct RaKlockModule : Module {
    enum ParamIds {
        BPM_PARAM,
        SWING_PARAM,
        RUN_PARAM,
        RESET_PARAM,
        OUT1_PARAM,
        OUT2_PARAM,
        OUT3_PARAM,
        OUT4_PARAM,
        OUT5_PARAM,
        OUT6_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        V_OCT_INPUT,
        SWING_INPUT,
        RUN_INPUT,
        RESET_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        CLK_OUTPUT,
        OUT1_OUTPUT,
        OUT2_OUTPUT,
        OUT3_OUTPUT,
        OUT4_OUTPUT,
        OUT5_OUTPUT,
        OUT6_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        BEAT_LIGHT,
        RUN_LIGHT,
        RESET_LIGHT,
        NUM_LIGHTS
    };

    dsp::SchmittTrigger runInputTrigger;
    dsp::SchmittTrigger resetInputTrigger;
    bool running = false;
    bool prevRunButton = false;
    bool prevResetButton = false;

    float phase = 0.f;
    int beatCount = 0;
    int prevBeatCount = 0;

    int lastRatioIdx[6] = {-1, -1, -1, -1, -1, -1};
    float divPhase[6] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f};

    dsp::PulseGenerator clkPulse;
    dsp::PulseGenerator beatPulse;
    dsp::PulseGenerator resetPulse;
    dsp::PulseGenerator outPulse[6];

    std::atomic<float> displayBpm{0.f};
    std::atomic<float> displaySwing{0.f};
    std::atomic<float> displayPhase{0.f};
    std::atomic<int> displayBeatCount{0};
    std::atomic<bool> displayBeat{false};
    std::atomic<bool> displayRunning{false};

    RaKlockModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(BPM_PARAM, 0.f, 1.f, 0.5f, "BPM", " BPM", 0.f, 333.f, 0.f);
        configParam(SWING_PARAM, 0.f, 1.f, 0.f, "Swing", "%", 0.f, 100.f);
        configButton(RUN_PARAM, "Run");
        configButton(RESET_PARAM, "Reset");
        for (int i = 0; i < 6; i++) {
            RatioParamQuantity *pq = configParam<RatioParamQuantity>(OUT1_PARAM + i, 0.f, (float)(NUM_RATIOS - 1), RATIO_DEFAULT, "");
            pq->name = string::f("Out %d", i + 1);
            pq->snapEnabled = true;
        }
        configInput(V_OCT_INPUT, "v/oct");
        configInput(SWING_INPUT, "Swing CV");
        configInput(RUN_INPUT, "Run");
        configInput(RESET_INPUT, "Reset");
        configOutput(CLK_OUTPUT, "CLK");
        for (int i = 0; i < 6; i++)
            configOutput(OUT1_OUTPUT + i, string::f("Out %d", i + 1));
        configLight(BEAT_LIGHT, "Beat");
        configLight(RUN_LIGHT, "Run");
        configLight(RESET_LIGHT, "Reset");
    }

    int ratioIndex(int out) {
        return clamp((int)lroundf(params[OUT1_PARAM + out].getValue()), 0, NUM_RATIOS - 1);
    }

    void resetOutPhases() {
        for (int i = 0; i < 6; i++) {
            lastRatioIdx[i] = -1;
            divPhase[i] = 0.f;
        }
    }

    void doReset() {
        phase = 0.f;
        beatCount = 0;
        prevBeatCount = 0;
        resetOutPhases();
        resetPulse.trigger(1e-3f);
    }

    void onReset() override {
        running = false;
        prevRunButton = false;
        prevResetButton = false;
        doReset();
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        json_object_set_new(rootJ, "running", json_boolean(running));
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *j = json_object_get(rootJ, "running");
        if (j) running = json_boolean_value(j);
    }

    void process(const ProcessArgs &args) override {
        bool wasRunning = running;

        float swing = params[SWING_PARAM].getValue();
        if (inputs[SWING_INPUT].isConnected()) {
            swing += inputs[SWING_INPUT].getVoltage() / 10.f;
            swing = clamp(swing, 0.f, 1.f);
        }
        displaySwing.store(swing, std::memory_order_relaxed);

        if (runInputTrigger.process(inputs[RUN_INPUT].getVoltage()))
            running = !running;
        bool runButtonPressed = params[RUN_PARAM].getValue() > 0.5f;
        if (runButtonPressed && !prevRunButton)
            running = !running;
        prevRunButton = runButtonPressed;

        if (resetInputTrigger.process(inputs[RESET_INPUT].getVoltage()))
            doReset();
        bool resetButtonPressed = params[RESET_PARAM].getValue() > 0.5f;
        if (resetButtonPressed && !prevResetButton)
            doReset();
        prevResetButton = resetButtonPressed;

        if (!wasRunning && running) {
            phase = 0.f;
            beatCount = 0;
            prevBeatCount = 0;
            clkPulse.trigger(1e-3f);
            beatPulse.trigger(30e-3f);
            resetOutPhases();
            for (int i = 0; i < 6; i++)
                outPulse[i].trigger(1e-3f);
            displayPhase.store(0.f, std::memory_order_relaxed);
            displayBeatCount.store(0, std::memory_order_relaxed);
        }

        if (!running) {
            for (int i = 0; i < NUM_OUTPUTS; i++)
                outputs[i].setVoltage(0.f);
            lights[BEAT_LIGHT].setBrightnessSmooth(0.f, args.sampleTime);
            lights[RUN_LIGHT].setBrightnessSmooth(0.f, args.sampleTime);
            lights[RESET_LIGHT].setBrightnessSmooth(0.f, args.sampleTime);
            displayRunning.store(false, std::memory_order_relaxed);
            return;
        }

        lights[RUN_LIGHT].setBrightnessSmooth(1.f, args.sampleTime);

        float bpm = params[BPM_PARAM].getValue() * 333.f;
        bpm *= powf(2.f, inputs[V_OCT_INPUT].getVoltage());
        bpm = clamp(bpm, 0.1f, 333.f);
        float period = 60.f / bpm;
        float phaseInc = args.sampleTime / period;

        float prevPhase = phase;
        phase += phaseInc;
        if (phase >= 1.f) {
            phase -= 1.f;
            beatCount++;
        }
        bool wrapped = (phase < prevPhase);

        if (wrapped) {
            clkPulse.trigger(1e-3f);
            beatPulse.trigger(30e-3f);
        }

        // Ratio outputs: multipliers fire on their (swung) sub-beat grid,
        // divisions fire via their own phase accumulator.
        for (int i = 0; i < 6; i++) {
            int idx = ratioIndex(i);
            if (idx != lastRatioIdx[i]) {
                lastRatioIdx[i] = idx;
                divPhase[i] = 0.f;
            }
            float mult = RATIOS[idx].mult;
            if (mult < 1.f) {
                divPhase[i] += phaseInc * mult;
                if (divPhase[i] >= 1.f) {
                    divPhase[i] -= 1.f;
                    outPulse[i].trigger(1e-3f);
                }
            }
            else {
                int N = (int)lroundf(mult);
                if (wrapped)
                    outPulse[i].trigger(1e-3f);
                for (int k = 1; k < N; k++) {
                    // Odd pulse of each pair is the swung offbeat
                    float phi = ((float)k + (float)(k % 2) * swing) / (float)N;
                    if (prevPhase < phi && phase >= phi)
                        outPulse[i].trigger(1e-3f);
                }
            }
        }

        float delta = args.sampleTime;
        bool clkActive = clkPulse.process(delta);
        bool beatActive = beatPulse.process(delta);
        bool resetActive = resetPulse.process(delta);

        outputs[CLK_OUTPUT].setVoltage(clkActive ? 10.f : 0.f);
        for (int i = 0; i < 6; i++)
            outputs[OUT1_OUTPUT + i].setVoltage(outPulse[i].process(delta) ? 10.f : 0.f);

        lights[BEAT_LIGHT].setBrightnessSmooth(beatActive ? 1.f : 0.f, delta);
        lights[RESET_LIGHT].setBrightnessSmooth(resetActive ? 1.f : 0.f, delta);

        displayBpm.store(bpm, std::memory_order_relaxed);
        displayPhase.store(phase, std::memory_order_relaxed);
        displayBeatCount.store(beatCount, std::memory_order_relaxed);
        displayBeat.store(beatActive, std::memory_order_relaxed);
        displayRunning.store(true, std::memory_order_relaxed);
    }
};

struct KlockDisplay : Widget {
    RaKlockModule *module;
    std::shared_ptr<Font> font;
    Widget *knobBpm = NULL;
    Widget *knobSwing = NULL;
    Widget *knobRatio[6] = {};
    float brightness = 0.f;

    KlockDisplay() {
        font = APP->window->loadFont(asset::system("res/fonts/ShareTechMono-Regular.ttf"));
    }

    void draw(const DrawArgs &args) override {
        const float W = box.size.x;
        const float H = box.size.y;

        // Screen backdrop — painted slightly larger than the box to cover the
        // SVG bezel outline, recolored with a muted purple border to match the accent
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, -3, -3, W + 6, H + 6, 4);
        nvgFillColor(args.vg, nvgRGB(0x0b, 0x0a, 0x10));
        nvgFill(args.vg);
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStrokeColor(args.vg, nvgRGB(0x4a, 0x40, 0x66));
        nvgStroke(args.vg);

        if (!module) return;

        float bpm = module->displayBpm.load(std::memory_order_relaxed);
        float swing = module->displaySwing.load(std::memory_order_relaxed);
        float phase = module->displayPhase.load(std::memory_order_relaxed);
        int beatCount = module->displayBeatCount.load(std::memory_order_relaxed);
        bool beat = module->displayBeat.load(std::memory_order_relaxed);
        bool running = module->displayRunning.load(std::memory_order_relaxed);

        // Which control is being dragged right now, polled from Rack's event state
        Widget *dragged = APP->event ? APP->event->getDraggedWidget() : NULL;
        int activeOut = -1;
        for (int i = 0; i < 6; i++) {
            if (dragged && dragged == knobRatio[i]) {
                activeOut = i;
                break;
            }
        }
        bool bpmActive = (dragged != NULL && dragged == knobBpm);
        bool swingActive = (dragged != NULL && dragged == knobSwing);

        float target = (running && beat) ? 1.f : (running ? 0.2f : 0.06f);
        brightness += (target - brightness) * 0.15f;
        int alpha = (int)(brightness * 240 + 15);
        alpha = clamp(alpha, 0, 255);

        const float cx = W / 2.f;

        // ---- Clock case ----
        nvgBeginPath(args.vg);
        nvgRoundedRectVarying(args.vg, 16, 6, W - 32, H - 12, 40, 40, 10, 10);
        nvgFillColor(args.vg, nvgRGB(0x17, 0x13, 0x0d));
        nvgFill(args.vg);
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStrokeColor(args.vg, nvgRGB(0x6b, 0x57, 0x33));
        nvgStroke(args.vg);

        // ---- Dial face ----
        const float fy = 44.f;
        const float fr = 27.f;
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, cx, fy, fr);
        nvgFillColor(args.vg, nvgRGB(0x10, 0x10, 0x10));
        nvgFill(args.vg);
        nvgStrokeWidth(args.vg, 2.f);
        nvgStrokeColor(args.vg, beat ? nvgRGBA(0x99, 0x6d, 0xd2, clamp(alpha + 40, 0, 255))
                                     : nvgRGBA(0x8a, 0x6d, 0x3b, alpha));
        nvgStroke(args.vg);

        // Tick marks (12 o'clock-style)
        NVGcolor tickCol = nvgRGBA(0xd8, 0xc8, 0xa0, alpha);
        for (int i = 0; i < 12; i++) {
            float a = (float)i * (float)M_PI / 6.f;
            bool major = (i % 3 == 0);
            float r1 = major ? 16.5f : 18.5f;
            float r2 = major ? 23.5f : 21.5f;
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, cx + sinf(a) * r1, fy - cosf(a) * r1);
            nvgLineTo(args.vg, cx + sinf(a) * r2, fy - cosf(a) * r2);
            nvgStrokeWidth(args.vg, major ? 1.5f : 1.f);
            nvgStrokeColor(args.vg, tickCol);
            nvgStroke(args.vg);
        }

        // Hands — hour/minute count elapsed beats, second sweeps each beat
        auto hand = [&](float angle, float len, float width, NVGcolor col) {
            float ex = cx + sinf(angle) * len;
            float ey = fy - cosf(angle) * len;
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, cx, fy);
            nvgLineTo(args.vg, ex, ey);
            nvgStrokeWidth(args.vg, width);
            nvgStrokeColor(args.vg, col);
            nvgLineCap(args.vg, NVG_ROUND);
            nvgStroke(args.vg);
        };

        hand((float)(beatCount % 720) / 720.f * 2.f * (float)M_PI, 11.f, 2.2f, nvgRGBA(0xe6, 0xdc, 0xc0, alpha));
        hand((float)(beatCount % 60) / 60.f * 2.f * (float)M_PI, 16.f, 1.5f, nvgRGBA(0xe6, 0xdc, 0xc0, alpha));
        hand(phase * 2.f * (float)M_PI, 22.f, 1.f, nvgRGBA(0x99, 0x6d, 0xd2, clamp(alpha + 40, 0, 255)));

        // Hub
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, cx, fy, 2.2f);
        nvgFillColor(args.vg, nvgRGBA(0xe6, 0xdc, 0xc0, alpha));
        nvgFill(args.vg);

        // ---- Pendulum (metronome-style, oscillates back and forth per beat) ----
        // One full left-right oscillation per beat; the beat (phase 0/1) falls at the
        // extremes, alternating sides each beat like a tick-tock.
        float ang = running ? 0.5f * sinf(2.f * (float)M_PI * (phase - 0.25f)) : 0.f;
        float px = cx + sinf(ang) * 26.f;
        float py = 78.f + cosf(ang) * 26.f;

        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, cx, 78.f);
        nvgLineTo(args.vg, px, py);
        nvgStrokeWidth(args.vg, 2.f);
        nvgStrokeColor(args.vg, nvgRGBA(0x6b, 0x57, 0x33, alpha));
        nvgStroke(args.vg);

        nvgBeginPath(args.vg);
        nvgCircle(args.vg, cx, 78.f, 3.f);
        nvgFillColor(args.vg, nvgRGBA(0x8a, 0x6d, 0x3b, alpha));
        nvgFill(args.vg);

        nvgBeginPath(args.vg);
        nvgCircle(args.vg, px, py, 9.f);
        nvgFillColor(args.vg, nvgRGBA(0x99, 0x6d, 0xd2, clamp(alpha + 30, 0, 255)));
        nvgFill(args.vg);
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, px, py, 3.6f);
        nvgFillColor(args.vg, nvgRGBA(0x24, 0x15, 0x40, alpha));
        nvgFill(args.vg);

        // ---- Readout — BPM/out value right, swing left ----
        if (font) {
            nvgFontFaceId(args.vg, font->handle);
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

            char cap[8];
            char val[8];
            bool ratioActive = (activeOut >= 0 && activeOut < 6);
            if (ratioActive) {
                int rIdx = clamp((int)lroundf(module->params[RaKlockModule::OUT1_PARAM + activeOut].getValue()), 0, NUM_RATIOS - 1);
                snprintf(cap, sizeof(cap), "OUT %d", activeOut + 1);
                snprintf(val, sizeof(val), "%s", RATIOS[rIdx].label);
            }
            else {
                snprintf(cap, sizeof(cap), "BPM");
                if (running)
                    snprintf(val, sizeof(val), "%.0f", bpm);
                else
                    snprintf(val, sizeof(val), "---");
            }

            if (ratioActive) {
                // Ratio readout — larger and steady full brightness while dragging, no beat flash
                nvgFontSize(args.vg, 9.f);
                nvgFillColor(args.vg, nvgRGBA(0x99, 0x6d, 0xd2, 255));
                nvgText(args.vg, 44.f, 83.f, cap, NULL);
                nvgFontSize(args.vg, 22.f);
                nvgFillColor(args.vg, nvgRGBA(0x99, 0x6d, 0xd2, 255));
                nvgText(args.vg, 44.f, 103.f, val, NULL);
            }
            else {
                // BPM — steady full brightness while the BPM knob is dragged, otherwise beat-pulsed
                int a = bpmActive ? 255 : clamp((int)(alpha * 1.7f), 40, 255);
                nvgFontSize(args.vg, 10.f);
                nvgFillColor(args.vg, nvgRGBA(0x99, 0x6d, 0xd2, a));
                nvgText(args.vg, 44.f, 82.f, cap, NULL);
                nvgFontSize(args.vg, 22.f);
                nvgFillColor(args.vg, nvgRGBA(0x99, 0x6d, 0xd2, a));
                nvgText(args.vg, 44.f, 100.f, val, NULL);
            }

            // Swing — steady full brightness while the Swing knob is dragged, otherwise beat-pulsed
            int swAlpha = swingActive ? 255 : clamp((int)(alpha * 1.7f), 30, 255);
            char swingText[8];
            snprintf(swingText, sizeof(swingText), "%d%%", (int)(swing * 100.f));
            nvgFontSize(args.vg, 10.f);
            nvgFillColor(args.vg, nvgRGBA(0x99, 0x6d, 0xd2, swAlpha));
            nvgText(args.vg, 148.f, 82.f, "SWG", NULL);
            nvgFontSize(args.vg, 22.f);
            nvgFillColor(args.vg, nvgRGBA(0x99, 0x6d, 0xd2, swAlpha));
            nvgText(args.vg, 148.f, 100.f, swingText, NULL);
        }
    }
};

struct PurpleLight : GrayModuleLightWidget {
    PurpleLight() {
        addBaseColor(nvgRGB(0x99, 0x6d, 0xd2));
    }
};

struct RaKlockWidget : ModuleWidget {
    RaKlockWidget(RaKlockModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-klock.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        auto *display = new KlockDisplay();
        display->box.pos = Vec(24, 24);
        display->box.size = Vec(192, 124);
        display->module = module;
        addChild(display);

        // Controls — row 1
        addInput(createInputCentered<RaPort>(Vec(30, 172), module, RaKlockModule::V_OCT_INPUT));
        addParam(createParamCentered<RaKnobLarge>(Vec(95, 172), module, RaKlockModule::BPM_PARAM));
        addChild(createLightCentered<LargeLight<PurpleLight>>(Vec(150, 172), module, RaKlockModule::BEAT_LIGHT));
        addInput(createInputCentered<RaPort>(Vec(180, 172), module, RaKlockModule::SWING_INPUT));
        addParam(createParamCentered<RaKnob>(Vec(214, 172), module, RaKlockModule::SWING_PARAM));

        // Controls — row 2: run/reset + main CLK output
        addInput(createInputCentered<RaPort>(Vec(30, 230), module, RaKlockModule::RUN_INPUT));
        addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(85, 230), module, RaKlockModule::RUN_PARAM, RaKlockModule::RUN_LIGHT));
        addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(135, 230), module, RaKlockModule::RESET_PARAM, RaKlockModule::RESET_LIGHT));
        addInput(createInputCentered<RaPort>(Vec(175, 230), module, RaKlockModule::RESET_INPUT));
        addOutput(createOutputCentered<RaPort>(Vec(210, 230), module, RaKlockModule::CLK_OUTPUT));

        // Ratio outputs — 6 knobs with jacks below
        // Out 1
        addParam(createParamCentered<RaKnob>(Vec(24, 292), module, RaKlockModule::OUT1_PARAM));
        addOutput(createOutputCentered<RaPort>(Vec(24, 348), module, RaKlockModule::OUT1_OUTPUT));

        // Out 2
        addParam(createParamCentered<RaKnob>(Vec(64, 292), module, RaKlockModule::OUT2_PARAM));
        addOutput(createOutputCentered<RaPort>(Vec(64, 348), module, RaKlockModule::OUT2_OUTPUT));

        // Out 3
        addParam(createParamCentered<RaKnob>(Vec(104, 292), module, RaKlockModule::OUT3_PARAM));
        addOutput(createOutputCentered<RaPort>(Vec(104, 348), module, RaKlockModule::OUT3_OUTPUT));

        // Out 4
        addParam(createParamCentered<RaKnob>(Vec(144, 292), module, RaKlockModule::OUT4_PARAM));
        addOutput(createOutputCentered<RaPort>(Vec(144, 348), module, RaKlockModule::OUT4_OUTPUT));

        // Out 5
        addParam(createParamCentered<RaKnob>(Vec(184, 292), module, RaKlockModule::OUT5_PARAM));
        addOutput(createOutputCentered<RaPort>(Vec(184, 348), module, RaKlockModule::OUT5_OUTPUT));

        // Out 6
        addParam(createParamCentered<RaKnob>(Vec(216, 292), module, RaKlockModule::OUT6_PARAM));
        addOutput(createOutputCentered<RaPort>(Vec(216, 348), module, RaKlockModule::OUT6_OUTPUT));

        // Point the display at the knobs so it can detect drags via Rack's event state
        display->knobBpm = getParam(RaKlockModule::BPM_PARAM);
        display->knobSwing = getParam(RaKlockModule::SWING_PARAM);
        for (int i = 0; i < 6; i++)
            display->knobRatio[i] = getParam(RaKlockModule::OUT1_PARAM + i);
    }
};

Model *modelRaKlock = createModel<RaKlockModule, RaKlockWidget>("ra-klock");