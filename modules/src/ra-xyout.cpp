// ============================================================
// fname metadata — friendly panel label names
// Read by util/gen-panel.mjs for the rendered SVG label text.
// Overrides the configParam/Input/Output tooltip names.
// ============================================================
// fname: LERP_PARAM "LERP"
// fname: LERP_IN "LERP CV"
// fname: RANGE "RANGE"
// fname: X_OUTPUT "X"
// fname: Y_OUTPUT "Y"
#include "ra-components.hpp"
#include <atomic>

using namespace rack;

extern Plugin *pluginInstance;

struct RaXyoutModule : Module {
	enum ParamIds {
		LERP_PARAM,   // lerp speed 0..10
		RANGE,        // output range: 0–10V / ±5V / 0–1V
		NUM_PARAMS
	};
	enum InputIds {
		LERP_IN,      // lerp speed CV
		NUM_INPUTS
	};
	enum OutputIds {
		X_OUTPUT,
		Y_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	// Target XY position (0..1), set by clicking the pad on the UI thread
	std::atomic<float> targetX{0.5f};
	std::atomic<float> targetY{0.5f};

	// Current lerped position (audio thread)
	float curX = 0.5f;
	float curY = 0.5f;

	// Published to the UI thread for drawing
	std::atomic<float> displayX{0.5f};
	std::atomic<float> displayY{0.5f};
	std::atomic<float> displayTargetX{0.5f};
	std::atomic<float> displayTargetY{0.5f};

	RaXyoutModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(LERP_PARAM, 0.f, 10.f, 1.f, "Lerp", " speed");
		configSwitch(RANGE, 0.f, 2.f, 0.f, "Range", {"0–10V", "±5V", "0–1V"});
		configInput(LERP_IN, "Lerp speed CV");
		configOutput(X_OUTPUT, "X");
		configOutput(Y_OUTPUT, "Y");
	}

	void onReset() override {
		Module::onReset();
		curX = 0.5f;
		curY = 0.5f;
		targetX.store(0.5f, std::memory_order_relaxed);
		targetY.store(0.5f, std::memory_order_relaxed);
	}

	void process(const ProcessArgs &args) override {
		// Lerp speed — CV overrides the knob when connected, clamped 0..10
		float speed = inputs[LERP_IN].isConnected()
			? clamp(inputs[LERP_IN].getVoltage(), 0.f, 10.f)
			: params[LERP_PARAM].getValue();

		// Slew coefficient from speed (lambda = speed; speed 0 = instant)
		float coeff;
		if (speed <= 0.f)
			coeff = 1.f;
		else
			coeff = 1.f - std::exp(-speed * args.sampleTime);

		// Read the UI-set target
		float tx = clamp(targetX.load(std::memory_order_relaxed), 0.f, 1.f);
		float ty = clamp(targetY.load(std::memory_order_relaxed), 0.f, 1.f);

		// Lerp the current position toward the target
		curX += (tx - curX) * coeff;
		curY += (ty - curY) * coeff;

		// Map the normalized position (0..1) onto the selected output range
		int mode = (int)std::round(params[RANGE].getValue());
		float scale, offset;
		switch (mode) {
			default:
			case 0: scale = 10.f; offset = 0.f; break;    // 0–10V
			case 1: scale = 10.f; offset = -5.f; break;   // ±5V
			case 2: scale = 1.f;  offset = 0.f; break;    // 0–1V
		}
		outputs[X_OUTPUT].setVoltage(curX * scale + offset);
		outputs[Y_OUTPUT].setVoltage(curY * scale + offset);

		// Publish to the UI thread
		displayX.store(curX, std::memory_order_relaxed);
		displayY.store(curY, std::memory_order_relaxed);
		displayTargetX.store(tx, std::memory_order_relaxed);
		displayTargetY.store(ty, std::memory_order_relaxed);
	}
};

// Square XY pad: shows a grid, a target marker where the user clicked, and a
// dot tracking the lerped output position. Clicking (or dragging) sets the target.
struct XyPad : OpaqueWidget {
	RaXyoutModule *module;
	// Inner margin (rack units) around the pad's box where coordinates map
	float margin = 10.f;
	// Widget-local mouse position, tracked across drags via mouse deltas
	Vec dragPos;

	// Map a widget-local mouse position to normalized 0..1 coordinates
	Vec mouseToNorm(Vec p) {
		float span = box.size.x - 2.f * margin;
		if (span <= 0.f)
			return Vec(0.5f, 0.5f);
		return Vec(
			clamp((p.x - margin) / span, 0.f, 1.f),
			clamp((p.y - margin) / span, 0.f, 1.f));
	}

