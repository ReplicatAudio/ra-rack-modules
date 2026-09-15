// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: A_PARAM "A"
// fname: B_PARAM "B"
// fname: C_PARAM "C"
// fname: D_PARAM "D"
// fname: T_PARAM "T"
// fname: A_INPUT "A CV"
// fname: B_INPUT "B CV"
// fname: C_INPUT "C CV"
// fname: D_INPUT "D CV"
// fname: T_INPUT "T CV"
// fname: X_INPUT "X"
// fname: Y_INPUT "Y"
// fname: A_TRIG "A tr"
// fname: B_TRIG "B tr"
// fname: C_TRIG "C tr"
// fname: D_TRIG "D tr"
// fname: OUTPUT_MODE "OUT"
// fname: LERP_MODE "MODE"
// fname: CV_OUTPUT "CV"
#include "ra-components.hpp"
#include <atomic>
#include <cstring>

using namespace rack;

extern Plugin *pluginInstance;

// Index of the OUTPUT_MODE param in RaLerperModule (must match the enum),
// used by LerperValueQuantity to scale knob tooltips to the output range.
static constexpr int LERPER_OUTPUT_MODE_PARAM = 9;

// Value-knob quantity: the hover tooltip shows the knob value scaled to the
// currently selected output mode (0–10V, ±5V, or 0–1V).
struct LerperValueQuantity : ParamQuantity {
	float getDisplayValue() override {
		float v = getValue();
		if (!module)
			return v;
		int mode = (int)std::round(module->params[LERPER_OUTPUT_MODE_PARAM].getValue());
		float scale, offset;
		switch (mode) {
			default:
			case 0: scale = 0.5f; offset = 5.f; break;   // 0–10V
			case 1: scale = 0.5f; offset = 0.f; break;   // ±5V
			case 2: scale = 0.05f; offset = 0.5f; break; // 0–1V
		}
		return v * scale + offset;
	}
};

struct RaLerperModule : Module {
	enum ParamIds {
		A_PARAM,
		B_PARAM,
		C_PARAM,
		D_PARAM,
		T_PARAM,
		A_TRIG,   // trigger buttons
		B_TRIG,
		C_TRIG,
		D_TRIG,
		OUTPUT_MODE,
		LERP_MODE,
		NUM_PARAMS
	};
	enum InputIds {
		A_INPUT,  // value CV
		B_INPUT,
		C_INPUT,
		D_INPUT,
		T_INPUT,  // time/speed CV
		X_INPUT,  // XY interpolation position CVs
		Y_INPUT,
		A_TRIG_IN,  // trigger CV inputs
		B_TRIG_IN,
		C_TRIG_IN,
		D_TRIG_IN,
		NUM_INPUTS
	};
	enum OutputIds {
		CV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		A_LIGHT,
		B_LIGHT,
		C_LIGHT,
		D_LIGHT,
		NUM_LIGHTS
	};

	// Corner order: 0=a(top-left), 1=b(top-right), 2=c(bottom-left), 3=d(bottom-right)
	static constexpr float cornerX(int i) { return (i == 1 || i == 3) ? 1.f : 0.f; }
	static constexpr float cornerY(int i) { return (i == 2 || i == 3) ? 1.f : 0.f; }

	int activeTarget = 0;
	float currentValue = 0.f;   // running output
	float dotX = 0.f;           // dot position on screen, normalized 0..1
	float dotY = 0.f;

	std::atomic<float> displayValue{0.f};
	std::atomic<float> displayDotX{0.f};
	std::atomic<float> displayDotY{0.f};
	std::atomic<int> displayTarget{0};
	std::atomic<float> displayXyX{0.5f};
	std::atomic<float> displayXyY{0.5f};
	std::atomic<bool> displayXyActive{false};

	dsp::SchmittTrigger btnTriggers[4];
	dsp::SchmittTrigger inTriggers[4];

	RaLerperModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam<LerperValueQuantity>(A_PARAM, -10.f, 10.f, 0.f, "A", " V");
		configParam<LerperValueQuantity>(B_PARAM, -10.f, 10.f, 0.f, "B", " V");
		configParam<LerperValueQuantity>(C_PARAM, -10.f, 10.f, 0.f, "C", " V");
		configParam<LerperValueQuantity>(D_PARAM, -10.f, 10.f, 0.f, "D", " V");
		configParam(T_PARAM, 0.f, 10.f, 1.f, "Time", " s");

		configButton(A_TRIG, "Lerp to A");
		configButton(B_TRIG, "Lerp to B");
		configButton(C_TRIG, "Lerp to C");
		configButton(D_TRIG, "Lerp to D");

		configInput(A_INPUT, "A CV");
		configInput(B_INPUT, "B CV");
		configInput(C_INPUT, "C CV");
		configInput(D_INPUT, "D CV");
		configInput(T_INPUT, "Time CV");
		configInput(X_INPUT, "X CV");
		configInput(Y_INPUT, "Y CV");
		configInput(A_TRIG_IN, "A trigger");
		configInput(B_TRIG_IN, "B trigger");
		configInput(C_TRIG_IN, "C trigger");
		configInput(D_TRIG_IN, "D trigger");

		configSwitch(OUTPUT_MODE, 0.f, 2.f, 0.f, "Output mode", {"0–10V", "±5V", "0–1V"});
		configSwitch(LERP_MODE, 0.f, 1.f, 0.f, "Lerp mode", {"target", "mix"});

