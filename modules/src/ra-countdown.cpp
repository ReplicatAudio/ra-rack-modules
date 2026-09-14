// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: COUNT_PARAM "Count"
// fname: COUNT_INPUT "C"
// fname: INCREMENT_PARAM "Inc"
// fname: INCREMENT_INPUT "Inc"
// fname: DECREMENT_PARAM "Dec"
// fname: DECREMENT_INPUT "Dec"
// fname: MODE_PARAM "Mode"
// fname: AUTO_PARAM "Auto"
// fname: ZERO_PARAM "Zero lvl"
// fname: ZERO_INPUT "Z"
// fname: MAX_OUT_PARAM "Max lvl"
// fname: MAX_OUT_INPUT "Mx"
// fname: ZERO_BUTTON_PARAM "Zero"
// fname: ZERO_TRIG_INPUT "0"
// fname: MAX_BUTTON_PARAM "Max"
// fname: MAX_TRIG_INPUT "10"
// fname: OUT1_OUTPUT "Out"
#include "ra-components.hpp"
#include <atomic>

using namespace rack;

extern Plugin *pluginInstance;

struct RaCountdownModule : Module {
	enum ParamIds {
		COUNT_PARAM,
		INCREMENT_PARAM,
		DECREMENT_PARAM,
		MODE_PARAM,
		AUTO_PARAM,
		ZERO_PARAM,
		MAX_OUT_PARAM,
		ZERO_BUTTON_PARAM,
		MAX_BUTTON_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		COUNT_INPUT,
		INCREMENT_INPUT,
		DECREMENT_INPUT,
		ZERO_INPUT,
		MAX_OUT_INPUT,
		ZERO_TRIG_INPUT,
		MAX_TRIG_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT1_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	// Running countdown accumulator (0..count). When this reaches `count`,
	// the output fires.
	int acc = 0;
	// Gate-held flag: in gate mode the output stays high from fire until reset.
	bool gateHeld = false;
	// Guards against re-firing every frame while acc stays at the threshold.
	bool fired = false;
	std::atomic<int> displayAcc{0};

	dsp::SchmittTrigger incrementTrig;
	dsp::SchmittTrigger decrementTrig;
	dsp::SchmittTrigger zeroTrig;
	dsp::SchmittTrigger maxTrig;
	dsp::SchmittTrigger incrementBtnTrig;
	dsp::SchmittTrigger decrementBtnTrig;
	dsp::SchmittTrigger zeroBtnTrig;
	dsp::SchmittTrigger maxBtnTrig;
	dsp::PulseGenerator outputPulse;

	RaCountdownModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(COUNT_PARAM, 0.f, 256.f, 8.f, "Count", "", 0.f, 1.f);
		configButton(INCREMENT_PARAM, "Increment");
		configButton(DECREMENT_PARAM, "Decrement");
		configSwitch(MODE_PARAM, 0.f, 1.f, 0.f, "Output mode", {"Trig", "Gate"});
		configSwitch(AUTO_PARAM, 0.f, 1.f, 0.f, "Auto reset", {"Off", "Auto"});
		configParam(ZERO_PARAM, -10.f, 10.f, 0.f, "Zero output", " V");
		configParam(MAX_OUT_PARAM, -10.f, 10.f, 10.f, "Max output", " V");
		configButton(ZERO_BUTTON_PARAM, "Zero");
		configButton(MAX_BUTTON_PARAM, "Max");

		configInput(COUNT_INPUT, "Count CV");
		configInput(INCREMENT_INPUT, "Increment trig");
		configInput(DECREMENT_INPUT, "Decrement trig");
		configInput(ZERO_INPUT, "Zero output CV");
		configInput(MAX_OUT_INPUT, "Max output CV");
		configInput(ZERO_TRIG_INPUT, "Zero trig");
		configInput(MAX_TRIG_INPUT, "Max trig");

		configOutput(OUT1_OUTPUT, "Output");
		onReset();
	}

	void onReset() override {
		acc = 0;
		gateHeld = false;
		fired = false;
	}

