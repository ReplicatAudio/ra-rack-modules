// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: TRIGGER_BUTTON_PARAM "push"
// fname: TRIG_INPUT "In"
// fname: TIME_PARAM "Step"
// fname: CLOCK_INPUT "Clock"
// fname: MODE_PARAM "Mode"
// fname: OUT1_OUTPUT "Out"
#include "ra-components.hpp"
#include <atomic>

using namespace rack;

extern Plugin *pluginInstance;

static const NVGcolor BUMP_PURPLE = nvgRGB(0x99, 0x6d, 0xd2);
static const NVGcolor BUMP_WHITE = nvgRGB(0xee, 0xee, 0xee);

struct RaBumpModule : Module {
    enum ParamIds {
        TIME_PARAM,
        MODE_PARAM,
        TRIGGER_BUTTON_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        TRIG_INPUT,
        CLOCK_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT1_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    bool steps[8] = {};
    int stepIndex = 0;
    bool steppingActive = false;
    float stepTimer = 0.f;
    float clockFreq = 1.f;
    std::atomic<int> displayMs;

    dsp::Timer clockTimer;
    dsp::SchmittTrigger clockTrigger;
    dsp::SchmittTrigger inputTrig;
    dsp::SchmittTrigger buttonTrig;
    dsp::PulseGenerator pulse;

    RaBumpModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(TIME_PARAM, 0.f, 1.f, 0.25f, "Step time", " ms", 0.f, 1000.f, 0.f);
        configSwitch(MODE_PARAM, 0.f, 1.f, 0.f, "Mode", {"Passthrough", "No pass"});
        configButton(TRIGGER_BUTTON_PARAM, "Push");
        configInput(TRIG_INPUT, "Trigger");
        configInput(CLOCK_INPUT, "Clock");
        configOutput(OUT1_OUTPUT, "Output");
        for (int i = 0; i < 8; i++)
            steps[i] = true;
    }

    void onReset() override {
        for (int i = 0; i < 8; i++)
            steps[i] = true;
        stepIndex = 0;
        steppingActive = false;
        stepTimer = 0.f;
        clockFreq = 1.f;
        displayMs.store(0, std::memory_order_relaxed);
    }

    bool isStepSet(int i) {
        return steps[i];
    }

    void toggleStep(int i) {
        steps[i] ^= true;
    }

    // Current playhead position, or -1 when the sequence is not stepping.
    int currentStep() {
        return steppingActive ? stepIndex : -1;
    }

    void fireStep(int i) {
        if (steps[i])
            pulse.trigger(1e-3f);
    }

    void process(const ProcessArgs &args) override {
        bool edge = inputTrig.process(inputs[TRIG_INPUT].getVoltage())
            || buttonTrig.process(params[TRIGGER_BUTTON_PARAM].getValue());
        int mode = (int)std::round(params[MODE_PARAM].getValue());

        // Screen shows the step time knob's value in ms.
        displayMs.store((int)std::lround(clamp(params[TIME_PARAM].getValue(), 0.f, 1.f) * 1000.f), std::memory_order_relaxed);

        // Clock sync: measure the period from the clock input when connected.
        if (inputs[CLOCK_INPUT].isConnected()) {
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

        // Step duration: the clock period when synced, otherwise the knob (0-1s).
        float stepDur = inputs[CLOCK_INPUT].isConnected()
            ? 1.f / clockFreq
            : clamp(params[TIME_PARAM].getValue(), 0.001f, 1.f);

        // Retrigger: start stepping from step 0 and play it immediately.
        if (edge) {
            stepIndex = 0;
            stepTimer = 0.f;
            steppingActive = true;
            fireStep(0);
        }

        // Advance through the remaining steps at the step rate.
        if (steppingActive) {
            stepTimer += args.sampleTime;
            while (steppingActive && stepTimer >= stepDur) {
                stepTimer -= stepDur;
                stepIndex++;
                if (stepIndex >= 8) {
                    steppingActive = false;
                    break;
                }
                fireStep(stepIndex);
            }
        }

        float out = pulse.process(args.sampleTime) ? 10.f : 0.f;
        if (mode == 0)
            out += inputs[TRIG_INPUT].getVoltage();
        outputs[OUT1_OUTPUT].setVoltage(clamp(out, -10.f, 10.f));
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        json_t *stepsJ = json_array();
        for (int i = 0; i < 8; i++)
            json_array_append_new(stepsJ, json_boolean(steps[i]));
        json_object_set_new(rootJ, "steps", stepsJ);
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *stepsJ = json_object_get(rootJ, "steps");
        if (stepsJ) {
            for (int i = 0; i < 8; i++) {
                json_t *s = json_array_get(stepsJ, i);
                if (s)
                    steps[i] = json_boolean_value(s);
            }
        }
    }
};

// One step of the vertical sequence — toggled on click, lit when set, with a
// halo when it is the currently-playing step.
struct BumpStepButton : OpaqueWidget {
    int index;
    RaBumpModule *module;

