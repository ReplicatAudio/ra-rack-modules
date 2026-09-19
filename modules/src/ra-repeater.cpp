// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: TRIG_INPUT "In"
// fname: CLOCK_INPUT "Clk"
// fname: DELAY1_PARAM "Del"
// fname: DELAY1_INPUT "Del CV"
// fname: CHANCE1_PARAM "Chance"
// fname: CHANCE1_INPUT "Cha CV"
// fname: OUT1 "Out"
// fname: GLOBAL_OUTPUT "All"
// fname: MODE_PARAM "Mode"
// fname: TRIGGER_BUTTON_PARAM "push"
#include "ra-components.hpp"
#include <atomic>

using namespace rack;

extern Plugin *pluginInstance;

struct RaRepeaterModule : Module {
    enum ParamIds {
        DELAY1_PARAM,
        DELAY2_PARAM,
        DELAY3_PARAM,
        DELAY4_PARAM,
        CHANCE1_PARAM,
        CHANCE2_PARAM,
        CHANCE3_PARAM,
        CHANCE4_PARAM,
        MODE_PARAM,
        TRIGGER_BUTTON_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        TRIG_INPUT,
        CLOCK_INPUT,
        DELAY1_INPUT,
        DELAY2_INPUT,
        DELAY3_INPUT,
        DELAY4_INPUT,
        CHANCE1_INPUT,
        CHANCE2_INPUT,
        CHANCE3_INPUT,
        CHANCE4_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT1,
        OUT2,
        OUT3,
        OUT4,
        GLOBAL_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        LIGHT1_R,
        LIGHT1_G,
        LIGHT1_B,
        LIGHT2_R,
        LIGHT2_G,
        LIGHT2_B,
        LIGHT3_R,
        LIGHT3_G,
        LIGHT3_B,
        LIGHT4_R,
        LIGHT4_G,
        LIGHT4_B,
        NUM_LIGHTS
    };

    // Retrigger each time the input crosses the trigger threshold.
    dsp::SchmittTrigger inputTrig;
    // Manual trigger button edge detector.
    dsp::SchmittTrigger buttonTrig;
    // One short pulse generator per repeat output.
    dsp::PulseGenerator outPulses[4];
    // Samples remaining until each repeat fires; -1 = not scheduled.
    float timers[4];
    // Clock sync: measured period of the clock input when connected.
    dsp::Timer clockTimer;
    dsp::SchmittTrigger clockTrigger;
    float clockFreq = 1.f;
    // Output LED brightness, peaks on fire and decays to 0.
    float ledBrightness[4];
    // Last-seen delay knob value per output, used to spot the last-moved knob.
    float lastDelayValue[4];
    // Screen value: delay in ms of the most recently moved delay knob.
    std::atomic<int> displayMs;

    RaRepeaterModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configInput(TRIG_INPUT, "Trigger");
        configInput(CLOCK_INPUT, "Clock");
        for (int i = 0; i < 4; i++) {
            configParam(DELAY1_PARAM + i, 0.f, 1.f, 0.f, string::f("Delay %d", i + 1), " ms", 0.f, 1000.f, 0.f);
            configParam(CHANCE1_PARAM + i, 0.f, 1.f, 1.f, string::f("Chance %d", i + 1), "%", 0.f, 100.f);
            configInput(DELAY1_INPUT + i, string::f("Delay %d CV", i + 1));
            configInput(CHANCE1_INPUT + i, string::f("Chance %d CV", i + 1));
            configOutput(OUT1 + i, string::f("Repeat %d", i + 1));
            configLight(LIGHT1_R + i * 3, string::f("Light %d R", i + 1));
            configLight(LIGHT1_G + i * 3, string::f("Light %d G", i + 1));
            configLight(LIGHT1_B + i * 3, string::f("Light %d B", i + 1));
            timers[i] = -1.f;
            ledBrightness[i] = 0.f;
            lastDelayValue[i] = 0.f;
        }
        configSwitch(MODE_PARAM, 0.f, 1.f, 0.f, "Mode", {"Passthrough", "No pass"});
        configButton(TRIGGER_BUTTON_PARAM, "Manual");
        configOutput(GLOBAL_OUTPUT, "All");
        displayMs.store(0, std::memory_order_relaxed);
    }

    void onReset() override {
        for (int i = 0; i < 4; i++) {
            timers[i] = -1.f;
            ledBrightness[i] = 0.f;
            lastDelayValue[i] = 0.f;
        }
        clockFreq = 1.f;
        displayMs.store(0, std::memory_order_relaxed);
    }

    void process(const ProcessArgs &args) override {
        bool edge = inputTrig.process(inputs[TRIG_INPUT].getVoltage())
            || buttonTrig.process(params[TRIGGER_BUTTON_PARAM].getValue());
        int mode = (int)std::round(params[MODE_PARAM].getValue());

        float globalOut = 0.f;

        bool clocked = inputs[CLOCK_INPUT].isConnected();

        // Clock sync: measure the period from the clock input when connected.
        if (clocked) {
            clockTimer.process(args.sampleTime);
            if (clockTrigger.process(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 2.f)) {
                float f = 1.f / clockTimer.getTime();
                clockTimer.reset();
                if (0.001f <= f && f <= 1000.f)
                    clockFreq = f;
            }
        }
        else {
            clockFreq = 1.f;
        }

        // Delay time max: the clock period when synced, otherwise 1 s.
        float delayMax = clocked ? 1.f / clockFreq : 1.f;

        for (int i = 0; i < 4; i++) {
            // Delay time: the clock period divided by the knob position when
            // clocked; otherwise knob 0-1s plus CV (5 V = 1 s), clamped 0-1 s.
            float delayT;
            if (clocked) {
                delayT = clamp(params[DELAY1_PARAM + i].getValue(), 0.f, 1.f) * delayMax;
            } else {
                delayT = params[DELAY1_PARAM + i].getValue() + inputs[DELAY1_INPUT + i].getVoltage() / 5.f;
                delayT = clamp(delayT, 0.f, 1.f);
            }

            // Track the most recently moved delay knob for the screen readout.
            float knobV = params[DELAY1_PARAM + i].getValue();
            if (knobV != lastDelayValue[i]) {
                lastDelayValue[i] = knobV;
                displayMs.store((int)std::lround(clamp(knobV, 0.f, 1.f) * 1000.f), std::memory_order_relaxed);
            }

            // Chance: knob 0-100% plus CV (10 V = 100%), clamped 0-100%.
            float chance = params[CHANCE1_PARAM + i].getValue() + inputs[CHANCE1_INPUT + i].getVoltage() / 10.f;
            chance = clamp(chance, 0.f, 1.f);

            if (edge) {
                // Roll the chance for this output on each incoming trigger.
                if (random::uniform() < chance) {
                    long delaySamples = (long)(delayT / args.sampleTime);
                    if (delaySamples <= 0) {
                        outPulses[i].trigger(1e-3f);
                        ledBrightness[i] = 1.f;
                    } else
                        timers[i] = (float)delaySamples;
                }
            } else if (timers[i] > 0.f) {
                // Count down the scheduled repeat to its fire moment.
                timers[i] -= 1.f;
                if (timers[i] <= 0.f) {
                    outPulses[i].trigger(1e-3f);
                    ledBrightness[i] = 1.f;
                    timers[i] = -1.f;
                }
            }

            float out = outPulses[i].process(args.sampleTime) ? 10.f : 0.f;
            outputs[OUT1 + i].setVoltage(out);
            globalOut += out;

            // Fade out: bright on fire, then dims to off.
            ledBrightness[i] *= expf(-args.sampleTime * 6.f);

            // Purple color scaled by the fading brightness; off when fully faded.
            float level = ledBrightness[i];
            float r = 0.8f * level;
            float g = 0.1f * level;
            float bl = 1.0f * level;
            lights[LIGHT1_R + i * 3 + 0].setBrightness(r);
            lights[LIGHT1_R + i * 3 + 1].setBrightness(g);
            lights[LIGHT1_R + i * 3 + 2].setBrightness(bl);
        }

        // Global output: all repeats, plus the raw input when in passthrough mode.
        float global = globalOut;
        if (mode == 0)
            global += inputs[TRIG_INPUT].getVoltage();
        outputs[GLOBAL_OUTPUT].setVoltage(clamp(global, -10.f, 10.f));
    }
};

