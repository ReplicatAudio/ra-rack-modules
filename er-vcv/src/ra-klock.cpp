#include "ra-widgets.hpp"
#include <atomic>

using namespace rack;

extern Plugin *pluginInstance;

struct RaKlockModule : Module {
    enum ParamIds {
        BPM_PARAM,
        SWING_PARAM,
        RUN_PARAM,
        RESET_PARAM,
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
        RUN_OUTPUT,
        RESET_OUTPUT,
        X2_OUTPUT,
        X4_OUTPUT,
        X8_OUTPUT,
        DIV2_OUTPUT,
        DIV4_OUTPUT,
        DIV8_OUTPUT,
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
    int tick = 0;
    int beatCount = 0;
    int prevTick = 0;
    int prevBeatCount = 0;

    dsp::PulseGenerator clkPulse;
    dsp::PulseGenerator x2Pulse;
    dsp::PulseGenerator x4Pulse;
    dsp::PulseGenerator x8Pulse;
    dsp::PulseGenerator div2Pulse;
    dsp::PulseGenerator div4Pulse;
    dsp::PulseGenerator div8Pulse;
    dsp::PulseGenerator resetPulse;
    dsp::PulseGenerator beatPulse;

    std::atomic<float> displayBpm{0.f};
    std::atomic<float> displaySwing{0.f};
    std::atomic<bool> displayBeat{false};
    std::atomic<bool> displayRunning{false};

    RaKlockModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(BPM_PARAM, 0.f, 1.f, 0.5f, "BPM", " BPM", 0.f, 333.f, 0.f);
        configParam(SWING_PARAM, 0.f, 1.f, 0.f, "Swing", "%", 0.f, 100.f);
        configButton(RUN_PARAM, "Run");
        configButton(RESET_PARAM, "Reset");
        configInput(V_OCT_INPUT, "v/oct");
        configInput(SWING_INPUT, "Swing CV");
        configInput(RUN_INPUT, "Run");
        configInput(RESET_INPUT, "Reset");
        configOutput(CLK_OUTPUT, "CLK");
        configOutput(RUN_OUTPUT, "Run");
        configOutput(RESET_OUTPUT, "Reset");
        configOutput(X2_OUTPUT, "x2");
        configOutput(X4_OUTPUT, "x4");
        configOutput(X8_OUTPUT, "x8");
        configOutput(DIV2_OUTPUT, "/2");
        configOutput(DIV4_OUTPUT, "/4");
        configOutput(DIV8_OUTPUT, "/8");
        configLight(BEAT_LIGHT, "Beat");
        configLight(RUN_LIGHT, "Run");
        configLight(RESET_LIGHT, "Reset");
    }

    void doReset() {
        phase = 0.f;
        tick = 0;
        prevTick = 0;
        beatCount = 0;
        prevBeatCount = 0;
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

    int tickFromPhase(float phase, float swing) {
        float onbeatRatio = 0.5f + swing * 0.3f;
        float offbeatRatio = 0.5f - swing * 0.3f;
        float onbeatStep = (offbeatRatio > 0.f) ? (onbeatRatio / 4.f) : 0.125f;
        float offbeatStep = (offbeatRatio > 0.f) ? (offbeatRatio / 4.f) : 0.125f;
        if (phase < onbeatRatio)
            return (int)(phase / onbeatStep);
        else
            return 4 + (int)((phase - onbeatRatio) / offbeatStep);
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
            tick = 0;
            prevTick = 0;
            beatCount = 0;
            prevBeatCount = 0;
            clkPulse.trigger(1e-3f);
            x2Pulse.trigger(1e-3f);
            x4Pulse.trigger(1e-3f);
            x8Pulse.trigger(1e-3f);
            div2Pulse.trigger(1e-3f);
            div4Pulse.trigger(1e-3f);
            div8Pulse.trigger(1e-3f);
            beatPulse.trigger(50e-3f);
        }

        if (!running) {
            outputs[CLK_OUTPUT].setVoltage(0.f);
            outputs[X2_OUTPUT].setVoltage(0.f);
            outputs[X4_OUTPUT].setVoltage(0.f);
            outputs[X8_OUTPUT].setVoltage(0.f);
            outputs[DIV2_OUTPUT].setVoltage(0.f);
            outputs[DIV4_OUTPUT].setVoltage(0.f);
            outputs[DIV8_OUTPUT].setVoltage(0.f);
            outputs[RUN_OUTPUT].setVoltage(0.f);
            lights[BEAT_LIGHT].setBrightnessSmooth(0.f, args.sampleTime);
            lights[RUN_LIGHT].setBrightnessSmooth(0.f, args.sampleTime);
            displayRunning.store(false, std::memory_order_relaxed);
            return;
        }

        outputs[RUN_OUTPUT].setVoltage(10.f);
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

        prevTick = tick;
        tick = clamp(tickFromPhase(phase, swing), 0, 7);

        bool tickChanged = (tick != prevTick);
        bool wrapped = (phase < prevPhase);
        if (tickChanged || wrapped) {
            x8Pulse.trigger(1e-3f);
            if (tick % 2 == 0)
                x4Pulse.trigger(1e-3f);
            if (tick == 0 || tick == 4)
                x2Pulse.trigger(1e-3f);
            if (tick == 0) {
                clkPulse.trigger(1e-3f);
                beatPulse.trigger(50e-3f);
            }
        }

        if (beatCount != prevBeatCount) {
            if (beatCount % 2 == 0)
                div2Pulse.trigger(1e-3f);
            if (beatCount % 4 == 0)
                div4Pulse.trigger(1e-3f);
            if (beatCount % 8 == 0)
                div8Pulse.trigger(1e-3f);
            prevBeatCount = beatCount;
        }

        float delta = args.sampleTime;
        bool clkActive = clkPulse.process(delta);
        bool x2Active = x2Pulse.process(delta);
        bool x4Active = x4Pulse.process(delta);
        bool x8Active = x8Pulse.process(delta);
        bool div2Active = div2Pulse.process(delta);
        bool div4Active = div4Pulse.process(delta);
        bool div8Active = div8Pulse.process(delta);
        bool resetActive = resetPulse.process(delta);
        bool beatActive = beatPulse.process(delta);

        outputs[CLK_OUTPUT].setVoltage(clkActive ? 10.f : 0.f);
        outputs[X2_OUTPUT].setVoltage(x2Active ? 10.f : 0.f);
        outputs[X4_OUTPUT].setVoltage(x4Active ? 10.f : 0.f);
        outputs[X8_OUTPUT].setVoltage(x8Active ? 10.f : 0.f);
        outputs[DIV2_OUTPUT].setVoltage(div2Active ? 10.f : 0.f);
        outputs[DIV4_OUTPUT].setVoltage(div4Active ? 10.f : 0.f);
        outputs[DIV8_OUTPUT].setVoltage(div8Active ? 10.f : 0.f);
        outputs[RESET_OUTPUT].setVoltage(resetActive ? 10.f : 0.f);

        lights[BEAT_LIGHT].setBrightnessSmooth(beatActive ? 1.f : 0.f, delta);
        lights[RESET_LIGHT].setBrightnessSmooth(resetActive ? 1.f : 0.f, delta);

        displayBpm.store(bpm, std::memory_order_relaxed);
        displayBeat.store(beatActive, std::memory_order_relaxed);
        displayRunning.store(true, std::memory_order_relaxed);
    }
};