		configOutput(CV_OUTPUT, "CV");
	}

	void onReset() override {
		Module::onReset();
		activeTarget = 0;
		currentValue = 0.f;
		dotX = 0.f;
		dotY = 0.f;
	}

	void process(const ProcessArgs &args) override {
		// Target values a b c d (CV overrides knob when connected)
		float targets[4];
		for (int i = 0; i < 4; i++)
			targets[i] = inputs[A_INPUT + i].isConnected()
				? inputs[A_INPUT + i].getVoltage()
				: params[A_PARAM + i].getValue();

		// Time / speed — CV overrides knob when connected, clamped to 0..10s
		float time = inputs[T_INPUT].isConnected()
			? clamp(inputs[T_INPUT].getVoltage(), 0.f, 10.f)
			: params[T_PARAM].getValue();

		// Detect triggers from buttons and CV inputs
		bool fired[4];
		for (int i = 0; i < 4; i++)
			fired[i] = btnTriggers[i].process(params[A_TRIG + i].getValue())
				|| inTriggers[i].process(inputs[A_TRIG_IN + i].getVoltage());

		for (int i = 0; i < 4; i++)
			if (fired[i])
				activeTarget = i;

		// XY-interpolated target: active whenever either X or Y CV is patched.
		// Its position on the matrix maps the CVs (0..10V) onto u/v in 0..1.
		bool xyActive = inputs[X_INPUT].isConnected() || inputs[Y_INPUT].isConnected();
		float xyX = inputs[X_INPUT].isConnected()
			? clamp(inputs[X_INPUT].getVoltage() / 10.f, 0.f, 1.f)
			: 0.5f;
		float xyY = inputs[Y_INPUT].isConnected()
			? clamp(inputs[Y_INPUT].getVoltage() / 10.f, 0.f, 1.f)
			: 0.5f;

		// Lerp coefficient from time
		float coeff;
		if (time <= 0.f)
			coeff = 1.f;
		else {
			float lambda = 1.f / time;
			coeff = 1.f - std::exp(-lambda * args.sampleTime);
		}

		// Lerp mode: target = slew the output value toward the target; mix = slew
		// the mix weights (the dot position) and mix the live inputs each sample
		// using those weights, without filtering the signal itself.
		int lerpMode = (int)std::round(params[LERP_MODE].getValue());

		// Choose the target position: the XY position when active, otherwise the
		// selected corner. In target mode the output value slews toward the value
		// at this target; in mix mode the mix weights slew toward this position.
		float targetDotX, targetDotY;
		if (xyActive) {
			targetDotX = xyX;
			targetDotY = xyY;
		} else {
			targetDotX = cornerX(activeTarget);
			targetDotY = cornerY(activeTarget);
		}

		// Lerp the screen dot / mix weights toward the target position
		dotX += (targetDotX - dotX) * coeff;
		dotY += (targetDotY - dotY) * coeff;

		float out;
		if (lerpMode != 0) {
			// Mix the live inputs using the slewed weights (u,v). The lerp only
			// fades the mix weights; the audio inputs are mixed unprocessed.
			float u = dotX, v = dotY;
			out = targets[0] * (1 - u) * (1 - v)
				+ targets[1] * u * (1 - v)
				+ targets[2] * (1 - u) * v
				+ targets[3] * u * v;
			currentValue = out;
		} else {
			// Target value at the current (unslewed) target position
			float targetValue;
			if (xyActive) {
				float u = xyX, v = xyY;
				targetValue = targets[0] * (1 - u) * (1 - v)
					+ targets[1] * u * (1 - v)
					+ targets[2] * (1 - u) * v
					+ targets[3] * u * v;
			} else {
				targetValue = targets[activeTarget];
			}
			// Lerp the output value toward the active target
			currentValue += (targetValue - currentValue) * coeff;
			out = currentValue;
		}

		// Apply the output range mode (maps the internal ±10V onto the chosen range)
		int mode = (int)std::round(params[OUTPUT_MODE].getValue());
		float outScale, outOffset;
		switch (mode) {
			default:
			case 0: outScale = 0.5f; outOffset = 5.f; break;   // 0–10V
			case 1: outScale = 0.5f; outOffset = 0.f; break;   // ±5V
			case 2: outScale = 0.05f; outOffset = 0.5f; break; // 0–1V
		}
		out = out * outScale + outOffset;
		outputs[CV_OUTPUT].setVoltage(out);

		// Publish to UI thread. displayTarget 4 marks the active XY target.
		displayValue.store(out, std::memory_order_relaxed);
		displayDotX.store(dotX, std::memory_order_relaxed);
		displayDotY.store(dotY, std::memory_order_relaxed);
		displayTarget.store(xyActive ? 4 : activeTarget, std::memory_order_relaxed);
		displayXyX.store(xyX, std::memory_order_relaxed);
		displayXyY.store(xyY, std::memory_order_relaxed);
		displayXyActive.store(xyActive, std::memory_order_relaxed);

		// Active target button lit, others off; no corner lit when XY is active
		for (int i = 0; i < 4; i++)
			lights[A_LIGHT + i].setBrightnessSmooth(!xyActive && i == activeTarget ? 1.f : 0.f, args.sampleTime);
	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "activeTarget", json_integer(activeTarget));
		json_object_set_new(rootJ, "currentValue", json_real(currentValue));
		json_object_set_new(rootJ, "dotX", json_real(dotX));
		json_object_set_new(rootJ, "dotY", json_real(dotY));
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		json_t *at = json_object_get(rootJ, "activeTarget");
		if (at) activeTarget = clamp(json_integer_value(at), 0, 3);
		json_t *cv = json_object_get(rootJ, "currentValue");
		if (cv) currentValue = json_real_value(cv);
		json_t *dx = json_object_get(rootJ, "dotX");
		if (dx) dotX = json_real_value(dx);
		json_t *dy = json_object_get(rootJ, "dotY");
		if (dy) dotY = json_real_value(dy);
	}
};

// Square screen: a corner point for each target (a b c d) and a dot
// showing the current output position, with the active target highlighted.
struct LerpDisplay : Widget {
	RaLerperModule *module;

	void draw(const DrawArgs &args) override {
		// Backdrop — painted slightly larger than the box to cover the SVG bezel
		// outline, recolored with a muted purple border to match the accent
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, -3, -3, box.size.x + 6, box.size.y + 6, 4);
		nvgFillColor(args.vg, nvgRGB(0x10, 0x10, 0x10));
		nvgFill(args.vg);
		nvgStrokeWidth(args.vg, 1.5f);
		nvgStrokeColor(args.vg, nvgRGB(0x4a, 0x40, 0x66));
		nvgStroke(args.vg);