	void process(const ProcessArgs &args) override {
		// Quantized count target (0-256)
		float countVal = inputs[COUNT_INPUT].isConnected()
			? inputs[COUNT_INPUT].getVoltage()
			: params[COUNT_PARAM].getValue();
		countVal = clamp(countVal, 0.f, 256.f);
		int count = (int)std::round(countVal);

		// Output voltages (zero when low, max when fired/high)
		float zeroV = inputs[ZERO_INPUT].isConnected()
			? inputs[ZERO_INPUT].getVoltage()
			: params[ZERO_PARAM].getValue();
		float maxV = inputs[MAX_OUT_INPUT].isConnected()
			? inputs[MAX_OUT_INPUT].getVoltage()
			: params[MAX_OUT_PARAM].getValue();

		bool incr = incrementTrig.process(inputs[INCREMENT_INPUT].getVoltage())
			| incrementBtnTrig.process(params[INCREMENT_PARAM].getValue());
		bool decr = decrementTrig.process(inputs[DECREMENT_INPUT].getVoltage())
			| decrementBtnTrig.process(params[DECREMENT_PARAM].getValue());
		bool zero = zeroTrig.process(inputs[ZERO_TRIG_INPUT].getVoltage())
			| zeroBtnTrig.process(params[ZERO_BUTTON_PARAM].getValue());
		bool max = maxTrig.process(inputs[MAX_TRIG_INPUT].getVoltage())
			| maxBtnTrig.process(params[MAX_BUTTON_PARAM].getValue());

		if (zero) {
			// Zero override: force output to zero and restart the countdown
			acc = 0;
			fired = false;
			gateHeld = false;
			outputPulse.reset();
		} else {
			// Max override: set the count straight to the count target, which
			// immediately satisfies the threshold and fires the output.
			if (max) acc = count;

			if (incr) acc++;
			if (decr) acc = acc > 0 ? acc - 1 : 0;

			// Fire when the accumulator reaches the count threshold
			if (count > 0 && acc >= count) {
				if (!fired) {
					fired = true;
					gateHeld = true;
					outputPulse.trigger(1e-3f);
				}
				// Auto reset: restart the countdown immediately after firing.
				// Gate output is still held until a zero override.
				if (params[AUTO_PARAM].getValue() > 0.5f)
					acc = 0;
				else
					acc = count; // stay armed at the threshold until reset
			} else {
				fired = false;
			}
		}

		// Output: gate mode holds high until zero; trig mode is a short pulse
		int mode = (int)std::round(params[MODE_PARAM].getValue());
		bool high = (mode == 1) ? gateHeld : outputPulse.isHigh();
		float out = high ? maxV : zeroV;
		outputs[OUT1_OUTPUT].setVoltage(clamp(out, -10.f, 10.f));

		displayAcc.store(acc, std::memory_order_relaxed);
	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "acc", json_integer(acc));
		json_object_set_new(rootJ, "gateHeld", json_boolean(gateHeld));
		json_object_set_new(rootJ, "fired", json_boolean(fired));
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		json_t *a = json_object_get(rootJ, "acc");
		if (a) acc = clamp(json_integer_value(a), 0, INT_MAX);
		json_t *g = json_object_get(rootJ, "gateHeld");
		if (g) gateHeld = json_boolean_value(g);
		json_t *f = json_object_get(rootJ, "fired");
		if (f) fired = json_boolean_value(f);
	}
};

// Screen showing the current countdown value
struct CountdownDisplay : LedDisplay {
	RaCountdownModule *module;
	std::shared_ptr<Font> font;

	CountdownDisplay() {
		font = APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
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

		if (!module || !font) return;

		int v = module->displayAcc.load(std::memory_order_relaxed);

		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, 16);
		nvgFillColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

		char buf[16];
		snprintf(buf, sizeof(buf), "%d", v);
		nvgText(args.vg, box.size.x / 2, box.size.y / 2, buf, NULL);
	}
};

struct RaCountdownWidget : ModuleWidget {
	RaCountdownWidget(RaCountdownModule *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-countdown.svg")));

		addChild(createWidget<RaScrew>(Vec(0, 0)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
		addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

		float leftX = 30;
		float rightX = 90;
		float centerX = box.size.x / 2;

		// Screen at top
		CountdownDisplay *display = createWidget<CountdownDisplay>(Vec(30, 12));
		display->box.size = Vec(60, 32);
		display->module = module;
		addChild(display);

		float ys[] = {60, 92, 124, 156, 188, 220, 252, 284, 316};

		// Count knob + count CV
		addParam(createParamCentered<RaKnob>(Vec(leftX, ys[0]), module, RaCountdownModule::COUNT_PARAM));
		addInput(createInputCentered<RaPort>(Vec(rightX, ys[0]), module, RaCountdownModule::COUNT_INPUT));

		// Increment button + increment trig
		addParam(createParamCentered<RaButton>(Vec(leftX, ys[1]), module, RaCountdownModule::INCREMENT_PARAM));
		addInput(createInputCentered<RaPort>(Vec(rightX, ys[1]), module, RaCountdownModule::INCREMENT_INPUT));

		// Decrement button + decrement trig
		addParam(createParamCentered<RaButton>(Vec(leftX, ys[2]), module, RaCountdownModule::DECREMENT_PARAM));
		addInput(createInputCentered<RaPort>(Vec(rightX, ys[2]), module, RaCountdownModule::DECREMENT_INPUT));

		// Zero button + zero trig
		addParam(createParamCentered<RaButton>(Vec(leftX, ys[3]), module, RaCountdownModule::ZERO_BUTTON_PARAM));
		addInput(createInputCentered<RaPort>(Vec(rightX, ys[3]), module, RaCountdownModule::ZERO_TRIG_INPUT));

		// Max button + max trig
		addParam(createParamCentered<RaButton>(Vec(leftX, ys[4]), module, RaCountdownModule::MAX_BUTTON_PARAM));
		addInput(createInputCentered<RaPort>(Vec(rightX, ys[4]), module, RaCountdownModule::MAX_TRIG_INPUT));

		// Output mode switch
		addParam(createParamCentered<RaSwitch2>(Vec(leftX, ys[5]), module, RaCountdownModule::MODE_PARAM));

		// Auto reset switch
		addParam(createParamCentered<RaSwitch2>(Vec(leftX, ys[6]), module, RaCountdownModule::AUTO_PARAM));

		// Zero output level knob + zero CV
		addParam(createParamCentered<RaKnobTrim>(Vec(leftX, ys[7]), module, RaCountdownModule::ZERO_PARAM));
		addInput(createInputCentered<RaPort>(Vec(rightX, ys[7]), module, RaCountdownModule::ZERO_INPUT));

		// Max output level knob + max CV
		addParam(createParamCentered<RaKnobTrim>(Vec(leftX, ys[8]), module, RaCountdownModule::MAX_OUT_PARAM));
		addInput(createInputCentered<RaPort>(Vec(rightX, ys[8]), module, RaCountdownModule::MAX_OUT_INPUT));

		// Output jack at bottom center
		addOutput(createOutputCentered<RaPort>(Vec(centerX, 348), module, RaCountdownModule::OUT1_OUTPUT));
	}
};

Model *modelRaCountdown = createModel<RaCountdownModule, RaCountdownWidget>("ra-countdown");