	void setFromMouse(Vec p) {
		Vec n = mouseToNorm(p);
		module->targetX.store(n.x, std::memory_order_relaxed);
		module->targetY.store(n.y, std::memory_order_relaxed);
	}

	void onButton(const event::Button &e) override {
		if (module && e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS) {
			dragPos = e.pos;
			setFromMouse(dragPos);
			e.consume(this);
		}
		OpaqueWidget::onButton(e);
	}

	void onDragMove(const event::DragMove &e) override {
		// Accumulate mouse deltas so the pad tracks the cursor while dragging
		dragPos += e.mouseDelta;
		if (module)
			setFromMouse(dragPos);
		OpaqueWidget::onDragMove(e);
	}

	void draw(const DrawArgs &args) override {
		// Backdrop
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 4.f);
		nvgFillColor(args.vg, nvgRGB(0x10, 0x10, 0x12));
		nvgFill(args.vg);
		nvgStrokeWidth(args.vg, 1.5f);
		nvgStrokeColor(args.vg, nvgRGB(0x4a, 0x40, 0x66));
		nvgStroke(args.vg);

		if (!module) return;

		float span = box.size.x - 2.f * margin;
		float minP = margin;
		float maxP = box.size.x - margin;

		// Faint grid
		nvgBeginPath(args.vg);
		nvgStrokeWidth(args.vg, 0.8f);
		nvgStrokeColor(args.vg, nvgRGB(0x26, 0x22, 0x38));
		for (int g = 1; g < 6; g++) {
			float t = minP + span * g / 6.f;
			nvgMoveTo(args.vg, t, minP);
			nvgLineTo(args.vg, t, maxP);
			nvgMoveTo(args.vg, minP, t);
			nvgLineTo(args.vg, maxP, t);
		}
		nvgStroke(args.vg);

		// Target marker (where the user clicked)
		float tx = clamp(module->displayTargetX.load(std::memory_order_relaxed), 0.f, 1.f);
		float ty = clamp(module->displayTargetY.load(std::memory_order_relaxed), 0.f, 1.f);
		float tpx = minP + tx * span;
		float tpy = minP + ty * span;
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, tpx, tpy, 5.f);
		nvgFillColor(args.vg, nvgRGB(0x4a, 0x40, 0x66));
		nvgFill(args.vg);
		nvgStrokeWidth(args.vg, 1.2f);
		nvgStrokeColor(args.vg, nvgRGB(0x99, 0x6d, 0xd2));
		nvgStroke(args.vg);

		// Current lerped position dot
		float cx = clamp(module->displayX.load(std::memory_order_relaxed), 0.f, 1.f);
		float cy = clamp(module->displayY.load(std::memory_order_relaxed), 0.f, 1.f);
		float px = minP + cx * span;
		float py = minP + cy * span;
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, px, py, 6.f);
		nvgFillColor(args.vg, nvgRGB(0x99, 0x6d, 0xd2));
		nvgFill(args.vg);
		nvgStrokeWidth(args.vg, 1.5f);
		nvgStrokeColor(args.vg, nvgRGB(0xe6, 0xd5, 0xff));
		nvgStroke(args.vg);
	}
};

struct RaXyoutWidget : ModuleWidget {
	RaXyoutWidget(RaXyoutModule *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ra-xyout.svg")));

		addChild(createWidget<RaScrew>(Vec(0, 0)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
		addChild(createWidget<RaScrew>(Vec(0, box.size.y - RACK_GRID_WIDTH)));
		addChild(createWidget<RaScrew>(Vec(box.size.x - RACK_GRID_WIDTH, box.size.y - RACK_GRID_WIDTH)));

		// ---- Square XY pad ----
		XyPad *pad = new XyPad;
		pad->box.pos = Vec(24, 50);
		pad->box.size = Vec(72, 72);
		pad->module = module;
		addChild(pad);

		// ---- Lerp speed (knob + cv) ----
		addParam(createParamCentered<RaKnobSmall>(Vec(38, 176), module, RaXyoutModule::LERP_PARAM));
		addInput(createInputCentered<RaPort>(Vec(82, 176), module, RaXyoutModule::LERP_IN));

		// ---- Output range switch ----
		addParam(createParamCentered<RaSwitch3>(Vec(60, 210), module, RaXyoutModule::RANGE));

		// ---- Outputs ----
		addOutput(createOutputCentered<RaPort>(Vec(38, 244), module, RaXyoutModule::X_OUTPUT));
		addOutput(createOutputCentered<RaPort>(Vec(82, 244), module, RaXyoutModule::Y_OUTPUT));
	}
};

Model *modelRaXyout = createModel<RaXyoutModule, RaXyoutWidget>("ra-xyout");