    void draw(const DrawArgs &args) override {
        bool set = module && module->isStepSet(index);
        bool current = module && module->currentStep() == index;
        Rect r = box.zeroPos();

        if (current) {
            // Bright, clearly visible outline around the active step.
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, RECT_ARGS(r.grow(Vec(2, 2))), 2.5f);
            nvgStrokeWidth(args.vg, 2.f);
            nvgStrokeColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
            nvgStroke(args.vg);
        }

        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, RECT_ARGS(r), 1.5f);
        nvgFillColor(args.vg, set ? BUMP_WHITE : BUMP_PURPLE);
        nvgFill(args.vg);
    }

    void onDragStart(const event::DragStart &e) override {
        if (e.button == GLFW_MOUSE_BUTTON_LEFT && module)
            module->toggleStep(index);
        OpaqueWidget::onDragStart(e);
    }

    void onDragEnter(const event::DragEnter &e) override {
        if (e.button == GLFW_MOUSE_BUTTON_LEFT && module) {
            BumpStepButton *origin = dynamic_cast<BumpStepButton *>(e.origin);
            if (origin && origin->module) {
                bool v = origin->module->isStepSet(origin->index);
                if (v != module->isStepSet(index))
                    module->toggleStep(index);
            }
        }
        OpaqueWidget::onDragEnter(e);
    }
};

struct BumpTimeDisplay : LedDisplay {
	RaBumpModule *module;
	std::shared_ptr<Font> font;

	BumpTimeDisplay() {
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

struct RaBumpWidget : ModuleWidget {
    RaBumpWidget(RaBumpModule *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-bump.svg")));

        addChild(createWidget<RaScrew>(Vec(0, 0)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
        addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
        addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

        // Push button + trigger input across the top
        addParam(createParamCentered<RaButton>(Vec(30, 45), module, RaBumpModule::TRIGGER_BUTTON_PARAM));
        addInput(createInputCentered<RaPort>(Vec(90, 45), module, RaBumpModule::TRIG_INPUT));

        // Step time knob + clock input
        addParam(createParamCentered<RaKnobTrim>(Vec(30, 124), module, RaBumpModule::TIME_PARAM));
        addInput(createInputCentered<RaPort>(Vec(90, 124), module, RaBumpModule::CLOCK_INPUT));

        // Screen centred in the push / in / step / block square
        BumpTimeDisplay *disp = createWidget<BumpTimeDisplay>(Vec(43.f, 68.f));
        disp->box.size = Vec(34.f, 34.f);
        disp->module = module;
        addChild(disp);

        // 8 vertical step buttons down the centre
        float y = 135.f;
        for (int i = 0; i < 8; i++) {
            BumpStepButton *b = new BumpStepButton;
            b->module = module;
            b->index = i;
            b->box.pos = Vec(60.f - 11.f, y - 11.f);
            b->box.size = Vec(22.f, 22.f);
            addChild(b);
            y += 28.f;
        }

        // Mode switch + output at the bottom, above the screws
        addParam(createParamCentered<RaSwitch2>(Vec(30, 345), module, RaBumpModule::MODE_PARAM));
        addOutput(createOutputCentered<RaPort>(Vec(90, 345), module, RaBumpModule::OUT1_OUTPUT));
    }
};

Model *modelRaBump = createModel<RaBumpModule, RaBumpWidget>("ra-bump");