		if (!module) return;

		float dotX = clamp(module->displayDotX.load(std::memory_order_relaxed), 0.f, 1.f);
		float dotY = clamp(module->displayDotY.load(std::memory_order_relaxed), 0.f, 1.f);
		int target = module->displayTarget.load(std::memory_order_relaxed);
		bool xyActive = module->displayXyActive.load(std::memory_order_relaxed);

		// Corner positions mapped from corner index to screen geometry
		const float m = 14.f;
		const float cx[4] = {m, box.size.x - m, m, box.size.x - m};
		const float cy[4] = {m, m, box.size.y - m, box.size.y - m};
		const char *names[4] = {"A", "B", "C", "D"};

		// Draw thin inner frame of the square
		nvgBeginPath(args.vg);
		nvgRect(args.vg, m - 4, m - 4, box.size.x - 2 * (m - 4), box.size.y - 2 * (m - 4));
		nvgStrokeWidth(args.vg, 1.f);
		nvgStrokeColor(args.vg, nvgRGB(0x33, 0x2c, 0x4a));
		nvgStroke(args.vg);

		// Faint interpolation grid (XY matrix)
		nvgBeginPath(args.vg);
		nvgStrokeWidth(args.vg, 0.8f);
		nvgStrokeColor(args.vg, nvgRGB(0x26, 0x22, 0x38));
		const float gridMin = m - 4, gridMax = box.size.x - (m - 4);
		for (int g = 1; g < 6; g++) {
			float t = gridMin + (gridMax - gridMin) * g / 6.f;
			nvgMoveTo(args.vg, t, gridMin);
			nvgLineTo(args.vg, t, gridMax);
			nvgMoveTo(args.vg, gridMin, t);
			nvgLineTo(args.vg, gridMax, t);
		}
		nvgStroke(args.vg);

		// Corner points + labels, highlight the active corner (none when XY active)
		for (int i = 0; i < 4; i++) {
			bool active = (!xyActive && i == target);
			// Corner point
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, cx[i], cy[i], active ? 5.f : 3.5f);
			nvgFillColor(args.vg, active ? nvgRGB(0xd2, 0xa9, 0xff) : nvgRGB(0x55, 0x4a, 0x77));
			nvgFill(args.vg);
			// Label
			nvgFontFaceId(args.vg, APP->window->uiFont->handle);
			nvgFontSize(args.vg, active ? 13.f : 11.f);
			nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgFillColor(args.vg, active ? nvgRGB(0xe6, 0xd5, 0xff) : nvgRGB(0x55, 0x4a, 0x77));
			nvgText(args.vg, cx[i], cy[i] + (i < 2 ? 18.f : -18.f), names[i], NULL);
		}

		// The XY-interpolated target: a lighter dot when either X or Y is set
		if (xyActive) {
			float xyX = clamp(module->displayXyX.load(std::memory_order_relaxed), 0.f, 1.f);
			float xyY = clamp(module->displayXyY.load(std::memory_order_relaxed), 0.f, 1.f);
			float xpx = m + xyX * (box.size.x - 2 * m);
			float xpy = m + xyY * (box.size.y - 2 * m);
			// Lighter target dot
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, xpx, xpy, 7.f);
			nvgFillColor(args.vg, nvgRGB(0xd0, 0xb6, 0xff));
			nvgFill(args.vg);
			nvgStrokeWidth(args.vg, 1.5f);
			nvgStrokeColor(args.vg, nvgRGB(0xf2, 0xea, 0xff));
			nvgStroke(args.vg);
			// Label
			nvgFontFaceId(args.vg, APP->window->uiFont->handle);
			nvgFontSize(args.vg, 10.f);
			nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgFillColor(args.vg, nvgRGB(0xee, 0xe4, 0xff));
			nvgText(args.vg, xpx, xpy, "XY", NULL);
		}

		// Dot representing the current output position
		float px = m + dotX * (box.size.x - 2 * m);
		float py = m + dotY * (box.size.y - 2 * m);
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, px, py, 6.f);
		nvgFillColor(args.vg, nvgRGB(0x99, 0x6d, 0xd2));
		nvgFill(args.vg);
		nvgStrokeWidth(args.vg, 1.5f);
		nvgStrokeColor(args.vg, nvgRGB(0xe6, 0xd5, 0xff));
		nvgStroke(args.vg);
	}
};