struct BpmDisplay : Widget {
    RaKlockModule *module;
    float brightness = 0.f;

    void draw(const DrawArgs &args) override {
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
        nvgFillColor(args.vg, nvgRGB(0x0a, 0x0a, 0x0a));
        nvgFill(args.vg);

        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
        nvgStrokeWidth(args.vg, 1.f);
        nvgStrokeColor(args.vg, nvgRGB(0x33, 0x33, 0x33));
        nvgStroke(args.vg);

        if (!module) return;

        float bpm = module->displayBpm.load(std::memory_order_relaxed);
        float swing = module->displaySwing.load(std::memory_order_relaxed);
        bool beat = module->displayBeat.load(std::memory_order_relaxed);
        bool running = module->displayRunning.load(std::memory_order_relaxed);

        float target = (running && beat) ? 1.f : (running ? 0.25f : 0.08f);
        brightness += (target - brightness) * 0.15f;

        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

        int alpha = (int)(brightness * 240 + 15);
        alpha = clamp(alpha, 0, 255);
        NVGcolor col = nvgRGBA(0xff, 0xcc, 0x00, alpha);

        char bpmText[8];
        if (running)
            snprintf(bpmText, sizeof(bpmText), "%.0f", bpm);
        else
            snprintf(bpmText, sizeof(bpmText), "---");

        nvgFontSize(args.vg, 14);
        nvgFillColor(args.vg, col);
        nvgText(args.vg, box.size.x / 2, 16, bpmText, NULL);

        char swingText[8];
        snprintf(swingText, sizeof(swingText), "S%d", (int)(swing * 100.f));

        nvgFontSize(args.vg, 10);
        nvgFillColor(args.vg, nvgRGBA(0xff, 0xcc, 0x00, alpha));
        nvgText(args.vg, box.size.x / 2, 32, swingText, NULL);
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

        addInput(createInputCentered<RaPort>(Vec(22, 40), module, RaKlockModule::V_OCT_INPUT));
        addParam(createParamCentered<RaKnobLarge>(Vec(75, 40), module, RaKlockModule::BPM_PARAM));
        addChild(createLightCentered<TinyLight<RedLight>>(Vec(128, 40), module, RaKlockModule::BEAT_LIGHT));

        addInput(createInputCentered<RaPort>(Vec(22, 118), module, RaKlockModule::SWING_INPUT));

        auto *display = new BpmDisplay();
        display->box.pos = Vec(97, 98);
        display->box.size = Vec(30, 40);
        display->module = module;
        addChild(display);
        addParam(createParamCentered<RaKnob>(Vec(75, 118), module, RaKlockModule::SWING_PARAM));

        addInput(createInputCentered<RaPort>(Vec(22, 174), module, RaKlockModule::RUN_INPUT));
        addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(62, 174), module, RaKlockModule::RUN_PARAM, RaKlockModule::RUN_LIGHT));
        addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(95, 174), module, RaKlockModule::RESET_PARAM, RaKlockModule::RESET_LIGHT));
        addInput(createInputCentered<RaPort>(Vec(128, 174), module, RaKlockModule::RESET_INPUT));

        addOutput(createOutputCentered<RaPort>(Vec(25, 232), module, RaKlockModule::CLK_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(75, 232), module, RaKlockModule::RUN_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(125, 232), module, RaKlockModule::RESET_OUTPUT));

        addOutput(createOutputCentered<RaPort>(Vec(25, 277), module, RaKlockModule::X2_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(75, 277), module, RaKlockModule::X4_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(125, 277), module, RaKlockModule::X8_OUTPUT));

        addOutput(createOutputCentered<RaPort>(Vec(25, 322), module, RaKlockModule::DIV2_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(75, 322), module, RaKlockModule::DIV4_OUTPUT));
        addOutput(createOutputCentered<RaPort>(Vec(125, 322), module, RaKlockModule::DIV8_OUTPUT));
    }
};

Model *modelRaKlock = createModel<RaKlockModule, RaKlockWidget>("ra-klock");