// Screen showing the most recently moved delay knob's value in milliseconds.
struct RepeaterTimeDisplay : LedDisplay {
	RaRepeaterModule *module;
	std::shared_ptr<Font> font;

	RepeaterTimeDisplay() {
		font = APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
	}

	void draw(const DrawArgs &args) override {
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, -3, -3, box.size.x + 6, box.size.y + 6, 4);
		nvgFillColor(args.vg, nvgRGB(0x10, 0x10, 0x10));
		nvgFill(args.vg);
		nvgStrokeWidth(args.vg, 1.5f);
		nvgStrokeColor(args.vg, nvgRGB(0x4a, 0x40, 0x66));
		nvgStroke(args.vg);

		if (!module || !font) return;

		int ms = module->displayMs.load(std::memory_order_relaxed);

		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, 14);
		nvgFillColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

		char buf[16];
		snprintf(buf, sizeof(buf), "%d", ms);
		nvgText(args.vg, box.size.x / 2, box.size.y / 2, buf, NULL);
	}
};

struct RaRepeaterWidget : ModuleWidget {
    RaRepeaterWidget(RaRepeaterModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-repeater.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        // Column centres: delay knob | delay CV | chance knob | chance CV | output
        float xs[5] = {30.f, 60.f, 90.f, 120.f, 150.f};

        // Clock input to the left of the trigger input, both near the top
        addInput(createInputCentered<RaPort>(Vec(xs[1], 48), module, RaRepeaterModule::CLOCK_INPUT));
        addInput(createInputCentered<RaPort>(Vec(xs[2], 48), module, RaRepeaterModule::TRIG_INPUT));

        // Manual trigger button above the first delay knob
        addParam(createParamCentered<RaButton>(Vec(xs[0], 48), module, RaRepeaterModule::TRIGGER_BUTTON_PARAM));

        // Four repeat rows
        float ys[4] = {80.f, 140.f, 200.f, 260.f};
        for (int i = 0; i < 4; i++) {
            addParam(createParamCentered<RaKnobTrim>(Vec(xs[0], ys[i]), module, RaRepeaterModule::DELAY1_PARAM + i));
            addInput(createInputCentered<RaPort>(Vec(xs[1], ys[i]), module, RaRepeaterModule::DELAY1_INPUT + i));
            addParam(createParamCentered<RaKnobTrim>(Vec(xs[2], ys[i]), module, RaRepeaterModule::CHANCE1_PARAM + i));
            addInput(createInputCentered<RaPort>(Vec(xs[3], ys[i]), module, RaRepeaterModule::CHANCE1_INPUT + i));
            addOutput(createOutputCentered<RaPort>(Vec(xs[4], ys[i]), module, RaRepeaterModule::OUT1 + i));
            addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(Vec(xs[4], ys[i] - 23.f), module, RaRepeaterModule::LIGHT1_R + i * 3));
        }

        // Global output and mode switch at the bottom
        addOutput(createOutputCentered<RaPort>(Vec(xs[2], 330), module, RaRepeaterModule::GLOBAL_OUTPUT));
        addParam(createParamCentered<RaSwitch2>(Vec(xs[4], 330), module, RaRepeaterModule::MODE_PARAM));

        // Screen to the left of the global output jack, left edge aligned with the delay button column
        RepeaterTimeDisplay *disp = createWidget<RepeaterTimeDisplay>(Vec(30.f, 315.f));
        disp->box.size = Vec(30.f, 30.f);
        disp->module = module;
        addChild(disp);
    }
};

Model *modelRaRepeater = createModel<RaRepeaterModule, RaRepeaterWidget>("ra-repeater");