struct RaLerperWidget : ModuleWidget {
	RaLerperWidget(RaLerperModule *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-lerper.svg")));

		addChild(createWidget<RaScrew>(Vec(0, 0)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
		addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

		// ---- Corner target values (knob + cv) ----
		// a: top-left, b: top-right, c: bottom-left, d: bottom-right
		addParam(createParamCentered<RaKnobSmall>(Vec(36, 60), module, RaLerperModule::A_PARAM));
		addInput(createInputCentered<RaPort>(Vec(80, 60), module, RaLerperModule::A_INPUT));

		addParam(createParamCentered<RaKnobSmall>(Vec(204, 60), module, RaLerperModule::B_PARAM));
		addInput(createInputCentered<RaPort>(Vec(160, 60), module, RaLerperModule::B_INPUT));

		addParam(createParamCentered<RaKnobSmall>(Vec(36, 305), module, RaLerperModule::C_PARAM));
		addInput(createInputCentered<RaPort>(Vec(80, 305), module, RaLerperModule::C_INPUT));

		addParam(createParamCentered<RaKnobSmall>(Vec(204, 305), module, RaLerperModule::D_PARAM));
		addInput(createInputCentered<RaPort>(Vec(160, 305), module, RaLerperModule::D_INPUT));

		// ---- Time / speed (knob + cv) ----
		addParam(createParamCentered<RaKnobSmall>(Vec(120, 88), module, RaLerperModule::T_PARAM));
		addInput(createInputCentered<RaPort>(Vec(120, 48), module, RaLerperModule::T_INPUT));

		// ---- Square screen ----
		LerpDisplay *display = new LerpDisplay();
		display->box.pos = Vec(72, 142);
		display->box.size = Vec(96, 96);
		display->module = module;
		addChild(display);

		// ---- Corner trigger buttons + trigger CV inputs ----
		// Each sits on its matching screen corner:
		// a=top-left, b=top-right, c=bottom-left, d=bottom-right
		addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(56, 142), module,
			RaLerperModule::A_TRIG, RaLerperModule::A_LIGHT));
		addInput(createInputCentered<RaPort>(Vec(56, 116), module, RaLerperModule::A_TRIG_IN));

		addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(184, 142), module,
			RaLerperModule::B_TRIG, RaLerperModule::B_LIGHT));
		addInput(createInputCentered<RaPort>(Vec(184, 116), module, RaLerperModule::B_TRIG_IN));

		addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(56, 238), module,
			RaLerperModule::C_TRIG, RaLerperModule::C_LIGHT));
		addInput(createInputCentered<RaPort>(Vec(56, 264), module, RaLerperModule::C_TRIG_IN));

		addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(184, 238), module,
			RaLerperModule::D_TRIG, RaLerperModule::D_LIGHT));
		addInput(createInputCentered<RaPort>(Vec(184, 264), module, RaLerperModule::D_TRIG_IN));

		// ---- XY interpolation position (cv) - active when either is set ----
		addInput(createInputCentered<RaPort>(Vec(120, 292), module, RaLerperModule::X_INPUT));
		addInput(createInputCentered<RaPort>(Vec(120, 320), module, RaLerperModule::Y_INPUT));

		// ---- Output + mode switches ----
		addOutput(createOutputCentered<RaPort>(Vec(120, 348), module, RaLerperModule::CV_OUTPUT));
		addParam(createParamCentered<RaSwitch3>(Vec(196, 348), module, RaLerperModule::OUTPUT_MODE));
		addParam(createParamCentered<RaSwitch2>(Vec(44, 348), module, RaLerperModule::LERP_MODE));
	}
};

Model *modelRaLerper = createModel<RaLerperModule, RaLerperWidget>("ra-lerper");